// The next action for a title (UX-PLAN X2): what Play on the title page, the
// Home hero and Continue should do, decided once, here, from the catalogue and
// the viewer's progress. The app renders it; it does not work it out again.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).
const completion = require('./completion');

const aired = (episode, now) => !episode.released || !(Date.parse(episode.released) > now);

function describe(action, episode, extra) {
  return Object.assign({
    action,
    videoId: episode.id,
    season: episode.season,
    episode: episode.episode,
    title: episode.title || '',
    released: episode.released || '',
  }, extra || {});
}

/**
 * meta: catalogue meta ({ type, id, seasons: [{ season, episodes: [...] }] }).
 * progress: { videoId: { position, duration, watched, updated } } for this title.
 * Returns one of:
 *   { action: 'resume', videoId, position, ... }   unfinished viewing
 *   { action: 'next', ... }                         the episode after the furthest one finished
 *   { action: 'start', ... }                        nothing watched yet
 *   { action: 'caughtUp', upcoming: {...} | null, again: {...} | null }
 *   { action: 'play' } / { action: 'again' }        films
 */
function target(meta, progress, now) {
  now = now || Date.now();
  progress = progress || {};
  if (!meta) return null;
  if (meta.type !== 'series') {
    const entry = progress[meta.id];
    const position = completion.resumePoint(entry);
    if (position > 0) return { action: 'resume', videoId: meta.id, position };
    return { action: entry && entry.watched ? 'again' : 'play', videoId: meta.id };
  }
  const episodes = [];
  (meta.seasons || []).forEach(s => {
    if (s.season > 0) (s.episodes || []).forEach(e => episodes.push(e));
  });
  if (!episodes.length) return null;
  const first = episodes.find(e => aired(e, now)) || null;

  // The most recent viewing decides when it was left unfinished.
  let latest = null;
  episodes.forEach(e => {
    const p = progress[e.id];
    if (p && (!latest || p.updated > progress[latest.id].updated)) latest = e;
  });
  if (latest) {
    const position = completion.resumePoint(progress[latest.id]);
    if (position > 0) return describe('resume', latest, { position });
  }

  // Otherwise the episode after the furthest one finished.
  let furthest = -1;
  episodes.forEach((e, i) => { if (progress[e.id] && progress[e.id].watched) furthest = i; });
  if (furthest < 0) return first ? describe('start', first) : { action: 'caughtUp', upcoming: describeUpcoming(episodes[0]), again: null };
  const following = episodes[furthest + 1];
  if (!following) return { action: 'caughtUp', upcoming: null, again: first ? describe('start', first) : null };
  if (aired(following, now)) return describe('next', following);
  return { action: 'caughtUp', upcoming: describeUpcoming(following), again: first ? describe('start', first) : null };
}

function describeUpcoming(episode) {
  return episode ? describe('upcoming', episode) : null;
}

module.exports = { target };
