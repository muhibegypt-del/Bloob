# Elements of A: all 37 engineering questions

Study date: 2026-10-14. This is the answer index for the supplied questionnaire, in its original order. Each question has been investigated and addressed. Some answers identify an implemented mechanism; others establish exactly which requested measurement or human decision is not recoverable from the supplied evidence. An unknown is not replaced with a guessed target or developer opinion.

## Scope and evidence

The current static study uses the decoded local JetPack snapshot, build `16.4.4.J34L139`, plus focused nonprotected native decompilation. The saved update's activation on the TV was not observed. Historical TV measurements below are from explicitly documented 12 October sessions and are not attributed to the newer update. This pass took no TV action.

REA was used for fresh focused startup, UI, player, quality and performance analyses, with focused graph traces and module views. The selected JS inputs parsed successfully; their inventories are complete within those selected inputs, while semantic resolution remains unknown/partial where dynamic dependencies, obfuscation or resource limits prevent a complete graph. Native analysis used the existing Ghidra project read only. Exact code offsets, artifact hashes, Evidence IDs and search boundaries are in these four companion notes:

- [Startup, measurements and decision limits](question-startup-findings.md)
- [UI, rendering, focus, design and accessibility](question-ui-findings.md)
- [Playback, tracks, scrubbing and TV lifecycle](question-playback-findings.md)
- [Quality, reliability, reporting, releases and platform](question-operations-findings.md)

The original archive and code were studied locally; this document contains paraphrased findings. It does not include extracted source, account material, protected-media internals or credentials. The [JetPack map](jetpack-analysis-2026-10-14.md) describes how the update was decoded and related to the base.

## The 12 priority questions

### 01. Performance budgets on LG and how they measure them

**Measurement machinery is established; budgets and actual latency results are unavailable.** The app records launch stages, page/network time, frame-duration distributions, CPU, memory, worker round-trip delay, media resolution and subtitle performance. Native counters expose heap/RSS, graphics times and surface/image-cache usage, and media-buffer/player usage. Memory is sampled every second and reporting groups run every five minutes; those intervals are not acceptable UI latencies. There is no supplied numeric key-to-focus, first-frame or usable-Home target, or a 2022 OLED percentile result.

The retained 12 October episode run used 74 threads and 368,440–374,148 kB RSS. Those are observations for one session, not budgets. No exact remote-key or first-presented-frame timestamp was recorded. Details: startup and operational notes.

### 02. The one change that made the largest responsiveness improvement

**A causal ranking cannot be recovered.** The code has coalesced frame requests, a dirty-render idle path, native asynchronous asset work, bounded list traversal, lazy widget realization, nearby data windows and asset-ready transitions. These are supported mechanisms worth implementing independently. No before/after profile or developer statement tells us which change produced the largest gain. Details: startup and UI notes.

### 03. LG standard media pipeline or direct decoder feeding, and the tradeoffs

**The app owns stream/session control and feeds LG's native media pipeline.** HLS uses a worker, media-source/source buffers and native track pumps that submit compressed samples through an LG elementary-stream adapter. Progressive URLs use a native URI-source adapter. The controls are app-owned; hardware decode/output remain LG's. The structure supplies buffer, seek, rendition, track and lifecycle control, but does not establish a measured speed advantage over another backend.

There is an important capability difference: the progressive adapter's audio/subtitle switching methods are empty, and it returns no thumbnail image. A shared polished control surface does not make every adapter equally capable. Details: playback note Q03 and the native [system map](Elements-of-A-system-map.md).

### 04. TV overlays, backgrounding, pipeline release and recovery

**There are separate unfocus, minimize, maximize and close policies.** The inspected LG config inherits Pause on unfocus and Nothing on focus. Minimize records whether playback was active, detaches the backend and reports the state; maximize attaches again and conditionally resumes. HLS detach saves the media clock, pauses the native player and evicts coded source-buffer frames; reattach restores the clock plus 0.001 seconds. Progressive detach/attach has its own stop/replay/seek path.

The app suppresses its own screensaver during active playback. Which event the physical quick menu, volume, Home or system screensaver produces was not timestamped in this pass, so those physical actions are not all assumed to map to the same policy. Details: playback note Q04.

### 05. Content frame rate, HDR and physical Dolby Vision/Atmos confirmation

**This LG build does not request content frame-rate matching.** The player has a match-frame-rate option, but the platform method returns false and the current LG subclass does not override it. Renditions are filtered using advertised native codec/display capabilities, dynamic range and configured limits.

Codec/range badges and support probes establish metadata and capabilities. They do not establish the panel's live HDR mode or receiver output. The user's earlier HDR/Atmos observations describe particular sessions; static code supplies no physical output confirmation. Details: playback note Q05.

### 06. Format surprises: DTS, TrueHD/eARC, Dolby profiles and PGS

**The inspected backend's support checks are recoverable; the team's surprises and a complete hardware matrix are not.** Its native-backed HLS checks explicitly handle AAC, AC3 and EC3 with relevant Atmos allowance checks. Text-caption paths include WebVTT, CEA708 and TTML. A TrueHD content badge is not evidence of decoding or eARC passthrough. DTS, TrueHD, PGS and exact Dolby Vision profile/level support are not established by these inspected checks.

Bloob must rank against its actual backend and output path. Model/receiver support and history of field surprises require different evidence. Details: playback note Q06.

### 07. Real-TV testing, automation and webOS coverage

**QA hooks exist; a lab and test matrix are not present.** Reporting carries QA session IDs and test categories, and base trace configuration contains D-pad/key and internal `Automator` filters. This establishes test instrumentation, not automated remote control, screen capture, physical-device coverage or release results. LG's simulator has documented media/platform differences from actual TVs. Details and official source: operational note section 6.

### 08. Streaming quality metrics and action thresholds

**The app measures play/buffer/seek/error intervals and includes a network diagnostic flow.** Native reporting counts stalls and their accumulated duration. Where available, adapters expose dropped-frame counters; a zero-valued fallback does not prove zero dropped frames.

The diagnostic toolkit uses a 5-second reachability timeout, a 120-second startup phase and a 30-second streaming observation. It labels observed bandwidth good when the minimum is at least 25 Mbps or maximum at least 50 Mbps. These are troubleshooting heuristics, not the normal playback SLA, ABR settings or production alert percentages. Those operational targets are unavailable. Details: operational note section 2.

### 09. Scaling effects by model or runtime measurements

**A static presentation tier and accessibility policy are established.** LOW/MEDIUM/HIGH/ULTRA gate different animation, blur, card and placeholder features. The inspected LG class inherits ULTRA. There is an override facility, but this is not evidence of automatic CPU/GPU benchmarking or automatic downscaling when the TV is slow. Reduce Motion/Transparency additionally affect animations and blur. Details: UI note short-list question 9.

### 10. A custom-drawn UI and LG Audio Guidance

**Widgets carry a semantic accessibility model and an app-owned reader speaks it.** Labels, roles, position/count, enabled state and focus accompany the graphics tree. The reader responds to focus/content/page changes, cancels or queues speech and calls native TTS. Device accessibility state controls whether it is enabled.

An optional semantic AX tree bridge exists, but the inspected LG configuration does not enable it. The supported LG inference is direct TTS; no live guidance event was captured. Details: UI note short-list question 10.

### 11. Scrubbing, hiding controls, thumbnails, subtitles and user-test changes

**Implemented rules are explicit.** Controls use Full/Reduced/Deck modes and separate actual media time from drag preview and thumbnail time. Default hiding delays are 5 seconds ordinarily, 8 seconds for pause/focused controls/tips/control menus, and 20 seconds for an open tab; reader use doubles the applicable delay. Dragging, full-screen trickplay and thumbnail scrubbing suppress the timer. Pause is not an indefinite-visible rule in this build.

Thumbnails have cancellation, bounded queues and a pressure-sensitive request rate. Subtitle bottom padding is computed from visible controls, menus and thumbnail geometry. HLS has preview support; the progressive adapter does not. No artifact records what user testing changed. Details: playback note Q11.

### 12. What the team would do differently if starting today

**This requires a developer statement.** Present code, compatibility branches and a known empty-update guard cannot reveal regrets, abandoned prototypes or retrospective priorities. Bloob recommendations derived from the study are our decisions, not testimony from the reference team. Details: startup note and UI note question 12.

## Performance and rendering

### 13. Keeping long rows smooth

**List traversal and realization are bounded.** Uniform/nonuniform size trees support index/position lookup; drawing begins near the viewport and advances through the needed extent plus focus requirements. Child descriptions become full widgets as processed. This does not prove universal fixed-pool card recycling or immediate disposal of every offscreen object.

Episode details load after 500 ms near selection, roughly ten entries either side, with distant downloaded entries replaced by placeholders beyond wider margins. Related browse shelves have a default insertion cap of 35. Images are prefetched and revealed after readiness; completed transitions stop their frame work. Details: UI performance section.

### 14. Artwork display sizes and compression

**Requests are size-aware.** Width/height, aspect ratio, crop and screen policy determine the resource URL. Default JPG quality parameters are 90 for a 720-high UI and 80 otherwise; they are quality parameters, not percentages of saved bytes. Default-accuracy requests on UHD 1440p/4K use three-quarter dimensions and 8K uses one-third dimensions, while maintaining layout scale. Logos and other callers can request precise sizes or other formats. No universal poster/hero dimension is implied. Details: UI artwork section.

### 15. Memory budgets and release while idle or playing

**Resource controls are visible; a global MB budget is not.** The UI limits nearby metadata and shelf growth, disposes replaced search content, and cancels completed animations. The native runtime reports image/surface/cache/font/media-buffer usage, allowing detailed profiling. Background HLS detach evicts coded frames.

One earlier playback-exit comparison fell from 358,464 to 272,612 kB RSS and 74 to 48 threads, with selected media device descriptors disappearing. That establishes release in one run, not a universal retained-memory ceiling or a leak-free guarantee. Details: UI memory section, operational counters and [historical system map](Elements-of-A-system-map.md).

### 16. UI work during playback and dropped frames

**The UI remains active for player state, controls and subtitles.** Completed artwork animations stop, page/modal transitions can idle the page below, and product previews avoid starting over a player page. Native graphics have a dirty-tree idle path. These reduce work but do not prove zero background activity or zero decode/UI contention. No measured dropped-frame outcome is available. Details: UI playback-work section and native runtime notes.

## Startup

### 17. Launch sequence and its former slowest part

**The sequence is known; historical bottleneck ranking is not.** Native host → base splash/control queue → first-frame scheduled updater → mount saved/downloaded update or fallback → bootstrap/startup chunks → stores/fonts/current-tab data → Home transition. Named timing markers expose these stages. A network-dependent update can delay startup, but the artifacts contain no old profile proving it was the slowest stage. Details: startup note.

### 18. First-screen work versus later work

**Splash, app first frame and usable Home are separate milestones.** Bootstrap starts a defined parallel chunk group; named routes, locale/keyboard and media modules load on demand. Product-page code is warmed at `startupFlowDone`; some page-entry paths prepare artwork before their reveal. Local webpack `require()` loading is not evidence that every chunk is fetched over the network. Details: startup and UI startup sections.

## Navigation and focus

### 19. Focus when refreshed content replaces old cards

**Identity, selection and scroll survive replacement.** Compatible container state is transferred; the previous widget ID is preferred, and the previous index is clamped when that ID is absent. Related shelves save selected index and scroll separately from widget objects. This makes a data refresh preserve the user's location where compatible. Details: UI navigation section.

### 20. Loading the next screen before selection

**There are distinct triggers.** Product-page code is prefetched after startup; artwork is prepared during some page entries; episode data waits 500 ms after selection; related browse content waits 300 ms after focus and rejects stale replies with a version token. This is not evidence that every focused poster fetches its entire detail page. Details: UI prefetch section.

### 21. Back behavior and exit confirmation

**Back closes the nearest owned interaction layer before app exit.** The root can dismiss an overlay/profile switcher, pop a page, return a root screen to its top or expand the menu. An Exit-key dialog exists and defaults to Cancel, while a generic exit policy can close directly. Thus prompts depend on the path; neither every Back press nor every exit necessarily prompts. The design team's reasoning is unrecorded. Details: UI Back section.

## Design for TV

### 22. Smallest text and sofa-distance testing

**The named standard table reaches Caption3 size 21 with leading 28; text fitting defaults to minimum 23.** Those are scaled design units, and individual callers can vary them. The table is not a universal physical-pixel minimum. The build has larger accessibility text settings, but no sofa-test results or rationale for those sizes are supplied. Details: UI typography section.

### 23. Contrast over artwork and video

**Rendering protects text with gradients, shadows, themed colors and dark/solid overlays.** One product-card path chooses a stronger dark blend for luminous artwork (0.55 versus 0.2), and the title-page fold coordinates hero opacity and background protection. Reduce Transparency supplies simpler surfaces. These are contrast-preservation mechanisms, not measured WCAG ratios or evidence of a particular audit process. Details: UI contrast section.

### 24. Rotating hero, test results and accessibility

**Automatic hero advance is gated.** The slideshow checks foreground/controller/performance permission and Reduce Motion. Home additionally requires topmost/scroll-at-top state and rejects advance while the screen reader is enabled. This confirms accessibility pausing in code. User preference or test success is unavailable. Details: UI hero section.

### 25. What user testing changed most in layouts

**No dated study outcomes are present.** Layouts, flags and variants do not identify participants, discarded designs or their causal influence. The study can recover focus rules, density transitions and loading/error states, but cannot honestly attribute them to particular user tests. Details: UI user-testing section.

## Playback

### 26. Start high or ramp up on a fast connection

**The initial HLS selector evaluates capabilities, bandwidth history and predicted startup cost.** It is neither an unconditional highest nor lowest choice. Library defaults enable adaptive startup with a 4,000 ms aim, initial ranges of 0.8–8 Mbps and height 300–1200, and a preferred bitrate below 5 Mbps. Later adaptation uses buffer/cost/compatibility and bandwidth estimates; default factors include 0.95 for down-selection, 0.90 for up-selection and 0.70 for sparse samples.

These are overridable library defaults, not a measured app SLA or proof of the variant selected during a TV run. Details: playback startup-variant section.

### 27. Seek landing time and position mismatch

**Seeking keeps requested time separate from acknowledged media time.** Relative steps can accumulate for 400 ms before one seek commits, then targets are clamped to the live window/duration. The bridge rejects stale position updates during the transition. HLS has hole/accuracy tolerances, a 5-second watchdog and bounded nudging; those are recovery parameters, not elapsed seek guarantees. Preview images report their available fragment time. The control should settle on the real media clock after acknowledgment. Details: playback seek section.

### 28. Commentary, descriptive audio, Atmos/stereo and forced subtitles

**Menus consume actual track collections and persistent IDs.** They localize supplied rendition names, fall back to language/name, label AD/CC/SDH and retain language/type preferences. Forced-subtitle selection can follow the selected audio language when no subtitle is active. Codec/channel badges are separate; the code does not prove every audio-menu row shows complete format details. Backend exposure and source metadata determine what is possible. Details: playback track section.

### 29. Preloading the next episode

**Both adapters have queue/preload facilities.** Progressive playback can load a next URI source; HLS can prefetch manifest/playlist/init data when its version/config gate permits. Verified app callsites include loading a full episode during a recap, background previews and scheduled sports items. The bounded study did not establish preloading every ordinary binge episode or a universal seconds-before-end trigger. Autoplay and prefetch are distinct policies. Details: playback next-item section.

### 30. A link expiring mid-watch

**Generic transport errors and retries are present; a generic expiring-source re-resolver is not established.** Recognizing an authorization/HTTP failure or reloading a playlist does not demonstrate refreshing an arbitrary resolved URL. The study excludes protected/account paths, and the reference has no AllDebrid integration. Bloob's renewal/resume policy must be its own explicit resolver/session contract. Details: playback expiry section.

## Reliability

### 31. Most common field failures and recovery

**Error handling is visible; prevalence is unavailable.** The HTTP layer classifies DNS/connect/read timeouts, other transport errors, failed HTTP statuses and slow completions. A configured issues service can supply localized recovery text, retry and troubleshooting flows; diagnostics can be canceled without stale replies taking over. No fleet counts establish which failure is most common or how often recovery succeeds. Details: operational section 3.

### 32. Slow or dropping Wi-Fi

**Network state, request failure, adaptive streaming and recovery UI are handled separately.** HLS adapts to measured bandwidth/buffer conditions. Diagnostics distinguish basic reachability, stream startup and sustain/stall behavior, using explanatory localized UI. The artifact does not establish a policy based on Wi-Fi signal strength or access-point type, or a universal source-URL renewal mechanism. Details: operational section 4 and playback ABR section.

### 33. Crash and error collection

**The native runtime exposes a crash-report property; app reporting checks consent and enriches it with environment information.** JS errors can carry stacks and sampled context; reporting has event limits and flushing policies. This establishes a client reporting route. It does not establish server symbolication, fleet frequencies, retention policy or the contents of a current crash. No crash was induced or report transmitted in this study. Details: operational section 5.

## Updates and releases

### 34. UI updates without a store release: rollout, kill switch and fallback

**The client downloads/mounts an update before bootstrap, caches it with an ETag and falls back on download/mount failure.** Specific failed paths have three retries and a 3-second request timeout. Zero-length downloads are rejected. These protections do not demonstrate recovery from every exception after the updated app starts.

The base loader checks a local `disableOTA` flag; the updated loader retains its comment but omits the executable check. A remote kill switch, percentage rollout, canaries and server rollback criteria cannot be recovered from the client URL and loader. Details: operational section 7 and corrected JetPack map.

### 35. Protecting older webOS versions

**A platform abstraction supplies device/build matching, capability limits, feature gates and compatibility shims.** Multi-view has explicit OS/model/player-count conditions and a restart requirement; one model family loses pointer input; older network/graphics signatures are adapted. These are concrete defensive policies. The live configuration and complete supported-model/firmware matrix are not supplied. Details and official compatibility links: operational section 8.

## Platform

### 36. Partner-only interfaces and sufficient public alternatives

**The reference uses its own native ABI plus an LG media adapter.** Presence of that adapter does not establish the commercial permission rules for another app. Official LG docs establish public HTML media playback, mediaOption/initial-position support, connection status and lifecycle APIs. The exact elementary-stream ABI's public availability was not established by the bounded official search. An official SDK/API agreement is needed to classify entitlement. Details with primary-source links: operational section 9.

### 37. LG quirks worth knowing

**The code has specific exceptions, not a universal folklore list.** Examples include pointer suppression on a named model, OS/model/second-player checks and restart after multi-view, throttled video-window size animation, property/graphics-signature shims, missing-stat fallbacks and distinct focus/background policies. Codec/frame/bitrate limits are configuration inputs. Those mechanisms justify centralized capability policy in Buna; they do not substitute for observing the particular TV/backend. Details: operational section 10 and playback lifecycle section.

## Additional clarification: the Home UI language

The shipped Home UI is JavaScript in `app.js`, which constructs widgets and calls Luna graphics APIs. Cards, shelves, screen composition, focus, navigation, season/episode presentation and player controls are implemented across these JavaScript bundles. The underlying rendering/input/frame runtime is native C++. That is the deployed language boundary established by the artifacts; it does not claim access to the team's complete original authoring source.

| UI responsibility | Shipped JavaScript location |
| --- | --- |
| Shared cards/lockups, shelves, widgets, typography, platform presentation policy and root navigation | `app.js` |
| Title details, season/episode rows, episode paging and title-page composition | `ProductPage.js` |
| Browse shelves and animated filter menus | `SuperBrowsePage.js` |
| Player overlay, transport bar, thumbnails, track menus and post-play | `PlayerPage.js` |
| Search keyboard/results workspace | `SearchPageFactory.js` |
| Library and show-library screens | `LibraryPage.js` |

The movie catalog, title text, episode records and artwork are data supplied to those components. They are not all hardcoded in the UI bundle. JavaScript owns their presentation; native graphics draws the resulting widget tree.

## Additional clarification: design principles in the files

The artifacts contain an implemented design system: named typography, style/color/gradient choices, spacing and safe areas, card/shelf layouts, focus treatments, motion policy, accessibility semantics and coordinated page/player states. The detailed UI note identifies concrete values and source offsets, including standard typography, image sizing, focus restoration, contrast protection and Reduce Motion/Transparency behavior.

A bounded check found no filename indicating a design-principles or human-interface guide in the extracted update, and no literal `Human Interface Guidelines`, `design principles`, `design system` or `DesignSystem` text in the seven inspected UI bundles (`app`, product, player, browse, search, library and settings). This establishes no explicit written guide at those locations; it does not prove that such a guide never existed in the original development repository. The visual and interaction rules themselves are encoded in shipped components and configuration and can be studied as implemented behavior.

## What the answers now make concrete for Bloob

The study supports an implementation specification for focus preservation, bounded list work, artwork preparation, accessibility semantics, player HUD modes/timers, a single playback clock/session, capability-aware adapters, attach/detach recovery, track metadata, ABR/seek policy, metrics and controlled fallback. Its strongest constraints are equally concrete: the reference's progressive adapter is limited, this LG build disables frame-rate matching, and capability labels do not verify physical output.

Release targets, hardware output confirmation, production failure frequency, the test lab, developer regrets and user-study outcomes remain separate evidence requirements. The missing values should be recorded as missing and Bloob's own targets chosen explicitly. No answer here is a claim that source organization alone guarantees performance parity.
