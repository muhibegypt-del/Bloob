/** A bounded, read-only event timeline for the TV inspector.
 * Payloads accept numbers and booleans only: no URLs, titles, keys, or replies.
 */
export type TraceArea = 'app' | 'remote' | 'api' | 'sources' | 'session' | 'media' | 'native' | 'metric';
export type TraceData = Record<string, number | boolean | null>;

export interface TraceEntry {
  seq: number;
  at: number;
  elapsedMs: number;
  area: TraceArea;
  event: string;
  data?: TraceData;
}

const LIMIT = 600;
const entries: TraceEntry[] = [];
const openedAt = typeof performance !== 'undefined' ? performance.now() : 0;
let sequence = 0;

export const trace = {
  record(area: TraceArea, event: string, data?: TraceData): void {
    const safe: TraceData = {};
    if (data) {
      for (const [key, value] of Object.entries(data)) {
        if (typeof value === 'boolean' || value === null) safe[key] = value;
        else if (typeof value === 'number') safe[key] = Number.isFinite(value) ? Math.round(value * 10) / 10 : null;
      }
    }
    entries.push({
      seq: ++sequence,
      at: Date.now(),
      elapsedMs: typeof performance !== 'undefined' ? Math.round(performance.now() - openedAt) : 0,
      area,
      event,
      ...(Object.keys(safe).length ? { data: safe } : {}),
    });
    if (entries.length > LIMIT) entries.splice(0, entries.length - LIMIT);
  },
  since(after = 0): TraceEntry[] { return entries.filter(entry => entry.seq > after); },
  list(): TraceEntry[] { return entries.slice(); },
  clear(): void { entries.length = 0; },
};
