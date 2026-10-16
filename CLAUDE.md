# Bloob and Buna: complete project handoff

Read this file before changing the application. This repository contains an independent LG webOS TV streaming prototype, research notes about a reference app called **Elements of A**, and a plan for **Buna**, our own playback and UI runtime. The user wants a polished television experience with excellent browsing, source choice, reliable playback, accurate seeking, working audio/subtitle controls, and efficient use of the LG media pipeline. The user explicitly wants no protected-content investigation. Do not present static code analysis as a live trace.

## Current review handoff — 15 October 2026

Before choosing the next engine work, read [the independent branch review](docs/Claude-branch-review-2026-10-15.md) and [the review and planning instructions for Claude](docs/Claude-review-and-planning-handoff.md). The review examined commit `8790488` on `claude/sleepy-ptolemy-o87vgv`; it identifies ten concrete reliability findings as well as strengths worth preserving. Verify each finding against the current code, correct anything unsupported or already fixed, then write the implementation plan specified in the handoff. **The current request is to review and plan; stop before implementing application changes.** Historical completion claims below are not substitutes for this verification.

**Done (15 October 2026, docs only):** [`docs/Claude-review-validation.md`](docs/Claude-review-validation.md) confirms all ten findings at `fb4f412`, with corrections and five additional findings (A1–A5). [`native/RELIABILITY-IMPLEMENTATION-PLAN.md`](native/RELIABILITY-IMPLEMENTATION-PLAN.md) sequences the fixes as R1–R7 (all planned, none implemented) and comes before `native/ENGINE-PLAN.md`, whose status marks were corrected. Start implementation with R1.

## The user's actual goal

Build **Bloob**, a distinct webOS application with:

1. A calm, premium TV interface: clear information hierarchy, spacious artwork, predictable focus, minimal player chrome, and graceful loading/error states. Take organizational and performance lessons from Elements of A; create original assets, text, and implementation. **Owner's rule (15 October 2026): copy Elements of A's measured design values (spacing, padding, sizes, radii, type, opacity, colour, timing, geometry) wherever the studies record them, with each one listed in `native/TOKENS.md`. Never copy code, artwork, icons, fonts, written text or branding.**
2. A direct catalog-to-playback path: browse films and series, pick an episode, discover candidate sources, resolve an authorized source through AllDebrid, select the correct file or quality when needed, and play through the TV media path.
3. **Buna**, an app-owned runtime layer that coordinates navigation, focus, playback sessions, source adapters, media state, tracks, metrics, and lifecycle. It should be independent, testable, and able to choose a web video backend now and a more capable native or segmented backend later if evidence justifies one.
4. Measured performance: fast first frame, stable HDR/audio output when the TV and source support it, smooth seeking, low idle overhead, memory release after exit, and no repeated work that harms TV responsiveness.

The current prototype is a vertical slice, not the finished product. The user has seen a high-quality resolved stream play with HDR and Atmos on this TV, but criticized the old player overlay and source chooser and had to re-pair AllDebrid at every start. Version 0.2.0 (12 October 2026) answers all three: Buna now owns playback (`tv-app/src/buna/`), the source screen is recommendation-first, and pairing storage tries several locations, verifies each write, and reports where the key went. All of it is covered by unit tests and a key-driven end-to-end run in desktop Chromium. **None of it has been verified on the TV yet**; see "TV verification" in `tv-app/README.md`.

## Native track (Buna N2, 13 October 2026; 1.1 UI pass the same day; 1.2.0 and 2.0.0 Glass on 14 October; 2.1.0 continuity on 15 October)

The owner chose native. `native/` is a C++ webOS app (`com.muhib.bloob`, "Bloob Native") built with the community webOS NDK (openlgtv buildroot-nc4, the toolchain Moonlight TV uses).

- **What it contains:** the whole app, not a test host.
  - Tabs: Search, Home, Films, Series, Settings.
  - Home: an artwork-led hero, then shelves.
  - Details: the page continues into seasons and episodes.
  - *Choose how to watch*: a best-match card, other versions, release details, AllDebrid PIN pairing and resolve progress.
  - Player: quiet during viewing, with seek preview, real track panels and up next.
  - Settings.
- **App map (start here for any change):** `native/APP-MAP.md` splits the app into 24 layers across experience, engine, service and foundation. Each layer has its job, files, state with evidence, gaps, and numbered increments such as **9.3**. Work one layer per change, cite the increment ID in the commit, and update that layer's state when the change lands or is tested on the TV. `native/QUALITY.md` defines quality (ISO/IEC 25010, Nielsen response limits, streaming quality of experience, and TV 10-foot guidance), scores every layer on seven dimensions, and ranks them. Re-score a layer when it changes.
- **UX implementation map:** `native/IMPLEMENTATION-UX.md` turns the Elements of A UI studies (now in `research/`, from `bloob-v1` commits `32c2ebc` and `59348e3`) into build items U0 to U9, with logic, code, tests and TV checks. Its *Status in 1.2.0* table lists the whole consolidated list the owner asked for in one go (F1–F8 fixes, P1–P6 platform items, U0–U10, G1–G5), where each landed and its evidence.
- **1.2.0 (14 October 2026, built, not yet on the TV):** tracks restored after reconnects and preferred audio/subtitle languages applied; start position in the load request with seek fallback (unverified option); `policyAction` handled; reduced seek HUD and timeout classes; subtitle position (undocumented call, switches itself off); clock and "Ends at"; next episode prepared in the credits; TV format warnings in the ranker (`tv-app/service/tv-profile.js`: DTS, TrueHD Atmos, disc Dolby Vision, bitrate against 95 Mbps Ethernet); one Play owner (`native/src/play.*`); scrolling season strip; search keyboard that gives way; Settings with subtitles, autoplay, Reduce motion, Disconnect (`POST /api/account/unpair`, which writes an unpaired mark and deletes nothing) and About; effects tiers; 22 px type floor; glyphs beyond Latin-1; `[launch]`, `[perf]` and `[qoe]` log lines; IPK modes fixed to 755/644 by `native/scripts/fix-ipk-modes.py`. `requiredACG` is still unset: run the read-only `native/scripts/acg-lookup.sh` on the TV first. QA work (tour assertions, CI, TV automation) was skipped at the owner's request.
- **2.0.0 Glass (14 October 2026, built, not yet on the TV):** the owner chose **Glass** from the concept canvas (artifact "Bloob UI concepts: Lantern and Glass") and asked for a full overhaul with a proper plan. `native/GLASS-PLAN.md` is that plan, phases G0–G8 with a status table and commits.
  - **Visual system.** It follows Apple's public tvOS type scale (floor 23 px), safe area (80/60) and 5-column grid. Text uses label roles, with *Increase contrast*.
  - **Glass materials.** Frosted glass is baked per panel from the blurred backdrop on image workers, with a soft knee so white text stays readable on bright art. Glass over video is smoked, because the video plane cannot be read. A solid fallback covers Lite, *Reduce transparency* and baking. Scrims adapt to the artwork's brightness.
  - **Direct Play.** Play from Home, the title page, an episode or Up next goes straight to the best version, showing only a progress sheet; Cancel returns to where Play was pressed. Films show a frosted *How to watch* panel on the title page. The old chooser is now *All versions*.
  - **Player.** A smoked bar with 5/8 s timers, 300 ms fades and a 400 ms seek commit.
  - **Spoken focus (G7).** With LG Audio Guidance on, focus is spoken as label, role and position, via `native/src/speech.*`. It uses the documented `audioGuidance` setting and `com.webos.service.tts/speak`, unverified on this TV, and the app declares `accessibility.supportsAudioGuidance`.
  - **Clean-up.** The 1.x type names, components and tokens are gone; `native/DESIGN.md` is rewritten for Glass. `native/TV-TEST.md` §8 is the 2.0 TV pass.
- **UX plan (next, 15 October 2026):** after a full series on 2.0.0, the owner judged the look right but the behaviour not "dynamic, intuitive or intelligent". On that viewing, each next episode chose its own version (often HD), and there was no way to choose a version for an episode. `native/UX-PLAN.md` is the plan, built from `research/Elements-of-A-UX-learning-frameworks.md` and its companion response, and `research/Swiftfin-tvOS-behaviour-for-Bloob.md`. It labels every claim as observed in Elements of A [O], read in Swiftfin [S], proposed [P] or Bloob today [B], and lists ten located causes in Bloob's code. It defines twelve contracts, X1–X12:
  - progress and completion;
  - the next-action resolver;
  - version preference and edition identity;
  - track intent;
  - the navigation and Back map;
  - place anchors;
  - bounded asynchronous work;
  - information budgets;
  - component states;
  - the player;
  - recovery;
  - accessibility.
  Work them in its stated order and tick its status table.
- **2.1.0 continuity (15 October 2026, built, not yet on the TV):** the first three UX-PLAN contracts, built in answer to the episode report.
  - **Progress v2 (X1).** Reports carry a session and sequence, so late reports are dropped. Completion is a policy in `tv-app/service/completion.js`. Continue has tiers, and a title can be removed from Continue.
  - **Next action (X2).** `tv-app/service/next.js` gives `target` in `/api/meta`: resume, next, start, or caught up with the air day. An unaired episode is never offered.
  - **Version continuity (X3).** A show keeps to the version last played or hand-picked. Seeders can no longer outvote resolution. The episode versions list has sections and pack tags, and the starting sheet has *Other versions*.
  - **Remaining.** Edition-aware resume and track intent are still to do.
  - **TV pass.** `native/TV-TEST.md` §9; the IPK is `native/artifacts/com.muhib.bloob_2.1.0_arm.ipk`.
- **2.2.0 Elements of A values (15 October 2026, built, not yet on the TV).** Every value the studies record is now applied and listed in `native/TOKENS.md`, which `theme.h` cites as `[EoA Xn]`:
  - label opacities (100/50/30/20%) and page grey `#2C2C2E`;
  - pills for seasons and tabs (66 high, 30 padding, 8 px focus growth, candidate 10%), with 16 px between controls;
  - focus timing 85/230 ms, a 300 ms focus rest, and screen-reader doubled player delays;
  - the 0.55/0.2 artwork blend under card titles, and the 10% settings platter.

  Buttons now measure once and centre their label on its capital-letter height, which fixes uneven margins. `native/scripts/eoa-token-export.py` is a read-only script for the owner's PC. It exports the values the studies don't record (button, card, radius, easing) as numbers only, never code.
- **2.3.0 full Elements of A token pass (15 October 2026, built, not yet on the TV).** It applies the full catalogue `research/Elements-of-A-design-tokens.md` (from `bloob-v1` `a505b7b`), row by row in `native/TOKENS.md`:
  - type weights;
  - buttons: 66 high, padding 30, +8 width on focus, White90, Dark/Default/Player variants;
  - cards: radius 20, +50 width on focus, two-level shadows;
  - the hero and title page: logo 420×126, Caption1 metadata and description, 742 first shelf, 321 pitch, the reference gradient stops and overlay;
  - the player: no panel, gradient to 60%, 14 px bar, Title2 Gray92 header, 66-high controls 25 apart, 454-wide track menu, 454×460 Up next;
  - search: 432-wide keyboard, 200 ms query;
  - settings rows, notifications;
  - motion: ease-in/ease-out timed curves, springs 30/150/1, 300 ms linear crossfades.

  Glass stays on the tab bar, How to watch and sheets.
- **Engine upgrade (next, 15 October 2026).** The target is `docs/Buna-engine-specification.md`, the finished-system spec from `bloob-v1` `73da849`. `native/ENGINE-PLAN.md` holds:
  - a section-by-section conformance table, marked Have / Partial / Gap;
  - phases E1–E8, in order:
    1. session truth;
    2. inspector;
    3. watch attempts and leases;
    4. events and frames;
    5. layout and focus runtime;
    6. resources and text;
    7. sample route, only if a measured gap appears;
    8. accessibility.

  A retained scene and a separate render thread are deferred until TV measurements show a frame problem.
- **2.4.0 runtime resources (16 October 2026, built, not yet on the TV).** The owner asked for every mechanism in the Elements of A CPU, memory and GPU study (`research/Elements-of-A-CPU-memory-GPU-map.md`) to be built into Bloob. `native/RUNTIME-RESOURCE-ADOPTION.md` maps all 52 findings to code and records each one's state.
  - **Frames and events (R5a).** Wakes are coalesced notifications, and only a visible change repaints, so the media clock no longer redraws the screen. Animation time is the median of three intervals. Pumps are bounded, the renderer state is cached, and shelf cards are drawn in batched passes.
  - **Resources (R5b).** One GPU budget is checked before any texture is created: 96 MB while browsing, 56 MB during playback or low system memory, 24 MB in the background. Eviction goes by relevance, then frame age. Background releases every texture and stops new fetches. The service trims its image cache to the same policy. Worker threads are named.
  - **Artwork (R5c).** Fetch and decode are separate stages, and each source picture is decoded once for all its variants. Requests are ordered by relevance (visible, near, prefetch). Stale work is cancelled at each stage, and uploads are paced.
  - **Retained layers (R6r).** Transitions paint the screen underneath once.
  - **Inspector (R5d).** `/tmp/bloob-state.json`, read with `scripts/tv-inspect.sh`.
  - **Found and fixed:** the worker pools had never woken the UI thread (a completion was tested after being moved), so replies and art could wait up to 1 s.
  - **TV pass:** `native/TV-TEST.md` §10.
- **2.5.0 sources, picture and subtitles (16 October 2026, built, not yet on the TV).** The owner's report on 2.4: subtitles far too large with no size option, Dolby Vision still showing as HDR, a wish for SDR, a versions list that isn't intuitive, and a question about better scrapers than Torrentio. Research is in `research/Source-indexes-for-Bloob.md`, `research/Dolby-Vision-on-webOS.md` and `research/Subtitle-styling-on-webOS.md`.
  - **Sources.** `tv-app/service/sources.js` asks Torrentio, TorrentsDB, StremThru's public Torznab (which carries Debrid Media Manager hash lists: many 4K remux, DV and Atmos releases), The Pirate Bay's API and EZTV. All are keyed by IMDb ID, and none is sent the AllDebrid key. Results merge by hash. The reply goes back 2.5 s after Torrentio answers at most, and later answers join the cache. Settings > Sources can return to Torrentio only.
  - **Picture.** Settings > Picture: Best available, HDR10 (skip Dolby Vision) or Prefer SDR (outranks resolution). `tv-profile.js` says webOS 24 reads Dolby Vision only from MP4/TS, so MKV Dolby Vision is labelled as the HDR10 it plays as. "DV" without an HDR10 layer (probably profile 5) is warned about. As an experiment, Dolby Vision loads carry Kodi's `DolbyHdrInfo` option with a fallback; the `[qoe] picture` log line reports the result. The real fix (an MP4/HLS remux, or the Starfish ES route) is not built.
  - **Subtitles.** Settings > Subtitle size (TV levels 0–4, default the smallest) and Subtitle background, sent with `setSubtitleFontSize` and friends after every enable.
  - **Versions list.** Filter pills by resolution with counts, sections by kind of release (Remux, Blu-ray, Web download…), two-line rows with source, size and group, and *Found by* in release details.
  - **TV pass:** `native/TV-TEST.md` §11; the IPK is `native/artifacts/com.muhib.bloob_2.5.0_arm.ipk`.
- **Design (2.0, Glass):** the visual system, with materials, tokens, type, focus, motion, states, the spoken interface and per-screen layouts, is in `native/DESIGN.md`. The tokens live in `native/src/theme.h`, the type scale, label roles and components in `ui.cpp`, and the frost bake in `images.cpp`.
- **Rendering and playback:**
  - The TV's SDL2 provides the window and remote. Bloob draws with its own renderer: Inter via stb_truetype with oversampling and gamma, exact-size anti-aliased shapes, and artwork via stb_image scaled on worker threads, on a 1920×1080 offscreen canvas.
  - Playback goes through LG uMediaServer over luna (`com.webos.media`), in an exported hardware video window under the UI.
- **Lifecycle (1.1):**
  - Focus loss alone, such as the LG quick menu, leaves playback alone.
  - Hidden or background pauses and saves.
  - Foreground sends `notifyForeground`.
  - A released pipeline (`unloadCompleted`, `released` or `suspended`, or a refused command) reopens at the same position, at most three times a minute.
  - This answers the 0.4 user report that playback died after the brightness menu. It is not yet verified on the TV.
- **Service:** the app bundles its own copy of the Bloob service (`tv-app/service`, staged as `com.muhib.bloob.service` on port 8790). The app talks to it over plain localhost HTTP. The service also serves:
  - the catalogue, search and an artwork proxy;
  - ranked sources and AllDebrid resolve jobs;
  - watch progress and settings.
  If the bus will not start the service, the app runs it directly with the TV's Node and stops it on exit.
- **Build and test scripts:**
  - `native/scripts/build.sh`: tests, cross-build, ABI guards (no GLIBCXX, and since 2.4 no glibc symbol newer than GLIBC_2.4), service staging, IPK with fixed modes (`native/artifacts/com.muhib.bloob_2.5.0_arm.ipk`).
  - `native/scripts/tv-inspect.sh`: reads the runtime inspector snapshot `/tmp/bloob-state.json` from the TV, read-only.
  - `native/scripts/ui-tour.sh`: drives the whole app on the desktop through the acceptance task, plus quick-menu, Home and pipeline-lost events, against the real service with fixture sources.
  - `native/scripts/ui-snapshots.sh`: covers player states.
  - `native/TV-TEST.md`: the TV pass, with the acceptance task, viewing distance and lifecycle.

**TV status:**

- **0.3.x verified on the TV:** OLED55G26LA, webOS 9.2.2. Window, remote, luna, uMediaServer load and the exported video window worked, and the test clip played.
- **0.4.0 ran on the TV (user report):**
  - The app worked.
  - Fonts showed jaggies.
  - Some UI was buggy.
  - Playback died after the LG quick menu.
  - The look was not yet the calm, Apple-like feel the user wants.
- **1.1.0 is the response to that report and has not yet run on the TV.** Neither have 1.2.0 (`native/TV-TEST.md` §7), 2.0.0 Glass (§8) or 2.1.0 continuity (§9).
- **2.2.0 ran on the TV (user report, 15 October 2026).** It was "a huge improvement, extremely smooth". Four problems were reported:
  - Subtitles showed *Off* until you scrolled.
  - DV titles came up as HDR every time.
  - The UI did nothing while the Magic Remote cursor was on screen.
  - Audio choices were missing.

  2.2.1 fixes the track panel, which showed one row too few. It maps pointer clicks, the wheel and the right button to OK, Up/Down and Back. It names the HDR format the TV reports and logs `[qoe] picture: version claims Dolby Vision, TV reports …`. It stops ranking 4K Blu-ray encodes as disc Dolby Vision. Whether the TV engages Dolby Vision for MKV over uMediaServer is still unknown: read that log line from `/tmp/bloob-native.log` on the TV.

See `native/README.md`. Elements of A is UI inspiration only. Technical references are the open-source Moonlight TV and Kodi webOS code, read but not copied. The web app (`tv-app/`) remains installed and working.

## Repository and local-only material

| Path | Purpose |
| --- | --- |
| `tv-app/src/` | SolidJS TV UI, focus/navigation, catalog and playback screens |
| `tv-app/service/` | Local Node service for UI hosting, source discovery, AllDebrid PIN pairing, resolution, and a range-enabled test clip |
| `tv-app/app/` | webOS app manifest and launch shell |
| `tv-app/scripts/` | TV inspection and playback probes for our own app |
| `tv-app/artifacts/` | Last known installable prototype IPK; rebuild after source changes |
| `research/Elements-of-A-system-map.md` | Reference app organization, watch path, native adapter map, and evidence limits |
| `research/Elements-of-A-UI-UX-code-study.md`, `research/Elements-of-A-UI-responsiveness.md` | Reference UI/UX organisation and responsiveness (static evidence), the basis of `native/IMPLEMENTATION-UX.md` |
| `research/Buna-player-study.md` | Focused player UI/session/seek/track analysis and translation ideas |
| `research/live-episode-study.md` | Paid-episode process sample and pause/resume comparison |
| `research/Luna-runtime-architecture.md` | Deep native/JS event, frame, graphics, and media runtime map with evidence limits |
| `research/TV-access-and-observation.md` | Root SSH access, read-only sampling, remote-control status, and trace procedure |
| `research/ghidra-scripts/` | Scripts used to inventory focused native functions |
| `research/*.tsv` | Function and vtable inventories, not executable code |

The local checkout also contains extracted reference-app files, an ARM binary, formatted code, targeted decompiler output, analysis databases, third-party tooling, and a raw process-stat capture. They are intentionally excluded from this public repository. Do not add those files or any account material. The public notes summarize behavior and preserve evidence addresses without publishing the source implementation. The repository's source of truth is the current code and the three linked research notes. Read them before asserting a behavior.

For runtime work, read `research/Luna-runtime-architecture.md` and `research/TV-access-and-observation.md` as well. `research/runtime-symbols.tsv` is a broad, nonprotected symbol inventory generated by `research/ghidra-scripts/InventoryRuntimeSymbols.java`; it is not a live call trace. The local decompiler exports are untracked. The reference app's bundled Luna runtime is distinct from webOS `luna://` services. TV second-screen pairing, key injection, and screenshot capture have not yet been validated. The owner explicitly forbids deleting anything on the TV.

Publication caveat: `main` was replaced with a clean root commit to remove the prior wording from branch history, and old local Git objects were pruned. GitHub still served the displaced commit by direct API lookup after the force push. A branch rewrite cannot guarantee server-side deletion of detached Git objects or caches. If strict server-side erasure is required, the repository owner must seek GitHub's purge process; do not claim the old object has disappeared merely because `main` is clean.

## Build and deployment state

- `tv-app` is a Vite 8 + SolidJS 1.9 + TypeScript project. Local checks: `npm run typecheck`, `npm test` (Vitest: Buna session, ranker, resolver, release parser, account store), and `npm run test:e2e` (real app + service + Buna in Chromium, driven by remote keys; needs ffmpeg). `npm run check` runs all of them.
- From `tv-app`, run `npm install` when dependencies are absent. For a local UI/service check, start `node service/server.js`, then `npm run dev`. Vite forwards `/api` to the local service on port 8787.
- For a TV package, run `npm run package` from `tv-app`: it builds, stages `dist/` into `service/www/` (removing old bundles first), and writes `artifacts/com.muhib.tvprototype_<version>_all.ipk`. The app ID is `com.muhib.tvprototype` (title Bloob); its service ID is `com.muhib.tvprototype.service`.
- The current committed IPK is `tv-app/artifacts/com.muhib.tvprototype_0.2.0_all.ipk`. It is a snapshot, not proof that later source changes are installed. Rebuild and inspect package contents before a TV test. Tests live in `tv-app/tests/` and `src/**/*.test.ts` so they never ship in the service folder.
- The TV's JS service runs an older Node than the dev machine. Keep `service/*.js` free of `?.` and `??`. The UI targets Chrome 87 (webOS 22); `tsconfig` uses the ES2020 library so newer APIs fail the typecheck.
- The rooted TV has an existing webOS CLI device alias `rootedTV`. Keep connection secrets out of scripts, docs, shell output, and Git. The user said **do not delete anything on the TV**. Use read-only inspection for reference-app study. For our app, use deliberate install/launch operations only when needed. Never remove TV files, packages, accounts, or caches merely to tidy up.
- The workspace directory name reflects the origin of the local research material. It is not the Bloob app ID and must not be used as a product identifier.

## Current Bloob architecture: exact flow

### Browsing and navigation

- `tv-app/src/App.tsx` owns the top-level flow: `Home` -> `Details` -> `Sources` -> `Player`. Every layer stays mounted (hidden) under the next, so Back restores focus and scroll without refetching. Hidden layers use `visibility: hidden` after their fade so they stop painting during playback. The state is `title`, `sourcesFor`, and a typed `playback: { intent: PlayIntent; source: ResolvedSource }`.
- `tv-app/src/keys.ts` translates LG D-pad, Back, and transport keys (415 Play, 19 Pause, 413 Stop, 417 FF, 412 RW; desktop 179/178). It maintains a handler stack so only the top screen handles one key event. Inside a text field, Backspace and left/right keep their editing meaning. Preserve the single-owner principle.
- `tv-app/src/screens/Home.tsx`, `Details.tsx`, `Sources.tsx`, and `Player.tsx` own their screen composition. `Details` loads full metadata, groups episodes by season, puts specials last, and labels the main button with the episode it actually plays (first episode of the selected season).
- `tv-app/src/data/cinemeta.ts` supplies movie/series metadata from Cinemeta, and art URLs from Metahub. It builds six catalog rows in parallel and keeps the bundled test row. Tile art uses small image variants and backdrop art uses medium variants to avoid needlessly decoding very large textures on the TV. External catalog and art availability are dependencies; add caching, error states, and licensing review before product release.
- The UI remains a provisional design. The user considers the existing source chooser and player controls inadequate. Do a deliberate remote-first design pass, with visual review on the actual TV, instead of polishing the existing implementation by small cosmetic changes.

### Source discovery and resolution

- `tv-app/service/server.js` is a local HTTP service listening on `127.0.0.1:8787` on the TV. The app shell launches the service; it serves the UI and `/api/*` routes. It is also runnable locally with plain Node.
- `POST /api/sources/search` accepts a `movie` or `series` type plus an IMDb-style title or episode ID (series require an episode). The current provider is hosted Torrentio. `service/release.js` turns each stream into a typed `SourceCandidate`: resolution, HDR (DV/HDR10+/HDR10/HDR/HLG), codec, audio (Atmos/TrueHD/DTS variants/DD+/…), channels, source type (REMUX/BluRay/WEB-DL/WEBRip/HDTV/DVD/CAM/TS/SCR), size, seeders, flag languages, multi-audio, pack and file-name hints. It accepts both `title` and `description` formats, de-duplicates by hash and file index, and returns up to 60. Every field is a claim parsed from names (`claimsFrom: 'release-name'`), not a probe. Torrentio blocks some datacenter IPs with Cloudflare; the TV's home connection was fine before.
- `src/sources/ranker.ts` scores candidates with explainable weights. Resolution leads. DV/HDR, Atmos/lossless and seeder health add. REMUX bitrate, packs, few seeders, and a foreign-only language tag subtract. Cinema recordings and zero-seeder torrents are never recommended. It returns one recommendation with a one-line reason, plus groups by resolution. Tune weights after real use; they are preferences, not facts.
- `src/screens/Sources.tsx` is recommendation-first. It has a large Recommended card (plain-language headline like "4K · Dolby Vision · Atmos", facts, reason, warnings, raw release name in small type), grouped alternatives (three per group, expandable), manual link/magnet entry, and an account row. Pressing Play while unpaired runs pairing and then continues straight to that source. Resolving shows a cancellable step list with AllDebrid download progress. Failures keep the list on screen and say what happened. Search results are cached for ten minutes per title or episode.
- Pairing routes: `POST /api/pin/start` requests a device PIN; the UI polls `POST /api/pin/check` every five seconds and shows a countdown and renewal. The AllDebrid API key remains in the local service, not in frontend state or the IPK. `GET /api/account` returns `paired`, `persistent`, `storage`, `durable`, and per-location `attempts` (location and error code, never the key).
- `tv-app/service/account-store.js` (likely cause of the re-pair complaint: only one directory was ever tried, and nobody checked it was writable). It now tries `/media/internal/com.muhib.tvprototype` (unchanged, so existing saves load), `~/.bloob`, the service folder (lost on reinstall), then the temp dir (lost on reboot). It writes via temp file + rename, reads back to verify, and reports the result. Revoked keys (`AUTH_*` from AllDebrid) are overwritten, not deleted, and the UI asks to pair again. Confirm with one real pair, an app restart, and `node scripts/inspect-tv.cjs <port> account` before calling it solved. Do not read or print the key when verifying.
- The service API accepts only `Host: 127.0.0.1|localhost` and `Content-Type: application/json` POSTs. This blocks cross-site form posts and DNS rebinding from other web content on the TV, which could otherwise use the paired account.
- Link routes: `POST /api/link/unlock` can yield a direct URL, delayed state, or quality options. `POST /api/link/quality` selects a quality. `POST /api/link/delayed` polls a delayed result. Magnet routes upload an info hash or magnet, poll readiness (with download progress and failure status), then list video files. `src/sources/resolver.ts` owns this flow and is cancellable at every wait. It picks the file in this order: the exact file the indexer named, then the only non-extra video, then the season/episode match (largest if several), then a clearly dominant largest file. Otherwise it asks. It hands Buna a `ResolvedSource` (URL in memory only, transport, file name, size, claims, candidate).
- A resolved URL is a sensitive bearer-like value. Keep it out of logs, screenshots of developer tools, telemetry, and Git. Validate URLs and avoid sending credentials to arbitrary hosts. Use only media the user is authorized to access.

### Current playback behavior (Buna, v0.2) and gaps

- `src/screens/Player.tsx` is a pure view. It renders a `<video>` element, calls `buna.open({ intent, source, video })`, mirrors `PlaybackState` through `useBunaState`, and sends commands (`play`, `pause`, `toggle`, `nudge`, `seekTo`, `commitPreview`, `cancelPreview`, `restart`, `selectAudio`, `selectSubtitle`, `retry`). It never reads media fields. `tv-app/README.md` lists Buna's files and guarantees.
- Clock: 250 ms sampling only while time can move (none while paused). `positionSource` and `durationSource` record provenance. A transient NaN duration keeps the last known value. The stall watchdog marks `buffering` when the clock stops for 3 s, even without a `waiting` event.
- Seek: direction keys move a preview, accelerating 10/30/60/120 s while held. One seek is committed 600 ms after the last press. The displayed position holds the target until the backend's `seeked` (or a settled clock reading, for TVs that drop `seeked`) confirms where it landed. A 15 s timeout returns to the media clock. Metrics record target, landing point, and settle time.
- HUD: full HUD on any key (6 s auto-hide while playing). A reduced timeline-only HUD appears when seeking starts from hidden. It is pinned while paused, ended, loading, in a menu, or on error. Back order: menu → seek preview → hide HUD → exit. OK while hidden toggles pause. A startup screen shows the title until the first presented frame (`requestVideoFrameCallback` where available). The buffering spinner is delayed 500 ms. Wall clock and end time are shown. Resume is automatic from a saved position (30 s minimum, below 95%) with a Start over control. The error screen offers Try again / Choose another source / Exit.
- Lifecycle: one live session (`BunaRuntime`). The session pauses and saves when webOS hides the app, and disposes on page hide. `dispose()` is idempotent and releases the decoder (`pause`, remove `src`, `load()`).
- Tracks: the web backend lists `audioTracks` and subtitle/caption `textTracks` when the engine exposes them. Capabilities say why not otherwise, marked `tv: 'unverified'`. External subtitles, thumbnails, frame-rate matching, and output info are reported unsupported. Do not fabricate selectable tracks or assume the web video backend can switch them.
- Measurement: `window.__buna.state()` and `window.__buna.metrics()` (resolve, metadata, first-frame, seek, stall, error, dispose; no URLs). `scripts/inspect-tv.cjs <port> buna` reads them over the web inspector.
- One resolved source reportedly played with HDR and Atmos on the user's TV. This is a user observation, not a measured codec/output-mode trace. Do not generalize to all containers, formats, or backend choices.
- The bundled Big Buck Bunny clip is 30 seconds, H.264/AAC at 480p, served with byte-range support. See `tv-app/service/www/media/CREDITS.txt` for its CC BY 3.0 attribution. It validates basic startup, pause, seek, and cleanup; it does not validate high-quality output.

## What Elements of A revealed

The reference app is an independently studied installed artifact. Its shipped code and native binary are local, untracked research inputs. **Static findings** and **observed TV behavior** must remain separate.

### UI and watch-action organization from shipped code

1. A shelf or product widget emits a semantic `Play` or `PlayWithModal` intent with a product/playable node. It does not create the player directly.
2. The root application resolves content type, selected episode, sign-in/profile/offer gates, playback context, and start position. It pushes a loading page, waits for its service, and lazy-loads the player page.
3. The player page asks a shared player store to create a playback session. The session owns player selection, state callbacks, clock, buffering, tracks, reporting, and lifecycle.
4. An HLS route uses a worker and custom `MediaSource`/`SourceBuffer` bridge. A progressive route uses `UriSource` and a native media player. The route is selected by URL/option and may stop a previous backend when switching.
5. The page draws video through its graphics context and draws its own overlay. The native layer contains `Media::MsePlayer`, `TrackPump`, and `LgEsPlayerPalImpl`, which prepare the LG ES pipeline and submit buffered samples. Native call edges and caveats are recorded in `research/Elements-of-A-system-map.md`.
6. The player session sends state to the UI, polls position-dependent state at 100 ms, and maintains seek, buffering, track, and post-play state. It also coordinates screen-saver suspension and teardown. The UI distinguishes full, reduced, and deck modes; short seek input previews a target and debounces the actual seek.
7. On exit, page disposal removes callbacks, ends the session, stops the backend, releases media resources, and may transition to a next-episode or post-play page.

The source app's `Luna.Media`/`Gfx` runtime is bundled with that app. webOS `luna://` services are separate. **Buna must implement its own public-facing runtime contract** and choose supported webOS backends; do not assume the reference app's bundled runtime can be linked or reused.

### Native and track specifics

- `Media::MsePlayer::playSource` stores source/options, changes state, sets up buffers, and attempts play. `TrackPump` reads buffered samples and calls the ES player. `LgEsPlayerPalImpl` creates an LG pipeline, gates startup on buffer state, submits samples with backpressure, applies video geometry, and clears resources on stop. These are focused decompiler conclusions, not proof each function ran in a given episode.
- Earlier live play/exit comparison found 74 app threads and about 358 MB resident during playback, then 48 threads and about 273 MB after exit. Video/audio source-buffer and queue threads plus selected LG media device descriptors disappeared after exit. This supports explicit session teardown.
- The HLS worker has audio/subtitle selection messages and a source-buffer shim. The progressive `UriSource` adapter's `setAudioTrack`, `setSubtitleTrack`, and `updateSubtitleTrack` methods are empty in shipped JavaScript; its thumbnail method yields no image. A generic track menu does not prove every source backend can provide track switching or thumbnails.
- The exact LG virtual method targets beyond identified adapter edges, selected rendition, buffer depth, output mode, decoder configuration, first-frame timing, and dropped frames remain unmeasured. The user requested no protected-content work.

### New paid-episode observation

On 12 October 2026 an episode was visibly playing while SSH sampled the app. Its process had 74 threads and 368–374 MB resident memory across five short samples. Thread names included separate audio/video `SourceBuffer`, queue, app-ES, sink, controller, and decoder workers. A one-minute `/proc` sample during a user-reported ~30-second pause/resume found process CPU tick increments mostly 7–10 per two seconds in the quieter window and 27–84 after the rise near 19:31:03 BST. This correlates with the reported resume and suggests the retained session does substantially less work while paused. The exact key event and internal pause callback were not captured. The video source-buffer and sink thread counters alone were too sparse to identify decoding behavior. Full detail and limits are in `research/live-episode-study.md`.

## Why Buna is required

**Status (v0.2): the session controller, web backend, capability model, clock and seek controller, metrics, and runtime below are implemented in `tv-app/src/buna/`. The rationale is kept for context.** The original `Player.tsx` bundled media commands, clock polling, remote input, focus, menus, state labels, and cleanup in one component. This is workable for a test clip but insufficient for multiple media backends and high-quality TV behavior. Buna should be an **app-owned layer** that preserves one consistent session contract while backends differ in seek, track, buffering, and output capabilities. The UI should consume Buna state instead of reading arbitrary media fields directly. The source chooser should hand Buna a typed, resolved source with metadata and expiry, not a bare URL.

Proposed module boundaries; implement incrementally and adapt after TV measurements:

```text
CatalogProvider -> title/episode models -> PlayIntentRouter -> SourceDiscovery
  -> SourceRanker -> DebridResolver -> ResolvedSource
  -> BunaSessionManager -> BackendAdapter (WebVideo first; HLS/native only if justified)
                    -> Clock + seek controller
                    -> Track/subtitle controller
                    -> Buffer and playback state
                    -> Lifecycle/cleanup
                    -> Metric events
  -> PlayerView + TransportBar + TrackMenus + StatusOverlay

RemoteInput -> FocusManager -> active screen command -> BunaSessionManager
```

Suggested typed contracts:

- `PlayIntent`: content ID/type, episode ID, start position, origin screen, and user action timestamp.
- `SourceCandidate`: provider, release identity, info hash or link, quality claims, size, codec/audio/HDR claims, peers/availability, language, and provenance. Preserve uncertainty.
- `ResolvedSource`: playable URL held only in memory, transport type, expected expiry, optional headers, container/codec hints, file identity, and subtitle candidates. Never put it in persistent UI logs.
- `BackendCapabilities`: seekable ranges, audio track selection, text track selection, external subtitle support, thumbnails, frame-rate matching, HDR/output information, and whether each capability was **tested on this TV**.
- `PlaybackState`: phase (`idle`, `resolving`, `loading`, `buffering`, `playing`, `paused`, `seeking`, `ended`, `error`), source, current time, duration, buffered/seekable ranges, actual vs preview seek target, tracks, selected tracks, error, and first-frame timestamp.
- `BunaBackend`: `prepare`, `play`, `pause`, `seek`, `selectAudio`, `selectSubtitle`, `getState`, `subscribe`, `stop`, `dispose`. Unsupported operations should return an explicit capability result.

Keep data flow one way: backend events update the session; session state updates the view. Remote commands go through an input router to the active screen/session. Avoid duplicate timers, global listeners per screen, and UI state that guesses the underlying clock. A seek preview may move the visual thumb immediately, but committed position should follow a backend acknowledgement or fresh media-clock observation. Back must have one owner at a time.

### Backend decision

Start with the current public HTML video path because it has already played a resolved file and reportedly delivered HDR/Atmos. Do not assume that means it meets track, subtitle, seek, trick-play, output matching, or telemetry requirements. Measure those properties on representative content. A segmented/HLS adapter or native LG ES integration is justified only for a concrete, measured gap that supported public APIs cannot close. Keep native investigation focused on ordinary media behavior and do not copy extracted implementation text into the app.

## Immediate work order for the next assistant

1. Read `research/Elements-of-A-system-map.md`, `research/Buna-player-study.md`, and `research/live-episode-study.md`. Inspect current `tv-app/src/screens/Player.tsx`, `Sources.tsx`, `keys.ts`, and the local service. Preserve the distinction between implemented features, user reports, static findings, and TV observations.
2. Finish the controlled runtime study. SSH `/proc` sampling works. A network-input service and capture service exist, and the TV's second-screen WebSocket port is reachable, but no paired input or screenshot client has been verified. Establish a least-privilege paired client if needed, keeping its bearer key out of Git. A TV prompt may require the user to accept it. Then capture visible control behavior and timestamps for play, pause, repeated seeking, audio/subtitle menus, Back, and exit. Do not claim autonomous control before it works.
3. **Install 0.2.0 and verify on the TV** (highest priority): pair once and restart (persistence), test clip, one authorized resolved source, `buna` metrics for first frame and seek landing, track exposure, HDR/audio observation, Back behavior, and decoder cleanup. If the account row says the TV refused to save, record the per-location error codes. Record only non-sensitive metrics and screenshots that do not disclose account data.
4. Buna follow-ups after TV data: set `tv` evidence on capabilities that were measured; tune the clock interval and stall threshold to observed event density; decide whether track switching needs an HLS/MSE or native backend (the `BunaRuntime` backend factory is the plug-in point); add external WebVTT subtitles if required.
5. Sources follow-ups: tune ranker weights with real results; move the indexer behind a provider interface in the service (the parser already isolates Torrentio's format); consider an availability check that does not send the AllDebrid key to third parties.
6. Create an original design system for browsing and playback: focus ring, spacing, typography, motion, image loading, HUD states, menus, and remote interaction rules. Test on the actual TV at viewing distance. Accessibility and response to every remote key matter more than a desktop screenshot.
7. Measure first frame, seek settle time, pause CPU, sustained memory, network/buffer behavior, and post-exit release for the same test material. Compare backend choices on equal content. Update the research ledger with observation timestamps and limitations.

## Known issues and checks before calling the slice complete

| Area | Current state | Completion evidence |
| --- | --- | --- |
| AllDebrid persistence | Multi-location verified storage with diagnostics (v0.2); TV restart test pending | Pair once, restart app/service, `GET /api/account` reports `paired: true`, `persistent: true`, `durable: true` without re-entering PIN |
| Source chooser | Recommendation-first redesign with ranker reasons, grouped alternatives, cancellable progress (v0.2); desktop-reviewed only | User can identify and start a recommended source without reading raw release jargon; alternate/file/quality decisions remain clear on the TV |
| Seek clock | Buna clock with provenance, preview/commit, ack-based landing, stall watchdog; verified in desktop Chromium | On real direct and segmented media on the TV, displayed time follows playback, `seek` metrics show `landed` close to `to`, post-seek video catches up, and end behavior is correct |
| Audio/subtitles | Capability-reported menus; often no exposed tracks | Menu shows actual available tracks, switching demonstrably changes output, external subtitles work if included as a requirement |
| Player aesthetic | Redesigned remote-first HUD (v0.2); desktop screenshots only | Remote-first design review and observed TV interaction pass |
| Backend efficiency | User observed smooth high-quality output; limited process snapshots | Measured first-frame, CPU, memory, seek, output mode, and cleanup on repeatable content |
| Reference runtime study | Strong static call map plus two limited TV observations | Timestamped key/UI/player/media trace without account or stream identifiers |

## Safe working rules

- The TV is rooted, but root access is for narrow inspection and our explicitly requested app deployment. **Do not delete any TV data or packages.** Do not alter the reference app. Do not collect account keys, passwords, full stream URLs, or protected-content details.
- Do not commit the local extracted executable, bundled code/assets, decompiler output, Ghidra databases, raw process-stat capture, downloaded tooling, or any token-bearing file. Read `.gitignore` and inspect staged content before every push. The IPK currently committed was scanned for the terminology being removed, but rebuilds need the same check.
- Keep the repository's public terminology consistent: call the studied reference **Elements of A**. Use **Buna** for our original runtime and **Bloob** for the product. Avoid source-app identifiers and executable names in tracked text, paths, generated packages, and commit metadata.
- For every reverse engineering claim, label it **static evidence**, **live observation**, **user report**, or **design proposal**. Symbols and decompiler output show potential code paths; only a measured TV trace proves a path ran in that session.
- When testing external sources, use content the user is authorized to watch. Keep resolver and indexer integrations replaceable and respect their terms and rate limits. No reference-app code or assets belong in Bloob.
