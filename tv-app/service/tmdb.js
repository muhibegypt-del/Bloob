// TMDB rows for Home, Films and Series (2.7): trending, collections, studios,
// networks, acclaimed and decade picks, some rotating daily so Home changes.
// Items keep IMDb IDs (sources and Cinemeta details are keyed by them); the
// TMDB → IMDb mapping is fetched once per title and kept in tmdb-ids.json.
//
// The key is the owner's, in keys.json in the service's data folder
// ({"tmdb": "<key>"}), written on the TV by hand. It is never sent to the app,
// logged or committed; requests carry it only to api.themoviedb.org. Without a
// key, catalog.js keeps its Cinemeta rows.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const { getJson } = require('./net');
const accountStore = require('./account-store');
const keys = require('./keys');

const API = 'https://api.themoviedb.org/3';
const IMG = 'https://image.tmdb.org/t/p';
const LOGO = id => `https://images.metahub.space/logo/medium/${id}/img`;
const TTL = 30 * 60 * 1000;
const day = () => Math.floor(Date.now() / 86400000);

// Checked against the API on 23 October 2026.
const COLLECTIONS = [[230, 'The Godfather'], [119, 'The Lord of the Rings'], [263, 'The Dark Knight'], [2344, 'The Matrix'],
  [726871, 'Dune'], [8091, 'Alien'], [264, 'Back to the Future'], [84, 'Indiana Jones'], [8945, 'Mad Max'], [404609, 'John Wick'],
  [87359, 'Mission: Impossible'], [10, 'Star Wars'], [1241, 'Harry Potter'], [131635, 'The Hunger Games']];
const STUDIOS = [[41077, 'A24'], [10342, 'Studio Ghibli'], [3, 'Pixar']];
const NETWORKS = [[49, 'HBO'], [2552, 'Apple TV'], [213, 'Netflix']];
const DECADES = [1970, 1980, 1990, 2000, 2010];
// In browsing order.
const MOVIE_GENRES = [[28, 'Action'], [878, 'Science fiction'], [18, 'Drama'], [53, 'Thriller'], [35, 'Comedy'], [80, 'Crime'],
  [27, 'Horror'], [12, 'Adventure'], [14, 'Fantasy'], [16, 'Animation'], [9648, 'Mystery'], [10749, 'Romance'], [36, 'History'],
  [10752, 'War'], [37, 'Western'], [99, 'Documentary'], [10402, 'Music'], [10751, 'Family']];
const TV_GENRES = [[18, 'Drama'], [80, 'Crime'], [10765, 'Sci-fi & fantasy'], [35, 'Comedy'], [9648, 'Mystery'],
  [10759, 'Action & adventure'], [16, 'Animation'], [99, 'Documentary'], [10768, 'War & politics'], [10764, 'Reality'],
  [37, 'Western'], [10751, 'Family']];
const names = list => list.reduce((m, g) => { m[g[0]] = g[1]; return m; }, {});
const MOVIE_GENRE_NAMES = names(MOVIE_GENRES), TV_GENRE_NAMES = names(TV_GENRES);

const pick = (list, offset) => list[(day() + (offset || 0)) % list.length];

// ---- key

const apiKey = () => keys.get('tmdb');

// ---- requests

const cache = new Map();
function cached(key, load) {
  const hit = cache.get(key);
  if (hit && hit.expires > Date.now()) return hit.value;
  const value = load();
  cache.set(key, { value, expires: Date.now() + TTL });
  value.catch(() => { if (cache.get(key) && cache.get(key).value === value) cache.delete(key); });
  if (cache.size > 200) cache.delete(cache.keys().next().value);
  return value;
}

async function api(path, params) {
  const key = await apiKey();
  if (!key) throw new Error('No TMDB key');
  const q = new URLSearchParams(Object.assign({ api_key: key, language: 'en-GB' }, params || {}));
  return getJson(`${API}${path}?${q}`, { timeout: 8000 });
}

// A few lookups at a time, so a first Home load does not flood the TV's network.
let active = 0;
const waiting = [];
function limited(work) {
  return new Promise((resolve, reject) => {
    const run = () => { active++; work().then(resolve, reject).then(() => { active--; if (waiting.length) waiting.shift()(); }); };
    if (active < 16) run(); else waiting.push(run);
  });
}

// ---- TMDB → IMDb

const idStore = accountStore.createFileStore('tmdb-ids.json', accountStore.defaultLocations);
let ids = null;
let saveTimer = null;
async function idMap() {
  if (!ids) ids = (await idStore.read().catch(() => null)) || {};
  return ids;
}
async function imdbFor(kind, tmdbId) {
  const map = await idMap();
  const k = `${kind}:${tmdbId}`;
  if (Object.prototype.hasOwnProperty.call(map, k)) return map[k];
  const data = await limited(() => api(`/${kind}/${tmdbId}/external_ids`)).catch(() => null);
  if (!data) return '';  // try again another time
  map[k] = /^tt\d+$/.test(String(data.imdb_id || '')) ? data.imdb_id : '';
  if (!saveTimer) saveTimer = setTimeout(() => { saveTimer = null; idStore.write(map).catch(() => {}); }, 3000);
  return map[k];
}

// ---- items

function item(kind, r, imdb) {
  const genres = kind === 'tv' ? TV_GENRE_NAMES : MOVIE_GENRE_NAMES;
  const date = String(r.release_date || r.first_air_date || '');
  return {
    id: imdb,
    type: kind === 'tv' ? 'series' : 'movie',
    name: r.title || r.name || '',
    year: date.slice(0, 4),
    runtime: '',
    genres: (r.genre_ids || []).map(g => genres[g]).filter(Boolean).slice(0, 3),
    rating: '',  // the app labels ratings as IMDb's; TMDB's score is not
    description: r.overview || '',
    tile: `${IMG}/w780${r.backdrop_path}`,
    backdrop: `${IMG}/w1280${r.backdrop_path}`,
    logo: LOGO(imdb),
    poster: r.poster_path ? `${IMG}/w342${r.poster_path}` : '',
  };
}

async function items(results, kindOf) {
  const usable = (results || []).filter(r => r && r.backdrop_path && (r.title || r.name) && kindOf(r));
  const mapped = await Promise.all(usable.slice(0, 20).map(async r => {
    const kind = kindOf(r);
    const imdb = await imdbFor(kind, r.id);
    return imdb ? item(kind, r, imdb) : null;
  }));
  return mapped.filter(Boolean);
}

const fixed = kind => () => kind;
const byMediaType = r => (r.media_type === 'movie' ? 'movie' : r.media_type === 'tv' ? 'tv' : '');

// [id, title, load] → { id, title, items }
function row(id, title, load) {
  return cached(id, load).then(list => ({ id, title, items: list }), () => ({ id, title, items: [] }));
}
const list = (path, params, kindOf) => () => api(path, params).then(d => items(d.results, kindOf));
const collection = c => () => api(`/collection/${c}`).then(d => {
  const parts = (d.parts || []).slice().sort((a, b) => String(a.release_date || '9').localeCompare(String(b.release_date || '9')));
  return items(parts, fixed('movie'));
});
const page = n => String(1 + ((day() + n) % 5));  // acclaimed lists go five pages deep, a different one each day
// Acclaimed: well rated by many people (TMDB's own top-rated list favours titles with few votes).
const acclaimed = (kind, n) => list(`/discover/${kind}`, { sort_by: 'vote_average.desc', 'vote_count.gte': kind === 'tv' ? '1500' : '5000',
  page: page(n) }, fixed(kind));

function decadeRow(offset) {
  const from = pick(DECADES, offset);
  return row(`decade-${from}`, `Best of the ${from}s`, list('/discover/movie', {
    sort_by: 'vote_average.desc', 'vote_count.gte': '3000',
    'primary_release_date.gte': `${from}-01-01`, 'primary_release_date.lte': `${from + 9}-12-31` }, fixed('movie')));
}

// ---- rows

async function enabled() {
  return Boolean(await apiKey());
}

function home() {
  const c = pick(COLLECTIONS), s = pick(STUDIOS), n = pick(NETWORKS);
  return Promise.all([
    row('trending-today', 'Trending today', list('/trending/all/day', {}, byMediaType)),
    row('trending-movies', 'Films everyone’s watching', list('/trending/movie/week', {}, fixed('movie'))),
    row('trending-series', 'Series everyone’s watching', list('/trending/tv/week', {}, fixed('tv'))),
    row(`collection-${c[0]}`, `${c[1]} collection`, collection(c[0])),
    row(`studio-${s[0]}`, `From ${s[1]}`, list('/discover/movie', { with_companies: String(s[0]), sort_by: 'popularity.desc' }, fixed('movie'))),
    row(`network-${n[0]}`, `${n[1]} series`, list('/discover/tv', { with_networks: String(n[0]), sort_by: 'popularity.desc' }, fixed('tv'))),
    row(`movie-acclaimed-${page(0)}`, 'Acclaimed films', acclaimed('movie', 0)),
    decadeRow(0),
    row(`tv-acclaimed-${page(2)}`, 'Acclaimed series', acclaimed('tv', 2)),
  ]).then(rows => rows.filter(r => r.items.length));
}

function browse(type) {
  const tv = type === 'series';
  const kind = tv ? 'tv' : 'movie';
  const genres = tv ? TV_GENRES : MOVIE_GENRES;
  const noun = tv ? 'series' : 'films';
  const specs = [
    row(`${kind}-trending`, `Trending ${noun}`, list(`/trending/${kind}/week`, {}, fixed(kind))),
    row(`${kind}-popular`, `Popular ${noun}`, list(`/${kind}/popular`, {}, fixed(kind))),
    row(`${kind}-acclaimed-${page(1)}`, `Acclaimed ${noun}`, acclaimed(kind, 1)),
  ];
  if (tv) NETWORKS.forEach(n => specs.push(row(`network-${n[0]}`, `${n[1]} series`,
    list('/discover/tv', { with_networks: String(n[0]), sort_by: 'popularity.desc' }, fixed('tv')))));
  else {
    STUDIOS.forEach(s => specs.push(row(`studio-${s[0]}`, `From ${s[1]}`,
      list('/discover/movie', { with_companies: String(s[0]), sort_by: 'popularity.desc' }, fixed('movie')))));
    specs.push(decadeRow(1));
  }
  genres.forEach(g => specs.push(row(`${kind}-genre-${g[0]}`, g[1],
    list(`/discover/${kind}`, { with_genres: String(g[0]), sort_by: 'popularity.desc', 'vote_count.gte': '200' }, fixed(kind)))));
  return Promise.all(specs).then(rows => rows.filter(r => r.items.length));
}

// Films and Series need hundreds of IMDb lookups the first time; do them in the
// background after Home, so the tabs open from cache.
let warmed = 0;
function warm() {
  if (Date.now() - warmed < TTL / 2) return;
  warmed = Date.now();
  browse('movie').then(() => browse('series')).catch(() => {});
}

module.exports = { enabled, home, browse, warm, item, COLLECTIONS };
