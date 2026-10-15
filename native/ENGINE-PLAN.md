# Buna engine upgrade: from today's native app to the specification

**Target:** `docs/Buna-engine-specification.md` (from `bloob-v1` `73da849`), the finished-system spec.
**Baseline:** Bloob Native 2.3.0 (`native/src`, about 9,000 lines of C++), 15 October 2026.
**Reliability first:** [`RELIABILITY-IMPLEMENTATION-PLAN.md`](RELIABILITY-IMPLEMENTATION-PLAN.md) (planned, 15 October 2026) comes before the phases below. It fixes the validated findings in [`docs/Claude-review-validation.md`](../docs/Claude-review-validation.md) and takes over the parts of E1–E3 they need. The marks in §1 were corrected on the same date against that validation; each correction cites its finding.
**Evidence:** on the TV, only 2.2.0 has run (user report: "extremely smooth", with four quirks). Everything else is desktop builds and tests. Where this file says something "works", it means in the code and on the desktop, unless it says "TV".

## 1. Where we stand, section by section

Each section is marked **Have** (meets the spec's contract), **Partial**, or **Gap**.

| Spec | Contract | Bloob today | Mark |
| --- | --- | --- | --- |
| §3 Host | Native host, launch milestones, platform adapter | SDL2 window, luna bus, uMediaServer, exported video window. `[launch]` marks process, surface, Home data and first Home frame. The service is started over the bus, else with Node directly. | **Have** (SDL instead of raw Wayland/EGL) |
| §4.1 Threads | UI owner; render owner; bounded workers | One UI thread does state, layout and rendering. API and image work run on bounded worker pools (`tasks.*`). | **Partial**: no separate render owner |
| §4.2 Envelopes | Owner, generation and operation on every completion | Completions are dropped when the owning screen's `alive_` token expires. uMediaServer loads carry a generation. There is no per-operation ID. | **Partial** |
| §4.3 Queues | Reserved capacity, coalescing, overflow counters | SDL queue plus worker queues. Clock events are not coalesced. There are no overflow counters. | **Gap** (low risk today) |
| §4.4 Clocks | Monotonic, media and wall clocks kept apart | Monotonic `SDL_GetTicks` for animation; the media clock comes from uMediaServer; wall clock for *Ends at*. Media epochs are not modelled. | **Partial** |
| §5 Frames | On-demand frames, no idle repaint, damage | Draws only when dirty, animating or a redraw is due; idle sleeps up to 1 s. Every frame is a full 1920×1080 redraw, with no damage tracking. | **Partial**: idle sleeps, but every bus or worker wake forces a full redraw even when nothing visible changed (validation F10; planned R5) |
| §6 Scene | Retained scene, immutable snapshots, layout model | Immediate-mode drawing per screen. Layout is hand-placed numbers, with a measure-once button since 2.2. | **Gap**: the largest structural gap |
| §6.4 Video | One place converts rectangles; underlay keeps transparency | Exported window under the UI. One `place_video`; transparent canvas over video. | **Have** |
| §7 Images and text | Keyed requests, priorities, budgets, readiness | Keyed decode on workers, a 192 MB GPU budget with eviction, fade-in, prefetch. Text: stb_truetype atlases, glyphs on demand. No shaping or bidi. | **Partial** |
| §8 Lists | Indexed offsets, bounded realization, data windows | Shelves draw only the visible range. A season's episodes all load at once; the ±10 window and 500 ms delay are not implemented. Refresh reconciles by ID only on Continue. | **Partial** |
| §9 Input and focus | One owner per action, layered focus, deck states, Back table | One key owner (top screen); each screen has its own focus code. Back mostly follows the spec's table. Magic Remote clicks are mapped as of 2.2.1. There is no shared focus engine or deck lifecycle states. | **Partial** (UX-PLAN X0 covers this) |
| §10 Motion and policy | Retargeting, tiers, reduce motion and transparency | Timed curves and springs (2.3). Full and Lite tiers, Reduce motion, Reduce transparency, Increase contrast. | **Have** |
| §11 Session | State axes, revisioned snapshots, clock rules | One `Phase` enum mixes transport, operations and failure. User intent is kept apart. Seek preview, pending and landing are kept apart. The clock interpolates and stops when paused or seeking. There are no snapshot revisions or capability reasons. Corrected: backend `Playing`/`Paused` overwrite user intent and reopening resets it (F03); a timestamp-free `seekDone` is taken as the landing (F04). | **Partial** (planned R3) |
| §12 Adapters | URI route; segmented/sample route; output truth | URI route through uMediaServer only. Output truth: the release claim and the TV's `hdrType` are compared in the log (2.2.1), but not shown with provenance. No sample route. | **Partial** |
| §13 Seek, tracks, captions | Seek transaction, acknowledged tracks, preferences | Seek commits after 400 ms and waits for `seekDone`, but `seekDone` carries no position and is counted as a landing with zero error (F04). Audio reads `trackSelected` flags; subtitle selection is set before any acknowledgement and failures are only logged (F05). Tracks are restored after reconnects. Preferred languages apply. Captions are drawn by the TV with obstruction lift (U7). Chapters and skips need data we don't have. | **Partial** (was Have; planned R3) |
| §14 Continuity | Watch-attempt ID, revisioned progress, lease renewal | Progress v2 with session and sequence (X1), next action (X2), version intent (X3). No watch-attempt ID across resolve and play. When a link expires there is no automatic re-resolve, only "choose another". Corrected: the final report can be lost on exit and the service writes 3 s late (F01); concurrent writes collide (F02); prepared reuse matches the headline, not the source (F06); resolves have no generation (F07); native resume disagrees with the service (F09). | **Partial** (planned R1, R4) |
| §15 Lifecycle | Focus vs visibility, intent generations, idempotent cleanup | 1.1 lifecycle: focus loss leaves playback alone; hide pauses and saves; pipeline loss reopens, three times a minute at most. Not yet tested on the TV. There is no explicit generation for a user pause during an interruption. | **Partial** |
| §16 Accessibility | Semantic tree, spoken focus, reduced effects | Each screen's `spoken()` string goes to LG TTS (G7, unverified). No semantic tree, no container relationships. | **Partial** |
| §17 Errors | Structured errors, bounded recovery | Player errors offer Try again, Another version or Exit. Resolve failures keep context. Errors are strings, not structured. | **Partial** |
| §18 Inspector | Local read interface, causal traces, metrics | `/tmp/bloob-native.log` with `[perf]`, `[qoe]` and `[launch]` lines. A Diagnostics screen. A key-to-submit proxy exists (ends when `SDL_RenderPresent` returns, `perf.h`), not physical presentation. No read interface or trace correlation; "first frame" is really Playing (A2). | **Partial** (was Gap; planned R5) |
| §19 Packaging | Versioned set, ABI guard, atomic writes | One IPK with app and service. `build.sh` rejects GLIBCXX imports; the GLIBC maximum is not enforced (today's binary needs only GLIBC_2.4, by observation, A3). `requiredACG` is unset. Service writes are atomic one at a time but collide when concurrent (F02). | **Partial** (was Have) |

## 2. What the spec gets right for us, and where to be careful

**Highest value now:**
- **§11 state axes and §12.4 output truth.** These map directly onto the TV reports: DV shown as HDR, and tracks that seemed missing. A session that holds *declared*, *detected* and *observed* separately would have made both bugs obvious.
- **§18 inspector.** We are blind on the TV except for a log file, and every TV issue so far needed guesswork. A read-only state snapshot is the cheapest way to stop guessing.
- **§14.2 watch attempts and §14.4 lease renewal.** Late replies and expired links are where users lose their place.

**Treat with caution:**
- **§6 retained scene, immutable snapshots and a separate render thread.** This is a rewrite of every screen. The owner reports 2.2.0 as extremely smooth on the TV, so there is no measured problem it would fix today. Adopt it in two steps:
  - first a layout layer (UX-PLAN X0) on the current immediate-mode renderer;
  - a retained scene only if TV measurements show frame-time or damage problems.
- **§12 segmented/sample route (LG ES).** Its own §1 and our CLAUDE.md both say "only for a concrete, measured gap". One candidate gap exists: whether uMediaServer engages Dolby Vision for MKV. The 2.2.1 `picture:` log line answers that. "Not engaged" alone is a hypothesis, not a justification: the sample route is justified only if a diagnosed gap is shown to be closable by it through public LG interfaces (reliability plan R7).

**Housekeeping:** the spec links `docs/Buna-native-runtime-target.md`, `native-host/README.md` and `research/question-operations-findings.md`. Those live on `bloob-v1`, not on this branch, so the links resolve there only.

## 3. Upgrade phases

The order follows spec §21.2, adjusted for what is already built and for TV evidence. **All phases are planned, and all come after R1–R5 of the reliability plan.** E1's intent axis, confirmed clock and track operations move to R3; E2's snapshot to R5; E3's watch-attempt ID to R4. E3's lease renewal and structured errors stay here. Each phase ships as its own build and is checked on the TV before the next starts.

| Phase | Spec | Work | Done when |
| --- | --- | --- | --- |
| **E1 Session truth** | §11, §12.4, §13.3 | Split `Phase` into axes: lifetime, user intent, backend transport, active operations, failure. Add a revisioned snapshot. Record each capability with a reason (supported, unsupported, unknown). Keep output truth with provenance (declared from the release name, detected from `sourceInfo`, observed from `videoInfo`) and show it on the player. Make track selection an operation with an acknowledgement and a failure. | Unit tests over the axes. On the TV, the player shows "Dolby Vision (TV reports HDR10)" when they differ, and the track menu says why a choice failed. |
| **E2 Inspector** | §18 | A read-only state file written on change: build, page and focus stack, session snapshot, capabilities, tracks, resource counts, last errors. No URLs or keys. `scripts/tv-inspect.sh` reads it over SSH, read-only. Input-to-present timing recorded per key. | One command shows the TV's current Bloob state, and key-to-frame p50/p95 appear in `[perf]`. |
| **E3 Watch attempts and leases** | §14.2, §14.4, §17 | One watch-attempt ID from Play to first frame; late replies are dropped by ID. An expired or dead link re-resolves the same version at the confirmed position, bounded and per failure category. Errors become structured (domain, operation, retryable, actions). | Desktop fault injection: an expired link resumes at the same second, and a stale resolve can't open a player. |
| **E4 Events and frames** | §4.2–4.4, §5 | Event envelopes with owner, generation and operation. Clock events coalesced. Overflow counters. Media epochs on seek and reload. Damage rectangles for the common case (focus moves on Home). | The inspector shows queue depths. Idle CPU is unchanged; a focus move redraws only damaged areas. |
| **E5 Layout and focus runtime** | §6.2, §8, §9 (UX-PLAN X0) | Shared layout primitives (row, column, inset, overlay, virtual list), a focus engine (layers, return anchors, horizontal anchor between rows), page deck states, the episode data window (±10, 500 ms). | One journey (Home → title → episode → play → Back) runs on it, with restoration identical after artwork eviction. |
| **E6 Resources and text** | §7 | Byte budgets per class in a device profile, in-flight admission, decode reservations, upload limits per frame. Text shaping and fallback through one service. | Repeated journeys show flat memory in the inspector. |
| **E7 Sample route** (only if E1/E2 prove a gap) | §12.1–12.3 | A segmented and ES adapter behind the same session contract: fetch, demux, bounded queues with backpressure, epochs. | The diagnosed gap is closed on the TV. Dolby Vision for MKV is a candidate gap, not an established one. |
| **E8 Accessibility and completion** | §16, §21.3 | A semantic tree next to the scene, ordered speech, and the completion checklist in §21.3. | The acceptance journey works with Audio Guidance and reduced effects. |

**A retained scene and a separate render thread are not scheduled.** They are reconsidered only if E2's measurements on the TV show a frame-time problem.

## 4. What this file promises

It promises a plan and a gap list, not a finished engine. Update the marks in §1 as each phase lands, and record TV evidence separately from desktop evidence.
