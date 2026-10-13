// Catalogue for the native app: home and browse rows, title details with
// seasons, and search, from Cinemeta, normalised and cached. Artwork URLs point
// at sizes the TV decodes cheaply (480×270 tiles, 1080p backdrops, PNG logos).
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const { getJson } = require('./net');

const CINEMETA = 'https://v3-cinemeta.strem.io';
const ART = 'https://images.metahub.space';
const TTL = 30 * 60 * 1000;
const cache = new Map();

function cached(key, ttl, load) {
  const hit = cache.get(key);
  if (hit && hit.expires > Date.now()) return hit.value;
  const value = load();
  cache.set(key, { value, expires: Date.now() + ttl });
  // A failure must not be cached.
  value.catch(() => { if (cache.get(key) && cache.get(key).value === value) cache.delete(key); });
  if (cache.size > 300) cache.delete(cache.keys().next().value);
  return value;
}

const art = {
  tile: id => `${ART}/background/small/${id}/img`,
  backdrop: id => `${ART}/background/medium/${id}/img`,
  logo: id => `${ART}/logo/medium/${id}/img`,
  poster: id => `${ART}/poster/small/${id}/img`,
};

function year(m) {
  if (m.releaseInfo) return String(m.releaseInfo);
  if (m.year) return String(m.year);
  return '';
}

function preview(m) {
  const genres = m.genres || m.genre || [];
  return {
    id: m.id,
    type: m.type,
    name: m.name || '',
    year: year(m),
    runtime: m.runtime || '',
    genres: genres.slice(0, 3),
    rating: m.imdbRating ? String(m.imdbRating) : '',
    description: m.description || '',
    tile: art.tile(m.id),
    backdrop: art.backdrop(m.id),
    logo: art.logo(m.id),
    poster: art.poster(m.id),
  };
}

function usable(m) {
  return m && typeof m.id === 'string' && /^tt\d+$/.test(m.id) && m.name && m.background;
}

function list(type, catalog, extra) {
  const path = `/catalog/${type}/${catalog}${extra ? '/' + extra : ''}.json`;
  return cached(path, TTL, async () => {
    const data = await getJson(CINEMETA + path, { timeout: 8000 });
    return (data.metas || []).filter(usable).map(m => preview(Object.assign({ type }, m)));
  });
}

const HOME = [
  ['popular-movies', 'Popular films', 'movie', 'top'],
  ['popular-series', 'Popular series', 'series', 'top'],
  ['rated-movies', 'Highest rated films', 'movie', 'imdbRating'],
  ['rated-series', 'Highest rated series', 'series', 'imdbRating'],
  ['action', 'Action', 'movie', 'top', 'genre=Action'],
  ['scifi', 'Science fiction', 'movie', 'top', 'genre=Sci-Fi'],
  ['comedy', 'Comedy', 'movie', 'top', 'genre=Comedy'],
  ['animation', 'Animation', 'movie', 'top', 'genre=Animation'],
  ['drama-series', 'Drama series', 'series', 'top', 'genre=Drama'],
  ['documentary', 'Documentary', 'movie', 'top', 'genre=Documentary'],
];

const GENRES = {
  movie: ['Action', 'Adventure', 'Comedy', 'Crime', 'Drama', 'Thriller', 'Sci-Fi', 'Horror', 'Romance', 'Animation', 'Family', 'Documentary'],
  series: ['Drama', 'Crime', 'Comedy', 'Sci-Fi', 'Mystery', 'Action', 'Animation', 'Documentary', 'Reality-TV', 'Fantasy'],
};

async function rows(specs) {
  const lists = await Promise.all(specs.map(spec => list(spec[2], spec[3], spec[4]).catch(() => [])));
  return specs
    .map((spec, i) => ({ id: spec[0], title: spec[1], items: lists[i].slice(0, 24) }))
    .filter(row => row.items.length);
}

function home() {
  return rows(HOME).then(result => {
    if (!result.length) throw new Error('The catalogue is unreachable. Check the TV\'s internet connection.');
    return { rows: result };
  });
}

function browse(type) {
  if (type !== 'movie' && type !== 'series') return Promise.reject(new Error('Unknown type'));
  const noun = type === 'movie' ? 'films' : 'series';
  const specs = [[`${type}-popular`, `Popular ${noun}`, type, 'top'], [`${type}-rated`, `Highest rated ${noun}`, type, 'imdbRating']]
    .concat(GENRES[type].map(g => [`${type}-${g.toLowerCase()}`, g === 'Sci-Fi' ? 'Science fiction' : g.replace('-TV', ' TV'), type, 'top', `genre=${g}`]));
  return rows(specs).then(result => ({ rows: result }));
}

// Some listings shout ("THE ONE WHERE…"). Quiet those down; leave mixed case alone.
const SMALL_WORDS = new Set(['a', 'an', 'and', 'as', 'at', 'but', 'by', 'for', 'in', 'of', 'on', 'or', 'the', 'to', 'vs']);
function calmTitle(title) {
  if (!/[A-Z]{2}.*\s.*[A-Z]{2}/.test(title) || /[a-z]/.test(title)) return title;  // one word may be an acronym
  return title.toLowerCase().replace(/[a-z\u00e0-\u00ff][^\s-]*/g, (word, offset) =>
    offset > 0 && SMALL_WORDS.has(word) ? word : word.charAt(0).toUpperCase() + word.slice(1));
}

function episodeOf(v) {
  return {
    id: String(v.id || ''),
    season: Number(v.season) || 0,
    episode: Number(v.episode !== undefined ? v.episode : v.number) || 0,
    title: calmTitle(String(v.name || v.title || '')),
    overview: v.overview || v.description || '',
    released: v.released || v.firstAired || '',
    thumbnail: typeof v.thumbnail === 'string' && /^https:\/\//.test(v.thumbnail) ? v.thumbnail : '',
  };
}

function meta(type, id) {
  if ((type !== 'movie' && type !== 'series') || !/^tt\d+$/.test(String(id))) return Promise.reject(new Error('Invalid title'));
  return cached(`meta:${type}:${id}`, TTL, async () => {
    const data = await getJson(`${CINEMETA}/meta/${type}/${id}.json`, { timeout: 8000 });
    const m = data.meta;
    if (!m) throw new Error('Title not found');
    const result = Object.assign(preview(Object.assign({ type }, m)), {
      cast: (m.cast || []).slice(0, 6),
      director: (m.director || []).slice(0, 2),
      released: m.released || '',
      seasons: [],
    });
    if (type === 'series') {
      const bySeason = new Map();
      (m.videos || []).map(episodeOf).filter(e => /^tt\d+:\d+:\d+$/.test(e.id)).forEach(e => {
        if (!bySeason.has(e.season)) bySeason.set(e.season, []);
        bySeason.get(e.season).push(e);
      });
      // Regular seasons in order, specials (season 0) last.
      const numbers = Array.from(bySeason.keys()).sort((a, b) => (a === 0) - (b === 0) || a - b);
      result.seasons = numbers.map(n => ({
        season: n,
        label: n === 0 ? 'Specials' : `Season ${n}`,
        episodes: bySeason.get(n).sort((a, b) => a.episode - b.episode),
      }));
    }
    return result;
  });
}

function search(query) {
  const q = String(query || '').trim().slice(0, 80);
  if (q.length < 2) return Promise.resolve({ items: [] });
  const extra = 'search=' + encodeURIComponent(q);
  return Promise.all([list('movie', 'top', extra).catch(() => []), list('series', 'top', extra).catch(() => [])])
    .then(([movies, series]) => {
      // Interleave so both kinds appear near the top, keeping each list's relevance order.
      const items = [];
      for (let i = 0; i < Math.max(movies.length, series.length); i++) {
        if (movies[i]) items.push(movies[i]);
        if (series[i]) items.push(series[i]);
      }
      return { items: items.slice(0, 40) };
    });
}

module.exports = { calmTitle, home, browse, meta, search, art };
