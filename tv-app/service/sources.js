// Source discovery across several public indexes (2.5.0).
//
// Before 2.5 Bloob asked one index, Torrentio. Kodi's CocoScrapers finds rarer
// releases because it asks Torrentio plus many more indexes and merges them by
// torrent hash; Bloob now does the same, narrowly: only JSON indexes keyed by
// IMDb ID, none of which is ever sent the AllDebrid key.
//
//   torrentio   Stremio addon, pre-scraped, names the file in a pack
//   torrentsdb  Stremio addon in Torrentio's format (YTS, EZTV, 1337x, Knaben, TPB…)
//   stremthru   public Torznab JSON; includes Debrid Media Manager hash lists,
//               rich in 4K remux, Dolby Vision and Atmos releases
//   apibay      The Pirate Bay's JSON API, searched by IMDb ID
//   eztv        EZTV's JSON API (series only)
//
// Every provider has its own timeout. The answer goes back once Torrentio (the
// fast one) and the others have answered, or a short grace after Torrentio,
// whichever is first; slower answers still join the cached list for the next
// look. Every field is still a claim parsed from a release name.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const { getJson } = require('./net');
const { parseStream, parseStreams } = require('./release');

const USER_TIMEOUT_MS = 6000;     // a provider slower than this is skipped for this search
const GRACE_AFTER_FIRST_MS = 2500; // after Torrentio answers, wait at most this for the rest
const PER_PROVIDER_LIMIT = 250;

const LANGUAGE_CODES = {
  en: 'English', fr: 'French', es: 'Spanish', 'es-419': 'Spanish', de: 'German', it: 'Italian', pt: 'Portuguese',
  ru: 'Russian', uk: 'Ukrainian', pl: 'Polish', hu: 'Hungarian', nl: 'Dutch', zh: 'Chinese', ja: 'Japanese',
  ko: 'Korean', hi: 'Hindi', ta: 'Tamil', te: 'Telugu', ar: 'Arabic', tr: 'Turkish', cs: 'Czech', sv: 'Swedish',
};

/** A Stremio addon's stream list (Torrentio's format). */
function stremio(host) {
  return async (type, id) => {
    const json = await getJson(`https://${host}/stream/${type}/${encodeURIComponent(id).replace(/%3A/g, ':')}.json`,
      { timeout: USER_TIMEOUT_MS, maxBytes: 5000000 });
    return { streams: json.streams, limit: 60 };
  };
}

/**
 * A torrent from a plain index, shaped as a Stremio stream so one parser reads
 * every source. A pack's size is the whole pack, so it is kept apart from the
 * file size the bitrate estimate uses.
 */
function torrentStream(t) {
  const hints = {};
  if (t.fileName) hints.filename = t.fileName;
  if (t.size > 0) hints.videoSize = t.size;
  const stats = t.seeders !== undefined ? `\n👤 ${t.seeders}` : '';
  return { infoHash: t.hash, title: `${t.name}${stats}`, behaviorHints: hints };
}

function videoName(name) {
  return /\.(mkv|mp4|m4v|ts|avi)$/i.test(name) ? name : undefined;
}

function attrs(item) {
  const out = {};
  (item.attr || []).forEach(a => {
    const at = a && a['@attributes'];
    if (at && at.name) out[at.name] = at.value;
  });
  return out;
}

function languagesOf(value) {
  const codes = String(value || '').toLowerCase().split(',').map(s => s.trim()).filter(Boolean);
  const multiAudio = codes.some(c => c === 'multi audio' || c === 'dual audio');
  const languages = [];
  codes.forEach(c => {
    const name = LANGUAGE_CODES[c];
    if (name && languages.indexOf(name) < 0) languages.push(name);
  });
  return { languages, multiAudio };
}

/** StremThru's public Torznab, as JSON. */
async function stremthru(type, id) {
  const [imdb, season, episode] = id.split(':');
  const query = type === 'series'
    ? `t=tvsearch&imdbid=${imdb}&season=${season}&ep=${episode}`
    : `t=movie&imdbid=${imdb}`;
  const json = await getJson(`https://stremthru.13377001.xyz/v0/torznab/api?${query}&o=json`, { timeout: USER_TIMEOUT_MS, maxBytes: 4000000 });
  const items = ((json && json.channel) || {}).items || [];
  return {
    torrents: items.map(item => {
      const a = attrs(item);
      const lang = languagesOf(a.language);
      return {
        hash: String(a.infohash || item.guid || '').toLowerCase(),
        name: String(item.title || ''),
        fileName: videoName(String(item.title || '')),
        size: Number(a.size) || 0,
        seeders: a.seeders !== undefined && a.seeders !== '' ? Number(a.seeders) : undefined,
        languages: lang.languages,
        multiAudio: lang.multiAudio,
      };
    }),
  };
}

/** The Pirate Bay's JSON API, which accepts an IMDb ID as the query. */
async function apibay(type, id) {
  const imdb = id.split(':')[0];
  const list = await getJson(`https://apibay.org/q.php?q=${imdb}&cat=0`, { timeout: USER_TIMEOUT_MS, maxBytes: 2000000 });
  return {
    torrents: (Array.isArray(list) ? list : [])
      .filter(t => t && /^[a-f0-9]{40}$/i.test(t.info_hash || '') && !/^0+$/.test(t.info_hash))
      .map(t => ({ hash: t.info_hash.toLowerCase(), name: String(t.name || ''), size: Number(t.size) || 0, seeders: Number(t.seeders) || 0 })),
  };
}

/** EZTV's JSON API: one show's torrents, by IMDb number. */
async function eztv(type, id) {
  if (type !== 'series') return { torrents: [] };
  const imdb = id.split(':')[0].replace(/^tt/, '');
  const json = await getJson(`https://eztvx.to/api/get-torrents?imdb_id=${imdb}&limit=100&page=1`, { timeout: USER_TIMEOUT_MS, maxBytes: 2000000 });
  return {
    torrents: ((json && json.torrents) || []).map(t => ({
      hash: String(t.hash || '').toLowerCase(),
      name: String(t.title || t.filename || ''),
      fileName: videoName(String(t.filename || '').replace(/\[eztv[^\]]*\]/i, '')),
      size: Number(t.size_bytes) || 0,
      seeders: Number(t.seeds) || 0,
    })),
  };
}

const PROVIDERS = [
  { id: 'torrentio', label: 'Torrentio', primary: true, search: stremio('torrentio.strem.fun') },
  { id: 'torrentsdb', label: 'TorrentsDB', search: stremio('torrentsdb.com') },
  { id: 'stremthru', label: 'StremThru', search: stremthru },
  { id: 'apibay', label: 'The Pirate Bay', search: apibay },
  { id: 'eztv', label: 'EZTV', series: true, search: eztv },
];

const EPISODE_TAG = /\bs\d{1,2}[ ._-]?e\d{1,3}\b|\b\d{1,2}x\d{2,3}\b/i;

/**
 * Whether a torrent named `name` can hold episode `episode` of `season`: the
 * episode itself, a pack of its season, a range of seasons that includes it,
 * or a complete series. Indexes keyed by show (not episode) need this filter.
 */
function fitsEpisode(name, season, episode) {
  const text = String(name || '');
  const exact = new RegExp(`\\bs0*${season}[ ._-]?e0*${episode}(?!\\d)|\\b${season}x0*${episode}(?!\\d)`, 'i');
  if (exact.test(text)) return true;
  if (EPISODE_TAG.test(text)) return false;  // another episode
  const range = /\bs(?:eason)?[ ._-]?0*(\d{1,2})[ ._]*(?:-|to)[ ._]*s?(?:eason)?[ ._-]?0*(\d{1,2})\b/i.exec(text);
  if (range) return season >= Number(range[1]) && season <= Number(range[2]);
  const seasons = [];
  const pattern = /\b(?:s|season[ ._-]?)0*(\d{1,2})(?!\d)/gi;
  let m;
  while ((m = pattern.exec(text))) seasons.push(Number(m[1]));
  if (seasons.length) return seasons.indexOf(season) >= 0;
  return /\b(complete|collection|all[ ._-]seasons)\b/i.test(text);
}

/** Candidates from one provider's answer, for the requested title or episode. */
function candidatesFrom(provider, answer, type, id) {
  if (answer.streams) return parseStreams(answer.streams, provider.id, answer.limit || 60);
  const parts = id.split(':');
  const season = Number(parts[1]);
  const episode = Number(parts[2]);
  const seen = new Set();
  const out = [];
  for (const t of answer.torrents || []) {
    if (!/^[a-f0-9]{40}$/.test(t.hash) || seen.has(t.hash) || !t.name) continue;
    if (type === 'series' && !fitsEpisode(t.name, season, episode)) continue;
    seen.add(t.hash);
    const candidate = parseStream(torrentStream(t), provider.id);
    if (!candidate) continue;
    if (t.languages && t.languages.length && !candidate.languages.length) candidate.languages = t.languages;
    if (t.multiAudio) candidate.multiAudio = true;
    if (candidate.pack && candidate.sizeBytes) {
      // The pack's size, not the episode's: keep it out of the bitrate estimate.
      candidate.packBytes = candidate.sizeBytes;
      delete candidate.sizeBytes;
    }
    out.push(candidate);
    if (out.length >= PER_PROVIDER_LIMIT) break;
  }
  return out;
}

/**
 * Merge by torrent hash. Providers are listed best-described first, so the
 * first copy of a hash keeps its file name and index; later copies add their
 * provider and a higher seeder count.
 */
function merge(lists) {
  const byKey = new Map();
  const result = [];
  lists.forEach(list => list.forEach(c => {
    const key = c.kind === 'magnet' ? c.value : c.id;
    const have = byKey.get(key);
    if (!have) {
      const copy = Object.assign({}, c, { providers: [c.provider] });
      byKey.set(key, copy);
      result.push(copy);
      return;
    }
    if (have.providers.indexOf(c.provider) < 0) have.providers.push(c.provider);
    if (c.seeders !== undefined && (have.seeders === undefined || c.seeders > have.seeders)) have.seeders = c.seeders;
    if (!have.sizeBytes && c.sizeBytes) have.sizeBytes = c.sizeBytes;
    if (!have.fileName && c.fileName) have.fileName = c.fileName;
  }));
  return result;
}

/**
 * Search the chosen providers. `onLate(candidates, report)` receives the full
 * merged list if a provider answers after the reply went back.
 * Resolves to { candidates, report: [{ id, label, count, ms, error }] }.
 */
function discoverAll(type, id, options) {
  options = options || {};
  const chosen = PROVIDERS.filter(p => (options.only ? options.only.indexOf(p.id) >= 0 : true) && (!p.series || type === 'series'));
  const providers = options.providers || chosen;
  const started = Date.now();
  const lists = providers.map(() => []);
  const report = providers.map(p => ({ id: p.id, label: p.label, count: 0, ms: 0, error: '' }));
  let replied = false;
  return new Promise(resolve => {
    let pending = providers.length;
    let graceTimer = null;
    const finish = () => {
      if (replied) return;
      replied = true;
      if (graceTimer) clearTimeout(graceTimer);
      resolve({ candidates: merge(lists), report: report.map(r => Object.assign({}, r)) });
    };
    const hardTimer = setTimeout(finish, USER_TIMEOUT_MS + 500);
    providers.forEach((p, i) => {
      Promise.resolve().then(() => p.search(type, id)).then(answer => {
        lists[i] = candidatesFrom(p, answer || {}, type, id);
        report[i].count = lists[i].length;
      }, error => {
        report[i].error = String((error && error.message) || error).slice(0, 80);
      }).then(() => {
        report[i].ms = Date.now() - started;
        pending--;
        if (pending === 0) {
          clearTimeout(hardTimer);
          if (!replied) finish();
          else if (options.onLate) options.onLate(merge(lists), report);
          return;
        }
        if (replied) {
          if (options.onLate) options.onLate(merge(lists), report);
          return;
        }
        if (p.primary && !graceTimer) graceTimer = setTimeout(finish, GRACE_AFTER_FIRST_MS);
      });
    });
    if (!providers.length) finish();
  });
}

module.exports = { discoverAll, fitsEpisode, merge, candidatesFrom, torrentStream, PROVIDERS, GRACE_AFTER_FIRST_MS };
