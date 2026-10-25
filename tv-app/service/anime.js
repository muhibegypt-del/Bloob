// Anime (3.1): which titles are anime, and how anime indexes name an episode.
//
// Bloob's catalogue numbers episodes the IMDb way (Season 2, Episode 5). Anime
// sites give each season (often each cour) its own entry, and releases are
// named for that entry: "[SubsPlease] Sousou no Frieren S2 - 05 (1080p)",
// "… 2nd Season - 05", or by absolute number for long runners ("One Piece - 1120").
// Fribb's anime-lists (github.com/Fribb/anime-lists, refreshed by its author
// daily) maps IMDb and TVDB seasons to Kitsu, AniList and AniDB entries with
// episode offsets; Bloob keeps a small index of it, refreshed weekly. AniList
// gives each entry's titles (romaji, English, synonyms) for text searches.
// Nothing here is sent the AllDebrid key.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const accountStore = require('./account-store');
const { getJson, postJson } = require('./net');

const LIST_URL = 'https://raw.githubusercontent.com/Fribb/anime-lists/master/anime-list-mini.json';
const REFRESH_MS = 7 * 24 * 60 * 60 * 1000;

const indexStore = accountStore.createFileStore('anime-index.json', accountStore.defaultLocations);
const titleStore = accountStore.createFileStore('anime-titles.json', accountStore.defaultLocations);

let index = null;      // { at, byImdb: { tt…: [entry] } }
let loading = null;
let titles = null;     // { anilistId: { at, titles: [] } }
let titleSaveTimer = null;
const contexts = new Map();  // `${type}:${id}` -> Promise<context|null>

/** A compact entry: Kitsu and AniList IDs, type, TVDB/TMDB season and offset. */
function compact(e) {
  const season = e.season || {};
  const offset = e.episode_offset || {};
  return {
    kitsu: e.kitsu_id || 0,
    anilist: e.anilist_id || 0,
    type: e.type || '',
    tvdbSeason: season.tvdb !== undefined ? season.tvdb : null,
    tvdbOffset: offset.tvdb || 0,
    tmdbSeason: season.tmdb !== undefined ? season.tmdb : null,
    tmdbOffset: offset.tmdb || 0,
  };
}

function buildIndex(list) {
  const byImdb = {};
  (Array.isArray(list) ? list : []).forEach(e => {
    (e.imdb_id || []).forEach(id => {
      if (!/^tt\d+$/.test(id) || (!e.kitsu_id && !e.anilist_id)) return;
      (byImdb[id] = byImdb[id] || []).push(compact(e));
    });
  });
  return { at: Date.now(), byImdb };
}

async function refresh() {
  const list = await getJson(LIST_URL, { timeout: 60000, maxBytes: 12000000 });
  const built = buildIndex(list);
  if (Object.keys(built.byImdb).length < 1000) throw new Error('anime list looks incomplete');
  index = built;
  indexStore.write(index).catch(() => {});
  console.log(`[anime] index: ${Object.keys(index.byImdb).length} titles`);
  return index;
}

/** The index, loading the saved one first; refreshes in the background when a week old. */
function load() {
  if (index && Date.now() - index.at < REFRESH_MS) return Promise.resolve(index);
  if (!loading) {
    loading = (async () => {
      if (!index) index = await indexStore.read().catch(() => null);
      if (index && index.byImdb) {
        if (Date.now() - index.at >= REFRESH_MS) refresh().catch(e => console.log(`[anime] refresh failed: ${e.message}`));
        return index;
      }
      return refresh();
    })().then(i => { loading = null; return i; }, e => { loading = null; console.log(`[anime] index unavailable: ${e.message}`); return null; });
  }
  return loading;
}

function warm() { load(); }

/** AniList's titles for an entry: romaji, English, native-free synonyms. */
async function titlesFor(anilistId) {
  if (!anilistId) return [];
  if (!titles) titles = (await titleStore.read().catch(() => null)) || {};
  const have = titles[anilistId];
  if (have && Date.now() - have.at < 30 * 24 * 60 * 60 * 1000) return have.titles;
  const query = 'query ($id: Int) { Media(id: $id, type: ANIME) { title { romaji english } synonyms episodes } }';
  const json = await postJson('https://graphql.anilist.co', { query, variables: { id: anilistId } }, { timeout: 6000 }).catch(() => null);
  const media = json && json.data && json.data.Media;
  if (!media) return have ? have.titles : [];
  const list = [media.title && media.title.romaji, media.title && media.title.english].concat(media.synonyms || [])
    .filter(t => t && /^[\x20-\x7eÀ-ɏ’]+$/.test(t));  // Latin script only: release names are
  const unique = list.filter((t, i) => list.findIndex(u => norm(u) === norm(t)) === i).slice(0, 6);
  titles[anilistId] = { at: Date.now(), titles: unique, episodes: media.episodes || 0 };
  if (!titleSaveTimer) titleSaveTimer = setTimeout(() => { titleSaveTimer = null; titleStore.write(titles).catch(() => {}); }, 3000);
  return unique;
}

function norm(s) {
  return String(s || '').toLowerCase().replace(/&/g, 'and').replace(/[^a-z0-9]+/g, ' ').trim();
}

/**
 * Which anime entry and episode number a catalogue episode is. `seasons` is the
 * catalogue's season list (for absolute numbers); returns null for non-anime.
 */
function entryFor(entries, type, season, episode, seasons) {
  if (!entries || !entries.length) return null;
  if (type === 'movie') {
    const movie = entries.find(e => e.type === 'MOVIE') || entries[0];
    return { entry: movie, episode: 0, absolute: 0 };
  }
  // Absolute number: every regular episode before this one.
  let absolute = episode;
  (seasons || []).forEach(s => { if (s.season > 0 && s.season < season) absolute += (s.episodes || []).length; });
  const pick = (seasonKey, offsetKey) => {
    const fits = entries.filter(e => e[seasonKey] === season && e[offsetKey] < episode);
    if (!fits.length) return null;
    const e = fits.sort((a, b) => b[offsetKey] - a[offsetKey])[0];  // the cour this episode falls in
    return { entry: e, episode: episode - e[offsetKey], absolute };
  };
  const bySeason = pick('tvdbSeason', 'tvdbOffset') || pick('tmdbSeason', 'tmdbOffset');
  if (bySeason) return bySeason;
  // A long runner with one entry and no season mapping (One Piece): absolute numbers.
  const whole = entries.find(e => e.tvdbSeason === null && e.type === 'TV');
  return whole ? { entry: whole, episode: absolute, absolute } : null;
}

/**
 * The anime context for a title or episode (`tt…` or `tt…:S:E`), or null when
 * it isn't anime. { kitsuId, anilistId, episode (the entry's number), absolute,
 * season, titles, showTitles }.
 */
function context(type, id) {
  const key = `${type}:${id}`;
  if (contexts.has(key)) return contexts.get(key);
  const promise = (async () => {
    const idx = await load();
    const [imdb, s, e] = String(id).split(':');
    const entries = idx && idx.byImdb[imdb];
    if (!entries) return null;
    const season = Number(s) || 0, episode = Number(e) || 0;
    let seasons = [];
    let showTitle = '';
    try {
      const meta = await require('./catalog').meta(type, imdb);
      seasons = meta.seasons || [];
      showTitle = meta.name || '';
    } catch (_) { /* titles from AniList only */ }
    const found = entryFor(entries, type, season, episode, seasons);
    if (!found) return { anime: true, kitsuId: 0, anilistId: 0, season, catalogEpisode: episode, episode, absolute: episode, titles: [], showTitles: [showTitle].filter(Boolean) };
    const own = await titlesFor(found.entry.anilist);
    // The series' first entry names the show ("Sousou no Frieren") for "S2 - 05" releases.
    const first = entries.find(x => x.tvdbSeason === 1) || entries[0];
    const base = first === found.entry ? own : await titlesFor(first.anilist);
    return {
      anime: true,
      kitsuId: found.entry.kitsu,
      anilistId: found.entry.anilist,
      season,
      catalogEpisode: episode,
      episode: found.episode,
      absolute: found.absolute,
      titles: own,
      showTitles: base.concat(showTitle ? [showTitle] : []),
    };
  })().catch(() => null);
  contexts.set(key, promise);
  if (contexts.size > 500) contexts.delete(contexts.keys().next().value);
  return promise;
}

/** "05", "5", "1120": the number as release names write it, never inside a longer number. */
function numberAt(n) {
  return `0*${n}(?:v\\d)?(?!\\d|p\\b|bit)`;
}

// SxxEyy naming: right if the season is the catalogue's and the number is the
// catalogue's, the entry's or the absolute one.
function sxeFits(raw, ctx) {
  const m = /\bs0*(\d{1,2})[ ._-]?e0*(\d{1,4})(?!\d)/i.exec(raw);
  if (!m) return null;
  const n = Number(m[2]);
  return Number(m[1]) === ctx.season && (n === ctx.catalogEpisode || n === ctx.episode || n === ctx.absolute);
}

// " - 05", " #05", "E05", "Ep 05", "Episode 5" for any of the numbers this episode goes by.
function dashFits(raw, ctx) {
  const numbers = [ctx.episode, ctx.absolute].filter((n, i, a) => n > 0 && a.indexOf(n) === i);
  return numbers.some(n => new RegExp(`(?:\\s-\\s|\\s#|\\bep?\\.?\\s?|\\bepisode\\s)${numberAt(n)}`, 'i').test(raw));
}

/**
 * Whether a release named `name` holds this episode: the show's title, then
 * SxxEyy, or " - 05" with the right season marker ("S2", "2nd Season") or the
 * season entry's own title, or the absolute number; or a batch whose range or
 * "Batch"/"Complete" covers it. Returns 'episode', 'batch' or ''.
 */
function matchEpisode(name, ctx) {
  const raw = String(name || '');
  const text = ` ${norm(raw)} `;
  const has = list => list.some(t => norm(t) && text.indexOf(` ${norm(t)} `) >= 0);
  const ownTitle = has(ctx.titles || []);
  if (!ownTitle && !has(ctx.showTitles || [])) return '';
  const sxe = sxeFits(raw, ctx);
  if (sxe !== null) return sxe ? 'episode' : '';
  const s = ctx.season;
  const marker = s > 1
    ? new RegExp(`\\b(?:s0*${s}|season\\s?0*${s}|${s}(?:st|nd|rd|th)\\s+season|part\\s?${s})\\b`, 'i').test(raw)
    : !/\b(?:s0*[2-9]|season\s?0*[2-9]|[2-9](?:nd|rd|th)\s+season)\b/i.test(raw);
  const thisSeason = ownTitle || marker;
  if (thisSeason && dashFits(raw, ctx)) return 'episode';
  if (ctx.absolute > ctx.episode && dashFits(raw, { episode: ctx.absolute, absolute: ctx.absolute })) return 'episode';
  if (!thisSeason) return '';
  const range = /(?:^|[\s[(])0*(\d{1,4})\s?(?:-|~|to)\s?0*(\d{1,4})(?=[\s\])]|$)/.exec(raw);
  if (range) {
    const from = Number(range[1]), to = Number(range[2]);
    const inside = n => n >= from && n <= to;
    if (to > from && (inside(ctx.episode) || inside(ctx.absolute))) return 'batch';
    if (to > from) return '';
  }
  return /\b(batch|complete|bd\s?box)\b/i.test(raw) ? 'batch' : '';
}

/** The file in a pack that is this episode (largest when several match), or null. */
function episodeFile(files, ctx) {
  const matches = (files || []).filter(f => {
    const base = String(f.name || '').split('/').pop().replace(/\.[a-z0-9]{2,4}$/i, '');
    const sxe = sxeFits(base, ctx);
    if (sxe !== null) return sxe;
    const numbers = [ctx.episode, ctx.absolute].filter(n => n > 0);
    return numbers.some(n => new RegExp(`(?:^|[\\s._\\-\\[(])(?:e|ep|episode)?[\\s._]?${numberAt(n)}(?=[\\s._\\-\\])]|$)`, 'i').test(base));
  });
  if (!matches.length) return null;
  return matches.sort((a, b) => (b.size || b.length || 0) - (a.size || a.length || 0))[0];
}

module.exports = { warm, load, context, entryFor, matchEpisode, episodeFile, titlesFor, norm, buildIndex };
