// Skip intro, recap and credits (4.0): where they are in a film or episode,
// from public, keyless community databases keyed by IMDb ID:
//   - TheIntroDB (api.theintrodb.org/v3/media): intro, recap, credits in ms;
//   - IntroDB (api.introdb.app/segments): intro, recap, outro, with
//     "no_intro"-style flags;
//   - AniSkip (api.aniskip.com/v2), anime only: openings, endings and recaps
//     by MyAnimeList ID and the entry's episode number (anime.js maps IMDb
//     seasons to MyAnimeList entries).
// Each source's answer is returned on its own, best first; the app merges them
// behind the file's own chapters and drops what doesn't fit the file's length
// (native/src/skips.cpp). Answers are kept 6 hours, failures never.
// Nothing here is sent the AllDebrid key.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const { getJson } = require('./net');
const anime = require('./anime');

const KEEP_MS = 6 * 60 * 60 * 1000;
const TIMEOUT_MS = 8000;
const cache = new Map();  // key -> { at, value }

function ms(v) {
  const n = Number(v);
  return isFinite(n) && n >= 0 ? Math.round(n) : -1;
}

function segment(kind, startMs, endMs) {
  const start = ms(startMs);
  if (start < 0) return null;
  const end = endMs === null || endMs === undefined ? -1 : ms(endMs);
  if (end >= 0 && end <= start) return null;
  return { kind, startMs: start, endMs: end };
}

/** TheIntroDB v3: { intro: [{start_ms, end_ms}], recap: [...], credits: [{start_ms, end_ms|null}] }. */
function fromTheIntroDb(j) {
  const out = [];
  const add = (kind, list) => (Array.isArray(list) ? list : []).slice(0, 1).forEach(x => {
    const s = x && segment(kind, x.start_ms, x.end_ms);
    if (s) out.push(s);
  });
  if (j && typeof j === 'object') {
    add('intro', j.intro);
    add('recap', j.recap);
    add('credits', j.credits || j.outro);
  }
  return out;
}

/** IntroDB: { intro: {start_ms, end_ms}, recap, outro, no_intro… }. */
function fromIntroDb(j) {
  const out = [];
  if (!j || typeof j !== 'object') return out;
  const add = (kind, x) => {
    const s = x && segment(kind, x.start_ms, x.end_ms);
    if (s) out.push(s);
  };
  add('intro', j.intro);
  add('recap', j.recap);
  add('credits', j.outro);
  return out;
}

/**
 * AniSkip v2: results [{ interval: {startTime, endTime} (seconds), skipType
 * "op" | "ed" | "recap" | "mixed-op" | "mixed-ed", episodeLength }]. Several
 * submissions can disagree; the one whose episode length is nearest the file's wins.
 */
function fromAniSkip(j, durationMs) {
  const results = j && Array.isArray(j.results) ? j.results : [];
  const kinds = { op: 'intro', 'mixed-op': 'intro', recap: 'recap', ed: 'credits', 'mixed-ed': 'credits' };
  const best = {};
  results.forEach(r => {
    const kind = kinds[r && r.skipType];
    const iv = r && r.interval;
    if (!kind || !iv) return;
    const s = segment(kind, Number(iv.startTime) * 1000, Number(iv.endTime) * 1000);
    if (!s) return;
    const length = Number(r.episodeLength) * 1000;
    // Without the file's length, plain op/ed beat the "mixed" ones.
    const off = durationMs > 0 && length > 0 ? Math.abs(length - durationMs) : String(r.skipType).indexOf('mixed') === 0 ? 1 : 0;
    if (durationMs > 0 && length > 0 && off > durationMs * 0.1) return;  // made for another cut
    if (!best[kind] || off < best[kind].off) best[kind] = { off, s };
  });
  return ['intro', 'recap', 'credits'].filter(k => best[k]).map(k => best[k].s);
}

async function ask(name, url, parse) {
  try {
    const j = await getJson(url, { timeout: TIMEOUT_MS, maxBytes: 200000 });
    return { name, segments: parse(j) };
  } catch (e) {
    // 404 means "nothing known": an answer, not a failure.
    return e && e.statusCode === 404 ? { name, segments: [] } : { name, segments: [], failed: true };
  }
}

/**
 * The skips for `type` ('movie' | 'series') and `id` (`tt…` or `tt…:S:E`).
 * `durationMs`: the file's length, when known (AniSkip's choice of submission).
 * Returns { sources: [{ name, segments: [{ kind, startMs, endMs }] }] }, best first.
 */
async function skipsFor(type, id, durationMs) {
  const [imdb, s, e] = String(id || '').split(':');
  if (!/^tt\d+$/.test(imdb)) return { sources: [] };
  const season = Number(s) || 0, episode = Number(e) || 0;
  const isEpisode = type === 'series' && season > 0 && episode > 0;
  const key = `${type}:${id}:${Math.round((durationMs || 0) / 60000)}`;
  const kept = cache.get(key);
  if (kept && Date.now() - kept.at < KEEP_MS) return kept.value;

  const enc = encodeURIComponent(imdb);
  const asks = [
    ask('TheIntroDB', isEpisode ? `https://api.theintrodb.org/v3/media?imdb_id=${enc}&season=${season}&episode=${episode}`
      : `https://api.theintrodb.org/v3/media?imdb_id=${enc}`, fromTheIntroDb),
    ask('IntroDB', isEpisode ? `https://api.introdb.app/segments?imdb_id=${enc}&season=${season}&episode=${episode}`
      : `https://api.introdb.app/segments?imdb_id=${enc}&is_movie=true`, fromIntroDb),
  ];
  let isAnime = false;
  if (isEpisode) {
    const ctx = await anime.context(type, id).catch(() => null);
    if (ctx && ctx.malId && ctx.episode > 0) {
      isAnime = true;
      asks.push(ask('AniSkip', `https://api.aniskip.com/v2/skip-times/${ctx.malId}/${ctx.episode}?types=op&types=ed&types=recap&types=mixed-op&types=mixed-ed&episodeLength=0`,
        j => fromAniSkip(j, durationMs || 0)));
    }
  }
  const answers = await Promise.all(asks);
  // For anime, AniSkip's community times are made per MyAnimeList entry: they lead.
  if (isAnime) answers.unshift(answers.pop());
  const value = { sources: answers.map(a => ({ name: a.name, segments: a.segments })) };
  if (!answers.some(a => a.failed)) {
    cache.set(key, { at: Date.now(), value });
    if (cache.size > 300) cache.delete(cache.keys().next().value);
  }
  return value;
}

module.exports = { skipsFor, fromTheIntroDb, fromIntroDb, fromAniSkip };
