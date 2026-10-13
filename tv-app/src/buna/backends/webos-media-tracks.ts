import { fail, ok, type CommandResult, type Track } from '../types';

// A webOS video element can expose the native media pipeline even when its DOM
// audioTracks/textTracks lists are empty. This adapter only observes the mediaId
// belonging to our element; it never searches or controls another pipeline.
type NativeVideo = HTMLVideoElement & { mediaId?: string };
type Bridge = {
  onservicecallback: ((message: string) => void) | null;
  call(url: string, parameters: string): void;
  cancel(): void;
};
type BridgeConstructor = new () => Bridge;
type NativeTrack = Track & { index: number };

const MEDIA_SERVICE = 'luna://com.webos.media/';
const VALID_MEDIA_ID = (id: unknown): id is string =>
  typeof id === 'string' && !!id && id !== '<invalid mediaId>';

function record(value: unknown): Record<string, unknown> | null {
  return value && typeof value === 'object' && !Array.isArray(value) ? value as Record<string, unknown> : null;
}

function entries(value: unknown): Record<string, unknown>[] {
  return Array.isArray(value) ? value.map(record).filter((item): item is Record<string, unknown> => !!item) : [];
}

function string(value: unknown): string {
  return typeof value === 'string' ? value : '';
}

/** Parse only indexed tracks from the sourceInfo event for this mediaId. */
export function parseSourceTracks(message: unknown): { audio: NativeTrack[]; text: NativeTrack[] } | null {
  const source = record(record(message)?.sourceInfo);
  if (!source) return null;
  const program = entries(source.programInfo)[0]; // webOS TV shape
  const audio = program ? entries(program.audioTrackInfo) : entries(source.audio_streams); // OSE shape
  const text = program ? entries(program.subtitleTrackInfo ?? program.textTrackInfo) : entries(source.text_streams);
  const parse = (tracks: Record<string, unknown>[], prefix: string): NativeTrack[] => tracks.map((item, index) => ({
    id: `native-${prefix}#${index}`,
    index,
    label: string(item.label || item.name || item.codec),
    language: string(item.language || item.lang),
    selected: item.selected === true || item.enabled === true,
  }));
  return { audio: parse(audio, 'a'), text: parse(text, 't') };
}

function bridgeConstructor(): BridgeConstructor | null {
  const global = window as Window & { PalmServiceBridge?: BridgeConstructor; WebOSServiceBridge?: BridgeConstructor };
  return global.PalmServiceBridge || global.WebOSServiceBridge || null;
}

export class WebosMediaTracks {
  private readonly video: NativeVideo;
  private readonly changed: () => void;
  private readonly Constructor: BridgeConstructor | null;
  private subscription: Bridge | null = null;
  private requestBridge: Bridge | null = null;
  private finishRequest: ((result: CommandResult) => void) | null = null;
  private poll: number | undefined;
  private mediaId: string | null = null;
  private audio: NativeTrack[] = [];
  private text: NativeTrack[] = [];
  private selectedAudio: string | null = null;
  private selectedText: string | null = null;
  private subtitleOff = false;
  private disposed = false;

  constructor(video: HTMLVideoElement, changed: () => void) {
    this.video = video as NativeVideo;
    this.changed = changed;
    this.Constructor = bridgeConstructor();
  }

  start(): void {
    this.stop();
    if (!this.Constructor || this.disposed) return;
    let attempts = 0;
    const check = () => {
      if (this.disposed) return;
      if (VALID_MEDIA_ID(this.video.mediaId)) {
        window.clearInterval(this.poll);
        this.poll = undefined;
        this.subscribe(this.video.mediaId);
      } else if (++attempts >= 100) { // Do not poll forever on unsupported firmware.
        window.clearInterval(this.poll);
        this.poll = undefined;
      }
    };
    check();
    if (!this.mediaId) this.poll = window.setInterval(check, 100);
  }

  audioTracks(): Track[] {
    return this.audio.map(track => ({ ...track, selected: this.selectedAudio === track.id || (!this.selectedAudio && track.selected) }));
  }

  textTracks(): Track[] {
    return this.text.map(track => ({ ...track, selected: !this.subtitleOff && (this.selectedText === track.id || (!this.selectedText && track.selected)) }));
  }

  canSelectAudio(): boolean { return !!this.mediaId && this.audio.length > 1 && !!this.Constructor; }
  canSelectText(): boolean { return !!this.mediaId && this.text.length > 0 && !!this.Constructor; }

  async selectAudio(id: string): Promise<CommandResult> {
    const track = this.audio.find(item => item.id === id);
    if (!track || !this.canSelectAudio()) return fail('unsupported', 'This audio track is unavailable');
    const result = await this.request('selectTrack', { mediaId: this.mediaId, type: 'audio', index: track.index });
    if (result.ok) { this.selectedAudio = id; this.changed(); }
    return result;
  }

  async selectText(id: string | null): Promise<CommandResult> {
    if (!this.canSelectText()) return fail('unsupported', 'Embedded subtitle selection is unavailable');
    const track = id === null ? null : this.text.find(item => item.id === id);
    if (id !== null && !track) return fail('failed', 'Subtitle track not found');
    if (track) {
      const selected = await this.request('selectTrack', { mediaId: this.mediaId, type: 'text', index: track.index });
      if (!selected.ok) return selected;
    }
    const enabled = await this.request('setSubtitleEnable', { mediaId: this.mediaId, enable: !!track });
    if (enabled.ok) {
      this.subtitleOff = !track;
      this.selectedText = track?.id || null;
      this.changed();
    }
    return enabled;
  }

  stop(): void {
    this.requestBridge?.cancel();
    this.finishRequest?.(fail('not-ready', 'The video source changed'));
    window.clearInterval(this.poll);
    this.poll = undefined;
    this.subscription?.cancel();
    this.subscription = null;
    this.mediaId = null;
    this.audio = [];
    this.text = [];
    this.selectedAudio = null;
    this.selectedText = null;
    this.subtitleOff = false;
  }

  dispose(): void {
    this.disposed = true;
    this.stop();
  }

  private subscribe(mediaId: string): void {
    if (!this.Constructor || this.disposed) return;
    this.mediaId = mediaId;
    const bridge = new this.Constructor();
    this.subscription = bridge;
    bridge.onservicecallback = raw => {
      if (this.disposed || this.subscription !== bridge || this.video.mediaId !== mediaId) return;
      let event: unknown;
      try { event = JSON.parse(raw); } catch { return; }
      const response = record(event);
      if (response?.returnValue === false) { this.stop(); this.changed(); return; }
      const tracks = parseSourceTracks(event);
      if (!tracks) return;
      this.audio = tracks.audio;
      this.text = tracks.text;
      this.changed();
    };
    try { bridge.call(`${MEDIA_SERVICE}subscribe`, JSON.stringify({ mediaId, subscribe: true })); }
    catch { this.stop(); }
  }

  private request(method: string, parameters: Record<string, unknown>): Promise<CommandResult> {
    if (!this.Constructor || this.disposed || !this.mediaId) return Promise.resolve(fail('unsupported', 'The TV media service is unavailable'));
    if (this.requestBridge) return Promise.resolve(fail('not-ready', 'A track change is still in progress'));
    return new Promise(resolve => {
      const bridge = new this.Constructor!();
      this.requestBridge = bridge;
      let settled = false;
      const finish = (result: CommandResult) => {
        if (settled) return;
        settled = true;
        window.clearTimeout(timeout);
        if (this.requestBridge === bridge) this.requestBridge = null;
        if (this.finishRequest === finish) this.finishRequest = null;
        resolve(this.disposed ? fail('disposed', 'Playback has ended') : result);
      };
      this.finishRequest = finish;
      const timeout = window.setTimeout(() => { bridge.cancel(); finish(fail('failed', 'The TV did not confirm the track change')); }, 4000);
      bridge.onservicecallback = raw => {
        let response: Record<string, unknown> | null = null;
        try { response = record(JSON.parse(raw)); } catch { /* malformed service reply */ }
        finish(response?.returnValue === true ? ok : fail('failed', string(response?.errorText) || 'The TV rejected the track change'));
      };
      try { bridge.call(`${MEDIA_SERVICE}${method}`, JSON.stringify(parameters)); }
      catch { finish(fail('failed', 'The TV media service could not be called')); }
    });
  }
}
