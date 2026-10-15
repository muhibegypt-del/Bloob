# Bloob native 2.0: Glass (Buna N2)

A native webOS app (`com.muhib.bloob`, "Bloob Native") written in C++ against the community webOS NDK. It is the whole app: catalogue, search, details with seasons and episodes, source choice, AllDebrid pairing and resolution, and playback through the TV's own media pipeline. It draws its own UI and owns playback with the native Buna session. The map of every layer of the app, with its current state, gaps and next increments, is in [`APP-MAP.md`](APP-MAP.md). The visual system (Glass since 2.0), with screen layouts and states, is in [`DESIGN.md`](DESIGN.md); how Glass was built, phase by phase, is in [`GLASS-PLAN.md`](GLASS-PLAN.md). The TV test script, including the hand-the-remote-over acceptance task, is in [`TV-TEST.md`](TV-TEST.md).

It installs alongside the web app (`com.muhib.tvprototype`) and does not replace it.

## How it fits together

```
bloob (C++)                                   com.muhib.bloob.service (Node, bundled)
  SDL window + remote  ──────────────┐          catalogue/search/details   ← Cinemeta
  screens + renderer (Inter)         │ HTTP     artwork proxy + cache       ← metahub / TMDB images
  native Buna session ──┐            ├────────► sources + ranking           ← Torrentio
                        │            │ 127.0.0.1 AllDebrid pairing + resolve jobs
  luna (libhelpers) ────┤            │ :8790    watch progress, settings
                        ▼            │
  com.webos.media (uMediaServer) → hardware video plane under the UI
```

The service is the same code as the web app's (`tv-app/service`), staged as the app's own JS service with its own ID and port (`scripts/stage-service.mjs`). It reads the web app's saved AllDebrid pairing if there is one, and keeps its own copy. The app starts the service over the system bus. If the bus refuses, it runs the service directly with the TV's Node as a child process. The app stops the service when it exits.

| Part | Files | What it does |
| --- | --- | --- |
| Shell | `main.cpp` | Window, remote, tab bar (Search, Home, Films, Series, Settings), screen stack with fade-and-rise transitions, lifecycle (overlay vs background), service start-up, idle-friendly frame loop (draws only when something changes or animates) |
| Screens | `browse.cpp`, `search.cpp`, `details.cpp`, `sources.cpp`, `player.cpp`, `settings.cpp`, `pairing.cpp` | Home/Films/Series with a full-bleed backdrop that follows focus; search with an on-screen keyboard; title page with direct Play, How to watch, resume and episodes; All versions and the direct-start progress sheet (pairing, resolve progress, file/quality questions); player with a smoked-glass bar; settings and diagnostics |
| Toolkit | `theme.h`, `ui.*`, `common.*`, `gfx.*`, `icons.*` | Glass tokens, tvOS type scale and label roles, text layout (wrap, ellipsis, kerning, oversampled glyphs with gamma), eased animations, glass panels (frosted, smoked, solid), glass buttons and badges, cards with sheen, soft shadows, adaptive scrims, exact-size anti-aliased shapes and line icons |
| Artwork | `images.*` | Fetched through the service, decoded and scaled on worker threads to the exact size drawn, rounded corners and title shading baked in, faded in, least-recently-drawn textures released over a 192 MB budget; frost for each glass panel baked from the blurred backdrop; text-region brightness measured for adaptive scrims |
| Plumbing | `http.*`, `tasks.*`, `api.*`, `model.*`, `json.*` | Localhost HTTP client, worker pools whose results land on the UI thread, typed service API, data model |
| Speech | `speech.*` | LG Audio Guidance: reads the setting and speaks the focused control through the TV's text-to-speech service (unverified on the TV) |
| Playback | `media.*`, `session.*`, `play.*`, `luna.*` | uMediaServer over luna (`load`, `subscribe`, `play`, `pause`, `seek`, `selectTrack`, `setSubtitleEnable`, `unload`); native Buna session (clock, seek preview/commit, stall watchdog, errors) |

### Behaviour worth knowing

- **Play** on a title goes to *Choose how to watch*, with the recommended source focused. It always names exactly what it plays: "Resume S2 · E4", "Play S1 · E8" (next episode), or "Play".
- **Pairing**: if AllDebrid is not connected, pressing Play shows a PIN and continues to that source once paired. Settings can reconnect at any time.
- **Progress** is saved every 15 s while playing, on pause and on exit. Home's *Continue watching* resumes unfinished titles and offers the next episode for finished ones. Playback resumes automatically (a *Start over* button appears).
- **Up next**: in the last 150 s of an episode, Bloob ranks the next one's sources and starts preparing the recommended version, so it starts quickly. When the episode ends, it starts after a 10 s countdown (Back to stay). Settings can turn the countdown off.
- **Back**:
  - **Player:** closes menus, then hides the controls, then exits to where you came from.
  - **Rows:** back to the top, then the tabs.
  - **Tabs:** go to Home. A second Back exits Bloob, after a prompt.
- **LG quick menu, volume and other overlays** leave playback alone. Going to the background (Home, another app) pauses and saves. If the TV releases the media pipeline meanwhile, the player shows *Reconnecting* and reopens at the same position, at most three times a minute.
- **Audio and subtitles** list the file's real tracks from the TV's media pipeline, naming commentary, forced and SDH tracks. The preferred audio and subtitle languages (Settings) choose the starting tracks, and the viewer's choice survives a reconnect.
- **Controls**: any key shows the title, timeline, clock, "Ends at" and the controls row. Left or Right from a clear picture shows only the timeline, which hides 2 s after the seek lands.
- **Settings**: preferred audio language, subtitles, play next automatically, Reduce motion, Disconnect AllDebrid, About (version, credits, licences) and Diagnostics.
- **Effects**: if drawing is measured slow (two 5 s windows with render p95 over 25 ms), shadows and blur turn off for the session. `BLOOB_EFFECTS=full` pins full effects (desktop renders).
- **Log lines for measurement**: `[launch]` stages, `[perf]` key-to-present and render times every 5 s, and one `[qoe]` line per viewing (first frame, stalls, watched time, seeks, reloads).

## Build

```sh
# once: webOS NDK (openlgtv buildroot-nc4, as used by Moonlight TV)
curl -L -o /tmp/sdk.tgz https://github.com/openlgtv/buildroot-nc4/releases/download/webos-a38c582/arm-webos-linux-gnueabi_sdk-buildroot-x86_64.tar.gz
tar xzf /tmp/sdk.tgz -C /opt && /opt/arm-webos-linux-gnueabi_sdk-buildroot/relocate-sdk.sh

native/scripts/build.sh          # unit tests → cross build → ABI guard → service staging → IPK with 755/644 modes → artifacts/com.muhib.bloob_<v>_arm.ipk
native/scripts/acg-lookup.sh     # read-only: which permission groups the TV defines for the services Bloob calls
native/scripts/ui-tour.sh [dir]  # desktop tour of the whole app, following the acceptance task: real service + live catalogue,
                                 # fixture sources, scripted remote, quick-menu / Home / pipeline-lost events; 1920×1080 PNGs
native/scripts/ui-snapshots.sh   # desktop player states with simulated TV media events
```

Desktop runs use `bloob_host`: the same code with a stand-in system bus (`src/luna_host.cpp`) and SDL's dummy video driver. `tests/fixtures/dev-service.js` fakes the indexer and AllDebrid for the tour (`BLOOB_DEV_FIXTURES`), because neither is reachable from build machines. It is never packaged. `ares-package` comes from the web app's dev dependencies.

The built binary needs only `GLIBC_2.4` and no `GLIBCXX`, because the C++ runtime is linked statically. It uses SDL 2.0.0-era functions plus the LG extensions, which are resolved at runtime. It is 32-bit ARM EABI5 soft-float, the same as the TV's own executables.

## Install and test on the TV (Windows `cmd`)

```bat
cd /d %USERPROFILE%\Downloads
ares-install -d rootedTV com.muhib.bloob_2.3.0_arm.ipk
ares-launch -d rootedTV com.muhib.bloob
ssh root@192.168.1.26 "cat /tmp/bloob-native.log"
```

`ares-shell` does not work under the CLI's `tv` profile, so read the log over SSH. The log records hosts only, never full links or account keys. If the service was started directly (the fallback), its own output is in `/tmp/bloob-service.log`.

A direct link can still be played without the catalogue:

```bat
ares-launch -d rootedTV com.muhib.bloob -p "{\"uri\":\"https://...\",\"title\":\"Test\",\"autoplay\":true}"
```

## Verified on the TV

- **0.3.x (13 October 2026), OLED55G26LA, webOS 9.2.2:** window, remote, luna (`getSystemInfo`), uMediaServer `load` with the exported video window, and the bundled clip playing.
- **Not yet verified:**
  - **0.4.0 / 1.1.0:** the bundled service starting on the TV, and artwork and catalogue loading there.
  - **1.1.0:** the fix for the quick menu (user report on 0.4: playback died after opening it), legibility at viewing distance, and the acceptance task. See `TV-TEST.md`.
  - **1.2.0:** everything in `IMPLEMENTATION-UX.md` *Status in 1.2.0*, including the start-position load option and subtitle position, which rely on TV behaviour nobody has confirmed. See `TV-TEST.md` §7.
  - **2.0.0 (Glass):** frost on the TV and its bake time, the Lite fallback, direct Play, the new player timers, Reduce transparency, Increase contrast and spoken focus. See `TV-TEST.md` §8.
  - **AllDebrid on the native app:** pairing and resolving.
  - **Real films:** seeking, and audio and subtitle switching, on real content.

## Next

- Measure first frame, seek landing and memory on the TV with real content. The log already records them.
- **N3** (only if measurements show the need): a Starfish ES backend that demuxes and feeds LG's decoder, as Kodi's webOS port does. It would add Dolby Vision profile conversion, audio fallback and buffering control.

## References (read, not copied)

- **Moonlight TV (GPL-3.0):** the NDK, SDL hints, packaging and the exported video window.
- **Kodi's webOS port (GPL-2.0+):** media pipeline options and TV capability queries.
- **webosbrew documentation:** remote scancodes.

No code from these projects is included. stb_truetype and stb_image are public domain (MIT alternative). Inter is under the SIL Open Font License (`third_party/fonts/Inter-LICENSE.txt`).
