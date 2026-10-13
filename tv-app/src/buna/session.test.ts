import { beforeEach, describe, expect, it, vi } from 'vitest';
import { metrics } from './metrics';
import { memoryPositionStore } from './positions';
import { BunaRuntime } from './runtime';
import { BunaSession, nudgeStep } from './session';
import { FakeBackend, ManualScheduler, testSource } from './testing';
import type { PlayIntent, ResolvedSource } from './types';

const intent = (overrides: Partial<PlayIntent> = {}): PlayIntent => ({
  contentId: 'tt0000001', contentType: 'movie', title: 'Test', origin: 'sources', requestedAt: 0, ...overrides,
});

function setup(options: { source?: ResolvedSource; intent?: Partial<PlayIntent>; saved?: number } = {}) {
  const backend = new FakeBackend();
  const scheduler = new ManualScheduler();
  const positions = memoryPositionStore();
  if (options.saved) positions.data.set(options.intent?.videoId || 'tt0000001', options.saved);
  const session = new BunaSession({ backend, scheduler, positions, source: options.source ?? testSource(), intent: intent(options.intent) });
  return { backend, scheduler, positions, session };
}

/** Started, metadata loaded (600 s), playing from 0. */
async function playing() {
  const ctx = setup();
  await ctx.session.start();
  ctx.backend.loadMetadata(600);
  ctx.backend.startPlaying();
  return ctx;
}

const flush = () => new Promise(resolve => setTimeout(resolve, 0));

beforeEach(() => {
  metrics.clear();
  vi.spyOn(console, 'info').mockImplementation(() => {});
});

describe('startup', () => {
  it('moves loading → playing and records first frame timing', async () => {
    const { backend, scheduler, session } = setup();
    expect(session.state.phase).toBe('idle');
    scheduler.time = 1200;
    const started = session.start();
    expect(session.state.phase).toBe('loading');
    await started;
    expect(backend.calls).toEqual(['prepare:0', 'play']);
    backend.loadMetadata(600);
    expect(session.state.phase).toBe('loading');
    expect(session.state.duration).toBe(600);
    expect(session.state.durationSource).toBe('media');
    backend.startPlaying();
    expect(session.state.phase).toBe('playing');
    expect(session.state.firstFrameMs).toBe(1200);
    expect(metrics.list().map(m => m.name)).toEqual(['session-start', 'metadata', 'first-frame']);
  });

  it('resumes from the saved position', async () => {
    const { backend, session } = setup({ saved: 300 });
    await session.start();
    expect(backend.calls[0]).toBe('prepare:300');
    expect(session.state.resumedFrom).toBe(300);
    expect(session.state.position).toBe(300);
  });

  it('prefers an explicit start position over the saved one', async () => {
    const { backend, session } = setup({ saved: 300, intent: { startPosition: 42 } });
    await session.start();
    expect(backend.calls[0]).toBe('prepare:42');
  });

  it('keys resume positions by episode', async () => {
    const { backend, session } = setup({ saved: 500, intent: { videoId: 'tt0000001:1:2' } });
    await session.start();
    expect(backend.calls[0]).toBe('prepare:500');
  });

  it('fails with a retryable error when metadata never arrives', async () => {
    const { scheduler, session } = setup();
    await session.start();
    scheduler.advance(45000);
    expect(session.state.phase).toBe('error');
    expect(session.state.error).toMatchObject({ code: 'startup-timeout', retryable: true });
  });

  it('refuses an expired source without touching the backend', async () => {
    const { backend, session } = setup({ source: testSource({ expiresAt: Date.now() - 1 }) });
    const result = await session.start();
    expect(result.ok).toBe(false);
    expect(backend.calls).toEqual([]);
    expect(session.state.error?.code).toBe('expired');
  });

  it('turns a rejected first play into an error, but a later rejection into a pause', async () => {
    const first = setup();
    first.backend.playResult = { ok: false, reason: 'failed', message: 'NotAllowedError' };
    await first.session.start();
    expect(first.session.state.error?.code).toBe('play-rejected');

    const later = await playing();
    later.session.pause();
    later.backend.playResult = { ok: false, reason: 'failed', message: 'nope' };
    await later.session.play();
    expect(later.session.state.error).toBeNull();
    expect(later.session.state.intent).toBe('pause');
  });

  it('ignores an interrupted play request', async () => {
    const { backend, session } = await playing();
    backend.playResult = { ok: false, reason: 'not-ready', message: 'AbortError' };
    session.pause();
    await session.play();
    expect(session.state.error).toBeNull();
  });
});

describe('clock', () => {
  it('samples the media clock only while time can move', async () => {
    const { backend, scheduler, session } = await playing();
    backend.snap.currentTime = 1;
    scheduler.advance(250);
    expect(session.state.position).toBe(1);
    expect(session.state.positionSource).toBe('media');
    expect(scheduler.activeTimers).toBe(1);

    session.pause();
    expect(session.state.phase).toBe('paused');
    expect(scheduler.activeTimers).toBe(0);
    backend.snap.currentTime = 99;
    scheduler.advance(5000);
    expect(session.state.position).toBe(1);
  });

  it('falls back to the seekable range when duration is not reported', async () => {
    const { backend, session } = setup();
    await session.start();
    Object.assign(backend.snap, { duration: Infinity, seekable: { start: 0, end: 1800 }, readyState: 1 });
    backend.emit({ type: 'metadata' });
    expect(session.state.duration).toBe(1800);
    expect(session.state.durationSource).toBe('seekable');
  });

  it('keeps the last known duration when the TV reports NaN transiently', async () => {
    const { backend, session } = await playing();
    backend.snap.duration = NaN;
    backend.snap.seekable = null;
    backend.emit({ type: 'time' });
    expect(session.state.duration).toBe(600);
  });

  it('follows a pause the backend initiated', async () => {
    const { backend, session } = await playing();
    backend.snap.paused = true;
    backend.emit({ type: 'pause' });
    expect(session.state.intent).toBe('pause');
    expect(session.state.phase).toBe('paused');
  });

  it('detects a stall without a waiting event, and recovers', async () => {
    const { backend, scheduler, session } = await playing();
    backend.snap.currentTime = 5;
    scheduler.advance(250);
    scheduler.advance(3000);
    expect(session.state.phase).toBe('buffering');
    expect(session.state.stalled).toBe(true);
    backend.snap.currentTime = 5.5;
    scheduler.advance(250);
    expect(session.state.phase).toBe('playing');
    expect(metrics.list().map(m => m.name)).toContain('stall-end');
  });

  it('shows buffering on waiting and clears it once the clock advances', async () => {
    const { backend, scheduler, session } = await playing();
    backend.emit({ type: 'waiting' });
    expect(session.state.phase).toBe('buffering');
    backend.snap.currentTime = 0.5;
    scheduler.advance(250);
    expect(session.state.phase).toBe('playing');
  });

  it('ends, forgets the resume position, and can play again', async () => {
    const { backend, positions, session } = await playing();
    positions.data.set('tt0000001', 400);
    Object.assign(backend.snap, { ended: true, paused: true, currentTime: 600 });
    backend.emit({ type: 'ended' });
    expect(session.state.phase).toBe('ended');
    expect(positions.data.has('tt0000001')).toBe(false);
    await session.toggle();
    expect(backend.calls.slice(-1)[0]).toBe('play');
    expect(session.state.intent).toBe('play');
  });
});

describe('seeking', () => {
  it('previews nudges without seeking, then commits once after the quiet period', async () => {
    const { backend, scheduler, session } = await playing();
    session.nudge(1);
    session.nudge(1);
    expect(session.state.previewTarget).toBe(20);
    expect(session.displayPosition).toBe(20);
    expect(backend.calls.filter(c => c.startsWith('seek'))).toEqual([]);
    scheduler.advance(599);
    expect(backend.calls.filter(c => c.startsWith('seek'))).toEqual([]);
    scheduler.advance(1);
    expect(backend.calls.filter(c => c.startsWith('seek'))).toEqual(['seek:20']);
    expect(session.state).toMatchObject({ phase: 'seeking', previewTarget: null, pendingTarget: 20 });
  });

  it('does not let a stale clock reading move the thumb while a seek is in flight', async () => {
    const { backend, scheduler, session } = await playing();
    session.seekTo(300);
    backend.snap.currentTime = 2; // TV still reporting the old position
    scheduler.advance(250);
    expect(session.state.position).toBe(0);
    expect(session.displayPosition).toBe(300);
  });

  it('commits the position from the backend acknowledgement, not the request', async () => {
    const { backend, session } = await playing();
    session.seekTo(300);
    backend.finishSeek(297.5); // keyframe landing
    expect(session.state).toMatchObject({ phase: 'playing', pendingTarget: null, position: 297.5, positionSource: 'seek-ack' });
    expect(metrics.list().find(m => m.name === 'seek')).toMatchObject({ to: 300, landed: 297.5 });
  });

  it('acknowledges from a settled clock reading when the TV drops seeked', async () => {
    const { backend, scheduler, session } = await playing();
    session.seekTo(120);
    backend.finishSeek(120, false);
    scheduler.advance(250);
    expect(session.state.pendingTarget).not.toBeNull(); // too soon to trust
    scheduler.advance(250);
    expect(session.state).toMatchObject({ pendingTarget: null, position: 120, phase: 'playing' });
  });

  it('gives up on a seek that never settles and returns to the media clock', async () => {
    const { backend, scheduler, session } = await playing();
    session.seekTo(120);
    scheduler.advance(15000);
    expect(session.state.pendingTarget).toBeNull();
    expect(metrics.list().map(m => m.name)).toContain('seek-timeout');
    backend.finishSeek(119);
    expect(session.state.position).toBe(119);
  });

  it('accelerates while a direction is held and resets on reversal', async () => {
    const { session } = await playing();
    session.seekTo(300);
    const steps: number[] = [];
    for (let i = 0; i < 10; i++) {
      const before = session.displayPosition;
      session.nudge(1);
      steps.push(session.displayPosition - before);
    }
    expect(steps).toEqual([10, 10, 10, 10, 30, 30, 30, 30, 60, 60]);
    const before = session.displayPosition;
    session.nudge(-1);
    expect(session.displayPosition - before).toBe(-10);
    expect(nudgeStep(20)).toBe(120);
  });

  it('clamps to the seekable range and stops short of the end', async () => {
    const { session } = await playing();
    session.nudge(-1);
    expect(session.state.previewTarget).toBe(0);
    session.cancelPreview();
    session.seekTo(10_000);
    expect(session.state.pendingTarget).toBe(599);
  });

  it('cancels an uncommitted preview', async () => {
    const { backend, scheduler, session } = await playing();
    session.nudge(1);
    expect(session.cancelPreview()).toBe(true);
    scheduler.advance(2000);
    expect(backend.calls.some(c => c.startsWith('seek'))).toBe(false);
    expect(session.cancelPreview()).toBe(false);
  });

  it('commits a preview immediately on request', async () => {
    const { backend, session } = await playing();
    session.nudge(1);
    expect(session.commitPreview().ok).toBe(true);
    expect(backend.calls.slice(-1)[0]).toBe('seek:10');
  });

  it('refuses to seek before metadata or when the file is not seekable', async () => {
    const early = setup();
    await early.session.start();
    expect(early.session.nudge(1)).toMatchObject({ ok: false, reason: 'not-ready' });

    const fixed = setup();
    await fixed.session.start();
    Object.assign(fixed.backend.snap, { duration: 60, seekable: null, readyState: 1 });
    fixed.backend.emit({ type: 'metadata' });
    expect(fixed.session.seekTo(10)).toMatchObject({ ok: false, reason: 'unsupported' });
  });

  it('restart seeks to zero and clears the resume marker', async () => {
    const ctx = setup({ saved: 300 });
    await ctx.session.start();
    ctx.backend.loadMetadata(600);
    ctx.backend.startPlaying();
    ctx.session.restart();
    expect(ctx.backend.calls.slice(-1)[0]).toBe('seek:0');
    expect(ctx.session.state.resumedFrom).toBeNull();
  });

  it('seeking after the end leaves the ended state', async () => {
    const { backend, session } = await playing();
    Object.assign(backend.snap, { ended: true, paused: true });
    backend.emit({ type: 'ended' });
    session.seekTo(100);
    backend.finishSeek(100);
    expect(session.state.phase).toBe('paused');
  });
});

describe('tracks', () => {
  it('reports unsupported audio selection honestly', async () => {
    const { session } = await playing();
    expect(await session.selectAudio('a#0')).toMatchObject({ ok: false, reason: 'unsupported' });
  });

  it('switches audio and subtitles when the backend exposes them', async () => {
    const { backend, session } = await playing();
    backend.audio = [{ id: 'a#0', label: 'English', language: 'en', selected: true }, { id: 'a#1', label: 'French', language: 'fr', selected: false }];
    backend.text = [{ id: 't#0', label: 'English', language: 'en', selected: false }];
    backend.emit({ type: 'tracks' });
    expect(session.state.capabilities.audioTrackSelection.supported).toBe(true);
    expect((await session.selectAudio('a#1')).ok).toBe(true);
    expect(session.state.audioTracks.find(t => t.selected)?.id).toBe('a#1');
    expect((await session.selectSubtitle('t#0')).ok).toBe(true);
    expect(session.state.textTracks[0].selected).toBe(true);
    expect((await session.selectSubtitle(null)).ok).toBe(true);
    expect(session.state.textTracks[0].selected).toBe(false);
  });
});

describe('errors and retry', () => {
  it('ignores aborts but surfaces real errors and blocks commands', async () => {
    const { backend, scheduler, session } = await playing();
    backend.emit({ type: 'error', error: { code: 'aborted', message: '', retryable: true } });
    expect(session.state.phase).toBe('playing');
    backend.emit({ type: 'error', error: { code: 'network', message: 'down', retryable: true } });
    expect(session.state.phase).toBe('error');
    expect(scheduler.activeTimers).toBe(0);
    expect(session.pause()).toMatchObject({ ok: false, reason: 'not-ready' });
  });

  it('retries at the last position', async () => {
    const { backend, scheduler, session } = await playing();
    backend.snap.currentTime = 250;
    scheduler.advance(250);
    backend.emit({ type: 'error', error: { code: 'network', message: 'down', retryable: true } });
    await session.retry();
    expect(backend.calls.filter(c => c.startsWith('prepare'))).toEqual(['prepare:0', 'prepare:250']);
    expect(session.state.phase).toBe('loading');
    backend.loadMetadata(600);
    backend.startPlaying();
    expect(session.state.phase).toBe('playing');
  });
});

describe('lifecycle', () => {
  it('disposes completely and only once', async () => {
    const { backend, positions, scheduler, session } = await playing();
    backend.snap.currentTime = 200;
    scheduler.advance(250);
    session.nudge(1);
    const states: string[] = [];
    session.subscribe(s => states.push(s.phase));
    session.dispose();
    session.dispose();
    expect(backend.calls.filter(c => c === 'dispose')).toHaveLength(1);
    expect(backend.listenerCount).toBe(0);
    expect(scheduler.activeTimers).toBe(0);
    expect(positions.data.get('tt0000001')).toBe(200);
    expect(states.slice(-1)[0]).toBe('idle');
    backend.emit({ type: 'playing' });
    expect(session.state.phase).toBe('idle');
    expect(session.pause()).toMatchObject({ ok: false, reason: 'disposed' });
    expect((await session.play()).ok).toBe(false);
  });

  it('does not resurrect a session disposed during startup', async () => {
    const { backend, session } = setup();
    const started = session.start();
    session.dispose();
    await started;
    await flush();
    expect(backend.calls).not.toContain('play');
    expect(session.state.phase).toBe('idle');
  });

  it('runtime keeps exactly one live session', () => {
    const backends: FakeBackend[] = [];
    const runtime = new BunaRuntime(memoryPositionStore());
    const host = { createBackend: () => { const b = new FakeBackend(); backends.push(b); return b; } };
    const first = runtime.open({ intent: intent(), source: testSource(), host });
    const second = runtime.open({ intent: intent(), source: testSource(), host });
    expect(first.isDisposed).toBe(true);
    expect(backends[0].disposed).toBe(true);
    expect(runtime.current).toBe(second);
    second.dispose();
    expect(runtime.current).toBeNull();
  });
});
