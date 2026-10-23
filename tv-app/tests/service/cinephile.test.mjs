import { describe, expect, it } from 'vitest';
import cineModule from '../../service/cinephile.js';
import rankModule from '../../service/rank.js';

const { cutOf, tagsOf, tierOf, compare } = cineModule;
const { rank } = rankModule;
const h = ch => ch.repeat(40);
const c = (id, release, claims, over) => Object.assign({ id: `magnet:${h(id)}`, provider: 'torrentio', kind: 'magnet', value: h(id), release, claims, languages: [], seeders: 50, claimsFrom: 'release-name' }, over);

describe('cuts and tags from release names', () => {
  it('reads the cut, and treats a release that says nothing as theatrical', () => {
    expect(cutOf({ release: 'LOTR.2001.EXTENDED.2160p.UHD.BluRay.REMUX' })).toBe('Extended');
    expect(cutOf({ release: 'Blade.Runner.1982.The.Final.Cut.2160p' })).toBe('Final Cut');
    expect(cutOf({ release: 'Film.2010.Directors.Cut.1080p' })).toBe('Director’s Cut');
    expect(cutOf({ release: 'Film.2010.Uncut.1080p' })).toBe('Unrated');
    expect(cutOf({ release: 'Film.2010.1080p.BluRay' })).toBe('');
    expect(cutOf({ release: 'DC.League.of.Super-Pets.2022.1080p' })).toBe('');
    expect(tagsOf({ release: 'Film.2010.2160p.Hybrid.Criterion.REMUX' })).toEqual(['Hybrid', 'Criterion']);
  });
  it('places a version in a cinephile tier', () => {
    expect(tierOf({ sourceType: 'REMUX', claims: { resolution: '4K' } })).toBe('uhd-remux');
    expect(tierOf({ sourceType: 'WEB-DL', claims: { resolution: '1080p' } })).toBe('web-hd');
    expect(tierOf({ sourceType: 'HDTV', claims: { resolution: '720p' } })).toBe('other');
  });
});

describe('the cinephile versions list', () => {
  const remux = c('a', 'Film.2001.Extended.2160p.UHD.BluRay.REMUX.DV.TrueHD.7.1.Atmos-FraMeSToR',
    { resolution: '4K', hdr: ['DV', 'HDR10'], audio: ['TrueHD', 'Atmos'], channels: '7.1' }, { sourceType: 'REMUX', group: 'FraMeSToR', sizeBytes: 120e9 });
  const sameElsewhere = Object.assign({}, remux, { id: `magnet:${h('b')}`, value: h('b'), provider: 'stremthru', sizeBytes: 121e9 });
  const theatrical = c('c', 'Film.2001.2160p.WEB-DL.DV.DDP5.1.Atmos-GRP',
    { resolution: '4K', hdr: ['DV'], audio: ['DD+', 'Atmos'], channels: '5.1' }, { sourceType: 'WEB-DL', group: 'GRP', sizeBytes: 20e9 });

  it('merges one release found twice, and lists the cuts on offer', () => {
    const r = rank([remux, sameElsewhere, theatrical], {});
    const all = [r.recommended].concat(...r.groups.map(g => g.items));
    expect(all).toHaveLength(2);
    const v = all.find(x => x.group === 'FraMeSToR');
    expect(v.dupes).toBe(1);
    expect(v.sources).toEqual(['torrentio', 'stremthru']);
    expect(v.line).toBe('HDR10 (Dolby Vision P7 disc) · TrueHD Atmos 7.1 · 120 GB · FraMeSToR · Extended');
    expect(v.respected).toBe(true);
    expect(r.cuts).toEqual([{ name: 'Theatrical', count: 1 }, { name: 'Extended', count: 1 }]);
  });

  it('compares a version with the best of its own cut, in plain words', () => {
    const best = { id: 'x', tier: 'uhd-remux', resolution: '4K', cut: '', pictureName: 'Dolby Vision P7', audio: 'TrueHD Atmos 7.1', lossless: true, bytes: 100e9 };
    const web = { id: 'y', tier: 'web-4k', resolution: '4K', cut: '', pictureName: 'Dolby Vision P8', audio: 'DD+ Atmos 5.1', lossless: false, bytes: 20e9, ready: true };
    expect(compare(web, best)).toBe('Web version, lossy audio, 80% smaller, ready now');
    expect(compare(best, best)).toBe('');
  });

  it('prefers a version that is ready now only between equals', () => {
    const a = c('d', 'Film.2001.1080p.WEB-DL.DDP5.1-AAA', { resolution: '1080p', audio: ['DD+'], channels: '5.1' }, { sourceType: 'WEB-DL', group: 'AAA', sizeBytes: 8e9 });
    const b = Object.assign(c('e', 'Film.2001.1080p.WEB-DL.DDP5.1-BBB', { resolution: '1080p', audio: ['DD+'], channels: '5.1' }, { sourceType: 'WEB-DL', group: 'BBB', sizeBytes: 8e9 }), { ready: true });
    expect(rank([a, b], {}).recommended.id).toBe(b.id);
    const uhd = c('f', 'Film.2001.2160p.WEB-DL.DDP5.1-CCC', { resolution: '4K', audio: ['DD+'], channels: '5.1' }, { sourceType: 'WEB-DL', group: 'CCC', sizeBytes: 20e9 });
    expect(rank([uhd, b], {}).recommended.id).toBe(uhd.id);
  });
});
