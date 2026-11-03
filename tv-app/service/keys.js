// The owner's API keys (TMDB, MDBList), from keys.json in the service's data
// folder, written on the TV by hand: {"tmdb": "…", "mdblist": "…"}. Read at most
// once a minute, so a new key works without a restart. Never sent to the app,
// logged or committed.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const accountStore = require('./account-store');

const store = accountStore.createFileStore('keys.json', accountStore.defaultLocations);
let read = { at: 0, value: {} };

async function get(name) {
  if (Date.now() - read.at >= 60000) {
    const saved = await store.read().catch(() => null);
    read = { at: Date.now(), value: saved && typeof saved === 'object' ? saved : {} };
  }
  const v = read.value[name];
  return typeof v === 'string' ? v.trim() : '';
}

module.exports = { get };
