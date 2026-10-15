# Elements of A: UI/UX engineer study guide

## Purpose

This guide identifies what an engineer should study inside the reference application to understand and independently recreate its UI/UX feel.

The central task is to understand **how components acknowledge intent, preserve context, schedule work and return control to the user**. Shared interaction behaviour, individual pages and runtime services must work together.

For Bloob, this means extracting useful component and interaction contracts for its own C++ UI and Buna runtime. Bloob can retain its existing visual identity and use its own local catalogue, version resolver and progress server.

## Evidence and reading conventions

The underlying REA studied the saved LG webOS update, build `16.4.4.J34L139`, through static analysis and direct inspection of shipped JavaScript, with a separate native-runtime study.

* **Observed mechanisms** are documented in the linked research and its file/function evidence ledger.
* **Questions and worksheets** below describe what an engineer should extract; they are not claims that every component implements every listed state.
* **Proposed contracts** are original recommendations for Bloob.
* The saved update's active installation and feature selection were not observed during that study. Present code paths are not proof that every path was enabled on the TV.
* Exact server recommendation ranking and cross-device conflict resolution remain partially unknown. These are separate from the substantial client UI/UX logic available to study.

Start with the [five-framework UX analysis](Elements-of-A-UX-learning-frameworks.md) and its [evidence ledger](Elements-of-A-UX-learning-frameworks.md#evidence-ledger-and-replication-boundaries).

## 1. Study map, in priority order

| Priority | Area | Where in the application | What the engineer needs to understand |
|---|---|---|---|
| 1 | Shared component behaviour | `app.js`: `SelectableWidget`, shared card styles and metadata components | Focus, contextual selection, press/release, cancellation, disabled states, scale, shadows and component-specific overrides. These establish consistency across screens. |
| 2 | Navigation and restoration | `app.js`: root navigation, `mimic` and selection-restoration helpers | Focus ownership, overlays, Back handling and preservation of selection/scroll when content is replaced or refreshed. |
| 3 | Title-page composition | `ProductPage.js`: `CanonicalLockup`, `ProductPageInner.animateFold` | Primary-action hierarchy, metadata disclosure and the coordinated transition from overview to episodes/supporting information. |
| 4 | Seasons and episodes | `ProductPage.js`: `GroupShelfWidget`, `EpisodePaging` | Directional movement between rails, season/episode context, stable placeholders, nearby loading and retained scroll. |
| 5 | Browsing and search | `SuperBrowsePage.js`, `SearchPageFactory.js` | Filter/menu ownership, focus-triggered loading, delayed requests, cancellation, stale-response rejection and keyboard/results navigation. |
| 6 | Artwork and motion | `ProductPage.js`: `BackgroundWidget`; `app.js`: artwork and presentation policies | Holding existing artwork while replacements load, fallback behaviour, interruptible transitions, contrast protection and reduced-motion rules. |
| 7 | Player interaction | `PlayerPage.js`: `UnifiedPlayerWidget`, `TransportBar`, `PlayerUiState` | Overlay states, interaction-aware timers, remote controls, seek preview versus commitment, thumbnails, menus, captions and Back. |
| 8 | Continuity and playback state | `PlayerStore.js`; `app.js`: progress cache and invalidation events | Authoritative playback state, checkpoints, completion, next-item preparation and updating the browsing experience after playback. |

### 1.1 Shared components: recover the common interaction language

Study `SelectableWidget` before trying to reproduce individual pages. Its focus, press, visibility, geometry and cancellation mechanisms explain how multiple component families share an interaction language.

Extract:

* Which state receives the next remote command.
* How context-selected differs from actively focused.
* When press acknowledgement begins, commits or cancels.
* Which visual properties change, and whether logical geometry changes with them.
* How a disabled or replaced component prevents false activation.
* Which defaults are shared and which are overridden by a particular component.

**Proposed Bloob output:** one reusable state contract for cards, buttons, season selectors and menu items, with explicit component overrides.

Do not turn one shared timing value into a universal prescription. The reference itself has component-specific overrides. An animation duration is also not a required input delay or a measured remote-to-frame latency.

Evidence: the framework report's [focus section](Elements-of-A-UX-learning-frameworks.md#3-focus--spatial-ergonomics), especially ledger entries E14 and E15.

### 1.2 Navigation: study who owns the user's place

Trace the root page/overlay system and the compatible-state restoration helpers.

Extract:

* Which layer owns focus while a menu or modal is open.
* Whether the underlying page remains alive, idle or replaceable.
* The order in which Back dismisses interaction layers.
* How a selected item is matched after refresh.
* How index and scroll fallbacks work when the item disappears.
* When returning to the top or opening navigation is different from exiting.

**Proposed Bloob output:** a navigation contract and return-state record containing screen, section, item identity, scroll anchor and relevant season/episode context.

Evidence: ledger entries E11 and E18; [detailed navigation findings](question-ui-findings.md#navigation-and-focus-questions).

### 1.3 Title page: recover the attention hierarchy

Study `CanonicalLockup` and `ProductPageInner.animateFold` together.

Extract:

* How the principal valid action is resolved and presented.
* Which identity, progress and availability information remains visible.
* Which metadata is condensed or deferred.
* How movement toward episodes coordinates title/overview prominence, shelf placement and navigation targets.
* How alternative providers or versions remain discoverable.

**Proposed Bloob output:** an overview-to-episodes state model, with one coherent transition and explicit information priorities.

A single page-state variable is not sufficient evidence of equivalent behaviour. The important contract is the coordinated result across all affected components.

Evidence: framework report section 1 and ledger entries E01–E05.

### 1.4 Seasons and episodes: separate context, focus and data availability

Study `GroupShelfWidget` and `EpisodePaging`.

Extract:

* Up/Down relationships between season and episode rails.
* How an episode identifies its associated season.
* Which selection is contextual and which actively receives input.
* When a season change repositions episodes.
* What is inherited on content replacement.
* How episode positions remain stable before full details arrive.
* When nearby detail is fetched and distant detail can be released.

**Proposed Bloob output:** a season/episode focus graph plus a bounded loading policy. Navigation should remain meaningful while metadata is incomplete.

Evidence: ledger entries E06 and E15; [season-selector and episode design](Elements-of-A-implemented-design-system.md#6-season-selectors-and-episode-rails).

### 1.5 Browsing and search: study work scheduling as UX

Trace the page, filter/menu, shelf and search-state owners.

Extract:

* Immediate feedback versus work scheduled after focus settles.
* Which requests begin on focus and which require activation.
* How an obsolete request is cancelled or its late result rejected.
* How cached content, placeholders, empty results and errors are represented.
* How filters and result updates preserve location.
* How focus moves between query input, keyboard, hints and results.

**Proposed Bloob output:** a request-lifecycle contract, with stable IDs and cancellation/generation rules alongside screen states.

Evidence: [UI loading and focus findings](question-ui-findings.md#loading-the-next-screen-on-focus-after-a-pause-or-only-on-selection) and [search composition](Elements-of-A-implemented-design-system.md#search).

### 1.6 Artwork and motion: study continuity, interruption and completion

Study `BackgroundWidget`, artwork colour/theme inputs and presentation policy.

Extract:

* When incoming artwork becomes eligible to replace the current layer.
* What happens on image failure.
* Which work runs during a transition and stops afterwards.
* How rapid changes retarget or cancel pending work.
* How foreground legibility is protected over different images.
* How motion, transparency and capability settings change presentation.

**Proposed Bloob output:** a finite artwork-transition state machine and a central presentation policy.

Keep observed effects distinct: the LG study establishes scale/shadow cues and scroll-linked stage movement. It does not establish Siri Remote-style 3D card tilt. Its artwork palette mechanism is not proof of live video-pixel sampling.

Evidence: ledger entries E16, E17 and E19; [artwork and motion findings](Elements-of-A-UX-learning-frameworks.md#33-what-parallax-and-ambient-integration-actually-mean-here).

### 1.7 Player: recover the complete interaction contract

Study `UnifiedPlayerWidget`, `TransportBar` and `PlayerUiState`, then follow their semantic actions into the playback session.

Extract:

* Full, Reduced and Deck presentations and their entry/exit conditions.
* Visibility timing under ordinary viewing, pause, menus, accessibility and scrubbing.
* Remote commands in each state, including Back and cancellation.
* Actual media time, requested seek, drag preview and thumbnail time as distinct values.
* How seeks commit, accumulate, acknowledge or fail.
* How audio/subtitle menus reflect backend capabilities and available tracks.
* How captions avoid obstruction by controls and thumbnails.

**Proposed Bloob output:** a player state/remote table, with authoritative decoder state beneath the control surface.

Evidence: [player design](Elements-of-A-implemented-design-system.md#12-player-design) and [playback findings](question-playback-findings.md).

### 1.8 Continuity: study how playback changes the rest of the app

Trace `PlayerStore`, the progress cache, reporting/completion helpers and shelf invalidation events.

Extract:

* The identity associated with a checkpoint and how user identity is handled.
* Which clock and duration are authoritative.
* How completion is derived, including session metadata and fallback policy.
* Which events cause details, continuation and library surfaces to refresh.
* How refresh preserves the user's current place.
* What is established about failure handling, and what remains unknown.

**Proposed Bloob output:** a canonical progress/next-action contract with content, edition, user-task and session identities distinguished.

The four-identity design and proposed Bloob ranking are recommendations in the framework report, not recovered specifications of the reference's server.

Evidence: [continuity engineering](Elements-of-A-UX-learning-frameworks.md#2-the-up-next-mental-model--continuity-engineering), especially ledger entries E07–E13.

## 2. Study the runtime contracts underneath the widgets

The shipped JavaScript depends on native services for input, drawing, composition, image handling and frame callbacks.

Trace this complete interaction path:

**Remote input → focused widget → semantic action → state change → invalidation → rendered feedback.**

For each step, establish:

| Runtime responsibility | Questions to answer |
|---|---|
| Input delivery | Who receives a key? How are press, release, repeat and cancellation represented? |
| Focus | Which object owns authoritative focus, and when does it change? |
| State propagation | Which components react to the semantic action? Which remain unchanged? |
| Invalidation/composition | What schedules another update or redraw? When does that work stop? |
| Image lifecycle | How do loading, ready, error and disposal become visible to widgets? |
| Frame callbacks | Which transitions need frames, and how do they unsubscribe on completion or replacement? |
| Media session | Which acknowledgements/events update the UI's playback clock, buffering and available controls? |
| Accessibility | How do labels, roles, enabled/selected state and container relationships follow the same interaction state? |

**Proposed Buna objective:** provide these services predictably within Bloob's own architecture. The study does not make the reference's application-owned runtime ABI a public webOS API or an automatically reusable dependency.

Read the [Luna runtime architecture study](Luna-runtime-architecture.md) and [reference system map](Elements-of-A-system-map.md) for this boundary.

## 3. Required engineering outputs

### A. Component behaviour table

Create this table for each important card, shelf, selector, menu and player control:

| Input or event | Current state | Immediate feedback | Next state | Background work | Cancellation / return rule |
|---|---|---|---|---|---|
| To be specified | To be specified | To be specified | To be specified | To be specified | To be specified |

Keep immediate acknowledgement separate from eventual network/media completion. Record observed reference behaviour separately from Bloob's chosen rule.

### B. Screen focus graph

For each screen, define:

* Initial focus and fallback.
* Directional links between regions.
* Header versus content focus ownership.
* Modal/menu focus entry and return.
* Back/exit hierarchy.
* Selected-ID, index and scroll restoration.
* Behaviour when loading completes, content changes or the selected item disappears.

### C. Motion and presentation policy

Define component defaults and overrides, interruption/retargeting, focused-growth bounds, image fallback and reduced-motion/transparency variants. Preserve a usable static presentation.

### D. Loading and recovery policy

Define stable placeholders, request triggers, bounded nearby work, cancellation, stale-result rejection, empty/error states and retry. Retain task context through a failure.

### E. Return-state and progress policy

Define content/edition/session identity, accepted position, completion, next action and return anchors. For Bloob, explicitly specify durable checkpoints, acknowledgement and stale-session rejection rather than assuming the reference provides a complete reusable server policy.

## 4. The most revealing journey to study

Follow one complete path:

**Home card → title details → season → episode → playback → seeking/track menu → Back → updated Home.**

| Step | What to record |
|---|---|
| Home focus | Active card identity, immediate acknowledgement, contextual metadata and deferred work. |
| Open title | Navigation ownership, entry readiness, initial action focus and the retained Home state. |
| Move to seasons/episodes | Coordinated hierarchy change, directional links, context selection and loading placeholders. |
| Choose an episode | Explicit user intent versus a default next-action target; version/track policy and any required comparison. |
| Start playback | Session ownership, source readiness, acknowledged clock and initial controls. |
| Seek or open tracks | Preview versus actual state, focus, available options, commitment and cancellation. |
| Press Back | Which interaction layer closes first and when playback actually ends. |
| Return Home | Accepted progress, refreshed target, selected item ID, row and scroll restoration. |

Then examine the same contracts with slow loading, an interrupted request and a removed item. These are targeted edge-case questions; the current static study does not claim to have exercised every variant on the TV.

## 5. What makes this an actionable handover

The engineer should be able to answer, for every transition:

1. What does the user believe they are doing?
2. Which component owns that intent?
3. What responds immediately?
4. What information or state is preserved?
5. What work runs later, and how is obsolete work stopped?
6. What happens if that work fails?
7. What does Back/cancel do?
8. Where does the user return?

**The intended result is a coherent set of component, navigation, loading, playback and continuity contracts.** The visual treatment should reinforce those contracts throughout the app.

## Related reading

* [Five UX learning frameworks and evidence ledger](Elements-of-A-UX-learning-frameworks.md)
* [Companion principal-architect response](Elements-of-A-UX-architect-response.md)
* [Implemented design system](Elements-of-A-implemented-design-system.md)
* [UI/UX code study](Elements-of-A-UI-UX-code-study.md)
* [Detailed UI findings](question-ui-findings.md)
* [Detailed playback findings](question-playback-findings.md)
* [All 37 engineering questions](Elements-of-A-engineering-questions.md)
* [Runtime architecture](Luna-runtime-architecture.md)
* [System map](Elements-of-A-system-map.md)
