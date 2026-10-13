import { afterEach, describe, expect, it, vi } from 'vitest';
import { parseSourceTracks, WebosMediaTracks } from './webos-media-tracks';

afterEach(() => vi.unstubAllGlobals());

describe('webOS track metadata', () => {
  it('maps indexed TV sourceInfo tracks without inventing missing metadata', () => {
    expect(parseSourceTracks({ sourceInfo: { programInfo: [{
      audioTrackInfo: [{ language: 'eng', codec: 'eac3' }, { language: 'spa', label: 'Spanish' }],
      subtitleTrackInfo: [{ language: 'eng', label: 'English CC' }],
    }] } })).toEqual({
      audio: [
        { id: 'native-a#0', index: 0, label: 'eac3', language: 'eng', selected: false },
        { id: 'native-a#1', index: 1, label: 'Spanish', language: 'spa', selected: false },
      ],
      text: [{ id: 'native-t#0', index: 0, label: 'English CC', language: 'eng', selected: false }],
    });
    expect(parseSourceTracks({ returnValue: true })).toBeNull();
  });

  it('only reports a changed selection after the TV confirms it', async () => {
    class FakeBridge {
      static calls: FakeBridge[] = [];
      onservicecallback: ((message: string) => void) | null = null;
      url = '';
      parameters = '';
      cancelled = false;
      constructor() { FakeBridge.calls.push(this); }
      call(url: string, parameters: string) { this.url = url; this.parameters = parameters; }
      cancel() { this.cancelled = true; }
      reply(value: unknown) { this.onservicecallback?.(JSON.stringify(value)); }
    }
    vi.stubGlobal('window', { PalmServiceBridge: FakeBridge, setInterval, clearInterval, setTimeout, clearTimeout });
    const video = { mediaId: 'our-pipeline' } as unknown as HTMLVideoElement;
    const changed = vi.fn();
    const adapter = new WebosMediaTracks(video, changed);
    adapter.start();
    expect(FakeBridge.calls[0].url).toBe('luna://com.webos.media/subscribe');
    expect(JSON.parse(FakeBridge.calls[0].parameters)).toEqual({ mediaId: 'our-pipeline', subscribe: true });
    FakeBridge.calls[0].reply({ sourceInfo: { programInfo: [{ audioTrackInfo: [{ language: 'eng' }, { language: 'fra' }] }] } });
    expect(adapter.canSelectAudio()).toBe(true);
    const failed = adapter.selectAudio('native-a#1');
    expect(JSON.parse(FakeBridge.calls[1].parameters)).toEqual({ mediaId: 'our-pipeline', type: 'audio', index: 1 });
    FakeBridge.calls[1].reply({ returnValue: false, errorText: 'Unavailable' });
    expect(await failed).toMatchObject({ ok: false, message: 'Unavailable' });
    expect(adapter.audioTracks().some(track => track.selected)).toBe(false);
    const success = adapter.selectAudio('native-a#1');
    FakeBridge.calls[2].reply({ returnValue: true });
    expect(await success).toEqual({ ok: true });
    expect(adapter.audioTracks().find(track => track.selected)?.language).toBe('fra');
    adapter.dispose();
    expect(FakeBridge.calls[0].cancelled).toBe(true);
  });
});
