# Elements of A: asset scheduling and resource observability

## Evidence boundary

This complements the CPU, memory and GPU findings. It uses the three local `cpu-memory-gpu-*-decomp.txt` exports, including `cpu-memory-gpu-decomp.txt`, and `runtime-data-decomp.txt`, generated read-only from the existing Ghidra analysis. Target SHA-256: `182704ecaa5277ca837544272ac81cd704a1d0b8eb215afea04d143396d7affa`. These are direct Ghidra observations, not REA MCP Evidence records or live TV traces. The raw exports remain ignored by Git; this document paraphrases behavior.

Function addresses refer to that artifact. Observed means a body or call sequence is present; inferred means an interpretation; unknown means the inspected evidence does not establish the answer. Decompiled signatures, especially floating-point arguments and structure returns, can be wrong.

## 1. Artwork has a scheduling pipeline

**Observed:** `Gfx::ImageServer::ImageServer` (`0x0032302c`) creates an `ImageProcessor`, subscribes to state/header events, starts the processor and subscribes to scratchpad-prune events. The UI-facing image server, work processor and graphics cache are connected components with separate lifetimes.

**Observed:** `Gfx::ImageProcessor::run` (`0x00322290`) registers an asynchronous-event listener, constructs an `ImageDecoder`, connects its completion event and records it in a vector. The worker waits on a semaphore, consumes async events, processes retirement bookkeeping, calls `updateLoading`, calls `updateDecoding`, and releases tasks. On exit it releases outstanding task/data-request resources, destroys decoder objects and unregisters its listener. `ImageDecoder::ImageDecoder` (`0x002fa4cc`) initializes its own queue, semaphore and mutex and starts a platform thread.

**Inferred:** This is a coordinator plus a decoder worker, rather than artwork decoding inline in each frame. The inspected run method initially creates one decoder; its use of a vector does not prove adaptive worker scaling or a core-count-based pool. The surrounding image decoder also builds a tone-response lookup table during construction; that is evidence of preprocessing, not a quantified color-management benefit.

**Unknown:** Actual asset throughput, worker utilization, complete codec-specific decode costs and runtime concurrency beyond this initialization path.

## 2. Decode admission is related to clipping relevance

**Observed:** `Gfx::ImageProxy::handleClipDistance` (`0x0031c89c`) returns when its integer field at offset `+4` exceeds 999; otherwise it requests `ImageServer::decode` (`0x00325590`). This proves a relevance/distance gate exists between an image proxy and decoding.

**Unknown:** The field's producer, units and transform are outside this focused export. Do not turn the threshold into “1,000 pixels,” “1,000 cards,” or a universal prefetch radius. The method name suggests clipping distance; the exact spatial rule remains to be traced.

**Observed:** `ImageServer::decode` stamps proxy usage with the current server frame marker, maintains active/work lists, distinguishes composite images and existing backing, and obtains or updates an `ImageProcessor` task when processing is needed. The missing-backing path sets an image state and calls `acquireTask` (`0x0031e52c`) followed by `updateTask` (`0x0031fcc0`). It is not an unconditional new decode allocation for every draw call.

**Inferred:** Relevance, existing state and task reuse coordinate demand for an image. Correctly distinguishing “requested,” “being processed” and “ready” can prevent repeated work and premature readiness claims in an independent implementation.

## 3. Loading and decoding have separate state transitions

**Observed:** `ImageProcessor::updateLoading` (`0x00321974`) takes a pending task under a mutex. When image bytes are already held in a shared-buffer reference, it advances state and puts the task on one of two decode-ready lists. When bytes are missing, it creates a `DataLoader::DataRequest`, attaches a state-change handler and submits it to the engine's data loader, unless the processor's suspended flag prevents starting that request.

**Observed:** `ImageProcessor::updateDecoding` (`0x00320ea4`) visits available decoder slots and consumes tasks from one ready list before the other. It checks a task cancellation flag before starting the decode path. A cancelled task changes state, signals completion and wakes the coordinator. For other tasks it obtains bytes from the shared buffer or data request, handles missing header information, and invokes `ImageDecoder::decode` (`0x002fa5e8`).

**Unknown:** The original names and complete semantic classification of both ready-list priorities. A preferred queue is visible; calling it “visible before prefetch” would require tracing the flags that populate those queues. Header parsing also happens in this pipeline, so “all processing is confined to the codec thread” would be too strong.

**Inferred:** A useful transferable contract is to separate fetch readiness, decode readiness, cancellation and completion. The implementation can avoid performing obsolete codec work without assuming that cancelling a UI request instantly cancels an in-flight network or decoder operation.

## 4. Prefetch is stateful and uses the normal pipeline

**Observed:** `ImageServer::prefetch` (`0x00326a6c`) distinguishes video, ordinary and composite image data. For ordinary images it reuses a stored prefetch proxy where one exists; otherwise it creates a proxy, records prefetch state and invokes the normal decode path. Composite-image handling updates its own state/work-list bookkeeping. The video branch does not enter the same ordinary-image prefetch path.

**Inferred:** Prefetch is an owned request with reuse and state, rather than a blanket instruction to load all artwork. This is a useful model for limiting speculative work in Bloob.

**Unknown:** Total speculative byte budget, cross-screen prefetch lifetime, full network prioritization and which high-level UI events invoke this API in the running build. Earlier JavaScript findings cover specific UI triggers separately.

## 5. Frame markers connect visibility to task retirement

**Observed:** `ImageServer::frameDone` (`0x00326fec`) checks proxy last-use markers against the current server frame marker. Its stale-work branches release processor tasks through `lookupTask` (`0x0031e434`) and `releaseTask` (`0x0031e6fc`), reset selected image states, deactivate proxies and update list membership. It increments the server frame marker at the end.

**Observed:** `ImageServer::frameBegin` (`0x00329528`) visits animation-related proxy lists and calls `updateAnimation` (`0x00328010`). It also reacts to a mode change by resetting a selected image state across registered entries. The recovered float argument flow is unreliable; no animation-time formula is inferred from it.

**Inferred:** Visibility and recent use participate in task lifetime, while animated content has an explicit per-frame update path. This does not mean every offscreen image or texture is immediately freed: the graphics cache deliberately retains reusable zero-client-reference resources, as described in the memory study.

**Observed:** `ImageServer::releaseImageData` (`0x003296bc`) decrements a reference count under a mutex and begins unreferenced-data cleanup at zero. Later branches either erase/destroy image data or retain a cache record according to source/backing conditions. The URI comparisons' literal names were not resolved here, so this is not described as one universal delete-on-zero policy.

## 6. Observability covers several ownership domains

| Evidence | What it establishes | What it does not establish |
| --- | --- | --- |
| `Luna::PerformanceCounters::run`, `0x004b9680` | A separate loop reads peak RSS, current RSS and current swap through platform wrappers. | Accuracy of the underlying OS wrappers or a complete memory budget. The loop's timeout argument is not reliably recovered in this export. |
| `PerformanceCounters::updateStats`, `0x004b9bc0` | Frame throughput, several stage differences and frame/resource snapshots are collected under a mutex; periodic reporting uses the platform tick domain. | Hardware GPU timers, physical presentation timestamps or remote-input latency. |
| `PerformanceCounters::getJsonStats`, `0x004b9cf4` | A structured reporting path exists and can obtain current process memory values. | An exposed unauthenticated diagnostics endpoint, inspector attach route or a capture of those values in this session. |
| `ImageServer::getPerformanceCounters`, `0x0032b324` | Image states, active proxies and unique image-data records are counted; decoder memory is obtained through `ImageProcessor::getVideoDecoderMemoryUsage` (`0x0031e328`). | Total process memory, unique driver allocation bytes or the codec/output mode of the main video player. The counter helper belongs to the image-processing subsystem. |
| `NodeJsMT::getHeapStatistics`, `0x004bfd26`, and `getIsolateHeapStatistics`, `0x004bfbf8` | JavaScript isolates have separate sampled heap statistics; worker results can be cached across threads. | A global V8 heap limit or instantaneous synchronized measurements of every isolate. |

**Observed:** `Platform::getTicks` (`0x001049f8`) branches to `cw_getTicks` (`0x00105a94`). The recovered implementation calls `clock_gettime` with numeric clock ID 7, multiplies seconds by 1,000, and converts nanoseconds using the literal `0x431bde83` at `0x00105abc` and a shift of 50 bits. This establishes millisecond ticks for the inspected implementation. The signed 32-bit return also requires wrap-aware differences for long-running use.

**Unknown:** The frame structures' full field names and clock provenance at every producer, measurement overhead and the running app's enabled diagnostic options. A duration computed on the CPU must not be labeled a hardware GPU execution duration just because it surrounds graphics work.

## 7. Lessons for Bloob

These are independent implementation recommendations, not recovered reference code:

1. Give each asset a request identity, relevance, fetch/decode/upload state and cancellation/lifetime owner.
2. Coalesce repeated demand and reuse an existing task or ready resource rather than decoding on every render.
3. Treat cancellation as a state transition that can reject late results; do not assume a network or codec operation can always be forcibly stopped.
4. Keep speculative fetch/decode budget separate from visible asset work and from GPU residency.
5. Retire stale tasks at a predictable application boundary while retaining reusable resources under an explicit cache policy.
6. Keep process RSS, JS heap, decoded CPU pixels, GPU estimates, media-buffer accounting and in-flight reservations separate in diagnostics.
7. Name metrics by the milestone actually observed: request, decode complete, upload complete, render submission and presentation are distinct.

## Remaining questions

The narrow unresolved items are the clip-distance field's producer/units, semantic names of decode queue priorities, configured speculative limits, complete ImageDecoder codec behavior and exact external reporting integration. Runtime-only unknowns include cache hit rate, time spent waiting versus decoding, bytes resident in each domain and actual frame/presentation performance. These are specific gaps, not a claim that the inspected mechanisms are absent.
