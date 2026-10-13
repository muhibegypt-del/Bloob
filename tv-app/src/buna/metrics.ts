import type { BunaMetric } from './types';
import { trace, type TraceData } from './trace';

const LIMIT = 200;
const entries: BunaMetric[] = [];

/**
 * In-memory metric ring. Metrics carry timings and positions only, never URLs
 * or account data, so they are safe to read through the web inspector on the
 * TV (`window.__buna.metrics()`).
 */
export const metrics = {
  record(metric: BunaMetric) {
    entries.push(metric);
    if (entries.length > LIMIT) entries.splice(0, entries.length - LIMIT);
    const data: TraceData = {};
    for (const [key, value] of Object.entries(metric)) {
      if (key !== 'at' && typeof value === 'number') data[key] = value;
    }
    trace.record('metric', metric.name, data);
    if (typeof console !== 'undefined') console.info('[buna]', JSON.stringify(metric));
  },
  list(): readonly BunaMetric[] {
    return entries.slice();
  },
  clear() {
    entries.length = 0;
  },
};
