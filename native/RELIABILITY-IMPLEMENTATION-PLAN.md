# Reliability implementation plan

**Status: planned. Nothing in this file is implemented.**

**Basis.** This plan is grounded in [`docs/Claude-review-validation.md`](../docs/Claude-review-validation.md), which validates the 15 October 2026 branch review at commit `fb4f412`. The code there is identical to `8790488`, Bloob Native 2.3.0. Finding IDs F01–F10 come from the review; A1–A5 come from the validation.

**Relation to other plans.** It comes before [`ENGINE-PLAN.md`](ENGINE-PLAN.md) phases E1–E8 and absorbs the parts of them that the findings need:
- E1: the intent axis, a confirmed clock and track operations.
- E2: a read-only snapshot.
- E3: attempt IDs.

Layer numbers refer to [`APP-MAP.md`](APP-MAP.md).

**Why a new R sequence.** APP-MAP increments belong to one layer each. Every increment here crosses layers (for example the player, the shell and the service). Each R increment therefore names the APP-MAP increments it closes, and APP-MAP is updated when an R increment lands.

**What it keeps.** The Glass design, the token system, the immediate-mode renderer, the uMediaServer URI route and the service split all stay. No increment needs a render thread, a retained scene or another media backend.

## Sequence at a glance

| ID | Purpose | Findings | Size | Depends on |
| --- | --- | --- | --- | --- |
| **R1** | Saved progress and settings are durable, ordered and honest; one resume policy | F01, F02, F09, A1 (settings) | M | none |
| **R2** | Both service start routes reach the same initialised state; no writes before load | F08 | S | R1 (durable settings reply) |
| **R3** | Playback truth: intent ownership, confirmed clock, acknowledged tracks | F03, F04, F05, A2 | M | R1 (saves the confirmed position) |
| **R4** | Watch attempts: source identity, resolve generations, reliable cancel | F06, F07, A1 (resolve) | M | R3 (attempt ID on the session) |
| **R5** | Inspector snapshot, visible invalidation, bounded queues and records | F10, A2, A4, review §4.1 | M | R3 and R4 (state to show) |
| **R6** | Shared focus and layout, text, accessibility, startup rows | review §4.2–4.4, A5 | L, split | R5 (measurements) |
| **R7** | Another media route, only for a diagnosed capability gap | none yet | L | R5 evidence |

**Why R1 ships first.** It is the only increment that loses data the viewer cannot recover. A lost resume point or a reverted setting survives the session and every restart, while the playback-truth defects in R3 heal on the next action. R1 is also mostly service code with unit tests that run on any machine, so it is the cheapest to verify, and R3 depends on it: a confirmed position helps only if it reaches the disk.

**Why R2 comes before R3.** F08 can overwrite every saved preference with defaults the first time the viewer touches Settings. That is the same data-loss class as R1. The fix is small, and R1's durable settings reply is what makes "do not write before load" checkable.

---

## R1: durable continuity

### Identity and purpose

- **Findings:**
  - F01: the final report is lost, either by screen-owned cancellation or by the service exiting before its delayed write.
  - F02: concurrent writes collide, and failure is reported as success.
  - F09: two resume rules disagree.
  - A1: settings saves can run newest-first.
- **Layers:** 9 Player engine, 10 App shell (closes APP-MAP **10.2**, "flush the last progress save before quitting"), 14 Data and workers, 19 Progress and settings store. ENGINE-PLAN §14, §15, §19.
- **User symptoms:**
  - "Resume" is missing, or it is an older position, after leaving an episode and closing Bloob.
  - A Continue card says *Resume* but plays from the start (240-minute film at minute 229).
  - Two quick Settings toggles can leave the older value saved.
- **Invariants:**
  1. The final report of a viewing is owned by the app, not by a screen, and is sent even if the screen is gone.
  2. The service acknowledges a final report as *durable* only after the bytes are on disk and read back.
  3. Shutdown waits a bounded time for that acknowledgement, and the service flushes before it exits, on every stop route.
  4. One writer per document. Temporary names are unique. The caller is told *memory* or *durable*, or the failure.
  5. The service decides where to resume. The app uses the position it is given, and its offline fallback mirrors the service rule, tested from one shared table.
  6. Settings saves from one app reach the service in the order they were made.

### Scope

**Service (`tv-app/service`), keeping the old-Node syntax rule (no `?.` or `??`):**
- `account-store.js`:
  - `writeVerifiedFile` (`:168`): temporary name `${file}.${process.pid}.${counter}.tmp`, where `counter` is a module-level increment.
  - `createFileStore` (`:141`): a per-file serial queue (a promise chain). If several writes wait behind the one in flight, only the newest runs (latest wins). `write()` resolves to `{ ok: true, location }` or `{ ok: false, code }`, never `null`; the current callers that test for `null` are updated with it.
  - `writeVerified` (`:46`): same naming fix.
- `progress.js`:
  - New `flush()`: clears the timer, writes the current state through the serial queue and resolves `{ durable, code }`. A no-op if nothing changed since the last durable write, tracked with a dirty generation.
  - `save()` (`:77`): when the report is final (`state` is `stopped`, `ended` or `upnext`, or `final: true`), await `flush()` and add `durable` (and `code`) to the reply. Other reports keep the 3 s debounce and reply `durable: false`.
  - `saveSettings()` (`:244`): await the store result and return `{ settings, durable, code }`.
  - `continueWatching()` (`:157`) and `forTitle()` (`:179`): add `resumeAt: completion.resumePoint(e)` to every entry.
- `server.js`:
  - The `stop` handler (`:491`) awaits `progress.flush()` with a 1500 ms cap, then exits.
  - New `process.on('SIGTERM')` and `process.on('SIGINT')` handlers do the same.
  - `POST /api/settings` (`:272`) replies 200 with `durable`, or 503 `{ error, settings }` when storage failed. Memory still holds the change.
  - New `POST /api/flush` returns `{ durable, code }`, for shutdown.

**Native (`native/src`):**
- `api.h`, `api.cpp`:
  - `Api::post_final(path, body, done)` posts with no owner (`Tasks::run(work, nullptr)`), so `Tasks::loop` never drops it. Its callback captures only app-lifetime state.
  - `Api` counts outstanding final posts in an atomic (`finals_pending()`).
- `player.cpp`:
  - `save_progress` (`:391`) and `send_progress` (`:419`): the final reports from `close()` (`:189`), `play_next()` (`:428`) and the destructor (`:156`) use `post_final` with `final: true`. Periodic and paused reports keep `alive_`.
  - The destructor sends nothing if `close()` already sent the final report, so each viewing sends exactly one final report.
- `main.cpp` `App::shutdown()` (`:283`):
  1. Call `background()` on each screen.
  2. Clear the stack.
  3. Wait up to 1500 ms for `finals_pending() == 0`, pumping completions.
  4. `POST /api/flush`, waiting up to 1000 ms.
  5. Stop the service, as today.

  It logs `[continuity] final durable|memory|lost <ms>`.
- `ui.h` / `common.cpp` `Ctx::save_settings`: a single-flight writer. With one request in flight, newer changes are coalesced into one pending full object, sent when the reply arrives. This fixes A1 for settings without a revision field. On `durable: false` or a 503, show the toast "Couldn't save this setting" and keep the in-memory value.
- `model.h` and `model.cpp`: `Watch` gains `long long resume_at = -1` (−1 means absent), parsed from `resumeAt`; `ContinueItem` likewise.
- `play.h` / `play.cpp`:
  - `resume_point` (`:5`) returns `resume_at` when it is 0 or more.
  - Otherwise it applies a native mirror of `completion.isComplete`: the last 7%, bounded to 1–10 minutes, the clip rule under 2 minutes, and a 30 s minimum.
  - `kResumeMaxPercent` and its stale comment (`play.h:13`) are removed. `continue_item` (`:88`), `episode` and `title_default` all go through `resume_point`.

**Tests:**
- One shared table, `native/tests/fixtures/completion-cases.json`: duration, position, state, and the expected watched and resume values. It is read by a new `tv-app/tests/service/completion.test.mjs` and by `play_test.cpp`. The 240-minute film at minute 229 is a row. The current `play_test` case "96% → 0" changes to follow the table.

### State and operations

- **Progress document:** each change increments `generation`, and `durableGeneration` records the last generation written and verified. `flush()` reports `durable: true` only when it was asked for generation *g* and finishes with `durableGeneration ≥ g`.
- **Writer:** at most one write in flight per file, plus one queued (latest wins). Each write gets a unique temporary file, a rename and a read-back.
- **Final report:**
  - It moves through *queued*, *accepted (memory)* and *durable*, or ends *failed*.
  - The app's milestones are counted. `[continuity]` logs which milestone shutdown reached.
- **Timeouts:**
  - The final post uses the HTTP client's existing timeout.
  - Shutdown waits 1500 ms for finals and 1000 ms for flush, so 2.5 s worst case before the existing stop.
  - On timeout, shutdown proceeds; the service's own stop or SIGTERM flush remains.
- **Cancellation:** none. A final report is never cancelled.
- **Settings:**
  - At most one POST in flight per app.
  - The coalesced object always holds the latest value of every field.
  - Replies are applied only if they belong to the latest request; older replies are dropped.

### Ownership

| Thing | Owner | Survives |
| --- | --- | --- |
| Periodic or paused progress report | `PlayerScreen` (`alive_`) | Nothing; dropping it is harmless, because a newer report or the final report follows |
| Final report request | `Api` (app lifetime) | Screen destruction, up to shutdown's wait |
| Final report durability | Service `progress.flush()` | Service stop (bus `stop`, SIGTERM, SIGINT) |
| Settings writer | `Ctx` single-flight | Screen changes. On shutdown, one pending settings write is awaited inside the same 1500 ms |
| Resume decision | Service `completion.resumePoint`; native mirror only without `resumeAt` | Not applicable |

### Service and data changes

- **Schemas:**
  - `POST /api/progress` accepts an optional `final` and replies `{ accepted, durable, code? }` (adds `durable`).
  - `POST /api/settings` replies `{ ...settings, durable }`, or 503.
  - New `POST /api/flush`.
  - `resumeAt` is added to continue items and title progress entries.
- **Compatibility:** all additive. An old app ignores `durable` and `resumeAt`. A new app on an old service finds no `resumeAt`, so the mirror rule is used, and sees no `durable`, which it treats as *memory*.
- **Stored data:** no change to `progress.json` (v2) or `settings.json`. Leftover `*.tmp` files from the old naming are ignored on read. Removing them is a separate, explicit decision: the TV no-delete rule applies to anything outside our own store files.

### Recovery

- **Final post fails or times out:** the service may still hold the previous periodic report (at most 15 s old, `kSaveEveryMs`, `player.cpp:25`). That is the fallback, and it is logged.
- **Flush fails** (for example `EACCES` or `ENOSPC`): the reply says `durable: false` with a code. The app logs it, and Diagnostics shows the last storage result.
- **A settings write fails:** toast, memory keeps the value, and the next change retries the whole object.
- **An older service:** the mirror rule is used and durability is unknown; Diagnostics shows "older service".

### Verification

**Service unit tests (Node, temporary directories, every store location redirected):**
1. Twelve concurrent `write()` calls: every promise resolves `ok: true`, the file holds the last value, and no `.tmp` files remain.
2. Final `save()` then `process.exit` 10 ms after the reply: `progress.json` holds the report. Run as a child process.
3. `SIGTERM` 50 ms after a non-final save: the file holds the report.
4. Unwritable store (inject a failing `fs.rename` rather than rely on `chmod`, which root ignores): `saveSettings` returns `durable: false` and the route replies 503.
5. The completion table.

**Native tests:**
1. `play_test` reads the shared table, and `resume_at` takes precedence.
2. A tasks test: two workers blocked; post a final report; destroy the owner token; unblock. The final report runs and an owned job is dropped. This reproduces F01's native half on desktop.
3. Single-flight settings: three changes while the first is in flight lead to two requests in total, with the last carrying all three values.

**Desktop tour:** leave playback and quit within 1 s; the next start shows the right Continue position. Use the fixture service with a temporary data directory.

**No TV question remains.** The original one was whether the bus `stop` reaches the service before the app exits. The app's `stop` call is fire-and-forget (`main.cpp:294`, no reply callback, then a 50 ms delay). R1 removes the dependency instead of measuring it: the app waits for the HTTP reply of `POST /api/flush` before it sends `stop`, so durability never rests on a fire-and-forget bus message. See *Answered from public sources*, A1.

### Delivery

1. Service writer and flush.
2. Service routes and fields.
3. Native `post_final` and shutdown.
4. Settings single-flight.
5. Resume table.

Each step leaves the app working, and each lands as its own commit citing `R1` and the APP-MAP ID. Version 2.3.1, with an IPK through `scripts/build.sh`.

Rollback is installing the previous IPK. The stored data format is unchanged, so rollback is lossless.

### Completion

| Stage | Definition |
| --- | --- |
| Implemented | All scope items merged; APP-MAP 10.2 and layer 19 updated |
| Locally verified | All tests above pass; the desktop tour quit-within-1 s case keeps the position |
| TV-verified | On the TV: leave an episode, press Home and close Bloob within 3 s, relaunch, and the resume point matches; the log shows `[continuity] final durable`. Two quick Settings toggles survive a restart |

---

## R2: one service-ready path

### Identity and purpose

- **Findings:** F08.
- **Layers:** 10 App shell (closes APP-MAP **10.1**'s "which path was used"), 8 Settings. ENGINE-PLAN §3.
- **User symptoms:** on the fallback start route, the first-run welcome never appears, saved preferences are not applied, and the first change in Settings overwrites every other saved preference with defaults.
- **Invariant:** both routes call one `on_service_ready(port, route)`. Settings and account are loaded with retry, and no settings write is sent until settings are loaded.

### Scope

- `main.cpp`:
  - `start_service()` (`:181`) and `start_service_fallback()` (`:221`) end in a new `App::on_service_ready(int port, const char *route)`.
  - It gets `/api/settings` and `/api/account`, with up to three tries at 0.5, 1 and 2 s.
  - It sets every field that `start_service` sets today, including `settings_loaded`, `account_known` and recent searches.
  - It logs `[launch] service-ready route=bus|direct`.
- `common.cpp` `Ctx::save_settings`: refuses to send (it queues the change in the R1 single-flight) while `!settings_loaded`, then sends it merged onto the loaded settings.
- **Service lifetime.** The service keeps itself alive with an ActivityManager activity (`server.js:486`). The webOS FAQ ([JS service FAQ](https://webostv.developer.lge.com/develop/guides/js-service-faq)) says a service also stays active while another app or service holds a subscription to it, and the idle timer then exits it 5 s after it becomes inactive. Proposal: the app holds a `luna://<app>.service/alive` subscription for its whole life, and the service drops the keep-alive activity. If the app is killed without calling `stop`, the hub cancels the subscription and the service exits on its own, after the R1 flush (the service flushes on the cancel event). This needs the subscription cancel to reach `webos-service`, which is its documented behaviour; it is checked by `[service] app gone` in the log.
- `settings.cpp`: while settings are unknown, the rows show their values dimmed with "Loading…". If loading fails three times, the screen says so and offers Try again. Copy and design follow the existing component states.

### State and operations

- **States:** `unknown`, `loading(attempt)`, `loaded` or `failed`, separately for settings and for account.
- **Writes:** a write before `loaded` is queued, not sent.
- **Ownership:** all callbacks are app-lifetime (no screen owner), since this is shell state.

### Service and data changes

None.

### Recovery

- **Three failures:** Settings shows *failed* with retry. Playback works, because the defaults are used for display only and are never written.
- **Account unknown:** the pairing prompt waits, rather than claiming "not connected".

### Verification

1. Desktop with `BLOOB_SERVICE_ROUTE=direct`: a new environment switch that forces the fallback for tests. Expect the welcome on first run, and saved preferences applied.
2. Inject one failed `/api/settings`: the retry succeeds.
3. Inject three failures, toggle a setting, then let loading succeed: the saved file keeps its other fields.

**No TV question blocks this.** Which route the TV takes no longer matters once both converge; the log line only records it.

### Delivery

Ships with R1 as 2.3.1, or as 2.3.2 if R1 is already out. One commit. Rollback is the previous IPK.

### Completion

| Stage | Definition |
| --- | --- |
| Implemented | Both routes call `on_service_ready`; save guard in place |
| Locally verified | The three scenarios pass on desktop |
| TV-verified | The log shows the route, and Settings values survive a restart on that route |

---

## R3: playback truth

### Identity and purpose

- **Findings:**
  - F03: transport events overwrite intent.
  - F04: a pending seek is treated as confirmed.
  - F05: optimistic tracks and invisible failure.
  - A2: the first-frame label.
- **Layers:** 9 Player engine (new increments **9.7** intent, **9.8** confirmed clock, **9.9** track operations), 7 Player screen. ENGINE-PLAN E1 (intent axis, operations), §11, §13.
- **User symptoms:**
  - A paused episode can start playing again after the TV menu, an input switch or a pipeline reload.
  - The timeline claims a seek landed when it did not, and leaving then saves a position never reached.
  - A subtitle row shows as chosen even if the TV refused it.
- **Invariants:**
  1. User intent is written by `play()`, `pause()` and lifecycle policy only, never by backend events.
  2. Intent survives `open()` and reload.
  3. A lifecycle pause is recorded separately from the viewer's own pause.
  4. Preview, requested and confirmed time are separate. Only a fresh clock report confirms a seek landing. Progress saves confirmed time.
  5. A track change is pending until acknowledged; it reverts with a visible reason on refusal or timeout. Preferences are stored as language and role.

### Scope

- `session.h` / `session.cpp`:
  - **Intent.** `enum class Intent { Play, Pause }` plus `bool lifecycle_hold_`.
    - `on_event` no longer assigns intent on `Playing` or `Paused` (`:94`, `:102`). The observed transport goes to a separate `Transport` field.
    - `open(..., Intent initial)` takes the intent to carry. `PlayerScreen::reload` (`player.cpp:215`) passes the current one.
    - `Loaded` calls `port_->play()` only if intent is Play and there is no lifecycle hold.
    - When transport says Playing while intent is Pause and no seek or load is in progress, the session sends `pause()` once per transport change, recorded as a correction (`corrections` counter), so the viewer's choice wins.
  - **Lifecycle.** `hold()` and `release()` replace the lifecycle use of `pause()`. `background()` (`player.cpp:242`) holds; `foreground()` (`:250`) releases. A pause the viewer made during the hold stays.
  - **Clock.** It keeps `confirmed_ms_` (the last media-clock report outside a pending seek) and `seek_` as `{ id, target, requested_at, state: previewing | requested | completed | confirmed | failed }`.
    - `SeekDone` (`:134`) moves `requested` to `completed`: the command is done, but the landing is not known.
    - The first `Time` report after `completed` confirms. It records `landed = reported`, the error is `|reported − target|`, and `confirmed_ms_` is updated.
    - In the reference pipeline, `currentTime` is sent every 200 ms and is skipped only while a seek is in progress ([UriPlayer.cpp, `UPDATE_INTERVAL_MS` and the position timer](https://github.com/webosose/g-media-pipeline/blob/df8470b/src/player/UriPlayer.cpp)). A confirming report should therefore arrive within about 200 ms of `seekDone`. One second without one: confirmed from the next report, whatever arrives, and logged `[qoe] seek unconfirmed`.
  - `viewer_position()` (`player.cpp:208`) still shows the target while a seek is in flight, for display. `save_progress` (`:391`) uses `session_.confirmed_ms()`.
  - **Tracks.** `select_audio(i)` and `select_text(i)` (`:250`) create `track_op_ = { id, kind, index, state: pending | confirmed | failed, deadline }`. The selected index used for display stays the previous one until confirmed.
    - Confirmation comes from `trackSelected` with matching kind and index. `UmsPlayer` already updates both the audio and the text flags and forwards kind and index (`media.cpp:238–245`), but `Session::on_event` ignores the event today (`session.cpp:151`).
    - **What a reply means.** uMediaServer's `selectTrack` reply is not an acknowledgement. In the open-source uMediaServer, `Pipeline::selectTrack` validates the type and index, forwards the command to the pipeline process with no reply callback, and returns true; before the media is loaded it only queues the command ([Pipeline.cpp](https://github.com/webosose/umediaserver/blob/af89f8a/src/pipeline/Pipeline.cpp)). The open-source reference pipeline's `selectTrack` handler does nothing and returns true ([service.cpp](https://github.com/webosose/g-media-pipeline/blob/df8470b/src/service/service.cpp)). So `returnValue: true` means *forwarded*, and `returnValue: false` means *rejected* (bad type or index).
    - **The acknowledgement** is the documented `trackSelected` subscription event, `{type, index}` ([uMediaserver.cpp, subscribe](https://github.com/webosose/umediaserver/blob/af89f8a/src/server/uMediaserver.cpp)).
    - **Rules:** `returnValue: false` reverts at once with "Not available on this TV". `trackSelected` confirms. No event within 4 s keeps the choice but marks it *unconfirmed* in the inspector and the log, and does not revert: the TV draws the subtitles, so a revert could undo a switch that worked, and LG's own pipeline is closed source, so whether it sends `trackSelected` for text is not publicly documented. That one fact comes from the existing `[media] <-` log line, which records every `trackSelected` the TV sends, the first time the feature is used; it changes only a label, not the design.
- `media.h` / `media.cpp`: `select_track` and `set_subtitles` pass a `done(ok)` to `command()` (`:63`). The port gains an `on_command_result(kind, ok)` event to the session. Only `returnValue` is read, with the meaning above.
- **Buffering.** In the open-source reference pipeline, a network underrun moves GStreamer to PAUSED and the pipeline then emits both `bufferingStart` and `paused`; at 100% it returns to PLAYING and emits `bufferingEnd` and `playing` ([UriPlayer.cpp, `GST_MESSAGE_BUFFERING` and state changes](https://github.com/webosose/g-media-pipeline/blob/df8470b/src/player/UriPlayer.cpp)). So the F03 buffering sequence is the expected event order, not a corner case, and the intent rule above is required, not defensive.
- **Visible video.** uMediaServer documents a `videoFrame {valid}` subscription event ([uMediaserver.cpp, subscribe](https://github.com/webosose/umediaserver/blob/af89f8a/src/server/uMediaserver.cpp)). The session records the first `videoFrame` with `valid: true` as `first_frame`. Until one arrives the metric is `started`. `videoFrame` is added to the `known` event list in `media.cpp:184`.
- `player.cpp`:
  - The track panel (`choose_track`, `:563`, OK at `:633`) stays open with the row marked "Switching…" while pending.
  - On confirmation it closes. On `returnValue: false` the row shows "Not available on this TV" and the previous track stays checked. After 4 s with no event it closes with the new track checked (unconfirmed).
  - The stored preference is written as language and role, only on confirmation. It uses the existing `language` and `subtitleLanguage` settings plus a new `subtitleRole` (for example forced or SDH).
- `session.cpp` (A2): `first_frame_ms_` is renamed `started_ms_` and logged as `started`. `first_frame` comes only from `videoFrame` (above).
- **Tests.** `session_test.cpp` gains the probe sequences from the validation as regression tests:
  - Playing → BufferingStart → Paused → BufferingEnd: intent stays Play.
  - Pause → late Playing: a correction pause is sent.
  - Paused → reload: no play.
  - `seek_to` → `SeekDone` with no time: position unconfirmed, landed count 0.
  - `SeekDone` → `Time`: landed with the true error.
  - `select_text` without an acknowledgement: stays selected, marked unconfirmed after 4 s.
  - `returnValue: false`: reverts, failed.
  - `bufferingStart` → `paused` → `bufferingEnd` → `playing` (the reference pipeline's order): intent stays Play and no correction is sent.

  The existing assertion that `SeekDone` alone counts as a landing is changed with the reason in the commit.

### State and operations

| Operation | IDs | Acknowledged by | Timeout | Cancel |
| --- | --- | --- | --- | --- |
| Seek | Increasing `seek_.id`; a new seek supersedes a pending one, and a late `SeekDone` for an older ID is ignored by counting (uMediaServer has no ID, so the count of outstanding seeks is used) | `SeekDone` completes; `Time` confirms | 15 s to fail the command (today's value); 1 s for confirmation (5 reference clock periods) | A new seek |
| Track | `track_op_.id` | `trackSelected` (kind, index). `returnValue: false` rejects; `true` only means forwarded | 4 s, then *unconfirmed* (no revert) | Choosing another row supersedes it |
| Intent correction | Not applicable | The next transport event | Not applicable | Not applicable |

### Ownership

- The `Session` owns intent, hold, the seek and the track operation.
- `PlayerScreen` owns presentation only.
- Reload carries intent and confirmed position into the new session.
- The R1 final report uses the confirmed position.

### Service and data changes

- **Settings:** add `subtitleRole` (additive). The existing `language` (audio) and `subtitleLanguage` settings are reused, and are written only on a confirmed choice.
- **Progress schema:** no change; the reported position is now confirmed time.

### Recovery

- **Track refused:** the previous track stays and the reason shows; no retry loop.
- **Seek never confirmed:** the clock follows the next report; logged.
- **Correction pause refused:** it is handled like a refused `pause()` today (`Lost` → reload, which carries Pause intent).

### Verification

1. The unit tests above.
2. The desktop tour with fake media events, including an injected `selectTrack` refusal and a dropped `trackSelected`.
3. A scripted run: pause, `window:hidden`, `media:unloadCompleted`, `window:shown`. It stays paused.

**Answered from public sources** (see the section of that name): a refusal is `returnValue: false`, `paused` does arrive during rebuffering, and the clock runs at 200 ms. The design no longer depends on whether LG's closed pipeline sends `trackSelected` for text; the existing log shows it.

### Delivery

Intent first, then the clock, then tracks, as three commits. Version 2.4.0. Each commit passes `session_test` and the tour. Rollback is the previous IPK; the new settings fields are ignored by old builds.

### Completion

| Stage | Definition |
| --- | --- |
| Implemented | Session axes and operations as above; panel states; preference fields |
| Locally verified | The regression tests and the three tour scenarios pass |
| TV-verified | Paused playback stays paused through the quick menu, the Home key and back, and a pipeline reload. A seek's `[qoe]` line shows a measured landing error. Subtitle on, off and switch confirm or show a reason |

---

## R4: watch attempts

### Identity and purpose

- **Findings:**
  - F06: prepared reuse keyed by headline.
  - F07: no resolve generation, and a cancel lost before the job ID is known.
  - A1: cancel can overtake start.
- **Layers:** 6 Sources, 9 Player engine, 18 AllDebrid (new increments **6.4** attempt IDs, **18.x** cancel by attempt). ENGINE-PLAN E3 (watch-attempt part only; leases stay in E3).
- **User symptoms:**
  - After *Other versions*, the player can open the version that was abandoned.
  - Up next can play a different release from the one the show keeps to, while the app records the other.
  - Abandoned resolves keep running on AllDebrid.
- **Invariants:**
  1. Every Play creates one attempt `{ attempt_id, video_id, candidate_id, file_hint }`.
  2. Every reply and poll carries the attempt ID and is applied only if it matches the current attempt.
  3. Prepared work is reused only on equal `video_id` and `candidate_id`.
  4. Cancel is addressed by attempt and is honoured even if the job ID is not yet known.

### Scope

- `ui.h` (`:126`): `Prepared { video_id, candidate_id, job_id, attempt_id, at }`; `headline` becomes display-only.
- `sources.cpp`:
  - `start_job` (`:200`) creates `attempt_id` (counter plus a random suffix) and sends it in `/api/resolve/start`. Reuse is checked at `:211`.
  - `on_job` (`:226`) and `poll` (`:273`) compare the reply's `attempt` with `attempt_` and drop a mismatch with a `[resolve] stale` log.
  - `cancel_job` (`:288`) and `show_others` (`:297`) send `/api/resolve/cancel { attempt }` always.
  - `played.candidate` comes from the attempt, not from `chosen_`.
- `player.cpp` `prepare_next` (`:441`) records `candidate_id` and `attempt_id`.
- `api.cpp`: resolve start and cancel go through an ordered per-key lane (one in flight per key `resolve`, FIFO) so cancel cannot overtake start.
  - **Implementation:** a small `Api::Lane` that chains requests.
  - Settings already gets this from R1's single-flight. It stays separate because settings coalesce and resolve must not.
- **Service:**
  - `server.js` `/api/resolve/start` (`:291`) accepts an optional `attempt` (`/^[a-z0-9-]{4,40}$/`) and echoes it.
  - `resolve.js` `start` (`:161`) stores it on the job. `status` replies include it.
  - `/api/resolve/cancel` accepts `{ id }` or `{ attempt }`.
  - A cancel for an attempt with no job yet is remembered for 60 s, and a later `start` with that attempt is cancelled immediately.

### State and operations

- **Attempt states:** `preparing`, `resolving`, then `choosing` (file or quality), `ready`, `cancelled` or `failed`.
- **Rules:**
  - A new attempt supersedes the current one: the old one is cancelled by attempt.
  - Late replies for a superseded attempt are dropped.
  - The service keeps tombstones of cancelled attempts (bounded at 64 entries or 60 s).
- **Timeouts:** unchanged resolve timeouts.

### Ownership

- `SourcesScreen` owns the current attempt while it lives. `alive_` still drops replies after it is destroyed, which remains correct.
- `Ctx::Prepared` is owned by the shell and is invalidated when its `video_id` is no longer the next episode.
- Service jobs are owned by the service and are cancellable by ID or attempt.

### Service and data changes

- **Additive:** the `attempt` field on start, status and cancel, plus tombstones.
- **Compatibility:** an older app sends no attempt, and the old behaviour remains.
- **Storage:** no change.

### Recovery

- **Stale reply:** dropped and logged.
- **Cancel lost in transit:** the job still ends by its own timeout, as today.
- **Prepared job mismatch:** a fresh resolve. The cost is one resolve and no wrong playback.

### Verification

1. **Native:** a deferred-reply test harness for `SourcesScreen` logic. The attempt rule is a pure helper in `play.*` or a new `attempt.h`, so it is testable without SDL.
   - Start A, supersede it, start B, then deliver A ready: the player is not opened with A.
   - The prepared candidate differs while the headline is the same: no reuse.
2. **Service:** start with `attempt`, cancel by attempt before start, and the start is cancelled; status echoes the attempt.
3. **Tour:** *Other versions* during resolving, using fixture sources with a delayed first reply.

**TV-only:** none. This is logic only.

### Delivery

Service first (additive), then native, then the ordered lane. Version 2.4.x. Rollback is the previous IPK.

### Completion

| Stage | Definition |
| --- | --- |
| Implemented | Attempt IDs end to end; reuse by identity; ordered lane |
| Locally verified | The three tests and the tour scenario pass |
| TV-verified | One real *Other versions* during resolving plays the chosen version; the log shows the stale drop |

---

## R5: inspector, visible invalidation, bounded work

### Identity and purpose

- **Findings and gaps:**
  - F10: wakes force full redraws.
  - A2: metric names.
  - A4: Luna retired records.
  - Review §4.1: queue and upload bounds, UI-thread atlas and texture work.
- **Layers:** 12 Rendering (closes APP-MAP **12.3**), 14 Data and workers, 22 Observability (closes **22.1**). ENGINE-PLAN E2 (snapshot part), §4.3, §5.
- **User symptom.** None is reported. 2.2.0 was "extremely smooth". This increment makes TV questions answerable, and removes work that does not change the picture.
- **Invariants:**
  1. A frame is rendered only for input, animation, a deadline or a visible-state change, and each frame records which.
  2. Bus and worker wakeups drain events without forcing a frame.
  3. Per-frame completion and upload work is bounded.
  4. Retired Luna records are reclaimed only where it is provably safe, and counted otherwise.
  5. Metric names say what they measure.

### Scope

- **`main.cpp` invalidation.**
  - The wake handling at `:751`/`:754` stops setting `dirty_`.
  - `Ctx::invalidate(const char *cause)` sets `dirty_` and a cause bit, and completion callbacks that change visible state call it. Callers include image ready, data loaded, toasts, the player clock when a visible element shows time, and HUD changes.
  - **Transition safety.** For one release, a debug flag `BLOOB_WAKE_REDRAW=1` restores the old behaviour, so a missed `invalidate` can be told apart from a new bug.
  - `[perf]` gains `frames by cause: input/anim/invalidate/deadline` and `wakes`.
- **`player.cpp`.** With the HUD hidden and no seek, toast or caption-lift change, a `Time` event updates the session without invalidating.
- **`tasks.cpp` `pump`** (`:57`): process completions until a 4 ms budget or 16 items, then re-wake for the rest. Image texture creation counts toward the budget, and at most 2 uploads happen per frame (`images.cpp:445`).
- **`luna.cpp` retirement (A4).**
  - **Why records are retired today.** A record is the userdata for the bus callback. Freeing it while libhelpers may still deliver or touch it is a use-after-free. That is why `cancel` and the one-shot path retire instead of freeing (`:96`, `:119`).
  - **Proposal:**
    - One-shot records whose reply has been delivered are freed from the bus thread on its next idle (`g_idle_add` on the loop's context), after libhelpers' callback frame has returned.
    - Cancelled subscriptions stay retired, because no evidence says when unregistering stops delivery. They are counted in the inspector, with a warning above 256.
  - **Precondition.** Read libhelpers' callback handling in the open-source Moonlight TV and webOS-homebrew sources to confirm one-shot contexts are not touched after the filter function returns. Without that, keep retiring and only count.
- **Inspector (E2 subset, `native/src/inspect.*`).**
  - It writes `/tmp/bloob-state.json` on change, at most once per second, atomically (temp file and rename).
  - Contents:
    - build version and the service route;
    - the screen stack and focused element ID;
    - the session snapshot: intent, transport, confirmed and pending times, seek and track operations, the attempt ID, declared and reported HDR;
    - image bytes and entries; task queue depths; Luna live and retired counts; the last three errors with their domain.
  - **Never included:** URLs, keys, account data or release names.
  - `native/scripts/tv-inspect.sh` reads it over SSH, read-only.
- **Metric labels (A2).**
  - The `[perf]` key measure is renamed `key→submit` (it ends at the `SDL_RenderPresent` return).
  - `first_frame` becomes `started` (R3).
  - **22.1:** an end-of-session `[qoe] summary` line with started, seeks and their landing errors, stalls, reloads and watched duration.

### State and operations

- **Dirty causes** are a bitset, cleared on present.
- **The pump budget** carries over: remaining completions trigger a wake and are handled before the next frame's input.
- **Inspector writes** are coalesced: a change sets a flag, and the next tick at least 1 s after the previous write writes.

### Ownership

- The `App` owns invalidation and the inspector writer.
- `Luna` owns record reclamation on its own thread.
- Screens own their `invalidate` calls.

### Service and data changes

None. The inspector file is in `/tmp` and holds no user data.

### Recovery

- **A missed invalidate shows as stale UI.** `BLOOB_WAKE_REDRAW=1` confirms it, and the frame-cause counter shows it.
- **Inspector write failure:** logged once, and the inspector stops for the session.

### Verification

1. **Desktop:** a scripted playback with the HUD hidden for 30 s leads to zero `invalidate` frames from `Time`. HUD shown: 1 frame per visible second change.
2. **Tasks test:** 100 completions are spread over at least 7 pumps with the 16-item limit, and none is lost.
3. **Inspector:** the JSON contains no `http`, no `apikey` and no release name (grep test in the tour).
4. **Image test:** 10 ready images lead to no more than 2 uploads per frame.

**Known from public sources:** `currentTime` arrives every 200 ms in the reference pipeline, and `bufferRange` on its own 200 ms timer while buffering, so today's player redraws the full frame 5–10 times a second with nothing visible changing. LG's pipeline interface also has `setUpdateInterval(currentTimeInterval, bufferRangeInterval)` ([webos-userland pipeline.hpp](https://github.com/webosbrew/webos-userland/blob/00a5f87/include/player-factory/pipeline.hpp); the same call in [uMediaServer's Pipeline.cpp](https://github.com/webosose/umediaserver/blob/af89f8a/src/pipeline/Pipeline.cpp)). Whether `com.webos.media` accepts it from an app is not documented, so R5 may send it once after load (1000 ms with the HUD hidden, 250 ms with it shown), ignore a refusal, and rely on invalidation either way.

**Measured, not looked up** (these are properties of our own app on this TV, not facts anyone publishes):
- CPU while playing with the HUD hidden, before and after, using the read-only `/proc` sampling in `research/TV-access-and-observation.md`;
- whether one-shot Luna reclamation is safe in practice: the retired count stays flat and there are no crashes over a full episode.

### Delivery

Metric renames and the inspector first (no behaviour change), then the pump and upload bounds, then invalidation behind the flag, then Luna reclamation last and only with its precondition met. Version 2.5.0. Rollback is the previous IPK; invalidation can also be reverted at runtime with the flag.

### Completion

| Stage | Definition |
| --- | --- |
| Implemented | All scope items, with Luna reclamation either implemented with the precondition documented or recorded as count-only |
| Locally verified | The four checks pass, and the tour is visually identical (snapshot diff) |
| TV-verified | `tv-inspect.sh` shows live state, and a before-and-after CPU sample with the HUD hidden is recorded in `research/` with its limits |

---

## R6: focus and layout, text, accessibility, startup rows

Split when started. This is ranked by user impact and is not specified to the same depth until R1–R5 have TV results, because its order should follow what the viewer notices.

| Sub-increment | Purpose | Basis | First step |
| --- | --- | --- | --- |
| R6a Startup rows (A5) | Home shows rows as they arrive; the catalogue cache persists across restarts | `catalog.js:85` `Promise.all`; memory-only cache | The service streams the first two rows, and the rest follow through a `/api/home?part=` call; a disk cache with ETag-like freshness |
| R6b Focus and layout runtime | One focus engine (layers, return anchors, horizontal anchor) and layout primitives | ENGINE-PLAN E5, UX-PLAN X0; review §4.2 | One journey (Home → title → episode → play → Back) on the shared engine, with restoration after artwork eviction |
| R6c Text | Shaping and fallback through one service; atlas rebakes off the UI thread | Review §4.3; `gfx.cpp:116` rebakes on the UI thread | Measure rebake time in `[perf]` first; move it to a worker only if it is above budget |
| R6d Accessibility | A semantic node per focusable item (role, label, position, state) feeding speech | ENGINE-PLAN E8; review §4.4 | Generate `spoken()` from nodes on one screen |

Each sub-increment gets the full field set (scope, state, ownership, data, recovery, verification, delivery, completion) in this file before it starts.

## R7: another media route (conditional)

**Not scheduled yet, but the gap is now documented, not hypothetical.**

- **The gap.** Public user reports agree that LG's built-in player on 2022–2024 software shows Dolby Vision in MKV as its HDR10 base layer, and engages Dolby Vision for profiles 5 and 8 in MP4 or TS. An October 2025 report says webOS 25 on a G4 added DV in MKV ([Jellyfin forum](https://forum.jellyfin.org/t-jellyfin-on-lg-g4-webos25-dolby-vision)); a G2 owner reports DV from MP4 but not MKV ([Plex forum](https://forums.plex.tv/t/issues-with-dolby-vision-on-lg-tv/860541)); remuxing to TS triggers DV on a C2 ([Jellyfin forum](https://forum.jellyfin.org/t-dolby-vision-on-lg-webos)); profile 7 does not engage and is converted to 8.1 in practice. This matches the owner's 2.2.0 report on this G2: "DV titles came up as HDR every time". These are user reports, not LG documentation, so the `[qoe] picture:` line remains the check on this TV, but it is now a confirmation, not a discovery.
- **What closes it, cheapest first:**
  1. **Ranking (no new route).** Prefer DV releases in MP4 or TS when they exist, and stop presenting MKV DV as Dolby Vision on this TV class: show "Dolby Vision (plays as HDR10 here)" through `tv-profile.js`. A small change in R3's scope.
  2. **Container remux in the service.** Serve MKV DV (profiles 5 and 8) to uMediaServer as MPEG-TS, remuxed on the fly by the local service with no transcoding. This is what Jellyfin does for LG. It needs a streaming MKV demuxer and TS muxer in Node on the TV, plus range and seek handling: medium size, and CPU must be measured.
  3. **Sample route.** Demux in the app and feed elementary streams through LG's ES interfaces, as Kodi's webOS port does; users report DV MKV working through Kodi on a C2 ([MakeMKV forum](https://forum.makemkv.com/forum/viewtopic.php?p=150001)). LG's ES interface reports `NDL_ESP_FIRST_FRAME_PRESENTED` ([webos-userland media-common.h](https://github.com/webosbrew/webos-userland/blob/00a5f87/include/ndl-directmedia2/media-common.h)), so it would also give a true first-frame metric. Largest; ENGINE-PLAN E7's scope behind the same session contract.
- **Decision rule.** Option 1 ships with R3. Option 2 or 3 is chosen after R1–R5, by the owner, on cost; both preserve the session contract.

---

## Decisions settled from source

All of these need no hardware:
- final-report ownership;
- the writer design;
- flush semantics;
- the resume authority;
- intent ownership;
- seek confirmation by clock, with a 1 s confirmation window from the 200 ms reference clock;
- track operations as pending, confirmed only by `trackSelected`, and rejected only by `returnValue: false`;
- attempt IDs;
- the single service-ready path;
- invalidation causes.

## Answered from public sources

The first draft listed seven "TV-only" questions. Most are answered by public sources: webOS Open Source Edition's uMediaServer and reference media pipeline, webosbrew's headers from LG TV firmware, LG's webOS TV developer guides, and user reports. LG's own TV media pipeline is closed source. The open-source code shows the documented interface and a reference implementation of it, not LG's exact behaviour, so each answer says which it is.

| # | Question | Answer | Source | Kind | Plan change |
| --- | --- | --- | --- | --- | --- |
| A1 | Does the bus `stop` reach the service before the app exits? | Removed by design. The app's `stop` is fire-and-forget; R1 waits for the HTTP `POST /api/flush` reply before stopping. R2 also ties the service's life to an app subscription, which the hub cancels when the app dies | [webOS TV JS service FAQ](https://webostv.developer.lge.com/develop/guides/js-service-faq) (subscriptions keep a service active; 5 s idle exit) | Documented | R1, R2 |
| A2 | Which start route does the TV take? | Irrelevant after R2: both routes converge. The log records it | Not applicable | Design | R2 |
| A3 | What does a `selectTrack` reply mean, and what acknowledges a track? | `returnValue: true` means forwarded, not switched; `false` means rejected. `trackSelected {type, index}` is the documented acknowledgement | [uMediaserver.cpp](https://github.com/webosose/umediaserver/blob/af89f8a/src/server/uMediaserver.cpp), [Pipeline.cpp](https://github.com/webosose/umediaserver/blob/af89f8a/src/pipeline/Pipeline.cpp), [service.cpp](https://github.com/webosose/g-media-pipeline/blob/df8470b/src/service/service.cpp) | Documented interface; reference behaviour | R3 track rules |
| A4 | Does `paused` arrive during rebuffering? | Yes in the reference pipeline: underrun → PAUSED → `bufferingStart` and `paused`; recovery → `bufferingEnd` and `playing` | [UriPlayer.cpp](https://github.com/webosose/g-media-pipeline/blob/df8470b/src/player/UriPlayer.cpp) | Reference behaviour | R3 intent rule and test |
| A5 | How often does `currentTime` arrive; how soon after `seekDone`? | Every 200 ms, skipped only while seeking, so within about 200 ms of `seekDone`. `setUpdateInterval` exists in LG's pipeline interface | [UriPlayer.cpp](https://github.com/webosose/g-media-pipeline/blob/df8470b/src/player/UriPlayer.cpp), [pipeline.hpp](https://github.com/webosbrew/webos-userland/blob/00a5f87/include/player-factory/pipeline.hpp) | Reference behaviour; LG header | R3 1 s confirmation; R5 |
| A6 | Is there a public signal of visible video? | Yes: the `videoFrame {valid}` subscription event on the URI route; `NDL_ESP_FIRST_FRAME_PRESENTED` on LG's ES route | [uMediaserver.cpp](https://github.com/webosose/umediaserver/blob/af89f8a/src/server/uMediaserver.cpp), [media-common.h](https://github.com/webosbrew/webos-userland/blob/00a5f87/include/ndl-directmedia2/media-common.h) | Documented interface; LG header | R3 `first_frame` |
| A7 | Does DV engage for MKV? | Not on LG's built-in player before webOS 25 by consistent user reports; MP4 and TS do for profiles 5 and 8 | Jellyfin, Plex and MakeMKV forum threads under R7 | User reports | R7 is now a documented gap |

**Still only knowable from our own app on this TV**, because they are properties of Bloob, not of webOS:
- whether LG's closed pipeline sends `trackSelected` for text tracks (A3; the existing `[media] <-` log line shows it, and the design no longer depends on it);
- whether `com.webos.media` accepts `setUpdateInterval` from an app (A5; R5 tries it and ignores a refusal);
- whether `videoFrame` is sent on this firmware (A6; until it is seen, the metric stays `started`);
- Bloob's CPU and frame cost with the HUD hidden (R5);
- the R7 option 2 remux CPU cost, if chosen.

None blocks R1–R4.

## Status

| ID | Planned | Implemented | Locally verified | TV-verified |
| --- | --- | --- | --- | --- |
| R1 | 15 Oct 2026 | No | No | No |
| R2 | 15 Oct 2026 | No | No | No |
| R3 | 15 Oct 2026 | No | No | No |
| R4 | 15 Oct 2026 | No | No | No |
| R5 | 15 Oct 2026 | No | No | No |
| R6 | Outline only | No | No | No |
| R7 | Gap documented; option 1 with R3, options 2–3 by owner decision | No | No | No |
