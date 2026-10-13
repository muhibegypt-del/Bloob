// Watch progress for the native app: where each film or episode was left, so
// Home can offer "Continue watching" and Details can mark watched episodes.
// Stored as one JSON document next to the AllDebrid pairing, written at most
// every few seconds. Holds titles and positions only, never stream URLs.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const accountStore = require('./account-store');

const FILE = 'progress.json';
const LIMIT = 400;
const WRITE_DELAY_MS = 3000;
const store = accountStore.createFileStore(FILE, accountStore.defaultLocations);

let entries = null; // videoId -> entry
let loading = null;
let timer = null;

function load() {
  if (entries) return Promise.resolve(entries);
  if (!loading) {
    loading = store.read().then(saved => {
      entries = new Map();
      (saved && Array.isArray(saved.entries) ? saved.entries : []).forEach(e => { if (e && e.videoId) entries.set(e.videoId, e); });
      return entries;
    }).catch(() => { entries = new Map(); return entries; });
  }
  return loading;
}

function scheduleWrite() {
  if (timer) return;
  timer = setTimeout(() => {
    timer = null;
    const list = Array.from(entries.values()).sort((a, b) => b.updated - a.updated).slice(0, LIMIT);
    store.write({ v: 1, entries: list }).catch(() => {});
  }, WRITE_DELAY_MS);
}

const text = (value, max) => String(value || '').slice(0, max || 300);

/** Record a position. `entry` comes from the app; only known fields are kept. */
async function save(entry) {
  if (!entry || !/^tt\d+(:\d+:\d+)?$/.test(String(entry.videoId || '')) || !/^tt\d+$/.test(String(entry.id || ''))) {
    throw Object.assign(new Error('Invalid progress entry'), { status: 400 });
  }
  const map = await load();
  const position = Math.max(0, Number(entry.position) || 0);
  const duration = Math.max(0, Number(entry.duration) || 0);
  const watched = duration > 0 && position >= duration * 0.95;
  const value = {
    videoId: text(entry.videoId, 40),
    id: text(entry.id, 20),
    type: entry.type === 'series' ? 'series' : 'movie',
    name: text(entry.name, 200),
    season: Number(entry.season) || 0,
    episode: Number(entry.episode) || 0,
    episodeTitle: text(entry.episodeTitle, 200),
    position,
    duration,
    watched,
    updated: Date.now(),
    next: entry.next && /^tt\d+:\d+:\d+$/.test(String(entry.next.videoId || '')) ? {
      videoId: text(entry.next.videoId, 40),
      season: Number(entry.next.season) || 0,
      episode: Number(entry.next.episode) || 0,
      title: text(entry.next.title, 200),
    } : null,
  };
  map.set(value.videoId, value);
  scheduleWrite();
  return value;
}

/**
 * One row per title: the most recent entry. Unfinished items continue; a
 * finished episode with a known next episode offers that one.
 */
async function continueWatching() {
  const map = await load();
  const latest = new Map();
  Array.from(map.values()).sort((a, b) => b.updated - a.updated).forEach(e => { if (!latest.has(e.id)) latest.set(e.id, e); });
  const items = [];
  latest.forEach(e => {
    if (!e.watched && e.position >= 30000) items.push(Object.assign({ kind: 'resume' }, e));
    else if (e.watched && e.next) items.push(Object.assign({ kind: 'next' }, e));
  });
  return items.slice(0, 20);
}

async function forTitle(id) {
  const map = await load();
  const result = {};
  map.forEach(e => {
    if (e.id === id) result[e.videoId] = { position: e.position, duration: e.duration, watched: e.watched, updated: e.updated };
  });
  return result;
}

// App settings (preferred audio language, recent searches): one small document.
const settingsStore = accountStore.createFileStore('settings.json', accountStore.defaultLocations);
let settings = null;

async function getSettings() {
  if (!settings) settings = Object.assign({ language: 'English', recentSearches: [] }, (await settingsStore.read().catch(() => null)) || {});
  return settings;
}

async function saveSettings(input) {
  const current = await getSettings();
  if (typeof input.language === 'string' && /^[A-Za-z ]{2,30}$/.test(input.language)) current.language = input.language;
  if (Array.isArray(input.recentSearches)) current.recentSearches = input.recentSearches.filter(s => typeof s === 'string').map(s => s.slice(0, 60)).slice(0, 8);
  await settingsStore.write(current);
  return current;
}

module.exports = { save, continueWatching, forTitle, getSettings, saveSettings };
