/** `1:02:03`, `4:05`, or `--:--` when unknown. */
export function clock(seconds: number | null | undefined): string {
  if (seconds === null || seconds === undefined || !Number.isFinite(seconds) || seconds < 0) return '--:--';
  const s = Math.floor(seconds);
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const sec = String(s % 60).padStart(2, '0');
  return h ? `${h}:${String(m).padStart(2, '0')}:${sec}` : `${m}:${sec}`;
}

/** `+30s`, `−1:30`. */
export function delta(seconds: number): string {
  const sign = seconds < 0 ? '−' : '+';
  const abs = Math.abs(Math.round(seconds));
  return abs < 60 ? `${sign}${abs}s` : `${sign}${clock(abs)}`;
}

/** Wall-clock time, `21:47`, using the TV's locale. */
export function timeOfDay(epochMs: number): string {
  try {
    return new Date(epochMs).toLocaleTimeString(undefined, { hour: 'numeric', minute: '2-digit' });
  } catch {
    const d = new Date(epochMs);
    return `${d.getHours()}:${String(d.getMinutes()).padStart(2, '0')}`;
  }
}

/** `18.4 GB`, `700 MB`. */
export function bytes(value: number | undefined): string {
  if (!value || value <= 0) return '';
  if (value >= 1e9) return `${(value / 1e9).toFixed(value >= 1e10 ? 0 : 1)} GB`;
  return `${Math.round(value / 1e6)} MB`;
}
