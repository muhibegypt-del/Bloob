import { describe, expect, it } from 'vitest';
import subtitlesModule from '../../service/subtitles.js';
import subsyncModule from '../../service/subsync.js';

const { parse, hashOf, languageName } = subtitlesModule;
const { align, apply } = subsyncModule;

// A made-up film's speech: lines of 1–4 s with gaps of 0.5–6 s, from a fixed seed.
function speech(n, seed) {
  let x = seed, t = 5000;
  const rand = () => { x = (x * 1103515245 + 12345) % 2147483648; return x / 2147483648; };
  const out = [];
  for (let i = 0; i < n; i++) {
    const len = 1000 + rand() * 3000;
    out.push({ s: Math.round(t), e: Math.round(t + len), t: `line ${i}` });
    t += len + 500 + rand() * 5500;
  }
  return out;
}

describe('online subtitles (4.0)', () => {
  it('reads SubRip and WebVTT, tags and all', () => {
    const srt = '﻿1\r\n00:00:01,500 --> 00:00:03,000\r\n<i>Hello</i> {\\an8}there\r\n\r\n2\r\n00:01:02,000 --> 00:01:04,250\r\nSecond\r\nline\r\n';
    expect(parse(srt)).toEqual([{ s: 1500, e: 3000, t: 'Hello there' }, { s: 62000, e: 64250, t: 'Second\nline' }]);
    const vtt = 'WEBVTT\n\n00:05.000 --> 00:06.5\nShort &amp; sweet\n\n1:00:00.000 --> 1:00:01.000 align:start\nLate\n';
    expect(parse(vtt)).toEqual([{ s: 5000, e: 6500, t: 'Short & sweet' }, { s: 3600000, e: 3601000, t: 'Late' }]);
    expect(parse('garbage')).toEqual([]);
  });

  it('computes OpenSubtitles\' file hash', () => {
    const head = Buffer.alloc(65536), tail = Buffer.alloc(65536);
    for (let i = 0; i < 65536; i++) { head[i] = (i * 7) & 255; tail[i] = (i * 13 + 5) & 255; }
    const size = 7340032123;
    // The same sum with BigInt, as the definition reads.
    let want = BigInt(size);
    for (const b of [head, tail]) for (let i = 0; i < b.length; i += 8) want = (want + b.readBigUInt64LE(i)) & 0xFFFFFFFFFFFFFFFFn;
    expect(hashOf(size, head, tail)).toBe(want.toString(16).padStart(16, '0'));
  });

  it('names languages the way Bloob does', () => {
    expect(languageName('eng')).toBe('English');
    expect(languageName('pob')).toBe('Portuguese');
    expect(languageName('swe')).toBe('SWE');
  });
});

describe('subtitle timing (4.0)', () => {
  const reference = speech(300, 7);

  it('finds a plain shift', () => {
    const late = reference.map(c => ({ s: c.s + 2350, e: c.e + 2350, t: c.t }));
    const a = align(late, reference);
    expect(a.changed).toBe(true);
    expect(a.ratio).toBe(1);
    expect(Math.abs(a.shiftMs + 2350)).toBeLessThanOrEqual(40);
    const fixed = apply(late, a);
    expect(Math.abs(fixed[100].s - reference[100].s)).toBeLessThanOrEqual(40);
  });

  it('finds a frame-rate mix-up (a 25 fps subtitle on a 23.976 fps film)', () => {
    const pal = reference.map(c => ({ s: Math.round(c.s * 23.976 / 25), e: Math.round(c.e * 23.976 / 25), t: c.t }));
    const a = align(pal, reference);
    expect(a.changed).toBe(true);
    expect(a.ratio).toBeCloseTo(25 / 23.976, 6);
    const fixed = apply(pal, a);
    expect(Math.abs(fixed[250].s - reference[250].s)).toBeLessThanOrEqual(60);
  });

  it('leaves a subtitle that is already right alone, and refuses to guess with too little', () => {
    const same = reference.map(c => ({ s: c.s + 30, e: c.e + 30, t: c.t }));
    expect(align(same, reference).changed).toBe(false);
    expect(align(reference.slice(0, 10), reference).changed).toBe(false);
    // Unrelated speech: no confident fit.
    expect(align(speech(300, 99), reference).changed).toBe(false);
  });

  it('copes with lines split and merged differently, as another translation does', () => {
    const other = [];
    reference.forEach((c, i) => {
      if (i % 3 === 0) { const mid = Math.round((c.s + c.e) / 2); other.push({ s: c.s, e: mid, t: 'a' }, { s: mid + 100, e: c.e, t: 'b' }); }
      else other.push({ s: c.s - 120, e: c.e + 200, t: 'c' });
    });
    const shifted = other.map(c => ({ s: c.s - 4100, e: c.e - 4100, t: c.t }));
    const a = align(shifted, reference);
    expect(a.changed).toBe(true);
    expect(Math.abs(a.shiftMs - 4100)).toBeLessThanOrEqual(200);
  });
});
