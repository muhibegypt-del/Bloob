# Claude handoff: verify the review, then plan the implementation

## Assignment and deliverables

The owner wants a strong, independent assessment of the current Bloob native application followed by a concrete implementation plan. Preserve the working native C++/SDL application, LG playback integration, local catalogue/resolver service and finished Glass design.

Start with [the branch review dated 15 October 2026](Claude-branch-review-2026-10-15.md). Treat its findings as claims to verify, not instructions to accept without checking. The reviewed baseline is `8790488` on `claude/sleepy-ptolemy-o87vgv`, application version 2.3.0. Record the full commit you actually inspect and account for any subsequent code changes.

Produce these two documents:

1. **`docs/Claude-review-validation.md`** — adjudicate every finding, explain strengths and any additional important gaps, and record the evidence and its limits.
2. **`native/RELIABILITY-IMPLEMENTATION-PLAN.md`** — a sequenced, implementable plan grounded in that validation. Link it from `native/ENGINE-PLAN.md` and reconcile any misleading status claims there, citing evidence for each correction. Mark proposed work as planned.

**Stop after the validation and plan. Do not implement application fixes, install a package, change TV state or rewrite the renderer under this assignment.** The owner will use the plan to direct the next implementation work.

## 1. Read the architecture before judging it

Read these in order, following specific source references as needed:

1. [CLAUDE.md](../CLAUDE.md), starting with its current review handoff. Older sections describe several generations of the application; distinguish historical statements from the current native implementation.
2. [The independent review](Claude-branch-review-2026-10-15.md), including its verification boundary and strengths.
3. [Native application map](../native/APP-MAP.md) — ownership, layers and existing increment IDs.
4. [Buna engine specification](Buna-engine-specification.md) — the target architecture, rather than evidence that every feature exists today.
5. [Current engine plan](../native/ENGINE-PLAN.md), [UX plan](../native/UX-PLAN.md), [quality assessment](../native/QUALITY.md) and [design tokens](../native/TOKENS.md).
6. Relevant implementation and tests. Use [native README](../native/README.md), [TV test guidance](../native/TV-TEST.md) and build scripts to understand available verification routes.

Check the current diff and recent commits first. Do not review an old snapshot if a finding has already been fixed. If a referenced specification or dependency is absent, record that limitation rather than assuming its contents.

## 2. Validate every finding explicitly

Make one table row for each F01–F10. Include:

- Finding ID, current severity and user-visible consequence.
- Verdict: **confirmed**, **partly confirmed**, **rejected**, **already fixed**, or **needs TV evidence**.
- Current source path, type/function and verified line numbers.
- The complete causal chain: trigger, state/ownership failure and observable result.
- Evidence type: source analysis, isolated probe, existing test, package inspection or real TV observation.
- Reproduction/result, or the precise uncertainty preventing a conclusion.
- Correction to the review, if necessary, and the intended invariant for a fix.

Do not treat passing existing tests as proof that a finding is false. Conversely, do not turn a plausible race into a claim that it happens frequently on the TV. Read the surrounding callers, cleanup paths and tests before deciding.

### Targeted checks

| ID | What to trace or reproduce | Required distinction |
| --- | --- | --- |
| F01 | Follow final progress from `PlayerScreen::save_progress/send_progress`, through task ownership, HTTP acknowledgment, the service's delayed disk write and `App::shutdown`/service stop. Check both player destruction and app exit. | A queued request, an in-memory update and a durable disk acknowledgment are different milestones. Screen-owned cancellation and premature service exit are separate failure paths. |
| F02 | Inspect `account-store.js` `writeVerifiedFile`/`createFileStore.write` and `progress.js` `saveSettings`. Run sequential and concurrent writes against isolated storage. Inspect returned success/failure as well as resulting file contents. | A shared process-specific temporary filename can race; a file existing afterward does not prove every acknowledged write succeeded. Check failure propagation through the API. |
| F03 | Drive the pure `Session` with Playing → BufferingStart → Paused → BufferingEnd, and user Pause → late Playing. Trace reload and foreground handling. | User intent, backend observation and automatic lifecycle actions need distinct ownership. Reopening a session must not silently replace paused intent. |
| F04 | Request a seek, then deliver SeekDone without a confirmed position; inspect session time, seek metrics, displayed preview and saved progress. | Requested/preview time is not confirmed media time. A timestamp-free completion can finish an operation without proving the exact landing point. |
| F05 | Trace audio/subtitle selection through UI, `Session`, `media.cpp` and backend replies. Inject rejection, missing acknowledgment and an absent preferred track. | Pending selection, confirmed selection and persisted language/role preference are separate. Establish which acknowledgments the real LG API actually supplies. |
| F06 | Prepare next playback for source A, then select source B with the same displayed quality headline. Inspect `Ctx::Prepared` and `SourcesScreen::start_job`. | Display text is not source identity. Carry candidate/file/episode identity and relevant preference revision through preparation and launch. |
| F07 | While one source screen remains alive, start resolve A, show other versions, start B, then deliver A's delayed response. Trace `start_job/on_job/poll/show_others`. | Screen lifetime alone does not invalidate a superseded operation. Do not claim a destroyed screen can reopen unless separately demonstrated. |
| F08 | Compare normal bus-based service startup with fallback startup in `main.cpp`. Check settings, feature flags, recent searches and account initialization. | Both paths must reach equivalent initialized application state, including failure handling. |
| F09 | Compare service completion/resume policy with native `play::resume_point` and `continue_item`. Include a four-hour film at minute 229 and all playback entry points. | Service completion uses remaining-time bounds; the older native percentage rule can disagree. Identify the authoritative policy and any offline fallback. |
| F10 | Trace event wakeups, dirty flags, hidden player overlays and render submission. Check media-clock callbacks separately from visible changes. | Waking to consume events is not the same as invalidating the visible scene. Do not claim a measured CPU/frame-rate benefit from source inspection alone. |

Use temporary directories for service probes, with **all** account-store fallback locations redirected into the test directory. Do not read or modify a real pairing key, account, progress database or settings store. Use fake media events and controlled deferred responses for logic tests; real content and AllDebrid requests are unnecessary for these checks.

The earlier probes lived in a machine-local temporary directory and are not a portable test suite. Recreate minimal, self-contained probes where needed. A host compatibility shim is acceptable for pure C++ logic if its scope is stated. Do not spend the review installing an operating system or rebuilding unavailable platform infrastructure.

## 3. Preserve the verified strengths

The report identifies real architectural value: pure playback decisions, an app-owned native UI, separate service responsibilities, image workers and caches, transparent video-plane composition, direct-play flows, source ranking, continuity policy and existing performance logging. Verify these strengths and preserve their useful contracts.

Do not assume a render thread, retained scene graph, MPV integration or another media backend is a prerequisite for correcting persistence and session logic. Justify larger architectural changes with a concrete requirement that the current approach cannot satisfy.

Keep the current aesthetic and token system. This assignment concerns behaviour, ownership, correctness and efficiency. It does not authorize a new visual design.

## 4. Assess the remaining engine gaps

After F01–F10, assess the report's additional gaps without inflating them into confirmed failures:

- Total resource accounting: GPU images versus CPU decode buffers, queued uploads, glyphs, shapes and in-flight work; upload pacing and queue bounds.
- Retired Luna callback records: explain the current use-after-free protection before proposing safe reclamation and a bound.
- Focus/layout contracts across screens, stable restoration and interruptions.
- Startup catalogue availability, cache persistence and row-by-row readiness.
- Text shaping, bidirectional text, fallback and meaningful accessibility semantics.
- Metrics provenance: key-to-`SDL_RenderPresent` return is a submission proxy; Playing/positive media time is not proof of visible first frame.
- Native packaging, documented ABI limits and unresolved platform permissions. Rooted-TV operation is not proof of ordinary-device permissions.
- Source expiry/recovery and the current URI media path. A sample-stream path is an additional capability, not an established remedy for every format issue.

Full→Lite effects adaptation already exists in the main render loop. Review its actual rule and limitations instead of reporting it as absent. Likewise, input-to-present proxy logging exists; distinguish it from physical presentation timing.

## 5. Build a plan an engineer can implement

Use the validated findings to choose the final sequence. A reasonable starting order is:

1. Durable progress/settings, shutdown handshakes and one resume/completion policy: F01, F02, F09.
2. Playback intent and acknowledged seek/track transactions: F03, F04, F05.
3. Source identity, watch-attempt ownership and stale-operation rejection: F06, F07.
4. Shared service-ready initialization: F08.
5. Truthful inspector/metrics, visible invalidation and bounded resources: F10 and validated resource gaps.
6. Shared focus/layout, text and accessibility improvements, ranked by actual user impact.
7. Additional media backend work only for a specific remaining capability gap.

For **every implementation increment**, specify:

| Field | Required content |
| --- | --- |
| Identity and purpose | Stable ID, affected F findings, APP-MAP/ENGINE-PLAN links, user symptom and intended invariant. |
| Scope | Exact files, types/functions and contracts to change; dependencies on earlier increments. |
| State and operations | States, events, operation IDs/generations, acknowledgment rules, cancellation and timeout behaviour. |
| Ownership | Who owns each request, callback, durable write and final report; what survives screen destruction or app shutdown. |
| Service/data changes | Endpoint/schema changes, durable success/failure semantics, compatibility and migration for existing stored data. Say explicitly when no schema change is needed. |
| Recovery | Retry boundaries, stale-result handling, user-facing failure behaviour and safe fallback. |
| Verification | Smallest meaningful regression scenario, injected failure/order variation, observable pass criteria and any genuinely TV-only question. |
| Delivery | Implementation order, build/package implications and a reversible rollout approach for later authorized deployment. |
| Completion | Objective definition of done; separate implemented, locally verified and TV-verified status. |

Be concrete about a proposed durable-flush acknowledgment, serialized writer, intent ownership, confirmed media clock, pending track selection and resolve generations. These are design proposals to justify against current code, not claims that the reference runtime exposes a particular API. Do not invent private LG methods or acknowledgment payloads.

Prefer a small number of coherent increments over a sprawling rewrite. Reuse existing layer/increment numbering where it fits; explain any new sequence. Explain why the first increment should ship before the second. Identify decisions that can be made from the source and isolate the few that actually require hardware evidence.

## 6. Verification boundary and stopping point

The previous review ran 57 service tests across five files and 15 pure native test routines successfully, plus focused persistence/session probes and IPK structure checks. Those results concern the reviewed baseline and are described in the report; they are not evidence that a future change works.

No TV was contacted during that review, and no current native UI was visually inspected. Packaged service code is minified, so a raw source-hash mismatch does not establish a stale package. Do not claim the existing IPK exactly matches HEAD without build provenance.

One suspected task-completion wake bug was investigated but **not reproduced**: both trivial and nontrivial callback captures still woke correctly. Do not include it as a confirmed finding without new evidence.

Run targeted checks only where they settle a concrete question. The owner does not want repeated broad recording/testing exercises. Any remaining hardware-dependent questions belong in a short, explicit section of the plan; they do not justify blocking source-level planning.

Return a plain-English summary: what is already strong, what the validation changed, the first implementation increment and why, the rest of the sequence, and the remaining unknowns. Finish the two documents before suggesting implementation. Do not label planned changes as completed.

## Copyable instruction

> Read `docs/Claude-review-and-planning-handoff.md` and follow it. Independently verify F01–F10 in `docs/Claude-branch-review-2026-10-15.md` against the current branch. Write `docs/Claude-review-validation.md`, then `native/RELIABILITY-IMPLEMENTATION-PLAN.md` with exact ownership, state, API/data, dependency and acceptance details for each increment. Reconcile the existing engine plan with verified evidence. Preserve the working native architecture and Glass design. Stop after review and planning; do not implement or deploy application changes yet.
