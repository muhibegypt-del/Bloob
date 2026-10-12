// Standalone webOS service: serves the TV UI and handles AllDebrid pairing/resolution.
// No Stremio service or torrent engine is required for this prototype.
const http = require('http');
const https = require('https');
const fs = require('fs');
const path = require('path');
const accountStore = require('./account-store');

const PORT = Number(process.env.TV_APP_PORT || 8787);
const UI_DIR = path.join(__dirname, 'www');
const API = 'api.alldebrid.com';
const SOURCE_HOST = 'torrentio.strem.fun';
let apiKey = process.env.ALLDEBRID_API_KEY || '';
let accountSaved = false;
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

function readBody(req) {
  return new Promise((resolve, reject) => {
    let body = '';
    req.on('data', chunk => {
      body += chunk;
      if (body.length > 16384) {
        reject(new Error('Request is too large'));
        req.destroy();
      }
    });
    req.on('end', () => {
      try { resolve(body ? JSON.parse(body) : {}); }
      catch (_) { reject(new Error('Invalid JSON')); }
    });
    req.on('error', reject);
  });
}

function alldebrid(endpoint, fields, authenticated = true) {
  return new Promise((resolve, reject) => {
    if (authenticated && !apiKey) return reject(new Error('Pair AllDebrid first'));
    const data = fields ? new URLSearchParams(fields).toString() : '';
    const request = https.request({
      hostname: API,
      path: endpoint,
      method: fields ? 'POST' : 'GET',
      timeout: 20000,
      headers: {
        ...(authenticated ? { Authorization: `Bearer ${apiKey}` } : {}),
        ...(fields ? {
          'Content-Type': 'application/x-www-form-urlencoded',
          'Content-Length': Buffer.byteLength(data),
        } : {}),
      },
    }, response => {
      let text = '';
      response.on('data', chunk => { text += chunk; });
      response.on('end', () => {
        try {
          const result = JSON.parse(text);
          if (result.status !== 'success') {
            const apiError = result.error || {};
            return reject(new Error(apiError.message || apiError.code || 'AllDebrid request failed'));
          }
          resolve(result.data);
        } catch (error) { reject(error); }
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

function videoFiles(nodes, result = []) {
  for (const node of nodes || []) {
    if (node.e) videoFiles(node.e, result);
    else if (node.l && /\.(mp4|mkv|m4v|mov|webm|ts|avi)$/i.test(node.n || '')) {
      result.push({ name: node.n, size: node.s || 0, link: node.l });
    }
  }
  return result.sort((a, b) => b.size - a.size);
}

function discover(type, id) {
  return new Promise((resolve, reject) => {
    const request = https.get({
      hostname: SOURCE_HOST,
      path: `/stream/${type}/${id}.json`,
      timeout: 15000,
      headers: { Accept: 'application/json' },
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
        try {
          const seen = new Set();
          const streams = JSON.parse(body).streams || [];
          const sources = streams.flatMap(stream => {
            const hash = String(stream.infoHash || '').toLowerCase();
            const url = String(stream.url || '');
            const kind = /^[a-f0-9]{40}$/.test(hash) ? 'magnet' : isHttpUrl(url) ? 'link' : '';
            const value = kind === 'magnet' ? hash : url;
            if (!kind || seen.has(value)) return [];
            seen.add(value);
            const raw = String(stream.title || stream.name || 'Source');
            const release = raw.split('\n')[0].replace(/\s+/g, ' ').trim().slice(0, 120);
            const quality = /\b(2160p|4k|1080p|720p|480p)\b/i.exec(raw + ' ' + (stream.name || ''));
            const size = /💾\s*([\d.]+\s*[TGMK]B)/i.exec(raw);
            const peers = /👤\s*([\d,]+)/.exec(raw);
            const codec = /\b(x265|hevc|h\.?265|x264|h\.?264|av1)\b/i.exec(raw);
            const qualityLabel = quality ? (/^(2160p|4k)$/i.test(quality[1]) ? '4K' : quality[1].toLowerCase()) : '';
            return [{
              kind, value, release: release || 'Unnamed release',
              quality: qualityLabel,
              size: size ? size[1].replace(/\s+/g, ' ') : '',
              peers: peers ? Number(peers[1].replace(/,/g, '')) : null,
              codec: codec ? codec[1].toUpperCase() : '',
              provider: 'Torrentio',
            }];
          });
          const rank = { '4K': 0, '1080p': 1, '720p': 2, '480p': 3 };
          sources.sort((a, b) => {
            const qualityOrder = (rank[a.quality] === undefined ? 4 : rank[a.quality]) - (rank[b.quality] === undefined ? 4 : rank[b.quality]);
            return qualityOrder || (b.peers || 0) - (a.peers || 0);
          });
          const counts = {};
          resolve(sources.filter(source => {
            const group = source.quality || 'OTHER';
            counts[group] = (counts[group] || 0) + 1;
            return counts[group] <= 4;
          }).slice(0, 12));
        } catch (error) { reject(error); }
      });
    });
    request.on('timeout', () => request.destroy(new Error('Source search timed out')));
    request.on('error', reject);
  });
}

async function handleApi(req, res, pathname) {
  const input = req.method === 'POST' ? await readBody(req) : {};
  if (pathname === '/api/sources/search' && req.method === 'POST') {
    const type = input.type;
    const id = String(input.videoId || input.id || '');
    if (!['movie', 'series'].includes(type) || !/^tt\d+(?::\d+:\d+)?$/.test(id)) {
      return reply(res, 400, { error: 'Invalid title or episode ID' });
    }
    return reply(res, 200, { provider: 'Torrentio', sources: await discover(type, id) });
  }
  if (pathname === '/api/account' && req.method === 'GET') {
    return reply(res, 200, { paired: !!apiKey, persistent: accountSaved });
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
      try {
        await accountStore.save(apiKey);
        accountSaved = true;
      } catch (error) {
        accountSaved = false;
        console.error('Could not save AllDebrid pairing:', error.message);
      }
    }
    return reply(res, 200, { paired: !!apiKey, persistent: accountSaved, expiresIn: data.expires_in });
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
    });
  }
  if (pathname === '/api/link/quality' && req.method === 'POST') {
    if (!input.id || !input.stream) return reply(res, 400, { error: 'Choose a quality' });
    const data = await alldebrid('/v4/link/streaming', { id: String(input.id), stream: String(input.stream) });
    return reply(res, 200, { state: data.delayed ? 'waiting' : 'ready', url: data.link || null, delayed: data.delayed || null });
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
    if (magnet && magnet.error) throw new Error(magnet.error.message || magnet.error.code);
    return reply(res, 200, { id: magnet.id, ready: !!magnet.ready, name: magnet.name });
  }
  if (pathname === '/api/magnet/status' && req.method === 'POST') {
    if (!/^\d+$/.test(String(input.id || ''))) return reply(res, 400, { error: 'Missing magnet ID' });
    const data = await alldebrid('/v4.1/magnet/status', { id: String(input.id) });
    const magnet = data.magnets && data.magnets[0];
    if (!magnet) return reply(res, 404, { error: 'Magnet not found' });
    return reply(res, 200, { ready: magnet.statusCode === 4, status: magnet.status, downloaded: magnet.downloaded || 0, size: magnet.size || 0 });
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
      const range = /^bytes=(\d+)-(\d*)$/.exec(req.headers.range || '');
      const start = range ? Number(range[1]) : 0;
      const end = range && range[2] ? Math.min(Number(range[2]), stat.size - 1) : stat.size - 1;
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
  if (saved) {
    if (!apiKey) apiKey = saved;
    accountSaved = true;
  }
}).catch(error => console.error('Could not load AllDebrid pairing:', error.message));

http.createServer((req, res) => {
  const pathname = new URL(req.url, `http://127.0.0.1:${PORT}`).pathname;
  if (pathname.startsWith('/api/')) {
    handleApi(req, res, pathname).catch(error => reply(res, 502, { error: error.message || 'Service error' }));
  } else if (req.method === 'GET' || req.method === 'HEAD') {
    serveUi(req, res, pathname);
  } else reply(res, 405, { error: 'Method not allowed' });
}).listen(PORT, '127.0.0.1', async () => {
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
