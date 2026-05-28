@page reports_fapl_multithreading_workplan h5cpp Multithreading Architecture — FAPL Worker Pool (Phase I) → Async Mode (Phase II)

**Author:** Winston (System Architect)
**Date:** 2026-05-18
**Scope:** Two-phase plan to bring h5cpp from synchronous-by-default to fully thread-safe with
parallel filter compression. Phase I introduces an FAPL-scoped worker pool that parallelizes
compression/decompression for all I/O paths uniformly. Phase II layers a per-fd executor on top
that confines all HDF5 C API calls to one thread per file, enabling full multithreaded calls
into h5cpp with compile-time blocking of raw C API mixing on `async_fd_t`. Both phases use the
same "hidden pointer in HDF5 property list skip list" pattern validated in #242/#244.
**Related issues:** #242 (DAPL pipeline copy callback — landed), #241 / #243 (per-pt_t threading
— to be closed unmerged), supersedes
[[h5cpp-async-mode-compile-time-thread-safety-design]] which is now folded into Phase II.
**Status:** Approved 2026-05-18. Phase I begins as soon as #244 merges.

---

## 1. Executive Summary

Threading config in h5cpp belongs at **file scope** (FAPL), not per-dataset (DAPL) and not
per-`pt_t` (constructor argument). The path forward stacks two layers on the same FAPL:

| Phase | Scope | Adds | Effort |
|---|---|---|---|
| **I** | Workers | `h5::threads{N}` FAPL property → shared `worker_pool_t` for compression | 2-3 weeks |
| **II** | Executor | `h5::async` FAPL property → per-fd executor + compile-time blocking of C API | 1-2 months follow-on |

Each phase uses the `H5Pinsert2` + `shared_ptr`-in-slot pattern proven by the #244 fix. Phase II
is strictly additive; it does not refactor Phase I.

## 2. The Pattern (Validated in #244)

Both phases reuse a single mechanism: hide a heap-allocated resource handle inside an HDF5
property list using `H5Pinsert2`, with copy and close callbacks that maintain correct lifetime
under HDF5's internal property propagation.

```cpp
namespace h5::impl {
    template <class Resource>
    struct slot_t {
        std::shared_ptr<Resource> resource;
    };

    // Copy callback: HDF5 cloned the property bytes (the slot pointer is now
    // duplicated). Allocate a NEW slot whose shared_ptr aliases the same
    // resource — refcount++. Net effect: every FAPL copy shares the resource.
    template <class Resource>
    inline herr_t slot_copy_cb(const char*, size_t, void* value) {
        auto** slot_loc = static_cast<slot_t<Resource>**>(value);
        *slot_loc = new slot_t<Resource>{(*slot_loc)->resource};
        return 0;
    }

    // Close callback: drop one slot, which drops one shared_ptr reference.
    // Resource destructor runs when the last slot is freed.
    template <class Resource>
    inline herr_t slot_close_cb(const char*, size_t, void* ptr) {
        delete *static_cast<slot_t<Resource>**>(ptr);
        return 0;
    }
}
```

This pattern is the load-bearing primitive for both phases. Phase I instantiates it for
`worker_pool_t`; Phase II instantiates it again for `executor_t`. Different resources, same
mechanism, same lifecycle guarantees.

## 3. Phase I — FAPL Worker Pool

### 3.1. User-Facing API

```cpp
// Default — synchronous, single-threaded, today's behavior
h5::fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC);

// Parallel compression — N workers shared across all datasets in the file
h5::fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC, h5::threads{8});
h5::fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC, h5::threads{});  // hw_concurrency

// Dataset-level opt-in is the existing high_throughput DAPL flag — no count
h5::write(fd, "/feed",  data, h5::chunk{4096} | h5::gzip{6}, h5::high_throughput);
h5::write(fd, "/quiet", data, h5::chunk{4096} | h5::gzip{6});  // synchronous, no pool use
```

Naming convention: `h5::threads{N}` (NOT `h5::filter::threads{N}`). The tag is a top-level h5
construct, parallel in shape to `h5::high_throughput`. Concise, clear, minimal namespace
depth.

### 3.2. Storage Mechanism

```cpp
#define H5CPP_FAPL_WORKER_POOL "h5cpp_fapl_worker_pool"

namespace h5::impl {
    struct worker_pool_t {
        std::vector<std::jthread>            workers;
        bounded::mpmc::queue_t<work_t, 64>   submit_queue;   // any thread → workers
        bounded::mpmc::queue_t<result_t, 64> result_queue;   // workers → consumer
        std::atomic<int>                     in_flight{0};

        explicit worker_pool_t(unsigned n);
        ~worker_pool_t();   // request_stop on all, drain, join

        // Synchronous submit: block until result is ready (Phase I default)
        result_t compress_sync(work_t&&);
        // Async submit: return future (used by Phase II executor)
        std::future<result_t> compress_async(work_t&&);
    };
    using worker_pool_slot_t = slot_t<worker_pool_t>;
}

namespace h5 {
    struct threads { unsigned n; };

    namespace flag {
        using parallel = impl::fapl_call<impl::fapl_args<hid_t, unsigned>,
                                         impl::fapl_threads_set>;
    }
}
```

Property setup is the standard pattern:

```cpp
inline herr_t fapl_threads_set(::hid_t fapl, unsigned n) {
    if (H5Pexist(fapl, H5CPP_FAPL_WORKER_POOL)) return 0;
    auto* slot = new worker_pool_slot_t{
        std::make_shared<worker_pool_t>(n ? n : std::thread::hardware_concurrency())
    };
    return H5Pinsert2(fapl, H5CPP_FAPL_WORKER_POOL, sizeof(worker_pool_slot_t*), &slot,
        nullptr, nullptr, nullptr,
        slot_copy_cb<worker_pool_t>,
        nullptr,
        slot_close_cb<worker_pool_t>);
}
```

### 3.3. Consumer Sites

`pt_t`, `h5::write`, `h5::read` all gain the same lookup helper:

```cpp
namespace h5::impl {
    inline std::shared_ptr<worker_pool_t>
    resolve_worker_pool(::hid_t fapl_id) noexcept {
        if (!H5Pexist(fapl_id, H5CPP_FAPL_WORKER_POOL)) return nullptr;
        worker_pool_slot_t* slot = nullptr;
        H5Pget(fapl_id, H5CPP_FAPL_WORKER_POOL, &slot);
        return slot ? slot->resource : nullptr;
    }
}
```

If the lookup returns a pool, the parallel path is taken (chunks dispatched through the pool).
If null, the synchronous `basic_pipeline_t` runs.

`pt_t::pipeline` no longer holds a `std::variant` of pipeline alternatives. It holds a
reference to the file's pool (or nothing, for synchronous mode):

```cpp
struct pt_t {
    std::shared_ptr<impl::worker_pool_t> pool;   // null = synchronous mode
    /* ... existing scratch buffers etc. ... */
};
```

### 3.4. Migration of PR #243

PR #243 is **closed unmerged**. Its per-`pt_t` `h5::filter::threads{N}` constructor was a local
maximum that doesn't compose with file-scope pool ownership. The threading machinery
(`threaded_pipeline_t`, the bounded queues, the worker shutdown logic) is preserved and
refactored into `worker_pool_t`.

A short note will be added to the close comment pointing users at Phase I as the replacement
API.

### 3.5. Phase I Acceptance Criteria

| Mandatory | |
|---|---|
| `h5::threads{N}` FAPL property; pool is shared across all datasets in the file | |
| Regression test: 50 datasets in one file with `h5::threads{8}` → total worker thread count never exceeds 8 (+ main) | |
| FAPL copy semantics preserve sharing — `H5Fget_access_plist`, `H5Pcopy(fapl)` produce slot copies that share the pool, no fresh pools | |
| File close fully drains, stops, joins all workers — TSAN clean | |
| `h5::high_throughput` DAPL flag drives "use the pool" — absence falls back to synchronous | |
| Bytewise round-trip equivalence: synchronous vs. pool, across gzip / zstd / no-filter | |
| All existing ctest cases pass unchanged | |
| README section + design doc reference | |

| Should-have | |
|---|---|
| Throughput benchmark — multi-chunk write under gzip-6, pool vs. synchronous | |

### 3.6. Phase I Work Breakdown

| # | Sub-scope | Effort |
|---|---|---|
| 1 | `H5Pfapl_threads.hpp` — property name, slot, set/copy/close callbacks; lifecycle tests mirroring #244 scaffolding | 2-3 days |
| 2 | `H5worker_pool.hpp` — extract pool from `threaded_pipeline_t`; pool exposes `compress_sync` and `compress_async`; queues, scratch buffers, RAII shutdown | 3-4 days |
| 3 | Plumbing: `pt_t`, `h5::write`, `h5::read` resolve FAPL → pool → dispatch | 3-4 days |
| 4 | Migration: refactor / remove PR #243's per-`pt_t` pool variant | 1-2 days |
| 5 | TSAN-clean test coverage: pool sharing, FAPL copy regression, multi-fd isolation, fd shutdown | 2-3 days |
| 6 | Docs: README + design note | 1 day |

Total: **12-17 working days, ~2-3 weeks elapsed**. Single PR, 4-6 commits per CONTRIBUTING.

## 4. Phase II — Async Mode (Executor Layer)

### 4.1. What Phase II Adds

A second FAPL slot, parallel to the worker pool, that holds a per-fd executor thread. When
present, all h5cpp public API calls are routed through the executor instead of running on the
caller's thread. The executor is the only thread that calls HDF5; workers (Phase I's pool) are
still consulted for compression. Compile-time blocking of `::hid_t` conversion on
`async_fd_t` / `async_ds_t` / etc. prevents users from accidentally calling HDF5 directly
from threads other than the executor.

### 4.2. User-Facing API

```cpp
// Classic mode (Phase I) — synchronous calls, parallel compression
h5::fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC, h5::threads{8});
H5Gcreate2(fd, "/g", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);   // raw C — works
h5::write(fd, "/x", data, h5::chunk{...} | h5::gzip{6}, h5::high_throughput);

// Async mode (Phase II) — fully thread-safe, parallel compression
h5::async_fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC,
                               h5::async, h5::threads{8});
H5Gcreate2(fd, "/g", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
// ↑ COMPILE ERROR: 'operator ::hid_t()' deleted on async_fd_t

std::thread t1([&]{ h5::write(fd, "/a", data_a); });
std::thread t2([&]{ h5::write(fd, "/b", data_b); });
std::thread t3([&]{ auto r = h5::read<...>(fd, "/a"); });
// All three threads safely call h5cpp from anywhere. The executor serializes
// the underlying HDF5 work. Compression parallelizes across the worker pool.
```

### 4.3. Mechanism

```cpp
#define H5CPP_FAPL_ASYNC_EXECUTOR "h5cpp_fapl_async_executor"

namespace h5::impl {
    struct executor_t {
        std::jthread                                worker;
        bounded::mpsc::queue_t<command_base*, 256>  commands;
        std::shared_ptr<worker_pool_t>              pool;   // for compression

        explicit executor_t(std::shared_ptr<worker_pool_t> p);
        ~executor_t();

        template <class Fn>
        auto submit_and_wait(Fn&& f) -> std::invoke_result_t<Fn>;
    };
    using executor_slot_t = slot_t<executor_t>;
}

namespace h5 {
    struct async_tag_t {};
    inline constexpr async_tag_t async{};
}
```

The async FAPL flag installs the executor and (optionally — see below) also installs the worker
pool if one isn't already there:

```cpp
inline herr_t fapl_async_set(::hid_t fapl, /* no args */) {
    if (H5Pexist(fapl, H5CPP_FAPL_ASYNC_EXECUTOR)) return 0;
    auto pool = resolve_worker_pool(fapl);
    if (!pool) {
        // h5::async without h5::threads{N} — fall back to default-sized pool
        fapl_threads_set(fapl, 0);
        pool = resolve_worker_pool(fapl);
    }
    auto* slot = new executor_slot_t{std::make_shared<executor_t>(pool)};
    return H5Pinsert2(fapl, H5CPP_FAPL_ASYNC_EXECUTOR, sizeof(executor_slot_t*), &slot,
        nullptr, nullptr, nullptr,
        slot_copy_cb<executor_t>,
        nullptr,
        slot_close_cb<executor_t>);
}
```

### 4.4. Compile-Time C API Blocking

Already partially in place — `h5cpp/H5Iall.hpp` defines a `false, false` specialization of
`hid_t` that disables C API conversion. Phase II activates it for async descriptors:

```cpp
namespace h5 {
    using async_fd_t = impl::hid_t<impl::file_t,  H5Fclose, false, false, impl::hdf5::any>;
    using async_ds_t = impl::hid_t<impl::ds_t,    H5Dclose, false, false, impl::hdf5::any>;
    using async_gr_t = impl::hid_t<impl::group_t, H5Gclose, false, false, impl::hdf5::any>;
    using async_at_t = impl::hid_t<impl::attr_t,  H5Aclose, false, false, impl::hdf5::any>;
    /* ... ~10 descriptor types ... */
}
```

The `false, false` specialization needs a small refinement: replace private-inheritance hiding
of `operator ::hid_t()` with `= delete` for cleaner diagnostics
([[h5cpp-async-mode-compile-time-thread-safety-design]] §3.2 covers this).

h5cpp internal code retains access to the raw `::hid_t` via the wrapper's `handle` member, so
the executor can call HDF5 without restriction. The block is purely user-facing.

### 4.5. Operation Dispatch

Public operations gain a single concept-constrained overload that branches on descriptor type:

```cpp
template <class FD, class T>
requires concepts::file_descriptor<FD>
void write(const FD& fd, const std::string& path, const T& data, /* ... args ... */) {
    if constexpr (is_async_v<FD>) {
        auto exec = impl::resolve_executor(fd.handle);
        exec->submit_and_wait([&]{ impl::write_direct(fd, path, data, /*...*/); });
    } else {
        impl::write_direct(fd, path, data, /*...*/);  // classic path, may dispatch to pool
    }
}
```

The compile-time `if constexpr` branch eliminates async overhead for classic users.

### 4.6. Phase II Acceptance Criteria

| Mandatory | |
|---|---|
| `h5::async_fd_t` etc. compile-block raw C API conversion (compile-fail tests) | |
| `h5::open` / `h5::create` with `h5::async` tag returns `async_fd_t`; mode is transitive — async ds/gr/at | |
| Concurrent writes/reads from 8+ user threads to same `async_fd_t` complete correctly under TSAN | |
| Executor + pool lifecycle: file close fully drains, stops, joins both | |
| Exception in HDF5 call propagates back to originating user thread | |
| Bytewise round-trip equivalence: classic vs. async mode produce identical files | |

| Should-have | |
|---|---|
| `H5ES`-style async return path via `h5::write_async` returning `std::future<void>` | |
| Performance benchmark: async-mode throughput vs. HDF5's own `--enable-threadsafe` build on the same workload | |

### 4.7. Phase II Work Breakdown

| # | Sub-scope | Effort |
|---|---|---|
| 1 | Activate `hid_t<..., false, false, ...>` with `= delete`; add `async_*_t` aliases + `h5::async` tag + `is_async_v` trait + concept | 2-3 days |
| 2 | `H5Pfapl_async.hpp` — executor property, slot, callbacks; reuses Phase I pool | 2-3 days |
| 3 | `H5executor.hpp` — executor thread with MPSC command queue, sync facade `submit_and_wait`, exception propagation | 1 week |
| 4 | Concept-constrained overloads of `h5::open`, `h5::create`, `h5::write`, `h5::read`, `h5::append`, `h5::flush` for async descriptors | 2 weeks |
| 5 | Mode-transitive descriptors — async fd produces async ds, async ds produces async at, etc. | 3-4 days |
| 6 | Test suite: compile-fail tests for C API on async descriptors, runtime concurrency tests, TSAN, exception propagation | 1 week |
| 7 | Docs: README, user guide section on async mode | 2-3 days |

Total: **5-6 weeks**. Likely two PRs (descriptors + executor; then operation overloads).

## 5. Cross-Phase Design Decisions

### 5.1. Naming

- **`h5::threads{N}`** — file-scope worker count. Top-level h5 namespace, no `filter::` prefix.
- **`h5::high_throughput`** — existing DAPL flag, retained, means "use the pool for this dataset's I/O".
- **`h5::async`** — Phase II FAPL flag, means "route all calls through an executor + compile-time-block C API".

### 5.2. Pool Ownership Scope

**Per-`h5::fd_t`**, not process-wide. Each file owns its own pool via the FAPL slot. Different
files can have different pool sizes. Multi-file workloads get independent budgets and clean
isolation. Pool destruction is tied to the last live FAPL copy referencing it; in practice this
is the lifetime of the user's `h5::fd_t`.

### 5.3. Default Behavior

If `h5::threads{N}` is **not** specified on the FAPL: no pool created. `h5::high_throughput`
DAPL flag still works but falls back to synchronous `basic_pipeline_t`. Threading is strictly
opt-in at the file level.

If `h5::async` is specified **without** `h5::threads{N}`: Phase II's `fapl_async_set`
auto-installs a default-sized pool (hardware_concurrency) since the executor needs a pool to
do compression work.

### 5.4. Queue Kind

V1 hardcodes MPMC queues throughout the worker pool. Always-safe; modest CAS overhead. SPSC /
SPMC selection is a future optimization once benchmarks identify a workload that needs it.

## 6. Cross-Phase Architecture Diagram

```
┌──────────────────────────────────────────────────────────────────────┐
│  USER THREADS — any number, classic OR async                         │
│  h5::write / h5::read / h5::append on h5::fd_t  or  h5::async_fd_t   │
└────────────────────────┬─────────────────────────────────────────────┘
                         │
            classic ─────┤──── async (Phase II)
                         │
                         │             ┌──── Phase II additive layer
                         │             ▼
┌────────────────────────┴────────────────────────────────────────────┐
│  EXECUTOR THREAD (one per async_fd_t)        FAPL slot: executor_t  │
│  Owns ALL HDF5 C API calls; serialised access through MPSC queue.   │
│  Compile-time block on raw C API via async_fd_t / async_ds_t / etc. │
└────────────────────────┬─────────────────────────────────────────────┘
                         │ dispatches compression sub-work
                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│  WORKER POOL — Phase I                       FAPL slot: worker_pool │
│  N jthreads compress/decompress chunks in parallel.                 │
│  Shared across all datasets in the file. Reused by executor for     │
│  compression work in async mode.                                    │
└─────────────────────────────────────────────────────────────────────┘
```

Both slots live in the same FAPL. Both use the `H5Pinsert2` + `shared_ptr`-in-slot pattern.
Phase II is strictly additive; nothing in Phase I gets rewritten.

## 7. Dependencies and Sequencing

```
PR #244 (DAPL copy-cb + layout guard)    [merging — ✓ Linux/macOS/Windows green]
    │
    ▼
PR #243 (per-pt_t h5::filter::threads)   [CLOSE UNMERGED — superseded by Phase I]
    │
    ▼
Phase I PR — FAPL worker pool             [filed when #244 merges]
    │
    ▼
Phase II PRs — async mode                 [filed when Phase I is stable]
```

Phase I is unblocked by #244 landing. Phase II is unblocked by Phase I shipping and proving
the slot pattern at the FAPL level.

## 8. Open Questions for Steven

1. **Issue title and number for Phase I.** Proposed:
   `feature, FAPL-scoped worker pool with h5::threads{N} for parallel filter compression`.
   I'll file this against `staging` once #244 merges and use the assigned number for the branch.

2. **Tag naming confirmation.** `h5::threads{N}` for FAPL, `h5::async` for Phase II FAPL flag.
   Confirm or override.

3. **Phase II issue scope.** One umbrella issue with sub-PRs, or three independent issues
   (descriptors / executor / operation overloads)? Personal preference: one umbrella issue,
   2-3 PRs feeding it. Matches how #241 was originally scoped.

4. **`h5::write_async` future-returning variant.** Should-have for Phase II or strictly v1.13?
   Easier to leave out of v1 and ship later.

5. **Performance acceptance criteria.** Any specific throughput target relative to
   `basic_pipeline_t` or HDF5's threadsafe build that should gate the merge?

## 9. Decision

**Approved.** Phase I begins as soon as PR #244 merges. PR #243 will be closed with a comment
pointing at the Phase I issue. The architecture is two-layer: worker pool now, executor on top
later, both hidden in FAPL slots using the validated `H5Pinsert2` + `shared_ptr` pattern.

The result, when both phases land:

```cpp
// Single-threaded program with parallel compression
h5::fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC, h5::threads{8});
h5::write(fd, "/x", data, h5::high_throughput);
H5Gcreate2(fd, "/g", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);   // raw C still works

// Fully multithreaded program with compile-enforced safety
h5::async_fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC, h5::async, h5::threads{8});
std::thread t1([&]{ h5::write(fd, "/a", data_a); });
std::thread t2([&]{ h5::write(fd, "/b", data_b); });
std::thread t3([&]{ auto r = h5::read<std::vector<float>>(fd, "/a"); });
H5Gcreate2(fd, "/g", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
// ↑ compile error: ::hid_t conversion is deleted on async_fd_t — by design
```

One library, two modes, same underlying machinery. Classic users see zero change.
Multithreaded users get correctness-by-construction. The whole thing rides on the property-list
hidden-pointer pattern that's now battle-tested in production.

---

## Appendix — Discussion Trail

This workplan consolidates a multi-turn architectural discussion on 2026-05-17 and 2026-05-18
covering:

- The chaos-by-default failure mode of per-DAPL thread counts (50 datasets × 4 workers = 200
  threads).
- The PostgreSQL/ClickHouse/Spark precedent: resource budgets live at the server / process
  scope, not the object scope.
- The discovery that `H5Pinsert2` with `shared_ptr`-in-slot generalizes cleanly from the DAPL
  pipeline pointer (#242) to any heap-resident resource.
- The two-layer realization: worker pool and executor are independent FAPL tenants, not
  competing designs.
- The compile-time C API block (existing `hid_t<..., false, false, ...>` specialization,
  refined with `= delete`) as the safety property that makes async mode unbreakable at the type
  level.

Earlier related design docs:
- [[h5cpp-async-mode-compile-time-thread-safety-design]] — original async mode proposal,
  superseded by this workplan's Phase II
- [[h5cpp-threaded-pipeline-sigma-queue-design]] — the lock-free queue infrastructure that
  underpins the worker pool's internals
- [[h5cpp-product-positioning]] — the broader strategic frame within which this work
  contributes
