// Bloob's local webOS service: serves the TV UI and handles source discovery,
// AllDebrid pairing, and link resolution. Runs on the TV or locally with Node.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const http = require('http');
const https = require('https');
const fs = require('fs');
const path = require('path');
const accountStore = require('./account-store');
const { createTorrentioProvider } = require('./source-provider');

const PORT = Number(process.env.TV_APP_PORT || 8787);
const UI_DIR = path.resolve(process.env.TV_APP_UI_DIR || path.join(__dirname, 'www'));
const API = 'api.alldebrid.com';
const sourceProvider = createTorrentioProvider();
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

async function handleApi(req, res, pathname) {
  const input = req.method === 'POST' ? await readBody(req) : {};
  if (pathname === '/api/sources/search' && req.method === 'POST') {
    const type = input.type;
    const id = String(input.videoId || input.id || '');
    if (!['movie', 'series'].includes(type) || !/^tt\d+(?::\d+:\d+)?$/.test(id)) {
      return reply(res, 400, { error: 'Invalid title or episode ID' });
    }
    if (type === 'series' && !/:\d+:\d+$/.test(id)) return reply(res, 400, { error: 'Choose an episode first' });
    return reply(res, 200, { provider: sourceProvider.name, sources: await sourceProvider.search(type, id) });
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
}).catch(error => console.error('Could not load AllDebrid pairing:', error.message));

const server = http.createServer((req, res) => {
  const pathname = new URL(req.url, `http://127.0.0.1:${PORT}`).pathname;
  if (pathname.startsWith('/api/')) {
    if (!trustedRequest(req)) return reply(res, 403, { error: 'Forbidden' });
    handleApi(req, res, pathname).catch(error => reply(res, error.status || 502, { error: error.message || 'Service error' }));
  } else if (req.method === 'GET' || req.method === 'HEAD') {
    serveUi(req, res, pathname);
  } else reply(res, 405, { error: 'Method not allowed' });
});

server.listen(PORT, '127.0.0.1', async () => {
  await accountReady;
  ready = true;
  pendingStarts.splice(0).forEach(message => message.respond({ ready: true, port: PORT }));
});

try {
  process.env.NODE_PATH = (process.env.NODE_PATH || '') + ':/usr/lib/node_modules:/usr/lib/nodejs';
  require('module').Module._initPaths();
  const Service = require('webos-service');
  const service = new Service('com.muhib.tvprototype.service');
  service.activityManager.create('keepAlive', () => {});
  service.register('start', message => {
    if (ready) message.respond({ ready: true, port: PORT });
    else pendingStarts.push(message);
  });
} catch (_) {
  // Allows running the same server locally with plain Node.js.
}

module.exports = { server, trustedRequest };
