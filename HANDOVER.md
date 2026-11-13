# Bloob: handover for the next developer (13 November 2026)

Read this first, then `CLAUDE.md` (the rules), then the code.
This file is the short version: what Bloob is, how to build and ship it, the
rules, and what is unfinished.

## What Bloob is

A private film and series app for **one owner's rooted LG OLED55G26LA**
(webOS 9.2.2, on the home network at 192.168.1.26). You browse a catalogue
(TMDB and Cinemeta), pick a title, Bloob finds versions from public torrent
indexes, the owner's own **AllDebrid** account turns one into a stream, and
Bloob plays it with its own engine on the TV's hardware decoders.

Current version: **4.2.0**, `native/artifacts/com.muhib.bloob_4.2.0_arm.ipk`.
The owner reports (13 November 2026) that the app works well on the TV:
everything loads and plays. 4.2.0 itself was built and checked on a Linux
desktop only.

## The two halves

| Part | Where | Language | What it does |
| --- | --- | --- | --- |
| The app | `native/` | C++17, SDL2 | The UI (its own renderer), the player, Bloob's engine (FFmpeg demux → LG's elementary-stream pipeline), subtitles, Headphones mode |
| The service | `tv-app/service/` | Node.js (old TV Node: no `?.` or `??`) | Runs on the TV beside the app on `127.0.0.1:8790`: catalogue, search, artwork proxy, source searching and ranking, AllDebrid pairing and resolving, progress, settings, the stream bridge |

The old web version of the app was removed in 4.2; `tv-app/` now holds only
the service and its tests.

### Key files

- `native/src/main.cpp`: app shell, tabs, navigation, service start-up.
- `native/src/browse.cpp` (Home, Films, Series), `details.cpp` (title and
  series page), `sources.cpp` (*Choose how to watch*), `player.cpp`,
  `search.cpp`, `settings.cpp`, `pairing.cpp`.
- `native/src/theme.h`: design tokens; `ui.cpp`: type scale and components;
  `common.cpp`: titles, scrims, facts, backdrop; `gfx.cpp`: renderer.
- `native/src/es_player.cpp`, `demux.cpp`, `starfish.cpp`: Bloob's engine.
  `media.cpp`: LG's own player (the fallback).
- `tv-app/service/server.js` (routes), `sources.js` (every searcher),
  `match.js` (strict title matching), `release.js` (reads release names),
  `rank.js` + `cinephile.js` +
  `tv-profile.js` (ranking and the versions list), `resolve.js` (AllDebrid),
  `bridge.js` (local HTTP for the engine), `progress.js` (settings, watch
  progress).

## Build, test, ship

Everything below runs on a Linux machine. The webOS NDK must be in
`/opt/arm-webos-linux-gnueabi_sdk-buildroot` (openlgtv buildroot-nc4).

```sh
# Service tests (Node 20):
cd tv-app && npm ci && npx vitest run

# Native, desktop build and tests (needs cmake, ninja, SDL2 dev, nasm, ffmpeg):
cd native
scripts/build-ffmpeg-libs.sh host      # once; FFmpeg into build-ffmpeg/
scripts/build-dovi.sh host             # once; Dolby Vision converter (Rust)
cmake -S . -B build-host && cmake --build build-host -j
./build-host/native_tests

# Screenshots of the whole app on the desktop (no TV needed):
scripts/ui-tour.sh /tmp/tour           # PNGs of every screen
scripts/ui-snapshots.sh /tmp/player    # player states

# The TV package (tests, cross-build, ABI checks, IPK):
scripts/build.sh                       # → native/artifacts/com.muhib.bloob_<version>_arm.ipk
```

Raise the version in **both** `native/CMakeLists.txt` and
`native/package/appinfo.json`. CI (`.github/workflows/checks.yml`) runs the
service and native tests on every push.

### Installing on the TV

The owner runs these from Windows. `ares-install -d rootedTV` currently fails
authentication; plain SSH as root works:

```
scp com.muhib.bloob_4.2.0_arm.ipk root@192.168.1.26:/tmp/
ssh root@192.168.1.26 "luna-send -i -f luna://com.webos.appInstallService/dev/install '{\"id\":\"com.muhib.bloob\",\"ipkUrl\":\"/tmp/com.muhib.bloob_4.2.0_arm.ipk\",\"subscribe\":true}'"
```

The app's log on the TV is `/tmp/bloob-native.log`; the live state is
`/tmp/bloob-state.json` (`native/scripts/tv-inspect.sh`).

## Rules (the owner's, all still in force)

1. **Never delete anything on the TV.** No files, packages, accounts or
   caches, not even to tidy up. Installing and updating Bloob is fine.
2. **No secrets anywhere.** The AllDebrid key, TMDB and MDBList keys
   (`keys.json` in `/media/internal/com.muhib.bloob/`, written by the owner),
   the root password and resolved stream URLs never go into Git, logs,
   screenshots or the app package.
3. **Elements of A** is the name for a studied reference app. Its code, art,
   fonts and text are never copied. Its extracted files stay local and are
   ignored by Git.
4. **No ranking penalties the owner didn't ask for** (bitrate, packs, size).
5. **Calibration was removed in 4.1 on request. Don't bring it back.**
6. Label claims honestly: tested on the desktop, seen on the TV, or a
   deduction. Most of 4.x is desktop-only so far.

## The design: Glass (the owner's choice, again, in 4.2)

The owner rejected the 4.1 Swiss/Helvetica redesign and asked for "a nice
seamless minimalistic glass with full bleed art, and very nice snappy menus".
`native/DESIGN.md` describes Glass; the tokens are in `native/src/theme.h`.

- Artwork full screen; frosted-glass panels baked from the blurred artwork
  (`images.cpp`), smoked glass over video; soft shadows; rounded shapes.
- Type: Inter. Titles are each film's or show's own logo.
- Focus: a white pill or card that lifts slightly.
- Motion is quick (4.2): focus 85/140 ms, page and menu fades 160 ms, stiff
  scroll springs, a 180 ms rest before a focused title's art loads.
- Keep it minimal: no long divider lines, no extra labels. A normal,
  available version says nothing about availability; only *Ready now* and
  warnings show.

## Sources: how versions are found

`sources.js` asks Torrentio, Comet, Meteor, Zilean, Bitmagnet, TorrentsDB,
StremThru, Knaben, The Pirate Bay and EZTV, plus SeaDex, Nyaa and Torrentio
by Kitsu ID for anime. Results are merged by hash, then **every** release is
held to the title by `match.js`:

- The title is cut out of the release name (it ends at the year, episode
  number, resolution or a release word), normalised, and must equal the
  title or one of its aliases at Levenshtein similarity ≥ 0.85. Aliases come
  from TMDB (`tmdb.aliases`) and AniList for anime.
- Films must be within a year of their own year; a show's release can't be
  older than the show.
- This is the approach of RTN (used by Comet and Riven) and parse-torrent-title
  (used by Torrentio). It exists because the owner watched *Dark* (2017) and got
  *Dark Matter* and *The Terminal List*. Even Comet returned wrong shows.
- If a correct title goes missing, its releases are using a name TMDB doesn't
  know: add an alias rule rather than loosening the threshold.

Then `rank.js` scores versions (resolution first; HDR10 by default; ready on
AllDebrid is worth just under one resolution step) and `cinephile.js` labels
each one: its source kind, its cut (IMAX, Extended, Director's Cut…), tags
(Hybrid, Criterion…) and release group.

**How the list is shown (4.2, `native/src/sources.cpp`):** the owner found the
old list "nightmarish". Now:

- The best version sits in a card at the top with Play.
- Tabs: All · 4K · 1080p · 720p and below · SDR Remux · Season packs · May not
  play well. The row of tabs slides if it is too wide.
- Under a tab, **one flat list, best first**. No section headers, no folded
  "+17 more like these" rows, no edition tabs. Every release is one row:
  line one is resolution · picture · sound with the size on the right; line
  two is chips (source kind, cut, tags, group, *Season pack*, *Same as your
  last episode*).
- OK plays; Right opens the release details.

## Open work, most important first

1. **See 4.2.0 on the TV** and adjust Glass and the versions list to the
   owner's taste from real screenshots.
2. **One release-name parser.** `release.js`, `match.js`, `sources.js`
   (`fitsEpisode`) and `anime.js` each read release names with their own
   patterns. Merge them into one parser that returns one record per name.
3. **Start-up time for big files**, if the owner still finds it slow: FFmpeg
   is built without TLS, so the whole stream passes through the Node service
   (`bridge.js`). Building FFmpeg with TLS would let the engine read
   AllDebrid directly. Also the fixed waits: 6.5 s search, 3 s Ready check,
   the 20 s engine timeout (`player.cpp` `kEngineFirstFrameMs`).
4. **Big files to split:** `player.cpp` (2,000 lines), `main.cpp` (1,450),
   `server.js` (one 600-line chain of routes).
5. **Catalogue search** only asks Cinemeta; TMDB would add typo tolerance and
   people.
6. **Install route:** `ares-install -d rootedTV` fails to log in; SSH works.

## Where things are

- `CLAUDE.md`: the rules and the owner's taste.
- `native/DESIGN.md`: the Glass design system.
- `native/TV-TEST.md`: the TV test pass for each version.
- `research/`: the studies behind decisions (indexes, Dolby Vision,
  subtitles, headphones, the reference app's design values).
- Git history: every version from 0.2 on, and the long notes that used to be
  in `CLAUDE.md`.
