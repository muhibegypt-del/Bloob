// The versions list as a cinephile reads it (2.8): which cut a release is, what
// tier of source it comes from, the facts that tell two versions apart, and a
// plain comparison with the best one. Everything here is read from release
// names (claims), never probed; a release that says nothing is the theatrical cut.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).

// Cuts, in the order the edition row shows them. "Uncut" counts as Unrated.
const CUTS = [
  ['Theatrical', /\btheatrical\b/i],
  ['Extended', /\bextended\b/i],
  ['Director’s Cut', /\b(director'?s?|dir)[ ._-]?(cut|edition)\b/i],
  ['Final Cut', /\bfinal[ ._-]?cut\b/i],
  ['Ultimate Cut', /\bultimate[ ._-]?(cut|edition)\b/i],
  ['Unrated', /\b(unrated|uncut)\b/i],
  ['IMAX', /\bimax\b/i],
  ['Open Matte', /\bopen[ ._-]?matte\b/i],
  ['Special Edition', /\bspecial[ ._-]?edition\b/i],
  ['Redux', /\bredux\b/i],
];
const CUT_ORDER = CUTS.map(c => c[0]);

// Tags shown with a release, not separate cuts.
const TAGS = [
  ['Hybrid', /\bhybrid\b/i],
  ['Criterion', /\bcriterion\b/i],
  ['Remastered', /\bremaster(ed)?\b/i],
  ['Restored', /\b(4k[ ._-]?)?restor(ed|ation)\b/i],
];

// Release groups known for careful remuxes and encodes. A mark, never a penalty for others.
const RESPECTED = new Set(['framestor', 'bluranium', 'epsilon', 'cinephiles', 'sicfoi', 'triton', 'playbd', 'zq', 'decibel',
  'don', 'ctrlhd', 'hifi', 'hdmaniacs', 'beyondhd', 'wildcat', 'tayto', 'ntb', 'flux', 'cmrg']);

function textOf(c) {
  return [c.release, c.fileName].filter(Boolean).join(' ').replace(/[._]/g, ' ');
}

/** The cut a release claims, or '' (the theatrical cut, or not said). */
function cutOf(c) {
  const t = textOf(c);
  for (let i = 1; i < CUTS.length; i++) if (CUTS[i][1].test(t)) return CUTS[i][0];
  return '';
}

function tagsOf(c) {
  const t = textOf(c);
  return TAGS.filter(tag => tag[1].test(t)).map(tag => tag[0]);
}

function respected(c) {
  return RESPECTED.has(String(c.group || '').toLowerCase());
}

// Tiers, best first, in a cinephile's words.
const TIERS = [
  ['uhd-remux', 'UHD Blu-ray Remux'],
  // 4.1 (owner): SDR remuxes have a row of their own, and season packs a section.
  ['sdr-remux', 'SDR Remux'],
  ['bd-remux', 'Blu-ray Remux'],
  ['uhd-encode', '4K encodes from the disc'],
  ['web-4k', '4K web'],
  ['bd-encode', 'Blu-ray encodes'],
  ['web-hd', 'HD web'],
  ['pack', 'Season packs'],
  ['other', 'Everything else'],
];

/** The section a version is listed in; `kind: true` gives its kind of source, ignoring a pack. */
function tierOf(c, kind) {
  const res = (c.claims || {}).resolution || '';
  const src = c.sourceType || '';
  const uhd = res === '4K';
  if (c.pack && !kind) return 'pack';
  const sdr = !((c.claims || {}).hdr || []).length;
  if (src === 'REMUX') return sdr ? 'sdr-remux' : uhd ? 'uhd-remux' : 'bd-remux';
  if (src === 'BluRay') return uhd ? 'uhd-encode' : res === '1080p' ? 'bd-encode' : 'other';
  if (src === 'WEB-DL' || src === 'WEBRip') return uhd ? 'web-4k' : res === '1080p' ? 'web-hd' : 'other';
  return 'other';
}
const tierLabel = key => (TIERS.find(t => t[0] === key) || TIERS[TIERS.length - 1])[1];
const tierRank = key => { const i = TIERS.findIndex(t => t[0] === key); return i < 0 ? TIERS.length : i; };

const LOSSLESS = ['TrueHD', 'DTS-HD MA', 'DTS:X', 'LPCM', 'FLAC'];
function lossless(c) {
  return ((c.claims || {}).audio || []).some(a => LOSSLESS.indexOf(a) >= 0);
}

/** "Dolby Vision P7", "HDR10+", "HDR10", "SDR": the picture as a cinephile names it. */
function pictureName(c, dvProfile, p7Converts) {
  const hdr = (c.claims || {}).hdr || [];
  // Profile 7 (disc): Bloob's engine plays it as Dolby Vision 8.1 (4.0); with
  // the HDR10 picture preference, or a TV profile without the converter, it is its HDR10 base.
  if (hdr.indexOf('DV') >= 0 && dvProfile === 'p7') return p7Converts ? 'Dolby Vision P7 disc (as 8.1)' : 'HDR10 (Dolby Vision P7 disc)';
  if (hdr.indexOf('DV') >= 0) return dvProfile ? `Dolby Vision ${dvProfile.toUpperCase()}` : 'Dolby Vision';
  if (hdr.indexOf('HDR10+') >= 0) return 'HDR10+';
  if (hdr.length) return hdr.indexOf('HLG') >= 0 && hdr.length === 1 ? 'HLG' : 'HDR10';
  return 'SDR';
}

/** The row's one line: picture · audio · size · group · cut · tags. */
function line(c, parts) {
  return [parts.picture, parts.audio, parts.size, c.group || '', parts.cut, (parts.tags || []).join(' ')].filter(Boolean).join(' · ');
}

/**
 * How `v` differs from `best`, in plain words: "Same disc, no Atmos, 60% smaller".
 * Both are view objects with the cinephile fields.
 */
function compare(v, best) {
  if (!best || v.id === best.id) return '';
  const out = [];
  // By kind of source: a season pack's file is compared by what it is, not by its section.
  const vt = v.sourceTier || v.tier, bt = best.sourceTier || best.tier;
  if (vt === bt) out.push(vt.indexOf('remux') >= 0 && v.cut === best.cut ? 'Same disc' : 'Same source');
  else out.push(vt.indexOf('remux') >= 0 ? 'Disc remux' : vt.indexOf('encode') >= 0 ? 'Disc encode'
    : vt.indexOf('web') === 0 ? 'Web version' : 'Lower quality');
  if (v.resolution !== best.resolution) out.push(v.resolution);
  if (v.cut !== best.cut) out.push(v.cut || 'Theatrical');
  const vDv = v.pictureName.indexOf('Dolby Vision') === 0, bDv = best.pictureName.indexOf('Dolby Vision') === 0;
  if (vDv && !bDv) out.push('Dolby Vision');
  else if (!vDv && bDv) out.push(v.pictureName === 'SDR' ? 'no HDR' : 'no Dolby Vision');
  const vAtmos = /Atmos/.test(v.audio), bAtmos = /Atmos/.test(best.audio);
  if (vAtmos && !bAtmos) out.push('Atmos');
  else if (!vAtmos && bAtmos) out.push('no Atmos');
  if (v.lossless && !best.lossless) out.push('lossless audio');
  else if (!v.lossless && best.lossless && !(!vAtmos && bAtmos)) out.push('lossy audio');
  if (v.bytes && best.bytes) {
    const ratio = v.bytes / best.bytes;
    if (ratio < 0.85) out.push(`${Math.round((1 - ratio) * 100)}% smaller`);
    else if (ratio >= 1.5) out.push(`${ratio.toFixed(1)}\u00d7 the size`);
    else if (ratio > 1.15) out.push(`${Math.round((ratio - 1) * 100)}% larger`);
  }
  if (v.ready === true && best.ready !== true) out.push('ready now');
  const text = out.join(', ');
  return text.charAt(0).toUpperCase() + text.slice(1);
}

/**
 * The same release found by several indexes, or under several hashes: same
 * group, resolution, source, cut and size within 2%. Returns [kept, merged count].
 */
function sameRelease(a, b) {
  const ca = a.candidate, cb = b.candidate;
  if (!ca.group || String(ca.group).toLowerCase() !== String(cb.group || '').toLowerCase()) return false;
  if ((ca.claims || {}).resolution !== (cb.claims || {}).resolution || ca.sourceType !== cb.sourceType) return false;
  if (cutOf(ca) !== cutOf(cb)) return false;
  const sa = ca.sizeBytes || 0, sb = cb.sizeBytes || 0;
  return sa > 0 && sb > 0 && Math.abs(sa - sb) / Math.max(sa, sb) <= 0.02;
}

/** Edition row: the cuts on offer with counts, in CUT_ORDER; '' is Theatrical. */
function cutsOf(views) {
  const counts = {};
  views.forEach(v => { const k = v.cut || 'Theatrical'; counts[k] = (counts[k] || 0) + 1; });
  return CUT_ORDER.filter(k => counts[k]).map(name => ({ name, count: counts[name] }));
}

module.exports = { CUT_ORDER, TIERS, cutOf, tagsOf, respected, tierOf, tierLabel, tierRank, lossless, pictureName, line, compare, sameRelease, cutsOf };
