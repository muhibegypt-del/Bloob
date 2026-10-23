// Scores for title pages (2.7.1): Rotten Tomatoes, Metacritic, IMDb and
// Letterboxd, from MDBList with the owner's key (keys.js). Cached for a day;
// MDBList's free tier allows 1000 requests a day. No key: no scores.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const { getJson } = require('./net');
const keys = require('./keys');

const DAY_MS = 24 * 60 * 60 * 1000;
const cache = new Map();

// MDBList ratings → short labels, in the order they are shown.
function scores(data) {
  const by = {};
  (data && data.ratings || []).forEach(r => { if (r && r.source) by[r.source] = r; });
  const out = [];
  const has = r => r && r.value !== null && r.value !== undefined && r.value !== '';
  if (has(by.tomatoes)) out.push(`RT ${Math.round(Number(by.tomatoes.value))}%`);
  if (has(by.metacritic)) out.push(`Metacritic ${Math.round(Number(by.metacritic.value))}`);
  if (has(by.imdb)) out.push(`IMDb ${Number(by.imdb.value).toFixed(1)}`);
  if (has(by.letterboxd)) out.push(`Letterboxd ${Number(by.letterboxd.value).toFixed(1)}`);
  return out;
}

async function forTitle(type, imdbId) {
  if (!/^tt\d+$/.test(String(imdbId))) return [];
  const key = await keys.get('mdblist');
  if (!key) return [];
  const k = `${type}:${imdbId}`;
  const hit = cache.get(k);
  if (hit && hit.expires > Date.now()) return hit.value;
  const kind = type === 'series' ? 'show' : 'movie';
  const value = getJson(`https://api.mdblist.com/imdb/${kind}/${imdbId}?apikey=${encodeURIComponent(key)}`, { timeout: 5000 }).then(scores);
  cache.set(k, { value, expires: Date.now() + DAY_MS });
  value.catch(() => cache.delete(k));
  if (cache.size > 500) cache.delete(cache.keys().next().value);
  return value;
}

module.exports = { forTitle, scores };
