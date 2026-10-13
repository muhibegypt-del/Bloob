// Source ranking for the native app. Same weights and wording as the web
// app's src/sources/ranker.ts: preferences for this TV, not facts. Every input
// is a claim parsed from a release name.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).

const RESOLUTION_SCORE = { '4K': 40, '1080p': 30, '720p': 15, '480p': 5, SD: 5 };
const LOW_QUALITY = new Set(['CAM', 'TS', 'SCR']);
const GROUP_ORDER = ['4K', '1080p', '720p', 'Other'];

function score(candidate, prefs) {
  const notes = [];
  const warnings = [];
  const claims = candidate.claims || {};
  let value = claims.resolution && RESOLUTION_SCORE[claims.resolution] !== undefined ? RESOLUTION_SCORE[claims.resolution] : 10;
  let excluded = false;

  const hdr = claims.hdr || [];
  if (hdr.indexOf('DV') >= 0) { value += 6; notes.push('Dolby Vision'); }
  else if (hdr.length) { value += 5; notes.push(hdr[0]); }

  const audio = claims.audio || [];
  if (audio.indexOf('Atmos') >= 0) { value += 4; notes.push('Dolby Atmos'); }
  else if (audio.some(a => a === 'TrueHD' || a === 'DTS-HD MA' || a === 'DTS:X')) { value += 2; notes.push('Lossless audio'); }
  else if (claims.channels === '7.1' || claims.channels === '5.1') { value += 1; notes.push(`${claims.channels} surround`); }

  const cautions = [];
  if (candidate.sourceType && LOW_QUALITY.has(candidate.sourceType)) {
    excluded = true;
    value -= 60;
    warnings.push('Recorded in a cinema: poor picture and sound');
    cautions.push('Cinema recording');
  }
  if (candidate.sourceType === 'REMUX') {
    value -= 6;
    warnings.push('Very high bitrate; may buffer on slower connections');
    cautions.push('Very high bitrate');
  }

  if (candidate.kind === 'magnet') {
    const seeders = candidate.seeders;
    if (seeders === 0) {
      excluded = true;
      value -= 30;
      warnings.push('No seeders reported; may never become available');
      cautions.push('No seeders');
    } else if (seeders !== undefined) {
      value += Math.min(15, 5 * Math.log10(seeders + 1));
      if (seeders < 5) {
        value -= 8;
        warnings.push('Few seeders; may take a long time to prepare');
        cautions.push(`${seeders} seeders · slow`);
      }
    }
  }

  const preferred = String(prefs.language || 'English').toLowerCase();
  const languages = (candidate.languages || []).map(l => String(l).toLowerCase());
  const hasPreferred = languages.some(l => l.indexOf(preferred) === 0);
  if (languages.length && !hasPreferred && !candidate.multiAudio) {
    value -= 12;
    warnings.push(`Tagged ${candidate.languages.slice(0, 2).join(', ')} audio`);
    cautions.push(`${candidate.languages.slice(0, 2).join(' + ')} audio`);
  } else if (candidate.multiAudio) {
    notes.push('Multiple audio languages');
  }
  if (candidate.pack) { value -= 3; notes.push('Part of a season pack'); }

  return { candidate, score: Math.round(value * 10) / 10, excluded, notes, warnings, cautions };
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

function headline(candidate) {
  const c = candidate.claims || {};
  const parts = [c.resolution || 'Unknown quality'];
  const hdr = c.hdr || [];
  if (hdr.indexOf('DV') >= 0) parts.push('Dolby Vision');
  else if (hdr.length) parts.push(hdr[0]);
  if ((c.audio || []).indexOf('Atmos') >= 0) parts.push('Atmos');
  return parts.join(' · ');
}

function picture(candidate) {
  const hdr = (candidate.claims || {}).hdr || [];
  if (hdr.indexOf('DV') >= 0) return hdr.length > 1 ? 'Dolby Vision + HDR' : 'Dolby Vision';
  return hdr.length ? hdr[0] : 'SDR';
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

function availability(candidate) {
  if (candidate.kind === 'link') return 'Direct link';
  if (candidate.seeders === undefined) return 'Availability unknown';
  return `${candidate.seeders.toLocaleString('en-GB')} seeders`;
}

function view(ranked) {
  const c = ranked.candidate;
  return {
    id: c.id,
    headline: headline(c),
    resolution: (c.claims || {}).resolution || (c.sourceType && LOW_QUALITY.has(c.sourceType) ? c.sourceType : '—'),
    picture: picture(c),
    audio: audioText(c),
    size: sizeText(c.sizeBytes),
    availability: availability(c),
    facts: [c.sourceType, (c.claims || {}).videoCodec, audioText(c), sizeText(c.sizeBytes), availability(c)].filter(Boolean).join(' · '),
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

function rank(candidates, prefs) {
  const ranked = candidates.map(c => score(c, prefs || {})).sort((a, b) => b.score - a.score);
  const recommended = ranked.find(r => !r.excluded) || null;
  const groups = GROUP_ORDER.map(label => ({
    label,
    items: ranked.filter(r => r !== recommended && groupOf(r.candidate) === label).map(view),
  })).filter(group => group.items.length);
  return {
    recommended: recommended ? view(recommended) : null,
    why: recommended ? explain(recommended, ranked) : '',
    groups,
    total: candidates.length,
  };
}

module.exports = { rank, score, headline, sizeText };
