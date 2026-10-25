# Searchers and anime for Bloob (3.1, 25 October 2026)

What the owner asked: why anime searches are weak, how the Magneto Module works with AllDebrid, and whether Bloob can add every top-tier searcher. The evidence below is labelled: **read** (code or pages read), **probed** (a request from the build machine, a datacentre address, on 25 October 2026), **design proposal**.

## Magneto Module and POV (read)

- **POV** (`plugin.video.pov`, by kodifitzwell) is a Kodi video add-on: TMDB/Trakt catalogues plus its own source scrapers. **Magneto Module** (`script.module.magneto`) is a thin wrapper that offers POV's scrapers to other Kodi add-ons (Umbrella and others), and can act as a player for AIOStreams. Both come from `github.com/kodifitzwell/repo` (POV 6.10.22, Magneto 6.10.17).
- **Sources:** POV ships 19 scrapers: Torrentio, TorrentsDB, Comet, MediaFusion, Meteor, Zilean, DMM, Bitmagnet, Knaben, Nyaa, Anime Tosho, The Pirate Bay, TorrentGalaxy, Torlock, TorrentDownload, TorrentFunk, Kickass, Rutor and Torz. They are merged by hash.
- **AllDebrid:** POV calls `/v4/magnet/instant` to check caches. As far as I know (not checked against AllDebrid's documentation) AllDebrid retired that endpoint in 2024, which would explain why POV can also get cache status from AIOStreams (`aio_check_cache`). Bloob's "Ready now" check (add, read `ready`, remove only what it added) answers the same question directly.
- **Anime:** nothing special. Its Nyaa scraper searches "English title + SxxEyy", which misses most anime releases ("[SubsPlease] Sousou no Frieren S2 - 05").
- **Verdict:** Magneto's strength is breadth, comparable to CocoScrapers. It is not anime-aware.

## Why anime was weak in Bloob (read)

1. `rank.js` took 12 points off any version tagged with a language other than the viewer's, so Torrentio's Japanese flag marked down the usual way to watch anime.
2. Every index was asked by IMDb ID and IMDb season/episode, but anime sites split seasons (and cours) into separate entries and releases use the entry's own numbering or absolute numbers.
3. No anime index (Nyaa) was asked.

## What answered without a debrid key (probed)

| Index | Result |
|---|---|
| Knaben API (`POST api.knaben.org/v1`) | 200: hashes, seeders, sizes; carries Nyaa, TPB, 1337x and more |
| Nyaa RSS (`nyaa.si/?page=rss`) | 200: `nyaa:infoHash`, seeders, size |
| SeaDex (`releases.moe/api/collections/entries/records`) | 200: best/alternative releases per AniList ID, Nyaa hashes public (private-tracker hashes redacted), file lists |
| Fribb anime-lists (`anime-list-mini.json`) | 200: IMDb/TVDB/TMDB to Kitsu/AniList with season and offset, refreshed daily by its author |
| Torrentio | 403 from the build machine's datacentre address (Cloudflare), as before; works from the owner's home connection |
| Comet (ElfHosted) | 403 without a configuration |
| MediaFusion (ElfHosted) | 200 with no streams without a configuration |
| Zilean (ElfHosted public) | 404, gone |
| AIOStreams | aggregator needing its own configuration; it would duplicate the indexes above |
| Anime Tosho | stopped taking new torrents in May 2026 (its own notice) |

## Built in 3.1

- `tv-app/service/anime.js`: Fribb index (weekly), AniList titles, episode mapping (cour offsets, absolute numbers for long runners), release matching and pack file picking.
- `sources.js`:
  - **Knaben** for every title;
  - **SeaDex**, **Torrentio by Kitsu ID** and **Nyaa** for anime.
- `rank.js`: for anime, Japanese audio is not marked down; dual audio +2; SeaDex best +8 (leads at equal resolution), SeaDex alternative +2.
- `release.js`: bracketed groups, "BD" as Blu-ray.
- `resolve.js`: anime pack files.

**Design proposals not built:** Comet or MediaFusion with a configuration that sends no debrid key; TorBox as a second cache with Usenet.
