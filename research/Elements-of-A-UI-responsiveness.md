# Elements of A: why the UI feels responsive

This is a static study of the locally extracted version `16.2.4` and its bundled ARM runtime, compared with Bloob `0.2.2`. It excludes protected-content paths. The user's report that the reference UI feels snappy is a user observation. No remote-key-to-presented-frame timing was captured for the reference app, so the mechanisms below explain *how it is designed to avoid delay and uneven frames*, not a measured speed advantage or a proved ranking of causes.

## Evidence boundary

| Evidence | What it establishes |
| --- | --- |
| `res/bootstrap.js`, `res/app.js`, `res/ProductPage.js` | Shipped JavaScript control flow for startup, pages, artwork, animation, and episode paging. |
| `research/luna-runtime-decomp.txt`, functions `004b566c`, `004b5930`, `002e8bc8`, `002e80b4` | Focused Ghidra decompilation of frame scheduling and graphics processing. Decompiled variable names and indirect call targets can be imperfect. |
| `research/Luna-runtime-architecture.md` | Prior findings for the native-to-JavaScript input bridge, image/font workers, and render queue. |
| REA application analysis `ev_eabc8a4b1610165607c0df664863dbf5bc9c522d988c0d9ccc6dd49a3b6f3847` | Parsed 134 JavaScript files and inventoried 139 relevant files. Semantic coverage is partial on large bundles, so the narrow conclusions below were checked against shipped code. |
| `tv-app/src/` | Bloob source, sufficient for structural comparison but not proof of TV bottlenecks. |

## Responsiveness has four separate parts

1. **Input latency:** time from remote key to an updated focus or screen state.
2. **Frame pacing:** whether each visible motion step reaches the display without uneven gaps.
3. **Data readiness:** whether the next image, page code, and metadata are already available.
4. **Perceived waiting:** whether a transition presents meaningful feedback while work continues.

The reference app has mechanisms for all four. Its native renderer alone does not explain the full experience.

## Input to frame

The native `Luna::AppEngine::keyInput` queues an async event. The `Luna_Application_Wrapper` converts the event to a JavaScript value and invokes the registered callback. `res/bootstrap.js` fans `keyInput` out to the app's UI event system. The root app owns a page deck and focusable widget tree. A key can update selection in that tree without rebuilding the entire screen or fetching new content synchronously.

On the frame side, `Luna::AppEngine::internalDraw` enqueues a frame request and wakes the Node main thread. If a request is already outstanding, it accumulates elapsed time instead of adding another callback for every tick. This is a form of backpressure at the JavaScript frame boundary. `Gfx::IEngine::processFrame` checks decoded images, loaded fonts, and whether the drawing tree is dirty. It can queue a null render item when nothing requires a new tree update. Otherwise it builds clip work that `renderFrame` consumes. This separates UI tree work from rendering and gives an unchanged screen an idle path. It does **not** prove that every user-visible input made the next display refresh.

The image server and font server have asynchronous work paths. Artwork decoding and font loading therefore need not be performed in the key handler itself. The runtime records performance counters, but we did not read frame times from a live reference session.

## It starts with a small visible surface

`res/bootstrap.js` waits for its first `frame`, then schedules a group of extra chunks with `setImmediate`. `res/app.js` separately prefetches the product-page chunk for later use. Player and other pages are loaded as chunks when needed. This limits the code that must be ready for the initial visible frame while warming likely next routes. A chunk can still be cold or fail; prefetch is a strategy, not a guarantee.

The root owns `mainDeck` and pushes page widgets. When a user enters playback it can show a loading page while the player-page chunk and session are prepared. This means input gets a visible response before all playback work is finished. The loading page uses existing artwork or a solid color and delays its spinner in the ordinary path; its constructor has a shorter hold in a faster path. That is a presentation decision, not evidence that the underlying request finished sooner.

## It hides incomplete artwork transitions

`BackgroundWidget` in `res/ProductPage.js` calls `prefetch()` for incoming artwork and waits until its image state is ready or failed before beginning the crossfade. It subscribes to frame animation only while a pending or incoming layer exists and unsubscribes when the transition ends. A separate product-page entry path awaits `preloadContentImages()` before fading the page in, with a minimum loading-page display delay in one path. These choices trade a little initial waiting for fewer visible half-loaded frames and less continuous animation work.

The artwork helper in `res/app.js` computes requested dimensions from the screen and source aspect ratio, asks the platform config for an adjusted size, then formats a URL for that size and compression. This avoids treating every poster and backdrop as an arbitrary full-resolution image. `ProductPage` also checks `supportsRealTimeBlurEffects` and an accessibility transparency setting before choosing a blur-dependent presentation; it can use a solid color instead.

## It limits work near focus

`EpisodePaging` in `res/ProductPage.js` builds a list with placeholders, then requests missing episodes around the selected index. Selection can move before all episode details exist. The request is delayed 500 ms; nearby windows use roughly ten items on each side, and previously downloaded entries farther than about twenty positions from selection can be replaced with placeholders. That bounds loaded episode detail and avoids fetching on every rapid focus movement. The exact widget virtualization behavior for every other shelf is not established by this one class.

## Comparison with Bloob 0.2.2

| Area | Reference mechanism | Bloob source today | Likely effect to verify |
| --- | --- | --- | --- |
| Input and focus | App-owned widget focus; frame requests can be coalesced | One key handler and Solid signals; source focus queues `requestAnimationFrame` plus `scrollIntoView` for each change | Rapid key repeat can queue layout and scroll work. |
| List size | Episode details loaded around selection with placeholders | Home `Row` creates a DOM tile for every item; source results create DOM rows for visible groups | Browser layout, image decode, and focus styling can scale with list size. |
| Images | Dimension-aware image URLs; prefetch/ready gating | Lazy `<img>` tags, a next-hero preload, and a two-layer backdrop | Some image work is deferred, but every mounted tile still has DOM and a source URL. |
| Animation | Background animation unsubscribes when complete; blur capability fallback | Full-screen backdrop drift runs indefinitely; sources page has a 28 px full-screen blur; home scroll transition is 520 ms | Sustained compositor/GPU work and long visual settling may make input feel laggy. |
| Playback state to UI | App session state is distinct from drawing tree; idle render path | `useBunaState` JSON clones and reconciles the entire playback state at each clock update (normally 250 ms) | Repeated serialization of track arrays and store traversal can steal UI-thread time. |

These are **code-based suspects**, not measured causes of the user's current choppiness. The reference app also uses animation and delay; the difference is its deliberate gating, bounded work, and ability to stop scheduling completed animations. Bloob's HTML video route can still deliver excellent media output while the web UI thread misses frames.

## What to copy as original engineering principles

1. Show focus feedback immediately; move fetching, sorting, and decode out of the key path.
2. Keep only nearby rows and item details active. Preserve selection state separately from rendered widgets.
3. Decode or load the next backdrop before a transition, then crossfade once; stop animation when it ends.
4. Request artwork close to its display size and keep a bounded cache for likely next navigation.
5. Let an unchanged screen remain still. Avoid continuous full-screen transforms and blur on this TV unless measured affordable.
6. Coalesce repeated key and scroll work. A later focus position supersedes an earlier pending scroll.
7. Make the player view update only fields that changed; do not serialize a whole session at clock frequency.
8. Use a brief transition or skeleton to acknowledge navigation while data loads, but measure actual latency separately from presentation.

## Highest-value Bloob experiments

These can be checked on the current TV with one representative navigation session, without a broad media test matrix:

1. Instrument `keydown` to next painted focus with `requestAnimationFrame`, counting missed frames and long tasks while moving through Home and Sources.
2. Compare that trace with full-screen drift and source blur disabled, then with source scrolling coalesced. Keep each change only if frame pacing improves.
3. Repeat while a movie plays, then compare the player HUD with and without full-state JSON cloning at clock frequency.

No single static finding supplies a reference-app FPS, remote-input latency, GPU utilization, or proof that its graphics path is always superior to WebKit. A synchronized live key/render trace on the reference app and a Bloob trace on the same TV would rank the mechanisms more confidently. The present evidence is enough to prioritize Bloob's obvious work, but not to claim a 1:1 performance result.
