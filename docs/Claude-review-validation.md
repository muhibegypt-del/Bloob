# Validation of the 15 October 2026 branch review

**Inspected commit:** `fb4f41248f0ce76a81dcd5ec473617d4a3fd9975` on `claude/sleepy-ptolemy-o87vgv`. This is the review's baseline `8790488` plus three documentation files. `git diff 8790488 fb4f412 -- native tv-app` is empty, so every finding applies to the code exactly as reviewed. The application version is 2.3.0.

**Method:**
- **Source tracing.** I followed each finding through its callers, cleanup paths and tests.
- **Self-contained probes.** They live in the session scratchpad, not in the repository:
  - `service-probes.js` exercises the real service modules. Every storage location is redirected into a fresh temporary directory, and `HOME` is pointed at a temporary directory too. No pairing key, account, progress or settings store was read or written.
  - `session_probe.cpp` links the real `libbuna_core.a` (session, play, model) from the desktop build, with a fake `PlayerPort`. It feeds media events in controlled orders.
- **Out of scope:** no TV was contacted, no package was installed or rebuilt for this review, and no screen was visually inspected.

**Evidence labels:**
- **source**: call-path analysis;
- **probe**: an isolated run of real modules;
- **test**: an existing test;
- **TV**: an observation on the TV, of which there are none in this document.

## 1. Verdicts F01–F10

One row per finding. The causal chain, corrections and fix invariant follow in the section for each finding.

| ID | Severity and user-visible consequence | Verdict | Where (verified at `fb4f412`) | Evidence and result |
| --- | --- | --- | --- | --- |
| F01 | P1. The resume point, watched state or edition is lost when closing within 3 s of leaving playback | **Confirmed**; the native half is a race, not a certainty | `player.cpp:156,189,391,419`; `tasks.cpp:44`; `main.cpp:283–306`; `progress.js:18,47`; `server.js:491–494` | Source. Service probe: accepted, but no file 300 ms later; the file appears after 3.5 s |
| F02 | P1. Settings or progress silently not saved, while the API says 200 | **Confirmed** | `account-store.js:46,141–166,168`; `progress.js:244–257`; `server.js:272` | Source. Probe: 4 and 12 concurrent writes all returned `null` |
| F03 | P1. A paused viewing resumes by itself after a reload; buffering can leave it paused | **Confirmed** | `session.cpp:24,85–88,94–105`; `player.cpp:215,242,250` | Probe: three event orders, intent wrong in each |
| F04 | P1. The timeline and saved progress claim a seek target that was never confirmed; seek accuracy is over-reported | **Confirmed** | `session.cpp:110–118,134–142`; `media.cpp:251`; `player.cpp:208–212,391` | Probe: position 120000, landed 1, error 0 without any clock report. An existing test encodes it |
| F05 | P1. A subtitle shows as chosen even if refused; failure is invisible | **Confirmed**. What the TV acknowledges needs TV evidence | `session.cpp:151,250–258`; `player.cpp:563,633–636`; `media.cpp:63–90,238–245` | Source. Probe: `subtitle_index=2` with no acknowledgement |
| F06 | P1. Up next can play a different release from the one recorded | **Confirmed** | `ui.h:126–129`; `sources.cpp:211–215`; `player.cpp:441–466` | Source |
| F07 | P1. *Other versions* can open the abandoned version; abandoned jobs keep running | **Confirmed**, plus an uncancelled-job hole | `sources.cpp:200,226,273,288,297` | Source |
| F08 | P2. On the fallback route: no welcome, preferences not applied, the first change overwrites the others | **Confirmed**, and slightly worse than stated | `main.cpp:181–217,221–281` | Source |
| F09 | P1. *Resume* plays from the start for long titles near the end | **Confirmed** | `play.cpp:5–8,88`; `play.h:13`; `completion.js` | Probe: service 13 740 000 ms, native 0. An existing test encodes the old rule |
| F10 | P2. Full redraws on every bus and worker wake; the cost is unknown | **Confirmed** as a mechanism; the cost needs TV evidence | `main.cpp:111–120,751,754,794`; `luna.cpp:51–62` | Source |

### F01: final progress can disappear when playback or the app closes

**Native side.** `PlayerScreen::save_progress` (`native/src/player.cpp:391`) calls `send_progress` (`:419`). That calls `Api::post(..., alive_)` (`native/src/api.cpp:25–62`), which queues work on `Tasks::run` with the screen's `alive_` token as owner.

`Tasks::loop` (`native/src/tasks.cpp:37–55`) skips a queued job whose owner has expired (`if (job.owned && job.owner.expired()) continue;`). It does this before running the work, so the HTTP request is never sent.

Three paths submit a final report and then destroy the screen:
- **Leaving playback.** `exit()` (`:236`) calls `close()` (`:189`), which saves with state "stopped", then `ctx().pop()`. `Op::Pop` (`main.cpp:348–361`) drops the player in the next `apply_ops`, because a screen over video is never kept as `leaving_`.
- **Up next.** `play_next()` (`:428`) calls `close()` and then replaces the screen.
- **App close.** `App::shutdown()` (`main.cpp:283–306`) calls `background()` on each stacked screen (save "paused"), then `stack_.clear()`. That runs `~PlayerScreen` (`:156`), which calls `close()` (save "stopped") and then destroys `alive_`.

**Correction to the review.** The native loss is a race, not a certainty. There are two API workers (`main.cpp:116`). Normally one is idle and picks the job up within microseconds; once a job has started, the request completes even if its owner is gone, and only the reply callback is dropped. The final report is lost only when both workers are already busy, for example with a slow `/api/sources/ranked` or a resolve poll. In that case the job is still queued when the screen is destroyed. The shutdown wait (`main.cpp:291`, up to 1.5 s while `pending()`) cannot help, because dropped jobs are no longer pending.

**Service side.** `progress.save` updates memory and returns `{accepted:true}`. `scheduleWrite` (`tv-app/service/progress.js:47`) writes 3 s later (`WRITE_DELAY_MS`, `:18`). Two shutdown routes exit sooner:
- The bus `stop` handler exits after 300 ms (`server.js:491–494`).
- The fallback route sends `SIGTERM` (`main.cpp:293`). The service installs no `SIGTERM` handler, so Node exits at once.

Neither route flushes progress first.

**Probe.** `save` returned `accepted: true`. Exiting 300 ms later left no `progress.json`. The same save, left to run for 3.5 s, did produce the file. This matches the review.

**User effect.** Closing Bloob within 3 s of leaving an episode loses that episode's resume point, watched state and edition. Every graceful close is inside that window, since closing usually follows leaving.

**Invariant for the fix:**
- A final checkpoint is owned by the app, not by the screen.
- The service writes it durably before acknowledging it as durable.
- Shutdown waits a bounded time for that acknowledgement before stopping the service.

### F02: concurrent saves collide on one temporary file

**Source.** `writeVerifiedFile` (`tv-app/service/account-store.js:168`) always writes `${file}.${process.pid}.tmp`. Two writes in flight therefore share one temporary path. One write's `rename` moves the other's file away, and the loser's `rename` or read-back throws.

`createFileStore.write` (`:141–166`) catches every error and returns `null`. `saveSettings` (`progress.js:244–257`) awaits `settingsStore.write(current)` and ignores the result. `/api/settings` (`server.js:272`) then replies 200 with the merged settings. The account store's `writeVerified` (`account-store.js:46`) uses the same naming pattern.

**Probe.**
- A sequential write succeeded.
- Four concurrent writes: all four returned `null`, and the file ended up holding the first value.
- Twelve concurrent writes: all twelve returned `null`, and the file held the last value.

**Probe limit.** My attempt to show an unwritable store being reported as success did not work: the container runs as root, so `chmod` does not block writes. The failure propagation above is established from source, not from that probe.

**Who writes concurrently?**
- Settings: each Settings change posts the full settings object. Two quick toggles produce two writes.
- Progress: the 3 s timer serialises writes in normal use. Overlap needs a write that takes longer than 3 s.
- Account: pairing and unpairing are rare.

**Invariant:**
- Exactly one writer per document.
- Unique temporary names.
- The caller learns whether the change is in memory only or on disk, and a storage failure reaches the API reply.

### F03: backend transport events overwrite user intent

**Source.** `Session::on_event` sets `intent_play_ = true` on `Playing` (`native/src/session.cpp:94–101`) and `false` on `Paused` (`:102–105`). `Session::open` (`:24`) resets the whole session, so intent defaults back to Play.

`PlayerScreen::reload` (`player.cpp:215`) closes and reopens the session at the last known position. `Loaded` then calls `port_->play()` whenever `intent_play_` is true (`session.cpp:85–88`). `foreground()` (`:250`) reloads after a pipeline loss. `background()` (`:242`) pauses through `session_.pause()`, and that pause is lost when the session is reopened.

**Probe:**
- Playing → BufferingStart → Paused → BufferingEnd: `intent_play=0`, phase `paused`. A backend that pauses while buffering would leave the viewer paused.
- User Pause → late Playing: `intent_play=1`, phase `playing`.
- Paused, then reopened: `intent_play=1`. A reload resumes playback the viewer had paused, and this includes Bloob's own background pause.

**Note.** Whether the LG pipeline actually sends `paused` during a rebuffer, or late `playing` events, is unknown (TV). The ownership defect holds regardless.

**Invariant:**
- `play()`, `pause()` and lifecycle policy write intent; backend events never do.
- Intent survives a reload.
- A lifecycle pause is recorded separately from a user pause, with a generation, so returning from the background restores what the viewer last chose.

### F04: a requested seek is treated as confirmed playback time

**Source.** On `SeekDone` (`session.cpp:134–142`), the session sets `media_ms_ = *pending_`, counts the seek as landed and adds zero error. `UmsPlayer` emits `SeekDone` with no payload (`media.cpp:251`). `viewer_position()` (`player.cpp:208–212`) returns `*pending_` while a seek is unconfirmed, and `save_progress` persists it (`:391`).

The existing test encodes this behaviour (`native/tests/session_test.cpp`, seek section).

There is a better confirmation path already: a `Time` report within 3 s of the target, more than 300 ms after the seek (`session.cpp:110–118`). `SeekDone` short-circuits it.

**Probe.** `seek_to(120000)` followed by a timestamp-free `SeekDone` gave position 120000, landed 1, error sum 0. The probe also shows that before `SeekDone` the clock correctly kept 1000; the fault is only in `SeekDone`.

**User effect:**
- The timeline jumps to the target and claims it.
- Leaving during an unconfirmed or failed seek saves a position never reached.
- `[qoe]` reports perfect seek accuracy that was never measured.

**Invariant:**
- Preview, requested and confirmed times are kept apart.
- `SeekDone` completes the command; only a fresh clock report confirms the landing.
- Progress saves the last confirmed position, never a pending target.

### F05: track selection is optimistic and failure is invisible

**Source:**
- `Session::select_text` (`session.cpp:250–258`) sets `subtitle_index_` before any reply.
- `PlayerScreen` closes the panel on OK (`player.cpp:633–636`) via `choose_track` (`:563`).
- `UmsPlayer::command` (`media.cpp:63–90`) calls `done(ok)` only if the caller passed `done`; `select_track` and `set_subtitles` pass none. A refused `selectTrack` is logged; only a refused play, pause or seek becomes `Lost`.

Audio is closer to acknowledged: `trackSelected` updates the selected flags for audio and text (`media.cpp:238–245`), and the audio label reads that flag. The subtitle label reads the session's `subtitle_index_`, and `Session::on_event` ignores `TrackSelected` (`session.cpp:151`), so subtitles never consult it.

**Probe.** `select_text(2)` gave `subtitle_index=2` immediately; the port received `text:2 subs:on`.

**Needs TV evidence.** Whether this TV's uMediaServer emits `trackSelected` for both audio and text, and what a refusal looks like. The desktop tour fakes that event. No private method or payload should be assumed.

**Correction to `ENGINE-PLAN.md`.** §13 is marked **Have**; it should be **Partial**.

**Invariant:**
- A selection is a pending operation, confirmed by `trackSelected` or by a reply.
- On timeout or refusal it reverts and the menu says so.
- Preference is stored as language and role, never as an index.

### F06: prepared-next reuse matches display text, not source identity

**Source.** `Ctx::Prepared` (`native/src/ui.h:126–129`) stores `video_id`, `job_id` and `headline`. `SourcesScreen::start_job` (`sources.cpp:200`, check at `:211–215`) reuses the job when `chosen_.headline == prep.headline`.

`PlayerScreen::prepare_next` (`player.cpp:441–466`) prepares the recommended candidate. If the viewer then picks a different candidate whose headline is the same (common: "4K · Dolby Vision · Atmos"), the prepared job for the other candidate is reused. Meanwhile `played.candidate = chosen_.id` records the one picked (`sources.cpp`, `on_job` ready branch). Playback and version continuity then disagree.

**Note.** On the direct path (Up next → `start_playback(..., autostart)`), the auto-picked recommended candidate is usually the prepared one, so the mismatch needs a manual pick or a ranking change between preparing and starting. That is source analysis, not an observed incident.

**Invariant.** Reuse only when the candidate ID and episode video ID match. The headline is presentation only.

### F07: resolve operations on the same screen have no generation

**Source.** `SourcesScreen::on_job` (`sources.cpp:226`) accepts any reply while the modal is Resolving, ChooseFile or ChooseQuality, and sets `job_id_ = j["id"]`. `poll()` (`:273`) and `start_job()` (`:200`) capture only `alive_`.

Concrete sequence: start A; *Other versions* (`show_others`, `:297`) or a failure's *Try the next best*; start B; A's reply arrives while the modal is Resolving again. A becomes the current job. If A reaches "ready", the player opens with A's URL while `chosen_` is B.

**An additional hole the review did not name.** `show_others` and `cancel_job` (`:288`) send `/api/resolve/cancel` only when `job_id_` is already known. If A's start reply has not arrived yet, A is never cancelled on the service and keeps running.

A related source fact (A1 below): API jobs run newest-first, so a cancel can even overtake its own start.

**Not claimed.** A destroyed Sources screen cannot reopen a player; `alive_` covers that.

**Invariant:**
- Each resolve has an attempt ID and generation.
- Replies and polls must match the current generation and job ID.
- Cancelling an attempt with no job ID yet is recorded and honoured when the job ID arrives.

### F08: fallback startup skips settings and account initialisation

**Source.** `start_service()` (`main.cpp:181–217`) loads `/api/settings` (seven preferences, recent searches, `settings_loaded`) and `/api/account` (`paired`, `account_known`).

`start_service_fallback()` (`:221–281`) only sets the port and `paired`. It never loads settings, and never sets `settings_loaded` or `account_known`.

**Worse than stated:**
- `BrowseScreen::maybe_welcome` requires `settings_loaded && account_known`, so the first-run welcome never appears on the fallback route.
- `Ctx::save_settings` posts every field. One change in Settings therefore overwrites the saved language, subtitle, motion, transparency and contrast choices with defaults.
- On the bus route, a failed settings GET is not retried, which leaves the same state.

**Needs TV evidence:** which route the TV takes. The log says either `service ready on port … (version …)` or `(started directly)`.

**Invariant.** Both routes call one `on_service_ready()`, and the app retries until settings and account are loaded. Until then, no settings write is sent.

### F09: native resume policy disagrees with the service

**Source.** `play::resume_point` (`native/src/play.cpp:5–8`) returns 0 at 95% or more, with the comment "the service marks a video watched at 95%" (`play.h:13`). That is stale since 2.1.0: `completion.js` now uses the last 7%, bounded to 1–10 minutes.

`continue_item` (`play.cpp:88`) applies the native rule to Continue cards. The title page uses the service's `target.position` (`play.cpp:45–62`). `play::episode` (episode row) and the fallback `title_default` use the native rule.

**Probe:**
- 240-minute film at 229 min: the service gives watched=false and resumes at 13 740 000 ms.
- Native gives `resume_point = 0`, and `continue_item` gives `resume_ms = 0`.
- So the Continue card says *Resume* and then plays from the start.

The existing `play_test` encodes the old rule (96% → 0).

**Invariant.** The service decides resume once (`completion.resumePoint`). The app uses the position it is given. The native rule is only an explicitly versioned offline fallback that mirrors `completion.js`.

### F10: wakeups force full redraws when nothing visible changed

**Source:**
- Every worker completion and every Luna reply calls `push_wake` (`main.cpp:111–120`, `luna.cpp:51–62`).
- The loop sets `dirty_ = true` for every wake event (`main.cpp:751`, `:754`) and renders when dirty (`:794`).
- During playback, uMediaServer's `currentTime` subscription arrives through Luna. Each arrival therefore redraws and presents a full 1920×1080 transparent frame, even with the HUD hidden.

**Needs TV evidence:**
- How often `currentTime` arrives on this TV.
- What a hidden-HUD frame costs.

The `[perf]` lines can answer both once frames are attributed to their cause. Source inspection cannot claim a CPU or frame benefit.

**Invariant.** Events are processed, and a redraw happens only when visible state changed or a deadline is due.

## 2. Additional findings

| ID | Finding | Evidence | Weight |
| --- | --- | --- | --- |
| A1 | Requests made while both API workers are busy run newest-first. `Api::request` (`api.cpp:29`) uses `Tasks::run` with the default `urgent = true` (`tasks.h:21`), which does `push_front` (`tasks.cpp:23–26`). Two quick settings saves can land older-last (full-object writes), and a resolve cancel can run before its start. Progress is protected by `seq`. | source | P2; worsens F02 and F07 |
| A2 | `first_frame_ms` is set on `Playing` or the first positive `Time` (`session.cpp:99`, `:123`), not on visible video, yet logs call it "first frame". | source | Metric naming |
| A3 | `build.sh` rejects GLIBCXX imports but does not enforce the "GLIBC_2.4 only" maximum that CLAUDE.md and ENGINE-PLAN state. The current binary imports only `GLIBC_2.4` (`readelf -V` on the build output), so it complies by accident, not by guard. | source + build output | Release |
| A4 | `Luna::retired_` (`luna.cpp:83`, `:96`, `:119`) keeps every finished or cancelled call record and its callback until `stop()`. This protects `on_reply` from a freed `ctx` pointer after cancel. Growth is per command (play, pause, seek, TTS speak), so it is unbounded over a long session but small per call. | source | P3; needs a reclamation rule tied to the bus's no-more-callbacks guarantee |
| A5 | Home waits for every catalogue row (`catalog.js:85`, `Promise.all`), so the slowest list sets first content. The catalogue is cached in memory only. | source | P2 startup |

### Remaining engine gaps, assessed

None of these is a confirmed failure. They are gaps that limit what can be measured or guaranteed.

- **Resource accounting.**
  - Only GPU texture bytes are budgeted (192 MB, evicted to 75%, `images.cpp:472–478`).
  - Not counted: CPU decode buffers in flight, completed-but-not-uploaded pixels, glyph atlases and shapes.
  - Worker queues are unbounded, and `Tasks::pump` (`tasks.cpp:57–67`) runs every completion in one call.
  - Texture creation and upload happen inside those completions on the UI thread (`images.cpp:445`), so a burst of ready images lands in one frame.
  - Plausible frame-time spikes on Home; unmeasured.
- **Luna retired records (A4).**
  - The record is the bus callback's userdata. Freeing it while libhelpers may still deliver a reply, or touch the context after the filter returns, would be a use-after-free (`luna.cpp:96`, `:119`).
  - Retiring is therefore correct protection, and any reclamation must happen at a point where libhelpers provably no longer holds the pointer.
  - The bound is the gap, not the protection.
- **Focus and layout.**
  - Each screen owns its focus and its hand-placed layout.
  - Restoration works because screens below stay alive on the stack.
  - There is no shared rule for interruptions such as a toast, sheet or player over a page.
- **Startup (A5).** Home's first content waits for the slowest row, and nothing is cached on disk.
- **Text and accessibility.**
  - stb_truetype with on-demand glyphs; no shaping, bidi or font fallback (Arabic and CJK cannot render).
  - Atlas growth rebakes on the UI thread (`gfx.cpp:116–200`).
  - Speech is a string per screen (`spoken()`), not a semantic tree.
- **Metrics provenance (A2).**
  - `[perf]` key timing ends when `SDL_RenderPresent` returns, a submission proxy, as `perf.h:3` says. It is not light on the panel.
  - "First frame" is Playing or the first positive media time, not a visible frame.
- **Packaging (A3).**
  - The ABI guard rejects GLIBCXX only; the GLIBC maximum is unenforced.
  - `requiredACG` is unset.
  - The app runs on a rooted TV, which proves nothing about the permissions an ordinary device would grant.
- **Source expiry and recovery.**
  - A released pipeline reopens the same URL, at most three times a minute. A failed load shows the error choices.
  - There is no re-resolve of the same version (APP-MAP 9.5 is still open).
  - A sample-stream route would add capabilities. It is not an established remedy for expiry or for any format issue.
- **Full→Lite (exists).**
  - After any 5 s perf window with at least 20 frames, a render p95 above 25 ms counts as slow. Two slow windows in a row switch effects to Lite for the rest of the session (`main.cpp:635–643`).
  - Limits:
    - it is one-way;
    - it measures CPU-side render time only, not presentation;
    - it ignores windows with few frames, so a short animation stutter never triggers it;
    - `BLOOB_EFFECTS=full` pins it.

The suspected task-completion wake bug was not reproduced by the reviewer and is not included here.

## 3. Strengths confirmed

| Strength | Confirmed by |
| --- | --- |
| Platform-free playback core behind `PlayerPort` | `session_probe.cpp` links it with no SDL or TV |
| Pure play decisions (`play.*`) with tests | `play_test.cpp`, probe |
| Service separation (catalogue, rank, resolve, progress, completion, next) | 57 service tests pass at this commit (re-run after 2.3.0) |
| Image workers, keyed variants, stale-job skip, frost readiness | `images.cpp`, `common.cpp` |
| Transparent canvas over the video plane | `gfx.cpp` final copy keeps alpha |
| Direct Play with a cancellable sheet; ranked source sections | `sources.cpp`, `rank.js` |
| Progress v2 ordering (session and `seq`) | `continuity.test.mjs`; it protects against A1 reordering of progress |
| Honest `[perf]` labelling | `perf.h:3` |

The fixes below keep all of these. None of the ten findings needs a retained scene, a render thread or another media backend.

## 4. Corrections applied to `native/ENGINE-PLAN.md`

| Section | Was | Now | Why |
| --- | --- | --- | --- |
| §5 Frames | Partial (meets the idle rule) | Partial: idle sleeps, but any wake redraws | F10 |
| §11 Session | Partial | Partial, with F03 and F04 named | Probes |
| §13 Seek, tracks, captions | **Have** | **Partial** | F04, F05 |
| §14 Continuity | Partial | Partial, with F01, F02, F06, F07 and F09 named | Probes and source |
| §18 Inspector | Gap | Partial: logs and the present proxy exist; no snapshot or causal trace | `perf.h`, logs |
| §19 Packaging | Have (GLIBC 2.4 ABI guard) | Partial: GLIBCXX rejected, GLIBC maximum not enforced, ACG unset | A3 |
| E7 | DV MKV engaging as the completion criterion | A hypothesis; the sample route is justified only if a diagnosed capability gap is shown to be closable by it | Review §6 |

The plan's phases are superseded in order by `native/RELIABILITY-IMPLEMENTATION-PLAN.md`, which comes first.

## 5. Limits of this validation

- No TV evidence. Five questions need it: LG's track and seek acknowledgements (F05, F04), whether the backend pauses while buffering (F03), which service route the TV takes (F08), and the `currentTime` rate and hidden-HUD frame cost (F10).
- The probes are scratchpad files, not repository tests. The plan turns each one into a regression test.
- The F01 native race was established from source. It was not reproduced with saturated workers.
