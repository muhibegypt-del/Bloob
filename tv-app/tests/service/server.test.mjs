import { mkdtempSync, readFileSync, writeFileSync } from 'fs';
import { createRequire } from 'module';
import { tmpdir } from 'os';
import { join } from 'path';
import { afterAll, beforeAll, describe, expect, it } from 'vitest';

// Runs the real service on a random port. The account store is swapped for one
// over temporary folders, so the test never touches a real pairing.
const require = createRequire(import.meta.url);
const temp = (name) => mkdtempSync(join(tmpdir(), `bloob-${name}-`));

let server;
let base;
let dir;
let shared;

beforeAll(async () => {
  dir = temp('server');
  shared = temp('shared');
  const real = require('../../service/account-store.js');
  const locations = () => [
    { id: 'app-data', dir, durable: true },
    { id: 'shared:other', dir: shared, durable: true, readOnly: true },
  ];
  const fake = Object.assign(real.createStore(locations), {
    createStore: real.createStore,
    createFileStore: real.createFileStore,
    defaultLocations: locations,
  });
  await fake.save('saved-key');
  writeFileSync(join(shared, 'account.json'), JSON.stringify({ apiKey: 'shared-key' }));
  const path = require.resolve('../../service/account-store.js');
  require.cache[path] = { id: path, filename: path, loaded: true, exports: fake };
  process.env.TV_APP_PORT = '0';
  delete process.env.ALLDEBRID_API_KEY;
  delete process.env.BLOOB_DEV_FIXTURES;
  server = require('../../service/server.js').server;
  if (!server.listening) await new Promise(resolve => server.once('listening', resolve));
  base = `http://127.0.0.1:${server.address().port}`;
});

afterAll(() => new Promise(resolve => server.close(resolve)));

const get = (route) => fetch(base + route).then(r => r.json());
const post = (route, body) => fetch(base + route, {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify(body || {}),
}).then(async r => ({ status: r.status, body: await r.json() }));

async function until(predicate) {
  for (let i = 0; i < 100; i++) {
    const value = await predicate();
    if (value) return value;
    await new Promise(r => setTimeout(r, 20));
  }
  throw new Error('timeout');
}

describe('service settings', () => {
  it('has defaults for the new fields', async () => {
    expect(await get('/api/settings')).toMatchObject({
      language: 'English', subtitleLanguage: 'Off', reduceMotion: false, autoplayNext: true, recentSearches: [],
    });
  });

  it('round-trips valid values and ignores bad ones', async () => {
    const saved = await post('/api/settings', { reduceMotion: true, autoplayNext: false, subtitleLanguage: 'French', language: 'German' });
    expect(saved.body).toMatchObject({ reduceMotion: true, autoplayNext: false, subtitleLanguage: 'French', language: 'German' });
    const ignored = await post('/api/settings', { reduceMotion: 'yes', autoplayNext: 1, subtitleLanguage: '<b>', language: 42 });
    expect(ignored.body).toMatchObject({ reduceMotion: true, autoplayNext: false, subtitleLanguage: 'French', language: 'German' });
    expect((await post('/api/settings', { subtitleLanguage: 'Off' })).body.subtitleLanguage).toBe('Off');
    // Written to disk for the next start.
    const file = await until(() => {
      try { return JSON.parse(readFileSync(join(dir, 'settings.json'), 'utf8')); } catch (_) { return null; }
    });
    expect(file).toMatchObject({ reduceMotion: true, autoplayNext: false, subtitleLanguage: 'Off', language: 'German' });
  });
});

describe('service unpair', () => {
  it('forgets the pairing without deleting files and does not fall back to a shared one', async () => {
    expect((await until(async () => (await get('/api/account')).paired ? true : null))).toBe(true);
    const result = await post('/api/account/unpair');
    expect(result.status).toBe(200);
    expect(result.body.paired).toBe(false);
    expect(JSON.stringify(result.body)).not.toContain('saved-key');
    expect((await get('/api/account')).paired).toBe(false);
    expect((await get('/api/health')).paired).toBe(false);
    // Overwritten, not deleted; the other app's pairing is untouched.
    expect(JSON.parse(readFileSync(join(dir, 'account.json'), 'utf8'))).toEqual({ unpaired: true });
    expect(JSON.parse(readFileSync(join(shared, 'account.json'), 'utf8'))).toEqual({ apiKey: 'shared-key' });
    // A restart does not pick up the shared pairing again.
    const store = require('../../service/account-store.js');
    expect(await store.load()).toBe('');
    expect(store.getStatus().attempts).toEqual([{ location: 'app-data', result: 'unpaired' }]);
  });

  it('is guarded like the other routes', async () => {
    const plain = await fetch(base + '/api/account/unpair', { method: 'POST', body: '{}', headers: { 'Content-Type': 'text/plain' } });
    expect(plain.status).toBe(403);
  });
});
