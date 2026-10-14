// Turns indexer stream entries into structured, honest source candidates.
// Everything here is parsed from release names and indexer annotations; none of
// it is verified against the actual file until playback.

// Explicit line counts first: "1080p.UHD.BluRay" is 1080p, not 4K.
const RESOLUTIONS = [
  [/\b2160p\b/i, '4K'],
  [/\b1080[pi]\b/i, '1080p'],
  [/\b720p\b/i, '720p'],
  [/\b(480p|576p)\b/i, '480p'],
  [/\b(4k|uhd)\b/i, '4K'],
  [/\bsd\b/i, '480p'],
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

// Codecs are often stuck to their channel count ("AAC2.0", "DTS5.1"), so each
// ends at a word boundary or a digit.
const AUDIO = [
  [/\batmos(?=\d|\b)/i, 'Atmos'],
  [/\btrue-?hd(?=\d|\b)/i, 'TrueHD'],
  [/\bdts[ .:-]?x(?=\d|\b)/i, 'DTS:X'],
  [/\bdts[ .-]?hd([ .-]?ma)?(?=\d|\b)/i, 'DTS-HD MA'],
  [/\bdts(?=\d|\b)/i, 'DTS'],
  [/\b(ddp|dd\+|e-?ac-?3|eac3)/i, 'Dolby Digital Plus'],
  [/\b(dd|ac-?3)(?=\d|\b)/i, 'Dolby Digital'],
  [/\baac(?=\d|\b)/i, 'AAC'],
  [/\bflac(?=\d|\b)/i, 'FLAC'],
  [/\bopus(?=\d|\b)/i, 'Opus'],
];

// In order of preference when several appear; REMUX anywhere ("BDRemux") wins.
const SOURCE_TYPES = [
  [/remux/i, 'REMUX'],
  [/\b(hd-?cam|cam-?rip|cam)\b/i, 'CAM'],
  [/\b(hd-?ts|telesync|hd-?tc|telecine)\b/i, 'TS'],
  [/\b(dvd-?scr|screener|scr)\b/i, 'SCR'],
  [/\b(blu-?ray|bd-?rip|br-?rip|bdremux)\b/i, 'BluRay'],
  [/\bweb[ -]?dl\b/i, 'WEB-DL'],
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

// The title ends at the year, the episode number or the resolution. Words
// before it ("Cam", "Charlotte's Web", "Step Up 3D") are not claims.
const TITLE_END = /\s(?:19\d\d|20[0-3]\d|s\d{1,2}[ -]?e\d{1,3}|\d{1,2}x\d{2,3}|\d{3,4}[pi]|4k|uhd)(?=\s)/gi;
const THREE_D = /\b(3d|h-?sbs|half[ -]?sbs|sbs|h-?ou|half[ -]?ou|ou)\b/i;
const DISC_IMAGE = /\bcomplete[ -]?(uhd[ -]?)?blu-?ray\b|\biso\b|\bbdmv\b/i;
const SAMPLE = /\bsample\b/i;
// Source words no title uses, read anywhere ("[HDCAM] New Film 2026").
const UNMISTAKABLE_SOURCES = [
  [/\b(hd-?cam|cam-?rip)\b/i, 'CAM'],
  [/\b(hd-?ts|telesync|hd-?tc|telecine)\b/i, 'TS'],
  [/\b(dvd-?scr|screener)\b/i, 'SCR'],
  [/\b(blu-?ray|bd-?rip|br-?rip)\b/i, 'BluRay'],
  [/\bweb-dl\b/i, 'WEB-DL'],
  [/\bweb-?rip\b/i, 'WEBRip'],
];

/** The part of a normalised name after the title, or all of it. */
function afterTitle(t) {
  TITLE_END.lastIndex = 0;
  let match;
  while ((match = TITLE_END.exec(t))) {
    // A name may start with a number that belongs to the title ("1917 2019").
    if (match.index > 0) return t.slice(match.index);
  }
  return t;
}

/** Extract media claims from a release or file name. */
function parseRelease(text) {
  // Separators in release names: dots, underscores, brackets.
  const t = ` ${String(text || '').replace(/[._[\]()]+/g, ' ')} `;
  const tail = afterTitle(t);
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
    sourceType: /remux/i.test(t) ? 'REMUX' : first(SOURCE_TYPES, tail) || first(UNMISTAKABLE_SOURCES, t),
    threeD: THREE_D.test(tail) || undefined,
    discImage: DISC_IMAGE.test(t) || undefined,
    sample: SAMPLE.test(t) || undefined,
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

  const parts = [release, fileName, nameLines.slice(1).join(' ')].filter(Boolean);
  const parsed = parseRelease(parts.join(' '));
  // Title-sensitive claims are read per name, so one name's title can't leak
  // into another's tail. The release name speaks first.
  const each = parts.map(parseRelease);
  const sourceType = each.map(p => p.sourceType).filter(Boolean)[0];
  const container = parseRelease(fileName || '').container;
  const seeders = /👤\s*([\d,]+)/.exec(body);
  const sizeBytes = Number(hints.videoSize) > 0 ? Number(hints.videoSize) : parseSize(body);
  const languages = flags(body);
  const multiAudio = /\b(multi|dual)[ .-]?(audio|lang)/i.test(body + ' ' + release);
  const fileIdx = Number.isInteger(stream.fileIdx) ? stream.fileIdx : undefined;
  const pack = PACK.test(release) && !EPISODE.test(release);

  const claims = {};
  for (const key of ['resolution', 'hdr', 'videoCodec', 'audio', 'channels', 'discImage', 'sample']) if (parsed[key]) claims[key] = parsed[key];
  if (each.some(p => p.threeD)) claims.threeD = true;
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
    ...(sourceType ? { sourceType } : {}),
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
