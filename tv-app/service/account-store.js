const fs = require('fs').promises;
const os = require('os');
const path = require('path');

// The AllDebrid key must survive app and TV restarts. Which directories a webOS
// JS service may write depends on the firmware and on how the app was
// installed, so the store tries several locations in order of durability,
// verifies each write by reading it back, and reports what happened. The key
// itself is never logged or returned.
const APP_ID = 'com.muhib.tvprototype';
const FILE = 'account.json';

function defaultLocations() {
  const list = [];
  if (process.env.TV_APP_DATA_DIR) list.push({ id: 'configured', dir: process.env.TV_APP_DATA_DIR, durable: true });
  if (process.platform === 'win32') {
    list.push({ id: 'app-data', dir: path.join(process.env.LOCALAPPDATA || process.cwd(), 'Bloob'), durable: true });
    return list;
  }
  list.push({ id: 'app-data', dir: path.join('/media/internal', APP_ID), durable: true });
  const home = os.homedir();
  if (home && home !== '/') list.push({ id: 'home', dir: path.join(home, '.bloob'), durable: true });
  // Inside the installed service: survives restarts, may be replaced by a reinstall.
  list.push({ id: 'service', dir: path.join(__dirname, '.data'), durable: false });
  // Survives app/service restarts until the TV reboots.
  list.push({ id: 'temporary', dir: path.join(os.tmpdir(), APP_ID), durable: false });
  return list;
}

async function readKey(file) {
  const saved = JSON.parse(await fs.readFile(file, 'utf8'));
  return saved && typeof saved.apiKey === 'string' ? saved.apiKey : '';
}

async function writeVerified(location, value) {
  await fs.mkdir(location.dir, { recursive: true, mode: 0o700 });
  const file = path.join(location.dir, FILE);
  const temporary = `${file}.${process.pid}.tmp`;
  await fs.writeFile(temporary, JSON.stringify(value), { mode: 0o600 });
  await fs.rename(temporary, file);
  const check = JSON.parse(await fs.readFile(file, 'utf8'));
  if (JSON.stringify(check) !== JSON.stringify(value)) {
    const error = new Error('Read-back mismatch');
    error.code = 'EVERIFY';
    throw error;
  }
}

/** Build a store over a list of locations. The module exports one over the TV defaults. */
function createStore(getLocations) {
  let status = { location: null, durable: false, attempts: [] };

  function getStatus() {
    return { location: status.location, durable: status.durable, attempts: status.attempts.slice() };
  }

  /** Returns the first saved key found, checking every location. */
  async function load() {
    const attempts = [];
    for (const location of getLocations()) {
      try {
        const key = await readKey(path.join(location.dir, FILE));
        if (key) {
          status = { location: location.id, durable: location.durable, attempts };
          return key;
        }
        attempts.push({ location: location.id, result: 'empty' });
      } catch (error) {
        attempts.push({ location: location.id, result: error.code === 'ENOENT' ? 'none' : error.code || 'unreadable' });
      }
    }
    status = { location: null, durable: false, attempts };
    return '';
  }

  /** Save to the most durable writable location. Resolves with the status; never throws. */
  async function save(apiKey) {
    const attempts = [];
    for (const location of getLocations()) {
      try {
        await writeVerified(location, { apiKey });
        status = { location: location.id, durable: location.durable, attempts };
        return getStatus();
      } catch (error) {
        attempts.push({ location: location.id, result: error.code || 'failed' });
      }
    }
    status = { location: null, durable: false, attempts };
    return getStatus();
  }

  /** Stop a rejected key from being reloaded. Overwrites rather than deletes. */
  async function clear() {
    for (const location of getLocations()) {
      try {
        await fs.access(path.join(location.dir, FILE));
        await writeVerified(location, {});
      } catch (_) { /* nothing saved there */ }
    }
    status = { location: null, durable: false, attempts: [] };
  }

  return { load, save, clear, getStatus, locations: getLocations };
}

module.exports = Object.assign(createStore(defaultLocations), { createStore, defaultLocations });
