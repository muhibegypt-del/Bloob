import {
  fail, ok,
  type BackendCapabilities, type BackendEvent, type BackendSnapshot, type BunaBackend,
  type BunaError, type CommandResult, type ResolvedSource, type Track,
  type PlaybackHost,
} from '../types';
import { WebosMediaTracks } from './webos-media-tracks';

/** Bind a rendered video surface to Buna without making the runtime DOM-specific. */
export function webVideoHost(video: HTMLVideoElement): PlaybackHost {
  return { createBackend: () => new WebVideoBackend(video) };
}

type AudioTrackLike = { id?: string; label?: string; language?: string; enabled: boolean };
type TrackListLike<T> = { length: number; [index: number]: T } & Partial<EventTarget>;
type TrackVideo = HTMLVideoElement & {
  audioTracks?: TrackListLike<AudioTrackLike>;
  requestVideoFrameCallback?: (callback: () => void) => number;
  cancelVideoFrameCallback?: (handle: number) => void;
};

const MEDIA_EVENTS = [
  'loadedmetadata', 'durationchange', 'canplay', 'playing', 'waiting', 'pause',
  'seeking', 'seeked', 'ended', 'timeupdate', 'progress', 'error',
] as const;

const SUBTITLE_KINDS = new Set(['subtitles', 'captions']);

/**
 * Buna backend over the TV's HTMLVideoElement (LG's web media pipeline).
 * This is the only code in Bloob that touches a media element.
 */
export class WebVideoBackend implements BunaBackend {
  readonly kind = 'web-video';
  private readonly video: TrackVideo;
  private readonly listeners = new Set<(event: BackendEvent) => void>();
  private readonly detach: (() => void)[] = [];
  private pendingStart = 0;
  private firstFrameSent = false;
  private frameHandle: number | undefined;
  private disposed = false;
  private readonly nativeTracks: WebosMediaTracks;
  private transport: ResolvedSource['transport'] = 'progressive';

  constructor(video: HTMLVideoElement) {
    this.video = video as TrackVideo;
    this.nativeTracks = new WebosMediaTracks(video, () => this.emit({ type: 'tracks' }));
    this.video.preload = 'auto';
    this.video.playsInline = true;
    for (const name of MEDIA_EVENTS) this.listen(this.video, name, () => this.onMediaEvent(name));
    const audio = this.video.audioTracks;
    if (audio?.addEventListener) {
      for (const name of ['addtrack', 'removetrack', 'change']) this.listen(audio as EventTarget, name, () => this.emit({ type: 'tracks' }));
    }
    for (const name of ['addtrack', 'removetrack', 'change']) this.listen(this.video.textTracks, name, () => this.emit({ type: 'tracks' }));
  }

  async prepare(source: ResolvedSource, startPosition = 0): Promise<CommandResult> {
    if (this.disposed) return fail('disposed', 'Backend released');
    if (source.transport === 'dash') return fail('unsupported', 'DASH streams need a different Buna backend.');
    if (source.transport === 'hls' && !this.video.canPlayType('application/vnd.apple.mpegurl')) {
      return fail('unsupported', 'This TV does not play HLS through its web video element.');
    }
    this.cancelFrameCallback();
    this.firstFrameSent = false;
    this.pendingStart = startPosition > 0 ? startPosition : 0;
    this.nativeTracks.stop();
    this.transport = source.transport;
    this.video.src = source.url;
    this.video.load();
    this.nativeTracks.start();
    return ok;
  }

  async play(): Promise<CommandResult> {
    if (this.disposed) return fail('disposed', 'Backend released');
    try {
      await this.video.play();
      return ok;
    } catch (error) {
      const name = (error as Error)?.name;
      // AbortError: a pause/seek/load replaced this request. Not a failure.
      if (name === 'AbortError') return fail('not-ready', 'Play request was interrupted');
      return fail('failed', (error as Error)?.message || 'Playback could not start');
    }
  }

  pause(): CommandResult {
    if (this.disposed) return fail('disposed', 'Backend released');
    this.video.pause();
    return ok;
  }

  seek(seconds: number): CommandResult {
    if (this.disposed) return fail('disposed', 'Backend released');
    if (!Number.isFinite(seconds)) return fail('failed', 'Invalid seek position');
    if (!this.video.seekable.length) return fail('unsupported', 'This video is not seekable');
    this.pendingStart = 0;
    this.video.currentTime = seconds;
    return ok;
  }

  selectAudio(id: string): Promise<CommandResult> | CommandResult {
    if (id.startsWith('native-a#')) return this.nativeTracks.selectAudio(id);
    const tracks = this.video.audioTracks;
    if (!tracks || tracks.length < 2) return fail('unsupported', 'No alternative audio tracks are exposed');
    let found = false;
    for (let i = 0; i < tracks.length; i++) if (trackId(tracks[i].id, 'a', i) === id) found = true;
    if (!found) return fail('failed', 'Audio track not found');
    // Enable the chosen track before disabling the others so the pipeline never has zero audio tracks.
    for (let i = 0; i < tracks.length; i++) if (trackId(tracks[i].id, 'a', i) === id) tracks[i].enabled = true;
    for (let i = 0; i < tracks.length; i++) if (trackId(tracks[i].id, 'a', i) !== id) tracks[i].enabled = false;
    return ok;
  }

  selectSubtitle(id: string | null): Promise<CommandResult> | CommandResult {
    if (id?.startsWith('native-t#') || (id === null && this.nativeTracks.canSelectText() && !this.domTextTracks().length)) {
      if (this.transport === 'hls') return fail('unsupported', 'This TV cannot reliably switch embedded HLS subtitles.');
      return this.nativeTracks.selectText(id);
    }
    const tracks = this.video.textTracks;
    const found = id === null || this.domTextTracks().some(track => track.id === id);
    if (!found) return fail('failed', 'Subtitle track not found');
    for (let i = 0; i < tracks.length; i++) {
      const track = tracks[i];
      if (!SUBTITLE_KINDS.has(track.kind)) continue;
      const match = trackId(track.id, 't', i) === id;
      track.mode = match ? 'showing' : 'disabled';
    }
    return ok;
  }

  snapshot(): BackendSnapshot {
    const v = this.video;
    const seekable = v.seekable.length ? { start: v.seekable.start(0), end: v.seekable.end(v.seekable.length - 1) } : null;
    return {
      currentTime: v.currentTime,
      duration: v.duration,
      seekable: seekable && Number.isFinite(seekable.end) ? seekable : null,
      bufferedEnd: bufferedAhead(v),
      paused: v.paused,
      ended: v.ended,
      seeking: v.seeking,
      readyState: v.readyState,
    };
  }

  audioTracks(): Track[] {
    const tracks = this.video.audioTracks;
    const dom = tracks ? Array.from({ length: tracks.length }, (_, i) => ({
      id: trackId(tracks[i].id, 'a', i),
      label: tracks[i].label || '',
      language: tracks[i].language || '',
      selected: !!tracks[i].enabled,
    })) : [];
    const native = this.nativeTracks.audioTracks();
    return native.length > dom.length ? native : dom;
  }

  textTracks(): Track[] {
    const dom = this.domTextTracks();
    const native = this.nativeTracks.textTracks();
    return this.transport !== 'hls' && native.length > dom.length ? native : dom;
  }

  private domTextTracks(): Track[] {
    const tracks = this.video.textTracks;
    const result: Track[] = [];
    for (let i = 0; i < tracks.length; i++) {
      if (!SUBTITLE_KINDS.has(tracks[i].kind)) continue;
      result.push({ id: trackId(tracks[i].id, 't', i), label: tracks[i].label || '', language: tracks[i].language || '', selected: tracks[i].mode === 'showing' });
    }
    return result;
  }

  capabilities(): BackendCapabilities {
    const v = this.video;
    const loaded = v.readyState >= 1;
    const seekable = v.seekable.length > 0 && Number.isFinite(v.seekable.end(v.seekable.length - 1)) && v.seekable.end(v.seekable.length - 1) > 0;
    const audioCount = this.audioTracks().length;
    const textCount = this.textTracks().length;
    const nativeAudio = this.nativeTracks.canSelectAudio() && this.nativeTracks.audioTracks().length > (v.audioTracks?.length || 0);
    const nativeText = this.nativeTracks.canSelectText() && this.transport !== 'hls' && this.nativeTracks.textTracks().length > this.domTextTracks().length;
    return {
      seek: {
        supported: seekable,
        reason: seekable ? undefined : loaded ? 'The server does not allow seeking in this video.' : 'The video is still loading.',
        tv: 'unverified',
      },
      audioTrackSelection: {
        supported: nativeAudio || (!!v.audioTracks && audioCount > 1),
        reason: audioCount === 0
          ? "The TV hasn't exposed audio tracks for this file. It plays the default track."
          : audioCount <= 1 ? 'This file has one audio track.' : undefined,
        tv: 'unverified',
      },
      textTrackSelection: {
        supported: nativeText || this.domTextTracks().length > 0,
        reason: textCount ? this.transport === 'hls' && !this.domTextTracks().length
          ? 'Embedded HLS subtitle switching is unreliable on this TV.' : undefined
          : "The TV hasn't exposed selectable subtitles for this file.",
        tv: 'unverified',
      },
      externalSubtitles: { supported: false, reason: 'External subtitles are not implemented yet.', tv: 'unverified' },
      thumbnails: { supported: false, reason: 'Preview thumbnails need a segmented or native backend.', tv: 'unverified' },
      frameRateMatching: { supported: false, reason: 'Not controllable from the web video element.', tv: 'unverified' },
      outputInfo: { supported: false, reason: 'HDR and audio output mode are not reported to web apps.', tv: 'unverified' },
    };
  }

  subscribe(listener: (event: BackendEvent) => void): () => void {
    this.listeners.add(listener);
    return () => { this.listeners.delete(listener); };
  }

  stop(): void {
    this.cancelFrameCallback();
    this.nativeTracks.stop();
    this.pendingStart = 0;
    try {
      this.video.pause();
      this.video.removeAttribute('src');
      this.video.load(); // Releases the decoder and network connection.
    } catch { /* element already detached */ }
  }

  dispose(): void {
    if (this.disposed) return;
    this.disposed = true;
    this.listeners.clear();
    this.stop();
    this.nativeTracks.dispose();
    this.detach.splice(0).forEach(remove => remove());
  }

  // ---------------------------------------------------------------- internals

  private listen(target: EventTarget, name: string, handler: () => void) {
    target.addEventListener(name, handler);
    this.detach.push(() => target.removeEventListener(name, handler));
  }

  private emit(event: BackendEvent) {
    if (this.disposed) return;
    this.listeners.forEach(listener => listener(event));
  }

  private onMediaEvent(name: (typeof MEDIA_EVENTS)[number]) {
    switch (name) {
      case 'loadedmetadata':
        if (this.pendingStart > 0 && Number.isFinite(this.video.duration)) {
          // Resume: position before the first frame so playback starts at the saved point.
          const end = this.video.duration;
          this.video.currentTime = Math.min(this.pendingStart, Math.max(0, end - 5));
        }
        this.pendingStart = 0;
        this.emit({ type: 'metadata' });
        this.emit({ type: 'tracks' });
        return;
      case 'durationchange': return this.emit({ type: 'time' });
      case 'canplay': return this.emit({ type: 'canplay' });
      case 'playing':
        this.emit({ type: 'playing' });
        return this.watchFirstFrame();
      case 'waiting': return this.emit({ type: 'waiting' });
      case 'pause': return this.emit({ type: 'pause' });
      case 'seeking': return this.emit({ type: 'seeking' });
      case 'seeked': return this.emit({ type: 'seeked' });
      case 'ended': return this.emit({ type: 'ended' });
      case 'timeupdate': return this.emit({ type: 'time' });
      case 'progress': return this.emit({ type: 'progress' });
      case 'error': {
        // An error event with no src is the teardown in stop(); ignore it.
        if (!this.video.getAttribute('src')) return;
        return this.emit({ type: 'error', error: mediaError(this.video.error) });
      }
    }
  }

  private watchFirstFrame() {
    if (this.firstFrameSent) return;
    this.firstFrameSent = true;
    if (typeof this.video.requestVideoFrameCallback === 'function') {
      this.frameHandle = this.video.requestVideoFrameCallback(() => {
        this.frameHandle = undefined;
        this.emit({ type: 'first-frame' });
      });
    } else {
      // Older webOS engines: `playing` is the closest public signal to a presented frame.
      this.emit({ type: 'first-frame' });
    }
  }

  private cancelFrameCallback() {
    if (this.frameHandle !== undefined) this.video.cancelVideoFrameCallback?.(this.frameHandle);
    this.frameHandle = undefined;
  }
}

function trackId(id: string | undefined, prefix: string, index: number) {
  return id ? `${prefix}:${id}` : `${prefix}#${index}`;
}

/** End of the buffered range that contains the playhead (not simply the last range). */
function bufferedAhead(video: HTMLVideoElement): number {
  const t = video.currentTime;
  for (let i = 0; i < video.buffered.length; i++) {
    if (video.buffered.start(i) <= t + 0.5 && video.buffered.end(i) >= t) return video.buffered.end(i);
  }
  return 0;
}

export function mediaError(error: MediaError | null): BunaError {
  switch (error?.code) {
    case 1: return { code: 'aborted', message: 'Loading was cancelled.', retryable: true };
    case 2: return { code: 'network', message: 'The connection to the video server failed.', retryable: true };
    case 3: return { code: 'decode', message: "The TV couldn't decode this video. Another source may use a format it supports.", retryable: false };
    case 4: return {
      code: 'unsupported',
      message: "The TV can't open this file. The format may be unsupported or the link may no longer be valid.",
      retryable: false,
    };
    default: return { code: 'unknown', message: error?.message || 'Playback failed.', retryable: true };
  }
}
