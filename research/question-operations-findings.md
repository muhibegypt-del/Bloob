# Elements of A: operational questions answered from shipped artifacts

Study date: 2026-10-14. This document answers the operational, quality, reliability, release, compatibility, and platform questions in the supplied questionnaire. It paraphrases findings rather than reproducing implementation code. All work was offline and read only with respect to the TV.

## Evidence and boundaries

The effective OTA snapshot is build `16.4.4.J34L139`. Source locations below are zero-based UTF-8 byte offsets into its files, unless explicitly marked as the base `res` version. They identify code, not an observed execution. The update snapshot may contain an entrypoint which differs from the entrypoint the installed base actually runs before mounting the update.

Fresh focused REA analysis of `qos.js` and `QualityOfService.js`: parent `ev_25c1ab23b09dba19791d46e3c6c18660fe6edf2c1b5e1e1a2b8ac5040b6d6385`; summary `ev_7c79da5d7704d4157c139ff2e32eb574f39e13378c679668ff7a4a15adb39a21`. Both files parsed successfully; no execution occurred. Application inventory was complete within this two-file input, while dynamic/cross-function semantics remained unknown. A property trace returned a partial result without matches; it is not negative evidence that the methods do not exist.

Fresh focused REA analysis of anchored performance/platform source slices: parent `ev_31de9c7a609fc31958947bf9a452ad667941a44a60211cac4e0f46eedf13df96`; summary `ev_ddbfeef2b901e0df3d6efc78c5548f1519c9d15fa85d2905ff2ec9cc9fb54630`; performance module item `ev_ceaa18d0386f6cdc8e76d9c1b2e19f46eda97a337c8fa6ec183d69712e2d73b5`. The temporary slices came from original `app.js` byte ranges `[920361,936309)` and `[65706,68161)`, with a final statement terminator added for standalone parsing. Both parsed successfully, but their dependency context and dynamic runtime values are deliberately outside that input. Local source inspection corroborated method bodies and constants.

The ARM executable was additionally inspected using the existing Ghidra 12.1.4 analysis project, read only and without rerunning analysis. Four selected nonprotected functions were exported locally. The executable SHA-256 is `182704ecaa5277ca837544272ac81cd704a1d0b8eb215afea04d143396d7affa`. Addresses below are Ghidra project virtual addresses. This project maps the ELF at an added `0x10000`; recovered string file offsets below were translated through ELF load mapping rather than assuming addresses are file offsets.

Snapshot file identities:

| File | SHA-256 |
| --- | --- |
| `app.js` | `af5dd20af5f023b4e6289f8fc87df70cddd964b3b28adc5727d819bcda3ccef4` |
| `main.js` | `4fac46cc5db14a36d6c35907f4f561f39d9ce15113295f60c0179ffccd9b3016` |
| `qos.js` | `d3278ddd89057982d1b7e2bd59c44dad79d77df6fbdcee72818de77909bc2455` |
| `QualityOfService.js` | `6a63b8155fd3dd4915a365fdaff23457091d1ea3d8e200870ccbcb7b47b71b3b` |

## 1. What are the performance budgets, and how are they measured?

**Answer: the measurement machinery is visible; product budgets and real performance numbers are not in these artifacts.** There is a substantial performance reporter, not merely occasional console logging. It captures CPU usage percentages, UI frame count and a histogram of frame durations, memory averages, player resolution/media usage, subtitle renderer performance, network request timing, player import timing, launch stage markers, page load time, and worker responsiveness.

The frame timing handler subscribes to runtime frame events. Frame durations are rounded with a carried remainder, so repeated fractional frame times are not all rounded the same way. Page context records a start time and accumulates network time; completion reports total page load time separately from network time. Launch reporting gathers timing marks, distinguishes the first launch after a build change, reports interruptions, and computes `totalTimeMs` from the final timing endpoints. The worker probe calculates main-to-worker and worker-to-main delay from heartbeat timestamps.

The reporter samples current memory every **1,000 ms** and emits a periodic reporting group every **300,000 ms**. Those are sampling/reporting intervals, not an acceptable UI delay. It stops periodic reporting and flushes accumulated events when minimized or closed, and starts again on maximize. Remote metrics configuration supplies the sampling percentage and maximum event count. Missing performance configuration defaults the event lottery/limit to zero. A configured per-hour count limit prevents unlimited event reporting.

Locations: `app.js` bytes 920361 (frame rounding), 920559 (memory accumulator), 923334 (logging policy), 924260 (performance wrapper), 928090 (reporter), 930874 (counters and worker heartbeat), 932230 (report groups), 934304/934363 (5-minute report / 1-second memory timers), 934416 (stop and flush).

The native runtime has even more detailed counters. `Luna::PerformanceCounters::getJsonStats` at **0x004b9cf4** reads heap sizes, heap limit, array/shared buffer bytes, worker heaps, RSS, peak RSS, and swap. Its graphics group includes external/internal processing time, rendering time, drawing context and surface counts, surface memory and high-priority surface memory, vertex buffers, intermediate surfaces and render counts, cache hits/lookups, font memory, software decoder memory, and image ready/loading/error/prefetch/unreferenced counts. Its media group exposes parser usage, audio/video buffer usage, source buffer count, and player count. Relevant serialized field names reside at file offsets **15594830–15595486**, with `RSS` and `peakRSS` at **14081595** and **14081591**. These are counters available to the app; they are not measurements collected in this study.

No fixed key-to-visible-change target, launch-to-usable-Home target, time-to-first-frame target, 2022 OLED percentile, maximum total RSS budget, or acceptable dropped-frame percentage was established. The code also does not identify a single change as the biggest historical performance win. That latter question needs a developer statement or measured comparison between builds.

## 2. Which streaming quality metrics are watched, and what thresholds trigger action?

**Answer: the app tracks playback states and has a specific network troubleshooting heuristic. Its production quality targets remain unknown.** At the session layer, buffering start/stop, playback start/stop, errors, seek start/end, trickplay, resume, and post-play generate tracker events. State normalization pairs interrupted operations with their matching completion event, so an exit/error can finish an in-progress buffering or seeking interval rather than leaving reporting in an impossible state. See `PlayerStore.js` around bytes 41992–43300 and 61703.

The player adapter exposes total and dropped video frame counters if the platform provides them; otherwise it returns a zero-valued fallback. **A zero in that fallback is not proof that no frame was dropped.** See `PlayerStore.js` byte 128980. Runtime reporting can be sampled by remotely supplied player error/debug log probabilities; trace logging is disabled in the observed options. Reporting is annotated with device/firmware/app/player versions, network type, content mode, a QA session identifier, and initiating context. See `PlayerStore.js` byte 106809. This shows what can be reported, not a measured failure rate or buffering ratio.

The shipped quality toolkit makes these exact diagnostic decisions:

| Diagnostic constant or decision | Observed rule | Meaning and limit |
| --- | --- | --- |
| Basic connectivity request | **5,000 ms** timeout per configured URL; tries URLs in sequence until one resolves | Reachability check, not a UI responsiveness target |
| Diagnostic HLS startup phase | **120,000 ms** timeout | Waits for a fragment loaded, fragment changed, or level switched event, or a fatal error; it does not verify visible first-frame timing |
| Diagnostic streaming observation | **30,000 ms** timeout | Diagnostic sample window; not a 30-second tolerance promised for ordinary playback |
| Access-log bandwidth classification | Ignore provisional entries and entries without media requests; take minimum and maximum observed bitrate | Uses access-log values, not Wi-Fi RSSI |
| Good-bandwidth classification | Minimum observed bitrate **>=25 Mbps**, **or** maximum observed bitrate **>=50 Mbps** | Toolkit heuristic; not a universal HD/UHD requirement or adaptive bitrate setting |
| Unknown classification | Missing extrema, or maximum <= minimum | Lack of a useful range is treated as unknown, not automatically poor network quality |
| Streaming failure result | Stall count >0 **and** bandwidth classified as the remaining non-good range | Then shows a localized variable-bandwidth explanation with rounded Mbps range |
| Issues-service HTTP request | **5,000 ms** | Retrieves current issue/error text |
| Periodic issues refresh | Random **300–1,800 seconds**, with a 3-second enclosing fetch deadline | Spreads recurring status fetches; does not define video startup SLA |

Locations: `qos.js` bytes 2462/2476 (25/50 Mbps constants), 15102 (basic network check), 16500 onward (HLS diagnostic event condition), 18205 (stall/range failure condition), 19223/19254 (120/30 second limits), 35384 (range derivation), 8058 (periodic interval), 8455 onward (issues client). The table describes the **troubleshooting tool**, which runs against configured test entities. Do not put these diagnostic waits into Bloob as desired playback budgets.

The native reporter corroborates the reporting architecture: **0x002dbc50** tracks stall transitions, increments a session stall counter, accumulates stall time, and emits a stalled report to registered reporting agents. **0x002dba88** accounts for wall-clock play duration and media-timeline duration separately. **0x002dbecc** responds to source/state/rate changes, seeking, playback end, playback start, and errors. This permits derived quality statistics, but no production alert percentage or fleet percentile can be inferred from the implementation alone.

## 3. What are the common field failures, and how are they recovered?

**Answer: error categories and recovery flows are visible; their field frequency is not.** The general HTTP wrapper distinguishes DNS failure, connection timeout, read timeout, other transport failure, HTTP responses >=400, and successful requests taking more than 20 seconds. It retains recent error details and passes requests to an observer. Its request timeout is selected from application configuration when the caller omits one; this source does not establish the live configured value.

The wrapper has a **5-second** slow-request interval and a **20-second** warning threshold. The shown interval calls uptime; the visible body does not emit a warning at five seconds. The concrete slow-completion classification is the 20-second branch. See `app.js` bytes 936326–938900. These are operational logging choices, not actual performance measurements.

The quality toolkit first asks an issues service for error text tailored to device/platform/player context. It can keep issue descriptions in a periodically refreshed cache. If it has an applicable configured troubleshooting flow, it offers retry and troubleshooting. Otherwise it produces a localized generic unavailable message with acknowledgment. The troubleshooting sequence separates basic Internet reachability, ability to start a stream, and ability to sustain streaming; it can be stopped, and its session identifier suppresses late events after cancellation. Its modes, supported error types, basic URLs, and test entities come from configuration. See `qos.js` bytes 4000–8100, 22115 onward, and 28800 onward; `QualityOfService.js` connects the toolkit to the app/player.

The implementation does not establish which error is most common in LG deployments, how often a recovery succeeds, or which failure occurred in a particular user's run. Those require retained fleet reports or an actual controlled trace. A string or recovery branch is evidence of handling capability, not failure prevalence.

## 4. How are slow or dropping Wi-Fi handled?

**Answer: the visible policy separates connection state, transport failure, streaming adaptation, and an explanatory recovery UI.** The runtime adapter derives its online state from device network properties and exposes Ethernet/Wi-Fi/no-network types. A compatibility shim translates an older network representation into that newer representation. See `app.js` byte 1832236. Player reporting tags the network type, and the quality toolkit checks actual stream behavior rather than assuming Wi-Fi itself is defective.

When basic reachability fails, the toolkit asks the user to check the network. When a stream cannot start, it reports a playback connection problem. When streaming stalls and the observed bandwidth range is below its good classification, it reports the variable range and advises checking the connection. Retry/troubleshoot buttons are shown only where configured. The adaptive stream engine's segment retry/variant-selection policy is a separate playback finding; these toolkit thresholds must not be mistaken for that policy.

There is no evidenced policy here based on Wi-Fi signal strength, packet loss, access-point model, or a universal automatic source re-resolution service. In particular, these reference artifacts cannot establish an AllDebrid link renewal design. Bloob must own that resolver behavior.

For an original app using documented webOS services, LG's [Connection Manager](https://webostv.developer.lge.com/develop/references/connection-manager) exposes Internet availability and wired/Wi-Fi connection state and permits subscriptions to status changes. That is an available platform mechanism, not a claim that this runtime uses that exact public endpoint.

## 5. How are crash and error reports collected from TVs?

**Answer: native crash information is supplied as a runtime device property, then enriched and forwarded by the app's reporting layer.** The device wrapper parses a serialized `crashReport` property; absent or unparsable data yields no report. After logging initialization, the reporter checks consent, merges the crash report with app/build/device/network environment and a client identifier, records it locally, and mirrors it to the reporting system. See `app.js` bytes 1864591 and 943859.

JavaScript warnings/errors can include stack traces and the same expanded environment. Logging has recursion guards, remote sampling and maximum-event configuration, and a flush limiter. The default ordinary logging configuration is probability 1 with **five events per session**, unless remote configuration changes it. Debug output flushes after a group of **50 logs**, and ordinary flushes are throttled to once per **60 seconds** unless forced. Performance event limits use a different policy and must not be conflated with ordinary error log limits. See `app.js` byte 939832 onward, 943859 onward, and 923334.

This confirms a transport path for reports and policy controls. It does not prove the contents or frequency of real production crash reports, server-side symbolication, crash deduplication, retention period, or the contents of the live device property. No crash was induced or report uploaded during this study.

## 6. How does the team test on real TVs?

**Answer: the client supports QA session tagging and test category labels; the team's lab and automation process are not recoverable from this package.** Performance groups and player imports carry `qaSessionId` and `testCategory`; the QA session identifier is also passed to the runtime through an application control message. First-launch/build-change metrics and launch interruptions are handled explicitly. These are useful hooks for automated or manual QA, but neither proves a lab, automated remote input, screen capture, or a test coverage matrix.

No real-TV test automation harness, authoritative model coverage list, pass/fail run archive, or engineer description of the lab was identified in the bounded operational study. Any historical traces in this workspace establish only the specific runs they record, not the team's release certification process or the live behavior of this OTA snapshot.

The base `res/trace.json`, lines 23–40, defines dedicated D-pad/key traversal trace filters and an internal-build testing trace filter which includes `Automator`. The production filter includes configuration traces. This is evidence that an automation/testing trace category exists and internal versus production logging is intentionally different. It does not reveal how the automation is driven, whether it runs on physical TVs, or the lab's test coverage.

LG explicitly states that its [simulator](https://webostv.developer.lge.com/develop/tools/simulator-introduction) has different media specifications from actual TVs and does not support every platform feature. That is why a simulator result cannot supply the missing real-TV numbers.

## 7. How are UI updates rolled out without a store release?

**Answer: the shipped entrypoint downloads and mounts an update before importing bootstrap, and persists a usable update plus an ETag. Server rollout decisions remain outside the package.** The updater sends `If-None-Match` only when both the stored ETag and stored diff exist. A 304 response mounts the saved diff. A 200 response attempts to mount the downloaded bytes in memory and then save the diff/ETag when persistent storage is permitted. A failed request, other HTTP status, or missing response data attempts the saved version. If neither mounts, the code proceeds to bootstrap from the underlying base resources.

There are **three default retries** for specific missing/unmountable update paths, with a **3,000 ms request timeout**. The retry count is bounded; the snippet does not show a jittered exponential rollout-retry policy. A downloaded zero-length body is rejected because the mount interface can otherwise interpret it as an empty replacement filesystem. A saved diff which cannot mount is cleaned up together with its ETag. Persistence failures also invoke cleanup. These are code paths in the reference, not actions performed by this investigation.

Locations: OTA `main.js` bytes 6162 (updater), 6239 (default retry count), 8493 (start), 9253 (conditional request), 10248 (304), 10826 (empty-body guard), 12673 (request timeout), 15868 (saved mount), 17695 (save), 22458 (updater invocation before bootstrap).

**Kill-switch nuance:** base `res/main.js`, lines 183–190, has an actual secure-storage `disableOTA` existence check at the start of its updater. The update snapshot's `main.js` mentions the same behavior in a comment but omits that check from the shown implementation. The observed update caller is compiled with a constant enabled branch. Consequently, the base artifact contains a local OTA-disable mechanism, but the effective behavior depends on which entrypoint is running before the update is mounted. Neither version proves a remotely operated kill switch.

The URL identifies a platform/build/production channel, but the artifacts do not reveal server-side percentage cohorts, canary groups, geographic rollout, rollback policy, cache invalidation rules, or criteria used to withdraw an update. Returning an update is a server decision; the client code cannot answer how that decision is made. There is also no evidenced recovery from arbitrary exceptions after the mounted application has begun running: startup download/mount fallback is a narrower guarantee.

## 8. How are older webOS versions protected from incompatible changes?

**Answer: the app uses device property and build matching, platform capabilities, compatibility shims, and feature gates. This is not a declared supported-version matrix.** `DeviceCapLimitsParser` at `app.js` byte 49521 matches properties by exact value or list, supports starts/ends/includes matching, can invert named collections, and supports build comparisons. Configured device collections can limit bitrate, frame rate, frame size, mixed resolutions/aspect ratios, and certain presentation features. The live configuration values and model lists are not present in the inspected source.

The LG adapter reads OS/firmware information and exposes those limits. Multi-view is gated by **OS version >=10.1**, permitted model-family substrings **o24/o22/o26**, and detection of a second MSE player. After multi-view it requires restarting the player. Pointer input is disabled for model names containing **27ART10AKPL**. Video size animation is throttled on physical LG devices. These are explicit exceptions, not assumptions that every TV needs identical behavior. See `app.js` bytes 65706–68161; specific multi-view compatibility check at 67592 and required restart at 67958.

Compatibility wrappers in `app.js` also adapt old network-property representations and older drawing context/style signatures. This is evidence that the app maintains a runtime abstraction which can absorb platform differences. It does not prove how many historical OS versions are currently supported, that every gate has been exercised, or that this one snapshot is safe on all older models.

LG publishes separate platform/API compatibility tables in its [Luna Service introduction](https://webostv.developer.lge.com/develop/references/luna-service-introduction) and [web engine specifications](https://webostv.developer.lge.com/develop/specifications/web-api-and-web-engine). Bloob needs its own supported device/runtime contract; the reference's hidden device configuration is not a portable matrix.

## 9. Which interfaces are partner-only, and which public ones are enough?

**Answer: the artifact proves a custom native runtime and direct LG media adapter, but it cannot prove commercial entitlement rules.** The runtime's native adapter and sample submission functions are present; the JavaScript UI calls its own application/media/graphics interfaces. Those are distinct from the publicly documented `luna://` webOS service APIs, despite the shared word Luna. The reference's runtime API is an application-owned ABI, not automatically a reusable LG public SDK.

LG publicly documents HTML media playback, [mediaOption](https://webostv.developer.lge.com/develop/guides/mediaoption-parameter) for initial playback position and supported adaptation options, [Connection Manager](https://webostv.developer.lge.com/develop/references/connection-manager), audio controls, device information, and application lifecycle. LG's [resume guide](https://webostv.developer.lge.com/develop/guides/resuming-media-with-mediaoption) explains that supplying the initial position can avoid loading at zero and then seeking. These establish useful public routes for an original player.

The bounded official documentation search did not establish public support or authorization for this executable's exact elementary-stream/sample submission ABI. Calling that ABI partner-only based solely on symbol presence would overstate the evidence. An official API/SDK agreement or current LG statement is required to classify access.

## 10. What LG quirks does the code actually compensate for?

The concrete quirks evidenced here are model-specific pointer suppression, OS/model/player-count gating for multi-view, mandatory player restart after multi-view, throttled video window size animation, property-format compatibility, style/drawing-call compatibility, configurable codec/bitrate/frame-rate/frame-size limits, and explicit pause/reporting lifecycle handling. Native player statistics may be absent, in which case the adapter returns a fallback. The OTA mount rejects empty downloaded files to avoid an empty-filesystem replacement edge case.

These tell Bloob to centralize platform decisions and treat reported capabilities as inputs to source selection and player lifecycle. They do not supply physical HDR/Atmos/eARC confirmation, a universal supported format list, or model-independent track-switch behavior. Those require the playback evidence and the relevant actual hardware path.

## Direct implications for Bloob

An original Buna/Bloob runtime can implement the general mechanisms visible here: staged timing marks, frame-time distributions, separate memory/media/graphics counters, worker heartbeat latency, buffering/seek state intervals, cancellation-aware diagnostics, localized recovery choices, versioned device capability rules, constrained telemetry, and startup update fallback. Those mechanisms are sufficiently clear to specify an implementation.

The numbers to set as release targets still need to be chosen by Bloob and verified for the target TV. An operational counter identifies what to measure; a diagnostic timeout identifies how long a troubleshooting procedure waits. Neither reveals the reference team's intended UX budget, production field performance, historical best optimization, failure prevalence, testing organization, or engineering regrets.
