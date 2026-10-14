// Source ranking for the native app. Weights are preferences for this TV, not
// facts, and every input is a claim parsed from a release name. What the TV can
// play comes from a profile in tv-profile.js; nothing here names a model.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const { profileFor } = require('./tv-profile');

const RESOLUTION_SCORE = { '4K': 40, '1080p': 30, '720p': 15, '480p': 5, SD: 5 };
const LOW_QUALITY = new Set(['CAM', 'TS', 'SCR']);
const GROUP_ORDER = ['4K', '1080p', '720p', 'Other'];
const DTS = ['DTS', 'DTS-HD MA', 'DTS:X'];
// Typical runtimes when neither the app nor the catalogue gives one.
const TYPICAL_MINUTES = { movie: 120, series: 45 };

const WEIGHTS = {
  dolbyVision: 6,
  hdr: 5,
  atmos: 4,
  lossless: 2,
  surround: 1,
  // Larger than any picture, sound and availability bonus within one resolution.
  dtsSilent: 30,
  dtsMixed: 10,
  lowQuality: 60,
  unplayable: 60,
  noSeeders: 30,
  fewSeeders: 8,
  seedersMax: 15,
  otherLanguage: 12,
  preferredLanguage: 3,
  pack: 3,
  perMbpsOver: 0.3,
  bitrateMax: 20,
};

const plural = (n, one, many) => `${n} ${n === 1 ? one : (many || one + 's')}`;

function textOf(candidate) {
  return `${candidate.release || ''} ${candidate.fileName || ''}`;
}

/** What the picture will look like on this TV, from the claims. */
function pictureOn(candidate, profile) {
  const c = candidate.claims || {};
  const hdr = c.hdr || [];
  const dv = hdr.indexOf('DV') >= 0;
  // Disc Dolby Vision (profile 7) on a TV without it plays the HDR10 base layer.
  const disc = candidate.sourceType === 'REMUX' || (candidate.sourceType === 'BluRay' && c.resolution === '4K');
  const profile8 = /\b(dv|dovi)[ .-]?p8\b|\bprofile[ .-]?8\b/i.test(textOf(candidate));
  if (dv && disc && !profile8 && profile.dolbyVisionProfile7 !== 'yes') {
    return { label: 'HDR10 (disc Dolby Vision unsupported)', short: 'HDR10', bonus: WEIGHTS.hdr, discDolbyVision: true };
  }
  if (dv) return { label: hdr.length > 1 ? 'Dolby Vision + HDR' : 'Dolby Vision', short: 'Dolby Vision', bonus: WEIGHTS.dolbyVision };
  if (hdr.length) {
    const label = hdr[0] === 'HDR10+' && !profile.hdr10plus ? 'HDR10' : hdr[0];
    return { label, short: label, bonus: WEIGHTS.hdr };
  }
  return { label: 'SDR', short: '', bonus: 0 };
}

/** What the sound will be on this TV, from the claims. */
function soundOn(candidate, profile) {
  const audio = (candidate.claims || {}).audio || [];
  const dts = audio.filter(a => DTS.indexOf(a) >= 0);
  const others = audio.filter(a => DTS.indexOf(a) < 0 && a !== 'Atmos');
  const dtsBlocked = profile.dts === 'none' && dts.length > 0;
  const silent = dtsBlocked && others.length === 0;
  const truehd = others.indexOf('TrueHD') >= 0;
  const truehdOk = profile.truehdPassthrough === 'yes';
  // Atmos rides on DD+ or TrueHD; with no carrier named, streaming releases use DD+.
  const onTrueHd = truehd && others.indexOf('Dolby Digital Plus') < 0;
  const claimsAtmos = audio.indexOf('Atmos') >= 0 && !silent;
  return {
    silent,
    mixedDts: dtsBlocked && !silent,
    atmos: claimsAtmos && (!onTrueHd || truehdOk),
    atmosDowngrade: claimsAtmos && onTrueHd && !truehdOk,
    lossless: !silent && ((truehd && truehdOk) || (profile.dts !== 'none' && dts.some(a => a !== 'DTS'))),
  };
}

/** Estimated average bitrate in Mbit/s, or undefined without a size. */
function bitrateMbps(candidate, minutes) {
  if (!(candidate.sizeBytes > 0) || !(minutes > 0)) return undefined;
  return candidate.sizeBytes * 8 / (minutes * 60) / 1e6;
}

function runtimeOf(prefs) {
  const minutes = Number(prefs.runtimeMinutes);
  if (minutes > 0 && minutes < 1000) return minutes;
  return TYPICAL_MINUTES[prefs.type] || TYPICAL_MINUTES.movie;
}

function score(candidate, prefs) {
  prefs = prefs || {};
  const profile = profileFor(prefs.profile);
  const notes = [];
  // Problems, most serious first. Those with a short caution come first so the
  // app can pair cautions[0] with warnings[0].
  const issues = [];
  const issue = (severity, warning, caution) => issues.push({ severity, warning, caution });
  const claims = candidate.claims || {};
  let value = claims.resolution && RESOLUTION_SCORE[claims.resolution] !== undefined ? RESOLUTION_SCORE[claims.resolution] : 10;
  let excluded = false;

  const picture = pictureOn(candidate, profile);
  value += picture.bonus;
  if (picture.short) notes.push(picture.short);
  if (picture.discDolbyVision) issue(1, 'This TV doesn\'t support disc Dolby Vision; it plays as HDR10');

  const sound = soundOn(candidate, profile);
  if (sound.silent) {
    value -= WEIGHTS.dtsSilent;
    issue(9, 'This TV can\'t play DTS sound; you may hear nothing', 'No sound on this TV');
  } else {
    if (sound.mixedDts) {
      value -= WEIGHTS.dtsMixed;
      issue(6, 'Includes DTS sound, which this TV can\'t play; you may hear nothing', 'Sound may not play');
    }
    if (sound.atmos) { value += WEIGHTS.atmos; notes.push('Dolby Atmos'); }
    else if (sound.lossless) { value += WEIGHTS.lossless; notes.push('Lossless audio'); }
    else if (claims.channels === '7.1' || claims.channels === '5.1') { value += WEIGHTS.surround; notes.push(`${claims.channels} surround`); }
    if (sound.atmosDowngrade) issue(1, 'TrueHD Atmos may play as standard sound on this TV');
  }

  if (candidate.sourceType && LOW_QUALITY.has(candidate.sourceType)) {
    excluded = true;
    value -= WEIGHTS.lowQuality;
    issue(10, 'Recorded in a cinema: poor picture and sound', 'Cinema recording');
  }
  if (claims.discImage) {
    excluded = true;
    value -= WEIGHTS.unplayable;
    issue(10, 'A whole disc image, not a video file this TV can play', 'Disc image');
  }
  if (claims.threeD && !profile.threeD) {
    excluded = true;
    value -= WEIGHTS.unplayable;
    issue(10, 'A 3D version; this TV shows it as two squashed pictures', '3D version');
  }
  if (claims.sample) {
    excluded = true;
    value -= WEIGHTS.unplayable;
    issue(10, 'Only a short sample, not the whole thing', 'Sample only');
  }

  const mbps = bitrateMbps(candidate, runtimeOf(prefs));
  if (mbps !== undefined && mbps > profile.comfortableMbps) {
    value -= Math.min(WEIGHTS.bitrateMax, (mbps - profile.comfortableMbps) * WEIGHTS.perMbpsOver);
    if (mbps > profile.warnMbps) {
      issue(5, `Very high bitrate; may buffer on the TV's wired connection (about ${profile.ethernetMbps} Mbps)`, 'Very high bitrate');
    }
  }

  if (candidate.kind === 'magnet') {
    const seeders = candidate.seeders;
    if (seeders === 0) {
      excluded = true;
      value -= WEIGHTS.noSeeders;
      issue(8, 'No seeders reported; may never become available', 'No seeders');
    } else if (seeders !== undefined) {
      value += Math.min(WEIGHTS.seedersMax, 5 * Math.log10(seeders + 1));
      if (seeders < 5) {
        value -= WEIGHTS.fewSeeders;
        issue(4, 'Few seeders; may take a long time to prepare', 'Few sources · slow to prepare');
      }
    }
  }

  const preferredName = String(prefs.language || 'English');
  const preferred = preferredName.toLowerCase();
  const languages = (candidate.languages || []).map(l => String(l).toLowerCase());
  const hasPreferred = languages.some(l => l.indexOf(preferred) === 0);
  if (hasPreferred) {
    value += WEIGHTS.preferredLanguage;
    notes.push(`${preferredName} audio`);
  } else if (languages.length && !candidate.multiAudio) {
    value -= WEIGHTS.otherLanguage;
    issue(3, `Tagged ${candidate.languages.slice(0, 2).join(', ')} audio`, `${candidate.languages.slice(0, 2).join(' + ')} audio`);
  }
  if (candidate.multiAudio) notes.push('Multiple audio languages');
  if (candidate.pack) { value -= WEIGHTS.pack; notes.push('Part of a season pack'); }

  issues.sort((a, b) => (!!b.caution - !!a.caution) || b.severity - a.severity);
  return {
    candidate,
    score: Math.round(value * 10) / 10,
    excluded,
    silent: sound.silent,
    notes,
    warnings: issues.map(i => i.warning),
    cautions: issues.filter(i => i.caution).map(i => i.caution),
    picture,
    sound,
    mbps,
  };
}

function groupOf(candidate) {
  const r = (candidate.claims || {}).resolution;
  return r === '4K' || r === '1080p' || r === '720p' ? r : 'Other';
}

function sizeText(bytes) {
  if (!bytes) return '';
  if (bytes >= 1e9) return `${(bytes / 1e9).toFixed(bytes >= 1e10 ? 0 : 1)} GB`;
  return `${Math.max(1, Math.round(bytes / 1e6))} MB`;
}

/** "4K · Dolby Vision · Atmos": only what this TV is expected to deliver. */
function headline(candidate, prefs) {
  const profile = profileFor((prefs || {}).profile);
  const c = candidate.claims || {};
  const parts = [c.resolution || 'Unknown quality'];
  const picture = pictureOn(candidate, profile);
  if (picture.short) parts.push(picture.short);
  if (soundOn(candidate, profile).atmos) parts.push('Atmos');
  return parts.join(' · ');
}

function audioText(candidate) {
  const c = candidate.claims || {};
  const audio = (c.audio || []).filter(a => a !== 'Atmos');
  const atmos = (c.audio || []).indexOf('Atmos') >= 0;
  const main = audio[0] || (atmos ? 'Dolby' : '');
  const channels = c.channels === '2.0' ? 'Stereo' : c.channels === '1.0' ? 'Mono' : c.channels;
  const text = [atmos ? `${main} Atmos` : main, channels].filter(Boolean).join(' ').trim();
  return text || 'Unknown audio';
}

// Plain digits: the native app reads the leading number.
function availability(candidate) {
  if (candidate.kind === 'link') return 'Direct link';
  if (candidate.seeders === undefined) return 'Availability unknown';
  return plural(candidate.seeders, 'seeder');
}

function bitrateText(mbps) {
  return mbps === undefined ? '' : `about ${Math.round(mbps)} Mbit/s`;
}

function view(ranked, prefs) {
  const c = ranked.candidate;
  return {
    id: c.id,
    headline: headline(c, prefs),
    resolution: (c.claims || {}).resolution || (c.sourceType && LOW_QUALITY.has(c.sourceType) ? c.sourceType : '—'),
    picture: ranked.picture.label,
    audio: audioText(c),
    size: sizeText(c.sizeBytes),
    availability: availability(c),
    bitrateMbps: ranked.mbps === undefined ? null : Math.round(ranked.mbps),
    facts: [c.sourceType, (c.claims || {}).videoCodec, audioText(c), sizeText(c.sizeBytes), bitrateText(ranked.mbps), availability(c)].filter(Boolean).join(' · '),
    notes: ranked.notes,
    warnings: ranked.warnings,
    cautions: ranked.cautions,
    excluded: ranked.excluded,
    release: c.release,
    score: ranked.score,
  };
}

function explain(best, all) {
  const c = best.candidate;
  const mine = RESOLUTION_SCORE[(c.claims || {}).resolution] || 0;
  const higher = all.some(r => !r.excluded && (RESOLUTION_SCORE[(r.candidate.claims || {}).resolution] || 0) > mine);
  const health = c.kind === 'link' ? 'a direct link'
    : c.seeders === undefined ? 'unknown availability'
    : c.seeders >= 50 ? 'strong availability' : c.seeders >= 10 ? 'good availability' : 'limited availability';
  if (higher) return `Chosen over higher resolutions for its ${health} and compatibility.`;
  return `The best picture on offer, with ${health}.`;
}

/**
 * Rank candidates for one title. `prefs`: language, type ('movie'|'series'),
 * runtimeMinutes, profile (ID or object; the default TV otherwise).
 */
function rank(candidates, prefs) {
  prefs = prefs || {};
  const ranked = candidates.map(c => score(c, prefs)).sort((a, b) => b.score - a.score);
  const playable = ranked.filter(r => !r.excluded);
  // Never recommend a version this TV plays silently while one with sound exists.
  const recommended = playable.find(r => !r.silent) || playable[0] || null;
  const groups = GROUP_ORDER.map(label => ({
    label,
    items: ranked.filter(r => r !== recommended && groupOf(r.candidate) === label).map(r => view(r, prefs)),
  })).filter(group => group.items.length);
  return {
    recommended: recommended ? view(recommended, prefs) : null,
    why: recommended ? explain(recommended, ranked) : '',
    groups,
    total: candidates.length,
  };
}

module.exports = { rank, score, headline, sizeText, bitrateMbps, WEIGHTS };
