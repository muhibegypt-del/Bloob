import { describe, expect, it } from 'vitest';
import bridgeModule from '../../service/bridge.js';

const { bridgeTrack, summarise, describe: name } = bridgeModule;

const probe = summarise({
  format: { duration: '7260.5' },
  streams: [
    { codec_type: 'video', codec_name: 'hevc', width: 3840, height: 2160, side_data_list: [{ dv_profile: 8 }] },
    { codec_type: 'audio', codec_name: 'dts', profile: 'DTS-HD MA', channels: 8, tags: { language: 'eng' }, disposition: { default: 1 } },
    { codec_type: 'audio', codec_name: 'dts', profile: 'DTS', channels: 6, tags: { language: 'fre' } },
    { codec_type: 'audio', codec_name: 'ac3', channels: 2, tags: { language: 'eng', title: 'Commentary' } },
    { codec_type: 'subtitle', codec_name: 'subrip', tags: { language: 'eng', title: 'SDH' } },
  ],
  chapters: [{ start_time: '0', end_time: '95.5', tags: { title: 'Intro' } }],
});

describe('the audio bridge', () => {
  it('reads the probe', () => {
    expect(probe.durationMs).toBe(7260500);
    expect(probe.video).toEqual({ codec: 'hevc', width: 3840, height: 2160, dvProfile: 8 });
    expect(probe.audio[2].commentary).toBe(true);
    expect(probe.text[0].sdh).toBe(true);
    expect(probe.chapters[0]).toEqual({ start: 0, end: 95.5, title: 'Intro' });
  });
  it('bridges only when every main track is DTS, in the viewer\'s language first', () => {
    expect(bridgeTrack(probe, 'fr-FR')).toBe(1);
    expect(bridgeTrack(probe, 'en')).toBe(0);
    const withAc3 = Object.assign({}, probe, { audio: probe.audio.concat([{ index: 3, codec: 'eac3', commentary: false }]) });
    expect(bridgeTrack(withAc3, 'en')).toBe(-1);
    expect(bridgeTrack(null, 'en')).toBe(-1);
  });
  it('names the conversion', () => {
    expect(name(probe.audio[0])).toBe('DTS-HD MA 7.1 → Dolby Digital Plus 5.1 (Bloob)');
  });
});
