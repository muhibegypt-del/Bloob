// Bloob's local webOS service: serves the TV UI and handles source discovery,
// AllDebrid pairing, and link resolution. Runs on the TV or locally with Node.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const http = require('http');
const https = require('https');
const fs = require('fs');
const path = require('path');
const config = require('./config');
const accountStore = require('./account-store');
const { parseStreams } = require('./release');
const catalog = require('./catalog');
const images = require('./images');
const progress = require('./progress');
const { rank } = require('./rank');
const { createResolver } = require('./resolve');

const PORT = config.port;
const VERSION = require('./package.json').version;
const UI_DIR = path.resolve(process.env.TV_APP_UI_DIR || path.join(__dirname, 'www'));
const API = 'api.alldebrid.com';
const SOURCE_HOST = 'torrentio.strem.fun';
const PROVIDER = 'Torrentio';
let apiKey = process.env.ALLDEBRID_API_KEY || '';
let pendingPin = null;
let ready = false;
const pendingStarts = [];

function reply(res, status, value) {
  const body = JSON.stringify(value);
  res.writeHead(status, {
    'Content-Type': 'application/json; charset=utf-8',
    'Content-Length': Buffer.byteLength(body),
    'Cache-Control': 'no-store',
  });
  res.end(body);
}

class HttpError extends Error {
  constructor(status, message) { super(message); this.status = status; }
}

function readBody(req) {
  return new Promise((resolve, reject) => {
    let body = '';
    req.on('data', chunk => {
      body += chunk;
      if (body.length > 16384) {
        reject(new HttpError(413, 'Request is too large'));
        req.destroy();
      }
    });
    req.on('end', () => {
      try { resolve(body ? JSON.parse(body) : {}); }
      catch (_) { reject(new HttpError(400, 'Invalid JSON')); }
    });
    req.on('error', reject);
  });
}

// Only this app may call the API. A web page elsewhere on the TV could otherwise
// send "simple" cross-site POSTs (CSRF) or reach the service by DNS rebinding.
function trustedRequest(req) {
  const host = String(req.headers.host || '').replace(/:\d+$/, '');
  if (host !== '127.0.0.1' && host !== 'localhost') return false;
  if (req.method === 'POST' && !/^application\/json\b/i.test(String(req.headers['content-type'] || ''))) return false;
  return true;
}

// AllDebrid rejects revoked or invalid keys with these codes.
const AUTH_ERRORS = /^AUTH_(BAD_APIKEY|BLOCKED|USER_BANNED|MISSING_APIKEY)/;

function alldebrid(endpoint, fields, authenticated = true) {
  if (devFixtures) return devFixtures.alldebrid(endpoint, fields, authenticated);
  return new Promise((resolve, reject) => {
    if (authenticated && !apiKey) return reject(new HttpError(401, 'Connect AllDebrid first'));
    const data = fields ? new URLSearchParams(fields).toString() : '';
    const request = https.request({
      hostname: API,
      path: endpoint,
      method: fields ? 'POST' : 'GET',
      timeout: 20000,
      headers: Object.assign(
        authenticated ? { Authorization: `Bearer ${apiKey}` } : {},
        fields ? { 'Content-Type': 'application/x-www-form-urlencoded', 'Content-Length': Buffer.byteLength(data) } : {},
      ),
    }, response => {
      let text = '';
      response.on('data', chunk => { text += chunk; });
      response.on('end', () => {
        try {
          const result = JSON.parse(text);
          if (result.status !== 'success') {
            const apiError = result.error || {};
            if (authenticated && AUTH_ERRORS.test(String(apiError.code || ''))) {
              // The key was revoked: forget it so the UI asks to pair again.
              apiKey = '';
              accountStore.clear().catch(() => {});
              return reject(new HttpError(401, 'AllDebrid no longer accepts this TV. Connect it again.'));
            }
            return reject(new Error(apiError.message || apiError.code || 'AllDebrid request failed'));
          }
          resolve(result.data);
        } catch (error) { reject(new Error('AllDebrid returned an unreadable response')); }
      });
    });
    request.on('timeout', () => request.destroy(new Error('AllDebrid timed out')));
    request.on('error', reject);
    request.end(data);
  });
}

// Desktop UI tests point this at a module that fakes the indexer and AllDebrid
// (the real ones are unreachable from build machines). Never set on the TV.
const devFixtures = process.env.BLOOB_DEV_FIXTURES ? require(path.resolve(process.env.BLOOB_DEV_FIXTURES)) : null;
if (devFixtures) {
  if (!process.env.BLOOB_DEV_UNPAIRED) apiKey = 'fixture';
  console.log('Using development fixtures');
}

const resolver = createResolver((endpoint, fields) => alldebrid(endpoint, fields));

// Candidates from recent searches, so the app can ask to resolve one by ID
// without sending the magnet or link back.
const knownCandidates = new Map();
const discoveries = new Map();
const DISCOVERY_TTL = 10 * 60 * 1000;

function remember(candidates) {
  candidates.forEach(c => {
    knownCandidates.delete(c.id);
    knownCandidates.set(c.id, c);
  });
  while (knownCandidates.size > 3000) knownCandidates.delete(knownCandidates.keys().next().value);
}

async function discoverCached(type, id) {
  const key = `${type}:${id}`;
  const hit = discoveries.get(key);
  if (hit && Date.now() - hit.at < DISCOVERY_TTL) return hit.candidates;
  const candidates = devFixtures ? parseStreams(await devFixtures.streams(type, id), PROVIDER) : await discover(type, id);
  discoveries.set(key, { at: Date.now(), candidates });
  if (discoveries.size > 50) discoveries.delete(discoveries.keys().next().value);
  remember(candidates);
  return candidates;
}

function withArt(entry) {
  return Object.assign({}, entry, { tile: catalog.art.tile(entry.id), backdrop: catalog.art.backdrop(entry.id), logo: catalog.art.logo(entry.id) });
}

function isHttpUrl(value) {
  try { return ['http:', 'https:'].includes(new URL(value).protocol); }
  catch (_) { return false; }
}

const VIDEO_FILE = /\.(mp4|mkv|m4v|mov|webm|ts|m2ts|avi)$/i;

function videoFiles(nodes, result = [], folder = '') {
  for (const node of nodes || []) {
    if (node.e) videoFiles(node.e, result, folder ? `${folder}/${node.n}` : node.n);
    else if (node.l && VIDEO_FILE.test(node.n || '')) {
      result.push({ name: node.n, path: folder ? `${folder}/${node.n}` : node.n, size: node.s || 0, link: node.l });
    }
  }
  return result.sort((a, b) => b.size - a.size);
}

function discover(type, id) {
  return new Promise((resolve, reject) => {
    const request = https.get({
      hostname: SOURCE_HOST,
      path: `/stream/${type}/${encodeURIComponent(id).replace(/%3A/g, ':')}.json`,
      timeout: 15000,
      headers: { Accept: 'application/json', 'User-Agent': 'Bloob/0.2 (webOS)' },
    }, response => {
      if (response.statusCode !== 200) {
        response.resume();
        return reject(new Error(`Source indexer returned ${response.statusCode}`));
      }
      let body = '';
      response.on('data', chunk => {
        body += chunk;
        if (body.length > 5000000) request.destroy(new Error('Source list is too large'));
      });
      response.on('end', () => {
        try { resolve(parseStreams(JSON.parse(body).streams, PROVIDER)); }
        catch (_) { reject(new Error('Source indexer returned an unreadable response')); }
      });
    });
    request.on('timeout', () => request.destroy(new Error('Source search timed out')));
    request.on('error', reject);
  });
}

function accountStatus() {
  const storage = accountStore.getStatus();
  return {
    paired: !!apiKey,
    persistent: !!storage.location,
    storage: storage.location,
    durable: storage.durable,
    // Which locations were tried and why they failed; contains no key material.
    attempts: storage.attempts,
  };
}

function validVideoId(type, id) {
  if (type === 'movie') return /^tt\d+$/.test(id);
  if (type === 'series') return /^tt\d+:\d+:\d+$/.test(id);
  return false;
}

async function handleApi(req, res, url) {
  const pathname = url.pathname;
  const query = url.searchParams;
  const input = req.method === 'POST' ? await readBody(req) : {};
  if (req.method === 'GET') {
    if (pathname === '/api/health') return reply(res, 200, { ok: true, version: VERSION, app: config.appId, paired: !!apiKey });
    if (pathname === '/api/home') {
      const result = await catalog.home();
      const resume = await progress.continueWatching().catch(() => []);
      return reply(res, 200, { rows: result.rows, continue: resume.map(withArt) });
    }
    if (pathname === '/api/browse') return reply(res, 200, await catalog.browse(query.get('type')));
    if (pathname === '/api/search') return reply(res, 200, await catalog.search(query.get('q')));
    if (pathname === '/api/meta') {
      const meta = await catalog.meta(query.get('type'), query.get('id'));
      return reply(res, 200, { meta, progress: await progress.forTitle(meta.id).catch(() => ({})) });
    }
    if (pathname === '/api/image') {
      const image = await images.fetchImage(String(query.get('u') || ''));
      res.writeHead(200, { 'Content-Type': image.type, 'Content-Length': image.body.length, 'Cache-Control': 'public, max-age=86400' });
      return res.end(image.body);
    }
    if (pathname === '/api/resolve/status') {
      const job = resolver.status(query.get('id'));
      return job ? reply(res, 200, job) : reply(res, 404, { error: 'This request has expired. Choose the source again.' });
    }
    if (pathname === '/api/progress') return reply(res, 200, { items: (await progress.continueWatching()).map(withArt) });
    if (pathname === '/api/settings') return reply(res, 200, await progress.getSettings());
  }
  if (pathname === '/api/settings' && req.method === 'POST') return reply(res, 200, await progress.saveSettings(input));
  if (pathname === '/api/sources/ranked' && req.method === 'POST') {
    const type = input.type;
    const id = String(input.videoId || '');
    if (!validVideoId(type, id)) return reply(res, 400, { error: 'Invalid title or episode ID' });
    const candidates = await discoverCached(type, id);
    return reply(res, 200, Object.assign({ provider: PROVIDER }, rank(candidates, { language: input.language || 'English' })));
  }
  if (pathname === '/api/resolve/start' && req.method === 'POST') {
    let source = null;
    if (input.candidate) {
      const candidate = knownCandidates.get(String(input.candidate));
      if (!candidate) return reply(res, 404, { error: 'This source list has expired. Search again.' });
      source = { kind: candidate.kind, value: candidate.value, fileName: candidate.fileName };
    } else if (isHttpUrl(input.link)) {
      source = { kind: 'link', value: String(input.link) };
    } else if (/^magnet:\?xt=urn:btih:/i.test(String(input.magnet || '')) || /^[a-f0-9]{40}$/i.test(String(input.magnet || ''))) {
      source = { kind: 'magnet', value: String(input.magnet).trim() };
    } else {
      return reply(res, 400, { error: 'Choose a source, or enter a link or magnet' });
    }
    if (!apiKey) return reply(res, 401, { error: 'Connect AllDebrid first' });
    const hint = { fileName: source.fileName };
    if (Number.isInteger(input.season) && Number.isInteger(input.episode)) { hint.season = input.season; hint.episode = input.episode; }
    return reply(res, 200, resolver.start(source, hint));
  }
  if (pathname === '/api/resolve/choose' && req.method === 'POST') {
    const job = resolver.choose(input.id, input);
    return job ? reply(res, 200, job) : reply(res, 404, { error: 'This request has expired. Choose the source again.' });
  }
  if (pathname === '/api/resolve/cancel' && req.method === 'POST') {
    return reply(res, 200, { cancelled: resolver.cancel(input.id) });
  }
  if (pathname === '/api/progress' && req.method === 'POST') {
    return reply(res, 200, await progress.save(input));
  }
  if (pathname === '/api/sources/search' && req.method === 'POST') {
    const type = input.type;
    const id = String(input.videoId || input.id || '');
    if (!['movie', 'series'].includes(type) || !/^tt\d+(?::\d+:\d+)?$/.test(id)) {
      return reply(res, 400, { error: 'Invalid title or episode ID' });
    }
    if (type === 'series' && !/:\d+:\d+$/.test(id)) return reply(res, 400, { error: 'Choose an episode first' });
    return reply(res, 200, { provider: PROVIDER, sources: await discover(type, id) });
  }
  if (pathname === '/api/account' && req.method === 'GET') {
    return reply(res, 200, accountStatus());
  }
  if (pathname === '/api/pin/start' && req.method === 'POST') {
    const data = await alldebrid('/v4.1/pin/get', null, false);
    pendingPin = { pin: data.pin, check: data.check };
    return reply(res, 200, { pin: data.pin, url: data.user_url, expiresIn: data.expires_in });
  }
  if (pathname === '/api/pin/check' && req.method === 'POST') {
    if (!pendingPin) return reply(res, 400, { error: 'Start pairing first' });
    const data = await alldebrid('/v4/pin/check', pendingPin, false);
    if (data.activated && data.apikey) {
      apiKey = data.apikey;
      pendingPin = null;
      const saved = await accountStore.save(apiKey);
      if (!saved.location) console.error('Could not save AllDebrid pairing:', JSON.stringify(saved.attempts));
    }
    return reply(res, 200, Object.assign(accountStatus(), { expiresIn: data.expires_in }));
  }
  if (pathname === '/api/link/unlock' && req.method === 'POST') {
    if (!isHttpUrl(input.link)) return reply(res, 400, { error: 'Enter an HTTP or HTTPS link' });
    const data = await alldebrid('/v4/link/unlock', { link: input.link });
    return reply(res, 200, {
      state: data.delayed ? 'waiting' : data.streams && data.streams.length ? 'qualities' : 'ready',
      url: data.link || null,
      delayed: data.delayed || null,
      id: data.id || null,
      streams: data.streams || [],
      filename: data.filename || '',
      filesize: data.filesize || 0,
    });
  }
  if (pathname === '/api/link/quality' && req.method === 'POST') {
    if (!input.id || !input.stream) return reply(res, 400, { error: 'Choose a quality' });
    const data = await alldebrid('/v4/link/streaming', { id: String(input.id), stream: String(input.stream) });
    return reply(res, 200, { state: data.delayed ? 'waiting' : 'ready', url: data.link || null, delayed: data.delayed || null, filename: data.filename || '' });
  }
  if (pathname === '/api/link/delayed' && req.method === 'POST') {
    if (!input.id) return reply(res, 400, { error: 'Missing delayed link ID' });
    const data = await alldebrid('/v4/link/delayed', { id: String(input.id) });
    return reply(res, 200, { state: data.status === 2 ? 'ready' : data.status === 3 ? 'error' : 'waiting', url: data.link || null, timeLeft: data.time_left });
  }
  if (pathname === '/api/magnet/add' && req.method === 'POST') {
    const value = String(input.magnet || '').trim();
    if (!/^magnet:\?xt=urn:btih:/i.test(value) && !/^[a-f0-9]{40}$/i.test(value)) {
      return reply(res, 400, { error: 'Enter a magnet link or 40-character info hash' });
    }
    const data = await alldebrid('/v4/magnet/upload', { 'magnets[]': value });
    const magnet = data.magnets && data.magnets[0];
    if (!magnet) throw new Error('AllDebrid did not accept the magnet');
    if (magnet.error) throw new Error(magnet.error.message || magnet.error.code);
    return reply(res, 200, { id: magnet.id, ready: !!magnet.ready, name: magnet.name });
  }
  if (pathname === '/api/magnet/status' && req.method === 'POST') {
    if (!/^\d+$/.test(String(input.id || ''))) return reply(res, 400, { error: 'Missing magnet ID' });
    const data = await alldebrid('/v4.1/magnet/status', { id: String(input.id) });
    const magnet = data.magnets && (Array.isArray(data.magnets) ? data.magnets[0] : data.magnets);
    if (!magnet) return reply(res, 404, { error: 'Magnet not found' });
    return reply(res, 200, {
      ready: magnet.statusCode === 4,
      failed: magnet.statusCode >= 5,
      status: magnet.status,
      downloaded: magnet.downloaded || 0,
      size: magnet.size || 0,
      speed: magnet.downloadSpeed || 0,
    });
  }
  if (pathname === '/api/magnet/files' && req.method === 'POST') {
    if (!/^\d+$/.test(String(input.id || ''))) return reply(res, 400, { error: 'Missing magnet ID' });
    const data = await alldebrid('/v4/magnet/files', { 'id[]': String(input.id) });
    const magnet = data.magnets && data.magnets[0];
    return reply(res, 200, { files: videoFiles(magnet && magnet.files) });
  }
  reply(res, 404, { error: 'Unknown API route' });
}

const types = { '.html': 'text/html', '.js': 'application/javascript', '.css': 'text/css', '.png': 'image/png', '.jpg': 'image/jpeg', '.svg': 'image/svg+xml', '.woff2': 'font/woff2', '.ico': 'image/x-icon', '.mp4': 'video/mp4' };
function serveUi(req, res, pathname) {
  const file = pathname === '/' ? path.join(UI_DIR, 'index.html') : path.resolve(UI_DIR, '.' + pathname);
  if (!file.startsWith(UI_DIR + path.sep) && file !== path.join(UI_DIR, 'index.html')) return reply(res, 403, { error: 'Forbidden' });
  fs.stat(file, (error, stat) => {
    if (error && pathname.startsWith('/media/')) return reply(res, 404, { error: 'Media file not found' });
    const target = error || !stat.isFile() ? path.join(UI_DIR, 'index.html') : file;
    if (path.extname(target) === '.mp4') {
      const range = /^bytes=(\d*)-(\d*)$/.exec(req.headers.range || '');
      let start = 0;
      let end = stat.size - 1;
      if (range && range[1]) {
        start = Number(range[1]);
        if (range[2]) end = Math.min(Number(range[2]), stat.size - 1);
      } else if (range && range[2]) {
        start = Math.max(0, stat.size - Number(range[2])); // suffix range: last N bytes
      }
      if (start > end || start >= stat.size) {
        res.writeHead(416, { 'Content-Range': `bytes */${stat.size}` });
        return res.end();
      }
      res.writeHead(range ? 206 : 200, {
        'Content-Type': 'video/mp4', 'Accept-Ranges': 'bytes',
        'Content-Length': end - start + 1,
        ...(range ? { 'Content-Range': `bytes ${start}-${end}/${stat.size}` } : {}),
      });
      if (req.method === 'HEAD') return res.end();
      return fs.createReadStream(target, { start, end }).pipe(res);
    }
    fs.readFile(target, (readError, body) => {
      if (readError) return reply(res, 404, { error: 'Build the TV UI first' });
      res.writeHead(200, { 'Content-Type': types[path.extname(target)] || 'application/octet-stream', 'Cache-Control': target.endsWith('index.html') ? 'no-cache' : 'public, max-age=86400' });
      res.end(body);
    });
  });
}

const accountReady = accountStore.load().then(saved => {
  if (saved && !apiKey) apiKey = saved;
  // Found another Bloob app's pairing: keep our own copy too.
  if (saved && String(accountStore.getStatus().location || '').indexOf('shared:') === 0) return accountStore.save(saved);
}).catch(error => console.error('Could not load AllDebrid pairing:', error.message));

const server = http.createServer((req, res) => {
  const url = new URL(req.url, `http://127.0.0.1:${PORT}`);
  const pathname = url.pathname;
  if (pathname.startsWith('/api/')) {
    if (!trustedRequest(req)) return reply(res, 403, { error: 'Forbidden' });
    handleApi(req, res, url).catch(error => reply(res, error.status || 502, { error: error.message || 'Service error' }));
  } else if (req.method === 'GET' || req.method === 'HEAD') {
    serveUi(req, res, pathname);
  } else reply(res, 405, { error: 'Method not allowed' });
});

server.listen(PORT, '127.0.0.1', async () => {
  await accountReady;
  ready = true;
  pendingStarts.splice(0).forEach(message => message.respond({ ready: true, port: PORT, version: VERSION }));
});

try {
  process.env.NODE_PATH = (process.env.NODE_PATH || '') + ':/usr/lib/node_modules:/usr/lib/nodejs';
  require('module').Module._initPaths();
  const Service = require('webos-service');
  const service = new Service(config.serviceId);
  service.activityManager.create('keepAlive', () => {});
  service.register('start', message => {
    if (ready) message.respond({ ready: true, port: PORT, version: VERSION });
    else pendingStarts.push(message);
  });
  // The native app stops its service when it exits, so nothing idles in the background.
  service.register('stop', message => {
    message.respond({ stopping: true });
    setTimeout(() => process.exit(0), 300);
  });
} catch (_) {
  // Allows running the same server locally with plain Node.js.
}

module.exports = { server, trustedRequest };
