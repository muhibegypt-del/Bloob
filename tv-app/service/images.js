// Artwork proxy for the native app, which speaks plain HTTP to this service
// only. Fetches allowed image hosts, keeps recent images in memory and on disk,
// and serves them to the app. It is not an open proxy: hosts are allow-listed.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const crypto = require('crypto');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { get } = require('./net');
const config = require('./config');

const HOSTS = /(^|\.)metahub\.space$|^image\.tmdb\.org$|^m\.media-amazon\.com$/;
// The in-memory byte cache follows the app's memory policy (R5b): full while
// browsing, small during playback and in the background. The disk cache stays.
const MEMORY_BUDGETS = { normal: 40 * 1024 * 1024, playback: 12 * 1024 * 1024, background: 4 * 1024 * 1024 };
let memoryBudget = MEMORY_BUDGETS.normal;
const DISK_BUDGET = 200 * 1024 * 1024;

const memory = new Map(); // key -> { body, type }, in recency order
let memoryBytes = 0;
const inflight = new Map();
let diskDir = null;
let writesSincePrune = 0;

function pickDiskDir() {
  const candidates = [path.join('/media/internal', config.appId, 'images'), path.join(os.tmpdir(), config.appId + '-images')];
  for (const dir of candidates) {
    try {
      fs.mkdirSync(dir, { recursive: true });
      const probe = path.join(dir, '.probe');
      fs.writeFileSync(probe, 'x');
      fs.unlinkSync(probe);
      return dir;
    } catch (_) { /* try the next */ }
  }
  return null;
}

function remember(key, value) {
  if (memory.has(key)) { memoryBytes -= memory.get(key).body.length; memory.delete(key); }
  memory.set(key, value);
  memoryBytes += value.body.length;
  trim();
}

function trim() {
  while (memoryBytes > memoryBudget && memory.size > 1) {
    const oldest = memory.keys().next().value;
    memoryBytes -= memory.get(oldest).body.length;
    memory.delete(oldest);
  }
}

/** Applies the app's memory policy; returns the budget in bytes, or 0 if unknown. */
function setMemoryPolicy(policy) {
  const budget = MEMORY_BUDGETS[policy];
  if (!budget) return 0;
  memoryBudget = budget;
  trim();
  return budget;
}

function cacheBytes() { return memoryBytes; }

function prune() {
  if (!diskDir) return;
  fs.readdir(diskDir, (error, names) => {
    if (error) return;
    const files = [];
    let total = 0;
    names.forEach(name => {
      try {
        const stat = fs.statSync(path.join(diskDir, name));
        files.push({ name, size: stat.size, time: stat.mtimeMs });
        total += stat.size;
      } catch (_) { /* raced */ }
    });
    files.sort((a, b) => a.time - b.time);
    // These are our own cache files in our own cache folder.
    while (total > DISK_BUDGET && files.length) {
      const file = files.shift();
      total -= file.size;
      fs.unlink(path.join(diskDir, file.name), () => {});
    }
  });
}

function typeFor(body, fallback) {
  if (body[0] === 0xff && body[1] === 0xd8) return 'image/jpeg';
  if (body[0] === 0x89 && body[1] === 0x50) return 'image/png';
  return fallback;
}

async function fetchImage(url) {
  let target;
  try { target = new URL(url); } catch (_) { throw Object.assign(new Error('Invalid image URL'), { status: 400 }); }
  if (target.protocol !== 'https:' || !HOSTS.test(target.hostname)) throw Object.assign(new Error('Image host not allowed'), { status: 403 });
  const key = crypto.createHash('sha1').update(target.toString()).digest('hex');
  const hit = memory.get(key);
  if (hit) { remember(key, hit); return hit; }
  if (inflight.has(key)) return inflight.get(key);

  const work = (async () => {
    if (diskDir === null) diskDir = pickDiskDir() || '';
    const file = diskDir ? path.join(diskDir, key) : '';
    if (file) {
      try {
        const body = await fs.promises.readFile(file);
        const value = { body, type: typeFor(body, 'application/octet-stream') };
        remember(key, value);
        fs.utimes(file, new Date(), new Date(), () => {});
        return value;
      } catch (_) { /* not cached */ }
    }
    const result = await get(target.toString(), { timeout: 15000, maxBytes: 4 * 1024 * 1024, accept: 'image/jpeg,image/png', allow: host => HOSTS.test(host) });
    const value = { body: result.body, type: typeFor(result.body, result.type) };
    if (value.type !== 'image/jpeg' && value.type !== 'image/png') throw Object.assign(new Error('Unsupported image format'), { status: 415 });
    remember(key, value);
    if (file) {
      fs.writeFile(file, value.body, () => {});
      if (++writesSincePrune >= 100) { writesSincePrune = 0; prune(); }
    }
    return value;
  })();
  inflight.set(key, work);
  try { return await work; } finally { inflight.delete(key); }
}

setTimeout(() => { if (diskDir === null) diskDir = pickDiskDir() || ''; prune(); }, 5000).unref();

module.exports = { fetchImage, HOSTS, setMemoryPolicy, cacheBytes };
