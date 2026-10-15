# Elements of A: playback and TV lifecycle answers

Date: 2026-10-14. This answers the playback and TV lifecycle questions from the supplied question sheet using the saved effective OTA build `16.4.4.J34L139`, the retained ARM analysis, and earlier explicitly labelled TV observations. No TV action was taken during this pass.

Locations below are zero-based UTF-8 byte offsets in the effective updated files. They identify inspection locations, not copied implementation. The effective tree is the local temporary analysis tree `bloob-jetpack-effective-20261014`. Do not publish that extracted tree.

## Evidence and coverage

- Fresh REA player application parent: `ev_da453b30e11a0ce4399b181bbb343e7e31ba6ea45b884a9fb01665acc7f062fb`; summary `ev_413d63c73b480fd83fb81561c2a91730940d2377c3f23d852635711991a05930`. It parsed `PlayerPage.js`, `PlayerStore.js`, and `hls-adopted.js` as inert source: 3 files, 407,038 visited AST nodes, no parse failures. Its semantic graph is partial: HLS semantic analysis exhausted its owned heap and the retained PlayerStore semantic graph exceeded the parent result budget. This pass does not turn those limitations into negative evidence.
- Fresh REA inspection excerpts parent: `ev_27d5116e86d30f5ca38f066ed497ee96fd9ca3b2745ba694d771f09401896685`; summary `ev_c7376c76cd574c0a3725cffd534decb23415bf58ba70764dbf1dc3008715f04d`. Four small inert excerpts retain source hashes and source byte ranges: control dismissal, minimize recovery, session seek, and the entire updated LG configuration class. Four excerpts parsed without error; application inventory coverage is complete within that excerpt target. Semantic relationships remain partial/unknown because external methods and dependencies are intentionally outside the excerpt. Inspection class wrappers are analysis aids and were not executed.
- Full-bundle semantic thumbnail trace `ev_17db81f525a518ea1e503a380365104431adc9422b3a1bc3cafdb9706911bd45` exceeded MCP transport size. This report does not claim to have read its complete results or use unseen trace paths.
- Updated `PlayerStore.js` SHA-256: `b957612a3e17dfa5645cc95f46c4f9eb57db79130a40f7feebaaf42353944c06`. Updated `PlayerPage.js`: `51a5642497181181cc4bbae9527b9c891421763e4c25e4f528885df8b8934955`. Updated `app.js`: `af5dd20af5f023b4e6289f8fc87df70cddd964b3b28adc5727d819bcda3ccef4`.
- Source inspection supplements REA with focused reads of updated `774.js` (the media shims and HLS adapter), `worker.js`, and the relevant HLS source-selection rules. Native pointers below come from the retained read-only Ghidra exports, which contain unresolved virtual calls and imperfect inferred C++ types.

Static code establishes implemented mechanisms and shipped defaults. It does not supply field performance numbers, the product team's reasons, selected remote overrides, actual output format, or user research.

## Q03: Does playback use LG's pipeline or feed the decoder directly? What is gained or lost?

**Answered for the implemented structure.** Elements of A owns the stream/session/control layers and passes media into LG's media pipeline. The HLS path parses and manages segments through a worker and a media-source/source-buffer shim; the native core owns track pumps and submits compressed samples through its LG adapter. The hardware decoder and output remain LG's. The evidence does not describe a replacement software decoder or a second standalone stock-player application.

The app-owned session starts a selected adapter; `PlayerPage` draws and operates its controls. Updated `PlayerStore.js` contains the progressive adapter at byte `2368` onward and the worker bridge at about `6170`. The progressive adapter accepts URLs without `.m3u8` and creates a native URI source; the HLS bridge accepts `.m3u8` and exchanges seek, rate, track, cue and thumbnail messages with `worker.js`.

The native chain is `Media::MsePlayer` → `TrackPump` → `LgEsPlayerPalImpl`. `LgEsPlayerPalImpl::prepare` at `0x000ccfa0` creates an LG pipeline and submits media metadata. `internalSubmitSample` at `0x000cdb48` and `submitSample` at `0x000ce1f4` manage sample submission; `TrackPump` implements queue/buffer gating in the retained `track-pump-decomp.txt`. This is an elementary-stream integration with LG's pipeline. Exact private LG virtual-slot meanings are not fully resolved.

The control gain is visible in code: the HLS adapter exposes rendition selection, track selection, subtitle cues, source buffering, thumbnails, seeking, prefetch and lifecycle attachment. The actual improvement in start time or seek time over Bloob's current path was not measured in this pass. A decisive limitation is also visible: the progressive adapter's audio/subtitle switching methods are empty and its thumbnail method returns no image (`PlayerStore.js` bytes `5226–5990`). A polished generic player menu does not give every source backend those abilities.

The architecture lesson for Bloob is to keep a single playback session and app-owned controls above a backend with explicit capabilities. The native reference ABI is not established as an API another app can reuse.

## Q04: What happens under TV overlays, Home, backgrounding and the screensaver? How is playback recovered?

**Answered at the code-policy level; exact event mapping for each LG overlay remains a runtime question.** The lifecycle coordinator distinguishes `Unfocused`, `Minimized`, `Maximized` and `Close`, queues those events during startup, runs each store's preparation callbacks, then acknowledges completion. A return from `Unfocused` is treated as a focus event when the app was not backgrounded; a return after minimizing follows the maximize path (`app.js`, bytes `2060290–2064230`).

The updated LG configuration is a complete subclass at `app.js` bytes `65706–68161`, and the factory constructs that LG class. It inherits these shipped generic policies: unfocus → Pause (`app.js` byte `61075`, enum mapping at `54300`); focus → Nothing; minimize → DoNothing (`59318`); app-can-be-killed-while-minimized → false (`59278`). Here, the name DoNothing refers to the session-level policy: the implementation still detaches the media backend, saves state and reports the event. It does not mean the pipeline is left untouched.

`PlayerStore.js` bytes `102247–106023` show the consequences:

1. Unfocus pauses primary and secondary playback sessions. Focus alone does not call resume under the inherited LG policy.
2. Minimize records whether the main session was unpaused, detaches the backend and notifies its reporting layer.
3. Maximize attaches again and resumes only when the recorded flag says it should. Close clears queued work, pauses/stops sessions, removes player listeners and stops each instance.
4. A separate optional policy stops playback, backs up the play entity and position, and restarts only when the app returns within 60 seconds. This policy is present but is **not the updated LG class's inherited default**. Do not attribute it to every LG run.

The HLS backend's detach/recovery is concrete (`774.js` bytes `157044–157699`): detach the native player, set rate to zero, record the real media clock, and evict coded frames in every source buffer. Attach sets the saved clock plus `0.001` seconds and attaches the native player again. This explicitly handles losing the native attachment while preserving the higher-level session. The progressive backend separately records a position, stops the player and replays/seeks its retained source on attach (`PlayerStore.js` bytes `4636–5065`).

The app's own screensaver is suppressed while the primary session is neither Idle nor Error and has nonzero speed (`PlayerStore.js` byte `112413`). Stopping releases that suppression. The app root hides the screensaver and stops its timer when playback requests suppression; otherwise it resets the timeout (`app.js` bytes `1132111–1133000`). Its maximum visible lifetime is `7,200,000` ms, after which it closes the app. The inactivity timeout itself comes from configuration. This is evidence for an app-drawn screensaver policy, not a proof that every TV system screensaver uses the same path.

Quick menu, volume and Home can generate different native events. We did not pair those physical actions with timestamped callbacks here, so the exact event for each overlay and whether the LG OS physically released decoder resources cannot be inferred from the callback names. The implemented recovery path above is nevertheless clear and directly useful for Bloob.

## Q05: What is the frame-rate/HDR strategy? Does LG refresh rate match content? How are Dolby Vision and Atmos confirmed?

**Frame-rate policy answered; actual output verification remains unmeasured.** The session has a native `MatchContentFrameRate` player option (`PlayerStore.js` byte `83203`), enables it only when the platform configuration supports it and the video is not a background preview, and disables it on ordinary stop (`89881`). The updated generic configuration returns **false** from `supportsFrameRateMatching` (`app.js` byte `60141`). The complete updated LG subclass has no override. Therefore this inspected LG app path does not request content frame-rate matching through that option. A capability called `frameRateLimit` is a rendition limit, not a request to switch display refresh rate.

HDR selection combines native media capability information, display color-format flags, source metadata and user/options policy. The HLS platform adapter records advertised ranges as SDR, HDR10, HDR10+, DoVi and HLG; it caps codec dimensions and optional bitrate/frame rate against platform information (`774.js` bytes `147520–149020`). `MediaSource.isTypeSupported` checks native codec names and display masks, including HEVC plus a DoVi flag for relevant Dolby codec forms (`41417–43350`). HLS also filters HDR versus SDR variants by supported dynamic ranges (`hls-adopted.js` bytes `750377–751550`). Those are compatibility and rendition-selection mechanisms.

The player UI's reported HDR/audio badge is derived from stream codec/range and selected-track information (`PlayerStore.js` bytes `17975–18268`). Debug supported-format lists report **capabilities**. Neither alone proves that the panel entered a given HDR mode or that a receiver emitted Atmos. Earlier user-visible HDR/Atmos reports are observations of particular sessions. Definitive output confirmation would require actual display/audio-path status for the playing session; it is not in a static source file.

## Q06: Which formats cause problems on LG: DTS, TrueHD/eARC, Dolby profiles, PGS?

**Partially answerable; historical surprises and a complete hardware compatibility matrix are unavailable.** This reference adapter is built around its own controlled media formats. Its HLS audio codec enum recognizes MP3, AAC, xHE-AAC, AC3, EC3, FLAC and ALAC (`hls-adopted.js` byte `161704`); the library default disables ALAC, FLAC and xHE-AAC (`171875`). The native-backed media-source support check explicitly recognizes AAC, AC3 and EC3, with JOC/Atmos allowance checks (`774.js` bytes `41417–43350`). These show what the inspected adapter accepts, not every format LG can play through another application path.

DTS and TrueHD are not established as accepted formats in that support check. A generic content-data enum includes a TrueHD display label (`6759.js` byte `1020403`); that is not a decoder implementation or evidence of eARC bitstream passthrough. Do not rank a source as compatible just because a content badge names it.

The Dolby checks identify codec families and combine HEVC capability with the display's DoVi mask; they do not establish an exact accepted Dolby Vision profile/level matrix for the user's TV. The image-subtitle PGS format is not established by the inspected subtitle path. Text/caption handling is concrete: WebVTT is enabled, CEA708 captions are enabled, and the parser includes TTML track handling (`hls-adopted.js` bytes `78698–79200` and `169597–170030`). These are available parser paths, not proof of a particular file playing correctly.

The question of what surprised the development team, and whether a particular soundbar/TV/eARC combination passes TrueHD, cannot be reconstructed from these artifacts. For Bloob, use a capability gate for the actual backend and output path; keep unverified codecs explicitly unverified.

## Q11: What does the player do during scrubbing? Hide rules, thumbnails and subtitle placement?

**Answered for shipped interaction mechanisms and constants.** The control surface has Full, Reduced and Deck modes. The playback clock, dragging preview and thumbnail delta are separate values. Starting scrub/trickplay pauses or adjusts playback through callbacks; control layout can reduce while keeping transport visible. The other controls animate out over `250` ms when that reduction is active (`PlayerPage.js` bytes `328370–329020`). Short seek feedback icons last `450` ms; thumbnail visibility has a separate `3,000` ms hide delay (`228650`, `253019`, `264076`).

The precise control-dismiss defaults are at `PlayerPage.js` bytes `331740–332400`:

| Trigger | Delay before hide |
| --- | --- |
| Automatic / ordinary player activity | 5 seconds |
| Pause, focused tab item, focused control, visible tip, open control menu | 8 seconds |
| Opened tab/deck | 20 seconds |
| Screen reader enabled or platform requires virtual views always on | Double the applicable delay |

The dismissal timer is not started while dragging, full-screen trickplay or thumbnail scrubbing is active. The callback checks those flags again before hiding. Show/hide fades take `300` ms unless forced instantaneous by policy or the call. The inspected code does not justify a blanket statement that controls remain indefinitely visible whenever paused: Pause selects the 8-second timeout in this updated implementation.

Thumbnail work is bounded and measured. The controller begins with at most 8 thumbnails per second; platform configuration can supply 4/6/8 according to performance class (`app.js` byte `58100`). It keeps eight future trickplay slots, cancels work on mode changes, reuses suitable loaded previews when speeds change, and caps a ready queue at 16. It falls back to one thumbnail per second under request pressure; the static concurrency ceiling is 24 (`PlayerPage.js` bytes `228200–235000`, `239436–241950`, `250677`). A one-second health interval gradually increases the rate when fewer than two requests remain outstanding (`233149` region). The HLS adapter obtains an image I-frame around the requested time with cancellation, a `90,000` ms outer timeout and `120,000` ms request timeout, and returns the image fragment's actual start position (`774.js` byte `139245`). The progressive adapter supplies no preview image.

Subtitles are laid out above visible controls and floating menus. During thumbnail scrub, bottom padding includes the transport container minus the title bar plus the maximum thumbnail height. Otherwise the algorithm takes the greater of visible control height and the floating-menu obstruction plus a scaled 24-pixel gap, clamped below the vertical safe area (`PlayerPage.js` byte `327770` region; exact method `calculateSubtitlesBottomPadding` begins near `327710`). Menu size/children changes trigger recomputation (`321481`, `321670`); the page relays the result to the subtitle drawing layer. This is a geometry-driven placement rule, useful for avoiding enormous or obscured captions in Bloob.

The controller records first-preview delay, success/cancel/error counts and timing buckets. No artifact records what user testing changed or what sofa testing concluded.

## Full playback question: Start high on fast Wi-Fi or ramp up?

**Answered for the selector and library defaults; the chosen rendition in a live session is unknown.** The HLS engine has a specific initial-variant selector. It uses capability-filtered options, retained bandwidth/latency history, estimated playlist/parse/append/play costs, source quality scores when present, codec/range/audio traits, resolution and bitrate. It is not an unconditional highest-bitrate rule or unconditional lowest-bitrate rule (`hls-adopted.js` bytes `295324–295760`, `352665–354055`).

Its shipped defaults enable adaptive startup, aim at `4,000` ms startup, and define initial preferred/valid ranges: 0.8–8 Mbps, preferred bitrate below 5 Mbps, and height 300–1200 pixels (`170930–171335`). Bandwidth history window: `120,000` ms; history TTL: `600,000` ms; aggregation: quadratic time weighting; minimum stable sample: 50,000 bits. These are **library defaults**, not measured performance budgets or guaranteed restrictions on every session. The app/remote configuration can override them.

After startup, the default adaptive rules use 0.95 of estimated bandwidth for down-selection and 0.90 for up-selection; with only a small number of samples they use a conservative factor of 0.70. The low-sample threshold is 3; samples are considered on a `200` ms period; the default starvation allowance is 4 seconds (`hls-adopted.js` bytes `170052–170275`, `673779`). Buffer depth, fragment cost, codec/rendition compatibility and available variants also participate. A fast connection can therefore start higher when the estimate and predicted startup cost support it, then adapt; the code does not promise to open immediately at 4K.

## Full playback question: How quickly should seeking land, and what if it lands elsewhere?

**Mechanism answered; expected elapsed seek time is unavailable.** The session accumulates relative seek deltas, clamps them against the live seekable window or finite duration, and commits one backend seek after `400` ms unless instructed to commit immediately (`PlayerStore.js` bytes `93941–94808`; constant `eN=400` at `100384`). At the finite end it clamps away from the last second and uses a duration-minus-10-seconds target when required (`95733` region). Requested position is kept separate while the backend seeks; the main/worker bridge tracks the pending position and ignores stale worker updates during the transition.

The HLS adapter updates the real media time and publishes Seeking state (`774.js` byte `157992`). The native engine seeks source buffers, updates source/track state and drives LG seek (`Media::MsePlayer::seek` `0x000e9370`, `seekSourceBuffers` `0x000e1508`, LG seek `0x000cba08`). Indirect interface types remain imperfect in the decompilation.

HLS defaults include a 0.25-second time-accuracy threshold, maximum buffer hole of 0.5 seconds, maximum seek hole of 2 seconds, a 5-second seek watchdog, 200 ms stall checks, and 0.1-second nudges with at most 3 retries (`hls-adopted.js` bytes `163470–164100`, `170820`). Those are tolerances/watchdog policy, not a measured promise that every seek completes within 0.25 or 5 seconds. The UI should settle on acknowledged media position. Preview image position can also differ because it identifies an available image fragment, not an arbitrary decoded frame at precisely the pointer time.

## Full playback question: Audio commentary/descriptive audio, Atmos/stereo and forced subtitles?

**Answered for metadata and presentation; exact content availability varies.** The track menu uses actual track collections and persistent identifiers, records selection, and updates from session metadata. It localizes rendition names supplied by the content service, then falls back to language/track name. It marks AD, CC and SDH with text/icon treatments, supports an original-content label, and distinguishes team-radio variants. Ordering considers default tracks, UI language, recent preferred language, type and stable track metadata (`PlayerPage.js` bytes `111480–113420`; `app.js` bytes `879971–884990`).

The visible audio menu is not proved to append a codec/channel/bitrate string to every track row. Commentary can be identified through supplied localized rendition names; source metadata without those names can fall back to a basic language label. Stream audio-format badges are computed separately from codec and channels. Bloob should enrich track rows from its own trustworthy metadata rather than assume the reference always displays a complete codec description.

Audio preferences include language, audio-description policy, broadcast selection and retained episode audio type. Subtitle preferences include on/off/automatic, CC/SDH preference, system caption preference and preferred language. In the HLS adapter, selecting an audio track can select a forced subtitle persistent ID for the chosen audio language when no active subtitle track exists (`774.js` byte `160138` region). The progressive adapter's switching methods remain empty. The library/adapter supports the menu mechanism, but a backend that does not expose tracks cannot implement it simply by drawing the same menu.

## Full playback question: Preload the next episode before the current one ends?

**Preload facility answered; universal ordinary-episode preloading is not established.** `PlayerStore.queueNext` makes a next play item and asks the chosen backend to queue it (`114433–114740`). The progressive adapter loads a next URI source and reuses it on a matching URL (`3567–4340`). The HLS wrapper enables item preloading through a version comparator called with components `(2, 610)` (`774.js` byte `152781`), overriding the library's false default. This notation is not silently interpreted as semantic version `2.6.10`. When permitted, its next-item method calls HLS `prefetchSource` (`774.js` byte `140523`), and the HLS engine has manifest/playlist/init-segment caches and next-item prefetch operations.

Actual app callsites found in the effective tree include queuing the full episode while a video-summary/recap plays (`app.js` byte `825274`), the next background-preview slide (`1784786`), and a scheduled next sports item (`SportsPostPlayPage.js` byte `18878`). Those are stronger evidence than the mere existence of a queue method. The bounded inspection did not establish that every ordinary binge next episode is prefetched before the current episode ends, or a universal seconds-before-end trigger. It would be inaccurate to claim that universal behavior from this API alone.

Session teardown preserves a queued-item transition briefly, with a `5,000` ms cleanup timer if another session does not take over (`PlayerStore.js` byte `112790` region). The post-play policy separately supports episode autoplay and preserves track/subtitle preferences; a binge inactivity guard compares at least two episodes with two hours since interaction (`PlayerPage.js` bytes `31599–33490`). Prefetch policy and autoplay policy are distinct.

## Full playback question: What if a stream link expires mid-watch?

**Generic error treatment found; automatic source-link renewal is not proven.** The unprotected URL-loader layer classifies HTTP 401, 403 and 407 (plus certain transport conditions) as authentication/CORS errors (`hls-adopted.js` byte `177418`). That classification does not demonstrate a callback that re-resolves an expired AllDebrid URL. No such service integration exists in this reference study. Generic HLS retry, content steering, playlist reload and source item prefetch do not themselves renew an application's expiring source link.

This pass did not inspect account or protected-media renewal paths. The operational study finds general failure troubleshooting and retry UI, but no established generic source-URL re-resolution policy. The defensible answer is that the artifacts do not tell us an automatic mid-watch link-renewal mechanism for Bloob's source service. Bloob needs its own resolver policy: retain media identity and resume clock, re-resolve when the service reports expiry, and restart/rebind from the acknowledged position using a backend-supported path. That is an original design requirement, not a copied reference behavior.

## Questions whose requested answer is unavailable from static code

The artifacts do not establish measured key-to-frame/startup/seek targets, actual dropped frames caused by the UI, which decoder modes ran in a given session, developers' reasons for their backend choices, rejected experiments, product-test changes or regret. They do establish the mechanisms above. Those two evidence categories should remain separate when building Bloob's implementation and performance budget.
