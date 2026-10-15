# Elements of A: answers about UI performance, focus and accessibility

Date: 2026-10-14. This pass studies the nonempty JavaScript files in the decoded update for build `16.4.4.J34L139`. It describes inert shipped code, not execution on a TV. Activation of this saved update on a specific TV remains unverified. Original source and assets are kept outside Git; this document contains paraphrased findings and evidence pointers.

## Evidence and reading conventions

Offsets below are **zero-based UTF-8 byte offsets into the named original updated file**, not offsets into an analysis wrapper. A number is an implementation constant unless explicitly described as a measured value. Times in animation and timer APIs are milliseconds. Font and layout numbers are design units that pass through scaling helpers; they are not automatically physical TV pixels.

Original updated file SHA-256 identities: `app.js` `af5dd20af5f023b4e6289f8fc87df70cddd964b3b28adc5727d819bcda3ccef4`; `SuperBrowsePage.js` `77c182bc42387e5c4c4344e9253f60f4f8ff51ce8ca1586f34fb5ba64508cc25`; `ProductPage.js` `3cf224603413db5d9be5324efa7f9c11102086e800f6f8972155747e0f6d1870`; `SearchPageFactory.js` `6248f84fa94bbb3e4ec50d047860485c82d9eee9cb460eabaac7369ae7264f04`.

Fresh REA analysis of `ProductPage.js`, `SuperBrowsePage.js` and `SearchPageFactory.js`: parent Evidence `ev_cf4d94434d7ff45c629d8a8f1934f6a44262ba7d5e3b5f80fff1f53eb86d63ae`, summary `ev_dee23d11896a5633011755aa8b96e883905f229f71b8e133e53066acbbe658ec`. All three files parsed, 76,344 AST nodes, no parse failures. Application inventory is complete within that subset; semantic coverage is unknown with 18,448 unresolved static facts. This does not establish a complete execution graph.

Fresh REA analysis of four exact webpack factories from updated `app.js`: parent `ev_6d2b67babca8676bb9ce5d712d864af7711d8d78150c340fd996dcf51ec54b95`, summary `ev_e89d32f13253ffb433a12fdc2ae53ab6dcd59b5f9058fade412940dc916c2cf8`. Babel parsed the original bundle without executing it; exact factory text was placed inside an inert analysis wrapper in a temporary directory. The factories are Artwork `62062` at `[71966,78632)`, focus/state mimic `44379` at `[298054,300002)`, ScreenReader `8824` at `[1936395,1961632)`, and List/SumTree `96184` at `[2688398,2710391)`. REA parsed all four, 14,746 AST nodes, no parse failures. Inventory is complete within the subset; semantic coverage is unknown with 3,399 unresolved static facts. Factory text digests respectively: `83eeaa5678bce755c96345bf0422c30791dcb8af64e0392d9c9087fb935a3e49`, `0680abd4300b1e46077ee9725dbab465bc21bca4fab56c2a1cf9ac67d0ecf854`, `d3c1c5c61a2fe53a15df8e71328978f233f68ca4a1785ea2eaecfe8c9baeda9f`, and `726c0458623b5b8fdc6faae9c9a136a5c2547152bbfb1eb5fd16f72d6c5ac6d4`.

REA outgoing module trace `ev_3d15d90ad1a3e561d5e72273e3e4c2d027c37b4f8bf289a84034ad4e51b452ab` follows the ScreenReader factory subset: 3 seed matches, 21 graph nodes, 20 edges, no native handoff resolved. This corroborates static module relationships, not a live speech event. Literal graph traces for `contentChanges` and `prefetch` had no graph match (`ev_a9b71fb67361175888cc1e16c511862ac0dd7d3820bae3acb3312eae747a36f5`, `ev_345e856208a8fba34181a6809daec7d3539f00aeac67521f536ac2ef34d20f56`); direct shipped-byte inspection does show those operations. Missing graph seeds cannot be treated as absence of the feature.

## Questions from the short list

### 1. Performance budgets and measurements on LG

**Not recoverable as targets or real timings from this static UI evidence.** The code contains named performance tiers, animation durations, debounces and loading timeouts. None is a key-to-screen latency budget, launch-to-usable budget, or time-to-first-video-frame measurement. The supplied UI tree does not contain a dated engineering statement of such targets for a 2022 TV. Existing native counters and runtime measurements must be described separately. An animation that lasts 300 ms does not establish 300 ms input latency.

A bounded literal scan of nonlocale, nonkeyboard, nonHLS updated JS found no `inputLatency`, `keyLatency`, `focusLatency`, `keyToFrame`, `keyToPresent`, `inputTimestamp`, `renderLatency`, `uiLatency`, `longtask`, `LongTask`, `renderBudget`, `frameBudget`, `performanceBudget`, `animationBudget` or `inputBudget`. Launch-time instrumentation is present in main/bootstrap and the welcome page. The absence of these tokens cannot rule out unnamed or native instrumentation. The shared store also computes Node event-loop utilization from deltas of active and idle time (`app.js` around byte 1327437); this is a utilization percentage, not a key-to-visible latency metric.

### 2. The single biggest improvement to responsiveness

**The mechanisms are recoverable; their causal ranking and team history are not.** The strongest code evidence is bounded list rendering with indexed size lookup, lazy widget conversion, asynchronous artwork readiness, delayed nearby data fetch, explicit focus ownership and animations that stop when complete. These make plausible causes of smooth navigation. Establishing which change made the biggest difference requires before/after measurements or developer testimony. The code does not contain an experiment result ranking them.

### 9. Scaling visual effects by device or runtime measurement

**Answered for static policy, partially answered for the live device.** `app.js` around byte 54800 computes a presentation profile from `devicePerformanceLevel()` or an explicit supplied level. The enum at byte 2021010 has LOW=0, MEDIUM=1, HIGH=2 and ULTRA=3.

| Feature family | Gate in the updated profile |
| --- | --- |
| General, focus, auto-scroll, notification and floating-menu animations; automatic hero advance; real-time player position updates | level greater than LOW |
| Menu animations | level greater than MEDIUM |
| Page transitions, image placeholder circles, richer product cards, real-time blur, tabbed-menu animation, background-video positioning and portrait video | level at least ULTRA |
| Glimmer loading surfaces | level at least HIGH |
| Immediate spinner | level below MEDIUM |
| Transparent player controls, materials and sidebar animations | enabled in this profile independently of level |

The current nonempty LG class is in updated `app.js` factory 90058 around byte 65706; its configuration constructor creates `LG`. The LG class inherits the base configuration, whose `devicePerformanceLevel()` returns ULTRA at byte 1998220. An explicit override API exists around byte 1996307. The observed method is a tier policy; it is **not** a runtime CPU/GPU benchmark or proof that this TV automatically selects a lower tier when slow. The seven old zero-byte numbered update chunks are not used as evidence of a current policy.

Accessibility is a second policy input. Focus animation is forced instant when focus animations are disabled or Reduce Motion is active (`app.js` byte 29752). Search keyboard movement is forced instant when auto-scroll animations are disabled (`SearchPageFactory.js` byte 29171). Product-page blur falls back to a solid treatment when Reduce Transparency is active or real-time blur is unsupported (`ProductPage.js` byte 80200). Some scrolling paths choose slower spring parameters under Reduce Motion rather than eliminating all motion (`SuperBrowsePage.js` byte 145280).

### 10. Making a custom-drawn UI work with Audio Guidance

**Answered for the app mechanism; current LG integration is a static inference.** The UI maintains semantic information separately from drawing: accessible label, container label, type, location/index/count, enabled state and focus relationships. Widgets supply those properties even though they are drawn through native graphics. For example, browse menus mark themselves as Button and ScreenReaderFocusable (`SuperBrowsePage.js` byte 14541); season/episode rows supply a container label from the selected season (`ProductPage.js` byte 19630); search hints announce the count and first suggestion after a 1,500 ms delay (`SearchPageFactory.js` bytes 6372 and 33000).

`ScreenReader` listens for focus, visibility, content changes, button release, page change, input change, banner dismissal and announcements. It constructs speech requests from the semantic widget state, cancels or queues speech according to announcement type, and calls native `TextToSpeech.speak`/`cancel` with speech-state callbacks. Source: `app.js` factory 8824, especially bytes 1936395–1941095, 1952229 and 1954070. Accessibility settings wire the reader's enabled state to `DeviceProperties.accessibility.screenReader` at byte 965071 and following.

There is also an optional VirtualViews bridge that serializes a semantic tree with its focused widget and language and sends `Application.control("AXUpdateTree", ...)`. It waits for scrolling/transition completion, suppresses identical tree strings, and debounces reevaluation by 500 ms (`app.js` bytes 954036–955665). However, the base `supportsVirtualViews()` is false at byte 1997106 and the current LG class does not override it. Therefore the shipped LG path appears to use the direct TTS implementation; the presence of `AXUpdateTree` does **not** establish that it is enabled on LG. No live Audio Guidance event was captured in this pass.

### 12. What the team would change if starting again

**Unrecoverable from these artifacts.** Code supplies present mechanisms, comments and compatibility branches. It cannot establish what the original team regrets, which abandoned versions failed, or what they would build today. Assigning such an answer would fabricate a developer statement. Bloob can adopt lessons from the code; those are our engineering decisions rather than the reference team's testimony.

## Performance and rendering questions

### Smooth long rows: cards, visible drawing and image decode

**Answered for the list and image scheduling mechanisms; full recycling/memory behavior remains partial.** `ListWidget` uses uniform or nonuniform sum trees. They store subtree child counts, cumulative sizes and zero-size counts, allowing child-at-offset and index-to-offset lookup without adding every preceding item each time. Its renderer finds the starting child from the viewport offset, draws forward, records the rendered range and stops after covering the requested extent and maintaining focus/navigation requirements. A base widget is converted into a full widget as that range is processed (`app.js` bytes 2706140, 2707083–2709300). The implementation may extend work beyond the visible edge to include the selected or next focusable child; it is not literally a rule of rendering only pixels already on screen.

The range record alone does not prove a fixed pool of recycled card objects or that every offscreen full widget is disposed immediately. It proves bounded draw/layout traversal and lazy realization. Episode data has a separate retention mechanism: selection triggers loading near roughly ten positions on either side, using a 500 ms delay; distant downloaded entries are replaced with placeholders beyond margins of twenty (`ProductPage.js` bytes 2450–4050). The upper retention bound includes the returned chunk length, so saying "always exactly forty episodes" would be incorrect.

New related shelves use seven initial glimmer slots and transfer into content cards after image readiness (`SuperBrowsePage.js` bytes 124513, 137900–139600). Those slots are placeholder crossfade staging, not evidence of a universal seven-card recycling pool. The related-shelf controller defaults to a maximum of 35 shelves and prevents another insertion when that count is reached (bytes 124483, 126505, 129803). It stores focused index by entity and scroll state independently of replacement widgets.

Artwork prefetch is explicit. Product-page backgrounds retain the old layer, prefetch the incoming image, treat Ready or Error as transition readiness, and run a frame animation only while layers are pending or transitioning. They cancel it after completion (`ProductPage.js` bytes 6441–7947). Related shelf images also wait for Ready or Error before a reveal cascade (`SuperBrowsePage.js` bytes 141536–142400). JS prefetch delegates to the native image object; native background decode/worker behavior requires the separate runtime evidence rather than inference from the method name alone.

### Artwork sizes and compression

**Answered as a sizing algorithm, not one fixed TV image size.** Artwork requests take width/height from each surface, infer a missing dimension from source aspect ratio, apply contain/crop policy and clamp some defaults to screen dimensions. The helper asks platform configuration for an adjusted size, then formats the image URL with dimensions, format, crop code and compression value (`app.js` factory 62062, bytes 74205–75500).

Default format is JPG. The named compression parameters are 90 for a 720-high UI and 80 otherwise (`app.js` bytes 73563 and 78531–78632). These are source quality/URL parameters; they do not mean the downloaded byte size is reduced by exactly 90% or 80%. Particular surfaces override the format, for example PNG and Precise accuracy for logos. A caller can therefore request a different format or exact resolution.

At Default accuracy the generic platform helper scales UHD 1440p/4K requests by 3/4 and 8K requests by 1/3, rounding width and height. Other screen types use scale 1. This is `app.js` byte 1998336. The returned `imageScale` preserves the intended widget geometry. Example: a 400-unit requested width on the 3/4 path asks for a 300-pixel resource while retaining logical layout scaling. These are derived algorithm examples, not measurements of a live CDN response. There is no evidence that all posters, heroes and logos share one fixed requested size.

### TV memory budget and releases while idle or playing

**Partially answered.** This UI pass establishes bounded episode metadata, a 35-shelf insertion limit, lazy widget conversion, disposal of replaced search results and content, animation cancellation, removal of related shelves, and suppression of product trailer startup when a player page is in the deck. It does not establish a global MB budget, a decoded-image cache capacity, GPU texture allocation limit, or total retained heap while playing. Bounded searches across the updated JS files for `cacheBudget`, `maxMemory`, `memoryLimit`, `imageMemory`, `ImageServer` and `trimCache` found no literal matches; that is only a literal search boundary and cannot prove native budgets are absent.

Do not convert list culling into a memory claim. The list retains child descriptions and may retain realized widgets; the native graphics system can own decoded textures beyond the JS references. Those require native allocator/cache evidence or a measured runtime snapshot.

### UI work while video plays and dropped frames

**Partially answered.** The UI remains active for its player surface and control state. Product trailer launch checks for an existing player page, idle state, visibility and position before starting (`ProductPage.js` bytes 102350–103200). Page/modal transitions can idle the page below (`app.js` bytes 1152794 and 1153127). Completed background crossfades stop their frame callbacks rather than running forever. These mechanisms reduce unrelated work but do not prove zero background UI work, a maximum UI CPU budget, or zero dropped video frames. Media decode/output and UI graphics are separately owned subsystems, and measured contention belongs in the runtime report.

## Startup questions

### Launch sequence and the part that was slowest before optimization

**The sequence is answerable from the dedicated entrypoint analysis; optimization history is not.** The current entrypoint paints a splash, resolves/mounts an update or fallback, then imports bootstrap and startup bundles. Route chunks load later. This pass verifies one additional route policy: a ProductPage chunk import is registered for prefetch at `startupFlowDone` (`app.js` bytes 827160–827505). The sequence controller dispatches registered prefetch callbacks when transitioning to that phase (`app.js` byte 1884804). This is code loading, not a promise that every product's metadata and art were fetched at launch.

No historic profile or before/after trace is present to identify which phase was formerly slowest. Loading phase marks and retry logic are not an optimization history.

### What loads for the first screen and later

**Answered for static loading separation.** Bootstrap has a defined parallel startup chunk group. Page importers are functions that request named chunks on demand; ProductPage import resolves chunk 4006 to module 86794 and is warmed after `startupFlowDone`. Product images are also loaded before some page entry transitions, with a 7,000 ms image-load timeout (`ProductPage.js` byte 80323). Additional route/data work can therefore occur after initial UI readiness. The exact first screen depends on launch state, content and flags, which this inert analysis does not execute.

## Navigation and focus questions

### Focus when content changes underneath it

**Answered for the restoration rules.** The mimic helper recursively transfers compatible container state, requiring matching constructors and orientations. It preserves horizontal scroll, then clamps the prior selected index into the new child count. A specialized helper first looks for the previously selected widget ID and selects the corresponding new widget; if the ID is gone it falls back through the ordinary index policy (`app.js` factory 44379, bytes 298054–300002). This helper is applied to Up Next style shelves at byte 366008. It avoids treating refreshed card objects as the user's location.

Product-page replacement preserves compatible shelves and canonical-detail selection (bytes 78888 onward). Related shelves record selected index and scroll into a state object and apply them to new lists (`SuperBrowsePage.js` bytes 140423–141200). Season/episode rows use cumulative season counts to map selected episode to selected season; Up and Down route between the season row and episode list. These are explicit ownership rules, not random geometric focus heuristics.

### Loading the next screen on focus, after a pause or only on selection

**Answered as several separate policies.** ProductPage code is warmed at the startup-flow phase. Product-page art is prepared during page creation/entry. Nearby episode metadata is delayed 500 ms after selection. In the new browse page, focusing a related entity emits a semantic focus event (`app.js` byte 42943), the page handles it (`SuperBrowsePage.js` byte 112207), and its related shelf schedules a fetch after a default 300 ms focus debounce (bytes 122233, 136215 and 144633). Fetch results carry a version token and are ignored when outdated or disposed. Cached shelf data can be shown immediately. This is genuine focus-triggered content loading for that feature.

There is no basis to claim every focused poster fetches its full product page. Code-chunk prefetch, artwork prefetch, related-content fetch and episode pagination are different operations with different triggers.

### Back button and exit confirmation

**Answered for implemented behavior; the designers' reason is unknown.** The root key handler first closes a profile switcher or overlay, removes a nonroot page, and can return a root page to the top before exit/menu behavior. Search has explicit keyboard/results focus routes. The root expands the menu in several Back paths instead of immediately closing the app (`app.js` bytes 1133050–1135650).

An exit confirmation exists: the Exit key invokes a dialog, the default selected action is Cancel, and Exit closes the app (`app.js` byte 1142195). The generic platform exit policy returns Close rather than Prompt at byte 1997190, and `runExitAction()` switches on that policy. Thus this build has both a confirmation route and a direct close policy; it would be wrong to say either "all Back presses prompt" or "there is never an exit prompt." Why the designers selected those paths is not recorded in this evidence.

## Design for TV questions

### Smallest text and sofa-distance testing

**Typography constants are answered; testing rationale is unknown.** The standard typography table has Title1 76/96, Title2 57/66, Title3 48/56, Headline 38/46, Callout 31/38, Body 29/36, Caption1 25/32, Caption2 23/30, and Caption3 21/28 (size/leading; `app.js` factory 3038, Caption3 anchor byte 2196578). This identifies 21 as the smallest named standard style in that table, not a universal pixel minimum for every custom text or icon. The fit-to-string helper defaults to a minimum font size of 23 at byte 2195805. Individual callers can pass another minimum.

Font loading applies screen-size scale when enabled (`app.js` byte 2194994). The build also has larger accessibility text tiers and typography-dependent formatting. No supplied code or artifact establishes a sofa-distance user test, the participating viewers, or why 21/23 were chosen. Those numbers cannot be adopted as measured readable pixel sizes on our TV without accounting for scaling and display size.

### Contrast over video and artwork

**Answered for protective rendering, not for a formal audit.** The UI uses themed foreground colors, artwork-derived backgrounds, black/solid overlays, gradients, shadow/label styles and Reduce Transparency fallbacks. A product-card style checks background luminosity and applies a stronger dark blend for light artwork (0.55 versus 0.2), with an analogous overlay selection (`app.js` byte 744812). The title page changes hero opacity, shelf position, background protection and below-fold treatment through one fold value (`ProductPage.js` byte 80588).

These are concrete contrast-preservation mechanisms. They do not establish a WCAG contrast ratio for every dynamic image, an automated contrast checker, or a human design review process. Static numeric alpha values are not contrast-ratio measurements.

### Rotating hero and accessibility

**Answered for behavior; user-test results are unknown.** The generic slideshow's automatic-advance check requires an app that is not backgrounded, controller permission, the performance profile's advance flag, and Reduce Motion off (`app.js` byte 1801489). The home-stage controller additionally requires topmost page, scroll-at-top, no disabled internal setting and screen reader off (byte 1712510). A slideshow observes screen-reader enabled state and conditionally reactivates its advance behavior when permitted (byte 1795300). Some full-screen transition/content fade paths adjust timing for reader usage. Therefore the reference pauses automatic hero progression for Reduce Motion and the home-stage reader condition. This does not show that users preferred the banner, or which accessibility users participated in testing.

### What user testing changed most in TV layouts

**Unrecoverable from the shipped implementation.** The artifact shows layouts, feature gates and alternative branches. It does not link them to dated study results, participant feedback, discarded layouts or design decisions. A reasonable Bloob design handover can cite focus preservation, protected text, coordinated density shifts, loading/empty/error states and reduced motion behavior as observed principles, while labeling our choice of those principles as inference.

## Practical consequences for Bloob

The code supports concrete work now: store focus separately from card objects; use indexed list sizes and draw/realize bounded ranges; fetch nearby data after rapid navigation settles; gate image transitions on readiness; cancel completed frame work; separate code prefetch from data prefetch; use screen capability and accessibility policy to choose effects; and attach semantics to custom-drawn widgets so speech follows the same authoritative focus state.

It does not supply guaranteed LG latency budgets, an image-cache MB allowance, a proven ranking of performance causes, historic user tests, or the team's regrets. Those remain distinct evidence requirements. Replacing those unknowns with guessed answers would make this study less useful for building Bloob.
