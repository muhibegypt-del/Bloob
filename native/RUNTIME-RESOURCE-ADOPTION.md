# Adopting the Elements of A runtime resource findings in Bloob

**Status: implemented in 2.4.0 (16 October 2026), locally verified, not yet run on the TV.** Section 5 lists each finding's state.

**Basis:**
- The 16 October 2026 resource study, in five documents:
  - [CPU, memory and GPU map](../research/Elements-of-A-CPU-memory-GPU-map.md)
  - [CPU scheduling](../research/cpu-scheduling-deep-findings.md)
  - [Memory and resources](../research/memory-resource-deep-findings.md)
  - [GPU rendering](../research/gpu-rendering-deep-findings.md)
  - [Assets and observability](../research/assets-and-observability-deep-findings.md)
- Bloob Native 2.3.0, read file by file at `4d6d903`.

**Rules:**
- **Labels for the reference:** each Elements of A row is labelled *observed* (static evidence in that study) or *inferred*. None of it is a TV measurement.
- **Bloob citations:** every Bloob row cites the current code.
- **What we adopt:** "Adopt" means an original Bloob design built from the mechanism the study describes. It never means copying code; the raw decompiler exports stay outside Git.
- **Numbers:** the study's values (50 MiB, 301, 11, 3, 2,048, 0x32) are the reference's own defaults. Bloob takes the *mechanism* and sets its own values from TV measurement. Each place a reference value is a sensible starting point says so.

**Bloob's architecture in one paragraph.** One UI thread does everything: input, state, layout, drawing and texture upload. It draws immediate-mode through the TV's `SDL_Renderer` into a 1920×1080 target texture (`gfx.cpp`), then copies that texture to the window. Two API workers and three image workers (`main.cpp:116–117`) do blocking work. Each completion pushes one SDL wake event (`tasks.cpp:53`, `luna.cpp:60`), and every wake marks the frame dirty (`main.cpp:751`, `:754`). The window presents with vsync (`main.cpp:89`). Artwork is fetched, decoded and scaled to its drawn size on a worker, then uploaded on the UI thread. It is kept in one map with a 192 MB budget, enforced after the fact (`images.h:88`, `images.cpp:469–490`).

## 1. Every finding, mapped

Increment IDs refer to section 2. **N/A** means no Bloob counterpart exists today, with the reason given.

### 1.1 CPU, events and frames

| # | Elements of A mechanism | Bloob today | Adopt | Inc. |
| --- | --- | --- | --- | --- |
| C1 | **At most one outstanding frame request.** Elapsed time accumulates while one is pending, and the last completed frame record is reused (`internalDraw` `0x004b566c`, `waitNextFrame` `0x004b2c3c`; observed) | Single thread, so at most one frame by construction. But `render()` runs whenever `dirty_` is set (`main.cpp:794`). A burst of 30 wakes between two vsyncs is one render, but each burst that crosses a vsync is another full frame | Render at most once per display refresh while dirty. Add a frame-due test against the last present time, so wakes inside the refresh interval fold into the next frame | R5a |
| C2 | **Wakeups carry notifications; queues carry work.** One token queue plus a wake; the receiver drains everything (`doCallback` `0x004c05a0`, `asyncCallbackMain` `0x004c0004`; observed) | Payload queues exist (`Tasks::done_`, `Luna::queue_`), but every item also pushes its own SDL event (`main.cpp:111–115`). A busy image pool or a 200 ms media clock floods the SDL queue with wakes | Coalesce: an atomic `wake_pending` flag. The first producer pushes the SDL event; the loop clears the flag before draining. Wakes no longer set `dirty_` (F10) | R5a |
| C3 | **Listener snapshot.** Callbacks run from a copied list, outside the lock (`asyncCallbackMain`; observed) | `Luna::pump` copies one callback per item under the lock and runs it outside (`luna.cpp:100–125`): the same idea, already done | Keep. Document it as the rule for any new event source | — |
| C4 | **Shutdown order.** Request the stop, then join; queued callbacks are discarded explicitly; completion semaphores (`stopNode` `0x004b23f8`, `NodeJsMT::stop` `0x004bfa4c`; observed) | `Tasks::stop` clears the queue, then joins (`tasks.cpp:8–18`), silently dropping queued work, including a final progress report (F01) | R1 already makes final reports app-owned. Add: `Tasks::stop(Drain)`, which runs or drops each queued job by an explicit per-job policy (`must_run`, `may_drop`) and logs how many were dropped | R1 (+R5a) |
| C5 | **Event-processing budget per wake, with reschedule.** `processAsyncEvents(0x32)`; return value 2 wakes the loop again (`asyncCallbackWorker` `0x004bfc3c`; observed, units unresolved) | `Tasks::pump` and `Luna::pump` drain everything in one call (`tasks.cpp:57–67`) | A time budget per pump (start at 4 ms), then re-arm the wake for the rest (already in R5) | R5a |
| C6 | **Frame pacing.** A three-interval history, a smoothed frame time with a floor, the delay to the next frame capped at two intervals (`draw` `0x004b5930`, `getFrameTime` `0x004b2888`, `calculateNextFrameTimeWait` `0x004b28c4`; observed, constants partly unresolved) | Animation uses the raw elapsed time, `c.dt = min(0.1, now − last)` (`main.cpp:762`). One late frame makes every running animation jump; vsync alone sets pace | Animation time is the median of the last three frame intervals, floored at one refresh period, and resynchronised after an idle gap. The refresh rate comes from `SDL_GetCurrentDisplayMode` (already logged at `main.cpp:98`) | R5a |
| C7 | **Timed callback registry in the draw loop**, expired entries removed (`internalDraw` tail; observed, semantics unresolved) | `Ctx::redraw_at(t)` keeps one earliest deadline (`ui.h:136`) | Keep a single deadline, but record which owner asked for it (for the inspector) so a stuck redraw can be traced | R5d |
| C8 | **Memory-saving policy fanned out to every runtime, inherited by new ones, applied asynchronously** (`enableMemorySavingsMode` `0x004bfb10`; observed; the actual GC action is unresolved) | No memory-pressure handling, in the app or in the service | A `MemoryPolicy` {normal, saving} in `Ctx`. Saving lowers the image budget, drops the frost cache for hidden screens, trims the service's 40 MB image byte cache (`images.js:13`) through `/api/memory`, and is applied to new screens as they open. Triggers: background, playback, and a webOS low-memory signal *if* a public one exists. Check webOS OSE `com.webos.memorymanager` read-only before relying on it | R5b |
| C9 | **Heap statistics are cached samples with an age.** Aggregation reads the cache and asks for a refresh, without a barrier. Process memory (RSS) and swap are read on a separate counter loop (`getHeapStatistics` `0x004bfd26`, `PerformanceCounters::run` `0x004b9680`; observed) | Only texture bytes are known (`images.h:63`). Nothing reads process RSS, the service's memory, or worker queue depth | The inspector samples `/proc/self/status` (VmRSS, VmHWM, VmSwap) once per second off the UI thread, plus the service's `process.memoryUsage()` through `/api/health`. Each value carries its timestamp | R5d |
| C10 | **Throttled diagnostics.** A ten-second log throttle in the wait path (observed) | `logf` is unthrottled. A failing image host logs once per image | A per-key rate limit on repeated lines | R5d |
| C11 | **Millisecond monotonic clock with wrap-aware differences** (`cw_getTicks` `0x00105a94`; observed) | `SDL_GetTicks` is 32-bit and wraps after 49.7 days; differences are taken as `long long` of an unsigned value | Use `SDL_GetTicks64` where the TV's SDL has it (2.0.18 and later), else a wrap-extended 64-bit wrapper. One `now_ms()` everywhere | R5a |
| C12 | **Named threads with per-role stack sizes; no priority changes** (`startThread` `0x00104f64`, `cw_threadCreate` `0x00105b9c`; observed) | Unnamed `std::thread`s with the default stack (8 MB each on glibc: 5 workers plus Luna's GLib thread) | Name the threads (`pthread_setname_np`: `bl-api-1`, `bl-img-1` …) so `/proc` sampling can tell them apart. Set worker stacks explicitly; stb decoding needs little stack, so start at 256 KB and verify. No priority changes, as in the reference | R5b |

### 1.2 Memory and resource ownership

| # | Elements of A mechanism | Bloob today | Adopt | Inc. |
| --- | --- | --- | --- | --- |
| M1 | **Small-object arena:** 4 KiB blocks, 8-byte rounding, reuse after reset, grow the last allocation in place (`Pool::relocate` `0x0029a98c`; observed) | Per-frame `std::vector`/`std::string` churn in drawing (`wrap()`, `join()`, `order` in `browse.cpp:664`) | Per-frame scratch storage: a reset-per-frame arena for layout strings and vectors in hot draw paths. Only after the inspector shows allocator time; measure first. Media use only if R7 is built | R5b (measured) / R7 |
| M2 | **Shared, reference-counted media buffers** with an accounting owner that asserts zero bytes outstanding at teardown (`ContiguousMemory` `0x000f8bf0`, `~Allocator` `0x000f8a58`; observed) | Not applicable to the URI route. The same *pattern* applies to decoded pixels: `Pixels` are moved through a `shared_ptr<Result>` (`images.cpp:405–466`) and nobody counts them | A `MemoryDomain` counter per resource class (decoded pixels, textures, glyph atlases, shape textures, render targets) with RAII guards. In debug builds, assert each domain is zero at shutdown | R5b |
| M3 | **Tracked graphics budget:** default 50 MiB; a reservation for display surfaces; a setter that refuses too small a budget; `ensureMaxSurfaceMemoryUsage` reclaims down to it (`IEngine::IEngine` `0x002e7ba8`, `setMaxSurfaceMemory` `0x002e72e4`; observed) | 192 MB image budget, and only for artwork. The canvas and layer (2 × 8.3 MB), glyph atlases, shape textures (`gfx.cpp` maps), icons and frosts outside the image map are not counted | One `GpuBudget` for every texture Bloob creates. Reserve the fixed surfaces first (canvas, layer, video-overlay needs). A device-profile value: start at 96 MB and tune from the inspector; 50 MiB is the reference's default, not a measured LG figure. Lowering it reclaims down to it, as in the reference | R5b |
| M4 | **Admission before allocation.** Check usage plus the estimate against the budget; allocate outside the lock; on failure, reclaim one resource and retry; then drain once, age the cache, retry, and fail gracefully (`loadTexture` `0x003077e0`, `initRenderTarget` `0x00307b70`; observed) | `SDL_CreateTexture` is called unconditionally in the image completion (`images.cpp:445`); eviction runs at `end_frame` only when already over budget (`images.cpp:472`). A failed `SDL_CreateTexture` marks the image Failed and retries after 30 s | `GpuBudget::admit(bytes)` before any texture creation. It evicts until the new texture fits; if nothing can be evicted, the request is deferred (kept as decoded pixels for one frame, then dropped and re-requested), never failed. A failed create counts as allocation failure: reclaim and retry once | R5b |
| M5 | **Hash-keyed resource registry with client reference counts.** Zero references go to an unused FIFO per priority (four priorities); reacquiring reuses the resource (`acquireScratchpad` `0x003089d8`, `releaseScratchpad` `0x00306984`; observed) | Images are keyed by string (`key_for`, `images.cpp`), with no reference counts: "used this frame" (`used_at`) is the only liveness signal. Shape caches (`discs_`, `rings_`, `shadows_`, `sheens_`, `triangles_`, `Icons::cache_`) are keyed maps that grow forever | One `ResourceCache<K>` used by images, shapes and icons: key, bytes, priority (0 pinned, 1 visible, 2 nearby, 3 prefetch), last-use frame. Visible draws stamp the frame; screens can pin (hero and frost while a screen is on the stack). Zero-pin, unused resources go to per-priority FIFOs | R5b |
| M6 | **Object lifetime separate from GPU-handle lifetime.** A resident bit; `releaseHandles` drops backing but keeps the registry for recreation (`releaseHandles` `0x00306568`; observed) | A texture *is* the entry: evicting erases the key (`images.cpp:482`). Bloob forgets it ever decoded the image and starts over, including the network fetch | Entries keep their key, size and an optional compressed-bytes or CPU-pixel cache handle. Evicting drops only the `SDL_Texture`. Re-residency first checks the service's disk cache (already there, `images.js`) and skips the fade if the image was ready before | R5b |
| M7 | **Eviction by priority and age** in frame-counter units, with a minimum age (default 3); one eviction per call; priority-3 age weighted ×100 (`freeUnusedMemory` `0x003073b8`; observed) | Least recently used by wall clock, older than 1 s (`images.cpp:475`), in bulk down to 75% | Evict by priority, then frame age, starting with prefetch (3), then nearby (2), never pinned (0). Minimum age of 3 *rendered frames* (the reference default, a good start), so a texture drawn last frame is never evicted. One eviction per admission attempt (no bulk sweeps that cause upload storms) | R5b |
| M8 | **Frame-end cleanup:** old unused buffers destroyed; one bucket pruned above 300 objects when its head is 11 or more frames old (`endFrame` `0x00308218`; observed) | Bookkeeping of failed or idle entries is pruned only above 2,000 entries, after 60 s (`images.cpp:486–490`). Shape caches never shrink | `ResourceCache::end_frame()`: bounded work per frame; prune unused shape and sheen textures (focus sizes change, so `sheens_` accumulates one per card size); cap entry count per class | R5b |
| M9 | **Render-target donor reuse:** an unused target of the same size gives its texture to a new request instead of allocating (`initRenderTarget` `0x00307b70`; observed) | Exactly two targets: canvas and layer (`gfx.cpp:426`, `:738`). Transitions use the layer | When R6r adds cached layers, a pool of same-size targets lends its backing (1920×1080 especially) | R6r |
| M10 | **Format- and backend-aware size estimates:** RGBA 4 B/px, one-byte formats, two-plane YUV, rounding for tiling (`calculateMemorySize` `0x004ad820`; observed) | `bytes = pixels.data.size()` (`images.cpp:458`): exact CPU size, no allowance for driver tiling or padding | Estimate GPU bytes as `w*h*4`, rounded up to 16-pixel tiles (a conservative default); alpha-only shape textures as `w*h*4` too, because SDL uploads ARGB. Calibrate on the TV by comparing estimated total with the GPU or ION memory delta, if readable from `/proc` or debugfs, read-only | R5b |
| M11 | **Surface dimension cap:** the smallest power of two at least the display size, minimum 2,048 (observed) | Glyph atlases respect `max_texture_width/height` (`gfx.cpp:134–138`); image textures assume they fit | Clamp image and target sizes to the renderer's maximum; log any clamp | R5b |
| M12 | **Prune notifications:** subscribers learn that a keyed resource was evicted (`destroyScratchPad` `0x003071dc`; observed) | Screens poll `images.get()` every frame, so they notice on their own; but `settled()` waits (transitions) can deadlock-wait on an evicted image | Eviction never takes a pinned resource, and `settled()` treats "evicted" as "re-requested". Each screen's pins are its contract | R5b |
| M13 | **Suspend releases every GPU handle, programs included; objects are kept for lazy recreation** (`IEngine::suspend` `0x002e7384`; observed) | `App::background()` only notifies the top screen (`main.cpp:522–526`). All textures stay resident while hidden, including during another app | On background: release image textures, shape caches, frost, sheen and the layer target (keep the canvas, since the video window may still show through). Keep every registry entry. Foreground re-uploads lazily, visible first. Measured target: RSS and GPU memory drop while hidden | R5b |

### 1.3 Rendering

| # | Elements of A mechanism | Bloob today | Adopt | Inc. |
| --- | --- | --- | --- | --- |
| G1 | **Scene preparation, rendering and finalising are separate**, with a pool of three reusable preparation records and a null entry meaning "no update" (`processFrame` `0x002e8bc8`, `renderFrame` `0x002e80b4`; observed) | Prepare and draw are one pass (`Screen::draw()`) | Split each screen's draw into `layout()` (cached, invalidated by state) and `paint()`. Begin with Home's shelf geometry and text runs, the hottest path. No thread split yet | R6r |
| G2 | **Retained last scene plus a dynamic-content update.** If only textures changed, the plan is replayed, not rebuilt; if nothing changed, no render (`renderFrame`, `updateDynamicContent` `0x00309614`; observed) | Every frame rebuilds and redraws everything | R5a: no render without an invalidation. R6r: per-screen cached layers. When only a dynamic element (the clock, the timeline, a fading image) changes, repaint that element over the cached layer, not the screen | R5a, R6r |
| G3 | **Per-frame geometry in a pool restored to a saved state** (`ClipRender::renderFrame` `0x0030acc8`; observed) | Per-frame temporaries are heap-allocated | Same as M1: a per-frame arena if measured | R5b |
| G4 | **Offscreen intermediates keyed by content hash and size; a valid one is not redrawn; drawn in dependency order** (`makeTemp` `0x0030ab00`, `drawTarget` `0x0030a3e0`; observed) | The transition layer is redrawn every frame (`main.cpp:572–580`). During a push, the screen underneath is fully redrawn every frame as well (`main.cpp:592`), so a transition costs two full screens per frame | **Cached layers with validity keys.** For a transition, paint the outgoing screen once into a target, then composite it with alpha while it fades; repaint only if its key (screen generation plus image-ready count) changes. For the player, HUD-hidden frames have nothing to draw (G2). The same mechanism covers the glass bar, Home shelves under the hero and the settings list | R6r |
| G5 | **Skip an intermediate when it is redundant** (a single opaque quad covering the target) (`isTempRedundant` `0x0030e584`; observed) | Not applicable | Layer caching applies only when a layer holds more than one draw and is reused for at least 2 frames; otherwise draw directly | R6r |
| G6 | **Sorting and batching:** opaque first, then blended; adjacent draws with the same blend and shader share one draw call; a 65,536-vertex buffer (`OpenGlRenderQueue::process` `0x004acbe0`, `compareState` `0x0030e0f0`; observed) | `SDL_Renderer` decides. SDL 2.0.10 and later batch internally (`SDL_HINT_RENDER_BATCHING`), but interleaving textures (glyph atlas → disc → atlas → image) breaks batches. Bloob draws card by card (shadow, image, text, progress), maximising texture switches | Log `SDL_GetVersion` and the renderer name (version not logged today). Set `SDL_HINT_RENDER_BATCHING=1`. Reorder hot loops by texture where the visual order allows: all card shadows, then all card images, then all card text (one atlas), then the focused card on top. Measure frame time before and after on the TV | R5a |
| G7 | **Change state only when it differs; programs cached and created lazily** (`setState` `0x004ac954`, `enableProgram` `0x004adc80`; observed) | `SDL_SetRenderDrawBlendMode` and colour calls are made per primitive (`gfx.cpp`) | Cache the blend mode and draw colour in `Gfx` and skip redundant SDL calls (cheap, local) | R5a |
| G8 | **Separate mesh path with static vertex buffers** (`drawMeshObject` `0x004aca0c`; observed) | No 3D | **N/A**: no mesh content | — |
| G9 | **Textures allocated lazily and uploaded per format** (RGBA, alpha, two-plane YUV); an external OES path (`OpenGlScratchpad::upload` `0x004ad0ec`; observed) | Every upload is RGBA8. Shape textures (discs, rings, shadows) are alpha masks stored as ARGB, so 4 B/px | Alpha-only shapes cannot use 1 B/px through `SDL_Renderer` (no A8 format). Accept this, and count them (M3). YUV and OES belong to the sample route only (R7) | R5b / R7 |
| G10 | **GPU effect programs:** blur, glass, shadow, rounding, mask, colour (`ShaderCompiler` program families; observed) | Effects are baked on the CPU: frost (`frost_image`), blur, shadows as 9-slice textures, rounded corners baked into images. Correct and cheap at runtime, but each new glass rectangle or size needs a worker bake, and frost appears late (the transition waits on `settled`) | Keep CPU baking (SDL2's renderer has no custom shaders). Bake frosts *ahead*: when a screen opens, request the frosts its first layout needs at the same time as the backdrop. A GPU blur needs a GLES2 path. That is a renderer decision, recorded under R6r's "conditional" note and justified only if frost latency shows in measurements | R5c / R6r |
| G11 | **Framebuffer reuse with saved host bindings; depth only where needed** (`beginRendering` `0x004ac6dc`; observed) | Two persistent targets; no depth | Already lean. Keep | — |
| G12 | **GPU wait (`glFinish`) only under allocation pressure, then retry** (`drainPipeline` `0x004ac810`; observed) | Not applicable through `SDL_Renderer`, which hides `glFinish`. The equivalent risk is that `SDL_CreateTexture` and `SDL_UpdateTexture` inside completions can stall the frame | Upload pacing: at most N uploads (start at 2) or M bytes (start at 16 MB) per frame, highest priority first. The rest wait as decoded pixels. Deferral counts in the inspector | R5c |
| G13 | **Suspend releases programs and textures; recreated lazily** (observed) | See M13 | See M13 | R5b |
| G14 | **A separate secondary composition layer** (a fourth record, finalised after the main scene) (`finalizeFrame` `0x002e7080`; observed) | The tab bar and toast are drawn after the screen in the same pass (`main.cpp:600–623`) | Treat the shell (tab bar, toast, focus speech overlay) as its own cached layer with its own invalidation. Toasts and the tab bar then don't invalidate the page | R6r |
| G15 | **Per-frame intermediate memory accounting**, a trace when over budget (diagnostic only) (`updateMemoryUsage` `0x00308d8c`; observed) | None | Per-frame counters: uploads, upload bytes, render-target switches, draw calls if obtainable, evictions. Shown in the inspector and `[perf]` | R5d |

### 1.4 Artwork pipeline

| # | Elements of A mechanism | Bloob today | Adopt | Inc. |
| --- | --- | --- | --- | --- |
| A1 | **A coordinator plus decoder worker(s)**, with fetch and decode as separate stages (`ImageProcessor::run` `0x00322290`, `ImageDecoder` `0x002fa4cc`; observed) | Each image task fetches over HTTP *then* decodes on the same worker (`images.cpp:418–421`). Three workers blocked on network can't decode, and one large decode delays fetches | Two stages: **fetch** on the API pool, or a small fetch pool (I/O bound, more in flight); **decode and scale** on the image pool (CPU bound, sized to cores minus the UI thread). A per-image state machine: `wanted → fetching → fetched → decoding → decoded → resident`, plus `failed` and `cancelled` | R5c |
| A2 | **A relevance gate before decode** (`handleClipDistance` `0x0031c89c` returns when distance > 999; observed, units unknown) | Prefetch is positional: two cards past the visible range (`browse.cpp:658–661`), the logo of the shown hero (`:262`). Everything requested is decoded once its turn comes | Each request carries a **relevance**: visible (0); next screen-width either way, or the next focus target (1); prefetch (2). Decode admission takes relevance first, then recency. The decoded-pixel budget (M2) bounds prefetch, so prefetch can't push visible work out | R5c |
| A3 | **A request stamps use and reuses an existing task**, never decoding per draw (`ImageServer::decode` `0x00325590`; observed) | Already: `lookup` stamps `used_at` and `wanted`, and only Idle starts a load (`images.cpp:365–396`). The same image at a different size is a new key and a new decode, so the same backdrop is decoded twice for sharp and blurred, and again per frost rect | One decode per *source* URL per window: cache the decoded source pixels briefly (a CPU budget, M2), and derive the sharp, blurred and frost variants from them on the image pool. Removes repeated network and stb decodes for the hero and its glass | R5c |
| A4 | **Loading vs decoding states; two ready lists with preference; a suspended flag stops new loads; cancellation checked before decode** (`updateLoading` `0x00321974`, `updateDecoding` `0x00320ea4`; observed) | Staleness is checked once, before the fetch (`images.cpp:414`). After a slow fetch the image is decoded even if nobody wants it any more. No suspended state: loads continue in the background | Check `wanted` again after the fetch and before the decode. Two ready queues (relevance 0–1, then 2). Suspended while backgrounded or in saving policy: no new fetches; resume in relevance order | R5c |
| A5 | **Prefetch is owned and reused** (a stored proxy) (`prefetch` `0x00326a6c`; observed) | `prefetch()` is `get()` (`images.h:51–53`): same path, no separate owner or budget | A prefetch handle per screen; the screen's teardown releases its prefetches (relevance drops to none) | R5c |
| A6 | **Frame-end retirement:** stale tasks released by last-use frame marker; the marker advances each frame (`frameDone` `0x00326fec`; observed) | Wall-clock staleness (2.5 s) is checked only at the worker (`kStaleMs`, `images.cpp:29`) | At `end_frame`, retire queued requests whose last-use frame is more than K frames old (start K = 30) by flipping their shared `cancelled` flag; the workers check it at each stage | R5c |
| A7 | **Animated images updated per frame** (`frameBegin` `0x00329528`; observed) | No animated artwork | **N/A** today. If animated previews come, they use this path | — |
| A8 | **Reference-counted image data**; cache records kept conditionally (`releaseImageData` `0x003296bc`; observed) | Not applicable (single owner) | Covered by M5 and M6 (pins and registry) | R5b |
| A9 | **Image counters:** states, active proxies, unique data, decoder memory (`getPerformanceCounters` `0x0032b324`; observed) | `images.bytes()` only | The inspector shows counts per state, resident bytes per priority, decoded-pixel bytes, queue depth per stage, cancellations and evictions | R5d |

### 1.5 Observability

| # | Elements of A mechanism | Bloob today | Adopt | Inc. |
| --- | --- | --- | --- | --- |
| O1 | **A performance-counter loop:** peak and current process memory, swap; frame-stage timestamps; JSON stats (`PerformanceCounters::run` `0x004b9680`, `updateStats` `0x004b9bc0`, `getJsonStats` `0x004b9cf4`; observed) | `[perf]` covers key→submit and render time per 5 s window (`perf.h`); `[launch]` marks | The inspector file (R5 in the reliability plan) gains a `resources` block, `frames` stage timings (update, layout, paint, upload, submit) and `memory`. All JSON, no URLs | R5d |
| O2 | **Metric names by milestone** (submit is not GPU completion, not presentation) (map §9.8) | Already honest for key→present (`perf.h:3`); "first frame" is not (A2 in the validation) | Names: `key_to_submit`, `submit_ms`, `upload_ms`, `started` / `first_frame` (R3) | R5d |
| O3 | **Separate ownership domains:** JavaScript heaps, shared buffers, RSS, graphics (assets doc §6) | One number | Domains: app RSS, service RSS and heap, GPU estimate by class, decoded CPU pixels, service image byte cache, queues | R5d |

## 2. Increments

They fit into [RELIABILITY-IMPLEMENTATION-PLAN.md](RELIABILITY-IMPLEMENTATION-PLAN.md) after R4. They replace its single R5, and R6r is added ahead of the other R6 work.

**Order:**
1. **R5d first.** Measure before changing behaviour.
2. **R5a**, frames and events.
3. **R5b**, resources and memory.
4. **R5c**, the artwork pipeline.
5. **R6r**, retained layers.

Each increment changes behaviour behind one switch, so a TV comparison is before against after on the same build.

### R5d: runtime inspector and resource counters

- **Findings:** C7, C9, C10, O1–O3, A9, G15; validation A2.
- **Purpose:** show the TV's real CPU, memory and frame behaviour before tuning anything.
- **Invariant:** every metric names what it measures, carries a timestamp, and contains no URL, key or release name.
- **Scope:**
  - A new `native/src/inspect.*`, already planned in the reliability plan's R5, now with these blocks:
    - `frames`: update, layout, paint, upload and submit, as p50/p95 per window;
    - `wakes`: counts by source;
    - `memory`: VmRSS, VmHWM and VmSwap from `/proc/self/status`, sampled once per second on a worker;
    - `gpu`: estimated bytes per class;
    - `images`: counts per state, bytes, queues, cancellations, evictions, deferred uploads;
    - `service`: `process.memoryUsage()` and image cache bytes, via an extension of `/api/health`.
  - `perf.h`: split `render_ms` into stages.
  - `log.cpp`: a per-key rate limit.
  - `main.cpp`: log `SDL_GetVersion`, the renderer name and flags, the maximum texture size and the refresh rate once at start.
- **State:** counters only. The file is written at most once a second, atomically.
- **Ownership:** `App` owns the inspector. Samplers run on a worker, and their results are applied on the UI thread.
- **Service:** `/api/health` adds `memory: {rss, heapUsed, external}` and `imageCacheBytes`. Additive.
- **Recovery:** if the inspector fails, it logs once and turns itself off.
- **Verification:**
  1. Desktop: JSON schema test.
  2. A grep test: no `http`, `apikey` or release text.
  3. The counters match a scripted tour's known events, e.g. 10 images gives 10 resident.
- **TV:** one pass of Home, a title, playback and Home again, read with `tv-inspect.sh` (read-only). That gives the baseline for R5a–R6r.
- **Delivery:** first, alone. No behaviour change.
- **Completion:** implemented / locally verified / TV baseline recorded in `research/` with date and limits.

### R5a: frames and events

- **Findings:** C1, C2, C5, C6, C11, G2 (invalidation part), G6, G7; validation F10.
- **Invariants:**
  1. A wake drains events; only a visible change, an animation or a deadline paints.
  2. At most one paint per refresh interval.
  3. Animation time is smooth.
  4. Work per pump is bounded.
- **Scope:**
  - `main.cpp`:
    - an atomic `wake_pending_` in `push_wake`;
    - the wake no longer sets `dirty_`;
    - `Ctx::invalidate(cause)`;
    - a frame-due check (`now − last_present ≥ refresh − 2 ms`, or an input event);
    - `dt` from the median of three intervals, floored at one refresh and reset after idle;
    - `now_ms()` 64-bit.
  - `tasks.cpp`, `luna.cpp`: `pump(budget_ms)` returns `more`, and the loop re-arms the wake.
  - `gfx.cpp`: cached blend mode and colour; `SDL_HINT_RENDER_BATCHING`.
  - `browse.cpp`, `ui.cpp`: card drawing in texture-grouped passes (shadows, images, text, then the focused card), keeping the focused card on top.
  - `player.cpp`: `Time` events invalidate only if a time is visible.
  - The `BLOOB_WAKE_REDRAW=1` escape hatch, as already planned.
- **State:** dirty causes as a bitset; last present time; three-interval ring; wake flag.
- **Ownership:** the App loop.
- **Service:** none.
- **Recovery:** a missed invalidation shows as stale UI. The flag restores the old behaviour, and the inspector's frame causes locate it.
- **Verification:**
  1. A unit test of the frame-due and `dt` filter with synthetic timestamps.
  2. A desktop test: HUD hidden for 30 s gives no paints from `Time`, and wakes are counted.
  3. A desktop test: 100 completions in one burst give no more than one paint per 16 ms.
  4. A snapshot diff of Home before and after the card reorder shows identical pixels.
- **TV:** paints per second with the HUD hidden (expected near 0, against 5–10 today, since the reference clock is 200 ms), Home frame p95 before and after batching, and animation jitter seen at viewing distance.
- **Completion:** three stages, as above.

### R5b: resource ownership and memory

- **Findings:** C8, C12, M1–M8, M10–M13, G9, G13, A8.
- **Invariants:**
  1. Every GPU texture is admitted against one budget before creation.
  2. Eviction is by priority and frame age, never pinned or recently drawn.
  3. Objects outlive their GPU handles.
  4. Background releases handles.
  5. Every memory domain is counted.
- **Scope:**
  - A new `native/src/resources.*`:
    - `GpuBudget` {admit, release, reclaim};
    - `ResourceCache<K>` {acquire, pin, unpin, touch(frame), end_frame, release_handles};
    - `MemoryDomain` counters;
    - `MemoryPolicy`.
  - `images.*`: entries move onto it; textures become handles; eviction is priority/age; a re-residency path.
  - `gfx.*`, `icons.*`: shape, sheen, ring, shadow and icon textures go through the cache, with priority 2 and pruned at frame end. The canvas and layer are reserved.
  - `main.cpp`:
    - `background()` and `foreground()` call `release_handles` and lazy restore;
    - policy changes on background and playback (HUD hidden);
    - named threads and explicit stacks via `pthread_attr` in a small `Thread` wrapper (`std::thread` cannot set a stack size).
  - The service: `POST /api/memory {policy}` trims the image byte cache.
  - Starting values, each marked to tune:
    - GPU budget 96 MB;
    - playback budget 48 MB;
    - minimum age 3 frames;
    - shape-cache cap 256 entries;
    - worker stack 256 KB.
- **State:** per entry {key, bytes, priority, pins, last_frame, resident}; budget {used, reserved, limit}; policy {normal, saving}.
- **Ownership:**
  - Screens own their pins and release them in their destructors.
  - The cache owns the textures.
  - `App` owns the policy.
- **Service:** the additive `/api/memory`. No schema change for stored data.
- **Recovery:**
  - Admission failure defers and retries next frame, and never shows a broken image.
  - A failed `SDL_CreateTexture` is reclaimed and retried once, then deferred.
  - Foreground re-upload happens in visible order, without a fade for images already seen.
- **Verification:**
  1. Unit tests (no SDL, a fake texture factory):
     - admission at the limit evicts priority 3 before 2 and never pinned;
     - minimum age is respected;
     - background release keeps entries, and restore re-admits;
     - domain counters reach zero at teardown.
  2. Desktop: a scripted Home → title → player → Home loop 20 times keeps the GPU estimate flat.
- **TV:**
  - VmRSS while backgrounded during another app, before and after;
  - GPU estimate against any readable driver total;
  - no visible pop-in regressions at viewing distance.
- **Completion:** three stages.

### R5c: the artwork pipeline

- **Findings:** A1–A6, G10 (bake-ahead), G12.
- **Invariants:**
  1. Fetch and decode are separate stages.
  2. Visible work is never queued behind prefetch.
  3. A source is decoded once per window for all its variants.
  4. Stale work is cancelled at each stage boundary.
  5. Uploads are paced.
- **Scope:**
  - `images.*`:
    - the state machine;
    - relevance on every request;
    - a fetch stage on its own small pool (I/O; start at 4 in flight);
    - a decode stage on the image pool;
    - a decoded-source cache under a CPU budget (start at 48 MB) from which sharp, blurred and frost variants are derived;
    - frame-end retirement (K = 30 frames);
    - a second stale check after the fetch;
    - per-screen prefetch handles;
    - suspended state;
    - upload pacing (start at 2 uploads or 16 MB per frame);
    - frost requests issued with the backdrop when a screen opens.
  - `browse.cpp`, `details.cpp`, `common.cpp`: pass relevance; ask for frosts at open.
- **State:** as above, plus a shared `cancelled` flag per request.
- **Ownership:**
  - The image system owns the tasks.
  - Screens own prefetch handles.
  - Workers own nothing past the completion.
- **Service:** none. The service's disk cache already serves re-fetches.
- **Recovery:**
  - A cancelled request is simply re-requested if needed later.
  - A fetch failure keeps the existing 30 s retry.
- **Verification:**
  1. Unit tests: relevance ordering; cancellation at each stage; one source decode yields its variants.
  2. Desktop: scrolling a 40-card shelf fast shows cancelled counts and no stale decodes after the scroll stops, and the glass appears together with its backdrop.
- **TV:** time from focus to sharp card art (p50/p95), hero-to-glass delay, and frame p95 during scroll before and after.
- **Completion:** three stages.

### R6r: retained layers (cached intermediates)

- **Findings:** G1, G2 (retained part), G4, G5, G14, M9; G10's GPU-shader option recorded here as conditional.
- **Invariants:**
  1. A layer whose validity key is unchanged is composited, not repainted.
  2. Transitions paint the outgoing screen once.
  3. The shell layer is independent of the page.
- **Scope:**
  - `gfx.*`: a `LayerCache` {key → target texture}, with backing from a same-size target pool and admission through R5b's `GpuBudget`.
  - `main.cpp`:
    - transitions snapshot `below_top()` once into a layer and composite it, instead of `below_top()->draw()` every frame (`main.cpp:592`);
    - the shell (tab bar, toast) as its own layer.
  - `Screen`: `layer_key()`, which defaults to never cached, for incremental opt-in. Settings comes first, then Details, then Home shelves under a moving hero.
- **State:** a key per layer (screen generation, image-ready count, focus index, size); target handles.
- **Ownership:** `LayerCache` owns the targets; each screen owns its key.
- **Service:** none.
- **Recovery:** a key mismatch repaints; out of budget falls back to direct drawing.
- **Verification:**
  1. A snapshot diff, cached against uncached, is pixel-identical on every tour step.
  2. Transition paints drop from 2 screens per frame to 1 composite.
- **TV:** transition frame p95, and Home focus-move frame p95.
- **Conditional (not scheduled):** a GLES2 path with GPU blur and glass replaces CPU frost only if R5c's measurements show frost latency or memory above budget. That would be leaving `SDL_Renderer` for the drawing path, which is a renderer decision for the owner.
- **Completion:** three stages.

## 3. Not adopted, and why

| Finding | Why not |
| --- | --- |
| M1 media arena, M2 media buffers, G9 YUV/OES paths | They belong to a segmented or sample media route. They are adopted with R7, if R7 is ever built. The URI route hands the file to uMediaServer |
| G8 meshes, A7 animated images | No such content in Bloob |
| Worker-isolate heap mechanics (C8, C9 detail) | Bloob has no JavaScript runtime in the app. The analogue is the service's Node process, covered by R5d and R5b |
| Thread priorities | The reference doesn't set them either (C12). Don't invent them |

## 4. What only the TV can tell us

These are properties of Bloob on this TV, not facts anyone publishes:
- real VmRSS and GPU use per screen;
- frame-stage times;
- the effect of batching order;
- upload stall size;
- the background memory drop;
- frost latency.

R5d records them first, so every later increment has a before and an after. Two version questions are answered by R5d's first log line:
- the TV's SDL version, which decides whether batching and `SDL_GetTicks64` exist;
- whether a webOS low-memory signal is available (check, read-only).

## 5. Status in 2.4.0

| ID | Planned | Implemented | Locally verified | TV-verified |
| --- | --- | --- | --- | --- |
| R5d | 16 Oct 2026 | Yes | Inspector snapshot and schema read back in the desktop tour | No |
| R5a | 16 Oct 2026 | Yes | Unit tests (pacer, tasks); desktop tour pixel-identical apart from timing | No |
| R5b | 16 Oct 2026 | Yes | Unit tests (budget, eviction order); desktop tour; inspector | No |
| R5c | 16 Oct 2026 | Yes | Desktop tour; inspector (one decode per source, no stale work) | No |
| R6r | 16 Oct 2026 | Transitions and the leaving screen; target pool | Inspector: 6 paints and 24 reuses across the tour | No |

**Found while implementing:** the worker pools never woke the UI thread. `Tasks::loop` moved the completion into the queue and only then tested it, and a moved-from `std::function` is empty, so the wake never fired. Every API reply and image then waited for an unrelated wake: up to 1 s when the app was idle (the loop's longest wait). This is the "task-completion wake bug" the branch review suspected but could not reproduce. It has been there since 1.x and was in the 2.2.0 build that ran on the TV. Fixed in `tasks.cpp`; `tests/runtime_test.cpp` fails on the old code.

**Per finding:**

| Finding | State in 2.4.0 |
| --- | --- |
| C1 | Done. One paint per drain; when the renderer has no vsync, frames are also gated to one per refresh. With vsync, `SDL_RenderPresent` paces frames (gating as well would start each frame late and miss the next refresh) |
| C2, C5, C6, C11, C12 | Done |
| C3 | Already done in 2.3 (listener snapshot); kept |
| C4 | Partial: `Tasks::stop` counts and logs what it drops. Per-job "must run" belongs to R1's final reports |
| C7 | Partial: deadline frames are counted per cause; the owner of each deadline is not yet recorded |
| C8 | Done: policies Normal, Playback (player on top, or low system memory), Background. The service trims its byte cache to match. The system memory subscription is unverified on the TV |
| C9, C10 | Done |
| M1, G3 | Measured, not built: allocations per frame are counted (`alloc.cpp`) and reported. An arena waits for the TV numbers |
| M2 | Partial: decoded sources and pixels in flight are counted. There is no zero-at-shutdown assertion yet |
| M3, M4, M6, M7, M8, M9, M10, M13 | Done (M10's estimate awaits TV calibration) |
| M5 | Done with frame-age protection instead of explicit pins: anything drawn in the last 3 frames cannot be evicted, which covers everything on screen |
| M11 | Not needed: the largest texture Bloob creates is 1920×1080. The renderer's maximum is logged at start |
| M12 | Done: `settled()` re-requests an evicted image |
| G1 | Deferred: splitting every screen into layout and paint is a large refactor. The inspector's paint time on the TV decides whether it is needed |
| G2, G4 | Done for transitions: the screen underneath and the leaving screen are painted once, then composited. Per-screen cached layers wait for G1 |
| G5 | Inherent: layers are used only where a whole screen is reused |
| G6, G7, G12, G13, G15 | Done |
| G8, A7 | Not applicable (no meshes, no animated art) |
| G9 | Counted. YUV and OES formats belong to R7 |
| G10 | Shared-source decode removes the repeated fetch and decode behind frosts. GPU blur stays conditional on a GLES path |
| G11 | Already lean; kept |
| G14 | Deferred: the tab bar is about a twentieth of the screen, and a cached copy needs its own target. Its frost and pills also change with every backdrop fade and focus move, so the key would rarely hold. Revisit if the inspector shows shell paint cost |
| A1–A6, A8, A9, O1–O3 | Done |
