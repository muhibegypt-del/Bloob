// When a viewing counts as watched (UX-PLAN X1). This is Bloob's own policy,
// not a copy of a reference: semantic timing first, then one fallback, and an
// unknown runtime never completes. Tune the fallback from TV observations.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).

const RESUME_MIN_MS = 30000;          // shorter than this: nothing worth resuming
const SHORT_MS = 120000;              // clips: watched only at their very end
const SHORT_END_MS = 5000;
const SHARE = 0.07;                   // fallback: the last 7% of a title...
const MIN_REMAINING_MS = 60000;       // ...but at least the last minute
const MAX_REMAINING_MS = 600000;      // ...and at most the last ten

/** How much may remain while a title still counts as watched (fallback only). */
function remainingAllowance(durationMs) {
  return Math.min(MAX_REMAINING_MS, Math.max(MIN_REMAINING_MS, durationMs * SHARE));
}

/**
 * Whether a report means the viewer finished the title.
 * report: { position, duration, state, creditsMs } (ms; state from the player).
 *  - state 'upnext': the player is showing its Up next countdown, which it does
 *    only at the end of the title. Preparing the next episode is not a state.
 *  - state 'ended': the stream reached its end.
 *  - creditsMs: where the credits start, when the file or metadata says so.
 */
function isComplete(report) {
  const state = report && report.state;
  if (state === 'upnext' || state === 'ended') return true;
  const position = Math.max(0, Number(report && report.position) || 0);
  const duration = Math.max(0, Number(report && report.duration) || 0);
  if (!(duration > 0)) return false;
  const credits = Number(report.creditsMs) || 0;
  if (credits > 0 && credits < duration && position >= credits) return true;
  if (duration < SHORT_MS) return position >= duration - SHORT_END_MS;
  return duration - position <= remainingAllowance(duration);
}

/** Where to continue from, or 0 to start at the beginning. */
function resumePoint(entry) {
  if (!entry || entry.watched) return 0;
  const position = Number(entry.position) || 0;
  return position >= RESUME_MIN_MS ? position : 0;
}

module.exports = { isComplete, resumePoint, remainingAllowance, RESUME_MIN_MS };
