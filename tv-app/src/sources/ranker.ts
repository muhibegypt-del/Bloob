import type { SourceCandidate } from '../buna/types';

export interface RankedCandidate {
  candidate: SourceCandidate;
  score: number;
  /** Excluded from recommendation (cinema recordings, no seeders). Still listed. */
  excluded: boolean;
  /** Short plain-language notes, positive first. */
  notes: string[];
  warnings: string[];
}

export interface Ranking {
  recommended: RankedCandidate | null;
  /** Why the recommendation won, one sentence. */
  why: string;
  groups: { label: string; items: RankedCandidate[] }[];
}

export interface RankPreferences {
  /** Preferred audio language name, e.g. "English". */
  language: string;
}

const RESOLUTION_SCORE: Record<string, number> = { '4K': 40, '1080p': 30, '720p': 15, '480p': 5, SD: 5 };
const LOW_QUALITY = new Set(['CAM', 'TS', 'SCR']);

/**
 * Score candidates for this TV. The weights encode presentation preferences,
 * not facts: every input is a claim parsed from a release name.
 */
export function scoreCandidate(candidate: SourceCandidate, prefs: RankPreferences): RankedCandidate {
  const notes: string[] = [];
  const warnings: string[] = [];
  const claims = candidate.claims;
  let score = claims.resolution ? RESOLUTION_SCORE[claims.resolution] ?? 10 : 10;
  let excluded = false;

  const hdr = claims.hdr ?? [];
  if (hdr.includes('DV')) { score += 6; notes.push('Dolby Vision'); }
  else if (hdr.length) { score += 5; notes.push(hdr[0] === 'HDR' ? 'HDR' : hdr[0]); }

  const audio = claims.audio ?? [];
  if (audio.includes('Atmos')) { score += 4; notes.push('Dolby Atmos'); }
  else if (audio.some(a => a === 'TrueHD' || a === 'DTS-HD MA' || a === 'DTS:X')) { score += 2; notes.push('Lossless audio'); }
  else if (claims.channels === '7.1' || claims.channels === '5.1') { score += 1; notes.push(`${claims.channels} surround`); }

  if (candidate.sourceType && LOW_QUALITY.has(candidate.sourceType)) {
    excluded = true;
    score -= 60;
    warnings.push('Recorded in a cinema: poor picture and sound');
  }
  if (candidate.sourceType === 'REMUX') {
    // Untouched disc bitrates can exceed what the TV's network link sustains.
    score -= 6;
    warnings.push('Very high bitrate; may buffer on slower connections');
  }

  if (candidate.kind === 'magnet') {
    const seeders = candidate.seeders;
    if (seeders === 0) {
      excluded = true;
      score -= 30;
      warnings.push('No seeders reported; may never become available');
    } else if (seeders !== undefined) {
      score += Math.min(15, 5 * Math.log10(seeders + 1));
      if (seeders < 5) { score -= 8; warnings.push('Few seeders; may take a long time to prepare'); }
    }
  }

  const preferred = prefs.language.toLowerCase();
  const languages = candidate.languages.map(l => l.toLowerCase());
  const hasPreferred = languages.some(l => l.startsWith(preferred));
  if (languages.length && !hasPreferred && !candidate.multiAudio) {
    score -= 12;
    warnings.push(`Tagged ${candidate.languages.slice(0, 2).join(', ')} audio`);
  } else if (candidate.multiAudio) {
    notes.push('Multiple audio languages');
  }

  if (candidate.pack) {
    score -= 3;
    notes.push('Part of a season pack');
  }

  return { candidate, score: Math.round(score * 10) / 10, excluded, notes, warnings };
}

const GROUP_ORDER = ['4K', '1080p', '720p', 'Other'];

export function rankCandidates(candidates: SourceCandidate[], prefs: RankPreferences = { language: 'English' }): Ranking {
  const ranked = candidates.map(c => scoreCandidate(c, prefs)).sort((a, b) => b.score - a.score);
  const recommended = ranked.find(r => !r.excluded) ?? null;
  const groups = GROUP_ORDER.map(label => ({
    label,
    items: ranked.filter(r => r !== recommended && groupOf(r.candidate) === label),
  })).filter(group => group.items.length);
  return { recommended, why: recommended ? explain(recommended, ranked) : '', groups };
}

function groupOf(candidate: SourceCandidate) {
  const resolution = candidate.claims.resolution;
  return resolution === '4K' || resolution === '1080p' || resolution === '720p' ? resolution : 'Other';
}

function explain(best: RankedCandidate, all: RankedCandidate[]): string {
  const c = best.candidate;
  const topResolution = all.filter(r => !r.excluded).some(r => (RESOLUTION_SCORE[r.candidate.claims.resolution ?? ''] ?? 0) > (RESOLUTION_SCORE[c.claims.resolution ?? ''] ?? 0));
  const health = c.kind === 'link' ? 'a direct link'
    : c.seeders === undefined ? 'unknown availability'
    : c.seeders >= 50 ? 'strong availability' : c.seeders >= 10 ? 'good availability' : 'limited availability';
  if (topResolution) return `Chosen over higher resolutions for its ${health} and compatibility.`;
  return `The best picture on offer, with ${health}.`;
}

/** The headline for a candidate: what the viewer gets, not the release jargon. */
export function headline(candidate: SourceCandidate): string {
  const c = candidate.claims;
  const parts: string[] = [c.resolution ?? 'Unknown quality'];
  if (c.hdr?.includes('DV')) parts.push('Dolby Vision');
  else if (c.hdr?.length) parts.push(c.hdr[0]);
  if (c.audio?.includes('Atmos')) parts.push('Atmos');
  return parts.join(' · ');
}

/** Secondary facts line. */
export function details(candidate: SourceCandidate, size: (bytes?: number) => string): string {
  const c = candidate.claims;
  return [
    candidate.sourceType,
    c.videoCodec,
    c.audio?.find(a => a !== 'Atmos') ?? (c.channels ? `${c.channels}` : undefined),
    size(candidate.sizeBytes),
    candidate.kind === 'magnet' && candidate.seeders !== undefined ? `${candidate.seeders} seeders` : candidate.kind === 'link' ? 'Direct link' : undefined,
  ].filter(Boolean).join(' · ');
}
