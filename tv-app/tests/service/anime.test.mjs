import { describe, expect, it } from 'vitest';
import animeModule from '../../service/anime.js';
import releaseModule from '../../service/release.js';
import rankModule from '../../service/rank.js';
import resolveModule from '../../service/resolve.js';

const { entryFor, matchEpisode, episodeFile, buildIndex } = animeModule;
const { parseStream } = releaseModule;
const { rank } = rankModule;
const { pickFile } = resolveModule;

// Frieren's three seasons as Fribb's list maps them (TVDB seasons; TMDB keeps one season with offsets).
const frieren = buildIndex([
  { type: 'TV', anilist_id: 154587, kitsu_id: 46474, imdb_id: ['tt22248376'], season: { tvdb: 1, tmdb: 1 } },
  { type: 'TV', anilist_id: 182255, kitsu_id: 49240, imdb_id: ['tt22248376'], season: { tvdb: 2, tmdb: 1 }, episode_offset: { tmdb: 28 } },
  { type: 'TV', anilist_id: 209939, kitsu_id: 50629, imdb_id: ['tt22248376'], season: { tvdb: 3, tmdb: 1 }, episode_offset: { tmdb: 38 } },
]).byImdb.tt22248376;
const seasons = [{ season: 1, episodes: new Array(28).fill(0) }, { season: 2, episodes: new Array(10).fill(0) }];

const ctx = {
  season: 2, catalogEpisode: 5, episode: 5, absolute: 33,
  titles: ['Sousou no Frieren 2nd Season'], showTitles: ['Sousou no Frieren', "Frieren: Beyond Journey's End"],
};

describe('anime episodes', () => {
  it('maps a catalogue episode to its anime entry, with the absolute number', () => {
    const found = entryFor(frieren, 'series', 2, 5, seasons);
    expect(found.entry.anilist).toBe(182255);
    expect(found.entry.kitsu).toBe(49240);
    expect(found.episode).toBe(5);
    expect(found.absolute).toBe(33);
    const onePiece = buildIndex([{ type: 'TV', anilist_id: 21, kitsu_id: 12, imdb_id: ['tt0388629'] }]).byImdb.tt0388629;
    const op = entryFor(onePiece, 'series', 2, 3, [{ season: 1, episodes: new Array(61).fill(0) }]);
    expect(op.episode).toBe(64);  // long runners go by absolute number
  });

  it('recognises how anime releases name an episode, and rejects other seasons', () => {
    expect(matchEpisode('[SubsPlease] Sousou no Frieren S2 - 05 (1080p) [6AAEC79A].mkv', ctx)).toBe('episode');
    expect(matchEpisode('[Erai-raws] Sousou no Frieren 2nd Season - 05 [1080p CR WEB-DL AVC AAC][MultiSub]', ctx)).toBe('episode');
    expect(matchEpisode("[Yameii] Frieren: Beyond Journey's End - S02E05 [English Dub] [CR WEB-DL 1080p]", ctx)).toBe('episode');
    expect(matchEpisode('[SubsPlease] Sousou no Frieren - 33 (1080p)', ctx)).toBe('episode');  // absolute
    expect(matchEpisode('[SubsPlease] Sousou no Frieren - 05 (1080p)', ctx)).toBe('');  // season 1's
    expect(matchEpisode('[Raze] Sousou no Frieren S2 - 06 x265 10bit 1080p', ctx)).toBe('');
    expect(matchEpisode('[Judas] Sousou no Frieren S2 (01-10) [1080p][Batch]', ctx)).toBe('batch');
    expect(matchEpisode('Some Other Show S2 - 05 (1080p)', ctx)).toBe('');
  });

  it('finds the episode inside an anime pack', () => {
    const files = [{ name: 'Frieren - S02E04.mkv', size: 9 }, { name: '[G] Sousou no Frieren S2 - 05 [1080p].mkv', size: 2 }, { name: 'NCOP.mkv', size: 1 }];
    expect(episodeFile(files, ctx).name).toBe('[G] Sousou no Frieren S2 - 05 [1080p].mkv');
    expect(pickFile(files.concat([{ name: 'Extras/Menu.mkv', size: 1 }]), { season: 2, episode: 5, anime: { season: 2, catalogEpisode: 5, episode: 5, absolute: 33 } }).name)
      .toBe('[G] Sousou no Frieren S2 - 05 [1080p].mkv');
  });
});

describe('anime versions', () => {
  const c = (hash, title, extra) => Object.assign(parseStream({ infoHash: hash.repeat(40), title: `${title}\n👤 200` }, 'nyaa'), extra || {});
  it('reads the bracketed group and BD', () => {
    const v = c('a', '[kikuri] Sousou no Frieren 2nd Season - 05 (BD 1080p HEVC Opus) [kikuri].mkv');
    expect(v.group).toBe('kikuri');
    expect(v.sourceType).toBe('BluRay');
    expect(c('b', '[SubsPlease] Sousou no Frieren S2 - 05 (1080p) [6AAEC79A].mkv').group).toBe('SubsPlease');
  });

  it('does not mark Japanese audio down for anime, and leads with SeaDex best', () => {
    const jp = c('c', '[Group] Sousou no Frieren S2 - 05 (1080p)', { languages: ['Japanese'] });
    const best = c('d', '[kikuri] Sousou no Frieren 2nd Season - 05 (BD 1080p HEVC Opus)', { seadex: 'best' });
    const plain = rank([jp], { language: 'English', type: 'series' }).recommended;
    const animeView = rank([jp], { language: 'English', type: 'series', anime: true }).recommended;
    expect(animeView.score).toBeGreaterThan(plain.score);
    expect(animeView.warnings.join(' ')).not.toMatch(/Japanese/);
    const ranked = rank([jp, best], { language: 'English', type: 'series', anime: true });
    expect(ranked.recommended.id).toBe(best.id);
    expect(ranked.recommended.tags).toContain('SeaDex best');
  });
});
