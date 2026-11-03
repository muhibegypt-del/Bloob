/** Remembers where the user stopped watching. Stores content IDs and seconds only. */
export interface PositionStore {
  get(key: string): number | null;
  set(key: string, seconds: number, duration: number | null): void;
  clear(key: string): void;
}

const PREFIX = 'bloob.position.';
const MAX_ENTRIES = 200;

/** Positions too close to either end are not worth resuming. */
export const RESUME_MIN_SECONDS = 30;
export const RESUME_MAX_FRACTION = 0.95;

export function worthResuming(seconds: number, duration: number | null): boolean {
  if (!Number.isFinite(seconds) || seconds < RESUME_MIN_SECONDS) return false;
  return !(duration && duration > 0 && seconds / duration >= RESUME_MAX_FRACTION);
}

/** localStorage-backed store; every access is guarded because webOS storage can be unavailable. */
export function localPositionStore(storage: Storage | undefined = safeLocalStorage()): PositionStore {
  return {
    get(key) {
      try {
        const raw = storage?.getItem(PREFIX + key);
        if (!raw) return null;
        const saved = JSON.parse(raw) as { s: number; d: number | null };
        return worthResuming(saved.s, saved.d) ? saved.s : null;
      } catch { return null; }
    },
    set(key, seconds, duration) {
      try {
        if (!storage) return;
        if (!worthResuming(seconds, duration)) return storage.removeItem(PREFIX + key);
        storage.setItem(PREFIX + key, JSON.stringify({ s: Math.floor(seconds), d: duration, t: Date.now() }));
        prune(storage);
      } catch { /* storage full or unavailable: resume is a convenience */ }
    },
    clear(key) {
      try { storage?.removeItem(PREFIX + key); } catch { /* ignore */ }
    },
  };
}

function safeLocalStorage(): Storage | undefined {
  try { return typeof localStorage === 'undefined' ? undefined : localStorage; } catch { return undefined; }
}

function prune(storage: Storage) {
  const keys: { key: string; t: number }[] = [];
  for (let i = 0; i < storage.length; i++) {
    const key = storage.key(i);
    if (!key?.startsWith(PREFIX)) continue;
    try { keys.push({ key, t: JSON.parse(storage.getItem(key) || '{}').t || 0 }); } catch { keys.push({ key, t: 0 }); }
  }
  if (keys.length <= MAX_ENTRIES) return;
  keys.sort((a, b) => a.t - b.t).slice(0, keys.length - MAX_ENTRIES).forEach(entry => storage.removeItem(entry.key));
}

export const memoryPositionStore = (): PositionStore & { data: Map<string, number> } => {
  const data = new Map<string, number>();
  return {
    data,
    get: key => data.get(key) ?? null,
    set: (key, seconds, duration) => { worthResuming(seconds, duration) ? data.set(key, seconds) : data.delete(key); },
    clear: key => { data.delete(key); },
  };
};
