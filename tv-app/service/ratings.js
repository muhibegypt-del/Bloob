// Scores for title pages (2.7.1): Rotten Tomatoes, Metacritic, IMDb and
// Letterboxd, from MDBList with the owner's key (keys.js). Cached for a day;
// MDBList's free tier allows 1000 requests a day. No key: no scores.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const { getJson } = require('./net');
const keys = require('./keys');

const DAY_MS = 24 * 60 * 60 * 1000;
const cache = new Map();

// MDBList ratings → what the title page shows, in order: the site's logo (an
// SVG from MDBList, served through /api/image), the score, and a spoken label.
// Rotten Tomatoes shows the tomato from 60%, the splat below, as RT does.
const LOGO = name => `https://mdblist.com/static/ratings/${name}.svg`;

function scores(data) {
  const by = {};
  (data && data.ratings || []).forEach(r => { if (r && r.source) by[r.source] = r; });
  const has = r => r && r.value !== null && r.value !== undefined && r.value !== '' && Number.isFinite(Number(r.value));
  const out = [];
  if (has(by.tomatoes)) {
    const v = Math.round(Number(by.tomatoes.value));
    out.push({ text: `${v}%`, label: `Rotten Tomatoes ${v}%`, logo: LOGO(v >= 60 ? 'rotten-tomatoes' : 'rotten') });
  }
  if (has(by.metacritic)) {
    const v = Math.round(Number(by.metacritic.value));
    out.push({ text: String(v), label: `Metacritic ${v}`, logo: LOGO('metacritic') });
  }
  if (has(by.imdb)) {
    const v = Number(by.imdb.value).toFixed(1);
    out.push({ text: v, label: `IMDb ${v}`, logo: LOGO('imdb') });
  }
  if (has(by.letterboxd)) {
    const v = Number(by.letterboxd.value).toFixed(1);
    out.push({ text: v, label: `Letterboxd ${v}`, logo: LOGO('letterboxd') });
  }
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
