@page reports_multithreading_pipeline_state h5cpp Multithreaded Filter Pipeline — Current State (v1.12.7)

@warning **Status (v1.12.7): the FAPL-scoped parallel pipeline is currently inert through the public API.**
`h5::threads{N}` installs the worker pool on the user's FAPL, but the write/read dispatch sites
re-resolve it via `H5Fget_access_plist`, which does **not** preserve the inserted property — so
`impl::resolve_worker_pool` returns `nullptr` and every path falls back to the synchronous
`basic_pipeline_t`. `pool_pipeline_t` therefore does not execute in this release; the parallel
tests still pass because the synchronous fallback produces correct data. The design described below
is accurate as *intent*. The activation fix (and two related `high_throughput` direct-chunk defects)
is tracked in [issue #286](https://github.com/vargalabs/h5cpp/issues/286).

h5cpp v1.12.7 is architected around a multithreaded filter pipeline built on three orthogonal pieces:

| Piece | Role | User-facing API | Where it lives |
|---|---|---|---|
| **FAPL worker pool** | Decide *whether a pool exists at all, and how many workers* | `h5::threads{N}` (or `h5::threads{}` → `hardware_concurrency`) on a FAPL | `h5cpp/H5Pthreads.hpp` |
| **DAPL high-throughput flag** | Per-dataset opt-in to the parallel filter pipeline | `h5::high_throughput` on a DAPL | `h5cpp/H5Pdapl.hpp` |
| **`pool_pipeline_t`** | Actual filter-chain engine that submits per-chunk compress (or decompress) closures to the FAPL pool | Auto-selected when a pool is present; sibling of `basic_pipeline_t` | `h5cpp/H5Zpipeline_pool.hpp` |

The implementation is **pure C++20 standard library** — `std::jthread`, `std::future`,
`std::packaged_task`, `std::deque`, `std::shared_ptr`, `std::atomic::wait` / `notify_one`.
No third-party concurrency dependency. No lock-free queue work was needed for the
target latency profile.

Two distinct consumer paths exist today:

1. **One-shot writes/reads** (`h5::write` / `h5::read`) gated by `h5::high_throughput` on the DAPL.
2. **Streaming append** (`h5::append` / `h5::pt_t`) gated by the file's FAPL having `h5::threads{N}` installed.

Both paths land on the same `pool_pipeline_t` underneath; the difference is *who builds the pipeline instance* and *when it's destroyed*.

---

## 2. Layer 1 — FAPL Worker Pool (`h5cpp/H5Pthreads.hpp`)
### 2.1 User-facing surface

```cpp
// Fixed worker count
h5::fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC, h5::threads{8});

// hardware_concurrency-derived
h5::fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC, h5::threads{});

// Override the default 8× per-pipeline back-pressure cap
h5::fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC, h5::threads{8} | h5::backpressure{32});
```

`h5::threads{N}` is a FAPL property; `h5::backpressure{K}` is a sibling FAPL property
that overrides the per-pipeline outstanding-future cap (default `8 × worker_count`).
Both are composed onto a FAPL via the existing `operator|` chain that all property
flags use.

### 2.2 Storage mechanism

`H5Pthreads.hpp` registers the worker pool against HDF5's property-list machinery
through the same `H5Pinsert2` + slot pattern that `h5::high_throughput` uses for the
DAPL — with one critical difference. The DAPL flag allocates a **fresh** pipeline
scratch buffer per FAPL copy (per-write scratch state). The threads property
**shares** one live `worker_pool_t` across every FAPL copy via
`std::shared_ptr<worker_pool_t>`:

| Mechanism | DAPL `high_throughput` | FAPL `h5::threads` |
|---|---|---|
| Slot type | `worker_pool_slot_t` holding `pipeline_t<basic_pipeline_t>*` (scratch) | `worker_pool_slot_t` holding `std::shared_ptr<worker_pool_t>` |
| Copy callback | Allocate **fresh** scratch buffer (per-write scratch is not shareable) | Refcount-bump on the shared pool (workers ARE the shared resource) |
| Close callback | Free the scratch buffer | Drop the slot; pool destroyed when last live FAPL copy releases |
| Lifetime | Per-write; destroyed at the end of the call | File-scope; lives until the FAPL (and every copy HDF5 makes of it) is closed |

Worker threads are `h5::detail::stoppable_thread_t` (a `std::jthread` wrapper); on
pool destruction every worker receives `request_stop()` and joins cleanly. No
explicit shutdown call is needed.

### 2.3 Pool internals

`worker_pool_t` is a generic compute pool — **HDF5-agnostic at this layer**.
Workers pull type-erased tasks off a single doorbell-signalled queue and execute
them. The `submit<F>(callable)` API returns a `std::future<R>` via
`std::packaged_task` wrapped in a `std::function<void()>` for storage.

| Property | Value |
|---|---|
| Queue topology | Single MPMC-ish: `std::mutex` + `std::queue` + `doorbell_t` notify |
| Blocking primitive | `std::atomic::wait` / `notify_one` (futex on Linux, WaitOnAddress on Windows) — zero-cost when uncontended |
| Worker count | Fixed at construction; `n == 0` resolves to `hardware_concurrency()` |
| Cancellation | Cooperative via `std::stop_token`; destructor's wait-then-stop-then-join sequence is correct under concurrent submit |
| Reusability | Same pool serves the filter pipeline today; designed to serve a future async executor (`h5::async::*`) too |

---

## 3. Layer 2 — DAPL `high_throughput` Flag (`h5cpp/H5Pdapl.hpp`)

`h5::high_throughput` is the older of the two opt-ins. It pre-dates the FAPL pool and
ships a per-dataset scratch pipeline that historically operated synchronously on the
calling thread.

```cpp
h5::ds_t ds = h5::create<float>(fd, "grid", h5::current_dims{N},h5::high_throughput);
h5::write(ds, data);   // routes through the pipeline scratch path
```

### 3.1 What changed in v1.12.6

The `H5Dwrite.hpp` and `H5Dread.hpp` dispatch sites now check the file's FAPL for an
`h5::threads{N}` pool **first**:

| Condition (write side) | Behavior |
|---|---|
| DAPL `high_throughput` set + dataset chunked + FAPL has `h5::threads{N}` | Construct a local `pool_pipeline_t` borrowing the shared pool, dispatch through it, drain on destruction |
| DAPL `high_throughput` set + dataset chunked + FAPL has **no** pool | Use the per-DAPL `pipeline_t<basic_pipeline_t>` scratch (legacy path, synchronous) |
| DAPL `high_throughput` set + dataset **not** chunked | Fall through to `H5Dwrite` (the pipeline requires `H5Dwrite_chunk`; contiguous/compact datasets bypass it). Documented as a workaround for issue #242's Windows-MSVC segfault. |
| DAPL `high_throughput` **not** set | Standard `H5Dwrite` path; no filter pipeline involvement |

Read side mirrors the write side. Rank-1 chunked reads dispatch through the pool's
parallel-decompress path (Phase 1.5 — landed); higher-rank reads fall through to the
synchronous base-class path until rank-N parallel decomposition is implemented.

### 3.2 Lifecycle (one-shot write)

```
h5::write(ds, data, h5::high_throughput-on-DAPL):
  ┌──────────────────────────────────────────────────────────────┐
  │ resolve fid + fapl from ds                                   │
  │ pool = impl::resolve_worker_pool(fapl)                       │
  │ cap  = impl::resolve_backpressure(fapl, pool->worker_count())│
  │ pool_pipeline_t pipe{pool, cap}    ← short-lived local       │
  │ pipe.set_cache(dcpl, elem_size)    ← snapshot filter chain   │
  │ pipe.write(ds, offset, ..., dxpl, data)                      │
  │ ┐  for each chunk:                                           │
  │ │    copy chunk bytes to a worker-owned buffer               │
  │ │    pool->submit(closure capturing fc + bytes)              │
  │ │    push future to in_flight_ deque                         │
  │ │    drain ready futures (opportunistic, non-blocking)       │
  │ │    while in_flight_.size() >= cap: drain blocking          │
  │ ┘                                                            │
  │ ~pool_pipeline_t() ← drains every remaining future, in       │
  │                     submission order, before releasing       │
  │                     the shared pool refcount                 │
  └──────────────────────────────────────────────────────────────┘
```

Producer memory is bounded at `cap × max_chunk_compressed_size` bytes per pipeline
instance.

---

## 4. Layer 3 — Packet-Table Streaming (`h5cpp/H5Dappend.hpp`)

The packet-table path is the *long-lived* sibling of the one-shot DAPL path. A
`h5::pt_t` holds a `std::variant<unique_ptr<basic_pipeline_t>, unique_ptr<pool_pipeline_t>>`
and lives as long as the user's streaming session.

### 4.1 User-facing surface

```cpp
h5::fd_t fd = h5::create("stream.h5", H5F_ACC_TRUNC, h5::threads{8});
h5::pt_t pt = h5::create<float>(
  fd, "samples", h5::current_dims{0}, h5::max_dims{H5S_UNLIMITED}, h5::chunk{1024} | h5::gzip{6});
for (float sample : producer)
    h5::append(pt, sample);            // dispatch through the pool
h5::flush(pt);                          // drain in-flight closures
```

The decision between `basic_pipeline_t` (synchronous) and `pool_pipeline_t`
(async-dispatched) happens **once**, at `pt_t::init()`, when the dataset's file FAPL
is examined:

```cpp
hid_t fapl = H5Fget_access_plist(fid);
if (auto pool = impl::resolve_worker_pool(fapl)) {
    const unsigned cap = impl::resolve_backpressure(fapl, pool->worker_count());
    pipeline.emplace<unique_ptr<pool_pipeline_t>>(
        std::make_unique<pool_pipeline_t>(std::move(pool), cap));
} else {
    // default basic_pipeline_t stays — synchronous behavior, identical to pre-Phase-I
}
```

No DAPL flag is involved. The decision is made at packet-table-construction time and
cached for the lifetime of the `pt_t`.

### 4.2 Why the packet-table needs its own surface

Streaming writes have different semantics from one-shot:

| Concern | One-shot `h5::write` | Streaming `h5::append` |
|---|---|---|
| Pipeline lifetime | Per-call, destroyed on return | Per-session, lives until `pt.flush()` / destructor |
| Drain trigger | Pipeline destructor (always) | `pt.flush()`, `~pt_t()`, dataset close |
| User control of cadence | None — drain is implicit | Explicit: `h5::flush(pt)` is the user-visible barrier |
| Backpressure policy | Block the calling thread on the front future when the deque fills | Same — but the producer is typically a tight loop, so saturation feedback matters more |

The variant lets `pt_t` carry the right pipeline type without runtime overhead beyond
the variant discriminator; `visit_pipeline()` resolves through both alternatives
duck-typed (they share the `pipeline_t<>` base CRTP interface).

### 4.3 Drain on close

The packet-table destructor calls `flush()`, which calls `pipeline_->drain()` on the
pool variant. `drain()` blocks until every submitted future has completed and the
result has been written to the dataset. This guarantees that when a `pt_t` goes out
of scope:

1. Every appended record is durably in the HDF5 chunk cache.
2. Every closure submitted to the pool has finished.
3. The pool's shared refcount drops back to its pre-`pt_t` state.

If the user forgets to call `h5::flush(pt)`, the destructor still does the right
thing — at the cost of blocking the destructor until the worker pool drains.

---

## 5. `pool_pipeline_t` — Filter Chain Internals

### 5.1 Per-chunk closure pattern

Both the write and (rank-1) read paths snapshot the filter chain into a
**POD captured by value** in the closure:

```cpp
struct filter_chain_t {
    filter::call_t filter[H5CPP_MAX_FILTER];
    unsigned       flags[H5CPP_MAX_FILTER];
    std::size_t    cd_size[H5CPP_MAX_FILTER];
    unsigned       cd_values[H5CPP_MAX_FILTER][H5CPP_MAX_FILTER_PARAM];
    hsize_t        tail;
};
filter_chain_t fc{};   // memcpy from pipeline_t<>'s filter, flags, cd_size, cd_values
```

This means each submitted closure is fully self-contained: it owns its input buffer
(via `std::unique_ptr<std::byte[]>`), its scratch buffers (allocated inside the
worker), and a copy of the filter parameter values. No shared mutable state crosses
the worker boundary. The `pool_pipeline_t` instance itself is read-only from the
worker's perspective during the closure body.

The trade-off is the per-chunk memcpy of the filter chain POD. For typical filter
chains (1–3 filters, ~64 bytes per slot) this is negligible against the bytes-moved
cost of the chunk itself.

### 5.2 Write path

```
write_chunk_impl(offset, nbytes, src):
  1. Snapshot filter_chain_t into closure capture.
  2. Allocate worker-owned input buffer; memcpy(src, nbytes).
  3. Submit closure to pool:
       in worker:
         allocate two scratch buffers (filter_scratch_bound(nbytes) each)
         apply filter[0] from raw → wbuf0
         for j=1..tail-1: apply filter[j], ping-pong wbuf0/wbuf1
         shrink-copy result into a right-sized output buffer
         return {data, nbytes, mask, offset}
  4. Push future to in_flight_ deque.
  5. drain_in_flight(blocking=false)        ← opportunistic ready-future drain
  6. while in_flight_.size() >= cap_:
        drain_in_flight(blocking=true)       ← backpressure
```

H5Dwrite_chunk runs on the **calling thread** when the future completes, not on the
worker. HDF5's chunk-write API is not thread-safe across multiple datasets in one file,
so I/O is deliberately serialised on the producer thread. Workers handle filter
compute only.

### 5.3 Read path (rank-1 only, today)

```
read(ds, offset, ..., ptr):
  if !pool_ || rank != 1: fall through to basic_pipeline_t::read (sync)
  for each chunk j:
    on main thread:
      H5Dread_chunk into compressed buffer  ← HDF5 I/O on calling thread
    submit decompress closure to pool:
      in worker:
        apply inverse filter chain (filter[tail-1] → … → filter[0])
        return {data, dst_offset, copy_size}
    push read future to read_in_flight_ deque
  drain read_in_flight_:
    when future ready: memcpy into user buffer at dst_offset
    blocking when cap_ reached (backpressure)
```

Read parallelism wins on compressed datasets where the decompression dominates I/O.
For uncompressed (or weakly-compressed) data the calling-thread I/O is the bottleneck
and the pool sees idle time.

### 5.4 What is *not* in the read path yet

| Capability | Status |
|---|---|
| Rank-1 parallel decompress | ◇ Implemented but not engaged — the FAPL pool is dropped before dispatch (#286); falls back to synchronous read |
| Rank-N parallel decompress | ◇ Falls through to synchronous `pipeline_t<>::read` |
| Speculative read-ahead | ◇ Future — the structure (read_in_flight_ deque, separate drain) is in place; not currently triggered |
| Parallel I/O across multiple datasets | ◇ Not in scope — HDF5 chunk I/O is single-writer per file |

---

## 6. Cross-Path Behavior Matrix

How the two opt-in flags compose:

| FAPL state | DAPL state | Dataset layout | Effective pipeline |
|---|---|---|---|
| no `h5::threads{N}` | `h5::high_throughput` set | chunked | **`basic_pipeline_t` via DAPL scratch** (legacy synchronous filter chain) |
| no `h5::threads{N}` | `h5::high_throughput` set | contiguous/compact | Standard `H5Dwrite` (pipeline bypassed) |
| no `h5::threads{N}` | unset | any | Standard `H5Dwrite` / `H5Dread` |
| `h5::threads{N}` | `h5::high_throughput` set | chunked | **`pool_pipeline_t`** — local instance per call, borrows the shared pool |
| `h5::threads{N}` | `h5::high_throughput` set | contiguous/compact | Standard `H5Dwrite` (pipeline bypassed, pool unused) |
| `h5::threads{N}` | unset (one-shot path) | any | Standard `H5Dwrite` / `H5Dread` — DAPL flag is the per-dataset opt-in for one-shot calls |
| `h5::threads{N}` | n/a (packet-table path) | chunked | **`pool_pipeline_t`** via `pt_t::init()` — long-lived; drains on `pt.flush()` / destructor |
| `h5::threads{N}` | n/a (packet-table) | contiguous/compact | (Packet tables require chunking by construction — this combination doesn't arise) |

Two consequences worth highlighting:

1. **Setting `h5::threads{N}` on the FAPL alone is not enough** for one-shot writes —
   the dataset's DAPL must also carry `h5::high_throughput`. This is deliberate:
   it lets users carve out hot datasets from the pool while keeping cold ones on
   the synchronous path.
2. **Packet tables don't need the DAPL flag.** `pt_t::init()` checks the FAPL directly
   and binds the pool to the packet-table's lifetime. The DAPL flag is bypassed.

---

## 7. Backpressure Policy

| Setting | Value |
|---|---|
| Default cap | `8 × worker_count` outstanding compress (or decompress) futures per pipeline instance |
| Override | `h5::backpressure{K}` on the FAPL (separate from `h5::threads{N}`) |
| Compile-time floor | `H5CPP_FAPL_BACKPRESSURE_DEFAULT_FACTOR` macro (default 8) |
| Saturation behavior | Block the calling thread on `in_flight_.front().get()` until headroom returns |
| Memory bound | `cap × max_chunk_compressed_size` bytes per active pipeline instance |

The default factor of 8 keeps all workers fed during steady-state streaming while
bounding memory growth. Long pipelines (many filters) with large chunks may need a
lower cap; CPU-bound workloads with small chunks can raise it.

No exception is ever thrown for backpressure — the calling thread simply blocks until
a future completes. This matches `basic_pipeline_t`'s never-blocks-never-throws
contract semantically (the synchronous path never queues, so there's nothing to wait
on; the async path queues but the queue is bounded).

---

## 8. Current Limitations (honest list)

| Limitation | Impact | Path forward |
|---|---|---|
| Read parallelism only for rank-1 datasets | Higher-rank reads (matrices, tensors) bypass the pool | Generalise the chunk decomposition in `pool_pipeline_t::read` to rank-N |
| Filter chain snapshot is memcpy'd per chunk | Cost is ~64 bytes × max_filter × 2 (write); negligible in practice but measurable on micro-bench | Hold the `filter_chain_t` in a per-pipeline immutable struct shared by closure capture-by-reference |
| H5Dwrite_chunk on calling thread | I/O serialises through one thread regardless of worker count | A dedicated I/O thread (Phase II?) would let the calling thread return earlier; needs careful interaction with HDF5's library lock |
| No speculative read-ahead | Read pipeline overlaps decompress with calling-thread I/O but doesn't fetch ahead | Add an I/O-prefetch budget on the read path |
| Pool is single-queue MPMC | All workers pull from one mutex-guarded queue; uncontended fast path is cheap (futex), but a multi-queue work-stealing pool would scale better on >16-core machines | Profile-driven; not motivated by current workloads |
| No worker affinity / NUMA hints | Workers run wherever the OS scheduler places them | Add an opt-in `h5::cpu_affinity{...}` FAPL property if profiling shows wins |
| `h5::backpressure{K}` is per-pipeline, not per-dataset | All one-shot writes through the pool share the same cap | A per-DAPL override would be a small addition if needed |
| `h5::async::*` descriptor family doesn't yet route through this pool | Async-mode dispatch (Phase II type-level discrimination) is separate from the filter pipeline pool today | Unify the executor: the FAPL pool serves both filter compute and async-mode operation submission |

None of these are correctness issues. They're optimisation headroom.

---

## 9. Profiling Recipe

To verify the pool is doing useful work:

```cpp
// 1. Establish synchronous baseline
auto fd_baseline = h5::create("baseline.h5", H5F_ACC_TRUNC);
auto pt_baseline = h5::create<sample_t>(
  fd_baseline, "/x", h5::chunk{1024} | h5::gzip{6}, h5::max_dims{H5S_UNLIMITED});
for (auto s : samples) h5::append(pt_baseline, s);
h5::flush(pt_baseline);

// 2. Same workload, FAPL pool installed
auto fd_pool = h5::create("pool.h5", H5F_ACC_TRUNC, h5::threads{});
auto pt_pool = h5::create<sample_t>(
  fd_pool, "/x", h5::chunk{1024} | h5::gzip{6}, h5::max_dims{H5S_UNLIMITED});
for (auto s : samples) h5::append(pt_pool, s);
h5::flush(pt_pool);
```

Useful signals:

- `top -H` while the run is in flight — worker threads should show non-trivial CPU time when the producer is steady-state appending.
- Wall-clock ratio of (2) to (1) is the practical speedup; expect ~`min(worker_count, compress_cost / serialise_cost)` for compressed streams.
- For uncompressed appends the pool sees idle time and (2) ≈ (1) — no regression, but no win either.

The `examples/optimized/` and `examples/packet-table/` cookbook chapters carry
running variants of this recipe.

---

## 10. Source Index

| File | Lines | Role |
|---|---|---|
| `h5cpp/H5Pthreads.hpp` | ~300 | FAPL property + `worker_pool_t` + slot lifecycle |
| `h5cpp/H5Pdapl.hpp` | (existing) | DAPL `h5::high_throughput` flag + per-DAPL scratch pipeline |
| `h5cpp/H5Zpipeline.hpp` | ~460 | CRTP base `pipeline_t<Derived>`, `basic_pipeline_t`, `romio_pipeline_t` stub, `hadoop_pipeline_t` stub |
| `h5cpp/H5Zpipeline_pool.hpp` | ~400 | `pool_pipeline_t` — sibling CRTP descendant, write + rank-1 read |
| `h5cpp/H5Dwrite.hpp` | (existing) | One-shot write dispatch — `h5::high_throughput` + FAPL pool branch |
| `h5cpp/H5Dread.hpp` | (existing) | One-shot read dispatch — same DAPL+FAPL gate as write |
| `h5cpp/H5Dappend.hpp` | (existing) | Packet-table `h5::pt_t`, `h5::append`, `h5::flush` — variant + `pt_t::init()` pool resolution |
| `h5cpp/detail/doorbell.hpp` | (small) | `doorbell_t` atomic wait/notify wrapper used by the worker queue |
| `h5cpp/detail/stoppable_thread.hpp` | (small) | `std::jthread`-backed worker with cooperative stop |

---

## 11. Bottom line

For h5cpp v1.12.7 users:

- **Parallel filter compute is wired but currently inert (#286).** `h5::threads{}` on the FAPL is accepted and the API behaves correctly, but the pool is not resolved at the write/read dispatch sites — work runs on the synchronous path until the activation fix lands.
- **For one-shot writes**, `h5::high_throughput` on the DAPL is the per-dataset opt-in (synchronous today).
- **For streaming with `h5::pt_t` / `h5::append`**, the FAPL flag is the trigger — packet tables auto-detect (synchronous today).
- **Always pair both with chunked datasets and a non-trivial filter chain** (`h5::gzip{N}`, `h5::shuffle`, `h5::nbit`, `h5::scaleoffset`, custom). Without filters there's nothing to parallelise.
- **Drain points are explicit and synchronous.** `~pool_pipeline_t()` blocks until every closure completes; `h5::flush(pt)` does the same for streaming. No fire-and-forget mode.

Everything else — read parallelism for rank-N, I/O-thread offload, work-stealing, NUMA affinity — is *future work but not blocking*. The shipped state is a clean, debuggable, dependency-free implementation that wins on the most common bottleneck (gzip-compressed time-series streaming) without changing call sites.
