import { describe, expect, it } from 'vitest';
import sourcesModule from '../../service/sources.js';
import rankModule from '../../service/rank.js';
import profileModule from '../../service/tv-profile.js';

const { discoverAll, fitsEpisode, merge, candidatesFrom } = sourcesModule;
const { rank, dolbyVisionProfile } = rankModule;
const { PROFILES } = profileModule;

const hash = ch => ch.repeat(40);

describe('episode filter for show-wide indexes', () => {
  it('keeps the episode, its season pack and ranges that include it', () => {
    expect(fitsEpisode('Show.S01E10.2160p.WEB-DL', 1, 10)).toBe(true);
    expect(fitsEpisode('Show 1x10 720p', 1, 10)).toBe(true);
    expect(fitsEpisode('Show.S01.1080p.BluRay', 1, 10)).toBe(true);
    expect(fitsEpisode('Show Season 1 Complete 1080p', 1, 10)).toBe(true);
    expect(fitsEpisode('Show S01-S08 2160p', 3, 2)).toBe(true);
    expect(fitsEpisode('Show Complete Series 1080p', 4, 1)).toBe(true);
  });
  it('drops other episodes and other seasons', () => {
    expect(fitsEpisode('Show.S01E01.1080p', 1, 10)).toBe(false);
    expect(fitsEpisode('Show.S01E100.1080p', 1, 10)).toBe(false);
    expect(fitsEpisode('Show.S02.1080p', 1, 10)).toBe(false);
    expect(fitsEpisode('Show S03-S05', 1, 10)).toBe(false);
    expect(fitsEpisode('Show 1080p', 1, 10)).toBe(false);
  });
});

describe('merging indexes', () => {
  const torrentio = { id: 'torrentio' };
  const stremthru = { id: 'stremthru' };

  it('reads plain torrents, keeps pack size out of the bitrate and filters episodes', () => {
    const list = candidatesFrom(stremthru, {
      torrents: [
        { hash: hash('a'), name: 'Show.S01E10.2160p.WEB-DL.DV.HDR.DDP5.1.Atmos-GRP.mkv', fileName: 'Show.S01E10.2160p.WEB-DL.DV.HDR.DDP5.1.Atmos-GRP.mkv', size: 9e9, seeders: 40, languages: ['English'] },
        { hash: hash('b'), name: 'Show.S01.1080p.BluRay.x264-GRP', size: 60e9 },
        { hash: hash('c'), name: 'Show.S01E09.1080p.WEB-DL', size: 2e9, seeders: 9 },
        { hash: 'not-a-hash', name: 'Show.S01E10.720p' },
      ],
    }, 'series', 'tt1:1:10');
    expect(list.map(c => c.value)).toEqual([hash('a'), hash('b')]);
    expect(list[0]).toMatchObject({ provider: 'stremthru', seeders: 40, sizeBytes: 9e9, fileName: expect.stringMatching(/\.mkv$/) });
    expect(list[0].claims.container).toBe('MKV');
    expect(list[1].pack).toBe(true);
    expect(list[1].sizeBytes).toBeUndefined();
    expect(list[1].packBytes).toBe(60e9);
    expect(list[1].seeders).toBeUndefined();  // unknown, not zero
  });

  it('keeps the first copy of a hash and adds the others as providers', () => {
    const a = candidatesFrom(torrentio, { streams: [{ infoHash: hash('a'), fileIdx: 3, title: 'Film.2024.2160p.WEB-DL\n👤 12', behaviorHints: { filename: 'Film.2024.2160p.WEB-DL.mkv' } }] }, 'movie', 'tt1');
    const b = candidatesFrom(stremthru, { torrents: [{ hash: hash('a'), name: 'Film 2024 2160p WEB-DL', seeders: 80 }, { hash: hash('d'), name: 'Film 2024 1080p', seeders: 5 }] }, 'movie', 'tt1');
    const merged = merge([a, b]);
    expect(merged).toHaveLength(2);
    expect(merged[0]).toMatchObject({ id: `magnet:${hash('a')}#3`, fileIdx: 3, seeders: 80, providers: ['torrentio', 'stremthru'] });
  });
});

describe('asking several indexes', () => {
  const later = (ms, value, fail) => () => new Promise((resolve, reject) => setTimeout(() => (fail ? reject(new Error(fail)) : resolve(value)), ms));
  const stream = ch => ({ infoHash: hash(ch), title: `Film.2024.1080p.WEB-DL-${ch}\n👤 10` });

  it('answers a short grace after the primary index, and late answers still arrive', async () => {
    const lateLists = [];
    const started = Date.now();
    const result = await discoverAll('movie', 'tt1', {
      providers: [
        { id: 'torrentio', label: 'Torrentio', primary: true, search: later(10, { streams: [stream('a')] }) },
        { id: 'quick', label: 'Quick', search: later(30, { streams: [stream('b')] }) },
        { id: 'slow', label: 'Slow', search: later(3200, { streams: [stream('c')] }) },
      ],
      onLate: list => lateLists.push(list.length),
    });
    const took = Date.now() - started;
    expect(took).toBeLessThan(3200);
    expect(result.candidates.map(c => c.value)).toEqual([hash('a'), hash('b')]);
    expect(result.report.map(r => r.count)).toEqual([1, 1, 0]);
    await new Promise(r => setTimeout(r, 900));
    expect(lateLists).toEqual([3]);
  }, 10000);

  it('reports failures per index', async () => {
    const result = await discoverAll('movie', 'tt1', {
      providers: [
        { id: 'torrentio', label: 'Torrentio', primary: true, search: later(5, null, 'torrentio.strem.fun returned 403') },
        { id: 'other', label: 'Other', search: later(20, { streams: [stream('b')] }) },
      ],
    });
    expect(result.candidates).toHaveLength(1);
    expect(result.report[0].error).toMatch(/403/);
  });
});

describe('picture preference and Dolby Vision on this TV', () => {
  const c = (id, claims, over) => Object.assign({ id: `magnet:${hash(id)}`, provider: 'T', kind: 'magnet', value: hash(id), release: id, claims, languages: [], seeders: 50, claimsFrom: 'release-name' }, over);
  const byId = (result, id) => [result.recommended].concat(...result.groups.map(g => g.items)).find(v => v.id === `magnet:${hash(id)}`);

  it('guesses the Dolby Vision profile from the name', () => {
    expect(dolbyVisionProfile(c('a', { hdr: ['DV', 'HDR10'] }))).toBe('p8');
    expect(dolbyVisionProfile(c('a', { hdr: ['DV'] }))).toBe('p5');
    expect(dolbyVisionProfile(c('a', { hdr: ['DV', 'HDR10'] }, { sourceType: 'REMUX' }))).toBe('p7');
    expect(dolbyVisionProfile(c('a', { hdr: ['DV'] }, { release: 'Film.2160p.WEB-DL.DV.P8' }))).toBe('p8');
    expect(dolbyVisionProfile(c('a', { hdr: ['HDR10'] }))).toBe('');
  });

  // A TV whose player shows Dolby Vision only from MP4 (not the owner's: theirs shows it from MKV too).
  const mp4Only = Object.assign({}, PROFILES['lg-2022-oled'], { dolbyVisionContainers: ['MP4'] });

  it('shows Dolby Vision in MKV as Dolby Vision on this TV', () => {
    const mkv = c('m', { resolution: '4K', hdr: ['DV', 'HDR10'], container: 'MKV' });
    const unknown = c('u', { resolution: '4K', hdr: ['DV', 'HDR10'] });
    const result = rank([mkv, unknown], {});
    for (const id of ['m', 'u']) {
      expect(byId(result, id).headline).toBe('4K · Dolby Vision');
      expect(byId(result, id).dolbyVision).toBe('p8');
    }
  });

  it('names Dolby Vision as HDR10 on a TV that cannot show it from that container', () => {
    const mkv = c('m', { resolution: '4K', hdr: ['DV', 'HDR10'], container: 'MKV' });
    const mp4 = c('p', { resolution: '4K', hdr: ['DV', 'HDR10'], container: 'MP4' });
    const result = rank([mkv, mp4], { profile: mp4Only });
    expect(result.recommended.id).toBe(mp4.id);
    expect(result.recommended.headline).toBe('4K · Dolby Vision');
    expect(byId(result, 'm').headline).toBe('4K · HDR10');
    expect(byId(result, 'm').picture).toBe('HDR10 (Dolby Vision version)');
    expect(byId(result, 'm').warnings[0]).toMatch(/Dolby Vision in MKV files as HDR10/);
  });

  it('warns about Dolby Vision without an HDR10 layer only where Dolby Vision is not shown', () => {
    const p5 = c('a', { resolution: '4K', hdr: ['DV'] }, { sourceType: 'WEB-DL' });
    const hdr = c('b', { resolution: '4K', hdr: ['HDR10'] });
    expect(rank([p5, hdr], {}).recommended.id).toBe(p5.id);
    const result = rank([p5, hdr], { profile: mp4Only });
    expect(result.recommended.id).toBe(hdr.id);
    expect(byId(result, 'a').cautions).toContain('Colours may be wrong');
    // Asking for HDR10 skips Dolby Vision, so a version without an HDR10 layer is warned about too.
    expect(byId(rank([p5, hdr], { picture: 'hdr10' }), 'a').cautions).toContain('Colours may be wrong');
  });

  it('prefers SDR when asked, over a higher resolution', () => {
    const uhd = c('a', { resolution: '4K', hdr: ['HDR10'] });
    const sdr = c('b', { resolution: '1080p' });
    expect(rank([uhd, sdr], {}).recommended.id).toBe(uhd.id);
    const result = rank([uhd, sdr], { picture: 'sdr' });
    expect(result.recommended.id).toBe(sdr.id);
    expect(result.why).toMatch(/picture setting/);
    expect(rank([uhd], { picture: 'sdr' }).why).toMatch(/No SDR version/);
  });

  it('skips Dolby Vision when HDR10 is asked for', () => {
    const dv = c('a', { resolution: '4K', hdr: ['DV', 'HDR10'], container: 'MP4' });
    const result = rank([dv], { picture: 'hdr10' });
    expect(result.recommended.headline).toBe('4K · HDR10');
    expect(result.recommended.picture).toMatch(/Dolby Vision skipped/);
  });
});
