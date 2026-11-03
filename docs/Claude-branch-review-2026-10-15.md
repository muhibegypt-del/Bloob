# Bloob Native: implementation review

**Reviewed:** 15 October 2026.

**Branch:** `claude/sleepy-ptolemy-o87vgv`.

**Commit:** `8790488` — engine upgrade plan.

**Application version:** 2.3.0.

## 1. Assessment

This branch is a substantial engineering advance. It contains a complete native application path: a C++ executable, app-owned SDL rendering, input/navigation, artwork processing, a platform-independent playback core, LG uMediaServer integration, a separately packaged local service, source resolution and continuity logic.

The strongest work is the separation of product screens, native media, pure playback decisions and service responsibilities; resource-conscious artwork handling; coherent direct-play/source-choice flows; and the move toward accurate next-action and version-continuity behaviour.

The engine is an integrated application runtime with several strong foundations. It has not yet met the full Buna specification. More immediately, there are correctness gaps in persistence, playback state, source identity and cancellation that should precede a broad renderer refactor.

The existing tests passed. Focused probes still exposed failures because the suite largely covers expected event sequences and isolated policy functions. Several tests currently encode the very assumptions that need changing, especially seek acknowledgment and the old completion threshold.

### Review boundary

This was a source review, local core/service verification and IPK structure inspection. No TV was contacted, no package was installed and no existing TV file was changed. I did not render or visually inspect the current native application. WSL could not start because host virtualization is disabled; there is no working Linux/SDL review environment in this session.

The recorded claim that 2.2.0 was extremely smooth is a prior user report in [CLAUDE.md](../CLAUDE.md), not a measurement made during this review. It supports preserving the working native foundation. It does not establish 2.3.0 performance or correctness.

## 2. Priority findings

P1 means a normal viewing, source-choice or persistence journey can become incorrect. P2 means an important performance, robustness or product-completeness gap. These priorities describe the code paths; they do not assert a measured frequency on the TV.

### F01 — P1: final progress can disappear when playback/app closes

**Sources:** [PlayerScreen::save_progress/send_progress](../native/src/player.cpp), lines 389–431; [App::shutdown](../native/src/main.cpp), lines 283–299; [progress.scheduleWrite](../tv-app/service/progress.js), lines 18 and 47–57; [service stop](../tv-app/service/server.js), lines 491–494.

There are two independent lifetime problems:

1. The player's final report is submitted with its screen-owned `alive_` token. Removing the screen can expire that token before the queued work starts; `Tasks::loop` then drops it. A final progress save needs ownership beyond the screen being closed.
2. The service acknowledges `progress.save()` after updating memory, while disk persistence waits three seconds. The app waits at most about 1.5 seconds for API work and then terminates the fallback service; the service-bus stop path exits after 300 ms. Neither path flushes progress first.

**Local probe:** an isolated progress save returned accepted. Simulating the service's 300 ms exit left no progress file. Letting the same process complete its normal timer produced the file.

**User effect:** leaving an episode and closing Bloob can lose the latest resume point, watched state or continuation change despite an apparently successful save.

**Fix direction:** provide a service-owned serialized progress writer and explicit durable flush. Critical checkpoints survive screen disposal; shutdown waits for a bounded durable acknowledgment before stopping the service. Routine coalesced writes can remain inexpensive.

### F02 — P1: concurrent saves collide on the same temporary file

**Sources:** [writeVerifiedFile/createFileStore](../tv-app/service/account-store.js), lines 144–178; [saveSettings](../tv-app/service/progress.js), lines 244–256.

Every write to a given document uses the same temporary name based on filename and process ID. Concurrent writes can overwrite or rename each other's temporary file. `createFileStore.write()` catches these failures and can return null; `saveSettings()` ignores that result and still returns the settings object as a successful update.

**Local probe:** a sequential isolated write succeeded. Four concurrent writes through the same store all returned null in this run. A separate twelve-write run also returned null for every caller. A final file existed in the four-write case, but this does not establish which operation was durably accepted or make its failed acknowledgments correct.

**User effect:** rapid preference changes can appear saved without a reliable durable result. This undermines the pairing/progress persistence work even though the chosen storage locations and permissions are sensible.

**Fix direction:** serialize writes per document, use unique temporary names and distinguish an accepted in-memory update from a durable write. Report terminal storage failure. Atomic rename is useful only inside a complete concurrency and acknowledgment contract.

### F03 — P1: backend transport events overwrite user intent

**Sources:** [Session::on_event](../native/src/session.cpp), lines 94–105; [Session::open](../native/src/session.cpp), line 24; [PlayerScreen::reload/background/foreground](../native/src/player.cpp), lines 215–256.

`Playing` unconditionally sets `intent_play_` true. `Paused` unconditionally sets it false. That combines the user's desired action with a backend's observed transport state.

**Local probes:**

- Playing → BufferingStart → Paused → BufferingEnd left the session with Play intent false and phase Paused.
- A user pause followed by a late Playing event changed intent back to true and phase back to Playing.

Reload introduces another route: reopening resets the session with its default Play intent, so a resource-loss recovery can resume a viewing the user had paused. Backgrounding also pauses without retaining a distinct pre-interruption intent generation.

**User effect:** internal buffering or delayed callbacks can alter what the viewer asked for; recovery can resume unexpectedly or remain paused for the wrong reason.

**Fix direction:** commands own user intent. Backend callbacks update observed state only. Preserve intent across reload and use operation/generation identity to reject stale transport results.

### F04 — P1: requested seek position is treated as confirmed playback truth

**Sources:** [Session::on_event SeekDone](../native/src/session.cpp), lines 134–142; [PlayerScreen::viewer_position/save_progress](../native/src/player.cpp), lines 205–211 and 389–398; [seek tests](../native/tests/session_test.cpp).

On `SeekDone`, the session assigns the requested target to `media_ms_`, counts a landing and effectively records zero error without receiving a landing timestamp. The existing unit test explicitly expects this behaviour.

Separately, `viewer_position()` returns an unconfirmed pending target and `save_progress()` persists it. Exiting during a failed seek can therefore save a position the viewer never reached.

**Local probe:** a seek to 120,000 ms followed by a timestamp-free SeekDone reported position 120,000, one landed seek and error sum zero. No backend clock confirmed that position.

**User effect:** an attractive timeline can briefly report fiction, resume can jump to an unwatched target, and the seek-quality metric can look perfect while landing accuracy is unknown.

**Fix direction:** retain preview/requested/acknowledged time separately. A timestamp-free completion establishes command completion, not an exact landing. Await a fresh post-seek clock and persist confirmed media position; expose pending state independently.

### F05 — P1: track selection is optimistic and failure is not exposed

**Sources:** [Session::select_text](../native/src/session.cpp), lines 250–258; [PlayerScreen::choose_track/key](../native/src/player.cpp), lines 563–571 and 629–638; [UmsPlayer::command/select_track](../native/src/media.cpp), lines 61–105.

The subtitle index changes immediately when a command is issued. The track panel closes immediately after OK. Audio selection can eventually reflect a `trackSelected` event, but there is no pending selection transaction, acknowledgment timeout or user-facing failure path. Command failures are logged; ordinary track commands do not forward a failure result to the menu.

**Local probe:** selecting subtitle index 2 changed `subtitle_index()` to 2 with no acknowledgment.

The richer labels are real work and a worthwhile improvement. Their existence does not make switching reliable. `ENGINE-PLAN.md` marking this contract Have is too generous.

**Fix direction:** keep requested and selected tracks separate. Resolve the operation through an applicable acknowledgment; show failure and retain a usable menu when selection fails. Handle Off as a real operation too. Save language/role intent rather than a numeric track index across episodes.

### F06 — P1: prepared-next reuse matches display text instead of source identity

**Sources:** [PlayerScreen::prepare_next](../native/src/player.cpp), lines 439–463; [Ctx::Prepared](../native/src/ui.h); [SourcesScreen::start_job](../native/src/sources.cpp), lines 209–220.

The prepared-next record stores video ID, job ID and headline. Reuse checks that the selected version's headline matches the prepared headline. Many distinct sources legitimately share a label such as “4K · Dolby Vision · Atmos”.

If the user selects another candidate with the same headline, the existing job can resolve the original candidate while `played.candidate` identifies the newly selected candidate. Playback and recorded version continuity can disagree.

**Fix direction:** retain candidate ID, episode/file identity and relevant source-preference revision in the prepared record. Compare those identities before reuse. Headline is presentation data.

### F07 — P1: same-screen resolve operations have no generation check

**Sources:** [SourcesScreen::start_job/on_job/poll/show_others](../native/src/sources.cpp), lines 199–323; [Tasks ownership](../native/src/tasks.cpp), lines 38–65.

Screen lifetime guards are useful, but they cannot distinguish multiple operations while the same screen remains alive. `on_job()` accepts a reply based on modal state and replaces `job_id_` from its payload.

Concrete possible sequence: start A → open Other versions while A's start/status reply is in flight → start B → A's reply arrives while the modal is Resolving again. A can become the current job or open the wrong player. Leaving and destroying the screen is protected; superseding work within that screen is not.

**Fix direction:** a watch-attempt ID plus a generation per resolve operation, captured by each callback. Status replies must match both the active attempt and job ID. Cancelling a job and discarding its replies are separate obligations.

### F08 — P2: fallback service startup skips saved-settings initialization

**Sources:** [App::start_service](../native/src/main.cpp), lines 193–211; [App::start_service_fallback](../native/src/main.cpp), lines 221–279; [Ctx defaults](../native/src/ui.h).

The bus-start path loads settings, recent searches and pairing readiness. The direct-Node fallback sets the port and reads account state, but does not perform the settings fetch or set the same readiness flags.

**User effect:** the same app can start with different audio/subtitle, autoplay, motion and contrast preferences depending on how its service launched. A later full-settings save can overwrite persisted choices with untouched defaults.

**Fix direction:** both startup routes call one service-ready initialization routine, with explicit error/retry state for settings and account loading.

### F09 — P1: native resume policy disagrees with the service's completion policy

**Sources:** [play::resume_point](../native/src/play.cpp), lines 5–8; [kResumeMaxPercent](../native/src/play.h), line 13; [completion policy](../tv-app/service/completion.js); [play::continue_item](../native/src/play.cpp), lines 89–107.

The service uses its new completion policy: a seven-percent remaining allowance bounded between one and ten minutes. Native code retains a 95-percent cutoff and a comment claiming that is the service policy. The service-target path and local Continue/episode paths therefore disagree.

**Local cross-language probe:** for a 240-minute film at minute 229, the service says unfinished and returns resume position 13,740,000 ms. The native function returns zero. Continue can start from the beginning while the title page still offers Resume.

**Fix direction:** return the resolved resume action/position consistently from the service and use it across all entry points. Remove the duplicated old policy, with an explicitly versioned fallback for offline/older-service use.

### F10 — P2: wakeups force full UI redraws even when visual state is unchanged

**Sources:** [App::loop](../native/src/main.cpp), lines 749–755 and 794; [Luna::on_reply](../native/src/luna.cpp), lines 51–62; [PlayerScreen::draw](../native/src/player.cpp), lines 757–789.

Every worker/bus wake event sets `dirty_` true. During playback, clock notifications wake the loop even with the HUD hidden; those wakeups force a complete transparent UI redraw and present. Several events can be batched, but there is no distinction between state work and visible invalidation.

The idle-friendly loop is a good foundation. Calling it fully equivalent to the reference's dirty-scene path would overstate the current implementation.

**Fix direction:** process events first, then invalidate only for changed visible state or a real deadline. Keep session/progress updates active while the hidden overlay remains visually idle. Measure event rate and rendering cost before choosing partial-damage or retained-scene work.

## 3. What is particularly good

### 3.1 A genuine native product path exists

[CMakeLists.txt](../native/CMakeLists.txt), [main.cpp](../native/src/main.cpp), [platform.cpp](../native/src/platform.cpp) and [media.cpp](../native/src/media.cpp) establish a real native executable and LG service integration. The native package has its own app/service IDs and port, and can coexist with the web prototype.

SDL supplies platform/window/rendering facilities while Bloob owns its screens and interaction logic. This is a legitimate engineering foundation for Buna.

### 3.2 The core is separated enough to reason about

`Session` talks through `PlayerPort`. `play.*` expresses playback decisions without SDL or I/O. Catalogue, ranking, resolution and storage live in service modules. This allowed the existing tests and the review probes to run without a TV or the native UI.

That separation should be extended through snapshots, structured operations and capability results rather than replaced wholesale.

### 3.3 Artwork and materials receive real engineering attention

[images.cpp](../native/src/images.cpp) decodes/scales on workers, keys variants, skips stale queued image work, prepares blurred/frosted artwork, tracks image texture bytes and evicts older textures. [common.cpp](../native/src/common.cpp) coordinates background and frost readiness before transitions. Logos are requested at a stable large size rather than re-decoded for each animated size.

These are concrete measures addressing television CPU/GPU constraints. They are more significant than merely choosing attractive colours.

### 3.4 Rendering has a useful low-work foundation

The main loop waits for events/deadlines; animations report when they are active. [gfx.cpp](../native/src/gfx.cpp) preserves alpha through its offscreen canvas and final copy, important for an underlay video plane. Shapes/shadows/sheens use cached resources. Visible shelf ranges are bounded in [BrowseScreen::draw_shelf](../native/src/browse.cpp).

F10 and draw-time cache misses need attention, but the structure already provides an incremental path to lower overhead.

### 3.5 The interaction flows are becoming coherent

- Direct Play has a quiet starting sheet with cancellation and access to other versions.
- Source choice separates recommendation, alternatives and problematic versions; technical release details are progressively disclosed.
- Details keeps seasons and episodes within one title context.
- Search has debounce, a current-query guard and a keyboard that yields space to results.
- Back returns through owned layers, with one top-level key recipient.
- Continue refresh reconciles by item/episode identity.

These are supported in [sources.cpp](../native/src/sources.cpp), [details.cpp](../native/src/details.cpp), [search.cpp](../native/src/search.cpp), [browse.cpp](../native/src/browse.cpp) and the shell. Current appearance and physical ergonomics were not visually assessed in this session.

### 3.6 Continuity has become a real product subsystem

[progress.js](../tv-app/service/progress.js), [completion.js](../tv-app/service/completion.js) and [next.js](../tv-app/service/next.js) now provide session/sequence ordering, watched policy, resume/next/caught-up decisions, dismissal and persistent version preference. [rank.js](../tv-app/service/rank.js) incorporates version continuity and compatibility warnings.

This directly addresses the earlier complaint that each episode selected an unrelated quality. F01/F02/F06/F09 affect reliability, but the domain model is a major improvement.

### 3.7 Design values are centralized and traceable

[theme.h](../native/src/theme.h), [ui.cpp](../native/src/ui.cpp) and [TOKENS.md](../native/TOKENS.md) connect typography, spacing, button/card state, shadows and timing to named roles and recorded study values. Shared measurement/drawing logic and optical button centring are useful craft improvements.

Equivalent numbers do not establish equivalent rendered appearance across different fonts/layout/renderers. The centralization still makes adjustment and review substantially easier.

### 3.8 Capability, lifecycle and diagnostics are being treated explicitly

There are separate focus/hidden events, bounded pipeline reload attempts, richer source track metadata, network state, source-vs-TV HDR logging, reduced effects and TTS integration. [PerfWindow](../native/src/perf.h) explicitly identifies its measurement endpoint as SDL_RenderPresent return rather than light emitted by the panel.

The Full→Lite policy really exists in [App::render](../native/src/main.cpp): two sufficiently populated slow windows can lower effects. The branch therefore contains more than a static tier switch, although this policy is coarse and has no automatic return to Full.

## 4. Remaining engine gaps

### 4.1 Resource limits are partial

The 192 MiB artwork texture threshold is a post-upload eviction trigger, not a total CPU/GPU/process budget or strict admission bound. Recently used entries are protected, and decoded buffers/completions, fonts, shapes, shadows, frost work, render targets and media allocations are outside that figure.

`Tasks` limits worker count but not queued jobs or completion memory. `pump()` drains every completion in one call, so a batch can perform several texture uploads before another input opportunity. [Font::draw/flush/bake](../native/src/gfx.cpp) can rasterize/rebuild/upload an atlas on the UI/render thread.

Additionally, [Luna::retired_](../native/src/luna.cpp) keeps finished/cancelled call records and their captured callbacks until application stop. Retiring contexts protects late callbacks, but currently has no bounded reclamation scheme. This is cumulative retention, not a measured leak rate.

Needed: per-class accounting, in-flight admission, upload/completion budgets, queue limits/priorities and safe reclamation after platform callback quiescence.

### 4.2 The graphics/focus runtime is mostly screen-local

Screens directly draw and each implements directional navigation, focus animation and geometry. Uniform shelf culling is useful; there is no general retained scene, shared semantic focus graph, layout invalidation model or snapshot boundary.

The immediate consequence is duplicated rules and fragile consistency as screens gain features. A separate render thread is not automatically required. A shared layout/focus/lifetime layer can be introduced on the existing renderer, preserving the working UI while reducing duplication.

Partial redraw must respect framebuffer preservation and effect extents. Adding damage rectangles without that contract can create artifacts; it should not be treated as an unconditional improvement.

### 4.3 Startup/offline catalogue continuity is limited

[catalog.js](../tv-app/service/catalog.js) caches in memory. The native app normally stops its service on exit. Home collects all configured catalogue requests before returning rows, so the slowest request can delay useful content despite faster rows being available.

Persistent artwork is helpful, but it does not supply a persisted catalogue. A cached initial catalogue/progress surface with background refresh would improve repeated launch and offline behaviour. Current first-useful-content timing remains unmeasured here.

### 4.4 The media contract needs capability and operation truth

The URI/uMediaServer route is concrete. There is no implemented segmented/sample adapter, normalized capability-with-reason schema, live seekable-window model, output-provenance snapshot or renewable source lease.

The important near-term work is to establish accurate URI-backend state and limitations. Missing a sample route does not by itself make the existing hardware playback path poor.

The `first_frame_ms` field is set on Playing or the first positive time event, not on verified video presentation. Rename it to its actual event or add separate presentation evidence. Keep the honest RenderPresent caveat already present in `perf.h`.

### 4.5 Text and accessibility have significant completion work

The font code has UTF-8 decoding, kerning, on-demand glyphs and careful handling of malformed input. It has no shaping engine, bidi layout or font fallback chain. Unsupported scripts can become question marks; shaping-sensitive languages cannot be made correct through glyph lookup alone.

Speech derives strings from individual screens and invokes TTS after a settle delay. There is no semantic accessibility tree with persistent identity, relationships and action/state exposure. Device TTS integration is still unverified. The onscreen search keyboard is limited to English letters/digits and basic editing actions.

### 4.6 Recovery has bounded retries but limited cause modelling

Pipeline reopening is limited, and error screens preserve useful actions. Errors remain mostly strings, with no source renewal contract or uniform operation-specific recovery. Some ordinary command refusal can be classified as pipeline loss without a richer diagnosis.

Recovery should preserve user intent, confirmed position and source identity. Repeated failures need actionable cause/category rather than repeated reopen attempts on the same expired URL.

### 4.7 Packaging is real; release evidence needs tightening

I inspected the committed 2.3.0 IPK without installing it:

- 39 archive entries.
- Native manifest identifies `com.muhib.bloob`, 2.3.0, entry `bin/bloob`.
- Executable is ELF32, little-endian ARM, flags `0x05000200`.
- No world-writable archive entries were found.
- No account/progress/settings data filenames or obvious reference-artifact names were present in the inspected entry list.
- Service JavaScript is minified in the package; raw byte comparison with checkout source is therefore not proof of a source mismatch or an exact build match.

Package SHA-256: `9f49aae9c65a8b2e32300a6243a18dd647b0991781536f8f612c4bed4bb9779f`.

The build script checks for GLIBCXX dependency and corrects package modes. It does not implement the full advertised “GLIBC_2.4 only” maximum-version guard. The manifest also lacks a resolved requiredACG declaration; root-TV success cannot establish an ordinary installation permission contract. Several spec links refer to documents absent from this branch.

These are release/portability gaps, not evidence that the existing package cannot run on the owner's rooted TV.

## 5. UX assessment

| Pillar | Assessment | Reason |
| --- | --- | --- |
| Task completion | Strong direction, reliability incomplete | Direct Play, contextual next action and progressive source detail reduce effort; identity/acknowledgment bugs can defeat the action selected |
| Quality craft | Substantial source-level improvement | Shared roles, typography, optical measurement, artwork/frost readiness and focused card ordering are concrete; this session did not assess current pixels or frame pacing |
| Trustworthy behaviour | Needs work | Storage success, selected track, seek landing and first-frame metrics sometimes claim more than the backend/durable state establishes |

Additional experience gaps:

- Pointer clicks activate the focused item; there is no pointer hit-testing. This is an intentional compatibility bridge, but pointing at one card and activating another is an unresolved pointer UX.
- Language/role track intent is not carried comprehensively across episodes; numeric selections are principally retained for same-player reopen.
- Version identity is not yet strong enough to support edition-aware resume across different cuts/files.
- Several pages maintain their own return/focus policies; consistent stable-ID restoration is not yet universal.
- Source availability and format labels are heuristics derived from release metadata. The UI/service must preserve that provenance as actual backend data becomes available.
- Catalogue hero selection remains a simple rule over available rows and artwork. A recommendation-first presentation is present; a fully personalized/editorial discovery system is not established.

## 6. How I would amend the engine plan

The broad E1/E2 emphasis is sound. The order needs a data-integrity repair stage and more precise completion criteria.

| Order | Work | Concrete result |
| --- | --- | --- |
| 1 | Durable persistence and one resume policy: F01/F02/F09 | Pause/exit settings and progress survive normal close, and every entry point chooses the same resume action |
| 2 | Session/operation truth: F03/F04/F05 | User intent survives backend events; position and tracks show acknowledged state with pending/failure paths |
| 3 | Watch/source identity: F06/F07 | Prepared work and late replies cannot play or record a different selected version |
| 4 | Shared service-ready initialization: F08 | Startup behaviour/preferences are identical across bus and fallback routes |
| 5 | Inspector and accurate metric labels | Read-only current session/page/capability/operation/resource state, with real endpoint/provenance for each timing |
| 6 | Event invalidation and resource bounds: F10/§4.1 | Hidden HUD avoids unnecessary redraws; queued/decoded/uploaded work has explicit bounds |
| 7 | Shared layout/focus, text and accessibility | Expand the existing renderer incrementally with reusable interaction/lifetime contracts |
| 8 | Additional backend only for a diagnosed gap | A demonstrated capability improvement with preserved session semantics |

Do not use “Dolby Vision is not engaged” as sufficient evidence that an ES/sample route will fix it. Container/profile/metadata, platform support and composition/output behaviour must be distinguished. `ENGINE-PLAN.md` E7's Dolby Vision completion criterion is currently a hypothesis, not an established route to success.

The plan's Have mark for track/seek behavior should become Partial pending the identified fixes. Its inspector assessment should credit existing log/RenderPresent-proxy instrumentation while identifying the missing snapshot/causal interface. Render ownership should be assessed by correctness and frame cost; the number of threads alone is not a quality score.

## 7. Verification performed

### Existing suites

- `vitest run tests/service`: **5 files, 57 tests passed**.
- Native core suite: **passed**, covering 11 session/JSON routines and four play-decision routines.
- Core source was compiled with the installed Zig C++ compiler for a Windows host. A temporary compatibility header supplied POSIX `localtime_r` through Windows `localtime_s`; product source was not changed. This verifies the platform-independent core, not SDL, libhelpers or TV playback.

### Focused probes

| Probe | Result |
| --- | --- |
| Progress accepted, process exits after 300 ms | No progress file persisted |
| Same progress save, allow normal timer to finish | Progress file persisted |
| Sequential document write | Succeeded |
| Four concurrent writes to one document | All returned null in this run |
| Buffer-induced backend pause | User Play intent changed to false |
| Late Playing after user Pause | User Play intent changed to true |
| SeekDone without landing clock | Requested 120,000 ms treated as landed, error zero |
| Subtitle selection without acknowledgment | Selected index changed immediately |
| Four-hour title at minute 229 | Service resumes at 13,740,000 ms; native fallback returns zero |
| Worker completion wake | Fired in the local probe; not reported as a confirmed wakeup bug |

The persistence probes used isolated temporary stores with a custom location provider, not the user's account/progress locations. No upstream catalogue/debrid account was required. Source-identity and same-screen cancellation findings are established by call-path analysis, not claimed as observed TV incidents.

### Not verified here

Full native UI build/rendering, physical-TV latency, hidden-HUD CPU/GPU cost, actual HDR/Atmos/Dolby Vision output, source-specific track switching, TTS, installation permissions and long-session memory behaviour.

The scripted UI tour is a useful fixture/screenshot harness, but injects media events and has no equivalent end-to-end assertions for the live LG backend. Its composited still-image player screenshots cannot prove moving video output.

## 8. Conclusion

Keep this native foundation. It already embodies a meaningful portion of Buna's intended architecture and contains substantial work worth preserving.

The next quality leap comes from making state and identity reliable: durable progress, one resume policy, preserved user intent, confirmed seeks/tracks, exact source identity and operation cancellation. Then extend the inspector and resource contracts so performance work follows evidence.

The current branch is a capable native product prototype with strong visual/runtime foundations. A finished Buna engine requires these cross-cutting contracts to hold consistently across the complete viewing lifecycle.
