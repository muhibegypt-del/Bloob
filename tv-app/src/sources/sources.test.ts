import { describe, expect, it } from 'vitest';
import type { SourceCandidate } from '../buna/types';
import { details, headline, rankCandidates } from './ranker';
import { pickFile, sleep, transportFor, type FileOption } from './resolver';

let n = 0;
const candidate = (overrides: Partial<SourceCandidate>): SourceCandidate => ({
  id: `c${++n}`, provider: 'Test', kind: 'magnet', value: String(n).padStart(40, 'a'), release: 'Release',
  claims: {}, languages: [], claimsFrom: 'release-name', seeders: 100, ...overrides,
});

describe('rankCandidates', () => {
  it('recommends the best picture with healthy availability', () => {
    const uhd = candidate({ claims: { resolution: '4K', hdr: ['DV'], audio: ['Atmos'] }, sourceType: 'WEB-DL', seeders: 200 });
    const hd = candidate({ claims: { resolution: '1080p' }, seeders: 900 });
    const ranking = rankCandidates([hd, uhd]);
    expect(ranking.recommended?.candidate).toBe(uhd);
    expect(ranking.why).toBe('The best picture on offer, with strong availability.');
    expect(ranking.recommended?.notes).toEqual(['Dolby Vision', 'Dolby Atmos']);
    expect(ranking.groups.map(g => g.label)).toEqual(['1080p']);
  });

  it('never recommends cinema recordings or dead torrents', () => {
    const cam = candidate({ claims: { resolution: '4K' }, sourceType: 'CAM', seeders: 5000 });
    const dead = candidate({ claims: { resolution: '4K' }, seeders: 0 });
    const ok = candidate({ claims: { resolution: '720p' }, seeders: 20 });
    const ranking = rankCandidates([cam, dead, ok]);
    expect(ranking.recommended?.candidate).toBe(ok);
    expect(ranking.why).toBe('The best picture on offer, with good availability.');
    const listed = ranking.groups.flatMap(g => g.items);
    expect(listed.find(r => r.candidate === cam)?.warnings[0]).toMatch(/cinema/);
    expect(listed.find(r => r.candidate === dead)?.excluded).toBe(true);
  });

  it('prefers a release in the viewer language', () => {
    const dubbed = candidate({ claims: { resolution: '1080p' }, languages: ['Italian'], seeders: 500 });
    const english = candidate({ claims: { resolution: '1080p' }, seeders: 100 });
    expect(rankCandidates([dubbed, english]).recommended?.candidate).toBe(english);
    const multi = candidate({ claims: { resolution: '1080p' }, languages: ['Italian', 'English'], seeders: 500 });
    expect(rankCandidates([multi, english]).recommended?.candidate).toBe(multi);
  });

  it('returns no recommendation when nothing is usable', () => {
    expect(rankCandidates([candidate({ sourceType: 'TS' })]).recommended).toBeNull();
    expect(rankCandidates([]).recommended).toBeNull();
  });

  it('describes candidates in viewer terms', () => {
    const c = candidate({ claims: { resolution: '4K', hdr: ['HDR10'], videoCodec: 'HEVC', audio: ['Atmos', 'Dolby Digital Plus'] }, sourceType: 'WEB-DL', sizeBytes: 18e9, seeders: 42 });
    expect(headline(c)).toBe('4K · HDR10 · Atmos');
    expect(details(c, b => (b ? `${b / 1e9} GB` : ''))).toBe('WEB-DL · HEVC · Dolby Digital Plus · 18 GB · 42 seeders');
    expect(headline(candidate({}))).toBe('Unknown quality');
  });
});

describe('pickFile', () => {
  const f = (name: string, size: number): FileOption => ({ name, size, link: `https://host/${name}` });

  it('uses the exact file the indexer named', () => {
    const files = [f('Show.S01E01.mkv', 3e9), f('Show.S01E02.mkv', 2e9)];
    expect(pickFile(files, { fileName: 'Show.S01E02.mkv', season: 1, episode: 1 })?.name).toBe('Show.S01E02.mkv');
  });

  it('ignores samples and extras', () => {
    expect(pickFile([f('Movie.mkv', 8e9), f('Movie.sample.mkv', 9e9), f('Trailer.mp4', 1e8)], {})?.name).toBe('Movie.mkv');
  });

  it('matches SxxEyy and NxNN episode naming without matching E10 for E1', () => {
    const files = [f('Show.S02E10.mkv', 2e9), f('Show.S02E01.mkv', 2e9), f('Show.2x03.mkv', 2e9)];
    expect(pickFile(files, { season: 2, episode: 1 })?.name).toBe('Show.S02E01.mkv');
    expect(pickFile(files, { season: 2, episode: 3 })?.name).toBe('Show.2x03.mkv');
    expect(pickFile(files, { season: 2, episode: 4 })).toBeNull();
  });

  it('asks the viewer when no file clearly dominates', () => {
    expect(pickFile([f('Part1.mkv', 4e9), f('Part2.mkv', 3.9e9)], {})).toBeNull();
    expect(pickFile([f('Feature.mkv', 8e9), f('Bonus.mkv', 1e9)], {})?.name).toBe('Feature.mkv');
  });
});

describe('helpers', () => {
  it('detects transport from file names and URLs', () => {
    expect(transportFor('https://x/y/master.m3u8?token=1')).toBe('hls');
    expect(transportFor('Movie.mkv')).toBe('progressive');
    expect(transportFor('https://x/manifest.mpd')).toBe('dash');
  });

  it('sleep rejects promptly when cancelled', async () => {
    const controller = new AbortController();
    const waiting = sleep(60_000, controller.signal);
    controller.abort();
    await expect(waiting).rejects.toMatchObject({ name: 'AbortError' });
  });
});
