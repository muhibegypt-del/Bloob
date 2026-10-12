# Elements of A: Luna runtime architecture

This is a map of the reference application's bundled **Luna** runtime, not the webOS `luna://` service bus. It is based on the locally retained shipped JavaScript, native symbol inventory, focused Ghidra decompilation, and the separate TV observations in [the system map](Elements-of-A-system-map.md) and [episode study](live-episode-study.md). Addresses are entry addresses in the local ARM analysis image. Names and call edges are static evidence; they do not prove that every branch ran during the observed episode. Protected-content paths are excluded.

## End-to-end runtime path

```text
native launcher / AppEngine::setup
  -> create graphics engine, performance counters, NodeJsMT, configuration
  -> AppEngine::start: acquire media client, optional splash video, start runtime thread
  -> AppEngine::run: prepare Node arguments, mount bundled JavaScript, start Node/V8
  -> bootstrap: register Luna.Application event handlers and load UI chunks
  -> input/control event: native event -> wrapper -> V8 callback -> app event fanout
  -> UI mutation: application-owned drawing tree changes
  -> draw tick: async events -> frame request -> Node callback -> graphics processing
  -> graphics: dirty-tree update -> clip/render queue -> frame submission
  -> playback: app player session -> Media source/player -> LG media adapter
  -> shutdown: stop Node thread, destroy graphics, release player session
```

The UI, JavaScript event loop, drawing tree, graphics engine, and media engine are in one application runtime. The player overlay is drawn by that UI tree. The reference app does not hand its whole interface to a separate stock player.

## Process bootstrap and threads

| Entry | Finding |
| --- | --- |
| `LunaSdk::Luna::start` `004b0bec` | Gets the application engine, sets it up, and starts it. |
| `Luna::AppEngine::setup` `004b8930` | Creates `Gfx::Engine` with display dimensions, updates display properties, creates performance counters, initializes root certificates, creates `NodeJs::NodeJsMT`, parses configuration, and passes environment settings to the graphics engine through an unresolved virtual call. |
| `Luna::AppEngine::start` `004b3038` | Obtains the media implementation/client. An optional splash path draws a video into a graphics context. Starts a `Platform::Thread` and uses a semaphore to await runtime startup. |
| `Luna::AppEngine::run` `004b4340` | Configures the Node command line and version fields, registers an async event listener, mounts bundled JavaScript, records the runtime thread, starts Node via `NodeJsMT`, and unregisters the listener at exit. |
| `Luna::AppEngine::stopNode` `004b23f8` / `destroyGfx` `004b251c` | Stops Node and its thread, then destroys the graphics engine. These are runtime teardown functions; player-session cleanup is a separate layer. |

The TV runs an aarch64 kernel, while this executable is 32-bit ARM. That is a normal compatible user-process arrangement; it does not make the native executable a 64-bit binary. During one episode the app had 74 threads and around 368–374 MB RSS. Those live numbers include application and media work and cannot be assigned to Luna alone.

## Native-to-JavaScript event bridge

`Luna_Application_Wrapper` exposes callback properties for `frame`, key, mouse, multitouch, axis, control, display/device changes, and root assignment. `handle_onFrame` at `00c592e4` and `handle_onKeyInput` at `00c594a8` each convert a C++ event argument into a JavaScript value, create a V8 handle scope, and invoke the registered function through `node::MakeCallback`. This is a verified bridge from native event objects to JS callbacks. The wrapper's `setRoot` binding and `Luna::Application::setRoot` at `004b8b14` connect the application root to `AppEngine` and establish a drawing context.

The shipped `res/bootstrap.js` binds those `Luna.Application` callbacks to an event fanout for `axisInput`, `control`, `devicePropertiesChanged`, `displayInfoChanged`, `frame`, `inputDeviceChanged`, `keyInput`, `mouseInput`, and `multiTouchInput`. It replays control messages queued during loading, marks bootstrap/load timing, and after its first `frame` schedules additional UI chunks with `setImmediate`. It also supplies `performance` and `window` globals in the Node environment. This explains how native remote events become app navigation events and why part of the UI loads after the first frame.

`Luna::AppEngine::keyInput` at `004b616c` receives a native key event and queues an async invocation after a diagnostic key-sequence check. `processControlEvents` at `004b5e7c` drains a control-event queue under a mutex. The exact key mapping and all event-order edge cases need a timestamped live input trace; the static callbacks establish the structural path.

## Frame scheduling and rendering

`Luna::AppEngine::draw` at `004b5930` computes time to the next frame, delays, obtains frame timing, and calls `internalDraw` at `004b566c`. `internalDraw` first processes asynchronous events, prepares a `FrameRequest` under a mutex, and calls `NodeJs::NodeJsMT::doCallback` when a JS frame callback is needed. If a request is already pending, it accumulates frame delta rather than blindly enqueuing another callback. It waits for a frame response, then advances graphics work, media/performance metrics, console flushing, and timed callbacks. The precise target of some graphics virtual calls remains unresolved.

`NodeJsMT::doCallback` at `004c05a0` enqueues a callback token for the main side, posts a semaphore, and wakes the Node main thread; it can also wake a worker side. `asyncCallbackMain` at `004c0004` waits on that semaphore, drains callback tokens, and invokes registered delegates. It periodically samples V8 heap statistics. This is the cross-thread wakeup mechanism behind the app's frame and other native callbacks.

`Luna::AppEngine::waitNextFrame` at `004b2c3c` uses a semaphore and a mutex-protected queue of fixed-size frame records. Depending on flags it waits or times out, then copies the next frame record into engine state. A timeout diagnostic is possible. Exact timing modes require resolving the caller's flags and measuring TV timestamps; the source does not justify claiming a fixed frame rate.

`Gfx::IEngine::processFrame` at `002e8bc8` checks for newly decoded images, newly loaded fonts, and a dirty drawing tree. If none require work and no force flag is set, it can avoid building a render frame. Otherwise it takes a queued `ClipRender`, updates the drawing tree and optional debug overlay, processes clip history, calls `ImageServer::frameDone`, and records timings. `Gfx::IEngine::renderFrame` at `002e80b4` consumes that render work through clip rendering, texture/cache work, and text state. The renderer thus has a meaningful idle path and separates tree update from frame submission. We have not measured GPU time or confirmed how often the dirty path was used live.

`Gfx::DrawingContext::drawVideo` at `002eb2e0` forwards a `Media::Player`, destination rectangle, and style to `IDrawingContext::drawVideo`. That establishes the composition API for video plus app-drawn chrome. It does not establish whether the final video plane is textured or overlaid for each media backend. `Gfx::ImageServer::frameBegin` `00329528` and `frameDone` `00326fec` coordinate decoded images, animation frames, and task release; `Text::FontServer::requestFontLoad` `00340858` queues asynchronous font work and wakes its worker.

## Media and UI relationship

The app's semantic play intent goes through its root application, service/loading flow, player page, and shared player session before reaching a source adapter. The session owns state, clock, buffering, tracks, reporting, and lifecycle; the page owns the drawn overlay and remote interactions. The media routes observed statically include a segmented worker plus source-buffer route and a progressive URI route. The native media map in [the system map](Elements-of-A-system-map.md) traces `Media::MsePlayer` through `TrackPump` into `LgEsPlayerPalImpl` sample submission and buffer gating. During live playback, source-buffer, queue, sink, decoder, and LG media device descriptors were visible; many disappeared after leaving playback. This is the clearest efficiency evidence for explicit session teardown.

The runtime has performance-counter and metrics classes for frame time, memory, image use, and media state. Their existence and call sites show instrumentation capability, not that a particular build exposes a user-facing profiler or that our observations captured its values.

## What this means for Buna

Build an original application-owned coordinator with the same **separation of responsibilities**: semantic navigation; a single playback session; a clock with clear source and seekable range; input routing; a drawing/UI layer; backend adapters; and explicit resource release. Preserve the idle principle: schedule work on input, media state, animation, or asset completion, and avoid repainting an unchanged screen. Validate webOS-supported playback paths on the actual TV before adding any native bridge. The reference runtime's private ABI and implementation are not a reusable dependency.

## Evidence gaps and next measurements

- The native symbol inventory in `runtime-symbols.tsv` contains 6,068 nonprotected runtime-related function names, entry addresses, and function byte counts. It is an inventory, not 6,068 confirmed live calls. `ghidra-scripts/InventoryRuntimeSymbols.java` regenerates it from the local Ghidra project.
- No timestamped end-to-end trace yet joins a remote key press to a specific native callback, JS navigation mutation, render submission, media-state transition, and first decoded frame.
- Some virtual graphics and LG media calls have unresolved targets. Output plane choice, exact decoder mode, frame pacing, dropped frames, memory budgets, and selected rendition remain unknown.
- The current runtime observations concern one app version and one TV. A controlled paired remote/capture sequence and TV-side metrics would resolve these gaps without reading protected-content data.
