# Elements of A: five UX learning frameworks

**Study date:** 15 October 2026. **Reference:** saved LG webOS update, build `16.4.4.J34L139`.

## Finding and evidence conventions

The strongest transferable idea is **preserving the person's intent while changing the amount of information, the interaction layer and the underlying service**. Cards, shelves, focus, playback and progress reinforce one task model.

That is a design interpretation, not a recovered statement of the original team's strategy. The code establishes mechanisms; it does not prove a reduction in decision fatigue, a universal usability ranking or the team's psychological rationale.

Three evidence labels apply throughout:

* **Observed:** read in the shipped LG application. Sources and function/byte locations are in the evidence ledger. This is static analysis, not a new live-TV recording.
* **Documented product behaviour:** public vendor documentation, with platform scope stated. General tvOS guidance and mobile integration features are not automatically evidence of the same implementation on LG.
* **Inferred / proposed:** the cognitive interpretation and the original design rules recommended for Bloob or another application.

The recovered client does not expose the complete backend recommendation algorithm, cross-device conflict resolution or every third-party integration contract. These boundaries are identified where they matter.

## 1. Information Architecture & Cognitive Friction Reduction

### 1.1 The useful hierarchy is a hierarchy of questions

**Observed:** the client has different components for a stage/showcase, ordinary shelf item, title overview, episode selection and provider offer. `CanonicalLockup` assembles title/context, description, actions and supporting information. Its first resolved action becomes primary; other actions can use a more compact treatment. A shared fold transition changes the prominence of the overview as the user moves to supporting shelves. [E01, E02, E03]

**Inferred interpretation:** this supports a progression:

| Task stage | Question the interface should answer | Appropriate information |
|---|---|---|
| Recognise | What is this? | Title/artwork and enough context to distinguish film, show and episode. |
| Evaluate | Is it relevant and available to me? | A short premise, progress/next-episode state, availability and meaningful qualifications. |
| Act | What exactly happens if I choose it? | One prominent, valid action with a clear consequence. |
| Inspect | What alternatives or details do I need? | Seasons, provider/version comparison, full description and technical information. |

This is a proposed product model, not the server's recovered shelf-order specification. The reference can contain many shelves and promotional surfaces; a hero and horizontal rows do not by themselves solve endless browsing.

### 1.2 Metadata has an information budget

**Observed component breakdown:**

| Component | Recovered behaviour | Transferable lesson — inferred |
|---|---|---|
| Stage/showcase | A richer component than the ordinary scanning card; supports title/logo, description, metadata and optional explanation. | Use a different density for an invitation to investigate than for an item in a repeated list. |
| Compact metadata line | `MetadataLine.primary` accepts a genre cap and gives descriptive entries differing priorities/removability. | State which fields yield under pressure; do not let every field compete equally. |
| One showcase variant | Two genres maximum, a three-line description and an optional one-line explanation. These are variant-specific values. | Define a content budget per component, rather than truncating arbitrarily after data arrives. |
| Badge | Badge/state support includes live, rental and other supplied title qualifications. | Spend badge attention on a decision-changing fact. |
| Provider offer | A separate How to Watch surface can show provider identity, offer text/price and available versions. | Keep a sensible primary action while making comparison discoverable. |
| Primary title action | Smart actions are supplied/resolved through the model, unsuitable actions are filtered, then the first valid action receives primary treatment. | Resolve action validity before presentation, rather than showing a collection of misleading disabled controls. |

Sources: `MetadataLine.primary`, showcase helper `eI`, `Badge`, `CanonicalLockup.createButtons`, the VDS `getButtons` helper and `ProductHowToWatchShelfFactory`. [E01, E02, E04, E05]

Provider identity, ownership and entitlement are different facts. A provider mark identifies an offer/source; it does not by itself establish that the viewer can watch without subscribing or paying. Similarly, a format badge is metadata, not a physical output test. [E05]

### 1.3 Progressive disclosure also happens through movement

**Observed:** `ProductPageInner.animateFold` coordinates title/logo visibility, overview prominence, shelf transforms and background protection. Moving into episodes changes the information hierarchy through one state value. It is not just several independent elements scrolling away. `EpisodePaging` keeps episode positions available through placeholders and loads nearby detail after a delay. [E03, E06]

**Inferred cognitive explanation:** recognition is easier when identity and location remain stable while detail changes. Moving from “decide whether to watch this show” to “choose an episode” should feel like the same task becoming more specific.

### Replicable rule

**Proposed:** hide information when it is unnecessary for the current decision, while retaining identity, status, location and consequence.

For a data-heavy application:

1. Keep the record name, workflow state and next action visible.
2. On focus/selection, reveal a short decision summary.
3. On explicit inspection, reveal the complete comparison or technical detail.
4. Keep costs, destructive consequences, access restrictions and important warnings visible before commitment.
5. When details load or a panel opens, preserve the selected record and return position.

**Example:** an invoice list initially shows customer, amount, due state and next action. Selecting an invoice reveals the payment history; opening details reveals line items. Hiding payment status or the amount behind a tooltip would violate this rule.

**Bloob example:** a title card establishes identity and progress. Title details offer Resume S2 E5 and a discoverable Other Versions action. Codec, container, source provenance and full audio specifications belong in the version comparison surface; a compatibility restriction affecting Play must remain visible near Play.

## 2. The Up Next Mental Model & Continuity Engineering

### 2.1 The shelf represents a next action, not merely a past event

**Observed:** the client differentiates continuation contexts such as Continue, Available Now, Next Episode, Next Season, watchlist membership and subscription/purchase requirements. Its card label builder uses those contexts, progress and played state. Episode watched state considers play events across its available playables and uses the most recent dated event. [E07]

**Documented product behaviour:** the current mobile guide describes a prioritised Continue Watching row, next-episode replacement after completion, supported connected apps/channels and a live-sport priority example. That is product documentation for that platform; it does not reveal the LG server's complete ranking weights. [Personalisation guide](https://support.apple.com/en-za/guide/iphone/iphbf1137ac0/ios)

**Inferred mental model:** “Bring me back to what I intended to do, at the next meaningful step.” A series can stay conceptually present while its episode target changes.

### 2.2 What the recovered continuity machinery actually does

| Mechanism | Observed client behaviour | Why it matters — inferred |
|---|---|---|
| Resume checkpoint | `PlayPositionCache` stores a playable ID, position and timestamp with a user identity; it checks the requested playable and clears mismatched-user state. It is one local checkpoint record, not proof of a complete offline watch-history database. | A local interruption need not erase the most recent position, and one viewer's checkpoint should not become another viewer's resume target. |
| Server progress | `PlayerStore.getMainContentStartPosition` reads the playable's content cursor from its play event and guards an out-of-range start case. | Progress belongs to the playable/content model, not to whichever screen was last visible. |
| Playback reporting | The UTS tracker reports playback position, media/main-content lengths and completion state. The response can supply the interval before the next timed report. | Client activity updates the state used by the rest of the product. There is not one universal hardcoded five-second reporting cadence in this path. |
| Semantic completion | Completion can use session-supplied watched/post-play timing and a duration-based fallback. It is not simply “decoder reached the final byte.” | End credits and next-episode presentation can be reconciled with a useful completion decision. |
| Shelf invalidation | Caching policies subscribe to play, watched, watchlist, subscription and off-device add/remove events; interval refresh is another supported policy. | Screens are projections of changing user state, rather than independent copies of truth. |
| Explicit removal | Contextual Remove from Up Next updates the visible list. A specific live-sport path removes an item when its live end time arrives. | Users can correct the queue, and time-limited tasks need expiry rules. |
| Stable return | Compatible widgets inherit selection/scroll; the specialised policy prefers the previous item ID, then falls back to a valid index. | A refreshed shelf should not strand the user at a different title. |

Sources: `PlayPositionCache`, `UTSPlaybackTrackerBase`, its VOD tracking methods, the completion helpers, `invalidateWithEvents`, `instantRemoveFromUpNext`, `removeFromContinueWatchingWhenStreamEnded` and the mimic helpers. [E07–E12]

**Auto-removal boundary:** the client establishes explicit user removal, a special live-event expiry and progress/completion reporting that can cause a refreshed server shelf to change. It does not establish one universal local rule that deletes every finished movie from every shelf. Watchlist membership, watched history and continuation eligibility must not be treated as synonyms.

### 2.3 Cross-device and third-party continuity

**Documented product behaviour:** watch history is synced between supported devices and can be cleared; using play history for recommendations/watchlist is a setting. Supported connected integrations vary by platform and service. [TV privacy settings](https://support.apple.com/en-gb/guide/tvapp/atvb2eb91635/web), [supported-app personalisation](https://support.apple.com/en-za/guide/iphone/iphbf1137ac0/ios)

**Observed in this LG client:** off-device add/remove events can invalidate a shelf or create a notification. Watchlist operations address canonical content while playback also has playable/provider identifiers. [E09, E10, E13]

**Not recovered:** the server conflict-resolution policy, guaranteed sync latency, exact recommendation ranking, complete provider-to-canonical identity mapping and all third-party reporting contracts. “Seamless across everything” would overstate this evidence.

One failure path is also worth understanding: a failed activity-report request is logged and does not schedule its next timed request. Other contract events can still occur, but these functions do not establish a durable offline replay queue. This is a reason to build explicit reliability into Bloob, not to assume the reference guarantees it. [E08]

### 2.4 A unified State of Progress engine — proposed for Bloob

This is an original architecture recommendation, not the recovered vendor backend.

**Separate four identities:**

1. **Content:** series, season, episode or film.
2. **Edition/version:** the specific cut/file and compatible tracks.
3. **User task:** what this profile intends to continue or has explicitly saved.
4. **Playback session:** which device/session produced the checkpoint.

A resume record needs content ID, edition identity, authoritative position/runtime, completion state, session ID, event sequence/revision and last accepted activity. Keep watchlist membership and explicit dismissal separate. Track preferences should represent language/role intent, not a decoder-array index.

**Proposed next-action decision table:**

| State | Single return action | Shelf consequence |
|---|---|---|
| Valid unfinished episode/film | Resume that item at its acknowledged checkpoint. | Retain continuation. |
| Completed episode; next released episode available | Play the next eligible episode. | Replace the episode target under the same series identity. |
| Series caught up; next episode unreleased | Show caught-up/upcoming context. | Do not offer a false Play action. |
| Completed film | Start Over only as an explicit option. | Remove from Continue Watching; preserve history and independent watchlist intent. |
| Explicitly dismissed title | Restore only through an intentional new user action or defined policy. | Do not reinsert on every catalogue refresh. |
| Source URL expires | Resolve again while retaining content/session progress. | Do not erase progress or replace the title with an unrelated recommendation. |

**Proposed ranking:** active unfinished tasks first, then newly available continuation of followed series, then deliberately saved unstarted items. Recency can order comparable tasks. If live content exists, time urgency can be an explicit exception. These are transparent Bloob policies, not claims about the reference's algorithm.

**Proposed conflict rules:**

* Reject duplicate and out-of-order events using session/revision information.
* Give a newer accepted session precedence over a delayed stop from an older one.
* Treat seeking backward and Start Over as valid operations. Keeping the greatest position forever would break both.
* Do not blindly reuse a timestamp across editions with different cuts/timelines.
* Persist locally before an unreliable network send, then reconcile and acknowledge on the server.
* Refresh the data without moving the focused title out from under the user. Reorder on a suitable navigation boundary or preserve the selected ID explicitly.

**One-action return contract:** the surface should already know the target item, valid source policy and resume point before activation. If access, a provider choice or an account gate is still required, describe that honestly rather than promising immediate playback.

**Beyond media:** a SaaS dashboard can offer Continue onboarding at the first unfinished required step; a document tool can reopen the same document/section and restore the relevant panel. It should not merely reopen the home page and make the person find their unfinished work again.

## 3. Focus & Spatial Ergonomics

### 3.1 Focus is the remote user's location

**Observed:** `SelectableWidget` owns focus, press and visibility animation state, scaled geometry and activation/cancellation behaviour. Season selectors distinguish a context/focus-candidate treatment from active focus. The selected season can remain understandable while an episode receives commands. [E14, E15]

**Inferred cognitive interpretation:** the interface externalises location and action readiness. A person should not have to remember which item was highlighted before the background changed.

### Component state contract — proposed

| State | Meaning | Required feedback |
|---|---|---|
| Idle | Available, not targeted. | Stable identity and geometry. |
| Context-selected | Supplies context, such as the selected season. | A retained but subordinate treatment. |
| Focused | Receives the next remote command. | One unmistakable target. |
| Pressed | Input has been acknowledged. | Immediate, reversible feedback. |
| Activated/selected | The user committed an action or choice. | A persistent result where appropriate, not just a transient glow. |
| Disabled | Action unavailable. | Honest availability semantics; no misleading press success. |

Focus, selected state and hover are not interchangeable. Official tvOS guidance also distinguishes focus from activation; standard tvOS can use parallax. That guidance is not evidence that LG uses UIKit's focus system. [Focus and selection guidance](https://developer.apple.com/design/human-interface-guidelines/focus-and-selection/)

### 3.2 Tactility is acknowledgement plus depth

**Observed:** shared focus defaults include 85 ms arrival, 230 ms departure and 80 ms press/release animation; season components override these with their own values. A shared card style changes its shadow/elevation treatment on focus. These are logical component defaults, not measured input latency or universal animation targets. [E14, E15]

**Inferred interpretation:** quick arrival makes the new target assert itself; gentler departure lets the old target settle. Press/release and shadow/scale reinforce what is happening without requiring a pointer.

**Proposed guidelines:**

* Update logical focus and its visible acknowledgement before starting dependent network work.
* Reserve space for focused growth; do not relayout the entire row on every move.
* Retarget/cancel animations from their current state when rapid navigation changes intent.
* Let activation happen on the intended input gesture, not because focus passed over an item.
* Keep cancellation reversible; do not activate a widget that was disposed or replaced while input was in flight.
* Do not use glow alone. A modest shape, scale, outline or contrast change should still make the target clear without relying on one colour.

### 3.3 What parallax and ambient integration actually mean here

**Observed:** `EpicStageShelf.scrolled` performs clamped scroll-linked vertical stage movement. The shared widget/card system supplies scale and shadow depth cues. The inspected evidence does not establish Siri Remote-style three-dimensional tilt on LG. [E14, E16]

**Observed:** `Artwork.getColor` reads a supplied artwork palette; it is not evidence of continuously sampling live video pixels. Background logic protects contrast, supports Reduce Transparency and retains the current image while a replacement becomes Ready or Error. The transition stops subscribing to frame work once complete. [E17]

**Inferred application:** make content and controls feel related while protecting legibility. Ambient colour should support context, not cause the whole workspace to flash whenever focus moves quickly. Depth should distinguish interaction layers, not be a decorative tax on every frame.

### 3.4 Directional navigation is a component API

**Observed:** `GroupShelfWidget` routes Up from episodes to season groups and Down back to episodes. Episode choice updates its associated season; season focus can reposition the episode rail according to policy. Rail scroll state can be inherited. Restoration can preserve a prior item by ID. [E11, E15]

**Proposed directional rules:**

* A row owns horizontal item navigation; a section owns transitions between its header and contents.
* Up/Down transitions preserve the associated context, not an accidental nearest rectangle.
* Back closes the nearest active interaction layer before leaving the larger task.
* Returning from details restores the title, row and scroll anchor; use a defined fallback if that title disappeared.
* Loading placeholders reserve navigable positions so metadata arrival does not move the target.

The reference's root Back paths are conditional: overlays, nonroot pages, return-to-top/navigation and platform exit policies differ. It is not a universal “Back always opens an exit dialog” rule. [E18]

### 3.5 Adapt the principle to the input device

| Input | Replicable rule — proposed |
|---|---|
| TV D-pad | Persistent visible focus, deterministic directional relationships, explicit activation and Back. |
| Keyboard | Visible focus indicator, logical groups and return focus after dialogs. |
| Pointer | Hover may preview context, but selection and keyboard focus remain distinct. |
| Touch | Do not hide essential actions in a hover state that cannot be reached. Provide a clear tap/detail path. |

**Observed accessibility support:** custom widgets expose labels, roles, enabled state, item position/count and containers to an app-owned reader. Motion/automatic presentation is gated by accessibility and capability settings. [E19]

**Inferred rule:** semantic state and visual state must agree. The UX must remain understandable with motion/transparency/previews disabled; animation cannot be the only way to discover what changed.

## 4. Choice Architecture & High-Consequence Recommendation UI

### 4.1 Organise choices by intent

**Observed:** the client supports showcase/hero, ordinary browsing, provider-offer and promotional/transaction surfaces, with model-supplied explanatory text and actions. That supports differentiated emphasis; it does not expose the backend proportion of editorial versus personalised ranking. The application explicitly includes upsell, rental, purchase and subscription paths. It should not be described as free of commercial content. [E01, E04, E05, E20]

**Inferred product interpretation:** confident choice depends on matching presentation to the question:

| User intent | Suitable architecture — proposed | Meaningful primary action |
|---|---|---|
| Continue a commitment | Small, prioritised progress queue. | Resume / Play next episode. |
| Discover something relevant | Named collections and restrained featured recommendations. | Open a promising title. |
| Compare available ways to proceed | Structured provider/version/plan comparison. | Select a suitable option. |
| Make a consequential commitment | Explicit cost, requirements, consequence and confirmation where appropriate. | Buy, subscribe, publish or approve with an honest label. |

These tasks should not all become one undifferentiated grid.

### 4.2 Recommendation explanations must be truthful

**Observed:** a showcase variant can render a short note and a one-line explainability field. Editorial offer data also exists. These are presentation capabilities; the client does not prove why an individual recommendation was ranked. [E04, E20]

**Proposed:** show a concise reason that matches available evidence: New episode, Because you followed this series, Under 90 minutes, or Chosen by our editors. Do not label a popularity query as personalised or invent a preference match to fill an empty explanation field.

For SaaS, a recommended plan can say Fits five editors and two approval stages if those are real requirements. Recommended by itself is not a sufficient explanation of fit.

### 4.3 Preview follows attention and permission

**Observed:** video preview eligibility checks user preview preference, foreground/deck state, scroll position, product-page state, competing playback/dialogs and capability gates. Slide activation can delay video start according to component configuration. Automatic stage advance additionally checks motion/performance policy; the Home stage checks screen-reader state. There is no demonstrated universal preview delay. [E21]

**Inferred principle:** the person owns attention. A preview can answer what something feels like, but should not interrupt choosing another item or compete with the active player.

**Proposed preview contract:**

1. Focus acknowledgement is immediate; preview is a separate, cancellable activity.
2. Require deliberate dwell before expensive preview work.
3. Cancel when attention changes, the app backgrounds, a modal intervenes or primary playback starts.
4. Respect preview, sound, accessibility and capability preferences.
5. Keep equivalent title/context/action information available without video.

### 4.4 A clean surface must still reveal consequences

**Observed:** the provider-offer surface contains the provider/offer relationship and subscription/price text; client action filtering can distinguish a play action from transaction or How to Watch flows. [E02, E05]

**Inferred rule:** progressive disclosure may hide secondary specifications, but it must not hide what the user is committing to.

Examples:

* **Bloob:** suggest the highest-ranked compatible version and keep Other Versions nearby. Before Play, disclose unavailable audio/HDR capability if it changes the expected experience. A raw filename is not a good primary label.
* **Pricing:** show the suitable plan with billing period, total commitment and material limits. Put the exhaustive feature matrix nearby; do not obscure a cost behind an attractive primary button.
* **Dashboard:** foreground the next unresolved action and its consequence, with the full queue available. Do not make every notification a competing primary task.
* **Feature catalogue:** group by the user's goal and availability, and explain prerequisites before activation. Avoid making disabled features look ready merely to advertise them.

### 4.5 What to optimise

**Proposed product objective:** reduce time to a confident, successfully completed decision. Track mistaken launches, abandoned comparisons, unnecessary source changes, focus loss and failed resumes alongside discovery outcomes.

That objective is a recommendation for Bloob, not a recovered internal metric. The shipped code cannot tell us which user study caused a design change or prove that any particular visual treatment reduces endless browsing. [E22]

## 5. Design System & UX Principles Matrix

The implementation column records evidence. The cognitive column is an **inferred interpretation**, and the guidelines/anti-patterns are an **original design playbook**.

| Foundational rule | Reference implementation | Cognitive interpretation | Replicable guideline | Anti-patterns to avoid |
|---|---|---|---|---|
| **Task-shaped hierarchy** | Distinct discovery/detail/offer components; compact metadata budgets; coordinated title-page fold. [E01–E05] | Recognition and progressive disclosure reduce reconstruction and comparison work. | Make each screen answer one current question; retain identity, status and next action as detail expands. | Metadata walls; identical prominence for every action; hiding material costs or restrictions. |
| **Continuity by intent** | User-scoped checkpoint, playback reports/completion, canonical watchlist actions and shelf invalidation. [E07–E13] | Externalise memory so the person need not reconstruct unfinished work. | Resolve one valid return target; preserve task identity across sources/devices; reconcile versioned events. | Resume by array index or temporary URL; stale stop overwrites newer session; completed item remains a false resume. |
| **Explicit spatial ownership** | Focus/press/cancel states, coordinated season rails, ID-based restoration and Back layering. [E11, E14, E15, E18] | Stable location and reversible action support orientation and confidence. | Specify a focus graph and return anchors as part of every component's contract. | Multiple active-looking targets; row reflow on focus; navigation blocked on metadata; Back unexpectedly exits playback. |
| **Ambient integration with protection** | Artwork-supplied palette, contrast fallbacks, Ready/Error-gated image transitions and reduced-motion/transparency paths. [E16, E17, E19] | Perceptual continuity can support context while stable contrast protects recognition. | Let content influence ambience without changing foreground semantics or removing the static accessible path. | Live tint flicker; illegible text over art; blur/parallax required to understand selection; unbounded frame work. |
| **Guided choice with user control** | Valid first smart action, separate offer comparison, explainability fields and conditional previews. [E02, E04, E05, E21] | Reduce unnecessary comparison while preserving agency and consequential information. | Recommend a defensible next action, explain fit, expose alternatives and keep previews subordinate. | Every card is Recommended; aggressive autoplay; false personalisation; promotion disguised as continuation. |

### Component-level playbook for Bloob

All contracts below are **proposed**, informed by the cited mechanisms.

| Component | Required contract | Failure that contract prevents |
|---|---|---|
| Discovery card | Stable content ID; idle/focus/press/disabled states; one decision-changing status; accessible identity; activation separate from preview. | Hover/focus accidentally launches; ambiguous target; metadata overload. |
| Continuation card | Resolved episode/film target, position, continuation reason, valid source policy and dismiss action. | Opens generic details when a true direct resume was possible; wrong episode/version. |
| Shelf | Stable ID/order snapshot; selection ID and fallback index; loading/empty/error treatment; defined Up/Down/Back. | Refresh resets focus or moves the user's target. |
| Title page | Identity/context, one valid primary action, clear alternatives, retained return state and coordinated overview-to-episodes transition. | Repeated decision-making and losing the show/season context. |
| Season/episode selector | Independent season context and episode focus; stable placeholder positions; bounded nearby requests; explicit directional links. | Loading causes jumps; navigating ten seasons triggers ten unnecessary full requests. |
| Version picker | Semantic version labels, compatibility/reason, recommended choice, complete inspectable details and explicit selection result. | The viewer must decode filenames or repeatedly try unsuitable files. |
| Player | Authoritative media clock plus separate preview/requested seek; interaction-aware controls, usable tracks and Back/cancel hierarchy. | A responsive-looking seek bar that lies about actual playback. |
| Resume store | Canonical task state, session/revision ordering, durable checkpoints and explicit reset/completion operations. | Cross-device stale writes, incorrect replays and progress attached to expiring URLs. |

### Example end-to-end contract

1. Home shows **Continue: Series X — S2 E5 — 24 minutes left**.
2. Focus immediately acknowledges the card; optional surrounding context updates without replacing its identity.
3. Activation uses the already resolved next action and compatible source preference.
4. The player resumes from its accepted checkpoint. The UI displays the actual media clock.
5. Seeking previews separately, then commits/cancels explicitly.
6. On leaving playback, the accepted session checkpoint updates the shared progress model.
7. Home returns to the same card/row anchor. Completion may change its episode target, but never silently moves focus to an unrelated title.
8. Other Versions remains an explicit comparison path, retaining task context when closed.

This sequence is an original Bloob proposal. The reference supplies supporting mechanisms, not proof that every one of its own routes follows this exact sequence.

## Evidence ledger and replication boundaries

Byte offsets below refer to the effective saved update, not an assumed current TV installation. Some ranges are approximate anchors; function names disambiguate them. No extracted source is reproduced here.

| Evidence | Shipped file and type/function | Source location / finding |
|---|---|---|
| E01 | `ProductPage.js`, `CanonicalLockup.refresh` | Class begins about byte 61,283; overview, context, actions and supporting components. |
| E02 | `ProductPage.js`, `CanonicalLockup.buttonRow/createButtons/createSmartButton`; `app.js`, `ProductNode.getVDSSmartButtons`, VDS helper `getButtons` | Product about 66,468–69,000; app 137,614 and 1,472,534. Primary action follows supplied/resolved valid actions. |
| E03 | `ProductPage.js`, `ProductPageInner.animateFold/addVerticalNavigation` | About bytes 80,500–83,000; coordinated hierarchy/targets. |
| E04 | `app.js`, `MetadataLine.primary`; showcase helper `eI`, `ShowcaseWideLockup` Flavor_F | About 1,537,700; 440,471–443,492. Metadata priorities; one variant's two genres/three description lines/one explanation line. |
| E05 | `ProductPage.js`, `ProductHowToWatchShelfFactory`, card creators `tg/tc.create`; `app.js`, `CardOffer`, `Badge`, `ProductNode.getBadge` | Product about 51,000–54,500; app 238,187, 1,519,641 and 138,200. Offer/provider/badge information. |
| E06 | `ProductPage.js`, `EpisodePaging` | Early file, especially about 2,450–4,050. 500 ms nearby fetch, stable placeholders and bounded retained detail. |
| E07 | `app.js`, factory 85770 episode node `isWatched`; card metadata helper `em` and label builder | About 87,431–92,584 and 423,180–426,300. Most recent playable event and contextual continuation labels. |
| E08 | `PlayerStore.js`, `UTSPlaybackTrackerBase`, VOD `runContract/run/send/parseTimeOut/scheduleNext/runTimed/trackingEvent/record` | About 49,079–53,900. Activity reports, server-returned cadence, Done state and a failed-report scheduling limit. |
| E09 | `app.js`, `PlayPositionCache.init/savePlayPosition/tracker/getPlayPosition/setPlayPosition`; `PlayerStore.js`, `getMainContentStartPosition` | App about 1,219,815–1,221,400; player about 106,600. Local checkpoint and server content cursor. |
| E10 | `app.js`, factory 30002, `invalidateWithEvents` | Bytes 19,371–21,923. Refresh policy and play/watchlist/off-device invalidation events. |
| E11 | `app.js`, factory 44379, `mimic/mimicConfigSelectPreviouslySelectedOrSameIndexIfRemoved`; `SuperBrowsePage.js` related-shelf state | App about 298,054–300,002 and application at 366,008; browse about 140,423–141,200. Compatible state/ID/scroll restoration. |
| E12 | `app.js`, factory 33641 `instantRemoveFromUpNext`; factory 99392 `removeFromContinueWatchingWhenStreamEnded`; completion helper `getPlaylistCompletionThreshold` | About 34,069; 23,200; 1,424,918. Explicit removal, live-event expiry and session/fallback completion decision. |
| E13 | `app.js`, factory 15504 watchlist operations; event dispatchers/root listeners | Bytes 1,483,257–1,484,736; about 877,300 and 1,127,172. Canonical IDs and off-device add/remove notifications. |
| E14 | `app.js`, factory 81539, `SelectableWidget.animateFocusChange/initAnimations/getScaledTransform` and press/release/cancel paths; factory 31210 styles | About 2,384,157–2,389,700 and 2,394,108. Shared defaults 85/230/80 ms; component overrides exist. Shared shadow style about 1,498,931. |
| E15 | `ProductPage.js`, `GroupShelfWidget`, season styles and `inheritScrollValues` | Styles about 17,348; class 18,567; context, Up/Down, accessibility and scroll about 19,630–23,000. |
| E16 | `app.js`, `EpicStageShelf.scrolled` | About 1,710,390. Clamped scroll-linked stage translation; not evidence of 3D card tilt. |
| E17 | `app.js`, factory 62062 `Artwork.getColor/getTheme`, `solidBackgroundColor/getLuminosityFor`; `ProductPage.js`, `BackgroundWidget._setPendingLayer/_handleFrame/_unsubscribeFrame` | App about 75,605 and 744,745; product about 6,441–7,947. Supplied palette, contrast/fallback and finite image transition work. |
| E18 | `app.js`, root key handling, Exit confirmation and `runExitAction` | About 1,133,050–1,135,650, 1,142,195 and 1,997,190. Layered/conditional Back and platform exit policy. |
| E19 | `app.js`, factory 8824 `ScreenReader`; `SelectableWidget.initAnimations`; device presentation profile | About 1,936,395–1,961,632, 2,387,833–2,389,700 and 54,800. Semantics, motion/accessibility and capability gates. |
| E20 | `app.js`, `ShowcaseWideLockup`, `EpicStageLockup`, `BrandNode.getUpsellEditorialData` | About 434,094, 1,684,800 and 83,619. Presentation variants and editorial data, not ranking weights. |
| E21 | `app.js`, `EpicStageShelf.canPlayVideo/canAutoAdvance`, `SlideShowWidget.canPlayVideo/canAutoAdvance/activateSlide`; `ProductPage.js`, `ProductCard.startTrailer` | About 1,712,077 / 1,712,510; 1,801,489–1,802,900; product about 102,700. Preview, motion and attention gates. |
| E22 | Scope of the local shipped-artifact study | No source evidence establishes causal usability metrics, editorial/personalisation percentages, team user-test history or current active feature flags. |

### Completion is content-aware in the inspected build

`getPlaylistCompletionThreshold` uses supplied watched/post-play timing when suitable, otherwise `playedThresholdTimeForDuration`. The latter has duration bands, not a single 90% rule: under 10 seconds uses the duration; under 15 minutes uses duration minus 10 seconds; under 30 minutes minus 3 minutes; under 65 minutes minus 4 minutes; under 100 minutes minus 8 minutes; otherwise minus 11 minutes. These are recovered fallback values, not recommended universal defaults. `trackingEvent` also marks Done when showing Up Next. The exact resulting removal/ranking still depends on the service. [E08, E12]

### REA provenance

* Reused full-update parent: `ev_c13f3062de5c4ca184373b809416b4d23313f7a4652a3d57a1462793d9c0daed`. Inventory parsed 143 JS files; cross-file semantic coverage was partial because of resource limits.
* Revalidated current app-file inventory: `ev_ec5ddada3971880b5c6f00c221bfce097633ae09d5e1606b4352eb7e6522bf5f`. This confirms source identity and retained static parsing; it is not a runtime trace.
* Fresh focused continuity parent: `ev_fe22f51f17b5499df19d300a1fda9b2ad3d0c1b3ddbe57ba3ffda2f97014fa85`. Five exact factory slices from the authenticated `app.js`, wrapped only as inert analysis carriers; 6,020 AST nodes, no parse failures. Semantic coverage remains partial/unknown, with 1,396 semantic unknowns. The wrapper/provenance remains in temporary analysis storage, not this repository.
* Fresh focused UI-core parent: `ev_dc7390e96b052b69de9d525de5dedd5dc259dc022546a6c3536c95dbedf0cc82`. Four exact factory carriers covering artwork/restoration/lists/reader; 14,746 AST nodes, no parse failures, 3,399 semantic unknowns.
* Other function-specific findings were checked directly against the original effective update bytes. The focused carriers do not claim to cover every function in this ledger.

| Original effective file | SHA-256 |
|---|---|
| `app.js` | `af5dd20af5f023b4e6289f8fc87df70cddd964b3b28adc5727d819bcda3ccef4` |
| `ProductPage.js` | `3cf224603413db5d9be5324efa7f9c11102086e800f6f8972155747e0f6d1870` |
| `SuperBrowsePage.js` | `77c182bc42387e5c4c4344e9253f60f4f8ff51ce8ca1586f34fb5ba64508cc25` |
| `PlayerPage.js` | `51a5642497181181cc4bbae9527b9c891421763e4c25e4f528885df8b8934955` |
| `PlayerStore.js` | `b957612a3e17dfa5645cc95f46c4f9eb57db79130a40f7feebaaf42353944c06` |

### Boundaries that matter when applying this study

1. The saved update's active installation/feature selection was not observed here.
2. Readable client code is evidence for mechanisms, not hidden service algorithms or guaranteed cross-device synchronisation.
3. General tvOS focus/parallax guidance must not be presented as proof of the LG implementation.
4. Timing/style constants are not measurements of remote-to-frame latency or proof of user preference.
5. A coherent application can still contain commercial friction, backend limitations and recovery gaps. Copy the useful principles deliberately rather than assuming every reference branch is an ideal to reproduce.

Related local studies: [implemented design system](Elements-of-A-implemented-design-system.md), [UI evidence](question-ui-findings.md), [playback evidence](question-playback-findings.md), [JetPack identity](jetpack-analysis-2026-10-14.md).
