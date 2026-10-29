# Bloob app map

Bloob, split into its actual layers. For each layer this file records what the layer does, which files it lives in, what works and how we know, what is missing, and the next small changes. Work one layer at a time. Cite the increment ID (for example **9.2**) in the commit. When a change lands or is tested on the TV, update the layer's state here.

Snapshot: **2.1.0** (continuity, UX-PLAN X1–X3), 15 October 2026; **2.0.0** (Glass), 14 October 2026; branch `claude/sleepy-ptolemy-o87vgv`. Rows marked *(2.0)* are new in 2.0.0 and follow the phases in [`GLASS-PLAN.md`](GLASS-PLAN.md) (G0–G8); rows marked *(1.2)* are new in 1.2.0; the item IDs (F1, P3, U6…) are from the consolidated list in [`IMPLEMENTATION-UX.md`](IMPLEMENTATION-UX.md#status-in-120).

How the app should *behave* (continuity, place, async work, component states) is planned in [`UX-PLAN.md`](UX-PLAN.md), contracts X1–X12. How good each layer is, against a researched definition of quality, is in [`QUALITY.md`](QUALITY.md). The concrete build plan for responsiveness and continuity (U0 to U9) is in [`IMPLEMENTATION-UX.md`](IMPLEMENTATION-UX.md). Re-score a layer there when you change it.

## How to read the state

| Mark | Meaning |
| --- | --- |
| **TV ✓** | We saw it work on the OLED55G26LA (webOS 9.2.2). |
| **TV (user)** | The owner reported it from the TV. It has not been measured. |
| **Desktop** | It works in the desktop build: the scripted tour, unit tests or renders. |
| **Untested** | It is built but has never run anywhere that proves it works. |
| **Missing** | Not built. |

**Version history on the TV:**
- **0.3:** proved the window, remote, luna, uMediaServer and test-clip path.
- **0.4:** the owner reported that the app worked, with jaggies, some bugs, and playback dying after the LG quick menu.
- **1.1:** answers that report. It has **not yet run on the TV**, so almost everything 1.1 changed is *Desktop* at best.
- **1.2:** the consolidated implementation list (fixes, platform items, responsiveness, comfort). Also **not yet run on the TV**.
- **2.0:** Glass, the complete visual overhaul the owner chose, plus direct Play and spoken focus. **Not yet run on the TV**; `TV-TEST.md` §8 is its pass.
- **2.1:** continuity (UX-PLAN X1–X3). Progress sessions and completion, the next-action target, version intent per show, and the sectioned episode versions list. **Not yet run on the TV**; `TV-TEST.md` §9 is its pass.

## The layers at a glance

```
 EXPERIENCE   1 UX ─ 2 UI ─ 3 Home ─ 4 Search ─ 5 Details ─ 6 Sources ─ 7 Player screen ─ 8 Settings
                 │ what the viewer sees and does
 ENGINE       10 App shell ─ 11 Remote input ─ 12 Rendering ─ 13 Artwork ─ 14 Data & bus ─ 9 Player engine (Buna)
                 │ C++ in native/src, one process on the TV
 SERVICE      15 Service core ─ 16 Catalogue ─ 17 Sources & ranking ─ 18 AllDebrid ─ 19 Progress & settings
                 │ Node, tv-app/service, bundled as com.muhib.bloob.service on 127.0.0.1:8790
 FOUNDATION   20 Platform & packaging ─ 21 Testing & tooling ─ 22 Observability ─ 23 External services ─ 24 Web app (legacy)
```

| # | Layer | Overall state | Most important open item |
| --- | --- | --- | --- |
| 1 | UX: journeys and remote rules | Desktop | Nobody has done the acceptance task on the TV yet |
| 2 | UI: visual system | Desktop | Legibility and contrast at viewing distance on OLED |
| 3 | Home and browse | Desktop; TV (user) for 0.4 | Rows are a fixed list; no "see all" |
| 4 | Search | Desktop | No voice input |
| 5 | Details, seasons, episodes | Desktop | Season strip scrolling not yet seen on a many-season title |
| 6 | Sources and pairing | Desktop | Whether a source is cached on AllDebrid is unknown until resolving |
| 7 | Player screen | Desktop | No thumbnails; tracks only as the TV reports them |
| 8 | Settings and diagnostics | Desktop | None of the new preferences tried on the TV |
| 9 | Player engine (Buna N2) | TV ✓ test clip; real files Untested | Start-position option and subtitle position unverified on the TV |
| 10 | App shell and lifecycle | TV (user) for 0.4; 1.1 lifecycle Desktop | The quick-menu fix needs a TV check |
| 11 | Remote input | TV ✓ D-pad and Back | Magic Remote pointer ignored |
| 12 | Rendering engine | TV ✓ for 0.3/0.4; 1.1 smoothing Desktop | Chinese, Japanese and Korean have no font (drawn as ?) |
| 13 | Artwork pipeline | TV (user); Desktop | Memory headroom on the TV unmeasured |
| 14 | Data, workers and system bus | TV ✓ luna; Desktop for the rest | Model parsing has no tests |
| 15 | Service core and security | TV (user) for 0.4 | Bus start versus fallback start unmeasured |
| 16 | Catalogue | TV (user); Desktop | One source (Cinemeta); licensing review needed |
| 17 | Sources and ranking | Desktop (fixtures) | One indexer; weights untuned on real results |
| 18 | AllDebrid pairing and resolving | Web app TV (user); native Untested | Pairing must survive a restart on the TV |
| 19 | Progress and settings store | Desktop | One profile; files never pruned |
| 20 | Platform and packaging | TV ✓ | `requiredACG` waits for `acg-lookup.sh` output from the TV |
| 21 | Testing and tooling | Desktop | The tour takes pictures but checks nothing; no CI |
| 22 | Observability | TV ✓ log over SSH; Desktop for the rest | No memory sampling; dropped frames unknown |
| 23 | External services | — | Every one is a third party we do not control |
| 24 | Web app (legacy, `tv-app/src`) | TV (user) 0.2 | Decide whether to freeze it |

---

## Experience

### 1. UX: journeys and remote rules

**Job:** how a viewer gets from opening Bloob to watching, and back, with only the remote. It covers focus, Back and what each key means everywhere.

**Lives in:**
- The rules: `native/DESIGN.md` (Remote rules).
- The code: `native/src/main.cpp` (tabs, Back, exit), and the `key()` of each screen.
- The test: `TV-TEST.md` §1.

**State:**

| Item | Evidence |
| --- | --- |
| Pick, understand, start, change track, return: the acceptance task | Desktop (`ui-tour.sh`) |
| Back has one owner per screen. Exiting needs a second Back press, with a toast. | Desktop |
| Up from any screen reaches the tabs; Down returns to the content | Desktop |
| Returning restores focus, scroll and backdrop (screens stay alive under the next) | Desktop |
| After an episode ends, details offers the next one, with the last marked watched | Desktop |
| Done by a real person, at the sofa | Missing |
| *(1.2)* One Play owner (`play.h`): Home, Details, the episode row and Up next all go through `start_playback`; resume rules live in one place (U2) | Desktop (unit tests) |
| *(1.2)* Reduce motion: no slides, zooms, card lift or hero slideshow (U8) | Desktop |
| *(2.0)* Play starts the best version directly from Home, the title page, an episode or Up next; only the progress sheet shows, and Cancel returns to where Play was pressed (G4) | Desktop (tour) |
| *(2.0)* Spoken focus with LG Audio Guidance: label, role and position for every focusable control (G7) | Desktop (log); TV untested |

**Gaps:**
- No watchlist ("My list").
- No way to remove an item from Continue watching, or to mark something watched by hand.
- Colour and number keys do nothing.
- ~~No first-run explanation.~~ 1.2 adds a one-time AllDebrid note on Home (G1).

**Next:**
- **1.1** Run `TV-TEST.md` §1 with someone who has not seen Bloob. Turn every hesitation into an increment on the right layer.
- **1.2** Remove from Continue watching: a long press or a colour key on the card, with a confirmation toast.
- **1.3** My list: an Add button on details and a row on Home. Needs **19.3**.
- **1.4** Decide on exit gating. Android TV guidance says Back on the root should leave the app without confirmation, but Bloob asks for a second Back. Keep the toast or drop it, deliberately.

### 2. UI: visual system

**Job:** how everything looks: tokens, type, focus, motion, states and components.

**Lives in:** `native/DESIGN.md` (the spec, Glass since 2.0), `native/GLASS-PLAN.md` (how Glass was built), `src/theme.h` (tokens), `src/ui.h`/`ui.cpp` (type scale, label roles and components: glass panel, glass button, glass badge, card, progress, spinner, hint, missing-art) and `src/images.cpp` (frost bake).

**State:**

| Item | Evidence |
| --- | --- |
| Tokens: colour, spacing, radii, card sizes, motion rates | Desktop |
| Inter type scale (10 styles, floor 19 px) | Desktop |
| Focus: lift ×1.08, ring, white button | Desktop |
| Designed states: skeleton, missing art, errors, reconnecting | Desktop |
| Seen at viewing distance | Missing |
| *(1.2)* Type floor 22 px: Caption 23, Footnote and Eyebrow 22 (G5) | Desktop |
| *(1.2)* Effects tiers: two slow 5 s windows (render p95 over 25 ms) turn shadows and blur off for the session (U8) | Desktop |
| *(2.0)* Glass: frosted panels baked from the blurred backdrop, smoked glass over video, solid fallback; crossfade-safe panes (G1) | Desktop |
| *(2.0)* tvOS type scale (Display 104 … Caption2 23) and label roles; old 1.x type names and components removed (G0, G8) | Desktop |
| *(2.0)* Adaptive scrims from artwork brightness measured at decode; frost presses bright areas down so white text stays readable (G1, G6) | Desktop |
| *(2.0)* Card focus: ×1.08 lift, shadow and a baked diagonal sheen; white focus fill with black label on every control (G1) | Desktop |
| *(2.0)* Reduce transparency and Increase contrast settings (G6) | Desktop |

**Gaps:**
- The settings icon reads as a sun.
- The 50% tertiary text may be too faint on OLED.
- Motion rates are tuned for 60 fps desktop, not measured on the TV.
- No high-contrast or larger-text option.

**Next:**
- **2.1** TV legibility pass (`TV-TEST.md` §2). Adjust `secondary`/`tertiary` alpha and the type floor from what is seen.
- **2.2** Redraw the settings gear and check every icon at 32 px on the TV.
- **2.3** Larger-text setting: scale the type table by 1.15. Layouts already wrap and ellipsise.
- **2.4** Raise the type floor to about 22 px. TV guidance puts reading at 3 m at roughly that; Footnote and Eyebrow are 19 px and Caption is 21 px today. Check the layouts still fit.

### 3. Home and browse

**Job:** the first screen, plus the Films and Series tabs.

**Lives in:** `src/browse.cpp`. The data comes from **16** (`/api/home`, `/api/browse`) and **19** (`/api/progress`).

**State:**

| Item | Evidence |
| --- | --- |
| Hero: up to 6 featured titles (continue items first, then popular titles with a logo). Pages with Left/Right; advances after 9 s idle. | Desktop |
| 10 shelves on Home (popular, highest rated, 6 genres), up to 24 items each | Desktop; TV (user) for 0.4 |
| The backdrop follows focus after 250 ms; rows above the focused one fade | Desktop |
| Continue watching: resume, or the next episode | Desktop |
| Skeleton while loading; error with Try again | Desktop |
| *(1.2)* Focus kept on the same card by identity when Continue watching refreshes (F4) | Desktop |
| *(1.2)* Title details prefetched once focus rests on a card; Details opens from the cache (U4) | Desktop |
| *(1.2)* One-time note explaining AllDebrid when nothing is paired: Connect now / Later (G1; no QR code) | Desktop |
| *(2.0)* Full-bleed hero with adaptive scrims, frosted tab bar on the 60 px safe line, 320×180 cards in the tvOS grid, Continue cards with title, time left and progress inside the art (G2, G3) | Desktop |

**Gaps:**
- Shelves are a fixed list in `catalog.js`.
- No "see all" or genre pages.
- No personalisation.
- Films and Series show the same row style as Home.

**Next:**
- **3.1** "See all" at the end of a shelf, opening a grid page.
- **3.2** Genre page from the Films and Series tabs.
- **3.3** Order the Home rows by what was watched recently.

### 4. Search

**Job:** find a title by name.

**Lives in:** `src/search.cpp`. Data from **16** (`/api/search`). Recent searches are saved through **19**.

**State:**

| Item | Evidence |
| --- | --- |
| On-screen keyboard (6 columns) and 3-column results grid | Desktop |
| Searches 450 ms after the last key | Desktop |
| Recent searches, saved in settings | Desktop |
| *(1.2)* Keyboard gives way: it slides off, the query stays as a chip, results widen to 4 columns; landing, waiting, results and empty states (U9) | Desktop |
| *(2.0)* Keyboard on solid glass, white focus keys, recent searches as glass buttons, results in 5 columns once the keyboard steps aside (G6) | Desktop |

**Gaps:**
- No voice search with the LG mic.
- Films and series are mixed, with no filter.
- No "did you mean".

**Next:**
- **4.1** Check whether the TV's voice key reaches SDL. If it does, map it to search.
- **4.2** Label each result as a film or series, with its year.

### 5. Details, seasons and episodes

**Job:** the title page. It continues into seasons and episodes, and decides what Play plays.

**Lives in:** `src/details.cpp`, with `src/common.cpp` (backdrop, title or logo, meta line). Data from **16** (`/api/meta`) and **19** (progress for the title).

**State:**

| Item | Evidence |
| --- | --- |
| Logo or title, meta, IMDb rating, 3-line description, credits for films | Desktop |
| Main button names what it plays: Resume S1 · E2 / Play S1 · E3 / Play | Desktop |
| Down lifts the page into season pills and episode cards; About strip at the foot | Desktop |
| Watched checks and resume bars on episodes | Desktop |
| Specials last | Desktop (service) |
| *(1.2)* Season strip scrolls to keep the selected season visible, with chevrons for more (F3/U5) | Untested (tour title has one season) |
| *(1.2)* The main button comes from `play::title_default` (latest unfinished, else next, else first regular episode) (U2) | Desktop (unit tests) |
| *(2.0)* Title page in Glass with *Play*, *Start over*, *All versions*; films get the frosted *How to watch* panel (best version ticked, two alternatives) fetched as the page opens (G4) | Desktop |
| *(2.0)* Glass season buttons (candidate for the selected season) and episode cards with sheen (G4) | Desktop |

**Gaps:**
- **Season pills that do not fit across the screen are not drawn**, so shows with many seasons cannot reach the later ones. This is a real bug (`draw_episodes`, `if (sx + w > W - margin) break;`).
- No trailer.
- No "more like this".
- No mark-as-watched.

**Next:**
- **5.1** Season pills that scroll with focus. This is the bug above.
- **5.2** Mark an episode, or a season, as watched.
- **5.3** "More like this" row at the foot, from Cinemeta genres.

### 6. Sources and pairing ("Choose how to watch")

**Job:** make the best playable version obvious, and make the alternatives and release details reachable. It also pairs AllDebrid once and shows the resolve as it happens.

**Lives in:** `src/sources.cpp`, `src/pairing.cpp`. Data from **17** (`/api/sources/ranked`) and **18** (`/api/resolve/*`, `/api/pin/*`, `/api/account`).

**State:**

| Item | Evidence |
| --- | --- |
| Best match card in plain words, with a reason and a caution | Desktop |
| Other versions: 5 visible rows, scrolling, availability in words | Desktop |
| Release details drawer, opened with Right | Desktop |
| Pairing PIN sheet, then continues straight to the chosen source | Desktop (fixtures) |
| Resolve sheet with steps and progress. Asks for a file or quality when needed. On failure, "Try the next best". | Desktop (fixtures) |
| *(1.2)* "1 seeder", plain digits; TV format warnings for DTS, TrueHD Atmos, disc Dolby Vision, 3D, disc images, samples and bitrate against 95 Mbps Ethernet (F5, P3, P4) | Desktop (unit tests) |
| *(1.2)* A next episode prepared during the credits is picked up instead of starting a new job (U10) | Untested |
| *(2.0)* The Sources screen is now *All versions* (opened on request) and the direct-start progress sheet; versions, release details, file and quality questions and the failure sheet restyled as glass; a chosen alternative travels as `PlayRequest.candidate` (G4) | Desktop |
| *(2.0)* Pairing sheet on frosted glass over whichever screen opened it (G6) | Desktop |
| *(2.1)* Episode versions in sections (matches the last episode, other versions, *May not play well on this TV*). Each is tagged *Season pack* or *Single episode*. A hand-picked version becomes the show's intent. The starting sheet names a continued version and offers *Other versions* (X3.3) | Desktop (tour 19b, 19c) |
| *(2.5)* Filter pills by resolution with counts; sections by kind of release (Remux, Blu-ray encodes, Web downloads, Web rips…); two-line rows (what you get; source, size, group); *Found by* in release details and the indexes named under the list (6.4) | Desktop (tour 10, 11, 11b, 11c, 19c) |

**Gaps:**
- Nothing shows whether a release is already cached on AllDebrid (instant) or must download.
- No manual link or magnet entry (the web app had one).
- Ranker weights have not been checked against real results.

**Next:**
- **6.1** First real-source run on the TV: is the Best match the one you would have picked? Feed the answer into **17.1**.
- **6.2** Show "Ready now" when AllDebrid already has a release, once **18.3** exists.
- **6.3** Manual link entry, through the Search keyboard.
- **6.4** *(done in 2.5, desktop)* Structured versions list. Next: does the owner find it easier on the TV?

### 7. Player screen

*(2.5.3)* Playback info panel: ⓘ control, Info/I or blue key; live network (whole TV), file average bitrate, buffer report, playback stats, version, CPU and memory (`stats.*`, unit-tested parsers). Desktop snapshot `08b-info-panel`.

**Job:** stay quiet during viewing. When summoned, show accurate seeking, audio, subtitles and the next episode. Handle the start, the end and errors.

**Lives in:** `src/player.cpp` (view and key handling). The engine is **9**.

**State:**

| Item | Evidence |
| --- | --- |
| Starting screen (title, episode, delayed spinner, "Starting · 4K Dolby Vision") | Desktop |
| Controls appear on any key and hide after 5 s; title, episode, format badges | Desktop |
| Timeline with a seek bubble (target and jump); Left/Right speed up while held | Desktop |
| Controls row: play/pause, Audio · X, Subtitles · X, Start over, Next episode | Desktop |
| Track panels listing the TV-reported tracks | Desktop (simulated events) |
| Up next with a 10 s countdown; Back stays | Desktop |
| Error panel: Try again, Choose another version, Exit | Desktop |
| Reconnecting screen after the TV drops the stream | Desktop |
| Playback of the test clip | TV ✓ (0.3) |
| *(1.2)* Title on a solid scrim band so bright scenes cannot wash it out (F6) | Desktop |
| *(1.2)* Seeking from a clear picture shows only the timeline, which hides 2 s after landing; resting on the controls row keeps them 12 s (U6) | Desktop |
| *(1.2)* Wall clock top right and "Ends at" beside the controls (G3) | Desktop |
| *(1.2)* Track labels name commentary, forced and SDH tracks, more codecs and languages, and number duplicates "#2" (G2) | Desktop |
| *(1.2)* Up next follows the autoplay setting; the next episode is ranked and resolved in the last 150 s (U10) | Desktop (UI); preparation Untested |
| *(2.0)* Smoked-glass bar 56 px above the bottom (title, meta with badges, controls above the timeline), smoked clock pill with *Ends*, white seek bubble, track panels standing on the bar, Up next as a smoked card (G5) | Desktop (`ui-snapshots.sh`, tour) |
| *(2.0)* Timers 5 s, 8 s when paused or on a control (paused no longer pins), fades 300 ms, seek commit 400 ms (G5) | Desktop (unit test for the commit) |

**Gaps:**
- No seek thumbnails.
- No subtitle size or style.
- No skip-intro or skip-recap.

**Next:**
- **7.1** ~~Add the clock and "ends at" to the controls.~~ Done in 1.2 (G3).
- **7.2** Subtitle size and background, once **9.4** shows the TV can render them.

### 8. Settings and diagnostics

**Job:** the account, preferences, and a way to see what is wrong without a computer.

**Lives in:** `src/settings.cpp` (Settings and Diagnostics). Data from **18** (`/api/account`) and **19** (`/api/settings`).

**State:**

| Item | Evidence |
| --- | --- |
| AllDebrid row: connect, connected and where it is saved, storage errors only when real | Desktop |
| Preferred audio language (Left/Right) | Desktop. It only affects ranking; see **9.2**. |
| Diagnostics: display, remote keys, recent log | Desktop |
| Version and TV model | Desktop |
| *(1.2)* Rows: preferred audio, subtitles language, autoplay next episode, Reduce motion, Disconnect AllDebrid (OK twice), About (version, credits, licences) (G4, U8) | Desktop |
| *(1.2)* Diagnostics shows the effects tier, the last `[perf]` window and the network type | Desktop |
| *(2.0)* Glass rows with switches; *Reduce transparency* and *Increase contrast* saved by the service; About and Diagnostics in the tvOS type (G6) | Desktop |
| *(3.3)* **Calibrate** (`src/calibrate.cpp`): Check, Test on Expert 2, Calibrate Expert 1 (140 nits), Undo, Stop; starts `bin/bloob-cal` as root through the Homebrew Channel and shows its status file (CALIBRATION-PLAN.md) | Desktop, against `tools/fake-tv` |

**Gaps:**
- No subtitle language.
- No autoplay-next switch.
- No unpair.
- No about or licences page.

**Next:**
- **8.1** Subtitle language and autoplay-next settings. They need **19** fields and **9.2**.
- **8.2** Unpair AllDebrid. This overwrites the saved key with nothing; it does not delete files.
- **8.3** About page: version, licences (Inter OFL, stb, Big Buck Bunny CC BY).

---

## Engine (C++, `native/src`)

### 9. Player engine (Buna N2)

**Job:** one playback session. It covers:
- loading a URL into LG uMediaServer;
- the clock and seek preview/commit;
- buffering and stall detection, and tracks;
- recovery when the TV drops the stream;
- saving progress;
- teardown.

**Lives in:**
- `src/session.*`: the platform-free clock, seek, phases and errors. It has unit tests.
- `src/media.*`: `UmsPlayer`, which talks to `com.webos.media` over luna (load, subscribe, play, pause, seek, selectTrack, setSubtitleEnable, unload, notifyForeground/Background).
- `src/player.cpp`: the exported video window, reload, resume and progress saves.

**State:**

| Item | Evidence |
| --- | --- |
| Load, play and unload through uMediaServer in an exported hardware video window | TV ✓ (0.3, test clip) |
| Real resolved files (4K, HDR or Dolby Vision, Atmos) through this path | Untested natively. TV (user) in the web app only. |
| Clock interpolation (capped 2 s ahead), seek preview, commit after 600 ms, landing confirmation, 15 s timeout | Unit tests, Desktop |
| Stall watchdog (3 s) and startup timeout (45 s) | Unit tests |
| Track lists from `sourceInfo`; selectTrack and setSubtitleEnable are sent | Desktop (simulated). TV switching untested. |
| LG quick menu leaves playback alone; Home pauses and saves; foreground notify | Desktop (scripted window events) |
| Pipeline lost → reopen at the same position, at most 3 per minute | Desktop (simulated `unloadCompleted`) |
| Resume from ≥30 s below 95%; progress every 15 s, on pause and on exit | Desktop |
| First-frame and seek-landing times in the log | Desktop |
| *(1.2)* Chosen audio and subtitle tracks re-applied after a reconnect; preferred audio and subtitle languages applied at start (F7) | Desktop (simulated events) |
| *(1.2)* Start position sent in the load request (`transmission.playTime.start`); seek fallback if the first clock report is elsewhere; load retried without it if refused (P5) | Desktop; TV Untested |
| *(1.2)* `policyAction` treated as a lost pipeline; unknown event keys logged once each with their value (P2) | Desktop |
| *(1.2)* Subtitle position follows the controls: 0 hidden, 2 reduced, 4 full; switched off after one refusal (U7) | Untested (undocumented call) |
| *(1.2)* QoE accounting across reconnects: stalls and stall time, time wanting to play, media watched, seeks and landing error, reloads; one `[qoe]` line per viewing (U1) | Desktop (unit tests) |

**Gaps:**
- The preferred audio language is not chosen at start.
- Chosen audio and subtitle tracks are **not restored after a reconnect**.
- No external subtitles.
- No frame-rate matching or output-mode report.
- An expired AllDebrid link is not re-resolved; after 3 reloads it says the link may have expired.
- Buffer depth and dropped frames are not reported.

**Next:**
- **9.1** TV check with one real file: first frame, a seek landing, an audio switch, and the quick menu (`TV-TEST.md` §3).
- **9.2** Pick the preferred audio language at start, from `sourceInfo`.
- **9.3** Restore the selected audio and subtitle tracks after a reload.
- **9.4** Check which subtitle formats the TV renders (embedded SRT, ASS, PGS), then decide on external WebVTT.
- **9.5** Re-resolve through **18** when a reload fails with a likely expired link, then continue at the same position.
- **9.6** Native ES backend (N3) **only if** 9.1 to 9.4 show a gap uMediaServer cannot close.

### 10. App shell and lifecycle

**Job:** the process itself. It covers:
- the window, the frame loop (draws only when something changes) and the screen stack with transitions;
- the tabs, Back and exit;
- routing lifecycle events (overlay or background) to the top screen;
- starting and stopping the service;
- launch parameters and scripted runs.

**Lives in:** `src/main.cpp`, `src/screens.h` (screen contract), `src/ui.h` (`Ctx`, shared state).

**State:**

| Item | Evidence |
| --- | --- |
| Window, 1920×1080 canvas, idle loop | TV ✓ |
| Screen stack with fade-and-rise transitions; screens stay alive under the next | Desktop |
| Tabs (Search, Home, Films, Series, Settings); double-Back exit with a toast | Desktop |
| Focus loss is ignored; hidden or background pauses; shown or foreground resumes | Desktop |
| Service start over the bus, with a fork/exec fallback that runs the TV's Node | TV (user) for 0.4, which did not say which path ran |
| Launch parameters `{uri, title, autoplay, loadType}` play a link directly | Desktop; untested on the TV |
| Scripted runs (`BLOOB_SCRIPT`: key, wait, media, window, snap) | Desktop |
| *(1.2)* `[launch]` marks: window, fonts, service, home-data, home-frame (U1) | Desktop |
| *(1.2)* Connection Manager subscription sets online and wired/wifi; offline turns service errors into "You're offline" (P6) | Untested |

**Gaps:**
- Startup time is unmeasured.
- The Exit key quits at once; whether the final progress save reaches the service is unverified.
- Memory after leaving the player is unmeasured.
- No deep links (open a title from the launcher).

**Next:**
- **10.1** Log the time from launch to first Home frame, and which service-start path was used.
- **10.2** On Exit, flush the last progress save before quitting.
- **10.3** Deep link `{id, type}` launch parameters, which open details.

### 11. Remote input

**Job:** turn remote events into semantic keys.

**Lives in:** `src/keys.h`. The SDL hints in `main.cpp` let Back and Exit reach the app.

**State:**

| Item | Evidence |
| --- | --- |
| D-pad, OK, Back | TV ✓ |
| Transport keys (play, pause, stop, fast-forward, rewind) | Untested on TV |
| Colour keys mapped | Mapped, unused |
| Exit quits | Untested on TV |
| Key repeat drives scrolling and seek speed-up | Desktop |

**Gaps:**
- The Magic Remote pointer is ignored.
- Number keys are not mapped.
- The voice key is unknown.

**Next:**
- **11.1** Diagnostics already lists keys: press every key on the remote and record the scancodes in this file.
- **11.2** Basic pointer support: hover moves focus and click is OK, on cards, buttons and rows.

### 12. Rendering engine

**Job:** draw everything. It covers:
- text from Inter;
- anti-aliased shapes, shadows, gradients and line icons;
- layers for transitions.

**Lives in:** `src/gfx.*` (canvas, fonts and glyph atlases, shapes, shadows, layers), `src/icons.*`.

**State:**

| Item | Evidence |
| --- | --- |
| SDL renderer (opengles2 on the TV) to a 1920×1080 canvas | TV ✓ |
| Inter in 4 weights; exact-size glyphs, 2× oversampling ≤48 px, gamma lift, kerning | Desktop; TV (user) reported jaggies in 0.4, before this change |
| Exact-size anti-aliased discs, rings, triangles; soft shadows; gradients | Desktop |
| Layer capture for page transitions | Desktop |
| *(1.2)* Glyphs beyond Latin-1 baked on demand (Polish, Czech, Greek, Cyrillic…), capped at 2,000 per size (F8) | Desktop |
| *(1.2)* Shadows off in the Lite tier; `BLOOB_EFFECTS=full` pins the tier for desktop renders | Desktop |
| *(2.4, R5a)* Redraw only on visible change: wakes are coalesced notifications and no longer repaint; the media clock alone never repaints; one frame per refresh when the renderer has no vsync; animation time is the median of three frame intervals, and the first frame after idle steps one refresh (2.3 stepped up to 100 ms) | Unit tests; desktop tour |
| *(2.4, R5a)* SDL batching hint; render-state cache skips repeated colour and blend calls; shelf cards drawn in passes (shadows, bodies, text) so same-texture draws are adjacent | Desktop tour (pixel-identical Home) |
| *(2.4, R5b)* Shape, sheen and icon textures and glyph atlases counted against the GPU budget; unused shapes pruned; released in the background | Desktop (inspector) |
| *(2.4, R6r)* Transitions paint the screen underneath once into a retained layer and composite it; the leaving screen likewise; idle full-size targets freed after ~5 s | Desktop (inspector: 6 paints, 24 reuses in the tour) |

**Gaps:**
- **The glyph atlas covers Latin-1 and a few symbols only** (`atlas_codepoints`). Polish, Czech, Turkish, Greek, Cyrillic, Arabic, CJK and so on show as missing characters.
- There is no text shaping, so right-to-left scripts would also need more than glyphs.
- Frame time on the TV is unmeasured.

**Next:**
- **12.1** Load glyphs on demand for any code point Inter has (Latin Extended, Greek, Cyrillic).
- **12.2** Fallback font for scripts Inter lacks, starting with CJK and Arabic. Check the TV's system fonts first, which avoids shipping them.
- **12.3** *(Done in 2.4 through the inspector and `[perf]` stages; Diagnostics shows GPU use.)* Frame-time log in Diagnostics: average and worst over the last 5 s.

### 13. Artwork pipeline

**Job:** get images fast without using up the TV's memory.

**Lives in:** `src/images.*`. Fetching goes through **15**/`/api/image`.

**State:**

| Item | Evidence |
| --- | --- |
| Fetch and decode on 3 workers; scaled to the exact drawn size; corners and shading baked in | Desktop; TV (user) for 0.4 |
| Blurred backdrops (quarter size, 3-pass box blur) | Desktop |
| Logos decoded once at 760×240, then scaled | Desktop |
| Least-recently-drawn eviction above a 192 MB budget | Desktop |
| Fade-in; a failed backdrop keeps the previous one | Desktop |
| *(2.0)* Frost bake per panel on the image workers (quarter-size blur, half-resolution crop, tint, rounded corners), cached by rect and picture; luminance of the text region measured at decode for adaptive scrims (G1) | Desktop |
| *(2.4, R5c)* Fetch (4 I/O workers) and decode (3 CPU workers) are separate stages; one decode per source picture feeds the sharp, blurred and frost variants; requests carry a relevance (visible, near, prefetch) that orders both queues | Desktop (inspector: 19 fetches, 19 decodes, 25 variants in the tour) |
| *(2.4, R5c)* Staged cancellation: a request nobody drew for 30 frames is retired; workers check again after the fetch and before decoding and scaling. Uploads paced (6 textures or 12 MB per frame) | Desktop |
| *(2.4, R5b)* One GPU budget for every texture, admitted before creation; eviction by relevance then frame age, never a texture drawn in the last 3 frames; an evicted image keeps its record and reloads without fading in. Budgets: 96 MB browsing, 56 MB playback or low system memory, 24 MB background | Unit tests (policy); desktop |
| *(2.4, R5b)* Background releases every texture and decoded source and stops new fetches; foreground resumes them, most relevant first | Desktop tour |

**Gaps:**
- The budgets are starting values; TV memory is unmeasured (the inspector now reports it).
- Art that was not on screen when Bloob went to the background reloads on return (the reference does the same).
- No progressive or low-resolution placeholder before the full image.

**Next:**
- **13.1** Measure resident memory on the TV after scrolling every Home row (now readable with `tv-inspect.sh`). Set the budgets in `resources.cpp` from that.
- **13.2** Show a tiny blurred placeholder while a backdrop loads.

### 14. Data, workers and system bus

**Job:** move data between the screens, the local service and the TV's system bus, without blocking the frame.

**Lives in:**
- `src/http.*`: blocking HTTP/1.1, 127.0.0.1 only.
- `src/tasks.*`: worker pools (API 2, fetch 4, images 3), named threads with explicit stacks, three priority lanes, budgeted completion pumps. Newest jobs run first within a lane, and results are dropped if their screen has gone.
- `src/api.*`: the service client.
- `src/json.*`, `src/model.*`: parsing into `Item`, `Meta`, `Ranking`, `PlayRequest` and the rest.
- `src/luna.*`: the bus over LG libhelpers, with replies handed to the UI thread. `src/luna_host.cpp` stands in for it on the desktop.
- `src/platform.*`: the LG SDL extensions, resolved at runtime.

**State:**

| Item | Evidence |
| --- | --- |
| Luna calls and subscriptions | TV ✓ |
| HTTP to the local service, worker pools, owner-guarded results | TV (user) for 0.4; Desktop |
| JSON parser | Unit tests |
| Model parsing | Desktop only (no unit tests) |
| *(1.2)* Chunked-response parser rejects oversized chunk lengths (F1) | Desktop |
| *(1.2)* Slow requests (over 20 s) logged; status-0 errors split into timed out and not responding (P6) | Desktop |
| *(2.0)* Audio Guidance subscription (settingsservice) and TTS speak calls (`speech.cpp`); one refusal turns speech off (G7) | Desktop (host bus); TV untested |
| *(2.4)* **Fixed: workers never woke the UI thread.** The completion was moved into the queue before the wake test, so the test always failed (since 1.x); replies and artwork waited for an unrelated wake, up to 1 s when idle. Covered by `runtime_test` | Unit test (fails on the old code) |
| *(2.4, R5a)* Wakes coalesced (one SDL event per drain); bus and task pumps bounded to 6 ms per turn; retired Luna records counted (libhelpers is closed, so freeing them cannot be proved safe) | Unit tests; desktop |
| *(2.4, R5b)* Subscribes to the system memory state (`com.webos.memorymanager/getCurrentMemState`, unverified on the TV; the first reply is logged); low or critical moves browsing onto the playback budget | Desktop (host bus) |

**Gaps:**
- No unit tests for the model parsers.
- No shared retry or backoff policy; each screen has its own Try again.
- HTTP opens a new connection per request.

**Next:**
- **14.1** Unit tests for `parse_meta`, `parse_ranking`, `parse_resume` and `next_episode`, against recorded service JSON.
- **14.2** One retry with a short delay for idempotent GETs.

---

## Service (Node, `tv-app/service`, bundled into the IPK)

### 15. Service core and security

**Job:**
- Serve the app's API on localhost.
- Keep the AllDebrid key out of the app.
- Refuse requests that are not from the app.

**Lives in:**
- `server.js`: the routes.
- `config.js` and `bloob.json`: app ID, port, shared pairing.
- `net.js`: HTTPS with size caps and allowed redirect hosts.
- `native/scripts/stage-service.mjs`: staged as `com.muhib.bloob.service`, port 8790.

**Routes:**
- **Health and catalogue:** `health`, `home`, `browse`, `search`, `meta`, `image`.
- **Progress and settings:** `progress` (GET and POST), `settings` (GET and POST).
- **Sources and resolving:** `sources/ranked`, `sources/search`, `resolve/start|status|choose|cancel`.
- **AllDebrid account:** `account`, `pin/start|check`.
- **Web app only:** `link/unlock|quality|delayed`, `magnet/add|status|files`.

**State:**

| Item | Evidence |
| --- | --- |
| Accepts only `Host: 127.0.0.1/localhost`, and only JSON POSTs | Unit and integration tests |
| Older-Node compatible (no `?.`, no `??`) | Policy, by review |
| Runs on the TV | TV (user) for 0.4 |

**Gaps:**
- The API has no version number, so the app and service must ship together. They do today, in one IPK.
- The Node version on the TV is not recorded.

**Next:**
- **15.1** Record `process.version` in `/api/health`, and show it in Diagnostics.

### 16. Catalogue

**Job:** films, series, seasons, episodes and artwork links.

**Lives in:** `catalog.js` (Cinemeta, with Metahub artwork). It caches for 30 minutes, up to 300 entries, and never caches failures.

**State:**

| Item | Evidence |
| --- | --- |
| Home (10 rows), browse by type, meta with seasons (specials last), search | Desktop; TV (user) for 0.4 |
| Shouting episode titles are title-cased; acronyms are kept | Unit tests |

**Gaps:**
- One provider.
- Licensing review needed before any release.
- Some titles have no logo.
- No region or age-rating data.

**Next:**
- **16.1** Move the catalogue behind a small provider interface, the way **17.2** will for sources, so a second source can be added.

### 17. Sources: discovery and ranking

**Job:** find candidate releases for a title or episode, and rank them so one is clearly best.

**Lives in:**
- `release.js`: turns stream names into typed claims (resolution, HDR, codec, audio, channels, source type, size, seeders, languages).
- `rank.js`: explainable scores, cautions, groups.
- `sources.js` *(2.5)*: Torrentio, TorrentsDB, StremThru Torznab, The Pirate Bay and EZTV, merged by hash.

**State:**

| Item | Evidence |
| --- | --- |
| Release parser (both Torrentio formats) | Unit tests |
| Ranker: one recommendation with a reason; cinema recordings and zero-seeder releases excluded | Unit tests; Desktop with fixtures |
| Live Torrentio results | Not reachable from build machines. TV (user) in the web app. |
| *(1.2)* TV profile table (`tv-profile.js`, `lg-2022-oled`) drives warnings; all weights in one `WEIGHTS` table; bitrate from size over running time; a DTS-only version is never recommended while one with playable sound exists (P3) | Desktop (unit tests) |
| *(1.2)* Parser: 3D, disc image and sample claims; ambiguous source words read only after the title (P4) | Desktop (unit tests) |
| *(2.1)* Continuity: a show keeps to its version. Matches are the same pack or group (+25), then the same resolution and picture (+12); dropping below costs −10. Seeders are capped at 6, below one resolution step. Sections: matches, other versions, *May not play well on this TV*. Release group and binge group are parsed (X3.2) | Desktop (unit tests, tour) |
| *(2.5)* Several indexes behind a provider interface (17.2); episode filter for show-wide indexes; reply 2.5 s after Torrentio at most, late answers join the cache; Settings > Sources | Unit tests; live from a datacenter (Torrentio blocked there) |
| *(2.5.2)* No bitrate penalty on the owner's Wi-Fi TV (only a profile with a network limit applies one); no season-pack penalty | Unit tests |
| *(2.5)* Picture preference (best, HDR10, SDR); per-TV Dolby Vision containers (this TV: MKV, MP4, TS, per the owner; 2.5.0's "MP4 only" was wrong and is gone in 2.5.1); sections capped at 120/40 | Unit tests |

**Gaps:**
- Knaben (title text search, the CocoScrapers approach) is not added yet.
- Every quality field is a *claim from the name*, not a probe.
- Weights are untuned.

**Next:**
- **17.1** Log the top 5 ranked releases (names only) for 10 real titles on the TV. Tune the weights from that.
- **17.2** *(done in 2.5)* Put the indexer behind a provider interface.

### 18. AllDebrid: pairing and resolving

**Job:** pair once, keep the key safe and persistent, and turn a chosen release into a playable link.

**Lives in:**
- `account-store.js`: tries several folders, verifies each write by reading it back, and reports where the key went (never the key itself).
- `resolve.js`: jobs that add a magnet, wait, list files, pick the episode file, then unlock. Each job can be cancelled, and can ask for a file or a quality.
- The PIN routes in `server.js`.

**State:**

| Item | Evidence |
| --- | --- |
| PIN pairing | Desktop (fake AllDebrid); web app TV (user) |
| Persistent storage with verified writes and per-folder diagnostics | Unit tests; **TV restart test pending** |
| Resolve jobs with progress, cancelling, and asking for a file or quality | Unit tests; Desktop (fixtures) |
| Revoked key detected; asks to pair again | Overwrite is unit-tested; detection only reviewed in code |
| *(1.2)* `POST /api/account/unpair` forgets the key and writes an unpaired mark so the shared web-app pairing is not reloaded; nothing is deleted (G4) | Desktop (route test) |

**Gaps:**
- Persistence for the native app (`com.muhib.bloob`) on the TV is unproven.
- There is no "already cached" check before resolving.

**Next:**
- **18.1** On the TV: pair, quit, relaunch, and check Settings says "saved on this TV" (`TV-TEST.md` §4).
- **18.2** Turn a link-expired error into a re-resolve, for **9.5**.
- **18.3** Check AllDebrid availability without sending the key to third parties. Look at what AllDebrid's own API offers now.

### 19. Progress and settings store

**Job:** remember where each video was left, what was watched, and the viewer's choices.

**Lives in:** `progress.js` (`progress.json`, `settings.json` in the service data folder).

**State:**

| Item | Evidence |
| --- | --- |
| Position, duration and next episode, with its air date | Unit tests; Desktop |
| *(2.1)* Progress v2: session and `seq` ordering, with late or retired-session reports rejected. Completion policy in `completion.js`. Edition per checkpoint, version intent per show, and dismissal (X1.1, X1.3, X3.1) | Desktop (unit tests) |
| *(2.1)* Continue tiers: unfinished, *New episode*, next, then caught up with the air day. Finished films and dismissed titles are left out. `next.js` gives each title's target in `/api/meta` (X2.1) | Desktop (unit tests, tour) |
| Settings: preferred language, recent searches | Desktop |
| *(1.2)* Settings `subtitleLanguage`, `reduceMotion`, `autoplayNext`, `welcomeSeen`; wrong types ignored | Desktop (tests) |

**Gaps:**
- One profile.
- The file grows forever.
- No list field for My list.

**Next:**
- **19.1** Keep only the latest 500 progress entries.
- **19.2** Settings fields for **8.1**: subtitle language, autoplay next.
- **19.3** A `list` of saved titles, for **1.3**.

---

## Foundation

### 20. Platform and packaging

**Job:** build for the TV and ship one installable package.

**Lives in:**
- `native/CMakeLists.txt`.
- `native/scripts/build.sh`: tests, cross-build, ABI guard, service staging, IPK.
- `native/package/appinfo.json`.
- The community webOS NDK (openlgtv buildroot-nc4).

**State:**

| Item | Evidence |
| --- | --- |
| ARM EABI5 binary, static C++ runtime, needs only GLIBC_2.4 | TV ✓ |
| IPK with the app, fonts, test clip and service | TV ✓ (0.3, 0.4 installed) |
| `native/artifacts/com.muhib.bloob_1.1.0_arm.ipk` | Built; not yet installed |
| *(1.2)* IPK repacked after `ares-package`: directories and executables 755, files 644; the build fails on any writable entry (F2) | Desktop (build) |
| *(1.2)* `scripts/acg-lookup.sh`: read-only lookup of the TV's permission groups; `requiredACG` deliberately not guessed (P1) | Untested |

**Gaps:**
- No `requiredACG`. It works today on a developer-mode TV but may be needed later.
- The IPK is committed by hand.

**Next:**
- **20.1** Add `requiredACG` for the luna services we call, once the TV shows which are needed.

### 21. Testing and tooling

**Job:** prove changes before they reach the TV, and make the TV pass quick.

**Lives in:**
- `native/tests/session_test.cpp`: session and JSON.
- The `tv-app` Vitest suite: 68 tests across the service, web Buna and the ranker.
- `native/scripts/ui-tour.sh`: the whole app, scripted, with fixtures, including quick-menu, Home and pipeline-lost events.
- `native/scripts/ui-snapshots.sh`: player states.
- `native/tests/fixtures/dev-service.js`: fake indexer and AllDebrid. It is never packaged.
- `native/TV-TEST.md`.

**State:**

| Item | Evidence |
| --- | --- |
| Native tests: 10 groups (session clock, seek, stall, errors, tracks, end, JSON) | Pass |
| Vitest suite (68) | Pass |
| Desktop tour: 29 renders, plus lifecycle checks in its log | Desktop |
| TV automation (key injection, screenshots) | Untested; procedure in `TV-TEST.md` §5 |

**Gaps:**
- The tour only takes pictures. Nothing fails if a screen breaks.
- No CI.
- No model-parsing tests (see **14.1**).

**Next:**
- **21.1** Make the tour assert its log: `push player`, `first frame`, `landed`, `reloading at <non-zero>`, `pop details`. Fail the script if any is missing.
- **21.2** A GitHub Actions job running the native tests, the Vitest suite and the host build. The cross-build needs the NDK; cache it or skip that step.

### 22. Observability

**Job:** know what happened on the TV without guessing.

**Lives in:**
- `src/log.*`: `/tmp/bloob-native.log`, with links reduced to their host.
- Diagnostics in **8**.
- `/tmp/bloob-service.log` when the fallback start is used.

**State:**

| Item | Evidence |
| --- | --- |
| Log of navigation, media commands and events, first frame, seek landing, reloads | TV ✓ (0.3/0.4 logs read over SSH) |
| URLs reduced to host; no keys | By design, reviewed |
| *(1.2)* `[perf]` every 5 s while drawing: key-to-present p50/p95/max, render p50/p95/max, frames over 33 ms; `[launch]` and `[qoe]` lines (U1) | Desktop |
| *(2.4, R5d)* Runtime inspector: `/tmp/bloob-state.json` once a second (frames and their stages and causes, wakes, worker queues, Luna records, GPU budget by class, artwork pipeline, shapes, process memory, allocations per frame, the service's memory). `scripts/tv-inspect.sh` reads it, read-only. `[perf]` now names key→submit and adds stage p95s and allocations per frame; repeated failures are rate-limited in the log | Desktop |

**Gaps:**
- No metrics summary per session (22.1).

**Next:**
- **22.1** End-of-session summary line: first frame, seeks and their landing error, stalls, reloads, duration watched. Include the streaming quality metrics that predict abandonment: startup time (target under 2 s), **rebuffering ratio** (time stalled divided by time watched; target under 1%), and whether the start failed.
- **22.2** *(Partly done in 2.4: the inspector reports memory and frame stages.)* Sampling script for the native app, read-only over SSH, like `research/live-episode-study.md`: threads, resident memory and CPU during browse, play, pause and after exit.

### 23. External services

| Service | Used for | Layer | Risk |
| --- | --- | --- | --- |
| Cinemeta | Catalogue, metadata, search | 16 | Third-party, unversioned; licensing review needed |
| Metahub | Posters, backdrops, logos | 13, 16 | Same |
| Torrentio | Candidate releases | 17 | Cloudflare may block some networks; terms and rate limits apply |
| AllDebrid | Pairing, resolving, links | 18 | Account-bound. Links are bearer secrets; never log them. |
| LG uMediaServer, luna, SDL | Playback, system, window | 9, 10, 14 | Firmware updates can change behaviour |

### 24. Web app (legacy, `tv-app/src`)

**Job:** the first Bloob, built with SolidJS and web Buna (`com.muhib.tvprototype`, 0.2.0). It shares the service code with the native app.

**State:** installed on the TV. A real source played there with HDR and Atmos (user report). It is no longer the main track.

**Next:**
- **24.1** Decide: freeze it as a fallback, or retire it. Retiring it means leaving it installed; nothing is deleted on the TV.

---

## Suggested order

The TV has not seen 1.1 yet, so the first increments are measurement. After that, fix what the TV shows.

1. **1.1, 9.1, 18.1, 2.1:** the TV pass in `TV-TEST.md`. It costs one evening and decides most of what follows.
2. **5.1:** season pills that scroll. This is a bug any long show will hit.
3. **9.3, 9.2:** keep the viewer's tracks across a reconnect, and start in their language.
4. **12.1:** draw any character Inter has, because titles and episode names are not all Latin-1.
5. **21.1, 14.1:** make the tour fail when something breaks, and test the model parsers.
6. **10.2, 22.1:** a reliable last save, and a per-session summary in the log.
7. After that, choose between features (**1.2, 1.3, 3.1, 7.1, 8.1**) and quality (**6.1/17.1** tuning) based on what the sofa test showed.
