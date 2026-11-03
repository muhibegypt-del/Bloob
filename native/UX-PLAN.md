# Bloob UX plan: continuity, place and coherent behaviour

GLASS-PLAN.md changed how Bloob looks. This plan changes how it behaves. The owner's verdict after watching a whole series on 2.0.0 was that the glass look is right, but nothing about the app is "dynamic, intuitive or intelligent".

The aim, taken from the Elements of A study, is coherence. **Every component agrees about four things:**
- what you selected;
- what you can do now;
- what has happened;
- where you will return.

Bloob breaks that today in specific, located ways (§1). Each later section is a contract with numbered increments (**X0.1**, **X1.1** and so on). Cite them in commits and tick the status table at the end as they land.

## Evidence labels

Every claim in this file carries one of these labels.

| Label | Meaning | Where to check it |
| --- | --- | --- |
| **[O]** | Observed in Elements of A's shipped client. Static analysis, not a TV recording. | `research/Elements-of-A-UX-learning-frameworks.md`, entries E01–E22 |
| **[S]** | Read in Swiftfin's tvOS source at commit `791fe12`. Where Swiftfin delegates to the Jellyfin server, the server is named. | `research/Swiftfin-tvOS-behaviour-for-Bloob.md` |
| **[P]** | Proposed for Bloob. A design decision, not something either reference proves. | — |
| **[B]** | Bloob today, read in this checkout at 2.0.0. | file:line in this repository |

Some important points are **[P]**, not **[O]**:
- the four-identity model;
- the caught-up decision table;
- the continuation ranking;
- the end-to-end contract.

The study proposed them; it did not recover them. Fallback values recovered from the reference, such as completion bands and animation timings, are examples, not recommended defaults.

## 1. Where Bloob breaks coherence today

| # | Symptom the owner can hit | Cause [B] |
| --- | --- | --- |
| 1 | Each next episode picks its own version, often HD | The player's credits preparation (`player.cpp:403–427`) and episode Play (`details.cpp`, `play_episode`) ask the ranker afresh. Nothing remembers what was chosen for the show. |
| 2 | No way to choose a version for an episode | The How to watch panel is films only (`details.cpp`, `has_versions_panel`). |
| 3 | A finished film can stay in Continue | Watched means position at or past 95% of duration (`tv-app/service/progress.js:48`). Stopping in long credits leaves it unfinished. |
| 4 | Back during an episode's credits makes Play resume the credits | Same 95% rule. `play::title_default` (`play.cpp:38–61`) resumes the latest unfinished episode before considering the next. |
| 5 | Play can be offered for an episode that has not aired | `next_episode()` (`model.cpp:126–136`) ignores `released`, which is parsed (`model.cpp:68`) but never read. |
| 6 | A warning that changes the experience is hidden at the moment of commitment | Direct Play starts the best version. Its How to watch tile only says "Best version" (`details.cpp`, `draw_versions`), and series show no panel. |
| 7 | Continue can't be corrected, and ranks only by recency | `continueWatching()` (`progress.js:77–88`) has one rule (most recent per title) and no dismissal. |
| 8 | Home forgets which row you were on | Down from the hero always enters row 0 (`browse.cpp`, `enter_rows`). Back from any row returns to the hero (`go_top`). Only the Continue row restores its card by ID (`browse.cpp:118–131`). |
| 9 | Progress can be lost | The player posts every 15 s, fire and forget (`player.cpp:386`). The service delays writes by 3 s (`progress.js:10`). There is no acknowledgement, retry or ordering. |
| 10 | Starting playback gives no immediate feedback on OK | No pressed state exists. Feedback waits for the next screen. |

## 2. Contracts

### X0. The interaction runtime (shared, built before the screen contracts)

**Evidence:**
- **[O]** Elements of A's experience is a property of a shared widget runtime, not of individual pages:
  - `SelectableWidget` owns focus, context, press, cancel and geometry;
  - the `mimic` helpers restore your place by item ID with an index fallback;
  - `ListWidget` draws bounded ranges;
  - `ScreenReader` speaks from widget meaning (label, container, role, enabled, position);
  - a central presentation profile gates effects.

  Pages compose these pieces (`research/Elements-of-A-implemented-design-system.md` §1, §5, §9, §13; the study guide §1.1–§2).
- **[B]** Bloob has no such layer. Every screen keeps its own focus integers:
  - `browse.cpp`: `hero_`, `button_`, `row_`, `cols_`;
  - `details.cpp`: `zone_`, `button_`, `version_`, `season_`, `episode_`;
  - `search.cpp`: `krow_`, `kcol_`, `result_`, `recent_index_`;
  - `settings.cpp`: `row_`;
  - `player.cpp`: `zone_`, `control_`, `panel_index_`.

  Each screen also has its own key switch and its own `spoken()` string.

**Why it comes first:** X5, X6, X7, X9 and X12 would otherwise be built once per screen, which recreates the incoherence this plan exists to remove.

**Rules [P]:**
1. **Focusable:** one state machine per element (idle, context-selected, focused, pressed, activated, disabled, loading, failed). It carries a stable item ID, geometry, and meaning (label, role, container, position).
2. **Focus graph as data:** each screen declares its regions and directional links. Initial focus, fallbacks, Back order and modal entry and return live in that table, so the navigation map (X5) is testable without rendering.
3. **Semantic actions:** keys produce actions such as `Play(target)`, `OpenTitle(id)`, `ChooseVersion(id)` and `Dismiss(id)`. One router owns them. This is the `PlayIntentRouter` that CLAUDE.md describes. Key handlers stop calling the API directly.
4. **Request scope:** dwell, generation and cancellation for every purpose (X7).
5. **Anchor:** restore by item ID, then by clamped index, with scroll kept (X6).
6. **Semantics feed speech:** speech reads the same tree, including announcements (X12).

**Increments:**
- **X0.1** Types and a focus-graph engine with table tests.
- **X0.2** Move the title page onto it (the journey slice in §3).
- **X0.3** Home, then Search, Settings and the player.

### X1. Progress: one durable truth

**Evidence:**
- **[O]** Elements of A keeps a user-scoped local checkpoint (`PlayPositionCache`) and a server cursor on the playable's play event. Activity reports carry position, lengths and completion; the response sets the next report interval. Completion comes from session-supplied watched or post-play timing, with a duration-band fallback (`getPlaylistCompletionThreshold`). The tracker treats the `SHOWINGUPNEXT` playback state as done (E08, E09, E12). A failed report is logged and its next timed report is not scheduled (E08). That is a weakness, not a model.
- **[S]** Jellyfin decides completion on the server: under 5% stores no resume point, over 90% or within 1 s of the end is played, and anything shorter than 300 s counts as played. Unknown runtime is assumed played. Swiftfin reports every 5 s and keeps no durable retry queue.

**Rules [P]:**
1. **Separate records.** Content (film or episode ID), edition (the cut or file actually played, see X3), user task (continue, dismissed, watchlist) and playback session (session ID plus a sequence number) are kept apart. Watched, history, watchlist and continue-eligible are separate states, never synonyms.
2. **One completion policy, decided by the service.**
   - When the file or metadata gives semantic timing (a credits start, a post-play offset), use it.
   - Otherwise use one consistent fallback. Bloob's fallback must be chosen and tested; it is not copied from the reference.
   - The player entering its own **Up Next state** (the countdown is showing) is a completion signal. **Preparing** the next episode in the credits is not, because UI visibility and prefetch are not authorities.
   - An unknown runtime never marks a title watched. That is the opposite of Jellyfin.
3. **Durable, ordered checkpoints.**
   - The player sends `{session, seq, position, duration, state}`.
   - The service keeps a checkpoint per session, accepts only increasing `seq` from the current session, and rejects a stale session's late stop.
   - Backward seeks and Start over are valid moves. "Keep the greatest position" is wrong.
   - The service acknowledges each write. The player retries with backoff and sends a final checkpoint on pause, stop, Home, exit and pipeline loss.
   - The service writes through temp file and rename (it already does) and keeps checkpoints over that atomic write. **Measure the loss window on the TV; don't assume it.**
4. **Dismissal is a user state.** Remove from Continue writes `dismissed` with the time and edition. A refresh never reinserts it. A new start of that title clears it.

**Increments:**
- **X1.1** Service schema v2: per-session checkpoints with `seq`, completion state, dismissed state. Migrate v1 entries.
- **X1.2** Player reporting: session ID, sequence, acknowledgements, retry, final flush on every exit path.
- **X1.3** Completion policy module in the service, with unit tests for semantic timing, the fallback, Up Next state, unknown runtime and Start over.
- **X1.4** Remove from Continue: a long press on OK on a Continue card, then a confirmation toast. Spoken as "Remove from Continue, button".

**Done when:** unit tests cover ordering (out-of-order, duplicate, stale session) and completion. On the TV, pulling power mid-film and relaunching resumes within one report interval, and the interval is measured and recorded.

### X2. The next action: one resolver for every surface

**Evidence:**
- **[O]** Elements of A labels continuation contexts (Continue, Next Episode, Next Season, Available Now). Watched state uses the most recent dated play event across an episode's playables (E07).
- **[S]** Swiftfin's series Play is the server's Next Up result, else resume, else the first available episode. Next Up anchors on the furthest played regular episode (Jellyfin server). Player Next and Previous use plain adjacency, a different meaning of "next".

**Rules [P]:**
1. The service resolves a **play target** per title: `{videoId, season, episode, startMs, edition, versionIntent, reason}`. Reason is one of resume, next, start, caught up or unavailable.
   - Home cards, the title page's main button, episode focus, Up next and the Continue row all use it. `play.cpp` becomes a thin client of that decision.
2. **Caught up is a state.** The next episode has a future or missing `released` date: show "Next episode Friday 17", with no Play. An unaired episode is never offered.
3. **Choose one policy deliberately:** the furthest watched episode plus one (Jellyfin's rule, which skips earlier gaps) or the earliest unfinished episode. **Proposed:** furthest plus one, with an explicit way to choose an earlier episode on the title page.
4. **The reason is shown truthfully** on the card and button: "Resume S2 · E5 · 24 min left", "Next: S2 · E6", "New episode", "Caught up · next Friday". Popularity is never labelled as personal.
5. **Continue order [P]:** unfinished tasks, then newly available continuations of shows in progress, then the next episode of shows recently finished. Recency orders within a group.

**Increments:**
- **X2.1** Service `GET /api/target?id=` and `play_target` inside `/api/home`, `/api/meta` and `/api/progress`. Unit tests over air dates, specials, gaps and dismissed titles.
- **X2.2** The app uses the target everywhere. `play::title_default` and `continue_item` read it, and labels come from its reason.
- **X2.3** Caught-up presentation on cards, the title page and Up next.

**Done when** the same title shows the same next action on Home, its page and Up next in every unit-tested state, and no unaired episode can be started.

### X3. Version preference and exact edition identity

**Evidence:**
- **[S]** Swiftfin keeps no version memory. Its autoplay queue carries only bitrate. The Jellyfin server can match an alternate version by name for Next Up.
- **[O]** Elements of A watchlist actions address canonical content, while playback has playable and provider identifiers (E13).

**Rules [P]:**
1. **Version intent** is a preference per show, with a global default: resolution ceiling, HDR policy (Dolby Vision, HDR10 or any), audio language, audio capability (Atmos-capable, no DTS), and "same season pack when possible".
   - It is written when the viewer chooses a version, and inferred from the first choice in a show.
   - It is matched against each new episode's ranked versions. Same pack hash first, then the best version satisfying the intent, then the ranker's best with the reason shown ("Your usual 4K version isn't available for this episode").
2. **Edition identity** is the exact file played (info hash plus file index, or a direct link's file name and size). Resume uses it:
   - same edition: resume at the checkpoint;
   - different edition with a duration within 1%: resume at the checkpoint;
   - otherwise: offer "Resume from about 41:00 (different version)" or Start over. Never silently jump.
3. **Ranker fix:** popularity (seeders) must not outweigh resolution when AllDebrid can serve both. Cached availability matters more than swarm size.

**Increments:**
- **X3.1** Service: a version-intent store per series, plus edition IDs on checkpoints.
- **X3.2** Ranker: an intent-aware pick, seeders capped below the resolution step, and unit tests.
- **X3.3** Up next preparation and episode Play use the intent. The reason line appears when a fallback was needed.

**Done when** watching three episodes in a row keeps the version family chosen for the first, in unit tests over real ranked fixtures, and it is confirmed on the TV.

### X4. Track intent

**Evidence:**
- **[S]** Jellyfin remembers track indices per item when "remember" is on, and otherwise applies language, default and forced rules. Subtitle modes are None, Default, Smart, Always and Only Forced. Swiftfin's queue passes no track overrides to the next episode.
- **[B]** Bloob restores tracks after a reconnect (1.2) and applies preferred languages at start (`player.cpp`).

**Rules [P]:**
- Remember intent, not indices: audio language, role (main, commentary, description), channels and codec preference; subtitle language, forced or full, or an explicit **Off**.
- Off is stored separately from "use defaults".
- Each new file is matched against the intent and validated. A missing match falls back to the defaults, and the menu says so.

- **[O]** In Elements of A, choosing an audio track turns on forced subtitles in that audio's language when no subtitle track is active. AD, CC and SDH have their own labels. Menus order tracks by default flag, UI language, recent preference and type (`research/question-playback-findings.md`). Bloob adopts all three.

**Increments:**
- **X4.1** A track-intent store per series.
- **X4.2** Matching at start and after a reconnect, with unit tests over track tables (forced versus full, missing language, commentary).

### X5. Navigation and Back: one map for every screen

**Evidence:**
- **[O]** In Elements of A, `GroupShelfWidget` routes Up from episodes to seasons and Down back, and episode choice updates the season context. Back closes the nearest interaction layer first. Root Back paths are conditional, not one universal exit rule (E11, E15, E18).

**Rules [P]:** these tables are the contract. Behaviour not listed here is a bug.

**Home, Films and Series.**

| State | Up | Down | Left / Right | OK | Back |
| --- | --- | --- | --- | --- | --- |
| Hero buttons | Tab bar | **The row last used** (today: row 0) | Buttons, then page the hero | Act | Tab bar |
| Row card | Previous row, from the first row to the hero | Next row | Within the row; no wrap | Open the target (X2) | **Up one level:** the row's first card if focus is past it, then the hero |
| Tab bar | — | Into the content, where it was | Tabs | Into the content | Home tab, then "Press Back again" |

**Title page.**

| State | Up | Down | Left / Right | OK | Back |
| --- | --- | --- | --- | --- | --- |
| Buttons | Tab bar or nothing | How to watch (films), seasons or episodes (series) | Buttons | Act | Close the page |
| How to watch / versions | Buttons | — | Tiles | Play this version | Buttons |
| Seasons | Buttons | Episodes, landing on **that season's target episode** | Seasons. The season loads after focus rests 350 ms [S]. | Select the season | Buttons |
| Episodes | Seasons | — | Episodes | Play (X2 target, X3 intent) | Seasons |

**Sources, the player, Search and Settings** keep the orders already specified in `DESIGN.md` (Remote rules). This plan adds two rules:
- Direct-start Cancel returns to the origin (built in G4).
- OK on a track menu row closes the menu **after** the selection is confirmed by the TV, or says that it failed.

**Increments:**
- **X5.1** Home keeps `last_row_` and restores it on Down from the hero. Back from a row goes up one level instead of jumping to the hero.
- **X5.2** The episode and season map above, including Down landing on the season's target episode.
- **X5.3** A desktop tour step for every row of these tables. A failure is a wrong focus after a key.

### X6. Preserving your whole place

**Evidence:**
- **[O]** Elements of A's restoration prefers the previous item ID, then a valid index. Compatible widgets inherit scroll (E11).
- **[S]** Swiftfin refreshes Home on return only if away for more than 60 s, and coalesces change events by 350 ms.

**Rules [P]:**
1. Every focus position is an **anchor**: screen, row ID, item ID, horizontal scroll, and season and episode for series.
2. A refresh replaces the data, then resolves the anchor. If the item still exists, focus stays on it at the same screen offset. If it is gone, focus goes to the neighbour at the same index, clamped.
3. **No reordering under focus.** If a refresh would move the focused card, apply the new order when focus next leaves the row, or keep the card in its slot until then.
4. Return from playback re-reads only the affected rows (Continue, and the title's progress) after the service acknowledges the final checkpoint (X1.2). Nothing else is refetched.

**Increments:**
- **X6.1** An anchor type and an ID-based restore for every row.
- **X6.2** Deferred reorder.
- **X6.3** Targeted refresh after playback.

### X7. Bounded asynchronous work

**Evidence:**
- **[O]** Elements of A's `EpisodePaging` keeps stable placeholders and fetches nearby detail after a 500 ms delay, with bounded retained detail (E06). Image transitions are gated on Ready or Error (E17).
- **[S]** Swiftfin's search debounces 500 ms and cancels. Paging generations reject superseded results.
- **[B]** Bloob already debounces search 450 ms and rejects stale replies (`search.cpp:97–121`). Ranking checks `asked` (`details.cpp:115`). The backdrop dwells 250 ms. Every callback carries an owner token, so closed screens drop late replies.

**Rules [P]:**
1. **Placeholders hold geometry.** A loading card, row or episode has its final size and is focusable. The arrival of data never moves focus.
2. **Fetch on dwell, not on pass.** Season load (350 ms), the backdrop (250 ms), ranking for a focused title (proposed 600 ms) and any future preview (X12) start only after focus rests.
3. **One request per purpose, latest wins.** Each purpose (search, season, ranking, meta) has a generation counter. Older replies are dropped and the server-side work is cancelled where it is expensive (resolve jobs already are).
4. **Dwell and version token [O]:** Elements of A fetches related rows after a 300 ms focus debounce and drops outdated replies by a version token. It loads episode detail about ±10 around the selection after 500 ms, and releases it beyond ±20. Bloob gets a whole season's metadata in one call, so for Bloob this rule applies to images and rankings rather than episode data.
5. **Bounded caches.** Meta has 24 entries over 10 minutes (exists). Rankings cache per video for 10 minutes. Artwork has a 192 MB budget (exists). Prefetching is limited to visible items plus one screen either side.

**Increments:**
- **X7.1** A generation helper in `api.*` used by every screen.
- **X7.2** Focusable placeholders for rows and episodes.
- **X7.3** Dwell-gated ranking prefetch for the focused title, so Play is instant.

### X8. Information hierarchy

**Evidence:**
- **[O]** Elements of A runs recognise, evaluate, act, inspect. `MetadataLine.primary` gives fields priorities and removability. One showcase variant budgets two genres, three description lines and one explanation line. The first valid resolved action is primary, and invalid actions are filtered before presentation (E01–E05).

**Rules [P]:** each surface answers one question. Its budget is fixed before data arrives, and its fields yield in a stated order.

| Surface | Question | Shows | Yields first | Never hides |
| --- | --- | --- | --- | --- |
| Card at rest | What is this? | Art, plus the title in art or as a caption | — | Identity |
| Card focused | Is it for me? | Title, one decision-changing status (progress, New episode, Caught up) | Year, then genre | Status |
| Hero | Should I look closer? | Title art, one meta line (year · up to 2 genres · runtime), 2 lines of description, the reason line, the target action | Runtime, then the second genre | The action and its consequence |
| Title page | What exactly happens if I press Play? | Meta, 3 lines, the primary action labelled from X2, **any warning that changes the experience, next to Play** (X9), alternatives | Description lines | Warnings, the target episode |
| How to watch | Which version, and why? | Headline, plain-words picture and sound, one reason | Size, then seeders | Warnings |
| Release details | Everything technical | Release name, codec, bitrate, size, seeders | — | — |

**Increments:**
- **X8.1** A budget table in `DESIGN.md`. A meta-line builder with a priority order.
- **X8.2** A warning beside Play: the best version's first caution is shown under the button (for example "May play as standard sound"), and is spoken.
- **X8.3** How to watch for episodes (fixes symptom 2), using the X3 intent.

### X9. Component states

**Evidence:**
- **[O]** Elements of A's `SelectableWidget` owns focus, press, visibility and cancellation. Season selectors distinguish context from focus (E14, E15).

**Rules [P]:** every focusable component draws and speaks these states.

| State | Meaning | Visual | Spoken (G7) |
| --- | --- | --- | --- |
| Idle | Available, not targeted | Rest | — |
| Context-selected | Supplies context (the current season, current tab, chosen version) | Candidate fill plus a check | "selected" |
| Focused | Receives the next key | White, lifted | label, role, position |
| Pressed | OK received, work started | **An immediate, reversible press:** a slight scale-in and a spinner in place if work takes over about 150 ms | — |
| Activated | The choice took effect | A persistent result (check, new state) | the new state |
| Disabled | Not available now, with the reason | Disabled label colour, no lift | "dimmed", plus the reason |
| Loading | A placeholder, focusable | Skeleton | "loading" |
| Failed | Data could not load | A retry affordance in place | the error, plus "Try again" |

Animations retarget from their current value (already true of `Anim`). Neighbours never reflow on focus (already true). Activation is cancelled if the target is replaced while work is in flight.

**Increments:**
- **X9.1** Pressed state in `GlassButton` and `CardSpec`.
- **X9.2** Disabled-with-reason and Failed in place.
- **X9.3** Spoken "selected" and "dimmed".

### X10. Player interaction contract

**Evidence:**
- **[S]** Swiftfin suspends auto-hide while paused, scrubbing, in a supplement or a menu. Hide times are 10 s for controls and 2 s for progress only. Back order is system menu, scrub, supplement, controls, exit.
- **[O]** Elements of A distinguishes full, reduced and deck modes. Short seek input previews and debounces.
- **[B]** 2.0 times out at 5 s, or 8 s when paused or on a control. Paused is **not** pinned (`player.cpp`, `hud_pinned`).

**Rules [P]:**
1. **Timers follow Elements of A, not Swiftfin.**
   - **[O]** Ordinary viewing hides after 5 s. Pause, a focused control or an open menu use 8 s; an open deck uses 20 s. Every delay **doubles under the screen reader**. Fades take 300 ms. Dragging and trickplay suppress hiding, and the callback re-checks before it hides. Pause is **not** pinned.
   - **[S]** Swiftfin pins while paused.
   - Bloob 2.0 already matches Elements of A (5 s, 8 s, 300 ms). Add the screen-reader doubling, and suppress hiding while a seek preview is active.
2. A transport key acts on the first press and the controls appear as feedback. This is already true, and differs from [S] on purpose.
3. Seek: the preview moves the knob only. OK commits, Back or Play/Pause cancels the preview, and 400 ms of rest commits.
4. Back order: track menu, seek preview, controls, exit. Already true; keep it.
5. Up next: a cancellable countdown that triggers the completion signal (X1). Choosing Back to stay keeps you in the credits, already marked done.
6. **Seek near the end [O]:** keep clear of the last second, and target 10 s before the end when a seek would pass it.
7. **Binge guard [O]:** after at least two consecutive autoplayed episodes and two hours without any key press, ask "Still watching?" before starting the next.
8. **Subtitle placement [O]:** Elements of A places captions by geometry: the largest obstruction plus a 24-unit gap, clamped to the safe area, recalculated when menus change. LG's call takes levels, so map the bar's actual height to the nearest level instead of fixed 2 and 4.
9. **Lifecycle, a conscious difference:** Elements of A pauses on unfocus and doesn't resume on focus [O]. Bloob leaves playback alone on focus loss (1.1, after the 0.4 quick-menu report). Keep Bloob's rule.

**Increments:**
- **X10.1** Double the timers under Audio Guidance. Suppress hiding during a seek preview. Seek clamp at 10 s before the end.
- **X10.2** The Up next state reports completion.
- **X10.3** Track selection confirmed by the TV before the menu closes.
- **X10.4** Binge guard.
- **X10.5** Subtitle level from bar geometry.

### X11. Recovery without losing the task

**Evidence:**
- **[O]** Elements of A keeps the current image until its replacement is Ready or Error (E17).
- **[S]** Swiftfin tolerates optional metadata failures. It has no link renewal or source fallback.

**Rules [P]:** a failure never loses the title, the selection, the progress or the version intent.

| Failure | Bloob behaviour |
| --- | --- |
| Artwork fails | Keep the previous picture, or the title-coloured placeholder (exists). |
| Metadata late | The page keeps the shelf item's identity and actions. Only the late parts show skeletons. |
| Link expired (AllDebrid URL) | Re-resolve the same version silently at the current position, once. Then say so and offer another version. **Proposed from 1.2.1, not yet built.** |
| Version fails to start | The failure sheet (exists). Next best version in the same intent, then a wider choice. |
| Service unreachable | One plain sentence, Try again, focus kept. |

**Increments:**
- **X11.1** Link renewal at position.
- **X11.2** Intent-aware fallback order in the failure sheet.

### X12. Accessibility and presentation policy

**Evidence:**
- **[O]** Elements of A widgets expose label, role, enabled state, position and count, and containers to an app-owned reader. Motion and automatic presentation are gated by accessibility and capability settings (E19). Previews check preference, focus, dialogs, competing playback and capability (E21).
- **[B]** Bloob has Reduce motion, Reduce transparency, Increase contrast, spoken focus (G7, untested on the TV), and the slideshow paused under Audio Guidance.

**Rules [P]:**
1. Speak containers ("Popular films, row 2 of 6"; "Season 2" for episodes) on change, plus selected and dimmed states (X9).
2. **Announcements [O]:** Elements of A's reader also speaks content changes and delayed hints (the search suggestion count after 1.5 s), and distinguishes speech that interrupts from speech that queues. Bloob adds an `announce(text, interrupt|queue)` call. Uses: search result counts, "Starting 4K version", errors, caught up.
3. Everything is understandable with motion, transparency and previews off. No state is shown only by animation or blur.
4. **Preview contract**, for when trailers or moving art arrive:
   - focus feedback is immediate;
   - the preview starts only after a deliberate dwell;
   - it is cancelled on focus change, a dialog, backgrounding or playback;
   - it respects motion, sound and preview settings and Audio Guidance;
   - the static page carries the same information.

**Increments:**
- **X12.1** Container announcements.
- **X12.2** An audit of each screen with all effects off.
- **X12.3** The preview contract, written into DESIGN.md before any trailer work.

## 3. Order

1. **A short TV smoke pass** (`TV-TEST.md` §8): frost bake time, TTS, lifecycle, the subtitle call. Nothing from 1.1 onward has run on the TV, and these results shape X0 and X10.
2. **X1–X3** fix the episode and version problem, false resumes and unaired episodes (symptoms 1–5). They are service and data work, independent of X0. Also the episode version list (X8.3 plus X3.3).
3. **X0** the interaction runtime.
4. **One journey slice** on X0, built fully coherent before widening. Each step applies X5, X6, X8, X9, X10 and X12:

   > Home card → title page → season → episode → play → seek or tracks → Back → updated Home
5. **The rest:** X4, X7, X11, then X1.4, X2.3, X8.1 and the docs.

## 4. What to measure

These are added to the `[qoe]` and `[perf]` lines. They are measurements, not targets:
- key to visible acknowledgement;
- Play to first frame;
- **version changes after start** (a sign of a mistaken launch);
- failed resumes;
- focus restored to a different item than the one left;
- checkpoints lost across a forced exit.

The study's suggested objective [P] is **time to a confident, successfully completed decision**.

## 5. Deliberately not copied

| Behaviour | Source | Why not |
| --- | --- | --- |
| First Play/Pause press only reveals the controls | [S] | A transport key should act. |
| Immediate autoplay with no countdown | [S] | A cancellable countdown keeps control with the viewer. |
| Unknown runtime treated as watched | [S] (Jellyfin server) | Resolver metadata can be incomplete. |
| Autoplay tied to whether its button is shown | [S] | A preference must not depend on layout. |
| Failed progress report not rescheduled | [O] | Bloob needs durable progress (X1.2). |
| Completion duration bands as Bloob's default | [O] | They are the reference's fallback values, not evidence of the right policy. |

## Status

| Contract | State | Commit |
| --- | --- | --- |
| X0 Interaction runtime | Planned | |
| X1 Progress | **Built, desktop and unit tests (2.1.0); not on the TV.** X1.1 v2 store (sessions, `seq`, retired sessions, dismissal; v1 entries load as they are). X1.2 player sends session, `seq`, state and edition, retries a failed report while open, and flushes on pause, background, stop and end. X1.3 `completion.js`: Up next or end, then credits, then the last 7% bounded to 1–10 min; unknown runtime never completes. X1.4 is a *Remove from Continue* button on the title page, not a long press (no long-press input yet). Retries are in memory only, and the loss window is unmeasured. | `7cbb504`, `b06e8a3` |
| X2 Next action | **Built, desktop and unit tests (2.1.0); not on the TV.** `next.js` returns the target in `/api/meta`. The title page follows it, with the fallback kept for an older service. Continue is tiered: unfinished, *New episode*, next, then caught up with the air day and no Play. Up next, credits preparation and the fallback never offer an unaired episode. Not built: a separate `/api/target` route, and the target inside `/api/home` (Continue uses the same rules from its own entries). | `7cbb504`, `b06e8a3` |
| X3 Version and edition | **Partly built (2.1.0).** The edition (torrent hash, quality, release group, binge group) is kept on checkpoints, and the intent store records a hand-picked version per show. The ranker matches same pack or group (+25), then same resolution and picture (+12), lowers a drop below it (−10), and caps seeders at 6, below one resolution step. The episode versions list is sectioned: matches, other versions, *May not play well on this TV*, with pack or single-episode tags. A version line sits under Play, and *Other versions* is on the starting sheet. Not built: edition-aware resume (rule 2), and the language and Atmos parts of intent. | `7cbb504`, `b06e8a3` |
| X4 Track intent | Planned | |
| X5 Navigation map | Planned | |
| X6 Place | Planned | |
| X7 Bounded async | Planned (search, ranking and owner tokens already in place) | |
| X8 Information hierarchy | Planned | |
| X9 Component states | Planned | |
| X10 Player | Planned | |
| X11 Recovery | Planned | |
| X12 Accessibility | Partly in place (G6, G7) | |
