# Bloob: handover for the next developer (3 November 2026)

Read this first, then `CLAUDE.md` (the long project history), then the code.
This file is the short version: what Bloob is, how to build and ship it, the
rules, and what is unfinished.

## What Bloob is

A private film and series app for **one owner's rooted LG OLED55G26LA**
(webOS 9.2.2, on the home network at 192.168.1.26). You browse a catalogue
(TMDB and Cinemeta), pick a title, Bloob finds versions from public torrent
indexes, the owner's own **AllDebrid** account turns one into a stream, and
Bloob plays it with its own engine on the TV's hardware decoders.

Current version: **4.1.1**, `native/artifacts/com.muhib.bloob_4.1.1_arm.ipk`.
It was built and tested on a Linux desktop; **4.x has not yet been confirmed
on the TV.**

## The two halves

| Part | Where | Language | What it does |
| --- | --- | --- | --- |
| The app | `native/` | C++17, SDL2 | The UI (its own renderer), the player, Bloob's engine (FFmpeg demux → LG's elementary-stream pipeline), subtitles, Headphones mode |
| The service | `tv-app/service/` | Node.js (old TV Node: no `?.` or `??`) | Runs on the TV beside the app on `127.0.0.1:8790`: catalogue, search, artwork proxy, source searching and ranking, AllDebrid pairing and resolving, progress, settings, the stream bridge |

`tv-app/src/` is an older web version of the app (SolidJS). It still builds
but the native app replaced it; leave it alone unless asked.

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
  `match.js` (strict title matching), `rank.js` + `cinephile.js` +
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
scp com.muhib.bloob_4.1.1_arm.ipk root@192.168.1.26:/tmp/
ssh root@192.168.1.26 "luna-send -i -f luna://com.webos.appInstallService/dev/install '{\"id\":\"com.muhib.bloob\",\"ipkUrl\":\"/tmp/com.muhib.bloob_4.1.1_arm.ipk\",\"subscribe\":true}'"
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

## The design (4.1 Swiss full bleed: the owner approved it, keep to it)

The concept boards are on the canvas
https://claude.ai/artifact/3iCgPmqSJMT1TDquSQsXcS (the rows titled
"Swiss full bleed"). In the app:

- Artwork full screen, black page ground (OLED), dark washes only where text sits.
- Type: **TeX Gyre Heros** (free, drawn after Helvetica), Regular and Bold.
  Titles are each film's or show's **own logo**; set in Bold type only when
  there is no logo.
- A 12-column grid: 80 px margins, 24 px gutters (`theme::col_x`, `span_w`).
- One colour: signal red `#FF3B21` (`theme::signal`). **Always write
  `theme::signal`**: the TV's system headers define `signal`, so a bare
  `signal` breaks the TV build.
- Square shapes only (`Gfx::square`), no shadows, no glass, no spinners
  (a red bar slides along instead).
- Focus is a solid white block with black type, or a white frame around art.
- **No long horizontal lines or row dividers.** The owner called them
  "disgusting" in 4.1.0; they were removed in 4.1.1. Use spacing.
- Facts as small grey labels over values (`draw_facts`); key numbers big.

## Sources: how versions are found (4.1.1)

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
AllDebrid is worth just under one resolution step). `cinephile.js` sorts them
into sections: UHD Blu-ray Remux, **SDR Remux**, Blu-ray Remux, 4K disc
encodes, 4K web, Blu-ray encodes, HD web, **Season packs**, Everything else.
The app shows SDR Remux and Season packs as their own tabs too.

## Open work, most important first

1. **Confirm 4.1.1 on the TV.** Install it, browse, play a 4K remux and a web
   file, and check the versions list for a show and a film.
2. **Slow 4K starts.** Fixed so far, by deduction: a kept-alive connection in
   `bridge.js`, and a preference for versions already ready on AllDebrid.
   Still suspected, unmeasured:
   - the 20 s engine timeout before LG's player takes over (`player.cpp`
     `kEngineFirstFrameMs`);
   - the audio bridge for DTS- and TrueHD-only remuxes.

   Ask the owner for
   `grep -E "engine|first frame|gave up|bridge" /tmp/bloob-native.log | tail -30`
   after a slow start.
3. **Unfinished bits of the Swiss design:**
   - *Choose how to watch*: its filter tabs should be a vertical list in the
     left column, as on the board.
   - Settings: it should have the board's section list (Playback, Sound,
     Subtitles, Sources, The TV, About).
   - Up next: its new layout hasn't been seen running.
4. **Install route:** fix the `rootedTV` ares profile (it needs the owner's
   SSH key), or keep using SSH.
5. **Older plans still open** (see `CLAUDE.md`):
   - track memory;
   - edition-aware resume;
   - title-page cast, collection and similar rows;
   - Trakt;
   - the reliability plan `native/RELIABILITY-IMPLEMENTATION-PLAN.md`.

## Where the history is

- `CLAUDE.md`: every version from 0.2 to 4.1.1, what changed, why, and what
  was or wasn't seen on the TV.
- `native/TV-TEST.md`: the TV test pass for each version.
- `native/APP-MAP.md`, `native/QUALITY.md`: the app's layers and quality scores.
- `research/`: the studies behind decisions (indexes, Dolby Vision, subtitles,
  headphones, webOS rendering).
