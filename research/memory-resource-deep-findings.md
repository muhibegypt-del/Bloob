# Native media memory and graphics resource cache findings

## Evidence and confidence

Source: direct Ghidra headless exports `research/cpu-memory-gpu-decomp.txt`, `research/cpu-memory-gpu-followup-decomp.txt`, and `research/cpu-memory-gpu-lifetime-decomp.txt`, ARM little-endian 32-bit v8 target, SHA-256 `182704ecaa5277ca837544272ac81cd704a1d0b8eb215afea04d143396d7affa`. This is static analysis of the supplied native reference artifact. No runtime memory measurements were made. Function addresses below identify the exported bodies; indirect calls are described by their observed behavior or vtable slot when their concrete target was not recovered. Decompiler signatures and field names are not authoritative type recovery.

**Observed** means directly visible in these function bodies. **Inferred** means an interpretation supported by their interaction. **Unknown** means these exports do not establish it. This report covers media pools and the graphics scratchpad/array-buffer resource cache. ImageServer decode/prefetch/worker policies are being analyzed separately.

## Recovered constants and their scope

| Value | Observed use | Scope and units |
| --- | --- | --- |
| `0x1000` = 4,096 | `Media::Memory::Pool::relocate` allocates backing blocks with `malloc(0x1000)` | Bytes per backing block, not pool or process capacity |
| 8 | Pool rounds old and requested allocation lengths upward to an 8-byte boundary | Byte alignment/granularity |
| `0xfff` = 4,095 | Pool aborts if the rounded new allocation length exceeds this | Maximum rounded allocation is 4,088 bytes; a request of 4,089 rounds to 4,096 and fails the assertion |
| `0x50` = 80 | `ContiguousMemory::create` and `Scratchpad::create` each use `operator new(0x50)` | Wrapper/resource object bytes; excludes payload or GPU storage |
| `0x03200000` = 52,428,800 = 50 MiB | Graphics engine constructor, when no Config is supplied, initializes engine field `+0x34` | Default tracked graphics resource budget, not total application RSS |
| `0x800000` = 8,388,608 = 8 MiB | Configured surface budget and `setMaxSurfaceMemory` subtract this plus one display surface estimate | Reservation in the graphics budget calculation; purpose of the fixed reservation is not established |
| 3 | With no Config, engine field `+0x3c` | Default resource age threshold, in the cache frame-counter domain |
| `0x800` = 2,048 | Minimum dimension cap; increased by powers of two to cover the larger display dimension | Scratchpad width/height cap in pixels, initialized by the engine |
| `0x12d` = 301 | `endFrame` starts pruning unused priority-1 scratchpads when bucket count is at least this | Object count, not bytes |
| `0xb` = 11 | Same pruning requires head age at least this | Cache frame-counter increments, not a fixed elapsed time |
| 100 | `freeUnusedMemory` multiplies an eligible priority-3 head's age by this | Eviction score weighting, not bytes or milliseconds |

## Media pool: actual allocation and reuse

**Observed:** `Media::Memory::Pool::Pool` at `0x0029a8cc` initializes a vector of block pointers and calls `Pool::clear` at `0x0029a8b0`. Clear resets its cursor rather than freeing backing allocations. For an empty vector it sets block index to `-1` and offset to 4,096, causing the next positive allocation to take the new-block path. For a nonempty vector it resets index and offset to zero, permitting reuse of retained blocks.

**Observed:** `Pool::allocate` at `0x0029aba4` delegates to `Pool::relocate` at `0x0029a98c` with a null old pointer and old length zero. Relocate rounds both lengths to multiples of eight, asserts that the rounded requested allocation is at most 4,095, and obtains space within 4,096-byte blocks. Its actual backing allocator is `malloc(0x1000)`; block pointers are recorded in a vector. The implementation can reuse previously recorded block storage after cursor reset. It can also allocate additional blocks, so this is not a fixed-size 4 KiB pool.

**Observed:** An existing allocation can grow in place only when it is the most recent allocation in the current block and the required growth fits. Otherwise relocate obtains a new location and copies the rounded old length with `memcpy`. The shown code does not individually free the old location during this relocation. The arena cursor and backing blocks provide the lifetime boundary.

**Inferred:** This pool reduces small allocation overhead for segment/sample metadata. Static callers include `Media::SegmentData::allocateSample` at `0x0029ad76` and `Media::Memory::Vector<Media::SubSampleInfo>::Writable::reserve` at `0x002a2f4c`. It is distinct from the much larger compressed media payload lifetime described below.

**Observed:** The lifetime export's `Pool::~Pool` at `0x0029a864`, called from `SegmentData::~SegmentData` at `0x0029b5a4`, calls `free` on every recorded backing block, then deletes the vector's pointer storage. Clear retains blocks for reuse; pool destruction releases them.

**Unknown:** Total retained block count, aggregate pool high-water mark and actual workload sizes. The request assertion is not a whole-pool, media-buffer, V8-heap or process memory limit.

## ContiguousMemory: ownership and accounting, not payload allocation

**Observed:** `Media::ContiguousMemory::Allocator::increaseMemory` at `0x000f8b20` and `decreaseMemory` at `0x000f8b3a` only lock a mutex, add/subtract their unsigned length argument from the field at `+4`, and unlock. Neither body allocates, reallocates, frees or enforces a maximum. The class name alone must not be used to describe these two methods as actual memory allocation.

**Observed:** `ContiguousMemory::create` at `0x000f8bf0` allocates an 80-byte object and constructs it. `ContiguousMemory::ContiguousMemory` at `0x000f8b54` begins with reference count one, records the Allocator reference, copy-constructs a `SharedBuffer::Reference`, stores a 64-bit offset, initializes time ranges, and increases the allocator's accounting by `SharedBuffer::Reference::size()`. This wraps an existing shared buffer reference; the shown constructor does not allocate or copy the media payload.

**Observed:** `addRef` at `0x000f8ca4` asserts the reference count is positive, then atomically increments it using ARM exclusive access and barriers. `release` at `0x000f8d9c` similarly validates and decrements; on reaching zero it calls the destructor and deletes the 80-byte object. `~ContiguousMemory` at `0x000f8c18` subtracts the referenced buffer's size, destroys its time ranges, then destroys the `SharedBuffer::Reference`. `Allocator::~Allocator` at `0x000f8a58` aborts if its accounted count remains nonzero, then destroys its mutex. These give explicit checks against references outliving their accounting owner.

**Observed:** Static users cross asynchronous media paths: SegmentData construction acquires references; SegmentData destruction, TrackPump completion/removal/run, SourceBuffer cleanup/eviction/append, and streaming parser paths release references. The cache counter therefore follows wrapper lifetime rather than the duration of one parsing call.

**Observed, with signature uncertainty:** `data` at `0x000f8f90` checks that shared storage exists, requested length is nonzero, and the requested 64-bit range lies within the referenced buffer's base offset and size, then obtains a pointer through `SharedBuffer::Reference::operator[]`. The decompiler splits implicit object and 64-bit arguments awkwardly; the original C++ method annotation identifies `data(unsigned long long, unsigned int) const`. A literal reading of its emitted parameter names would be unsafe.

**Inferred:** The Allocator field is an outstanding referenced-media-byte accounting domain. It is useful for higher-level media eviction/accounting, but no capacity or backpressure can be attributed to these +/- methods. Summing wrapper sizes may also differ from unique physical backing bytes if multiple wrappers reference shared storage.

**Observed:** The lifetime export's `SharedBuffer::Reference::Reference` at `0x0010d5e4` copies the object pointer, reference-count control block and remaining reference/range fields. For a nonnull control block it calls `std::_Sp_counted_base<lock_policy 2>::_M_add_ref_copy`. `SharedBuffer::Reference::~Reference` at `0x0010d680` calls `std::__shared_count<lock_policy 2>::~__shared_count` on the embedded control block owner. This establishes shared-pointer-style reference ownership; this destructor's shown body does not call a method named `Buffer::deref` directly.

**Unknown:** Actual payload allocation, ultimate SharedBuffer backing disposal and deduplication, and any cap imposed by SourceBuffer/DataCache/stream policy. The concrete control-block disposal/backing allocator remains useful next evidence. No assertion here establishes physical contiguity, GPU placement or a whole-process media cap.

## Scratchpad cache: separate object and resident-resource lifetimes

**Observed:** `Gfx::ScratchpadCache::createInstance` at `0x00305d0c` asserts singleton creation, allocates a `0x90`-byte cache, initializes mutexes/trees/counters, and starts with zero frame counter and zero tracked residency. The scratchpad and array-buffer registries are ordered trees keyed by `Math::Hash<3>`.

**Observed:** `acquireScratchpad` at `0x003089d8` rejects zero dimensions, clamps each requested dimension to engine field `+0x40`, and looks up by hash under the cache mutex. A miss creates a scratchpad object using `Scratchpad::create` at `0x004ad718`, which allocates an 80-byte object and initializes it; this body does not allocate GPU storage. A hit increments the resource reference count, asserts dimensions match the hash's existing object, removes it from an unused list if needed, and updates its numeric priority. A supplied output can receive the resulting reference count. An optional boolean sets flag bit `0x02` only on creation in this body.

**Observed:** `releaseScratchpad` at `0x00306984` decrements a positive reference count under the mutex and, when requested by its boolean argument, stamps the current cache frame counter at resource field `+0x2c`. At reference count zero it validates dimensions, priority range 0–3, and unused-list linkage, then appends to the unused FIFO for its priority. The four list heads are at cache offsets `+0x38`, `+0x44`, `+0x50`, `+0x5c`. Release does not itself delete the object or GPU resource. Reacquisition can therefore reuse a zero-client-reference resource.

**Observed:** `acquireArrayBuffer` at `0x0030855c` similarly rejects zero shape parameters, reuses a hash-keyed object or creates one through `ArrayBuffer::create` (`0x004ac4bc`), increments reference count on reuse, validates both shape parameters and detaches a reused object from its unused list. These two shape fields are multiplied when calculating GPU buffer residency.

**Observed:** `destroyScratchPad` at `0x003071dc` unlinks a resource from its unused list, erases its hash-tree entry, and calls its virtual deleting destructor. For flag bit `0x02` it also queues `ScratchpadPrunedArg` notifications carrying the hash to subscribers. This is a full object eviction, unlike simply dropping a graphics handle.

**Inferred:** Flag bit `0x04` represents resident graphics backing in the observed cache operations: it gates uploads, tracking subtraction and handle release. Bit `0x02` requests prune notification. These are behavioral interpretations; the original flag names are not recovered.

## Graphics budget and size calculation

**Observed:** `Gfx::IEngine::IEngine` at `0x002e7ba8` uses a 50 MiB tracked graphics budget when its Config pointer is null, along with `+0x38 = 0` and `+0x3c = 3`. When Config is present, its first three words populate these fields, and the requested budget is reduced by 8 MiB plus `Scratchpad::calculateMemorySize(displayWidth, displayHeight, 5)`. Thus the default null-Config branch and configured branch do not perform the same initial subtraction. Which branch the actual application takes is not established here.

**Observed:** `IEngine::setMaxSurfaceMemory` at `0x002e72e4` accepts a total request only if strictly greater than the same display-surface estimate plus 8 MiB. For a too-small/equal request it traces and returns without assigning a new budget. On success it stores the remaining budget in engine `+0x34` and invokes `ensureMaxSurfaceMemoryUsage`.

**Observed:** The engine initializes dimension cap `+0x40` to the least power of two at least as large as the greater display dimension, with a minimum of 2,048, and shares it with the text engine. This is a per-surface dimension restriction, not a byte allowance.

**Observed:** `Scratchpad::memorySize` at `0x00305900` delegates to `Scratchpad::calculateMemorySize` at `0x004ad820` with width, height and format. For ordinary nonzero formats other than numeric 2 or 3, backend mode zero returns width × height × 4 bytes. Mode two rounds width and height to multiples of 16, multiplies by four, then rounds the product to 4,096 bytes. Mode one doubles both dimensions, normally rounds them to multiples of 64, and multiplies; its small-size branch rounds dimensions to multiples of eight and total storage to 512 bytes. Numeric format 2 has one-byte-per-pixel analogues without the doubling or ×4. Numeric format 3 adds two `OpenGlScratchpad::calculateMemorySize` calls, one full resolution and one at half dimensions rounded upward; its detailed byte formula depends on the unresolved callee.

**Inferred:** Memory accounting is a format/backend-dependent graphics allocation estimate in bytes, rather than a generic width × height × 4 assumption. Numeric format and backend-mode names and the runtime selected backend are unknown. For default backend mode zero, a format-5 1,920×1,080 surface estimates 8,294,400 bytes (about 7.91 MiB), making the setter's reserved amount about 15.91 MiB for that display; this is a conditional calculation, not an observed runtime display or actual budget.

## Admission, eviction and retries

**Observed:** `loadTexture` at `0x003077e0` asserts execution on the recorded graphics thread. If the resource has an associated upload/source field, an already resident scratchpad invokes `Scratchpad::upload` (`0x003059c0`) and succeeds. Otherwise it admits creation only when current tracked usage at cache `+0x68` plus `memorySize()` fits engine `+0x34`. The virtual allocation call executes outside the mutex. Successful allocation increments total tracked usage, and also the `+0x6c` subcounter for numeric priority zero. No corresponding new resident allocation is attempted when the budget check fails.

**Observed:** On over-budget admission or allocation failure, loadTexture repeatedly asks `freeUnusedMemory` to reclaim resources and retries. Once no more resources can be freed, it drains the render pipeline once, advances the cache frame counter by the configured minimum-age amount, and retries the reclamation/admission sequence. If still unsuccessful it traces and returns zero. Thus reclamation is bounded by available evictable resources and one explicit pipeline-drain retry, rather than allocating indefinitely through the policy.

**Observed:** `initRenderTarget` at `0x00307b70` has the same tracked-budget admission and incremental reclamation. Before its single pipeline-drain retry, it searches unused scratchpads from priority 3 down through priority 0 for numeric format zero and matching dimensions, swaps texture backing with the requested resource, destroys the donor, and reports success. This can reuse existing backing without an additional budget charge. It returns failure after the second exhausted sequence.

**Observed:** `setRenderTarget` at `0x00307e34` ensures a backing allocation through initRenderTarget when needed, then retries the virtual target-setting operation while reclaiming unused memory; it likewise drains/advances the counter at most once after reclamation is exhausted. `bindBuffer` at `0x0030756c` enforces the same shared byte budget for array-buffer residency, adding its shape-product byte size to total `+0x68` and buffer subcounter `+0x70`. It reclaims unused memory until success/exhaustion, but has no explicit pipeline-drain retry in its own body. Binding stamps the resource's last-use frame counter at `+0x24`.

**Observed:** `freeUnusedMemory` at `0x003073b8` performs one reclamation per successful call. It examines unused scratchpad FIFO heads from numeric priorities 3, 2 and 1, requiring age at least engine `+0x3c`. Priority-3 head age receives a ×100 score. The exported control flow compares priority 2 against that score and can retain a priority-3 candidate immediately when its score is at least the eligible priority-2 age; on other branches it considers priority 1 against the current score. Equal scores do not force replacement by the later candidate. Priority-0 head is not directly loaded as a scratchpad eviction candidate in this routine. This is priority-sensitive head selection, not a proven globally oldest LRU over all entries.

**Observed:** An elected scratchpad must have zero client references; otherwise an assertion aborts. If resident, its memory estimate is removed from `+0x68`, and the complete scratchpad object is destroyed. If no scratchpad candidate was selected, the routine walks unused array buffers until the first with a resident handle. If that resident buffer is younger than the minimum age it returns zero; it does not continue searching later resident buffers. Otherwise it subtracts the shape-product from both residency counters and calls the virtual handle-release operation, leaving object destruction for a later path.

**Observed:** `ensureMaxSurfaceMemoryUsage` at `0x00307774` does nothing when invoked off the recorded graphics thread. On that thread it locks, reclaims until usage meets the budget or reclamation fails, then drains the render pipeline and advances the frame counter by the configured age before a second reclaim pass when still over budget. It can stop while over budget if no reclaimable resource remains. Therefore a lowered budget is an admission/reclamation policy; this body does not guarantee that all live resources immediately fit the new value.

## Frame-end cleanup and suspension

**Observed:** `endFrame` at `0x00308218`, called by `IEngine::renderFrame` (`0x002e80b4`), locks the cache and walks unused array buffers. Resident buffers whose age meets engine `+0x3c` have their residency counters reduced, their handles released, and their cache objects destroyed through `destroyBuffer` (`0x00308168`). The same pass prunes priority-1 scratchpads while there are at least 301 and the FIFO head is at least 11 counter increments old; each must have zero references. It reduces the tracked resident estimate where needed and fully destroys objects. It stops at 300 objects or when the head is too young, then increments the frame counter by one.

**Inferred:** The default minimum age of three is in rendered-frame-counter units. Pipeline drains deliberately advance this counter, so it is not a wall-clock duration and cannot be converted into a guaranteed number of milliseconds without runtime frame/drain behavior. The 11-age/301-count path is an additional policy specific to one unused scratchpad bucket; it does not bound all scratchpad objects to 300.

**Observed:** `releaseHandles` at `0x00306568`, called from `IEngine::releaseHandles` (`0x002e7188`) and `IEngine::suspend` (`0x002e7384`), walks the entire registered scratchpad tree, emits prune notifications for bit-`0x02` resources, subtracts estimated resident sizes for bit-`0x04` resources, and invokes their virtual handle-release method. It also drops resident array-buffer handles and associated counters. The tree traversal is not restricted to unused scratchpads. The method preserves registered objects for future reuse/recreation and asserts tracked external slot state is empty. Handle release on suspend is therefore separate from zero-reference object eviction.

## Limits of the result and useful next evidence

The recovered 50 MiB value is a default graphics-residency accounting budget. It excludes at least the wrapper/container allocations, media SharedBuffer payloads, V8/Node heaps, decoded-image CPU backing, and other native allocations. The configured surface allowance additionally reserves 8 MiB and an estimated display surface, but these functions do not identify the fixed reservation's owner. No whole-process RSS cap, global memory-pressure callback, or OS memory limit follows from these values.

The exports establish meaningful pooling, explicit shared-reference lifetimes, hash reuse, deferred GPU allocation, priority/age eviction, bounded retry with pipeline draining, frame-end cleanup and suspension handle release. They do not establish real runtime peaks, leak freedom, GPU-driver accounting accuracy, cache hit rate, whether an application Config overrides the defaults, or whether all allocation callers obey these cache paths.

Useful additional addresses if a stronger allocation-level conclusion is needed: `Gfx::Scratchpad::Scratchpad` at `0x00305880`, `Gfx::OpenGlScratchpad::calculateMemorySize` at `0x004ad770`, `Gfx::Scratchpad::upload` at `0x003059c0`, `Gfx::Scratchpad::swapTexture` at `0x003059f0`, `Gfx::ArrayBuffer::create` at `0x004ac4bc`, `Gfx::ScratchpadCache::destroyBuffer` at `0x00308168`, SharedBuffer reference/backing implementation, engine Config construction/callers, and the concrete virtual graphics allocation/release methods. Exact priority enum names and the backend selection globals also remain unresolved. The unusual branch ordering in `freeUnusedMemory` deserves instruction-level confirmation before rewriting it as a simplified global scoring algorithm.
