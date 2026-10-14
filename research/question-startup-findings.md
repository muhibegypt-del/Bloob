# Elements of A: startup, measurements and decision limits

Scope: local effective OTA build `16.4.4.J34L139`, static code and prior recorded TV observations. No fresh TV input timing was collected in this pass. Source offsets below are zero-based UTF-8 byte offsets in the named effective OTA file. They are version-specific. Extracted source remains outside Git.

## Performance budgets and measured numbers

The code establishes instrumentation, not a numeric performance contract. `LaunchTimeInstrumentation` tracks begin/end/once events, labels cold/hot/first load/unknown launches and Home/welcome/interrupted completion, and combines JS markers with native `platformTimingEvents`. In `bootstrap.js`, `requireAppJs` begins at byte 618; `app.js` ends it near byte 1,828,995. App stages include UI language rendering, font work, store initialization, current-tab data and crossfade into Home. Those labels make a timing breakdown possible. They contain no measured results for key-to-visible-focus, play-to-first-frame or launch-to-usable-Home on a 2022 OLED, and do not reveal the team's target values.

The actual retained TV evidence is a process sample from 12 October: 74 threads during playback and 368,440–374,148 kB resident memory across five snapshots. The CPU percentage displayed by `ps` was a lifetime average. The pause/resume capture recorded lower CPU tick increments during the reported pause window and higher increments after resume, but has no exact remote-action or first-presented-frame timestamps. It establishes a playback footprint and correlation, not a memory budget, latency target, frame rate, dropped-frame count or causal ranking. See `live-episode-study.md` for source and limits.

## The biggest responsiveness improvement

A single largest improvement cannot be determined from shipped code. No before/after performance measurement or developer statement identifies it. The supported answer is that several mechanisms cooperate: the native frame callback queue coalesces outstanding requests; dirty-tree rendering has an idle path; image/font work has asynchronous paths; widget focus can change without recreating every screen; page loading is split; nearby episode data is bounded; and artwork transitions wait for asset readiness. The native frame mechanisms were previously decompiled at `0x004b566c`, `0x002e8bc8` and `0x002e80b4`. The current UI findings check the OTA version separately. Calling any one of these the biggest cause would invent a result.

## Launch sequence and the former slowest stage

The native host enters base `main.js`; that entry paints a scaled native splash, preserves incoming control messages and interruption state, waits for a native frame callback, and schedules startup with `setImmediate`. The updater fetches or mounts a saved JetPack before importing bootstrap. The updater has three retries and a 3,000 ms request/socket timeout; this is not a guaranteed 9-second overall deadline. The mount attempt can fall back to the bundled app. Updated bootstrap then imports five startup chunks together. App code initializes stores and startup data, handles the startup flow, prepares fonts/current-tab content and crosses into Home. Code marks the relevant stages, but the former slowest stage is not recoverable without historical measurements. A network-dependent update before bootstrap is a potential source of delay, not proof of a measured bottleneck.

Source: effective `main.js` first-frame block near byte 22,134 and updater constructor near byte 6,239; `bootstrap.js` parallel chunk load near byte 645; app startup marks near bytes 649,763–666,751 and 1,308,538–1,319,039. The entrypoint already running before the mount is from the base package; the OTA's own main module describes later launches if it becomes the entrypoint.

## What loads before the first screen and later

There are multiple first-screen milestones: a native splash, the app-owned first frame, and usable Home. The splash is created before the updater finishes. Bootstrap imports its required startup chunks (`9279`, QoS, `6759`, `9021`, and app) concurrently, and its chunk map resolves named page, locale, keyboard and media bundles for later use. Page code and content can be imported or prefetched when needed; the app records a separate launch-completed event once the startup flow reaches Home or welcome. This is code-defined staging, not evidence that every listed route was active on the TV or that chunks were downloaded from the network: webpack's runtime uses local `require()` against the mounted filesystem.

## What they would do differently today

The team's regrets, abandoned prototypes, priorities and retrospective choices are not stored in these artifacts. No REA operation can recover an unrecorded developer opinion. The updater's explicit empty-buffer caution documents a known failure case and defensive fix; it is not a statement about which architecture the team regrets. The answer requires a developer statement or a historical design/experiment document. The code supports independent recommendations for Bloob, but those must be labeled our recommendations.

## Fresh REA evidence

- Startup application analysis parent: `ev_327beda00314eac585f78e9732d4aaf13e4a4176e8c150b65faa87bbd22bfd6d`.
- Summary view: `ev_74e39fd6635c779f7ebab2084b8a2a209095e6edc979fad42f6b38b87bd16c6f`. Three relevant files, two JS files parsed, no parse failures, inventory complete. Semantic resolution has unknowns despite complete inventory.
- Bootstrap outgoing trace: `ev_d0ee7e94b02f7cd197879185626f333b86d95f7b76c5bfc2588ed7977cd7a8c5`: two seed matches, 27 nodes/27 edges, complete within this source; static/inferred relationships only.
- Literal `platformTimingEvents` trace: `ev_7b6306318f5222da4035a358002021cf4661e64ca312637986699b0f98ba85da` returned no graph match. Direct source establishes the call; the no-match is not absence of the feature.
- Effective `main.js` SHA-256: `4fac46cc5db14a36d6c35907f4f561f39d9ce15113295f60c0179ffccd9b3016`.
- Effective `bootstrap.js` SHA-256: `1b839620f19912cfca6c9ae416943ffc88279ad69ebdcfdc6b290e0a7c101b91`.
- Effective `app.js` SHA-256: `af5dd20af5f023b4e6289f8fc87df70cddd964b3b28adc5727d819bcda3ccef4`.
