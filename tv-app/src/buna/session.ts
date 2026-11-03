import { metrics } from './metrics';
import type { PositionStore } from './positions';
import { browserScheduler, type Scheduler } from './scheduler';
import {
  fail, ok,
  type BackendEvent, type BunaBackend, type BunaError, type CommandResult,
  type PlaybackPhase, type PlaybackState, type PlayIntent, type ResolvedSource,
} from './types';

export interface SessionConfig {
  /** Media clock sampling while the clock can move. Stops while paused. */
  clockIntervalMs: number;
  /** Quiet period after the last seek nudge before the seek is sent. */
  seekDebounceMs: number;
  /** Give up waiting for a seek acknowledgement and fall back to the media clock. */
  seekTimeoutMs: number;
  /** Fail startup when no metadata arrives in this time. */
  startupTimeoutMs: number;
  /** Treat a clock that has not advanced for this long while playing as a stall. */
  stallAfterMs: number;
  /** Persist the resume position at most this often during playback. */
  positionSaveMs: number;
}

export const DEFAULT_CONFIG: SessionConfig = {
  clockIntervalMs: 250,
  seekDebounceMs: 600,
  seekTimeoutMs: 15000,
  startupTimeoutMs: 45000,
  stallAfterMs: 3000,
  positionSaveMs: 15000,
};

/** Seek distance grows while the user keeps pressing (or holds) one direction. */
export function nudgeStep(burst: number): number {
  return burst >= 16 ? 120 : burst >= 8 ? 60 : burst >= 4 ? 30 : 10;
}

export interface SessionOptions {
  backend: BunaBackend;
  source: ResolvedSource;
  intent: PlayIntent;
  scheduler?: Scheduler;
  positions?: PositionStore;
  config?: Partial<SessionConfig>;
}

const CLOCK_PHASES: PlaybackPhase[] = ['loading', 'buffering', 'playing', 'seeking'];
const finite = (value: number) => Number.isFinite(value);
const sameJson = (a: unknown, b: unknown) => a === b || JSON.stringify(a) === JSON.stringify(b);

/**
 * One playback of one resolved source. Owns the clock, seeking, buffering
 * detection, tracks, resume position, metrics, and teardown. Backend events
 * update the session; the session publishes immutable PlaybackState snapshots.
 */
export class BunaSession {
  readonly intent: PlayIntent;
  private readonly backend: BunaBackend;
  private readonly source: ResolvedSource;
  private readonly scheduler: Scheduler;
  private readonly positions?: PositionStore;
  private readonly config: SessionConfig;
  private readonly positionKey: string;
  private readonly unsubscribeBackend: () => void;
  private readonly listeners = new Set<(state: PlaybackState) => void>();
  private readonly disposeListeners = new Set<() => void>();
  private current: PlaybackState;
  private disposed = false;
  private startedAt: number;

  // Facts learned from the backend, used to derive the phase.
  private metadata = false;
  private started = false;
  private waiting = false;
  private ended = false;

  private clockTimer: number | undefined;
  private startupTimer: number | undefined;
  private seekTimer: number | undefined;
  private seekTimeout: number | undefined;

  private seekSeq = 0;
  private seekFrom = 0;
  private seekStartedAt = 0;
  private burst = 0;
  private burstDirection = 0;
  private lastAdvanceAt = 0;
  private lastSampledTime = -1;
  private stallStartedAt = 0;
  private lastSavedAt = 0;

  constructor(options: SessionOptions) {
    this.backend = options.backend;
    this.source = options.source;
    this.intent = options.intent;
    this.scheduler = options.scheduler ?? browserScheduler;
    this.positions = options.positions;
    this.config = { ...DEFAULT_CONFIG, ...options.config };
    this.positionKey = options.intent.videoId || options.intent.contentId;
    this.startedAt = this.scheduler.now();
    this.current = {
      phase: 'idle',
      intent: 'play',
      position: 0,
      positionSource: 'initial',
      duration: null,
      durationSource: 'none',
      seekable: null,
      bufferedEnd: 0,
      previewTarget: null,
      pendingTarget: null,
      audioTracks: [],
      textTracks: [],
      capabilities: this.backend.capabilities(),
      error: null,
      firstFrameMs: null,
      resumedFrom: null,
      stalled: false,
    };
    this.unsubscribeBackend = this.backend.subscribe(event => this.onBackend(event));
  }

  get state(): PlaybackState { return this.current; }
  get isDisposed(): boolean { return this.disposed; }
  /** Seek position to draw: what the user is choosing, else what was sent, else the media clock. */
  get displayPosition(): number {
    const s = this.current;
    return s.previewTarget ?? s.pendingTarget ?? s.position;
  }

  subscribe(listener: (state: PlaybackState) => void): () => void {
    this.listeners.add(listener);
    listener(this.current);
    return () => { this.listeners.delete(listener); };
  }

  onDispose(listener: () => void): () => void {
    this.disposeListeners.add(listener);
    return () => { this.disposeListeners.delete(listener); };
  }

  // ---------------------------------------------------------------- commands

  async start(): Promise<CommandResult> {
    if (this.disposed) return fail('disposed', 'Playback has ended');
    if (this.isExpired()) return this.expire();
    metrics.record({ name: 'session-start', at: Date.now(), backend: this.backend.kind, transport: this.source.transport });
    const saved = this.intent.startPosition ?? this.positions?.get(this.positionKey) ?? 0;
    const resume = saved > 0 ? saved : 0;
    this.commit({ intent: 'play', position: resume, resumedFrom: resume || null });
    return this.load(resume);
  }

  async play(): Promise<CommandResult> {
    const blocked = this.guard();
    if (blocked) return blocked;
    if (this.ended) this.ended = false;
    this.commit({ intent: 'play' });
    const result = await this.backend.play();
    if (this.disposed) return fail('disposed', 'Playback has ended');
    if (!result.ok && result.reason !== 'not-ready') {
      // `not-ready` means a later pause/seek/load interrupted this request: not an error.
      if (!this.started) this.setError({ code: 'play-rejected', message: result.message || 'The TV refused to start playback.', retryable: true });
      else this.commit({ intent: 'pause' });
    }
    return result;
  }

  pause(): CommandResult {
    const blocked = this.guard();
    if (blocked) return blocked;
    this.commit({ intent: 'pause' });
    const result = this.backend.pause();
    this.savePosition();
    return result;
  }

  toggle(): Promise<CommandResult> | CommandResult {
    if (this.ended) return this.play();
    return this.current.intent === 'play' ? this.pause() : this.play();
  }

  /** Move the seek preview by one step. The seek is sent after a quiet period. */
  nudge(direction: 1 | -1): CommandResult {
    const blocked = this.guardSeek();
    if (blocked) return blocked;
    const s = this.current;
    this.burst = direction === this.burstDirection && s.previewTarget !== null ? this.burst + 1 : 0;
    this.burstDirection = direction;
    const base = s.previewTarget ?? s.pendingTarget ?? s.position;
    const target = this.clampSeek(base + direction * nudgeStep(this.burst));
    this.scheduler.clearTimeout(this.seekTimer);
    this.seekTimer = this.scheduler.setTimeout(() => {
      this.seekTimer = undefined;
      const chosen = this.current.previewTarget;
      if (chosen !== null && !this.disposed) this.commitSeek(chosen);
    }, this.config.seekDebounceMs);
    this.commit({ previewTarget: target });
    return ok;
  }

  /** Seek immediately (pointer, Start over, or a committed preview). */
  seekTo(seconds: number): CommandResult {
    const blocked = this.guardSeek();
    if (blocked) return blocked;
    this.scheduler.clearTimeout(this.seekTimer);
    this.seekTimer = undefined;
    return this.commitSeek(this.clampSeek(seconds));
  }

  /** Commit a pending preview now instead of waiting for the debounce. */
  commitPreview(): CommandResult {
    const target = this.current.previewTarget;
    if (target === null) return fail('not-ready', 'No seek in progress');
    return this.seekTo(target);
  }

  /** Discard an uncommitted seek preview. Returns whether there was one. */
  cancelPreview(): boolean {
    if (this.current.previewTarget === null) return false;
    this.scheduler.clearTimeout(this.seekTimer);
    this.seekTimer = undefined;
    this.burst = 0;
    this.commit({ previewTarget: null });
    return true;
  }

  restart(): CommandResult {
    const result = this.seekTo(0);
    if (result.ok) this.commit({ resumedFrom: null });
    return result;
  }

  selectAudio(id: string): CommandResult {
    const blocked = this.guard();
    if (blocked) return blocked;
    const capability = this.backend.capabilities().audioTrackSelection;
    if (!capability.supported) return fail('unsupported', capability.reason || 'Audio track selection is unavailable');
    const result = this.backend.selectAudio(id);
    this.refreshTracks();
    return result;
  }

  selectSubtitle(id: string | null): CommandResult {
    const blocked = this.guard();
    if (blocked) return blocked;
    const capability = this.backend.capabilities().textTrackSelection;
    if (!capability.supported) return fail('unsupported', capability.reason || 'Subtitle selection is unavailable');
    const result = this.backend.selectSubtitle(id);
    this.refreshTracks();
    return result;
  }

  /** Reload the same source at the last known position. */
  async retry(): Promise<CommandResult> {
    if (this.disposed) return fail('disposed', 'Playback has ended');
    if (this.isExpired()) return this.expire();
    const at = this.current.pendingTarget ?? this.current.position;
    this.resetLoadFacts();
    this.commit({ error: null, intent: 'play', previewTarget: null, pendingTarget: null, stalled: false });
    return this.load(at);
  }

  /** Stop playback, release the decoder, and drop every timer and listener. Idempotent. */
  dispose(): void {
    if (this.disposed) return;
    this.savePosition();
    metrics.record({ name: 'dispose', at: Date.now(), position: Math.round(this.current.position), watchedMs: Math.round(this.scheduler.now() - this.startedAt) });
    this.disposed = true;
    this.clearTimers();
    this.unsubscribeBackend();
    this.backend.dispose();
    this.current = { ...this.current, phase: 'idle', previewTarget: null, pendingTarget: null };
    this.listeners.forEach(listener => listener(this.current));
    this.listeners.clear();
    this.disposeListeners.forEach(listener => listener());
    this.disposeListeners.clear();
  }

  // ---------------------------------------------------------------- loading

  private async load(at: number): Promise<CommandResult> {
    this.scheduler.clearTimeout(this.startupTimer);
    this.startupTimer = this.scheduler.setTimeout(() => {
      if (!this.metadata && !this.disposed) {
        this.setError({ code: 'startup-timeout', message: 'The video did not start loading. The server may be slow or the link may no longer be valid.', retryable: true });
      }
    }, this.config.startupTimeoutMs);
    this.commit({});
    const prepared = await this.backend.prepare(this.source, at);
    if (this.disposed) return fail('disposed', 'Playback has ended');
    if (!prepared.ok) {
      this.setError({ code: 'unsupported', message: prepared.message, retryable: false });
      return prepared;
    }
    return this.play();
  }

  private resetLoadFacts() {
    this.metadata = false;
    this.started = false;
    this.waiting = false;
    this.ended = false;
    this.lastSampledTime = -1;
    this.clearTimers();
  }

  private isExpired() {
    return !!this.source.expiresAt && this.source.expiresAt <= Date.now();
  }

  private expire(): CommandResult {
    this.setError({ code: 'expired', message: 'This link has expired. Choose the source again to get a fresh one.', retryable: false });
    return fail('failed', 'Link expired');
  }

  // ---------------------------------------------------------------- backend events

  private onBackend(event: BackendEvent) {
    if (this.disposed) return;
    switch (event.type) {
      case 'metadata':
        if (!this.metadata) {
          this.metadata = true;
          this.scheduler.clearTimeout(this.startupTimer);
          metrics.record({ name: 'metadata', at: Date.now(), ms: Math.round(this.scheduler.now() - this.intent.requestedAt) });
        }
        this.refreshTracks();
        this.sample();
        break;
      case 'canplay':
        this.sample();
        break;
      case 'playing':
        this.started = true;
        this.waiting = false;
        this.lastAdvanceAt = this.scheduler.now();
        this.endStall();
        this.commit({ intent: 'play' });
        this.sample();
        break;
      case 'first-frame':
        if (this.current.firstFrameMs === null) {
          const ms = Math.round(this.scheduler.now() - this.intent.requestedAt);
          metrics.record({ name: 'first-frame', at: Date.now(), ms });
          this.commit({ firstFrameMs: ms });
        }
        break;
      case 'waiting':
        this.waiting = true;
        this.commit({});
        break;
      case 'pause': {
        const snap = this.backend.snapshot();
        // The backend is the authority: a pause we didn't ask for (e.g. the TV
        // suspending the app) still changes the intent the UI shows.
        if (snap.paused && !snap.ended) this.commit({ intent: 'pause' });
        this.savePosition();
        break;
      }
      case 'seeked':
        if (this.current.pendingTarget !== null && !this.backend.snapshot().seeking) this.ackSeek();
        else this.sample();
        break;
      case 'ended':
        this.markEnded();
        break;
      case 'time':
      case 'progress':
        this.sample();
        break;
      case 'tracks':
        this.refreshTracks();
        break;
      case 'error':
        // Aborts happen when we replace or release the source; they are not failures.
        if (event.error.code !== 'aborted') this.setError(event.error);
        break;
      case 'seeking':
        break;
    }
  }

  /** Read the backend clock once and update state with provenance. */
  private sample() {
    if (this.disposed) return;
    const snap = this.backend.snapshot();
    const s = this.current;
    const now = this.scheduler.now();
    const patch: Partial<PlaybackState> = { bufferedEnd: finite(snap.bufferedEnd) ? snap.bufferedEnd : 0 };

    if (finite(snap.duration) && snap.duration > 0) {
      patch.duration = snap.duration;
      patch.durationSource = 'media';
    } else if (snap.seekable && snap.seekable.end > 0) {
      patch.duration = snap.seekable.end;
      patch.durationSource = 'seekable';
    } // else keep the last known duration: some TVs report NaN transiently.
    if (!sameJson(snap.seekable, s.seekable)) patch.seekable = snap.seekable;

    if (s.pendingTarget !== null) {
      // Fallback acknowledgement for TVs that drop `seeked`: a settled, fresh clock reading.
      if (!snap.seeking && snap.readyState >= 2 && now - this.seekStartedAt > 300) return this.ackSeek(patch);
    } else if (!snap.seeking && finite(snap.currentTime) && snap.currentTime >= 0) {
      patch.position = snap.currentTime;
      patch.positionSource = 'media';
    }

    if (snap.ended && !this.ended) return this.markEnded(patch);

    // Stall watchdog: detects buffering on TVs that don't emit `waiting`.
    const watching = s.intent === 'play' && this.started && s.pendingTarget === null && !this.ended && !snap.paused;
    if (watching) {
      if (snap.currentTime > this.lastSampledTime + 0.05) {
        this.lastAdvanceAt = now;
        this.waiting = false;
        if (s.stalled) this.endStall(patch);
      } else if (!s.stalled && now - this.lastAdvanceAt >= this.config.stallAfterMs) {
        patch.stalled = true;
        this.stallStartedAt = now;
        metrics.record({ name: 'stall', at: Date.now(), position: Math.round(snap.currentTime) });
      }
    }
    this.lastSampledTime = snap.currentTime;
    if (s.intent === 'play' && now - this.lastSavedAt >= this.config.positionSaveMs) this.savePosition();
    this.commit(patch);
  }

  private endStall(patch?: Partial<PlaybackState>) {
    if (!this.current.stalled) return;
    metrics.record({ name: 'stall-end', at: Date.now(), ms: Math.round(this.scheduler.now() - this.stallStartedAt) });
    if (patch) patch.stalled = false;
    else this.commit({ stalled: false });
  }

  private markEnded(patch: Partial<PlaybackState> = {}) {
    this.ended = true;
    this.positions?.clear(this.positionKey);
    this.commit({ ...patch, intent: 'pause', stalled: false });
  }

  private refreshTracks() {
    const audioTracks = this.backend.audioTracks();
    const textTracks = this.backend.textTracks();
    const capabilities = this.backend.capabilities();
    const s = this.current;
    this.commit({
      ...(sameJson(audioTracks, s.audioTracks) ? {} : { audioTracks }),
      ...(sameJson(textTracks, s.textTracks) ? {} : { textTracks }),
      ...(sameJson(capabilities, s.capabilities) ? {} : { capabilities }),
    });
  }

  // ---------------------------------------------------------------- seeking

  private guard(): CommandResult | null {
    if (this.disposed) return fail('disposed', 'Playback has ended');
    if (this.current.error) return fail('not-ready', this.current.error.message);
    return null;
  }

  private guardSeek(): CommandResult | null {
    const blocked = this.guard();
    if (blocked) return blocked;
    if (!this.metadata) return fail('not-ready', 'The video is still loading');
    const capability = this.backend.capabilities().seek;
    if (!capability.supported) return fail('unsupported', capability.reason || 'Seeking is unavailable for this video');
    return null;
  }

  private clampSeek(seconds: number): number {
    const s = this.current;
    const start = s.seekable?.start ?? 0;
    const end = s.seekable?.end ?? s.duration ?? seconds;
    // Stop just short of the end so a long forward seek lands on a frame instead of ending.
    const upper = Math.max(start, end - 1);
    return Math.min(Math.max(seconds, start), upper);
  }

  private commitSeek(target: number): CommandResult {
    const seq = ++this.seekSeq;
    this.seekFrom = this.current.position;
    this.seekStartedAt = this.scheduler.now();
    this.burst = 0;
    this.ended = false;
    const result = this.backend.seek(target);
    if (!result.ok) {
      this.commit({ previewTarget: null, pendingTarget: null });
      return result;
    }
    this.scheduler.clearTimeout(this.seekTimeout);
    this.seekTimeout = this.scheduler.setTimeout(() => {
      if (seq !== this.seekSeq || this.disposed || this.current.pendingTarget === null) return;
      metrics.record({ name: 'seek-timeout', at: Date.now(), to: Math.round(target) });
      this.commit({ pendingTarget: null });
      this.sample();
    }, this.config.seekTimeoutMs);
    this.commit({ previewTarget: null, pendingTarget: target });
    return ok;
  }

  private ackSeek(patch: Partial<PlaybackState> = {}) {
    const target = this.current.pendingTarget;
    if (target === null) return;
    this.scheduler.clearTimeout(this.seekTimeout);
    const snap = this.backend.snapshot();
    const landed = finite(snap.currentTime) ? snap.currentTime : target;
    const now = this.scheduler.now();
    metrics.record({
      name: 'seek', at: Date.now(), from: Math.round(this.seekFrom), to: Math.round(target),
      settleMs: Math.round(now - this.seekStartedAt), landed: Math.round(landed * 10) / 10,
    });
    this.lastAdvanceAt = now;
    this.lastSampledTime = landed;
    this.commit({ ...patch, pendingTarget: null, position: landed, positionSource: 'seek-ack', stalled: false });
    this.savePosition();
  }

  // ---------------------------------------------------------------- state

  private setError(error: BunaError) {
    if (this.disposed) return;
    metrics.record({ name: 'error', at: Date.now(), code: error.code });
    this.savePosition();
    this.clearTimers();
    this.commit({ error, previewTarget: null, pendingTarget: null, stalled: false });
  }

  private savePosition() {
    if (!this.positions || !this.metadata || this.ended) return;
    const s = this.current;
    this.positions.set(this.positionKey, s.pendingTarget ?? s.position, s.duration);
    this.lastSavedAt = this.scheduler.now();
  }

  private derivePhase(s: PlaybackState): PlaybackPhase {
    if (this.disposed) return 'idle';
    if (s.error) return 'error';
    if (this.ended) return 'ended';
    if (s.pendingTarget !== null) return 'seeking';
    if (!this.started) return s.intent === 'pause' && this.metadata ? 'paused' : 'loading';
    if (s.intent === 'pause') return 'paused';
    if (this.waiting || s.stalled) return 'buffering';
    return 'playing';
  }

  private commit(patch: Partial<PlaybackState>) {
    if (this.disposed) return;
    const next = { ...this.current, ...patch };
    next.phase = this.derivePhase(next);
    let changed = false;
    for (const key in next) {
      if (next[key as keyof PlaybackState] !== this.current[key as keyof PlaybackState]) { changed = true; break; }
    }
    if (!changed) return;
    this.current = next;
    this.listeners.forEach(listener => listener(next));
    this.manageClock();
  }

  /** The clock runs only while time can move; a paused session does no periodic work. */
  private manageClock() {
    const run = !this.disposed && CLOCK_PHASES.includes(this.current.phase);
    if (run && this.clockTimer === undefined) {
      this.clockTimer = this.scheduler.setInterval(() => this.sample(), this.config.clockIntervalMs);
    } else if (!run && this.clockTimer !== undefined) {
      this.scheduler.clearInterval(this.clockTimer);
      this.clockTimer = undefined;
    }
  }

  private clearTimers() {
    this.scheduler.clearInterval(this.clockTimer);
    this.scheduler.clearTimeout(this.startupTimer);
    this.scheduler.clearTimeout(this.seekTimer);
    this.scheduler.clearTimeout(this.seekTimeout);
    this.clockTimer = this.startupTimer = this.seekTimer = this.seekTimeout = undefined;
  }
}
