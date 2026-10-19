// Private indexers through Torznab (2.6.0): the owner's tracker, served by
// their own Prowlarr (or Jackett) on the home network. Searched by IMDb ID
// alongside the public indexes; results are private torrents, which only work
// through the tracker with the owner's passkey, so they are resolved by
// handing AllDebrid the .torrent file (resolve.js), never a bare magnet.
//
// The feed URL and API key live in the service's own store (indexers.json),
// set from a computer with native/scripts/add-indexer.sh. They are never sent
// to the app, logged, or written to Git; download links carry the key, so they
// stay in the service too.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const { get } = require('./net');

const TIMEOUT_MS = 6000;

const ENTITIES = { amp: '&', lt: '<', gt: '>', quot: '"', apos: "'" };
function decode(text) {
  return String(text || '').replace(/&(#x?[0-9a-f]+|[a-z]+);/gi, (m, e) => {
    if (e[0] === '#') {
      const n = e[1] === 'x' || e[1] === 'X' ? parseInt(e.slice(2), 16) : parseInt(e.slice(1), 10);
      return Number.isFinite(n) ? String.fromCharCode(n) : m;
    }
    return Object.prototype.hasOwnProperty.call(ENTITIES, e) ? ENTITIES[e] : m;
  });
}

function tag(xml, name) {
  const m = new RegExp(`<${name}(?:\\s[^>]*)?>([\\s\\S]*?)</${name}>`, 'i').exec(xml);
  if (!m) return '';
  const cdata = /^\s*<!\[CDATA\[([\s\S]*?)\]\]>\s*$/.exec(m[1]);
  return decode(cdata ? cdata[1] : m[1]).trim();
}

function attribute(element, name) {
  const m = new RegExp(`\\s${name}\\s*=\\s*"([^"]*)"`, 'i').exec(element);
  return m ? decode(m[1]) : '';
}

/** Torznab XML → [{ title, link, size, seeders, infohash, magnet, imdb }]. */
function parseFeed(xml) {
  const text = String(xml || '');
  const error = /<error\s[^>]*>/i.exec(text);
  if (error && !/<item[\s>]/i.test(text)) {
    throw new Error(attribute(error[0], 'description') || `indexer error ${attribute(error[0], 'code')}`);
  }
  const items = [];
  const pattern = /<item[\s>][\s\S]*?<\/item>/gi;
  let m;
  while ((m = pattern.exec(text))) {
    const item = m[0];
    const attrs = {};
    const attrPattern = /<(?:torznab|newznab):attr\s[^>]*>/gi;
    let a;
    while ((a = attrPattern.exec(item))) attrs[attribute(a[0], 'name').toLowerCase()] = attribute(a[0], 'value');
    const enclosure = /<enclosure\s[^>]*>/i.exec(item);
    const link = (enclosure && attribute(enclosure[0], 'url')) || tag(item, 'link');
    const size = Number(tag(item, 'size')) || Number(attrs.size) || (enclosure ? Number(attribute(enclosure[0], 'length')) : 0) || 0;
    items.push({
      title: tag(item, 'title'),
      link,
      size,
      seeders: attrs.seeders !== undefined && attrs.seeders !== '' ? Number(attrs.seeders) : undefined,
      infohash: String(attrs.infohash || '').toLowerCase(),
      magnet: attrs.magneturl || (/^magnet:/i.test(link) ? link : ''),
      imdb: attrs.imdb || attrs.imdbid || '',
    });
  }
  return items;
}

/** The search URL for a title or episode; the API key goes in as `apikey`. */
function searchUrl(feed, type, id) {
  const [imdb, season, episode] = String(id).split(':');
  const u = new URL(feed.url);
  u.searchParams.set('apikey', feed.apikey || '');
  // Torznab's imdbid is the number without "tt"; Prowlarr maps it per tracker.
  u.searchParams.set('imdbid', imdb.replace(/^tt/, ''));
  if (type === 'series') {
    u.searchParams.set('t', 'tvsearch');
    u.searchParams.set('season', season);
    u.searchParams.set('ep', episode);
  } else {
    u.searchParams.set('t', 'movie');
  }
  return u.toString();
}

/** A provider for sources.js from one stored feed. */
function provider(feed, index) {
  return {
    id: `private${index}`,
    label: feed.name || 'Private tracker',
    private: true,
    search: async (type, id) => {
      const result = await get(searchUrl(feed, type, id), { timeout: TIMEOUT_MS, maxBytes: 4000000, accept: 'application/rss+xml, application/xml, text/xml' });
      const items = parseFeed(result.body.toString('utf8'));
      return {
        torrents: items.filter(i => i.title && (i.link || i.magnet)).map(i => ({
          hash: i.infohash,
          name: i.title,
          size: i.size,
          seeders: i.seeders,
          // Private: AllDebrid gets the .torrent (with the passkey), not a magnet.
          download: i.link && !/^magnet:/i.test(i.link) ? i.link : '',
          magnet: i.magnet,
        })),
      };
    },
  };
}

/** A feed as the app and logs may see it: name and host only, never the key or path. */
function publicFeed(feed) {
  let host = '';
  try { host = new URL(feed.url).host; } catch (_) { host = ''; }
  return { name: feed.name || 'Private tracker', host };
}

/** Validate a feed someone is saving. Returns an error message or ''. */
function invalidFeed(feed) {
  if (!feed || typeof feed !== 'object') return 'Expected { name, url, apikey }';
  try {
    const u = new URL(String(feed.url || ''));
    if (u.protocol !== 'http:' && u.protocol !== 'https:') return 'The feed URL must be http or https';
  } catch (_) { return 'The feed URL is not a URL'; }
  if (!String(feed.apikey || '').trim()) return 'The API key is missing';
  if (String(feed.name || '').length > 40) return 'The name is too long';
  return '';
}

module.exports = { parseFeed, searchUrl, provider, publicFeed, invalidFeed };
