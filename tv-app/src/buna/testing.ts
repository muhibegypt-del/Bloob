// Test doubles for Buna. Not imported by the app.
import type { Scheduler } from './scheduler';
import {
  fail, ok,
  type BackendCapabilities, type BackendEvent, type BackendSnapshot, type BunaBackend,
  type CommandResult, type ResolvedSource, type Track,
} from './types';

/** Manual clock: timers fire only when the test advances time. */
export class ManualScheduler implements Scheduler {
  time = 0;
  private nextId = 1;
  private timers = new Map<number, { at: number; fn: () => void; every?: number }>();

  now() { return this.time; }
  setTimeout(fn: () => void, ms: number) { const id = this.nextId++; this.timers.set(id, { at: this.time + ms, fn }); return id; }
  setInterval(fn: () => void, ms: number) { const id = this.nextId++; this.timers.set(id, { at: this.time + ms, fn, every: ms }); return id; }
  clearTimeout(id: number | undefined) { if (id !== undefined) this.timers.delete(id); }
  clearInterval(id: number | undefined) { this.clearTimeout(id); }
  get activeTimers() { return this.timers.size; }

  advance(ms: number) {
    const end = this.time + ms;
    for (;;) {
      let nextId = -1;
      let next: { at: number; fn: () => void; every?: number } | undefined;
      for (const [id, timer] of this.timers) if (timer.at <= end && (!next || timer.at < next.at)) { next = timer; nextId = id; }
      if (!next) break;
      this.time = next.at;
      if (next.every) next.at += next.every; else this.timers.delete(nextId);
      next.fn();
    }
    this.time = end;
  }
}

/** Scriptable backend that behaves like a well-mannered media element unless told otherwise. */
export class FakeBackend implements BunaBackend {
  readonly kind = 'fake';
  snap: BackendSnapshot = { currentTime: 0, duration: NaN, seekable: null, bufferedEnd: 0, paused: true, ended: false, seeking: false, readyState: 0 };
  audio: Track[] = [];
  text: Track[] = [];
  calls: string[] = [];
  playResult: CommandResult = ok;
  disposed = false;
  prepared: { source: ResolvedSource; start: number } | null = null;
  private listeners = new Set<(event: BackendEvent) => void>();

  async prepare(source: ResolvedSource, start = 0) { this.calls.push(`prepare:${start}`); this.prepared = { source, start }; return ok; }
  async play() { this.calls.push('play'); if (this.playResult.ok) this.snap.paused = false; return this.playResult; }
  pause() { this.calls.push('pause'); this.snap.paused = true; return ok; }
  seek(seconds: number) {
    this.calls.push(`seek:${seconds}`);
    if (!this.snap.seekable) return fail('unsupported', 'not seekable');
    this.snap.seeking = true;
    return ok;
  }
  selectAudio(id: string) { this.calls.push(`audio:${id}`); this.audio = this.audio.map(t => ({ ...t, selected: t.id === id })); return ok; }
  selectSubtitle(id: string | null) { this.calls.push(`sub:${id}`); this.text = this.text.map(t => ({ ...t, selected: t.id === id })); return ok; }
  snapshot() { return { ...this.snap }; }
  audioTracks() { return this.audio.map(t => ({ ...t })); }
  textTracks() { return this.text.map(t => ({ ...t })); }
  capabilities(): BackendCapabilities {
    const cap = (supported: boolean) => ({ supported, tv: 'unverified' as const, reason: supported ? undefined : 'no' });
    return {
      seek: cap(!!this.snap.seekable), audioTrackSelection: cap(this.audio.length > 1), textTrackSelection: cap(this.text.length > 0),
      externalSubtitles: cap(false), thumbnails: cap(false), frameRateMatching: cap(false), outputInfo: cap(false),
    };
  }
  subscribe(listener: (event: BackendEvent) => void) { this.listeners.add(listener); return () => { this.listeners.delete(listener); }; }
  stop() { this.calls.push('stop'); }
  dispose() { this.calls.push('dispose'); this.disposed = true; this.listeners.clear(); }

  get listenerCount() { return this.listeners.size; }
  emit(event: BackendEvent) { this.listeners.forEach(listener => listener(event)); }

  /** Simulate metadata for a seekable file of `duration` seconds. */
  loadMetadata(duration: number) {
    Object.assign(this.snap, { duration, seekable: { start: 0, end: duration }, readyState: 1 });
    this.emit({ type: 'metadata' });
  }
  startPlaying() {
    Object.assign(this.snap, { paused: false, readyState: 4 });
    this.emit({ type: 'playing' });
    this.emit({ type: 'first-frame' });
  }
  /** Finish the in-flight seek at `landed`. */
  finishSeek(landed: number, emitEvent = true) {
    Object.assign(this.snap, { currentTime: landed, seeking: false, readyState: 4 });
    if (emitEvent) this.emit({ type: 'seeked' });
  }
}

export const testSource = (overrides: Partial<ResolvedSource> = {}): ResolvedSource => ({
  url: 'https://media.invalid/video.mkv', transport: 'progressive', claims: {}, resolvedAt: Date.now(), ...overrides,
});
