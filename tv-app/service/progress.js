// Watch progress for the native app (UX-PLAN X1): where each film or episode
// was left, whether it was finished, which edition (exact file) was played,
// and which titles the viewer removed from Continue. One JSON document next to
// the AllDebrid pairing, written a few seconds after a change. Holds titles,
// positions and source IDs only, never stream URLs.
//
// Reports are ordered per session: the player sends a session ID and an
// increasing sequence number; a report from an older session, or an older
// report from the current one, is rejected so a late stop can't overwrite newer
// progress. Backward seeks and Start over are ordinary reports and are kept.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const accountStore = require('./account-store');
const { CUT_ORDER } = require('./cinephile');
const completion = require('./completion');

const FILE = 'progress.json';
const LIMIT = 400;
const RETIRED_SESSIONS = 6;
const WRITE_DELAY_MS = 3000;
const UPCOMING_DAYS = 14;             // a caught-up show stays in Continue this long before its next episode
const NEW_EPISODE_DAYS = 14;          // "New episode" if it aired this recently and after you finished
const store = accountStore.createFileStore(FILE, accountStore.defaultLocations);

let entries = null;   // videoId -> entry
let dismissed = null; // title id -> time removed from Continue
let intents = null;   // title id -> { edition, at }: a version the viewer chose for a show
let loading = null;
let timer = null;

function load() {
  if (entries) return Promise.resolve(entries);
  if (!loading) {
    loading = store.read().then(saved => {
      entries = new Map();
      dismissed = new Map();
      (saved && Array.isArray(saved.entries) ? saved.entries : []).forEach(e => { if (e && e.videoId) entries.set(e.videoId, e); });
      const gone = saved && saved.dismissed && typeof saved.dismissed === 'object' ? saved.dismissed : {};
      Object.keys(gone).forEach(id => { if (/^tt\d+$/.test(id)) dismissed.set(id, Number(gone[id]) || 0); });
      intents = new Map();
      const chosen = saved && saved.intents && typeof saved.intents === 'object' ? saved.intents : {};
      Object.keys(chosen).forEach(id => { if (/^tt\d+$/.test(id) && chosen[id] && chosen[id].edition) intents.set(id, chosen[id]); });
      return entries;
    }).catch(() => { entries = new Map(); dismissed = new Map(); intents = new Map(); return entries; });
  }
  return loading;
}

function scheduleWrite() {
  if (timer) return;
  timer = setTimeout(() => {
    timer = null;
    const list = Array.from(entries.values()).sort((a, b) => b.updated - a.updated).slice(0, LIMIT);
    const gone = {};
    dismissed.forEach((at, id) => { gone[id] = at; });
    const chosen = {};
    intents.forEach((value, id) => { chosen[id] = value; });
    store.write({ v: 2, entries: list, dismissed: gone, intents: chosen }).catch(() => {});
  }, WRITE_DELAY_MS);
}

const text = (value, max) => String(value || '').slice(0, max || 300);
const isoDate = (value) => /^\d{4}-\d{2}-\d{2}/.test(String(value || '')) ? String(value).slice(0, 24) : '';
const STATES = ['playing', 'paused', 'stopped', 'upnext', 'ended'];

/** Whether an incoming report may replace the stored one. */
function accepts(previous, report) {
  if (!previous || !report.session) return true;  // first report, or an app without sessions
  if (previous.session === report.session) return !(report.seq <= previous.seq);
  if ((previous.retired || []).indexOf(report.session) >= 0) return false;  // an older session, late
  return true;  // a new session started after the stored one
}

/**
 * Record a report. `entry` comes from the app; only known fields are kept.
 * `edition` is what the service knows about the source played (hash, quality),
 * never a URL. Returns { accepted, entry }.
 */
async function save(entry, edition) {
  if (!entry || !/^tt\d+(:\d+:\d+)?$/.test(String(entry.videoId || '')) || !/^tt\d+$/.test(String(entry.id || ''))) {
    throw Object.assign(new Error('Invalid progress entry'), { status: 400 });
  }
  const map = await load();
  const previous = map.get(String(entry.videoId));
  const report = {
    session: /^[a-z0-9-]{4,40}$/i.test(String(entry.session || '')) ? String(entry.session) : '',
    seq: Math.max(0, Math.floor(Number(entry.seq) || 0)),
    state: STATES.indexOf(entry.state) >= 0 ? entry.state : 'playing',
    position: Math.max(0, Number(entry.position) || 0),
    duration: Math.max(0, Number(entry.duration) || 0),
    creditsMs: Math.max(0, Number(entry.creditsMs) || 0),
  };
  if (!accepts(previous, report)) return { accepted: false, reason: 'stale', entry: previous };

  const sameSession = previous && report.session && previous.session === report.session;
  // Within one viewing, finishing is sticky: rewinding into the credits after Up
  // next doesn't make the title unfinished again. A new viewing decides afresh.
  const watched = completion.isComplete(report) || !!(sameSession && previous.watched);
  let retired = previous && previous.retired ? previous.retired.slice() : [];
  if (previous && previous.session && report.session && previous.session !== report.session) {
    retired = [previous.session].concat(retired).slice(0, RETIRED_SESSIONS);
  }
  const value = {
    videoId: text(entry.videoId, 40),
    id: text(entry.id, 20),
    type: entry.type === 'series' ? 'series' : 'movie',
    name: text(entry.name, 200),
    season: Number(entry.season) || 0,
    episode: Number(entry.episode) || 0,
    episodeTitle: text(entry.episodeTitle, 200),
    position: report.position,
    duration: report.duration,
    watched,
    state: report.state,
    session: report.session,
    seq: report.seq,
    retired,
    edition: edition || (previous && sameSession ? previous.edition : null) || null,
    updated: Date.now(),
    next: entry.next && /^tt\d+:\d+:\d+$/.test(String(entry.next.videoId || '')) ? {
      videoId: text(entry.next.videoId, 40),
      season: Number(entry.next.season) || 0,
      episode: Number(entry.next.episode) || 0,
      title: text(entry.next.title, 200),
      released: isoDate(entry.next.released),
    } : null,
  };
  map.set(value.videoId, value);
  // Starting a title again brings it back to Continue.
  if (dismissed.has(value.id) && dismissed.get(value.id) < value.updated) dismissed.delete(value.id);
  scheduleWrite();
  return { accepted: true, entry: value };
}

/** Whether the viewer removed a title from Continue (until they start it again). */
async function isDismissed(id) {
  await load();
  return dismissed.has(String(id));
}

/** Remove a title from Continue until the viewer starts it again. */
async function dismiss(id) {
  if (!/^tt\d+$/.test(String(id || ''))) throw Object.assign(new Error('Invalid title'), { status: 400 });
  await load();
  dismissed.set(String(id), Date.now());
  scheduleWrite();
  return { dismissed: true };
}

const DAY = 86400000;
const aired = (released, now) => !released || Date.parse(released) <= now;

/**
 * Continue: one card per title, the most recent viewing deciding it, in tiers:
 * unfinished, then a new episode out, then the next episode, then a show you
 * are caught up on whose next episode airs soon (no Play; it says when).
 * Finished films and dismissed titles are left out.
 */
async function continueWatching(now) {
  now = now || Date.now();
  const map = await load();
  const latest = new Map();
  Array.from(map.values()).sort((a, b) => b.updated - a.updated).forEach(e => { if (!latest.has(e.id)) latest.set(e.id, e); });
  const tiers = [[], [], [], []];
  latest.forEach(e => {
    if (dismissed.has(e.id)) return;
    if (completion.resumePoint(e) > 0) { tiers[0].push(Object.assign({ kind: 'resume' }, e)); return; }
    if (!e.watched || !e.next) return;
    const released = e.next.released;
    if (aired(released, now)) {
      const fresh = released && Date.parse(released) > e.updated - DAY && now - Date.parse(released) < NEW_EPISODE_DAYS * DAY;
      (fresh ? tiers[1] : tiers[2]).push(Object.assign({ kind: 'next', reason: fresh ? 'new' : 'next' }, e));
    } else if (Date.parse(released) - now < UPCOMING_DAYS * DAY) {
      tiers[3].push(Object.assign({ kind: 'upcoming', reason: 'caughtUp' }, e));
    }
  });
  return [].concat(tiers[0], tiers[1], tiers[2], tiers[3]).slice(0, 20);
}

/** Every entry of one title, by video ID (what the title page and resolver need). */
async function forTitle(id) {
  const map = await load();
  const result = {};
  map.forEach(e => {
    if (e.id === id) result[e.videoId] = { position: e.position, duration: e.duration, watched: e.watched, updated: e.updated };
  });
  return result;
}

/** The edition most recently played for a title (a show's last episode), or null. */
async function latestEdition(id) {
  const map = await load();
  let best = null;
  map.forEach(e => { if (e.id === id && e.edition && (!best || e.updated > best.updated)) best = e; });
  return best ? best.edition : null;
}

/**
 * Remember the version the viewer chose for a show, so its next episodes keep
 * to it (UX-PLAN X3). `edition` null forgets the choice.
 */
async function setIntent(id, edition) {
  if (!/^tt\d+$/.test(String(id || ''))) throw Object.assign(new Error('Invalid title'), { status: 400 });
  await load();
  if (edition) intents.set(String(id), { edition, at: Date.now() });
  else intents.delete(String(id));
  scheduleWrite();
  return { intent: edition || null };
}

/**
 * The version to keep to for a show: the one the viewer chose, else the one
 * they last played. A choice wins until it is changed.
 */
async function intentFor(id) {
  await load();
  const chosen = intents.get(String(id));
  if (chosen) return Object.assign({ from: 'chosen' }, chosen.edition);
  const played = await latestEdition(id);
  return played ? Object.assign({ from: 'played' }, played) : null;
}

/** Test hook: forget everything in memory (the file is left alone). */
function resetForTests() {
  entries = null;
  dismissed = null;
  intents = null;
  loading = null;
  if (timer) { clearTimeout(timer); timer = null; }
}

// App settings (audio and subtitle language, subtitle look, picture, sources, motion,
// transparency, contrast, autoplay,
// recent searches):
// one small document.
const settingsStore = accountStore.createFileStore('settings.json', accountStore.defaultLocations);
// subtitleSize: the TV's subtitle size level, 0 (smallest) to 4; the TV's own default is 2,
// which viewers found far too large. picture: 'best' | 'hdr10' | 'sdr'. sources: 'all' | 'torrentio'.
const SETTINGS_DEFAULTS = { language: 'English', subtitleLanguage: 'Off', subtitleSize: 0, subtitleBackground: false, picture: 'best', sources: 'all', cut: 'Theatrical', reduceMotion: false, reduceTransparency: false, highContrast: false, autoplayNext: true, welcomeSeen: false, recentSearches: [] };
const LANGUAGE = /^[A-Za-z ]{2,30}$/;
let settings = null;

async function getSettings() {
  if (!settings) settings = Object.assign({}, SETTINGS_DEFAULTS, (await settingsStore.read().catch(() => null)) || {});
  return settings;
}

/** Merge known, well-typed fields; anything else is ignored. */
async function saveSettings(input) {
  input = input || {};
  const current = await getSettings();
  if (typeof input.language === 'string' && LANGUAGE.test(input.language)) current.language = input.language;
  if (typeof input.subtitleLanguage === 'string' && LANGUAGE.test(input.subtitleLanguage)) current.subtitleLanguage = input.subtitleLanguage;
  if (Number.isInteger(input.subtitleSize) && input.subtitleSize >= 0 && input.subtitleSize <= 4) current.subtitleSize = input.subtitleSize;
  if (typeof input.subtitleBackground === 'boolean') current.subtitleBackground = input.subtitleBackground;
  // The cut the viewer last chose in a versions list (2.8): the edition row opens on it.
  if (typeof input.cut === 'string' && CUT_ORDER.indexOf(input.cut) >= 0) current.cut = input.cut;
  if (['best', 'hdr10', 'sdr'].indexOf(input.picture) >= 0) current.picture = input.picture;
  if (['all', 'torrentio'].indexOf(input.sources) >= 0) current.sources = input.sources;
  if (typeof input.reduceMotion === 'boolean') current.reduceMotion = input.reduceMotion;
  if (typeof input.reduceTransparency === 'boolean') current.reduceTransparency = input.reduceTransparency;
  if (typeof input.highContrast === 'boolean') current.highContrast = input.highContrast;
  if (typeof input.autoplayNext === 'boolean') current.autoplayNext = input.autoplayNext;
  if (typeof input.welcomeSeen === 'boolean') current.welcomeSeen = input.welcomeSeen;
  if (Array.isArray(input.recentSearches)) current.recentSearches = input.recentSearches.filter(s => typeof s === 'string').map(s => s.slice(0, 60)).slice(0, 8);
  await settingsStore.write(current);
  return current;
}

module.exports = { save, dismiss, isDismissed, continueWatching, forTitle, latestEdition, setIntent, intentFor, getSettings, saveSettings, resetForTests };
