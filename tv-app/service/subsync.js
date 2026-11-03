// Subtitle timing (4.0): lines a subtitle up with a reference made for the
// very file being played (another subtitle in any language: only where lines
// start and stop counts). Bloob's own method:
//   - each subtitle is a list of spoken intervals;
//   - for each frame-rate ratio a release can be off by (23.976, 24 and 25
//     fps mixed up) and each shift within two minutes, the score is how much
//     of the subtitle's speech falls on the reference's speech;
//   - a coarse pass every 0.5 s, then a fine one every 20 ms around the best;
//   - the result is used only when it fits clearly better than leaving the
//     subtitle as it is, so a subtitle that was already right stays right.
// Pure: tests/service/subsync.test.mjs. Keep this file compatible with the
// TV's older Node runtime (no `?.` / `??`).

const RATIOS = [1, 25 / 23.976, 23.976 / 25, 24 / 23.976, 23.976 / 24, 25 / 24, 24 / 25];
const RANGE_MS = 120000;

function spans(cues) {
  return cues.map(c => [c.s, c.e]).filter(x => x[1] > x[0]).sort((a, b) => a[0] - b[0]);
}

function total(list) {
  let t = 0;
  for (let i = 0; i < list.length; i++) t += list[i][1] - list[i][0];
  return t;
}

/** Overlap (ms) between `a` mapped by t*ratio+shift and `b`; both sorted. */
function overlap(a, b, ratio, shift) {
  let i = 0, j = 0, sum = 0;
  while (i < a.length && j < b.length) {
    const s1 = a[i][0] * ratio + shift, e1 = a[i][1] * ratio + shift;
    const s2 = b[j][0], e2 = b[j][1];
    const lo = s1 > s2 ? s1 : s2, hi = e1 < e2 ? e1 : e2;
    if (hi > lo) sum += hi - lo;
    if (e1 < e2) i++; else j++;
  }
  return sum;
}

/**
 * How to move `target` cues ({s, e} in ms) onto `reference`: { ratio, shiftMs,
 * score, before, changed }. `changed` is false when the subtitle is better left
 * alone (too little to go on, no clearly better fit, or already in place).
 */
function align(target, reference) {
  const a = spans(target), b = spans(reference);
  const none = { ratio: 1, shiftMs: 0, score: 0, before: 0, changed: false };
  if (a.length < 20 || b.length < 20) return none;
  const norm = Math.min(total(a), total(b)) || 1;
  const before = overlap(a, b, 1, 0) / norm;
  let best = { ratio: 1, shift: 0, score: before };
  RATIOS.forEach(ratio => {
    for (let shift = -RANGE_MS; shift <= RANGE_MS; shift += 500) {
      const sc = overlap(a, b, ratio, shift) / norm;
      if (sc > best.score) best = { ratio, shift, score: sc };
    }
  });
  const coarse = best;
  for (let shift = coarse.shift - 600; shift <= coarse.shift + 600; shift += 20) {
    const sc = overlap(a, b, coarse.ratio, shift) / norm;
    if (sc > best.score) best = { ratio: coarse.ratio, shift, score: sc };
  }
  const moved = best.ratio !== 1 || Math.abs(best.shift) >= 150;
  const changed = moved && best.score >= 0.35 && best.score - before >= 0.08;
  return { ratio: best.ratio, shiftMs: Math.round(best.shift), score: Math.round(best.score * 1000) / 1000, before: Math.round(before * 1000) / 1000, changed };
}

/** The cues moved by an alignment. */
function apply(cues, a) {
  if (!a || !a.changed) return cues;
  return cues.map(c => Object.assign({}, c, { s: Math.round(c.s * a.ratio + a.shiftMs), e: Math.round(c.e * a.ratio + a.shiftMs) }))
    .filter(c => c.e > 0);
}

module.exports = { align, apply, overlap, RATIOS };
