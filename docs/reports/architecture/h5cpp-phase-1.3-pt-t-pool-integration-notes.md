@page reports_phase_1_3_pt_t_pool_integration_notes Phase 1.3 — pt_t / h5::write / h5::read Pool Integration Notes

**Author:** Winston (System Architect)
**Date:** 2026-05-18 (drafted during overnight CI cycle for #250 Phase 1.2)
**Status:** Design exploration before implementation begins.

---

## Why this note exists

Phase 1.3 of the FAPL multithreading workplan is where the new `worker_pool_t`
gets wired into the actual write paths.  This raises three real design
questions that should be settled with the user before code lands:

1. How pt_t reaches the FAPL.
2. Whether pool dispatch is sync-first or async-pipelined.
3. What happens to the existing #241 per-`pt_t` `h5::filter::threads{N}` API.

The note is deliberately a *design memo*, not a plan — once Steven picks
positions, I'll convert to a concrete sub-task list.

## 1. FAPL access from pt_t

`pt_t::init(const h5::ds_t& handle)` already retrieves the file id via
`H5Iget_file_id(raw)`.  Adding the FAPL is two extra calls:

```cpp
hid_t fid  = H5Iget_file_id(raw);
hid_t fapl = H5Fget_access_plist(fid);
auto pool = h5::impl::resolve_worker_pool(fapl);
H5Pclose(fapl);
H5Fclose(fid);
```

`resolve_worker_pool` returns `std::shared_ptr<worker_pool_t>` (null if no
`h5::threads{N}` property installed).  pt_t stores the shared_ptr as a
member and uses it for chunk dispatch.

No design dispute here.

## 2. Sync-first vs async-pipelined dispatch

### Option A — Sync-first (simplest)

```cpp
void pt_t::write_chunk(off, nbytes, ptr) {
    if (pool_) {
        auto fut = pool_->submit([...]{ return compress(...); });
        auto result = fut.get();    // BLOCKS
        H5Dwrite_chunk(ds, dxpl, result.mask, off, result.nbytes, result.data);
    } else {
        visit_pipeline([&](auto& p){ p.write_chunk(off, nbytes, ptr); });
    }
}
```

- **Pro:** Trivial to implement.  No ordering concerns.  No drain queue.
- **Pro:** Easy regression-test: identical bytewise output as basic_pipeline_t.
- **Con:** Zero parallelism benefit for single-producer pt_t — the producing
  thread blocks on each chunk's compression.  The whole point of the pool is
  lost.
- **Con:** Pool's parallelism only kicks in if multiple pt_t instances
  submit concurrently, which means it benefits *multi-stream* workloads,
  not iex2h5-shaped *single-stream* workloads.

### Option B — Async-pipelined (matches threaded_pipeline_t shape)

```cpp
void pt_t::write_chunk(off, nbytes, ptr) {
    if (pool_) {
        auto fut = pool_->submit([...]{ return compress(...); });
        in_flight_.push_back({std::move(fut), off});
        drain_completed();          // opportunistic
    } else { /* synchronous path */ }
}

void pt_t::drain_completed() {
    while (!in_flight_.empty()) {
        auto& front = in_flight_.front();
        if (front.fut.wait_for(0s) != ready) break;   // not done yet
        auto result = front.fut.get();
        H5Dwrite_chunk(ds, dxpl, result.mask, front.off, result.nbytes, result.data);
        in_flight_.pop_front();
    }
}

void pt_t::flush() {
    while (!in_flight_.empty()) {
        // Block on the next pending future, write in order
        auto result = in_flight_.front().fut.get();
        H5Dwrite_chunk(...);
        in_flight_.pop_front();
    }
}
```

- **Pro:** Producer thread runs ahead of compression — compression of chunk
  N+1 overlaps with H5Dwrite_chunk of chunk N.
- **Pro:** Single-producer streaming workload (iex2h5) gets the parallelism
  benefit.
- **Pro:** Pattern matches existing threaded_pipeline_t — easier to reason
  about and to migrate from #241's API in Phase 1.4.
- **Con:** Deque of in-flight futures adds memory pressure (one closure +
  one std::packaged_task per chunk in flight).
- **Con:** Ordering preserved by deque, but if a worker fails halfway
  through, recovery is more complex than sync path.

### Recommendation

**Option B**.  Sync-first defeats the purpose of having a pool for the
primary use case (streaming sinks).  The deque adds complexity but is
how the existing threaded_pipeline_t already operates; we're reusing a
known-working pattern.

Bounded back-pressure: cap `in_flight_.size()` at some multiple of the
pool's worker count (e.g., 4× workers).  When the cap is reached,
`write_chunk` blocks on `drain_completed()` until the deque drains
below the cap.

## 3. Migration of #241 per-pt_t `h5::filter::threads{N}`

#241 (merged in the v1.12.4 cohort) added:

```cpp
h5::pt_t pt(ds, h5::filter::threads{4});
```

This constructs a per-`pt_t` threaded_pipeline_t with 4 workers.  Phase 1.3
introduces the FAPL-scoped pool.  Three options for the per-pt_t API:

### Option A — Deprecation shim that consults FAPL pool first

```cpp
pt_t(const ds_t& ds, h5::filter::threads workers) {
    // First check: does the file's FAPL have a pool?  If so, ignore
    // the per-pt_t worker count and use the file pool.
    // If not, fall back to the per-pt_t pool (original #241 behavior).
    // Emit a deprecation warning either way.
}
```

- **Pro:** Existing #241 user code continues to compile and run.
- **Pro:** When users adopt `h5::threads{N}` at the FAPL, they get the
  scaled behavior automatically.
- **Con:** Confusing — same syntax, different effect depending on FAPL.
- **Con:** Deprecation warning is a soft signal; many users ignore them.

### Option B — Remove the per-pt_t constructor entirely

```cpp
// h5::filter::threads becomes a deleted tag in this context:
h5::pt_t pt(ds, h5::filter::threads{4});   // compile error
```

- **Pro:** Forces users to adopt the correct (FAPL) API.
- **Pro:** No semantic confusion about what `threads{N}` means.
- **Con:** Breaking change in a minor release.
- **Con:** Users who adopted #241 between v1.12.4 and Phase 1.3's release
  have to rewrite call sites.

### Option C — Keep #241 forever as a different feature

The `h5::filter::threads{N}` constructor stays as a per-pt_t local-pool
option.  The FAPL `h5::threads{N}` is a separate file-scoped option.
Both coexist.

- **Pro:** No migration cost for #241 users.
- **Con:** Two threading APIs in the same library, with subtle semantic
  differences.  Documentation burden.
- **Con:** The local-pool model is exactly the chaos pattern we identified
  in the FAPL design discussion.  Keeping it as an option preserves the
  footgun.

### Recommendation

**Option B**.  Window of #241 adoption is short (v1.12.4 just shipped),
likely no production users yet.  The breaking change is a one-line
substitution at call sites:

```cpp
// Before (#241):
h5::pt_t pt(ds, h5::filter::threads{4});

// After (Phase I):
h5::fd_t fd = h5::create(path, flags, h5::threads{4});
h5::pt_t pt(ds);    // pool picked up from fd's FAPL transitively
```

Document in CHANGELOG.  If Steven wants softer migration, fall back to
Option A (deprecation shim) — easy to convert later.

## 4. h5::write and h5::read

The DAPL `high_throughput` flag was repaired in #242/#244.  Currently
those paths use `H5CPP_DAPL_HIGH_THROUGHPUT` to store a `pipeline_t<basic_pipeline_t>*`
in the DAPL, and `h5::write` / `h5::read` use it for chunked transfers.

For Phase 1.3 integration, two approaches:

### Approach 1 — DAPL pipeline pointer becomes pool-aware

Modify the DAPL pipeline so that when its containing FAPL has a pool, the
pipeline dispatches compress through the pool.  Requires the pipeline to
hold a weak reference to the FAPL pool.

Mechanically tricky — DAPL doesn't directly know its containing FAPL;
that's a runtime fd-level relationship.  The pipeline would have to be
resolved at `h5::write` time from the fd's FAPL.

### Approach 2 — h5::write / h5::read consult FAPL directly

Just like pt_t, h5::write / h5::read look up `h5::impl::resolve_worker_pool`
on the fd's FAPL.  If a pool is present AND the DAPL has `high_throughput`,
use the pool.  Otherwise fall through to existing behavior (synchronous
DAPL pipeline or plain H5Dwrite).

Cleaner.  Matches pt_t's pattern.  DAPL pipeline pointer stays unchanged.

### Recommendation

**Approach 2.**  Symmetric with pt_t.  No DAPL-FAPL coupling needed.

## 5. Sub-task list

Once design positions are locked, Phase 1.3 breaks down as:

1. `pt_t` integration (Option B from §2 + Option B from §3)
   - Add `shared_ptr<worker_pool_t> pool_` member
   - Resolve pool in `init()`
   - Replace `visit_pipeline(...write_chunk)` call sites with a unified
     `pt_t::write_chunk` member that branches pool vs. variant
   - Add in-flight deque + drain logic for async pipelining
   - Add back-pressure cap (4× pool worker count)
   - Remove `h5::filter::threads{N}` constructor from #241
   - Update tests in `test/H5Dappend.cpp` to use FAPL idiom

2. `h5::write` integration (Approach 2 from §4)
   - Resolve pool from fd's FAPL
   - When pool present + DAPL high_throughput set: submit closures to pool,
     drain in order, H5Dwrite_chunk on the calling thread
   - Otherwise: existing DAPL pipeline / standard H5Dwrite paths

3. `h5::read` integration (Approach 2 from §4)
   - Symmetric with h5::write but for decompression

4. Tests
   - End-to-end: streamed append with FAPL pool, verify parallelism via
     timing (writes scale with pool size, up to compress throughput limit)
   - Multi-fd isolation: two files with `h5::threads{4}` each → each owns
     its own pool, total threads = 2×4
   - High-throughput DAPL on a file with no pool → falls back to
     synchronous (regression test)

## 6. Open questions for Steven

1. **§2 dispatch model**: confirm Option B (async-pipelined) over Option A
   (sync-first).
2. **§3 #241 migration**: confirm Option B (break #241 API) over Option A
   (deprecation shim).
3. **§4 h5::write/read integration**: confirm Approach 2 (consult FAPL
   directly) over Approach 1 (DAPL pipeline becomes pool-aware).
4. **Back-pressure cap**: 4× worker count?  Configurable?  Default-only?

Implementation begins after these are settled.  Expected effort 5-7 days
once positions are locked.

---

## Post-implementation update (2026-05-18)

### Phase 1.3.2 outcome — pt_t integration shipped

Implemented end-to-end on PR #251.  pt_t now resolves the FAPL pool +
backpressure cap at init time and uses async-pipelined dispatch when the
pool is present.  All six `visit_pipeline → write_chunk` call sites
route through a single `dispatch_chunk` helper that picks pool or
synchronous based on `pool_` presence.

Filter chain snapshot is captured into a POD struct and copied by value
into each task closure.  Per-chunk allocation of raw input buffer + two
scratch buffers; thread-local scratch in the pool deferred as an
optimization.

flush() drains all in-flight futures in submission order before
returning.  Bytewise round-trip verified across synchronous /
pool-default / pool-with-explicit-cap and gzip-compressed datasets.

### Phase 1.3.3 design question — h5::write/read integration shape

The existing `h5::write` path looks like:

```cpp
hid_t dapl = h5::get_access_plist(ds);
if (DAPL has high_throughput && layout == H5D_CHUNKED) {
    pipeline_t<basic_pipeline_t>* pipe;
    H5Pget(dapl, H5CPP_DAPL_HIGH_THROUGHPUT, &pipe);
    pipe->write(ds, offset, stride, block, count, dxpl, ptr);
    // ↑ pipeline_t<>::write decomposes buffer into chunks via
    //   split_to_chunk_write and calls write_chunk per chunk.
} else {
    // Standard H5Dwrite path.
}
```

`pipeline_t<>::write → split_to_chunk_write → write_chunk per chunk` is
the chunk-decomposition machinery shared by both basic_pipeline_t and
threaded_pipeline_t.  pt_t reuses this implicitly via its `pipeline`
member.

To integrate the FAPL pool into `h5::write`, three options:

#### Option a — Add pool path in h5::write, duplicate chunking logic

In h5::write, before the existing `use_pipeline` branch, check the file's
FAPL for a pool.  If pool present + chunked layout: implement chunk
decomposition + per-chunk pool submission directly in h5::write's
function body.  Reuse the same compress-closure pattern as
`pt_t::write_chunk_via_pool`.

- **Pro:** Local to h5::write; doesn't touch pipeline_t<>.
- **Con:** Duplicates chunk decomposition logic that pipeline_t<>::write
  already handles correctly (including edge cases — strided / blocked
  selections).
- **Con:** When h5::read also needs pool integration, the duplication
  repeats.

#### Option b — Pool-aware pipeline alternative in the variant

Introduce a third CRTP descendant `pool_pipeline_t` (sibling of
basic and threaded) that holds a reference to an external worker_pool_t
+ backpressure cap, and overrides `write_chunk_impl` to dispatch via
the pool.  pt_t's variant becomes
`{basic, threaded, pool}`; the variant alternative is selected at
construction based on FAPL pool presence.

Then `pipeline_t<>::write → split_to_chunk_write → write_chunk_impl`
naturally routes through whichever alternative is in the variant.

- **Pro:** Reuses existing chunk decomposition infrastructure.
- **Pro:** pt_t and h5::write both benefit transparently.
- **Con:** Adds another variant alternative; #241's threaded alternative
  remains in the picture until Phase 1.4 removes it.
- **Con:** pool_pipeline_t needs to own an in-flight deque and a
  drain mechanism — currently those live on pt_t.  Migrate them to
  the pipeline type.

#### Option c — Refactor split_to_chunk_write into a free function

Extract `split_to_chunk_write` from `pipeline_t<>` into a free function
that takes a callable per-chunk dispatcher.  Both pt_t and h5::write
call it with their own dispatcher (synchronous or pool-aware).

- **Pro:** Cleanest separation of concerns.
- **Pro:** No new variant alternatives.
- **Con:** Largest refactor — touches several call sites.
- **Con:** Existing pipeline_t<>::write / read functions need to be
  updated to call the free function with a synchronous adapter.

### Recommendation

**Option b** in the near term: introduce `pool_pipeline_t` and let pt_t's
variant pick the right one at construction based on FAPL pool presence.
This:

- Subsumes Phase 1.4 (the #241 threaded alternative is replaced by
  pool_pipeline_t — same conceptually, different ownership).
- Naturally extends to h5::write and h5::read via the existing
  pipeline_t<>::write/read machinery.
- Keeps the in-flight deque and drain logic in one place (the
  pool_pipeline_t implementation), removed from pt_t.

This means re-doing some of the pt_t-side code that was just added in
Phase 1.3.2, but the existing tests act as regression guards for the
new arrangement.  Net code change is smaller than Phase 1.3.2 was.

Estimated effort for Option b: 2-3 days from a clean start.

If Steven prefers simpler-but-duplicated (Option a) for tactical reasons,
let me know.  Option c is the architectural ideal but the largest scope.

### Status at end of overnight session

Branch `250-feature-fapl-worker-pool` is in a stable state with:

- Phase 1.1 (slot scaffolding) ✔
- Phase 1.2 (worker pool with submit/wait_idle) ✔
- Phase 1.3.1 (h5::backpressure FAPL property) ✔
- Phase 1.3.2 step 1 (pt_t FAPL resolution) ✔
- Phase 1.3.2 step 2 (pt_t pool dispatch) ✔
- Phase 1.3.3 (h5::write/read pool dispatch) ⏸ design decision needed
- Phase 1.4 (remove #241 ctor) ⏸ contingent on Phase 1.3.3 shape
- Phase 1.5 (TSAN tests) ⏸ pending earlier phases
- Phase 1.6 (docs + final commit) ⏸ pending earlier phases

CI on the latest commit is green across the matrix.  PR #251 stays
draft until Phase 1.3.3 and 1.4 land.
