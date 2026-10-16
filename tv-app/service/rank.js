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
  // Availability helps, but must never outvote a resolution step (10 points).
  seedersMax: 6,
  otherLanguage: 12,
  preferredLanguage: 3,
  pack: 3,
  perMbpsOver: 0.3,
  bitrateMax: 20,
  // Continuity with the viewer's version of this show (UX-PLAN X3).
  sameEdition: 25,   // the same season pack, or the same release group's file
  sameQuality: 12,   // same resolution and picture
  belowIntent: 10,   // lower resolution than the viewer has been watching
  // The picture setting outranks resolution and continuity: asked-for SDR wins.
  picturePreferred: 45,
  dolbyVisionOnly: 15,
};

const RANK_OF = { '4K': 4, '1080p': 3, '720p': 2, '480p': 1, SD: 1 };
// With several indexes a film can have hundreds of versions; the list keeps the
// best of each section (the count of the rest is reported).
const SECTION_LIMIT = { continue: 30, others: 120, problems: 40 };
const PROBLEM_SECTION = 'May not play well on this TV';

const plural = (n, one, many) => `${n} ${n === 1 ? one : (many || one + 's')}`;

function textOf(candidate) {
  return `${candidate.release || ''} ${candidate.fileName || ''}`;
}

// Picture preferences (Settings > Picture): 'best' takes the best this TV shows,
// 'hdr10' avoids Dolby Vision, 'sdr' prefers SDR versions.
const PICTURE_PREFERENCES = ['best', 'hdr10', 'sdr'];

function picturePreference(value) {
  return PICTURE_PREFERENCES.indexOf(value) >= 0 ? value : 'best';
}

/**
 * Which Dolby Vision profile a release probably carries, from its name:
 * 'p7' (disc remux, dual layer), 'p8' (with an HDR10 base layer), 'p5' (Dolby
 * Vision only, no HDR10 fallback: streaming releases tagged "DV" alone), or ''.
 * A guess from naming habits, never a probe.
 */
function dolbyVisionProfile(candidate) {
  const c = candidate.claims || {};
  const hdr = c.hdr || [];
  if (hdr.indexOf('DV') < 0) return '';
  const text = textOf(candidate);
  if (/\b(dv|dovi)[ .-]?p?5\b|\bprofile[ .-]?5\b/i.test(text)) return 'p5';
  if (/\b(dv|dovi)[ .-]?p?8(\.\d)?\b|\bprofile[ .-]?8\b/i.test(text)) return 'p8';
  const disc = candidate.sourceType === 'REMUX' || !!c.discImage;
  if (disc) return 'p7';
  return hdr.some(h => h !== 'DV') ? 'p8' : 'p5';
}

/** What the picture will look like on this TV, from the claims and the viewer's preference. */
function pictureOn(candidate, profile, preference) {
  const c = candidate.claims || {};
  const hdr = c.hdr || [];
  const pref = picturePreference(preference);
  const dvProfile = dolbyVisionProfile(candidate);
  // Most releases are MKV; an unknown container is treated as one.
  const container = String(c.container || 'MKV').toUpperCase();
  const containers = profile.dolbyVisionContainers || [];
  const dvShows = containers.indexOf(container) >= 0;
  const base = hdr.filter(h => h !== 'DV')[0];
  const baseLabel = !base ? 'HDR10' : base === 'HDR10+' && !profile.hdr10plus ? 'HDR10' : base === 'HDR' ? 'HDR10' : base;
  if (dvProfile === 'p7' && profile.dolbyVisionProfile7 !== 'yes') {
    return { label: 'HDR10 (disc Dolby Vision unsupported)', short: 'HDR10', bonus: WEIGHTS.hdr, discDolbyVision: true, family: 'HDR', dolbyVision: dvProfile };
  }
  if (dvProfile === 'p5' && (!dvShows || pref === 'hdr10')) {
    // No HDR10 layer to fall back to: without Dolby Vision the colours are wrong.
    return { label: 'Dolby Vision only (colours may be wrong here)', short: 'Dolby Vision', bonus: 0, dolbyVisionOnly: true, family: 'DV', dolbyVision: dvProfile };
  }
  if (dvProfile && pref === 'hdr10') {
    return { label: `${baseLabel} (Dolby Vision skipped)`, short: baseLabel, bonus: WEIGHTS.hdr - 1, family: 'HDR', dolbyVision: dvProfile };
  }
  if (dvProfile && !dvShows) {
    return { label: `${baseLabel} (Dolby Vision version)`, short: baseLabel, bonus: WEIGHTS.hdr + 0.5, dolbyVisionAsHdr: true, family: 'HDR', dolbyVision: dvProfile };
  }
  if (dvProfile) return { label: hdr.length > 1 ? 'Dolby Vision + HDR' : 'Dolby Vision', short: 'Dolby Vision', bonus: WEIGHTS.dolbyVision, family: 'DV', dolbyVision: dvProfile };
  if (hdr.length) {
    const label = hdr[0] === 'HDR10+' && !profile.hdr10plus ? 'HDR10' : hdr[0];
    return { label, short: label, bonus: WEIGHTS.hdr, family: 'HDR', dolbyVision: '' };
  }
  return { label: 'SDR', short: '', bonus: 0, family: 'SDR', dolbyVision: '' };
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

/** Picture family for matching versions: Dolby Vision, other HDR, or SDR. */
function hdrFamily(candidate) {
  const hdr = (candidate.claims || {}).hdr || [];
  return hdr.indexOf('DV') >= 0 ? 'DV' : hdr.length ? 'HDR' : 'SDR';
}

/** The torrent hash of a candidate ID (magnet:<hash>#<file>), or ''. */
function hashOf(id) {
  const m = /^magnet:([a-f0-9]{40})/i.exec(String(id || ''));
  return m ? m[1].toLowerCase() : '';
}

/** What the service remembers about a version that was played: never a URL. */
function editionOf(candidate) {
  if (!candidate) return null;
  const hash = hashOf(candidate.id);
  return {
    // Only a torrent's ID is kept; a link's ID is its URL, which stays in memory.
    id: hash ? String(candidate.id).slice(0, 60) : '',
    hash,
    resolution: (candidate.claims || {}).resolution || '',
    hdr: hdrFamily(candidate),
    atmos: ((candidate.claims || {}).audio || []).indexOf('Atmos') >= 0,
    group: String(candidate.group || '').slice(0, 40),
    binge: String(candidate.binge || '').slice(0, 80),
    pack: !!candidate.pack,
  };
}

/**
 * How a candidate relates to the version the viewer has been watching:
 * 'same' (the same pack or release group at the same quality), 'like' (same
 * resolution and picture), 'lower' (a lower resolution), or ''.
 */
function matchOf(candidate, intent) {
  if (!intent) return '';
  const hash = hashOf(candidate.id);
  const resolution = (candidate.claims || {}).resolution || '';
  const sameQuality = !!intent.resolution && resolution === intent.resolution && hdrFamily(candidate) === (intent.hdr || 'SDR');
  if (hash && intent.hash && hash === intent.hash) return 'same';
  if (sameQuality && intent.group && candidate.group && intent.group.toLowerCase() === String(candidate.group).toLowerCase()) return 'same';
  if (sameQuality) return 'like';
  if (intent.binge && candidate.binge && intent.binge === candidate.binge) return 'like';
  if (intent.resolution && (RANK_OF[resolution] || 0) < (RANK_OF[intent.resolution] || 0)) return 'lower';
  return '';
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

  const preference = picturePreference(prefs.picture);
  const picture = pictureOn(candidate, profile, preference);
  value += picture.bonus;
  if (picture.short) notes.push(picture.short);
  if (picture.discDolbyVision) issue(1, 'This TV doesn\'t support disc Dolby Vision; it plays as HDR10');
  if (picture.dolbyVisionAsHdr) issue(1, `A Dolby Vision version; this TV shows Dolby Vision in an MKV file as HDR10`);
  if (picture.dolbyVisionOnly) {
    value -= WEIGHTS.dolbyVisionOnly;
    issue(4, 'Dolby Vision without an HDR10 layer; this TV may show it with wrong colours', 'Colours may be wrong');
  }
  if (preference === 'sdr' && picture.family === 'SDR') value += WEIGHTS.picturePreferred;

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

  const match = matchOf(candidate, prefs.intent);
  if (match === 'same') value += WEIGHTS.sameEdition;
  else if (match === 'like') value += WEIGHTS.sameQuality;
  else if (match === 'lower') value -= WEIGHTS.belowIntent;

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
    match,
    // Worth a separate section: silent, unplayable, or a serious playback risk.
    problem: excluded || sound.silent || sound.mixedDts || issues.some(i => i.severity >= 5),
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
  const picture = pictureOn(candidate, profile, (prefs || {}).picture);
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
    size: c.sizeBytes ? sizeText(c.sizeBytes) : c.packBytes ? `${sizeText(c.packBytes)} pack` : '',
    source: c.sourceType || '',
    group: c.group || '',
    availability: availability(c),
    bitrateMbps: ranked.mbps === undefined ? null : Math.round(ranked.mbps),
    facts: [c.sourceType, (c.claims || {}).videoCodec, audioText(c), sizeText(c.sizeBytes), bitrateText(ranked.mbps), availability(c)].filter(Boolean).join(' · '),
    notes: ranked.notes,
    warnings: ranked.warnings,
    cautions: ranked.cautions,
    excluded: ranked.excluded,
    release: c.release,
    score: ranked.score,
    match: ranked.match === 'same' || ranked.match === 'like' ? ranked.match : '',
    scope: c.pack ? 'pack' : 'episode',
    // Picture family for the list's filters, and the guessed Dolby Vision
    // profile the player may signal (never 'p7': the TV can't show it).
    family: ranked.picture.family,
    dolbyVision: ranked.picture.dolbyVision === 'p7' ? '' : ranked.picture.dolbyVision,
    container: (c.claims || {}).container || '',
    sources: (c.providers || [c.provider]).filter(Boolean),
  };
}

function explain(best, all, intent, preference) {
  const c = best.candidate;
  if (preference === 'sdr') {
    if (best.picture.family === 'SDR') return 'SDR, as your picture setting asks.';
    return 'No SDR version is offered; this is the best of the rest.';
  }
  if (best.match === 'same') return 'The same version as your last episode.';
  if (best.match === 'like') return `Matches your last episode: ${[intent.resolution, intent.hdr === 'DV' ? 'Dolby Vision' : intent.hdr === 'HDR' ? 'HDR' : ''].filter(Boolean).join(' · ')}.`;
  const mine = RESOLUTION_SCORE[(c.claims || {}).resolution] || 0;
  const higher = all.some(r => !r.excluded && (RESOLUTION_SCORE[(r.candidate.claims || {}).resolution] || 0) > mine);
  const health = c.kind === 'link' ? 'a direct link'
    : c.seeders === undefined ? 'unknown availability'
    : c.seeders >= 50 ? 'strong availability' : c.seeders >= 10 ? 'good availability' : 'limited availability';
  if (intent && intent.resolution) return `Your last episode's version isn't offered for this one; this is the closest, with ${health}.`;
  if (higher) return `Chosen over higher resolutions for its ${health} and compatibility.`;
  return `The best picture on offer, with ${health}.`;
}

/**
 * Rank candidates for one title. `prefs`: language, type ('movie'|'series'),
 * runtimeMinutes, profile (ID or object; the default TV otherwise), and
 * intent: the edition the viewer has been watching this show in, if any.
 *
 * Sections, in order: versions that continue the viewer's last episode (when
 * there is one), every other version (best resolution first), then those
 * likely to fail on this TV.
 */
function rank(candidates, prefs) {
  prefs = prefs || {};
  const ranked = candidates.map(c => score(c, prefs)).sort((a, b) => b.score - a.score);
  const playable = ranked.filter(r => !r.excluded);
  // Never recommend a version this TV plays silently while one with sound exists.
  const recommended = playable.find(r => !r.silent) || playable[0] || null;
  const rest = ranked.filter(r => r !== recommended);
  const continuing = prefs.intent ? rest.filter(r => !r.problem && (r.match === 'same' || r.match === 'like')) : [];
  const problems = rest.filter(r => r.problem);
  // Every other version, best resolution first (each row names its resolution).
  const others = [];
  GROUP_ORDER.forEach(label => rest.forEach(r => {
    if (!r.problem && continuing.indexOf(r) < 0 && groupOf(r.candidate) === label) others.push(r);
  }));
  const groups = [];
  const section = (label, kind, list) => {
    const kept = list.slice(0, SECTION_LIMIT[kind]);
    groups.push({ label, kind, items: kept.map(r => view(r, prefs)), more: list.length - kept.length });
  };
  if (continuing.length) {
    section(continuing.every(r => r.match === 'same') ? 'Same as your last episode' : 'Matches your last episode', 'continue', continuing);
  }
  // "Other versions" keeps the best of each resolution when trimmed, not only 4K.
  if (others.length) {
    let kept = others;
    if (others.length > SECTION_LIMIT.others) {
      const share = Math.floor(SECTION_LIMIT.others / GROUP_ORDER.length);
      const chosen = new Set();
      GROUP_ORDER.forEach(label => others.filter(r => groupOf(r.candidate) === label).slice(0, share).forEach(r => chosen.add(r)));
      others.forEach(r => { if (chosen.size < SECTION_LIMIT.others) chosen.add(r); });
      kept = others.filter(r => chosen.has(r));
    }
    groups.push({ label: 'Other versions', kind: 'others', items: kept.map(r => view(r, prefs)), more: others.length - kept.length });
  }
  if (problems.length) section(PROBLEM_SECTION, 'problems', problems);
  return {
    recommended: recommended ? view(recommended, prefs) : null,
    why: recommended ? explain(recommended, ranked, prefs.intent, picturePreference(prefs.picture)) : '',
    groups,
    total: candidates.length,
  };
}

module.exports = { rank, score, headline, sizeText, bitrateMbps, editionOf, hashOf, dolbyVisionProfile, picturePreference, WEIGHTS };
