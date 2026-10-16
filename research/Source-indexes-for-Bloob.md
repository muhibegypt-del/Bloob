# Torrent source discovery for an AllDebrid app: CocoScrapers, Stremio alternatives, and directly queryable APIs (checked 16 Oct 2026)

> Research note, 16 October 2026 (static and live web evidence, not a TV measurement). Acted on in 2.5.0: `tv-app/service/sources.js` merges Torrentio, TorrentsDB, StremThru, The Pirate Bay and EZTV; Knaben (text search) is not yet added.

**How this was checked.** I read the source of CocoScrapers 1.0.32, Torrentio, Comet and StremThru, cloned or unzipped from their public repositories. I also made live unauthenticated requests on 16 Oct 2026 from this cloud container, which uses a datacenter IP behind a proxy.
- Results from the TV's home IP may differ, especially for anything behind Cloudflare.
- No account, key or login was used.
- Each point below is marked **[verified]** (code read or request made), **[doc]** (taken from a project's own documentation) or **[inference]**.
- Legal note: most of these indexes list unauthorised copies. The project's own rule applies ("use only media the user is authorized to access"), and some indexers' terms or regional blocks may apply.

## 1. What CocoScrapers is

- **What it is.** CocoScrapers is a Kodi Python *module*, `script.module.cocoscrapers`, published as zips in https://github.com/CocoJoe2411/repository.cocoscrapers. The newest zip is 1.0.32, and its files are dated 2025-03-24 [verified]. Umbrella and similar addons call it; the module is not an addon you use on its own.
- **Providers in 1.0.32** (`lib/cocoscrapers/sources_cocoscrapers/torrents/`) [verified]:
  - 1337x (1337x.to, HTML)
  - bitcq, bitlord, bitsearch.to (HTML)
  - **comet** (comet.elfhosted.com)
  - eztv (eztvx.to)
  - isohunt2, kickass2
  - knaben (knaben.eu, HTML search page)
  - **mediafusion** (mediafusion.elfhosted.com)
  - nyaa
  - **piratebay** (apibay.org JSON)
  - torrentdownload, torrentfunk
  - torrentgalaxy (tgx.rs)
  - **torrentio**
  - torrentproject2, torrentquest (a magnetdl mirror), yourbittorrent
  - ytsmx (yts.mx JSON)

  A June 2026 community fork also has `newznab.py` and `torznab.py` (https://github.com/delulu710/script.module.cocoscrapers).
- **How it searches.** Most modules search by **title text**: the cleaned title plus the year, or plus `SxxEyy`. For series they also send pack queries: "Title S01", "Title Season 1", "Title Complete". Results are then filtered against title, aliases and year [verified, e.g. `knaben.py`, `bitsearch.py`]. Only a few modules use the IMDb ID: torrentio, comet, mediafusion, yts, and torrentgalaxy for films.
- **It is a superset of Torrentio.** It calls Torrentio's `/stream/{type}/{id}.json` itself, then adds about 18 title-search sites and two other Stremio indexes [verified].
- **Cache checking.** CocoScrapers does none: the module contains no instant-availability call. The host addon (Umbrella) checks debrid cache [verified by grep; Umbrella's own code not inspected].
- **Privacy problem.** The Comet module sends `debridService` and `debridApiKey`, base64-encoded in the URL path, to comet.elfhosted.com (`comet.py` lines 51–53) [verified]. That is exactly what Bloob forbids.
- **Why it finds more and rarer releases [inference]:**
  1. A text search reaches torrents that IMDb-keyed databases never matched to an ID: odd naming, packs, foreign or obscure groups.
  2. It queries search pages live, so it sees new uploads at once, where Torrentio only has what its scraper has already ingested.
  3. It merges many overlapping indexes and de-duplicates by hash.
  4. Pack searches surface season and complete-series torrents.

  The cost is speed: about 20 parallel HTTP requests with 5–10 s timeouts. Fragility is the other cost: domains die and Cloudflare blocks scrapers. The code's own comments say "all are behind cloudflare except .to", and two sites are noted as broken.

## 2. The Stremio and debrid ecosystem compared

| Service | Hosting | Debrid key sent to it? | File index / name | AllDebrid | Notes |
|---|---|---|---|---|---|
| **Torrentio** | Public: torrentio.strem.fun. Code at https://github.com/TheBeastLT/torrentio-scraper (last commit 16 Oct 2026) | Only if debrid is configured: the key is embedded in its playback URLs (`addon/moch/*.js`) | Yes: `infoHash`, `fileIdx`, `behaviorHints.filename`, `bingeGroup` | Yes | Providers: YTS, EZTV, RARBG (legacy dump), 1337x, TPB, KAT, TGx, MagnetDL, Nyaa, Rutor, Rutracker, several Latin/Italian/Spanish sites and others (`addon/lib/filter.js`). It is a pre-scraped database keyed by IMDb ID, so it is fast. **Here it returned Cloudflare 403 "Sorry, you have been blocked" from a datacenter IP** [verified]. AllDebrid "cached" status comes from its own crowdsourced database (`getMochCachedAvailabilityResults`) [verified] |
| **TorrentsDB** (torrentsdb.com) | Public | Not needed unless configured | Yes, in the same format as Torrentio | Listed | Manifest lists YTS, EZTV, 1337x, TorrentCSV, Knaben, TPB, KAT, Rutracker, YggTorrent and others. Unconfigured `/stream/movie/tt0133093.json` returned **91 streams** with `infoHash`, `fileIdx` and `filename` [verified]. Effectively a drop-in Torrentio alternative |
| **Comet** (g0ldyy/comet; last commit 29 Jul 2026) | Self-host. ElfHosted runs a public instance, but its docs say AllDebrid is not offered there | Yes: the config is base64 JSON in the path, carrying `debridService` and `debridApiKey` | Yes | Self-host only | Draws from Jackett, Prowlarr, Torrentio, Zilean, MediaFusion, StremThru, AIOStreams, TorBox, Nyaa, BitMagnet, TorrentsDB, Peerflix, a DMM-hashlist ingester and SeaDex [verified README and `comet/scrapers/`]. All debrid calls, including the cache check, go through StremThru (`comet/debrid/stremthru.py`). The unconfigured ElfHosted request returned 403 |
| **MediaFusion** (mhdzumair/MediaFusion; ElfHosted instance at v6.1.6) | Public and self-host | Credentials go inside an AES-encrypted manifest config | Yes (`infoHash`) | Yes | Sources: Prowlarr, Jackett, public indexers, Torrentio, Zilean, RSS. Unconfigured requests returned `{"streams":[]}`, so it needs a config [verified] |
| **StremThru** (MunifTanjim/stremthru; last commit 2 Oct 2026) | Public: stremthru.13377001.xyz. Self-host | Store and cache-check calls need the key. **The Torznab search does not** | Hash, title and parsed attributes. No file index | Yes (store adapter `store/alldebrid`) | Its "Torz" store is a crowdsourced hash database. The `dmm_hashlist` feature is on by default and imports DMM hashlists. See §3 for the request count |
| **Zilean** (iPromKnight/zilean) | Self-host. Comet's sample config points to a community instance | No | Hash and raw title | n/a, search only | Indexes DMM-shared hashlists. Offers `/dmm/filtered` (used by Comet) and Torznab. Its docs list `/imdb/search` |
| **DMM hashlists** | https://github.com/debridmediamanager/hashlists (about 15k HTML files, 15k+ commits) | No | Hash, filename, size | n/a | Libraries shared by Real-Debrid and AllDebrid users. **Heavy on remux and 4K material.** Comet and StremThru download the zipball |
| **Jackettio** (arvida42/jackettio; ElfHosted v1.7.0) | Self-host, plus ElfHosted | Yes | Yes | Yes | Needs a Jackett instance. Its strength is private trackers |
| **AIOStreams** (Viren070/AIOStreams) | Self-host or community instances | Yes, stored in a password-protected config on the instance | Passes through what each source returns | Yes | An aggregator: wraps other addons and has built-in Knaben, Torznab, Zilean, Bitmagnet, TGx, Prowlarr, EZTV and TorBox Search. De-duplicates by infohash, filename or "smart" key [doc] |
| **KnightCrawler** | **Retired.** "Development ceased in 2024 and the ElfHosted public instance was retired" (https://docs.elfhosted.com/app/knightcrawler/) | – | – | – | Do not use |
| **TorBox Search API** | Hosted at `https://search-api.torbox.app/torrents/imdb:{id}` | **Needs a TorBox API key** (`Authorization: Bearer`, per Comet `scrapers/torbox.py`) | – | Not applicable | Unusable without a TorBox account. Unreachable from here (502) |
| **Orion** (orionoid.com) | Hosted | Needs an app key and a user API key. The free tier is limited to "a few links … per day" | – | Lists AllDebrid cache lookups | Account-gated and rate-limited [doc] |
| **Prowlarr / Jackett** | Self-host | No | Torznab output | n/a | The most complete option, including private trackers, but needs a server. Bloob runs its service on the TV, where this is impractical [inference] |

## 3. Public JSON sources a device can query without a debrid key

All probes were made from this container on 16 Oct 2026.

- **StremThru Torznab** (JSON, keyed by IMDb ID) [verified]: `GET https://stremthru.13377001.xyz/v0/torznab/api?t=movie&imdbid=tt0133093&o=json`
  - For an episode: `t=tvsearch&imdbid=tt0944947&season=1&ep=10&o=json`. Caps: `t=caps`.
  - Response: `channel.items[]` with `title`, `guid`, a magnet `link`, and `attr[]` name/value pairs: `infohash`, `imdb`, `resolution`, `size`, `team`, `video`, `audio`, `language`, sometimes `seeders` and `indexername`.
  - **The Matrix returned 965 hashes.** They include multiple 2160p remuxes, DV/HDR and TrueHD Atmos releases, and many foreign ones. The Game of Thrones S01E10 query returned 552, mostly season and series packs.
  - Comet's own StremThru scraper uses this endpoint (`comet/scrapers/stremthru.py`).
  - The ElfHosted copy returns 403. The public instance has config limits on indexers and stores, and no published SLA.
- **apibay.org (The Pirate Bay)** (JSON) [verified]:
  - Search: `GET https://apibay.org/q.php?q=tt0133093&cat=0` **accepts an IMDb ID** as `q`. It returns at most 100 results with `name, info_hash, seeders, leechers, size, num_files, imdb, added`.
  - Empty results come back as a placeholder with name "No results returned" and hash `000…`.
  - File list: `GET https://apibay.org/f.php?id={id}` returns `[{name:[..], size:[..]}]`.
- **EZTV** (JSON, IMDb number without `tt`) [verified]:
  - Request: `GET https://eztvx.to/api/get-torrents?imdb_id=0944947&limit=100&page=1`.
  - Fields: `torrents[]` with `hash, filename, title, season, episode, seeds, peers, size_bytes, magnet_url`, plus `torrents_count`.
- **Knaben** (JSON meta-index of TPB, 1337x, RuTracker, Nyaa, YTS and others; **text search only**):
  - v1 [verified]: `POST https://api.knaben.org/v1` with a JSON body `{search_type, search_field:"title", query, size, order_by, order_direction, hide_xxx}`. Hits carry `title, hash, magnetUrl, bytes, seeders, peers, tracker, date`. "The Matrix 1999 2160p" returned 22 hits, including RuTracker DV, TPB remuxes and Atmos releases.
  - v2 (released 2026-10-01, which Knaben asks clients to move to): `GET https://api.knaben.org/v2/search?q=…&o=seeders&s=150&t=…&c=…` [doc, https://knaben.org/api/v2/].
  - The page says scrapers are not blocked as a policy, but heavy load is unwelcome.
- **TorrentsDB** (Stremio JSON, IMDb): `GET https://torrentsdb.com/stream/{movie|series}/{tt…[:s:e]}.json`, in the same shape as Torrentio [verified].
- **YTS**: `https://yts.mx/api/v2/list_movies.json?query_term=tt…` accepts an IMDb ID (comment in CocoScrapers `ytsmx.py`). From here the request **failed** (502 at the proxy; yts.lt redirects), so its status is unconfirmed.
- **HTML scraping only**: bitsearch.to (520 from here), 1337x (Cloudflare except `.to`), tgx.rs (connection reset from here; CocoScrapers searches it by IMDb ID), torrentproject2, yourbittorrent, torrentdownload, knaben.eu's web page (timed out). All are brittle.
- **Not suitable without a key or server**: Comet (needs a config), MediaFusion (needs a config), TorBox Search (TorBox key), Orion (account), Zilean (self-host).

## 4. AllDebrid cache checking

- **The endpoint is gone from the documentation.** The current docs (https://docs.alldebrid.com/) no longer list `/magnet/instant` at all.
- **When it went is not documented.** The changelog does not mention the removal. It records only that `/v4/magnet/status` was deprecated for `/v4.1` on 16/10/2024, with file lists moved to `/magnet/files`. I found **no official dated announcement**.
- **Community sources say it was removed.** Viren070's guide states that "Real Debrid, AllDebrid, and Debrid-Link have removed the API endpoint that allows addons to determine if a torrent is cached" (https://guides.viren070.me/stremio/setup).
- **How addons decide "cached" now:**
  - **Torrentio and MediaFusion** keep their own crowdsourced databases [Torrentio verified in code].
  - **StremThru** keeps a shared `magnet_cache` database (`internal/buddy/store.go` `CheckMagnet`). It can also ask an upstream peer, and Comet relies on it [verified].
  - **DMM hashlists** act as a proxy signal.
  - **Upload and read the status.** The only first-party, accurate method is to call `/v4/magnet/upload`, which returns `ready` per magnet, or `/v4.1/magnet/status`, then delete the magnet if it is not used.
- **Rate limits** are 12 requests/s and 600/min. The docs mention a cap of 30 active magnets [doc]. I could not confirm whether that cap applies to instantly-ready magnets.
- **Datacenter IPs.** One project reports that AllDebrid rejects `link/unlock` from datacenter IPs with `NO_SERVER` (https://github.com/NepiRaw/Stremio-IntellDebridSearch). Resolving from the TV, as Bloob does, avoids this.

## 5. Recommendation

**There is no single service that is clearly better than Torrentio and also respects the keep-the-key-local rule.**
- What made CocoScrapers feel "insane" was breadth plus live text search: Torrentio plus about 18 sites plus Comet and MediaFusion.
- The richest key-free index found is **StremThru's public Torznab**. Searched by IMDb ID it returned about 10 times TorrentsDB's count for the same film, including DMM-sourced remux, DV and Atmos hashes.

**Proposed architecture [inference]:** a CocoScrapers-style merge, done narrowly in the service.
1. Add a provider interface to `tv-app/service`. Send parallel requests with a hard per-provider timeout of about 4–6 s, and stream results to the UI as they arrive.
2. **Tier 1, IMDb-keyed JSON with no key:** Torrentio (it works from the home IP; it did not from here), StremThru Torznab, TorrentsDB, apibay (`q=tt…`), and EZTV for series.
3. **Tier 2, text JSON:** Knaben v2 with "Title Year" and "Title SxxEyy", plus pack queries, validated against title and year as CocoScrapers does.
4. Skip HTML scrapers at first.
5. De-duplicate by lowercase infohash. Keep the richest metadata per hash: the release name from one source and seeders from another. Record `claimsFrom` per source.
6. **Cache status:** use AllDebrid's own upload and status flow only on the shortlisted top N candidates, never all of them. Treat third-party "cached" flags as hints only.

**File selection.** Torrentio and TorrentsDB give `fileIdx` and `filename`. Torznab, apibay and Knaben do not; apibay `f.php` gives a file list. Bloob's resolver already picks files after AllDebrid's `/magnet/files`, so this is acceptable.

**Trade-offs:**
- More requests per title, and a slower worst case.
- Community instances can disappear or rate-limit; StremThru's public instance has no SLA.
- Cloudflare blocks shift over time: Torrentio blocked this datacenter today.
- Legality and indexer terms remain the owner's responsibility.
- Self-hosting Comet, AIOStreams or Prowlarr gives the most coverage, but it needs a server and, for Comet and AIOStreams, the debrid key, which conflicts with Bloob's privacy rule.

## Key sources

- CocoScrapers: https://github.com/CocoJoe2411/repository.cocoscrapers
- Torrentio: https://github.com/TheBeastLT/torrentio-scraper
- Comet: https://github.com/g0ldyy/comet
- StremThru: https://github.com/MunifTanjim/stremthru (docs/api/torz.md)
- MediaFusion: https://github.com/mhdzumair/MediaFusion
- AIOStreams: https://github.com/Viren070/AIOStreams
- Jackettio: https://github.com/arvida42/jackettio
- Zilean: https://github.com/iPromKnight/zilean
- DMM hashlists: https://github.com/debridmediamanager/hashlists
- KnightCrawler retirement: https://docs.elfhosted.com/app/knightcrawler/
- AllDebrid API: https://docs.alldebrid.com/
- Debrid cache-check changes: https://guides.viren070.me/stremio/setup
- Knaben API v2: https://knaben.org/api/v2/
- Orion: https://orionoid.com/
- TorBox Search usage in Comet: https://github.com/g0ldyy/comet/issues/331
