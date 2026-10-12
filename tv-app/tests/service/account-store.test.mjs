import { mkdtempSync, readFileSync, writeFileSync } from 'fs';
import { tmpdir } from 'os';
import { join } from 'path';
import { describe, expect, it } from 'vitest';
import accountStore from '../../service/account-store.js';

const { createStore, defaultLocations } = accountStore;
const temp = (name) => mkdtempSync(join(tmpdir(), `bloob-${name}-`));

/** A path that can never become a directory, even for root: its parent is a file. */
function unwritable() {
  const blocker = join(temp('block'), 'file');
  writeFileSync(blocker, 'x');
  return join(blocker, 'sub');
}

describe('account store', () => {
  it('saves, verifies, and reloads the key', async () => {
    const dir = temp('store');
    const store = createStore(() => [{ id: 'app-data', dir, durable: true }]);
    expect(await store.save('secret-key')).toMatchObject({ location: 'app-data', durable: true, attempts: [] });
    expect(JSON.parse(readFileSync(join(dir, 'account.json'), 'utf8'))).toEqual({ apiKey: 'secret-key' });

    const restarted = createStore(() => [{ id: 'app-data', dir, durable: true }]);
    expect(await restarted.load()).toBe('secret-key');
    expect(restarted.getStatus().location).toBe('app-data');
    expect(JSON.stringify(restarted.getStatus())).not.toContain('secret-key');
  });

  it('falls back to the next location and records why the first failed', async () => {
    const fallback = temp('fallback');
    const locations = () => [
      { id: 'app-data', dir: unwritable(), durable: true },
      { id: 'temporary', dir: fallback, durable: false },
    ];
    const status = await createStore(locations).save('k');
    expect(status.location).toBe('temporary');
    expect(status.durable).toBe(false);
    expect(status.attempts).toHaveLength(1);
    expect(status.attempts[0].location).toBe('app-data');
    expect(status.attempts[0].result).toMatch(/^E/);
    // A restart finds the key in the fallback location.
    expect(await createStore(locations).load()).toBe('k');
  });

  it('reports total failure without throwing', async () => {
    const store = createStore(() => [{ id: 'app-data', dir: unwritable(), durable: true }]);
    expect(await store.save('k')).toMatchObject({ location: null, durable: false });
    expect(await store.load()).toBe('');
  });

  it('distinguishes "never paired" from unreadable storage', async () => {
    const empty = temp('empty');
    const store = createStore(() => [{ id: 'app-data', dir: empty, durable: true }]);
    expect(await store.load()).toBe('');
    expect(store.getStatus().attempts).toEqual([{ location: 'app-data', result: 'none' }]);
  });

  it('clear() overwrites saved keys so a revoked key is not reloaded', async () => {
    const dir = temp('clear');
    const store = createStore(() => [{ id: 'app-data', dir, durable: true }]);
    await store.save('revoked');
    await store.clear();
    expect(JSON.parse(readFileSync(join(dir, 'account.json'), 'utf8'))).toEqual({});
    expect(await store.load()).toBe('');
  });

  it('keeps the original TV location first so existing pairings still load', () => {
    const previous = process.env.TV_APP_DATA_DIR;
    delete process.env.TV_APP_DATA_DIR;
    try {
      const ids = defaultLocations().map(l => l.id);
      expect(ids[0]).toBe(process.platform === 'win32' ? 'app-data' : 'app-data');
      if (process.platform !== 'win32') expect(defaultLocations()[0].dir).toBe('/media/internal/com.muhib.tvprototype');
    } finally {
      if (previous !== undefined) process.env.TV_APP_DATA_DIR = previous;
    }
  });
});
