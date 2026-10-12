/** Time source and timers, injectable so the session is deterministic under test. */
export interface Scheduler {
  now(): number;
  setTimeout(fn: () => void, ms: number): number;
  clearTimeout(id: number | undefined): void;
  setInterval(fn: () => void, ms: number): number;
  clearInterval(id: number | undefined): void;
}

export const browserScheduler: Scheduler = {
  now: () => performance.now(),
  setTimeout: (fn, ms) => globalThis.setTimeout(fn, ms) as unknown as number,
  clearTimeout: id => globalThis.clearTimeout(id),
  setInterval: (fn, ms) => globalThis.setInterval(fn, ms) as unknown as number,
  clearInterval: id => globalThis.clearInterval(id),
};
