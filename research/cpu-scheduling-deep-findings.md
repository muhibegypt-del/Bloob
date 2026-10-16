# CPU scheduling, frame handoff, and Node runtime findings

## Evidence and limits

This is a static analysis of the recovered 32-bit ARM native runtime, SHA-256 `182704ecaa5277ca837544272ac81cd704a1d0b8eb215afea04d143396d7affa`. Primary sources are the direct Ghidra headless exports `research/cpu-memory-gpu-decomp.txt`, `research/cpu-memory-gpu-followup-decomp.txt`, `research/cpu-memory-gpu-details-decomp.txt`, `research/cpu-memory-gpu-lifetime-decomp.txt`, and the instruction/literal export `research/runtime-data-decomp.txt`. Supporting lifecycle evidence comes from `research/luna-runtime-decomp.txt`, wrapper evidence from `research/luna-bridge-decomp.txt`, and additional function addresses from `research/runtime-symbols.tsv`.

These exports are raw Ghidra observations, **not REA Evidence records**. Function names and addresses below identify the source. No target execution, TV interaction, GPU measurement, or runtime scheduling trace was performed for this report. A recovered call proves a static call path, not that the path ran in a particular viewing session.

Ghidra has not recovered complete class types or the ARM floating-point calling convention consistently. Expressions such as a pointer cast to `float`, parameters named `in_r0`, and temporary variables typed as `Mutex *` can represent scalar data. Offsets below are raw displacements used to connect accesses across functions; their descriptive names are analysis labels, not recovered C++ field declarations. Some literal-pool constants and arguments to timeout and floating-point functions remain unresolved.

The clock unit is resolved: `Platform::getTicks` (`0x001049f8`) branches to `cw_getTicks` (`0x00105a94`). The latter calls `clock_gettime(7)` and adds seconds multiplied by `1000` to the nanoseconds component converted with reciprocal constant `0x431bde83` and a 50-bit shift. That is the compiler's integer division by `1,000,000`, establishing milliseconds. The clock identifier's numeric value is observed; a platform-specific clock name is not assigned here. This unit proof does not reveal omitted semaphore timeout arguments or the meaning of arbitrary event-processing parameters.

## Principal conclusions

1. The drawing side limits itself to one outstanding JavaScript frame request. While that request is pending, subsequent elapsed input is accumulated and a previously completed frame record can support continued drawing. This bounds frame-callback backlog in the shown producer path.
2. Node main-thread callback delivery preserves integer tokens in a queue and uses a semaphore for delivery accounting. `uv_async_send` wakes the loop; the wake itself is not the callback payload. Application-level frame coalescing is already established without assuming how libuv internally combines wakeups.
3. Main and worker paths do different work. Main dispatch drains explicit callback tokens to listener snapshots. Worker dispatch processes asynchronous events and updates its current isolate's cached statistics. A worker wake broadcasts to all registered worker loops.
4. Thread configuration supplies an optional stack size. The examined pthread creation wrapper does not apply an OS scheduling priority: its other optional setting is only printed in that body. Exact configured stack sizes and priority labels remain unknown.
5. Memory savings mode is persistent policy state in `NodeJsMT`, is applied to newly registered worker isolates, and requests interrupts in the main and existing worker environments. The interrupt callback bodies are still needed before claiming a particular GC, heap limit, or memory reduction mechanism.
6. Heap reporting is intentionally based on cached per-isolate samples. Reading the aggregate can request refreshes asynchronously, so the returned record need not reflect the instant of the read.

## Threads: creation, naming, stack settings, and stop ordering

### Observed creation path

`Platform::Thread::startThread` (`0x00104f64`, primary export) searches a global configuration table when it receives a non-null name. The search compares names and advances by three 32-bit words per record. For a match it passes pointers to the second and third words to `cw_threadCreate`, rather than passing those words' addresses as numeric settings. No match passes null optional-setting pointers. It also stores a copy of the supplied name in the thread object; a null-name path assigns a fallback string whose actual contents are not recovered here.

`cw_threadCreate` (`0x00105b9c`, follow-up export) initializes pthread attributes. When its optional stack-size pointer is present, it dereferences that pointer and calls `pthread_attr_setstacksize`. If setting the stack size fails, it does not proceed through the shown creation branch. A separate optional pointer is dereferenced only for a `printf` call in this wrapper. There is no `pthread_attr_setschedparam`, `pthread_setschedparam`, affinity call, or other scheduler-setting call in this function's recovered body or direct call set.

The wrapper allocates a 16-byte bookkeeping block, optionally duplicates the supplied name with `strdup`, stores the passed entry/argument values, and gives the block to a pthread start trampoline. On `pthread_create` failure it frees the duplicated name, deletes the block, and destroys the pthread attributes. The exact success-path lifetime of the block and copied name belongs to the trampoline and join wrapper, which have not been exported here.

`Platform::Thread::s_threadProc` (`0x00105250`, primary export) gets the actual current thread ID, stores it, and registers a thread-ID-to-name entry under a mutex before making an indirect virtual call. The indirect destination is consistent with the class's `run` dispatch, but the decompiler did not resolve the virtual target. This supports diagnostic thread-name bookkeeping; it does not establish OS thread naming.

### Priority conclusion and boundary

The optional configuration word printed by `cw_threadCreate` could describe a requested priority, but its meaning requires the resolved format string/table and cannot be established from the pointer's position alone. The precise conclusion is that **this examined pthread creation wrapper applies the configured stack size and does not apply an OS priority in its body**. It would be too broad to say that the whole application never changes thread priority: the start trampoline and other platform code have not been inspected.

### Stop is a join, not a cancellation signal

`Platform::Thread::stopThread` (`0x0010503c`, primary export) calls `cw_threadJoin`, clears the stored thread handle, then removes the diagnostic name-map entry under a mutex. The shown body contains no stop-flag write, cancellation call, or wake operation before joining.

The caller supplies stop ordering. `Luna::AppEngine::stopNode` (`0x004b23f8`, runtime export) checks that the Node runtime exists and appears active, calls `NodeJs::NodeJsMT::stop`, and only then calls `Platform::Thread::stopThread`. Thus joining is a completion barrier after a separate runtime stop request.

`NodeJsMT::stop` (`0x004bfa4c`, details export) checks its activity byte, calls `node::Custom::stop` (`0x00a58a30`), clears the byte, and compares the calling thread ID with the runtime's stored thread ID. For an external caller, it locks the callback mutex, deletes later callback-deque storage blocks, and resets the deque end to its current beginning. It then waits on a completion semaphore. This is a pending-callback discard path, not evidence that every queued callback is delivered during shutdown. The custom stop implementation and semaphore-count reconciliation remain outside the selected evidence.

`AppEngine::start` (`0x004b3038`, runtime export) creates its command-line object, starts its runtime thread, then waits on a semaphore. `AppEngine::run` (`0x004b4340`) registers callbacks and an asynchronous-event listener, records its thread ID, starts Node with two function objects, and unregisters the asynchronous-event listener when that call returns. A separate semaphore post is visible on a nonzero Node-start return path. `NodeJsMT::start` (`0x004c0308`, details export) sets the activity byte and runtime thread ID, registers linked modules, wraps startup callbacks, calls `node::Custom::start`, destroys its temporary function objects when that call returns, and posts the completion semaphore awaited by `NodeJsMT::stop`. An already active call returns without starting another runtime. Startup callback bodies are still required to describe all successful-start and failure-release cases; the existing evidence alone does not justify claiming every startup failure releases every waiter.

## Frame scheduling: a bounded producer and two wait orders

### The frame request and response channels

`AppEngine::internalDraw` (`0x004b566c`, primary export) first calls `Event::processAsyncEvents(0)`. In its active path it uses a raw byte at displacement `+0x15d` as an outstanding-request state. When the byte is clear, it constructs and appends a 40-byte `FrameRequest` under a request-queue mutex, sets the state, stores a tick value, and calls `NodeJsMT::doCallback` with target mask `1` and token `0`. A monotonically incremented request counter and accumulated elapsed input advance only when a request is submitted.

When the byte is already set, the shown producer does **not** append another frame request or emit another frame callback. It adds the current elapsed input to an accumulator at `+0x1c0`. The next submitted request incorporates that accumulator and resets it. This preserves elapsed-time information while avoiding a queue of obsolete frame callbacks. The source's floating argument recovery is unreliable, so the report does not treat the displayed `ApiContext *` cast as a real pointer-to-number conversion in the original program.

`AppEngine::handleNodeCallback` (`0x004b5f2c`, follow-up export) supplies the other half of the channel. For a nonnegative token, it removes one 40-byte request under the request mutex, releases the mutex, prepares a frame record, optionally invokes `EventArgSync<Luna::Application::FrameArg>::invoke` (`0x004b4770`) for an active application, calls an unresolved graphics virtual method, collects cached heap statistics, appends a 200-byte `FrameInfo` under a separate response-queue mutex, and posts the frame-completion semaphore. If the request queue is empty, it clears its progress marker and returns without posting a response.

The numeric nonnegative token is not used as a frame index in the shown handler; its sign selects the branch, and the request data comes from the queue. For a negative token, the handler calls `Event::processAsyncEvents(0)` and `processControlEvents` instead of consuming a frame request. This demonstrates separate event-processing and frame-processing callback semantics. It does not establish that every possible negative token means exactly the same thing in other handlers.

The actual JavaScript bridge is corroborated by `Luna_Application_Wrapper::handle_onFrame` (`0x00c592e4`, bridge export): it obtains the current V8 isolate, opens a handle scope, converts the C++ argument, and calls the installed JavaScript function through `node::MakeCallback`. That bridge body does not show JavaScript scheduling or rendering cost.

### What waiting consumes

`AppEngine::waitNextFrame` (`0x004b2c3c`, primary export) checks the outstanding-request state. With a pending request, raw flag bits `0x2` or `0x4` select `Semaphore::wait`; without those bits it calls `Semaphore::waitTimeOut`. The actual timeout argument is not recovered in the export.

On a successful semaphore wait it clears the outstanding-request byte, locks the response queue, and, when the queue is nonempty, copies and removes one 200-byte response. It also copies that response into a cache at `+0x94` and marks the cache valid. It returns a successful/nonzero result only when it consumed a record. The state is cleared even if a successful semaphore wait finds an empty response queue, so semaphore/queue consistency is a separate invariant in this implementation.

On a timeout/failure return from `waitTimeOut`, it leaves the outstanding-request state set and returns no new frame. It checks whether more than `10000` milliseconds have elapsed since its diagnostic timestamp, logs progress if tracing permits, then updates that timestamp. **The ten-second comparison throttles diagnostics; it is not the semaphore timeout.**

If no request is pending but flag `0x4` requests this wait, the handler logs an assertion reason and calls `cw_abort`. The shown producer's flag-`0x4` path queues a request before reaching this wait when necessary, which is consistent with the invariant that a synchronous wait must have work to await. Public enum names for these flags are not recovered.

### Ordinary pipelining and the flag-0x4 path

With flag `0x4` clear, `internalDraw` waits for any previous request **before** attempting to submit a new one. When a previous response arrives, it clears the outstanding state, queues the next JavaScript request, and can then draw from the completed response. This is consistent with a pipeline in which native drawing uses completed frame data while the JavaScript side prepares the next frame.

With flag `0x4` set, it skips the initial wait and performs the wait **after** the submission attempt. If a request was already pending, it accumulates elapsed input and waits for that request; if none was pending, it submits one and waits for it. This gives the flag a concrete effect without inventing an enum name.

When waiting produces no new response but a cached response exists, `internalDraw` copies that cached record and can still call the graphics methods, update performance counters and media metrics, finalize the native draw, flush console output, and process its callback registry. A marker is set in the local record to distinguish this reuse. When neither a new response nor a cache exists, the drawing branch is skipped. The cache therefore has a lifetime across draw calls, not just across one timeout attempt.

This proves frame-record reuse and callback backpressure in static code. It does not prove a fresh visual scene every draw, a particular frame presentation rate, or continued GPU execution. No GPU fence or display-present observation is part of this scope.

### Progress values are diagnostic, not completion proofs

`handleNodeCallback` writes `2` around the negative-token event/control path, `3` around frame-request processing, and `0` on return/empty-frame-queue exit. `waitNextFrame` logs the state through `callbackProgressToString` (`0x004b2bfc`, not exported). These values identify coarse handler phases, not an end-to-end key-to-screen trace. No additional phase names are assigned without that helper's body.

### Pacing and elapsed-time filtering

`AppEngine::draw` (`0x004b5930`, primary export) conditionally calls `calculateNextFrameTimeWait` and passes a nonzero result to `Platform::delay`. It records a tick timestamp and, when caller-provided elapsed input is not positive, rotates a three-entry history of observed inter-draw intervals and obtains a filtered value from `getFrameTime`. It calls `internalDraw`, increments a pacing counter for results `0` or `2`, and resets that counter for other results. The first accepted result in a sequence records the pacing origin.

`resetFrameTiming` (`0x004b27f0`, follow-up export) derives three initial intervals from rounded integer divisions of `1000`, `2000`, and `3000` by a rate-like member at `+0x3c`. It backdates the previous-tick member by the first interval and clears the pacing counter. If the rate is zero it logs and skips that initialization. With the clock unit now established, the history contains millisecond intervals. This is consistent with rounding compensation over three frames; exact rate-field naming still requires its display-property producer.

`getFrameTime` (`0x004b2888`) sums the three history values, imposes a lower bound derived from integer `3000 / rate - 1`, converts the value to floating point, and multiplies by an unresolved constant at `DAT_004b28c0`. This is evidence of smoothing/clamping, but the multiplier must be resolved before stating whether the return is seconds or an average in another unit.

`calculateNextFrameTimeWait` (`0x004b28c4`) uses the rate, current tick, pacing origin and pacing counter. It includes a `ceilf` call, can advance the counter toward elapsed time and return zero, and caps a computed delay at a value equivalent to twice a derived frame interval. The decompiler leaves the floating argument to `ceilf` undefined, and `DAT_004b2954` is unresolved. An exact pacing formula or a promise of particular hertz would exceed the recovered evidence.

At the end of the actual drawing branch, `internalDraw` iterates a mutex-protected callback tree. It compares the unsigned elapsed ticks with a stored threshold, substitutes an unresolved float constant on expiry, invokes the registered callback, and removes an entry if the callback returns zero or the selected float is zero. Callback names, the expiry constant, and the caller's lifetime contract are not recovered; this must not be described as a verified animation, timer-cancellation or resource-recovery API.

## Node callbacks: queue semantics, wake signals, and listener lifetimes

### Main-target callbacks preserve work separately from wakeups

`NodeJsMT::doCallback` (`0x004c05a0`, primary export) treats the target argument as a bitmask. Bit `0x1` locks a mutex, appends the integer token to a deque, releases the mutex, posts a semaphore, and calls `wakeMainThread`. Bit `0x2` calls `wakeWorkerThread`. A mask containing both bits executes both paths. Only the main-target branch queues the integer token in this function.

`wakeMainThread` (`0x004bfbac`) calls `node::Custom::isInitialized` and sends a libuv async wake only when initialized and a stored main async-handle value is nonzero. The main-target token has already been queued and the semaphore posted before these wake guards are tested. Thus a suppressed wake does not undo that enqueue in the shown function. The external-caller stop path described above explicitly discards queued callbacks; startup and callback-semaphore reconciliation require additional lifecycle evidence.

`asyncCallbackMain` (`0x004c0004`) also checks Node initialization before dispatch. On the initialized path it may refresh the current isolate's cached statistics, then repeatedly calls the callback semaphore's timeout wait. For each successful wait it removes one token under the queue mutex. It continues until the wait returns nonzero. The queue therefore preserves individual tokens even if wake notifications merge internally. The actual wait duration and libuv coalescing behavior have not been recovered here.

If listeners exist, it copies their `DelegateBase` entries into a temporary vector while holding the listener mutex, releases that mutex, then invokes the copied delegates with the address of the token. It destroys the temporary vector after dispatch and proceeds to the next semaphore iteration. If the event/listener object is absent, it continues draining tokens without invoking a listener.

The concrete lifetime boundary is a listener snapshot for **each token**, with the global listener lock released during calls. This permits dispatch without holding the listener-list mutation lock and gives copied delegates a lifetime until the current token's dispatch ends. The precise ownership guarantees of `DelegateBase` depend on its copy/destructor implementation, which is not inspected here. A listener removed during one snapshot can still be present in that snapshot; changes can affect a later token's newly copied snapshot. Those are consequences of the observed copy-before-call structure, not proof of arbitrary concurrent registration safety throughout the library.

No de-duplication of integer main callback tokens is visible in `doCallback` or `asyncCallbackMain`. The bounded frame behavior comes from the upstream `AppEngine` pending-request gate. Other producers are not automatically bounded by that gate.

### Worker dispatch is a broadcast wake and a bounded event-processing call

`wakeWorkerThread` (`0x004bfbc4`) checks initialization, locks the worker-record mutex, iterates a vector in 44-byte steps, and calls `uv_async_send` for every stored worker handle. It does not select a worker by the main integer token. It is a broadcast over the registered vector.

`asyncCallbackWorker` (`0x004bfc3c`) returns through a trace path when Node is uninitialized. Otherwise it calls `Event::processAsyncEvents(0x32)`. If that returns `2`, it calls the broadcast wake again. It then gets the current V8 isolate and its current Node event loop, locates the matching registered worker record under the worker mutex, and refreshes that record if its sample is older than `1000` milliseconds.

The literal `0x32` and return value `2` establish an event-processing budget/control value and a reschedule trigger at this call site. The implementation of `Event::processAsyncEvents` (`0x00107458`) is needed to say whether `0x32` counts events, tick units, or something else, and whether `2` means remaining work or another condition. Calling it a verified 50-millisecond budget would be unsupported.

## Worker registration and cleanup boundaries

`NodeJsMT::registerWorkerEventLoop` (`0x004c0708`, primary export) requires a current Node environment, a nonzero internal environment condition, and a current event loop. Under a mutex it searches the existing worker records by the event-loop value stored through each async handle. An already registered loop exits without adding another record.

For a new loop it releases the search lock, allocates a 64-byte async-handle object, associates it with `this`, initializes it with `uv_async_init`, and directly ORs an internal handle word with `0x10`. The intent of that internal bit must be checked against this bundled libuv's layout before calling it an unref or another public operation.

It constructs a reference-counted delegate containing the registering thread ID, the `NodeJsMT` object and a callback target, registers that delegate with `Event::registerAsyncEventListener`, collects an initial heap-statistics sample, and appends a worker record containing the delegate, async handle and nine-word statistics storage. It then makes an indirect registration call with the isolate, another callback and loop value. The details export establishes that `unregisterWorkerEventLoop` is called by the registration cleanup lambda (`0x004bff58`), corroborating cleanup registration; the indirect environment method's exact API identity is not recovered.

The search lock is released before allocation/initialization and reacquired for insertion, with no visible second duplicate search. A claim of duplicate-safe concurrent registration would therefore need a stronger external calling/threading invariant. Normal serial registration and the initial duplicate lookup are observed; a real race is not established without evidence that two callers can register the same loop concurrently.

`unregisterWorkerEventLoop` (`0x004bfe08`, details export) searches the vector for a matching loop under the worker mutex, makes a local copy of the record's delegate, and calls `uv_close` on its async handle unless the handle is already closing. It checks loop-related state and conditionally calls `uv_run(loop, 2)`, then shifts subsequent records left, shrinks the vector by 44 bytes, destroys the last record's delegate reference, unregisters the saved async-event listener and destroys the saved delegate reference. All of this appears inside the worker-mutex scope. The decompiler shows an additional `uv_is_closing` call with the loop value; that type-sensitive expression needs instruction/ABI verification before assigning it a higher-level loop-state meaning.

Thus worker removal explicitly retires the loop's wake handle and listener, removes it from future broadcast/statistics iterations, and releases record/delegate references. The lifetime export confirms that the close callback (`0x004bf7f0`) traces if enabled and calls the imported `free` function on its handle argument. This pairs registration's `malloc(0x40)` with a deallocator at asynchronous close completion. The complete `~NodeJsMT` (`0x004bff68`) lifetime is not covered. A single conditional `uv_run(...,2)` is not proof that every outstanding close callback has run before unregister returns, and a requested close must not be conflated with immediate memory release.

## Memory savings mode: fanout and asynchronous application

`NodeJsMT::enableMemorySavingsMode` (`0x004bfb10`) sets a raw policy byte at `+0x80` and calls the no-argument `node::Custom::enableMemorySavingsMode` (`0x00a5966c`). Disable (`0x004bfb5c`) clears the byte and calls the corresponding custom function (`0x00a59594`). `AppEngine` exposes small forwarding functions at `0x004b2bc8` and `0x004b2bd4`, identified in the symbol/caller inventory.

Both custom functions lock a global mutex, test a global environment pointer, request an interrupt in that environment, iterate its linked worker list, lock each worker's libuv mutex, and request the same policy interrupt for a non-null worker environment. They then release the per-worker and global locks. This is concrete existing-environment fanout; the outer `NodeJsMT` wrapper alone would not have established it.

`registerWorkerEventLoop` checks the stored policy byte and calls the isolate-taking overload `node::Custom::enableMemorySavingsMode(v8::Isolate*)` (`0x00a58a70`) on a newly registered worker when the byte is set. Thus policy intent persists for new worker registration as well as being sent to currently known environments.

The work is requested through `Environment::RequestInterrupt` specializations at `0x00a59604` (enable) and `0x00a5952c` (disable). The lifetime export shows both allocating a 16-byte callback object and transferring it through a `unique_ptr` into the environment's `CallbackQueue` under its libuv mutex. Each conditionally sends a libuv async wake while the queue mutex is held, releases that mutex, and calls `Environment::RequestInterruptFromV8` (`0x00a73980`). The temporary smart pointers are then destroyed after ownership transfer. No deduplication of policy callback objects is visible in these two wrappers; the queue implementation and callback virtual targets are separate evidence boundaries.

This supports asynchronous application on the environments' own execution paths, not proof that all isolates have changed policy before the public wrapper returns. No completion acknowledgement or waiting barrier appears in these examined enable/disable wrappers.

The actual interrupt lambda bodies and the isolate overload are absent. Consequently the evidence does **not** yet identify a specific V8 memory reducer, compaction action, GC schedule, page release, heap limit, code flushing policy, or immediate drop in RSS. Enable/disable mode names alone do not establish those mechanisms. The byte is stored before fanout, so an environment absent at fanout time can still be covered later by registration's inheritance path; behavior of environments that never pass through this registration remains unknown.

## Heap reporting: cached ownership, freshness, and separate accounting

`NodeJsMT::getIsolateHeapStatistics` (`0x004bfbf8`, follow-up export) obtains **the current isolate**. If one exists, it calls `v8::Isolate::GetHeapStatistics`, writes a tick timestamp and selected values into the supplied record, and zeros two final record words. If there is no current isolate, it leaves the destination unchanged. It is not a general getter that directly reads an arbitrary worker isolate from another thread.

`asyncCallbackMain` refreshes its stored main-isolate record when its age exceeds `1000` milliseconds, once on entry before draining callback tokens. `asyncCallbackWorker` refreshes the matching current-loop worker record with the same age test. Registration obtains an initial worker sample. The shown code therefore samples heap state in the isolate's execution context and keeps a cache for aggregation. This is an age check when the callback runs, not proof of an independent periodic one-second timer.

`NodeJsMT::getHeapStatistics` (`0x004bfd26`, primary export) locks the worker-record mutex, copies the main cached record into the output, adds selected words from all worker cached records, and fills a separate output word from `SharedBuffer::Allocator::getTotalSize` (`0x0010d238`). It releases the mutex and only then requests a main wake if the main sample is older than `1000` milliseconds and a worker broadcast wake if any worker sample is older than that threshold.

The returned output is therefore a snapshot of cached samples **before** any requested refresh completes. Worker samples can have different timestamps and ages. There is no immediate cross-isolate sampling barrier in this getter. The displayed arithmetic also preserves separate aggregate/composite slots and shared-buffer accounting; the nine raw record words should not all be summed or labelled as nine independent heaps. Exact names of the selected V8 statistics require the underlying V8 layout and the producer's local offsets to be resolved.

`handleNodeCallback` calls this getter while assembling `FrameInfo`, and `PerformanceCounters::updateStats` (`0x004b9bc0`, follow-up export) copies its corresponding stored values into the counters object along with frame-stage timestamp differences. This explains how cached runtime memory can accompany a frame record. It does not convert a frame's cached heap data into process RSS or GPU-resident bytes.

`PerformanceCounters::run` (`0x004b9680`, follow-up export) separately waits on a semaphore and polls `memoryGetPeakRss`, `memoryGetCurrentRss`, and `memoryGetCurrentSwap` while an activity byte remains set. It checks that byte before and after the wait, allowing a wake-and-disable shutdown shape, but the start/stop functions and actual interval are not included here. These process counters are distinct from V8 isolate statistics and shared-buffer accounting.

## Decisions useful to Bloob

These are implementation lessons inferred from the observed mechanisms, not recovered Bloob requirements or proof of identical Apple TV behavior on another platform.

| Decision | Evidence supporting it | Practical boundary |
|---|---|---|
| Keep at most one UI-frame computation outstanding and accumulate elapsed input while it runs. | `internalDraw` pending gate and accumulator; `handleNodeCallback` response queue. | Apply only where skipped intermediate computations are semantically safe. Input/control delivery has a separate branch. |
| Preserve callback payloads in a queue and use wake signals only to tell the receiver to inspect work. | `doCallback`, semaphore accounting and `asyncCallbackMain`. | A merged wake must not lose queued commands; impose an explicit backlog policy for producers beyond the frame path. |
| Allow native drawing to use a completed scene/frame record while the next UI update runs. | Wait-before-submit default order and cached `FrameInfo` reuse. | Measure scene age and interaction latency; drawing an old scene is not proof of a responsive UI. |
| Execute callbacks after releasing the listener-list lock, using an owned snapshot for the current dispatch. | Temporary delegate vector in `asyncCallbackMain`. | Specify whether removal affects the current dispatch or the next token, and verify ownership semantics in Bloob's implementation. |
| Separate per-isolate heap, shared buffers, process RSS/swap, and graphics resource counters. | `getHeapStatistics` versus `PerformanceCounters::run`. | Timestamp cached data and expose freshness; avoid adding overlapping counters as if they were disjoint memory. |
| Apply a memory policy to existing workers and inherit it when registering future workers. | Custom interrupt fanout and registration's policy-byte check. | Treat policy changes as asynchronous unless there is an explicit completion acknowledgement. Measure the actual memory effect. |
| Signal shutdown before joining threads and retain cleanup until asynchronous handle close completes. | `stopNode` before `Thread::stopThread`; completion semaphore in `NodeJsMT::start`/`stop`; `uv_close` and listener removal in worker unregister. | Specify whether queued callbacks are discarded on stop and verify final handle-close completion. |
| Tune stack budgets per worker role when justified by profiling. | Name lookup and `pthread_attr_setstacksize`. | No numerical stack sizes or working OS priorities were recovered; do not copy speculative settings. |

## Coverage and next evidence targets

| Question | Status | Remaining evidence |
|---|---|---|
| How are main/worker callbacks awakened? | Answered statically for the selected wrapper paths. | Actual scheduling latency and libuv wake coalescing require implementation/runtime evidence. |
| Where is frame backlog bounded? | Answered statically for `internalDraw`'s producer. | Confirm all other request producers and timestamp a real trace if measuring responsiveness. |
| What happens on a slow JavaScript frame? | Partially answered: elapsed accumulation, timeout return, cached record reuse, repeated diagnostic throttling. | Exact wait timeout, fatal/error paths and visual age under real load. |
| Is OS thread priority applied? | No application in the examined `cw_threadCreate` body; global answer unresolved. | pthread start trampoline, configuration table/string values, any separate priority setters. |
| What exactly does memory savings mode do? | Fanout, persistence, callback queue ownership, async wake and V8 interrupt requests answered; actual heap/GC actions unresolved. | Queued callbacks' virtual/lambda targets, isolate enable overload `0x00a58a70` and matching disable overload. |
| How are worker handles and listeners retired? | Unregister, `uv_close`, record removal, listener release and close callback's `free` answered; completion timing unresolved. | Destructor `0x004bff68`, close-progress observation, and exact custom stop/start callbacks. |
| What are the exact clock/time units and pacing constants? | Clock milliseconds answered by thunk, `cw_getTicks` and reciprocal constant; floating constants and actual waits unresolved. | `Platform::delay` `0x00104a00`, literal values at `DAT_004b28c0`/`DAT_004b2954`, timeout ABI/call sites and `Semaphore::waitTimeOut` `0x001053a8`. |
| What do worker event budget and progress enums mean? | Call-site behavior answered; enum/budget meanings unresolved. | `Event::processAsyncEvents` `0x00107458`, `callbackProgressToString` `0x004b2bfc`. |

The selected sources answer the main coordination mechanisms, millisecond clock unit, and worker-handle free at close completion. They leave explicit gaps in platform-wide priority policy, floating pacing constants, actual timeout values, memory-mode GC actions and shutdown completion timing. None of these static findings establishes live CPU utilization, parallel execution speedup, memory reclaimed, or GPU timing.
