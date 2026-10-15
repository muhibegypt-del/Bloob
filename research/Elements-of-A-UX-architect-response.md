# Elements of A UX: the principles beneath the interface

**My central finding: the experience preserves your intent while changing the amount of information, the active interaction layer and the underlying service.** You can move from browsing, to choosing an episode, to playback, then return without rebuilding your understanding of where you were.

I’ve consolidated the analysis into a reusable report: [**Elements of A — Five UX Learning Frameworks**](Elements-of-A-UX-learning-frameworks.md).

The analysis below separates:

- **Observed implementation:** recovered from the shipped LG application, build `16.4.4.J34L139`.
- **Documented product behaviour:** public documentation, with platform differences acknowledged.
- **Design interpretation:** the underlying principles and proposed rules for other applications.

The code reveals strong mechanisms. It does not reveal the complete recommendation algorithm or prove that a particular design reduces “doom scrolling.”

---

## 1. Information Architecture & Cognitive Friction Reduction

### The hierarchy follows a sequence of decisions

The useful structure is:

**Recognise → evaluate → act → inspect further.**

The recovered title page assembles identity, description, metadata, actions and supporting shelves as distinct components. Its first resolved valid action receives primary treatment. As the viewer moves down toward episodes, a coordinated transition reduces the prominence of the overview and brings the supporting content forward.

This is visible in `CanonicalLockup.createButtons` and `ProductPageInner.animateFold` in `ProductPage.js`. [Implementation evidence](Elements-of-A-UX-learning-frameworks.md#evidence-ledger-and-replication-boundaries)

**Design interpretation:** the page changes its information density as the task becomes more specific. Initially, you are deciding whether this title interests you. Further down, you are choosing what to watch within it.

### Metadata is edited, not merely displayed

| Component | Recovered mechanism | UX purpose — inferred |
|---|---|---|
| Hero/showcase | Richer description and contextual information than ordinary cards. | Help evaluate one candidate. |
| Title card | Compact identity and state-dependent information. | Support rapid recognition across alternatives. |
| Metadata line | Explicit limits and priorities for descriptive information. | Preserve important context when space becomes constrained. |
| Badge | State or qualification associated with the content. | Answer a decision-changing question quickly. |
| Provider offer | Separate provider/version comparison surface. | Keep alternatives available without making every interaction a comparison exercise. |
| Primary action | Filter unsuitable actions before promoting the first valid one. | Clarify what the viewer can actually do now. |

A concrete showcase variant caps genres at two, description at three lines and explanation at one line. That is **one component variant**, not a universal rule.

Sources include `MetadataLine.primary`, the `ShowcaseWideLockup` helper, `CanonicalLockup.createButtons` and `ProductHowToWatchShelfFactory`. [Metadata and component analysis](Elements-of-A-UX-learning-frameworks.md#12-metadata-has-an-information-budget)

### The progressive-disclosure rule to replicate

**Hide secondary detail while preserving identity, status, location and consequence.**

For Bloob:

- A browsing card establishes the title and its meaningful state.
- Title details establish the next episode or resume action.
- Other Versions exposes the comparison.
- Technical inspection exposes codecs, container and complete track details.
- A restriction that changes whether Play will work stays near Play.

A provider badge should not imply entitlement. A quality badge should not imply verified output capability.

**Beyond media:** an invoice list can show customer, amount, due state and next action. Payment history and line items can appear on inspection. Hiding the amount or overdue state would remove essential decision context.

### Does this solve doom scrolling?

It creates mechanisms that can reduce comparison work: meaningful grouping, a prominent action and bounded summaries. But the reference also supports many shelves and promotional surfaces.

**The transferable objective is a confident decision—not simply fewer cards or larger artwork.**

---

## 2. The “Up Next” Mental Model & Continuity Engineering

### Up Next is a next-action surface

A history list answers:

> What did I watch?

A useful continuation surface answers:

> What should happen when I return to this title?

The recovered client distinguishes contexts including Continue, Next Episode, Next Season, Available Now and watchlist membership. These contexts influence card labels and progress presentation.

The series can therefore retain its identity while its actionable episode changes. This is visible in the card metadata builder and episode-node play-event handling in `app.js`. [Continuation evidence](Elements-of-A-UX-learning-frameworks.md#2-the-up-next-mental-model--continuity-engineering)

### The recovered continuity machinery

| Layer | What the implementation establishes |
|---|---|
| Local checkpoint | `PlayPositionCache` stores playable identity, position, timestamp and user identity. It is a local checkpoint, not a complete offline history database. |
| Server progress | `PlayerStore.getMainContentStartPosition` reads the content cursor supplied with the playable’s play event. |
| Reporting | The UTS tracker reports position, duration and completion. The response can specify the next reporting interval. |
| Completion | Session timing and a duration-based fallback help determine when content is meaningfully finished. |
| Refresh | Play, watched, watchlist, subscription and off-device events can invalidate affected shelves. |
| Removal | Explicit removal updates the list; a specific live-event path handles expiry. |
| Return position | Restoration prefers the previous item ID, with a valid-index fallback. |

Sources: `PlayPositionCache`, `UTSPlaybackTrackerBase`, `getPlaylistCompletionThreshold`, `invalidateWithEvents`, `instantRemoveFromUpNext` and the mimic helpers. [Detailed continuity analysis](Elements-of-A-UX-learning-frameworks.md#22-what-the-recovered-continuity-machinery-actually-does)

An important discovery: **completion is not one universal watched percentage**. The inspected code can use supplied watched/post-play timing, otherwise duration bands. That supports the distinction between finishing the meaningful content and reaching the end of the file.

### Sorting and cross-device behaviour: what is known

Public documentation describes prioritised continuation, next-episode promotion, cross-device availability and supported connected integrations. These capabilities vary by platform and service. (public vendor support guide; link kept in the local research copy)

The LG client contains off-device change events and refresh mechanisms. However, its code does **not** reveal the complete server ranking or cross-device conflict-resolution algorithm.

Likewise, reporting completion is not proof of a universal local instruction to remove a film from every shelf. **Watchlist membership, history and continuation eligibility are separate states.**

### A State of Progress engine for Bloob — proposed

Separate four identities:

1. **Content:** film, series or episode.
2. **Edition:** the particular cut/version and timeline.
3. **User task:** what this viewer intends to continue.
4. **Session:** which device/playback session produced the latest accepted state.

Then resolve a single return action:

| State | Return action |
|---|---|
| Unfinished item | Resume its acknowledged checkpoint. |
| Completed episode, next available | Play the next eligible episode. |
| Series caught up | Show caught-up/upcoming context. |
| Completed film | Remove from continuation; retain history and independent watchlist intent. |
| Explicit dismissal | Respect it until a defined new action changes that state. |
| Expired source URL | Resolve again while preserving content identity and progress. |

This table is a **Bloob recommendation**, not a claim about the reference's hidden backend.

For reliable cross-device behaviour:

- Reject duplicate and out-of-order session events.
- Prevent an old device’s delayed stop from overwriting a newer session.
- Allow deliberate seeking backward and Start Over.
- Do not always keep the greatest timestamp.
- Do not transfer a timestamp blindly between different edits.
- Preserve the focused title when refreshed ranking changes.

**Beyond media:** “Continue onboarding” should return to the unfinished step with its context intact. Reopening a dashboard and making the person find that step again is incomplete continuity.

---

## 3. Focus & Spatial Ergonomics

### Focus is the remote user’s visible location

The recovered `SelectableWidget` owns focus, press, visibility and cancellation state. Season selectors distinguish contextual selection from active focus.

That distinction matters: a season can remain selected while an episode receives the next remote command. [Focus-state evidence](Elements-of-A-UX-learning-frameworks.md#3-focus--spatial-ergonomics)

A reusable component should distinguish:

| State | Meaning |
|---|---|
| Idle | Available but not targeted. |
| Context-selected | Supplies context, such as the active season. |
| Focused | Receives the next command. |
| Pressed | Input has been acknowledged. |
| Activated/selected | The action or choice has taken effect. |
| Disabled | Cannot currently activate. |

**Design interpretation:** clear states externalise location and readiness. The viewer should not need to remember where focus was before the artwork changed.

### What makes interaction feel tactile?

The shared defaults include faster focus arrival than departure, separate press/release feedback and changing shadow/elevation treatment. Components override these values.

For example, the inspected shared defaults include 85 ms arrival, 230 ms departure and 80 ms press/release. These are **implementation examples, not recommended universal timings or measured input latency**.

Sources: `SelectableWidget.animateFocusChange`, `initAnimations` and shared card styles. [Timing and depth evidence](Elements-of-A-UX-learning-frameworks.md#32-tactility-is-acknowledgement-plus-depth)

**Replicable guidelines:**

- Acknowledge focus before waiting for network work.
- Reserve room for focused growth; avoid shifting neighbouring cards.
- Retarget interrupted animations from their current state.
- Keep press feedback reversible.
- Cancel activation if the target disappears.
- Use redundant cues rather than colour or glow alone.

### Parallax and ambient colour: an important distinction

The LG implementation establishes:

- Scale and shadow depth cues.
- Scroll-linked stage movement through `EpicStageShelf.scrolled`.
- Artwork-supplied palette information.
- Contrast protection and transparency fallbacks.
- Image transitions gated by readiness or a designed error fallback.

It does **not** establish Siri Remote-style three-dimensional card tilt on LG. Nor does its palette mechanism prove continuous sampling of live video pixels. [Artwork and parallax evidence](Elements-of-A-UX-learning-frameworks.md#33-what-parallax-and-ambient-integration-actually-mean-here)

General tvOS guidance describes focus and parallax conventions, but that is a separate platform reference. [Official focus guidance](https://developer.apple.com/design/human-interface-guidelines/focus-and-selection/)

**Design interpretation:** ambience connects content to its surroundings; stable contrast and focus keep that connection usable.

### Directional navigation is part of the component contract

`GroupShelfWidget` explicitly coordinates season and episode rails. Up and Down move between related regions; episode selection updates its season context; compatible scroll state can be inherited.

This is stronger than letting a geometric “nearest rectangle” decide every movement. [Directional-navigation evidence](Elements-of-A-UX-learning-frameworks.md#34-directional-navigation-is-a-component-api)

For Bloob, specify:

- Horizontal movement within a rail.
- Vertical movement between related regions.
- Which episode receives focus when returning from seasons.
- What Back closes first.
- Which item and scroll anchor survive refresh.
- What happens when that item disappears.

For web and mobile, retain the principle but adapt the mechanism: keyboard focus must remain visible, pointer hover must remain distinct from selection, and essential actions must not exist only on hover.

---

## 4. Choice Architecture & High-Consequence Recommendation UI

### Different intentions need different choice structures

The reference supports distinct showcase, ordinary browsing, provider-offer and transactional components. It also explicitly supports promotional, rental, purchase and subscription content.

Therefore, “it contains no commercial clutter” is not a defensible code finding. The useful lesson is how a coherent system organises different kinds of choice. [Choice architecture evidence](Elements-of-A-UX-learning-frameworks.md#4-choice-architecture--high-consequence-recommendation-ui)

| Intent | Proposed structure | Primary action |
|---|---|---|
| Continue a commitment | Prioritised progress queue. | Resume / Play next episode. |
| Discover | Named collections and restrained featured candidates. | Investigate a title. |
| Compare | Structured versions/providers/plans. | Select a suitable option. |
| Commit | Explicit cost, requirements and consequence. | Buy, subscribe, approve or publish. |

An undifferentiated grid makes the user perform this classification mentally.

### Editorial and personalised content need honest explanations

The recovered showcase can display a short note and an explanation line. Editorial offer data also exists. These establish presentation support, **not the ranking algorithm or editorial/personalisation ratio**.

Sources: the showcase helper and `BrandNode.getUpsellEditorialData`. [Explanation evidence](Elements-of-A-UX-learning-frameworks.md#42-recommendation-explanations-must-be-truthful)

**Replicable rule:** explain a recommendation using information you actually possess.

Examples:

- New episode.
- Because you followed this series.
- Under 90 minutes.
- Chosen by our editors.
- Fits your five-editor approval workflow.

A popularity query should not be labelled personalised.

### Preview follows attention and permission

The recovered preview logic checks preferences, visibility, scroll/task state, competing playback/dialogs and capability gates. Automatic advancement also considers accessibility and motion policy.

Sources: `EpicStageShelf.canPlayVideo/canAutoAdvance`, slideshow activation and `ProductCard.startTrailer`. [Preview evidence](Elements-of-A-UX-learning-frameworks.md#43-preview-follows-attention-and-permission)

**Proposed contract:**

1. Focus feedback happens immediately.
2. Preview begins separately after deliberate dwell.
3. It is cancelled when attention changes.
4. It respects sound, motion and preview preferences.
5. The static interface remains fully informative.

### Progressive disclosure must preserve consequences

The provider-offer components can carry provider identity and offer/price information. The primary action system distinguishes playback from other actions.

**Design interpretation:** secondary specifications can be deferred; commitment-changing information should remain visible.

Applied elsewhere:

- **Pricing:** feature detail may collapse; billing period and material limits remain clear.
- **Dashboard:** foreground the next unresolved task while exposing the full queue.
- **Feature catalogue:** explain prerequisites before activation.
- **Bloob:** recommend a compatible version, with alternatives nearby; disclose a limitation that materially changes the expected playback experience.

The product objective I would choose is **time to a confident, successfully completed decision**. That is a recommendation for Bloob, not a recovered reference metric.

---

## 5. Design System & UX Principles Matrix

The implementation column is evidence-based; cognitive explanations and transfer rules are design interpretations.

| Principle | Reference implementation | Underlying principle | Replicable guideline | Avoid |
|---|---|---|---|---|
| **Task-shaped hierarchy** | Distinct discovery/detail/offer components, metadata budgets and coordinated folding. | Recognition and progressive disclosure. | Answer one current question while retaining identity, state and next action. | Metadata walls, competing primary actions, hidden restrictions. |
| **Continuity by intent** | Checkpoints, playback reports, completion and shelf invalidation. | Externalised memory and task resumption. | Resolve one valid return target across sessions and sources. | Progress keyed to temporary URLs; stale writes; false resume actions. |
| **Explicit spatial ownership** | Focus/press/cancel states, directional rails and ID-based restoration. | Orientation, spatial memory and reversibility. | Define focus and return behaviour alongside component layout. | Focus jumps, multiple active-looking targets, navigation blocked on metadata. |
| **Protected ambient integration** | Artwork palettes, contrast fallbacks and readiness-gated transitions. | Perceptual continuity with legibility. | Let content affect ambience without weakening foreground meaning. | Tint flicker, unreadable overlays, motion required for comprehension. |
| **Guided choice with control** | Valid primary action, alternatives, explanations and conditional previews. | Reduced comparison effort while preserving agency. | Recommend honestly, disclose consequences and expose alternatives. | False personalisation, aggressive autoplay, promotion disguised as continuation. |

[Full matrix and component playbook](Elements-of-A-UX-learning-frameworks.md#5-design-system--ux-principles-matrix)

### What this means concretely for Bloob

The strongest implementation sequence is:

1. **A card contract:** identity, meaningful status, focus, activation and accessible semantics.
2. **A shelf contract:** stable order, retained selection and explicit navigation.
3. **A title-page contract:** one valid primary action, clear alternatives and preserved return context.
4. **A progress contract:** authoritative state, semantic completion and ordered session events.
5. **A player contract:** actual clock, separate seek preview, usable tracks and predictable Back.
6. **A presentation policy:** ambience and motion that support those contracts.

**The transferable advantage is coherence: every component agrees about what you selected, what you can do, what has happened and where you return.** The visual polish reinforces that agreement.
