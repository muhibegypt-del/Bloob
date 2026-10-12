// Turns indexer stream entries into structured, honest source candidates.
// Everything here is parsed from release names and indexer annotations; none of
// it is verified against the actual file until playback.

const RESOLUTIONS = [
  [/\b(2160p|4k|uhd)\b/i, '4K'],
  [/\b1080[pi]\b/i, '1080p'],
  [/\b720p\b/i, '720p'],
  [/\b(480p|576p|sd)\b/i, '480p'],
];

const HDR = [
  [/\b(dv|dovi|dolby[ .-]?vision)\b/i, 'DV'],
  [/\bhdr10(\+|plus)/i, 'HDR10+'],
  [/\bhdr10(?![+p\w])/i, 'HDR10'],
  [/\bhdr(?![\w+])/i, 'HDR'],
  [/\bhlg\b/i, 'HLG'],
];

const CODECS = [
  [/\b(x265|h[ .]?265|hevc)\b/i, 'HEVC'],
  [/\b(x264|h[ .]?264|avc)\b/i, 'H.264'],
  [/\bav1\b/i, 'AV1'],
  [/\bvp9\b/i, 'VP9'],
];

const AUDIO = [
  [/\batmos\b/i, 'Atmos'],
  [/\btrue-?hd\b/i, 'TrueHD'],
  [/\bdts[ .:-]?x\b/i, 'DTS:X'],
  [/\bdts[ .-]?hd([ .-]?ma)?\b/i, 'DTS-HD MA'],
  [/\bdts\b/i, 'DTS'],
  [/\b(ddp|dd\+|e-?ac-?3|eac3)/i, 'Dolby Digital Plus'],
  [/\b(dd|ac-?3)(?=\d|\b)/i, 'Dolby Digital'],
  [/\baac\b/i, 'AAC'],
  [/\bflac\b/i, 'FLAC'],
  [/\bopus\b/i, 'Opus'],
];

const SOURCE_TYPES = [
  [/\bremux\b/i, 'REMUX'],
  [/\b(hd-?cam|cam-?rip|cam)\b/i, 'CAM'],
  [/\b(hd-?ts|telesync|hd-?tc|telecine)\b/i, 'TS'],
  [/\b(dvd-?scr|screener|scr)\b/i, 'SCR'],
  [/\b(blu-?ray|bd-?rip|br-?rip|bdremux)\b/i, 'BluRay'],
  [/\bweb-?dl\b/i, 'WEB-DL'],
  [/\bweb-?rip\b/i, 'WEBRip'],
  [/\bweb\b/i, 'WEB-DL'],
  [/\bhdtv\b/i, 'HDTV'],
  [/\bdvd(-?rip|5|9)?\b/i, 'DVD'],
];

// Indexers annotate non-default audio languages with country flags.
const FLAG_LANGUAGES = {
  GB: 'English', US: 'English', FR: 'French', DE: 'German', ES: 'Spanish', MX: 'Spanish (Latin America)',
  IT: 'Italian', PT: 'Portuguese', BR: 'Portuguese (Brazil)', RU: 'Russian', UA: 'Ukrainian', PL: 'Polish',
  NL: 'Dutch', SE: 'Swedish', NO: 'Norwegian', DK: 'Danish', FI: 'Finnish', CZ: 'Czech', SK: 'Slovak',
  HU: 'Hungarian', RO: 'Romanian', GR: 'Greek', TR: 'Turkish', IL: 'Hebrew', SA: 'Arabic', AE: 'Arabic',
  IN: 'Hindi', JP: 'Japanese', KR: 'Korean', CN: 'Chinese', TW: 'Chinese (Taiwan)', HK: 'Chinese (Hong Kong)',
  TH: 'Thai', VN: 'Vietnamese', ID: 'Indonesian', MY: 'Malay', PH: 'Filipino', IR: 'Persian', BG: 'Bulgarian',
  RS: 'Serbian', HR: 'Croatian', LT: 'Lithuanian', LV: 'Latvian', EE: 'Estonian', SI: 'Slovenian',
};

const UNITS = { TB: 1e12, GB: 1e9, MB: 1e6, KB: 1e3 };

function first(table, text) {
  for (const [pattern, value] of table) if (pattern.test(text)) return value;
  return undefined;
}

function all(table, text) {
  const found = [];
  for (const [pattern, value] of table) if (pattern.test(text) && !found.includes(value)) found.push(value);
  return found;
}

function flags(text) {
  const languages = [];
  const pattern = /([\u{1F1E6}-\u{1F1FF}])([\u{1F1E6}-\u{1F1FF}])/gu;
  let match;
  while ((match = pattern.exec(text))) {
    const code = String.fromCharCode(match[1].codePointAt(0) - 0x1F1E6 + 65, match[2].codePointAt(0) - 0x1F1E6 + 65);
    const language = FLAG_LANGUAGES[code] || code;
    if (!languages.includes(language)) languages.push(language);
  }
  return languages;
}

/** Extract media claims from a release or file name. */
function parseRelease(text) {
  // Separators in release names: dots, underscores, brackets.
  const t = ` ${String(text || '').replace(/[._[\]()]+/g, ' ')} `;
  const hdr = all(HDR, t);
  // "HDR10" implies "HDR"; keep the most specific labels only.
  const specific = hdr.filter(h => !(h === 'HDR' && hdr.some(o => o.startsWith('HDR10'))));
  let audio = all(AUDIO, t);
  if (audio.includes('DTS-HD MA') || audio.includes('DTS:X')) audio = audio.filter(a => a !== 'DTS');
  if (audio.includes('Dolby Digital Plus')) audio = audio.filter(a => a !== 'Dolby Digital');
  const channels = /(?:^|[^\d])([2578])[ .]([01])(?:[^\d]|$)/.exec(t);
  const container = /\.(mkv|mp4|m4v|avi|ts|webm|mov)\s*$/i.exec(String(text || ''));
  return {
    resolution: first(RESOLUTIONS, t),
    hdr: specific.length ? specific : undefined,
    videoCodec: first(CODECS, t),
    audio: audio.length ? audio : undefined,
    channels: channels ? `${channels[1]}.${channels[2]}` : undefined,
    container: container ? container[1].toUpperCase() : undefined,
    sourceType: first(SOURCE_TYPES, t),
  };
}

function parseSize(text) {
  const match = /💾\s*([\d.,]+)\s*([TGMK]B)/i.exec(text);
  if (!match) return undefined;
  const value = Number(match[1].replace(/,/g, ''));
  return Number.isFinite(value) ? Math.round(value * UNITS[match[2].toUpperCase()]) : undefined;
}

function isHttpUrl(value) {
  try { return ['http:', 'https:'].includes(new URL(value).protocol); } catch (_) { return false; }
}

const EPISODE = /\bs\d{1,2}[ .-]?e\d{1,3}\b|\b\d{1,2}x\d{2,3}\b/i;
const PACK = /\b(s\d{1,2}(?![ .-]?e\d)|seasons?[ .-]?\d+|complete|collection)\b/i;

/**
 * Convert one indexer stream entry into a candidate, or null when it has no
 * usable magnet/link. Accepts both `title` and `description` (newer addon SDKs).
 */
function parseStream(stream, provider) {
  if (!stream || typeof stream !== 'object') return null;
  const hash = String(stream.infoHash || '').toLowerCase();
  const url = String(stream.url || '');
  const kind = /^[a-f0-9]{40}$/.test(hash) ? 'magnet' : isHttpUrl(url) ? 'link' : null;
  if (!kind) return null;

  const body = String(stream.description || stream.title || '');
  const nameLines = String(stream.name || '').split('\n').map(s => s.trim()).filter(Boolean);
  const lines = body.split('\n').map(s => s.trim()).filter(Boolean);
  const release = (lines[0] || nameLines.join(' ') || 'Unnamed release').replace(/\s+/g, ' ').slice(0, 160);
  // Lines after the release name that aren't stats or flags name the file inside a pack.
  const fileLine = lines.slice(1).find(line => !/[👤💾⚙️]/u.test(line) && !/^[\u{1F1E6}-\u{1F1FF}\s/|]+$/u.test(line) && /\.\w{2,4}$|\bs\d{1,2}e\d/i.test(line));
  const hints = stream.behaviorHints || {};
  const fileName = String(hints.filename || fileLine || '').slice(0, 200) || undefined;

  const claimsText = [release, fileName, nameLines.slice(1).join(' ')].filter(Boolean).join(' ');
  const parsed = parseRelease(claimsText);
  const container = parseRelease(fileName || '').container;
  const seeders = /👤\s*([\d,]+)/.exec(body);
  const sizeBytes = Number(hints.videoSize) > 0 ? Number(hints.videoSize) : parseSize(body);
  const languages = flags(body);
  const multiAudio = /\b(multi|dual)[ .-]?(audio|lang)/i.test(body + ' ' + release);
  const fileIdx = Number.isInteger(stream.fileIdx) ? stream.fileIdx : undefined;
  const pack = PACK.test(release) && !EPISODE.test(release);

  const claims = {};
  for (const key of ['resolution', 'hdr', 'videoCodec', 'audio', 'channels']) if (parsed[key]) claims[key] = parsed[key];
  if (container) claims.container = container;

  return {
    id: `${kind}:${kind === 'magnet' ? hash : url}${fileIdx !== undefined ? `#${fileIdx}` : ''}`,
    provider,
    kind,
    value: kind === 'magnet' ? hash : url,
    release,
    ...(fileName ? { fileName } : {}),
    ...(fileIdx !== undefined ? { fileIdx } : {}),
    claims,
    ...(parsed.sourceType ? { sourceType: parsed.sourceType } : {}),
    ...(sizeBytes ? { sizeBytes } : {}),
    ...(seeders ? { seeders: Number(seeders[1].replace(/,/g, '')) } : {}),
    languages,
    ...(multiAudio ? { multiAudio } : {}),
    ...(pack ? { pack } : {}),
    claimsFrom: 'release-name',
  };
}

/** Parse, drop unusable entries, and de-duplicate by magnet+file or URL. */
function parseStreams(streams, provider, limit = 60) {
  const seen = new Set();
  const result = [];
  for (const stream of Array.isArray(streams) ? streams : []) {
    const candidate = parseStream(stream, provider);
    if (!candidate || seen.has(candidate.id)) continue;
    seen.add(candidate.id);
    result.push(candidate);
    if (result.length >= limit) break;
  }
  return result;
}

module.exports = { parseRelease, parseStream, parseStreams, flags };
