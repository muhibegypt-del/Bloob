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
const ratings = require('./ratings');
const images = require('./images');
const progress = require('./progress');
const next = require('./next');
const { rank, editionOf, hashOf } = require('./rank');
const { createResolver } = require('./resolve');
const { createReadyCheck } = require('./ready');
const bridge = require('./bridge');
const sources = require('./sources');

const PORT = config.port;
const VERSION = require('./package.json').version;
const UI_DIR = path.resolve(process.env.TV_APP_UI_DIR || path.join(__dirname, 'www'));
const API = 'api.alldebrid.com';
const SOURCE_HOST = 'torrentio.strem.fun';
const PROVIDER = 'Torrentio';  // the provider fixtures stand in for
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
const readyCheck = createReadyCheck((endpoint, fields) => alldebrid(endpoint, fields));

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

/** What progress keeps about the version played: quality and identity, never a URL. */
function editionFor(candidateId) {
  const id = String(candidateId || '');
  if (!id) return null;
  const known = knownCandidates.get(id);
  if (known) return editionOf(known);
  const hash = hashOf(id);
  return hash ? { id: id.slice(0, 60), hash } : null;
}

/** An edition as the app sees it: quality words only, no hash or ID. */
function publicEdition(edition) {
  return {
    from: edition.from || '',
    resolution: edition.resolution || '',
    hdr: edition.hdr || '',
    atmos: !!edition.atmos,
    pack: !!edition.pack,
  };
}

/** Which providers the viewer chose in Settings: every index, or Torrentio alone. */
async function providerChoice() {
  const settings = await progress.getSettings().catch(() => ({}));
  return settings.sources === 'torrentio' ? ['torrentio'] : null;
}

async function discoverCached(type, id) {
  const only = await providerChoice();
  const key = `${type}:${id}:${only ? only.join(',') : 'all'}`;
  const hit = discoveries.get(key);
  if (hit && Date.now() - hit.at < DISCOVERY_TTL) return hit;
  let found;
  if (devFixtures) {
    found = { candidates: parseStreams(await devFixtures.streams(type, id), 'torrentio'), report: [{ id: 'torrentio', label: PROVIDER, count: 0, ms: 0, error: '' }] };
    found.report[0].count = found.candidates.length;
  } else {
    // Late answers (after the reply went back) join the cached list for the next look.
    found = await sources.discoverAll(type, id, {
      only,
      onLate: (candidates, report) => {
        const entry = discoveries.get(key);
        if (!entry) return;
        entry.candidates = candidates;
        entry.report = report.map(r => Object.assign({}, r));
        remember(candidates);
      },
    });
  }
  const entry = { at: Date.now(), candidates: found.candidates, report: found.report };
  if (!found.candidates.length && found.report.every(r => r.error)) {
    // Every index failed: say why, and don't cache the failure.
    throw new Error(`No source index answered (${found.report.map(r => `${r.label}: ${r.error}`).join('; ')})`);
  }
  discoveries.set(key, entry);
  if (discoveries.size > 50) discoveries.delete(discoveries.keys().next().value);
  remember(found.candidates);
  return entry;
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
    if (pathname === '/api/health') {
      const mem = process.memoryUsage();
      return reply(res, 200, {
        ok: true, version: VERSION, app: config.appId, paired: !!apiKey,
        memory: { rss: mem.rss, heapUsed: mem.heapUsed, external: mem.external },
        imageCacheBytes: images.cacheBytes(),
      });
    }
    if (pathname === '/api/home') {
      const result = await catalog.home();
      const resume = await progress.continueWatching().catch(() => []);
      return reply(res, 200, { rows: result.rows, continue: resume.map(withArt) });
    }
    if (pathname === '/api/browse') return reply(res, 200, await catalog.browse(query.get('type')));
    if (pathname === '/api/search') return reply(res, 200, await catalog.search(query.get('q')));
    if (pathname === '/api/meta') {
      const meta = await catalog.meta(query.get('type'), query.get('id'));
      // Scores (MDBList): never hold the page up for more than 2.5 s.
      meta.scores = await Promise.race([ratings.forTitle(meta.type, meta.id).catch(() => []),
        new Promise(resolve => setTimeout(() => resolve([]), 2500))]);
      const watched = await progress.forTitle(meta.id).catch(() => ({}));
      const intent = meta.type === 'series' ? await progress.intentFor(meta.id).catch(() => null) : null;
      return reply(res, 200, {
        meta,
        progress: watched,
        target: next.target(meta, watched),
        intent: intent ? publicEdition(intent) : null,
        dismissed: await progress.isDismissed(meta.id).catch(() => false),
      });
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
  if (pathname === '/api/memory' && req.method === 'POST') {
    const budget = images.setMemoryPolicy(String(input.policy || ''));
    return budget ? reply(res, 200, { policy: String(input.policy), imageCacheBudget: budget, imageCacheBytes: images.cacheBytes() })
      : reply(res, 400, { error: 'Unknown memory policy' });
  }
  if (pathname === '/api/bridge/open' && req.method === 'POST') {
    // After resolving a version whose sound may be DTS only: does it need the bridge?
    if (!isHttpUrl(input.url)) return reply(res, 400, { error: 'Expected the resolved link' });
    return reply(res, 200, await bridge.open(PORT, String(input.url), String(input.language || 'English')));
  }
  if (pathname === '/api/bridge/source' && req.method === 'POST') {
    // Bloob's engine (3.0) reads the file itself, over plain HTTP from here:
    // the resolved link stays in this process's memory.
    if (!/^https?:\/\//.test(String(input.url || ''))) return reply(res, 400, { error: 'bad url' });
    return reply(res, 200, { src: bridge.source(PORT, String(input.url)) });
  }
  if (pathname === '/api/bridge/status' && req.method === 'GET') return reply(res, 200, bridge.status(String(query.get('token') || '')) || {});
  if (pathname === '/api/sources/ranked' && req.method === 'POST') {
    const type = input.type;
    const id = String(input.videoId || '');
    if (!validVideoId(type, id)) return reply(res, 400, { error: 'Invalid title or episode ID' });
    const found = await discoverCached(type, id);
    const candidates = found.candidates;
    const settings = await progress.getSettings().catch(() => ({}));
    // Runtime for the bitrate estimate: the app's, else the cached catalogue's,
    // else the ranker's typical film or episode length.
    const sent = Number(input.runtimeMinutes);
    const runtimeMinutes = sent > 0 && sent < 1000 ? sent : await catalog.cachedRuntime(type, id.split(':')[0]);
    // A show keeps to the version the viewer chose or last played (UX-PLAN X3).
    const intent = type === 'series' ? await progress.intentFor(id.split(':')[0]).catch(() => null) : null;
    const prefs = { language: input.language || 'English', type, runtimeMinutes: runtimeMinutes || undefined, intent: intent || undefined, picture: settings.picture };
    const answered = found.report.filter(r => !r.error && r.count > 0).map(r => r.label);
    // Ready now (2.8): ask the owner's AllDebrid about the top versions, for at most 3 s.
    let ranked = rank(candidates, prefs);
    if (apiKey && !devFixtures) {
      const top = [ranked.recommended].concat(...ranked.groups.filter(g => g.kind !== 'problems').map(g => g.items))
        .filter(Boolean).map(v => hashOf(v.id)).filter(Boolean).slice(0, 8);
      ranked = rank(await readyCheck.annotate(candidates, top, 3000), prefs);
    }
    return reply(res, 200, Object.assign({
      provider: answered.length ? answered.join(', ') : PROVIDER,
      providers: found.report,
      picturePreference: settings.picture || 'best',
      runtimeMinutes: runtimeMinutes || null,
      runtimeFrom: sent > 0 && sent < 1000 ? 'app' : runtimeMinutes ? 'catalogue' : 'typical',
    }, ranked));
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
    return reply(res, 200, await progress.save(input, editionFor(input.edition)));
  }
  if (pathname === '/api/progress/dismiss' && req.method === 'POST') {
    return reply(res, 200, await progress.dismiss(input.id));
  }
  if (pathname === '/api/intent' && req.method === 'POST') {
    // The viewer chose a version for a show; null forgets the choice.
    const edition = input.candidate === null ? null : editionFor(input.candidate);
    if (input.candidate !== null && !edition) return reply(res, 404, { error: 'This source list has expired. Search again.' });
    const result = await progress.setIntent(String(input.id || ''), edition);
    return reply(res, 200, { intent: result.intent ? publicEdition(result.intent) : null });
  }
  if (pathname === '/api/sources/search' && req.method === 'POST') {
    const type = input.type;
    const id = String(input.videoId || input.id || '');
    if (!['movie', 'series'].includes(type) || !/^tt\d+(?::\d+:\d+)?$/.test(id)) {
      return reply(res, 400, { error: 'Invalid title or episode ID' });
    }
    if (type === 'series' && !/:\d+:\d+$/.test(id)) return reply(res, 400, { error: 'Choose an episode first' });
    const found = await discoverCached(type, id);
    return reply(res, 200, { provider: PROVIDER, providers: found.report, sources: found.candidates });
  }
  if (pathname === '/api/account' && req.method === 'GET') {
    return reply(res, 200, accountStatus());
  }
  if (pathname === '/api/account/unpair' && req.method === 'POST') {
    // Forget the pairing: memory first, then overwrite every saved copy (never deletes).
    apiKey = '';
    pendingPin = null;
    await accountStore.forget();
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
    // The audio bridge's streams (3.0): ffmpeg reads the file here, the player the converted stream.
    const src = /^\/api\/bridge\/src\/([a-f0-9]{24})$/.exec(pathname);
    if (src && (req.method === 'GET' || req.method === 'HEAD')) return bridge.serveSource(src[1], req, res);
    const play = /^\/api\/bridge\/play\/([a-f0-9]{24})\.mkv$/.exec(pathname);
    if (play && req.method === 'GET') return bridge.servePlay(PORT, play[1], url.searchParams, req, res);
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
