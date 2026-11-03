import { describe, expect, it } from 'vitest';
import skipsModule from '../../service/skips.js';

const { fromTheIntroDb, fromIntroDb, fromAniSkip } = skipsModule;

describe('skip intro sources (4.0)', () => {
  it('reads TheIntroDB, with credits that run to the end', () => {
    const out = fromTheIntroDb({ intro: [{ start_ms: 314568, end_ms: 330969 }], credits: [{ start_ms: 2843000, end_ms: null }] });
    expect(out).toEqual([{ kind: 'intro', startMs: 314568, endMs: 330969 }, { kind: 'credits', startMs: 2843000, endMs: -1 }]);
  });

  it('reads IntroDB and ignores empty segments', () => {
    const out = fromIntroDb({ intro: { start_ms: 314500, end_ms: 331000 }, recap: null, outro: { start_ms: 2843000, end_ms: 2901000 }, no_recap: true });
    expect(out).toEqual([{ kind: 'intro', startMs: 314500, endMs: 331000 }, { kind: 'credits', startMs: 2843000, endMs: 2901000 }]);
  });

  it('refuses a segment that ends before it starts', () => {
    expect(fromIntroDb({ intro: { start_ms: 5000, end_ms: 4000 } })).toEqual([]);
    expect(fromTheIntroDb(null)).toEqual([]);
  });

  it('picks the AniSkip submission made for this cut', () => {
    const j = { found: true, results: [
      { interval: { startTime: 47.365, endTime: 137.365 }, skipType: 'op', episodeLength: 1540.061 },
      { interval: { startTime: 75, endTime: 135 }, skipType: 'mixed-op', episodeLength: 1420 },
      { interval: { startTime: 1342.795, endTime: 1430.616 }, skipType: 'ed', episodeLength: 1446.9973 },
      { interval: { startTime: 10, endTime: 20 }, skipType: 'recap', episodeLength: 2900 },  // another cut
    ] };
    const out = fromAniSkip(j, 1421000);
    expect(out).toEqual([{ kind: 'intro', startMs: 75000, endMs: 135000 }, { kind: 'credits', startMs: 1342795, endMs: 1430616 }]);
    // Without the file's length, the plain opening beats the mixed one.
    expect(fromAniSkip(j, 0)[0]).toEqual({ kind: 'intro', startMs: 47365, endMs: 137365 });
  });
});
