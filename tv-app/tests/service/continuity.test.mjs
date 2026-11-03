// UX-PLAN X1 (progress), X2 (next action) and X3 (version continuity).
import { mkdtempSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { beforeEach, describe, expect, it } from 'vitest';

process.env.TV_APP_DATA_DIR = mkdtempSync(join(tmpdir(), 'bloob-progress-'));
const completion = (await import('../../service/completion.js')).default;
const progress = (await import('../../service/progress.js')).default;
const { target } = (await import('../../service/next.js')).default;
const { rank } = (await import('../../service/rank.js')).default;

const MIN = 60000;
const DAY = 86400000;

describe('completion policy', () => {
  it('finishes on Up next or the end, else on credits, else near the end', () => {
    expect(completion.isComplete({ state: 'upnext', position: 10 * MIN, duration: 50 * MIN })).toBe(true);
    expect(completion.isComplete({ position: 42 * MIN, duration: 50 * MIN, creditsMs: 41 * MIN })).toBe(true);
    // 45-minute episode: the last 3.15 minutes count.
    expect(completion.isComplete({ position: 42 * MIN, duration: 45 * MIN })).toBe(true);
    expect(completion.isComplete({ position: 40 * MIN, duration: 45 * MIN })).toBe(false);
    // A long film is capped at the last ten minutes, not 7% (12.6 min).
    expect(completion.isComplete({ position: 168 * MIN, duration: 180 * MIN })).toBe(false);
    expect(completion.isComplete({ position: 171 * MIN, duration: 180 * MIN })).toBe(true);
    // Unknown length never completes.
    expect(completion.isComplete({ position: 99 * MIN, duration: 0 })).toBe(false);
  });
});

describe('progress v2', () => {
  beforeEach(() => progress.resetForTests());
  const ep = (over) => Object.assign({ id: 'tt1', videoId: 'tt1:1:1', type: 'series', name: 'Show', season: 1, episode: 1, duration: 45 * MIN }, over);

  it('rejects late reports from the same or an older session', async () => {
    expect((await progress.save(ep({ videoId: 'tt1:1:9', session: 'aaaa', seq: 2, position: 20 * MIN }))).accepted).toBe(true);
    expect((await progress.save(ep({ videoId: 'tt1:1:9', session: 'aaaa', seq: 1, position: 5 * MIN }))).accepted).toBe(false);
    expect((await progress.save(ep({ videoId: 'tt1:1:9', session: 'bbbb', seq: 1, position: 2 * MIN }))).accepted).toBe(true);
    // The first viewing's final flush arrives after the second began.
    expect((await progress.save(ep({ videoId: 'tt1:1:9', session: 'aaaa', seq: 3, position: 30 * MIN }))).accepted).toBe(false);
    expect((await progress.forTitle('tt1'))['tt1:1:9'].position).toBe(2 * MIN);
  });

  it('keeps a finished viewing finished when rewound in the same session', async () => {
    await progress.save(ep({ videoId: 'tt1:1:8', session: 'cccc', seq: 1, state: 'upnext', position: 43 * MIN }));
    const after = await progress.save(ep({ videoId: 'tt1:1:8', session: 'cccc', seq: 2, state: 'paused', position: 30 * MIN }));
    expect(after.entry.watched).toBe(true);
  });

  it('orders Continue: unfinished, new episode, next, caught up; dismissed left out', async () => {
    const now = Date.now();
    const iso = (t) => new Date(t).toISOString();
    await progress.save(ep({ id: 'tt10', videoId: 'tt10:1:1', session: 's10a', seq: 1, state: 'ended', position: 45 * MIN,
      next: { videoId: 'tt10:1:2', season: 1, episode: 2, released: iso(now - 30 * DAY) } }));
    await progress.save(ep({ id: 'tt11', videoId: 'tt11:1:1', session: 's11a', seq: 1, state: 'ended', position: 45 * MIN,
      next: { videoId: 'tt11:1:2', season: 1, episode: 2, released: iso(now + 3 * DAY) } }));
    await progress.save(ep({ id: 'tt12', videoId: 'tt12:1:1', session: 's12a', seq: 1, position: 10 * MIN }));
    await progress.save(ep({ id: 'tt13', videoId: 'tt13:1:1', session: 's13a', seq: 1, position: 10 * MIN }));
    await progress.save({ id: 'tt14', videoId: 'tt14', type: 'movie', name: 'Film', session: 's14a', seq: 1, state: 'ended', position: 120 * MIN, duration: 120 * MIN });
    await progress.dismiss('tt13');
    const list = await progress.continueWatching(now);
    expect(list.map(e => [e.id, e.kind])).toEqual([['tt12', 'resume'], ['tt10', 'next'], ['tt11', 'upcoming']]);
    // Starting a dismissed title again brings it back.
    await progress.save(ep({ id: 'tt13', videoId: 'tt13:1:1', session: 's13b', seq: 1, position: 12 * MIN }));
    expect((await progress.continueWatching(now + 1000)).map(e => e.id)).toContain('tt13');
  });

  it('keeps to a chosen version, else the version last played', async () => {
    const edition = { id: 'magnet:' + 'b'.repeat(40) + '#3', hash: 'b'.repeat(40), resolution: '4K', hdr: 'DV' };
    await progress.save(ep({ id: 'tt20', videoId: 'tt20:1:1', session: 's20a', seq: 1, position: 5 * MIN }), edition);
    expect(await progress.intentFor('tt20')).toMatchObject({ from: 'played', resolution: '4K' });
    await progress.setIntent('tt20', { hash: '', resolution: '1080p', hdr: 'SDR' });
    expect(await progress.intentFor('tt20')).toMatchObject({ from: 'chosen', resolution: '1080p' });
    await progress.setIntent('tt20', null);
    expect(await progress.intentFor('tt20')).toMatchObject({ from: 'played' });
  });
});

describe('next action', () => {
  const now = Date.parse('2026-10-15T12:00:00Z');
  const e = (s, n, released) => ({ id: `tt5:${s}:${n}`, season: s, episode: n, title: `E${n}`, released });
  const meta = {
    id: 'tt5', type: 'series',
    seasons: [
      { season: 1, episodes: [e(1, 1, '2026-01-01'), e(1, 2, '2026-01-08')] },
      { season: 2, episodes: [e(2, 1, '2026-10-01'), e(2, 2, '2026-10-22')] },
      { season: 0, episodes: [e(0, 1, '2025-12-01')] },
    ],
  };

  it('starts with the first aired regular episode', () => {
    expect(target(meta, {}, now)).toMatchObject({ action: 'start', videoId: 'tt5:1:1' });
  });

  it('resumes the latest unfinished viewing, else the next after the furthest finished', () => {
    const p = {
      'tt5:1:1': { position: 44 * MIN, duration: 45 * MIN, watched: true, updated: 1 },
      'tt5:1:2': { position: 10 * MIN, duration: 45 * MIN, watched: false, updated: 2 },
    };
    expect(target(meta, p, now)).toMatchObject({ action: 'resume', videoId: 'tt5:1:2', position: 10 * MIN });
    p['tt5:1:2'] = { position: 45 * MIN, duration: 45 * MIN, watched: true, updated: 3 };
    // Rewatching an early episode doesn't move you back.
    p['tt5:1:1'] = { position: 45 * MIN, duration: 45 * MIN, watched: true, updated: 4 };
    expect(target(meta, p, now)).toMatchObject({ action: 'next', videoId: 'tt5:2:1' });
  });

  it('is caught up when the next episode has not aired', () => {
    const p = { 'tt5:2:1': { position: 45 * MIN, duration: 45 * MIN, watched: true, updated: 5 } };
    const t = target(meta, p, now);
    expect(t.action).toBe('caughtUp');
    expect(t.upcoming).toMatchObject({ videoId: 'tt5:2:2', released: '2026-10-22' });
    expect(t.again).toMatchObject({ videoId: 'tt5:1:1' });
  });
});

describe('version continuity', () => {
  const HASH_A = 'a'.repeat(40);
  const c = (id, over) => Object.assign({ id: `magnet:${id.repeat(40).slice(0, 40)}#0`, provider: 'T', kind: 'magnet', value: id.repeat(40).slice(0, 40),
    release: id, claims: {}, languages: [], claimsFrom: 'release-name' }, over);

  it('keeps the next episode in the 4K version, even against a better-seeded 1080p', () => {
    const uhd = c('u', { claims: { resolution: '4K', hdr: ['DV', 'HDR10'] }, seeders: 12 });
    const hd = c('h', { claims: { resolution: '1080p' }, seeders: 2000 });
    expect(rank([uhd, hd], {}).recommended.id).toBe(uhd.id);
    const intent = { resolution: '4K', hdr: 'DV' };
    const result = rank([uhd, hd], { intent });
    expect(result.recommended).toMatchObject({ id: uhd.id, match: 'like' });
    expect(result.why).toMatch(/last episode/);
  });

  it('prefers the same season pack, and lists other matches first', () => {
    const pack = c('a', { claims: { resolution: '4K', hdr: ['DV', 'HDR10'] }, seeders: 8, pack: true });
    const other = c('b', { claims: { resolution: '4K', hdr: ['DV', 'HDR10'] }, seeders: 300 });
    const lower = c('d', { claims: { resolution: '1080p' }, seeders: 900 });
    const result = rank([other, pack, lower], { intent: { hash: HASH_A, resolution: '4K', hdr: 'DV' } });
    expect(result.recommended).toMatchObject({ id: pack.id, match: 'same', scope: 'pack' });
    expect(result.why).toBe('The same version as your last episode.');
    expect(result.groups.map(g => g.label)).toEqual(['Matches your last episode', 'Other versions']);
  });

  it('says so when the last version is not offered', () => {
    const hd = c('h', { claims: { resolution: '1080p' }, seeders: 50 });
    expect(rank([hd], { intent: { resolution: '4K', hdr: 'DV' } }).why).toMatch(/isn't offered/);
  });
});
