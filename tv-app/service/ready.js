// "Ready now" (2.8): which of the top versions the owner's AllDebrid already
// has, so the list can say so and the best match can prefer a ready copy at
// equal quality. AllDebrid has offered no public "is it cached" check since
// late 2024, so this asks the account itself: add the magnets, read `ready`,
// and remove again only the magnets this check added that are not ready.
// Magnets already in the account are never touched. Only AllDebrid sees the key.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).

const TTL_MS = 6 * 60 * 60 * 1000;
const MAX_CHECK = 8;

function createReadyCheck(alldebrid) {
  const known = new Map();  // hash -> { ready, at }

  function get(hash) {
    const k = known.get(hash);
    return k && Date.now() - k.at < TTL_MS ? k.ready : undefined;
  }

  async function check(hashes) {
    const todo = hashes.filter(h => /^[a-f0-9]{40}$/.test(h) && get(h) === undefined).slice(0, MAX_CHECK);
    if (!todo.length) return;
    // What the account already holds, so it is left exactly as it was.
    const before = await alldebrid('/v4.1/magnet/status', {}).catch(() => null);
    if (!before) return;
    const list = Array.isArray(before.magnets) ? before.magnets : before.magnets ? Object.keys(before.magnets).map(k => before.magnets[k]) : [];
    const mine = new Set(list.map(m => String(m.hash || '').toLowerCase()));
    todo.filter(h => mine.has(h)).forEach(h => {
      const m = list.find(x => String(x.hash || '').toLowerCase() === h);
      known.set(h, { ready: m.statusCode === 4, at: Date.now() });
    });
    const fresh = todo.filter(h => !mine.has(h));
    if (!fresh.length) return;
    const upload = await alldebrid('/v4/magnet/upload', fresh.map(h => ['magnets[]', h])).catch(() => null);
    const added = upload && Array.isArray(upload.magnets) ? upload.magnets : [];
    await Promise.all(added.map(async m => {
      const hash = String(m.hash || '').toLowerCase();
      if (!hash || m.error) return;
      known.set(hash, { ready: m.ready === true, at: Date.now() });
      if (m.ready !== true && m.id) await alldebrid('/v4/magnet/delete', { id: String(m.id) }).catch(() => {});
    }));
  }

  /** Marks candidates `ready` (true/false) where known, checking the top ones within `waitMs`. */
  async function annotate(candidates, topHashes, waitMs) {
    await Promise.race([check(topHashes).catch(() => {}), new Promise(resolve => setTimeout(resolve, waitMs))]);
    return candidates.map(c => {
      const m = /^magnet:([a-f0-9]{40})/i.exec(String(c.id || ''));
      const ready = m ? get(m[1].toLowerCase()) : undefined;
      return ready === undefined ? c : Object.assign({}, c, { ready });
    });
  }

  return { annotate, check, get };
}

module.exports = { createReadyCheck };
