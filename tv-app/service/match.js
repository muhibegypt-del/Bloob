// Strict title matching for every searcher's results (4.1.1).
//
// The way RTN (rank-torrent-name, used by Comet and Riven) and Torrentio's
// parse-torrent-title do it: cut the title out of the release name (it ends at
// the year, the episode or season number, the resolution or the first release
// word), normalise it, and keep the release only when that title is the show's
// or film's own title or one of its known aliases, almost letter for letter
// (similarity at least 0.85). "Dark Matter S01E05" is not "Dark"; "The
// Terminal List" is not "Dark". Films must also be within a year of their own.
// Keep this file compatible with the TV's older Node runtime (no `?.` / `??`).

const THRESHOLD = 0.85;
// A country after a show's title ("The Office US") is part of the release, not a different title.
const COUNTRY = /\s(?:us|uk|au|nz|ca)$/;

// Where the title ends: a year, SxxEyy / Sxx / "Season n" / NxNN, a resolution,
// or a word no title is made of.
const TITLE_END = new RegExp([
  '\\b(?:19\\d\\d|20[0-3]\\d)\\b',
  '\\bs\\d{1,2}(?:[ -]?e\\d{1,3})?\\b',
  '\\bseasons?[ -]?\\d{1,2}\\b',
  '\\b\\d{1,2}x\\d{2,3}\\b',
  '\\b(?:2160|1080|720|576|480)[pi]\\b',
  '\\b(?:4k|uhd|hdr|hdr10|dv|dovi|remux|bluray|blu ray|bdrip|brrip|web|webdl|web dl|webrip|hdtv|dvdrip|x264|x265|h264|h265|hevc|avc|complete|multi|dual|proper|repack|extended|unrated|imax|criterion|internal|limited)\\b',
].join('|'), 'i');

/** Lower case, no accents, "&" as "and", letters and digits only, single spaces. */
function norm(s) {
  return String(s || '')
    .normalize('NFKD').replace(/[̀-ͯ]/g, '')
    .toLowerCase()
    .replace(/&/g, ' and ')
    .replace(/['’`]/g, '')
    .replace(/[^a-z0-9]+/g, ' ')
    .trim();
}

/** The title part of a release name, normalised. */
function titleOf(release) {
  let s = String(release || '');
  s = s.replace(/^\s*(?:\[[^\]]*\]\s*|\([^)]*\)\s*|【[^】]*】\s*)+/, '');      // leading [Group] tags
  s = s.replace(/^\s*(?:www\.)?[a-z0-9-]+\.(?:com|org|net|to|me|cc|tv|se|is|xyz)\s*[-–:]\s*/i, '');  // "www.site.com - "
  s = s.replace(/[._]+/g, ' ');
  s = s.replace(/\s+-\s+\d{1,4}(?:v\d)?(?:\s|$).*$/, '');            // anime: "Frieren - 05 (1080p)"
  s = norm(s);
  const m = TITLE_END.exec(s);
  // A title may itself be a number or a year ("1917 2019", "2012 2009"): never cut at 0.
  if (m && m.index > 0) s = s.slice(0, m.index);
  else if (m && m.index === 0) {
    const rest = s.slice(m[0].length);
    const n = TITLE_END.exec(rest);
    s = m[0] + (n ? rest.slice(0, n.index) : rest);
  }
  return s.trim().replace(COUNTRY, '').trim();
}

/** Levenshtein similarity, 0..1, as RTN's title_match uses. */
function similarity(a, b) {
  if (a === b) return 1;
  if (!a || !b) return 0;
  const m = a.length, n = b.length;
  let prev = new Array(n + 1);
  let cur = new Array(n + 1);
  for (let j = 0; j <= n; j++) prev[j] = j;
  for (let i = 1; i <= m; i++) {
    cur[0] = i;
    for (let j = 1; j <= n; j++) {
      cur[j] = Math.min(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + (a[i - 1] === b[j - 1] ? 0 : 1));
    }
    const t = prev; prev = cur; cur = t;
  }
  return 1 - prev[n] / Math.max(m, n);
}

/** The forms a wanted title may take: as is, without "the", without a trailing year. */
function forms(title) {
  const t = norm(title).replace(COUNTRY, '').trim();
  const out = [t];
  const noYear = t.replace(/\s(?:19|20)\d\d$/, '').trim();
  if (noYear && noYear !== t) out.push(noYear);
  out.slice().forEach(x => { if (/^the /.test(x)) out.push(x.slice(4)); else out.push('the ' + x); });
  return out.filter(Boolean);
}

/** Whether a release's title is one of the wanted titles. */
function titleMatches(release, wanted) {
  const got = titleOf(release);
  if (!got) return false;
  for (const w of wanted || []) {
    for (const f of forms(w)) if (similarity(got, f) >= THRESHOLD) return true;
  }
  return false;
}

/** The release's year, if its name has one after the title. */
function yearOf(release) {
  const s = norm(String(release || '').replace(/[._]+/g, ' '));
  const all = s.match(/\b(?:19\d\d|20[0-3]\d)\b/g) || [];
  // The last year is the release's; an earlier one may be part of the title ("2001 A Space Odyssey 1968").
  return all.length ? Number(all[all.length - 1]) : 0;
}

/**
 * Keep the candidates whose release (or, for a pack, its file) names the wanted
 * title; films also within a year of their own. `expect` is { titles, year, type };
 * without titles nothing is dropped (the catalogue could not be read).
 */
function strict(candidates, expect) {
  if (!expect || !expect.titles || !expect.titles.length) return candidates;
  return candidates.filter(c => {
    const names = [c.release, c.fileName].filter(Boolean);
    const named = names.some(n => titleMatches(n, expect.titles));
    if (!named) return false;
    if (expect.type === 'movie' && expect.year) {
      const y = yearOf(c.release);
      if (y && Math.abs(y - expect.year) > 1) return false;
    }
    if (expect.type === 'series' && expect.year) {
      // A show's release can't be older than the show.
      const y = yearOf(c.release);
      if (y && y < expect.year - 1) return false;
    }
    return true;
  });
}

module.exports = { norm, titleOf, similarity, titleMatches, yearOf, strict, THRESHOLD };
