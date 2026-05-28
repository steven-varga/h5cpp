@page reports_async_mode_thread_safety h5cpp Async Mode — Compile-Time Thread-Safety via Type-Level Mode Discrimination

**Author:** Winston (System Architect)
**Date:** 2026-05-17
**Scope:** Add a thread-safe operating mode to h5cpp, opt-in at file-open time, enforced at compile
time via the existing `hid_t<..., false, false, ...>` type specialization. Mode is binary: classic
(seamless C API mixing, single-threaded) versus async (executor-routed, compile-time block on C
API). No HDF5 rebuild, no VOL connector, no source-compat impact for classic users.
**Related issues:** h5cpp #239 (v1.12 back-compat — must land first); follows
[[h5cpp-threaded-pipeline-sigma-queue-design]] which addressed compression parallelism.

---

## 1. Problem Statement

h5cpp guarantees seamless mixing of typed RAII descriptors with raw HDF5 C API calls. This is the
load-bearing feature distinguishing h5cpp from h5pp, HighFive, and other C++ HDF5 wrappers:

```cpp
h5::fd_t fd = h5::open("data.h5", H5F_ACC_RDWR);
H5Gcreate2(fd, "/group", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);   // raw C, works
h5::write(fd, "/x", data);                                          // wrapper, works
```

This guarantee is intrinsically **single-threaded**. Default HDF5 builds are not thread-safe; mixing
threads on the same `fd_t` requires either `--enable-threadsafe` (custom HDF5 build, global mutex,
slow) or external `std::mutex` discipline (user's problem).

For workloads that need thread-safe HDF5 access without rebuilding HDF5, h5cpp currently provides
no answer. Prior explorations considered:

- **Wire `threaded_pipeline_t` in (Choice 1 of earlier review):** parallel compression but no
  thread-safe public API.
- **`h5::thread{P, C}` property + `std::variant` of pipeline alternatives (Choice 2):** still
  per-`pt_t`, not per-fd; doesn't solve cross-fd concurrent access.
- **Custom VOL connector:** correct architecture but 3-6 months of dedicated work; HDF5 ≥ 1.12 floor;
  separately-built artifact.
- **Greenfield storage system:** 12-36 months, ecosystem-dead-on-arrival, no funding path.

**This document specifies a fifth approach** that ships in 4-6 weeks, requires no HDF5 rebuild, no
plugin, no new product, and uses h5cpp type-system machinery that already exists.

## 2. Design Goal

Two operating modes, declared at file open, compile-time-enforced thereafter:

| Mode | C API mixing | Multi-threaded h5cpp calls | Trade-off |
|---|---|---|---|
| **Classic** (`h5::fd_t`) | ✓ Seamless | ✗ User's problem | Today's h5cpp, unchanged |
| **Async** (`h5::async_fd_t`) | **✗ Compile-time blocked** | ✓ Safe from any thread | Routes through executor |

The user makes the choice **once**, at `h5::open` / `h5::create`. The return type encodes the
choice. All downstream descriptors (`ds_t`, `gr_t`, `at_t`, etc.) inherit the choice transitively.
The compiler rejects any attempt to mix modes or pass async descriptors to raw HDF5 C functions.

## 3. Mechanism — Type-Level Mode Discrimination

### 3.1 Existing infrastructure

`h5cpp/H5Iall.hpp` defines `hid_t` with `from_capi, to_capi` boolean template parameters:

```cpp
template <class T, capi_close_t capi_close, bool from_capi, bool to_capi, int kind>
struct hid_t;

// Classic specialization — C API conversions enabled
struct hid_t<T, capi_close, true, true, hdf5::any> {
    using hidtype = T;
    H5CPP__EXPLICIT operator ::hid_t() const { return handle; }   // ← the conversion
    ::hid_t handle;
};

// "No C API" specialization — conversion hidden via private inheritance
struct hid_t<T, capi_close, false, false, hdf5::any>
    : private hid_t<T, capi_close, true, true, hdf5::any> { /* ... */ };
```

The `false, false` variant exists but is currently unused. Activating it is the entire mechanism.

### 3.2 Refinement: `= delete` instead of private inheritance

Private inheritance produces "inaccessible base class" diagnostics — technically correct but ugly.
Replace with an explicit `= delete`:

```cpp
struct hid_t<T, capi_close, false, false, hdf5::any>
    : hid_t<T, capi_close, true, true, hdf5::any>
{
    using base = hid_t<T, capi_close, true, true, hdf5::any>;
    using base::base;
    operator ::hid_t() const = delete;
};
```

Compiler diagnostic becomes:
```
error: use of deleted function 'h5::impl::hid_t<...>::operator hid_t() const'
   note: declared here
```

Acceptable. Can be improved further with a `[[deprecated("use h5cpp wrapper operations in async mode")]]`
attribute on the deleted overload to surface a hint.

### 3.3 Parallel descriptor types

Add `async_*` aliases for every public descriptor:

```cpp
namespace h5 {
    // Existing
    using fd_t = impl::pid_t<impl::file_t, H5Fclose>;            // hid_t<..., true,  true,  ...>

    // New — parallel "no C API" descriptors
    using async_fd_t = impl::hid_t<impl::file_t,  H5Fclose, false, false, impl::hdf5::any>;
    using async_ds_t = impl::hid_t<impl::ds_t,    H5Dclose, false, false, impl::hdf5::any>;
    using async_gr_t = impl::hid_t<impl::group_t, H5Gclose, false, false, impl::hdf5::any>;
    using async_at_t = impl::hid_t<impl::attr_t,  H5Aclose, false, false, impl::hdf5::any>;
    using async_sp_t = impl::hid_t<impl::space_t, H5Sclose, false, false, impl::hdf5::any>;
    // ... ~10 descriptor types
}
```

Each async descriptor carries the same `::hid_t handle` as its classic counterpart, plus a
`std::shared_ptr<impl::async::executor_t>` for routing operations through the bus (see §4).

### 3.4 Trait-based mode introspection

```cpp
namespace h5 {
    template <class FD>
    inline constexpr bool is_async_v = /* ... */;   // false for fd_t, true for async_fd_t

    namespace concepts {
        template <class FD>
        concept file_descriptor = /* matches both fd_t and async_fd_t */;

        template <class FD>
        concept async_descriptor = file_descriptor<FD> && is_async_v<FD>;
    }
}
```

Used by overload constraints in §4.2 and by user code that wants to write mode-agnostic helpers.

## 4. Public API Shape

### 4.1 Opening — explicit tag

The opt-in is a tag passed at `h5::open` or `h5::create`:

```cpp
namespace h5 {
    struct async_tag_t {};
    inline constexpr async_tag_t async{};
}

// Classic — unchanged signature, returns h5::fd_t
h5::fd_t fd = h5::open("data.h5", H5F_ACC_RDWR);

// Async — explicit tag, returns h5::async_fd_t (distinct type)
h5::async_fd_t fd = h5::open("data.h5", H5F_ACC_RDWR, h5::async);
h5::async_fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC, h5::async);
```

Return type is determined at the call site by tag presence. No runtime mode flag.

### 4.2 Operations — concept-constrained overloads

Each public operation (`write`, `read`, `append`, `flush`, `close`, plus the operations that return
sub-descriptors like `h5::open(fd, "/path")`) dispatches on descriptor type:

```cpp
namespace h5 {
    // Single signature, branches via if constexpr
    template <class FD, class T>
    requires concepts::file_descriptor<FD>
    void write(const FD& fd, const std::string& path, const T& data) {
        if constexpr (is_async_v<FD>) {
            // Build write command, submit to fd's executor, block on result
            fd.executor().submit_and_wait(
                impl::async::make_write_command(fd.handle, path, data));
        } else {
            impl::write_direct(fd, path, data);   // current implementation, unchanged
        }
    }
}
```

The `if constexpr` branch is compile-time eliminated. Classic-mode users pay no cost. Async-mode
calls route through the executor.

Operations that **return** descriptors propagate the mode:

```cpp
template <class FD>
requires concepts::file_descriptor<FD>
auto open(const FD& fd, const std::string& path) {
    if constexpr (is_async_v<FD>) {
        return fd.executor().submit_and_wait_returning<async_ds_t>(/* ... */);
    } else {
        return ds_t{ H5Dopen(static_cast<::hid_t>(fd), path.c_str(), H5P_DEFAULT) };
    }
}
```

Mode is transitive — an `async_fd_t` produces `async_ds_t`, which produces `async_at_t`, and so on.
No path exists in the type system to escape async mode without explicit conversion (which would
require an explicit reverse-tag, similar to a `reinterpret_cast` — discouraged, possibly disabled).

### 4.3 The packet table

`pt_t` also gets an async variant:

```cpp
namespace h5 {
    template <class DS>
    requires concepts::dataset_descriptor<DS>
    struct pt_t;

    using async_pt_t = pt_t<async_ds_t>;   // alias
}
```

`pt_t::append` checks `is_async_v<DS>` and routes accordingly. Local batch buffering remains the
same — only chunk-sized commits flow through the executor.

## 5. The Executor

### 5.1 Per-file scope

One executor per `async_fd_t`. Stored as `std::shared_ptr<impl::async::executor_t>` in the
descriptor. The shared pointer ensures the executor outlives the last reference, including
descriptors derived from the file (datasets, attributes, etc.) — all carry the same shared pointer.

Multi-file workloads naturally scale: each file has its own executor, no cross-file contention.
Cross-file ordering is the caller's responsibility (same as today).

### 5.2 Command interface

```cpp
namespace h5::impl::async {

struct command_base {
    std::promise<void> done;
    virtual ~command_base() = default;
    virtual void execute() noexcept = 0;
};

template <class Fn>
struct command_t final : command_base {
    Fn work;
    explicit command_t(Fn&& f) : work(std::forward<Fn>(f)) {}
    void execute() noexcept override {
        try { work(); done.set_value(); }
        catch (...) { done.set_exception(std::current_exception()); }
    }
};

struct executor_t {
    bounded::mpsc::queue_t<std::unique_ptr<command_base>, 256> cmds;
    std::jthread worker;

    executor_t() : worker([this](std::stop_token st) { run(st); }) {}

    void submit_and_wait(std::unique_ptr<command_base> cmd) {
        auto fut = cmd->done.get_future();
        while (!cmds.push(std::move(cmd))) std::this_thread::yield();
        fut.get();   // propagates exceptions
    }

    void run(std::stop_token st) {
        std::unique_ptr<command_base> cmd;
        while (cmds.wait_pop(cmd, st)) cmd->execute();
    }
};

} // namespace h5::impl::async
```

Type erasure via virtual dispatch is acceptable here — one virtual call per command at chunk-sized
granularity. Cost is dwarfed by the HDF5 work itself.

### 5.3 Async (non-blocking) facade — phase 2

Sync facade (above) blocks the caller until the executor finishes. Some users want overlap. Add
`_async` variants that return `std::future<T>`:

```cpp
template <class FD, class T>
requires concepts::async_descriptor<FD>
std::future<void> write_async(const FD& fd, const std::string& path, const T& data);
```

Same underlying command machinery; the only difference is the caller doesn't `.wait()` on the
future. This is additive — no breaking change to the sync facade. Ships in phase 2 (see §7).

### 5.4 Compression integration

The existing `threaded_pipeline_t` (see [[h5cpp-threaded-pipeline-sigma-queue-design]]) becomes the
executor's compression backend, not a parallel concept. When the executor processes a chunked write
command, it dispatches the compression to the worker pool, awaits the compressed bytes, then calls
`H5Dwrite_chunk` on the executor thread. Compression parallelism is preserved; the executor thread
only does HDF5 calls.

## 6. Compile-Time Enforcement

### 6.1 What fails to compile in async mode

```cpp
h5::async_fd_t fd = h5::open("data.h5", H5F_ACC_RDWR, h5::async);

// Raw HDF5 C API — blocked
H5Gcreate2(fd, "/g", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
// error: cannot convert 'async_fd_t' to 'hid_t' for argument 1
// note: 'operator hid_t() const' is deleted

H5Iinc_ref(fd);
// error: same

static_cast<::hid_t>(fd);
// error: same

// Mixing modes — blocked
h5::fd_t classic_fd = fd;
// error: no viable conversion from 'async_fd_t' to 'fd_t'

h5::write(classic_fd, "/x", data);   // OK (classic)
h5::write(fd, "/x", data);            // OK (async — different overload)
// But you can't pass an async_fd_t where a classic fd_t is expected.
```

### 6.2 What still compiles

All h5cpp wrapper operations (`h5::open`, `h5::create`, `h5::read`, `h5::write`, `h5::append`,
`h5::flush`, `h5::close`) — they have async overloads.

User-defined helpers that take `h5::fd_t` by reference will not accept `async_fd_t` — the user must
template their helper on the descriptor type or write parallel overloads. This is correct: a helper
that calls raw C API on the fd cannot safely accept an async fd.

### 6.3 Concept-based user helpers

Users who want mode-agnostic helpers use the concept:

```cpp
template <class FD>
requires h5::concepts::file_descriptor<FD>
void my_helper(const FD& fd, const std::string& path) {
    auto data = h5::read<std::vector<float>>(fd, path);   // OK — h5::read is constrained
    // ...
}
```

This compiles for both `fd_t` and `async_fd_t`.

## 7. Implementation Plan

| Phase | Scope | Effort |
|---|---|---|
| **1** | Activate `hid_t<..., false, false, ...>` with `= delete` on the conversion. Add `async_*_t` aliases. Add `async_tag_t`. Add `is_async_v` trait + concepts. | 2-3 days |
| **2** | Implement `impl::async::executor_t` with MPSC queue, jthread worker, sync facade. Per-file ownership via `std::shared_ptr`. | 1 week |
| **3** | Async overloads for `h5::open`, `h5::create` (file-level). Verify mode propagates correctly. | 3-4 days |
| **4** | Async overloads for `h5::write`, `h5::read`, `h5::append`, `h5::flush`. Wire through `pt_t<async_ds_t>`. | 2 weeks |
| **5** | Compression pool integration — `threaded_pipeline_t` becomes the executor's compression backend. | 1 week |
| **6** | Test suite: multi-thread concurrent writes/reads, compile-fail tests for C API mixing (`-Werror` + expected fail), executor lifecycle, exception propagation. | 1 week |
| **7** | Documentation: README section, user guide page, conference-talk abstract draft. | 2-3 days |

Total: **5-6 weeks** of focused work. Each phase is independently mergeable.

Optional phase 8: async (non-blocking) facade returning `std::future<T>`. Ships separately once core
sync facade is stable.

## 8. Testing

### 8.1 Compile-fail tests

A new harness under `test/compile-fail/` exercises the type-system gates:

```cpp
// test/compile-fail/async_capi_blocked.cpp
#include <h5cpp/all>
int main() {
    h5::async_fd_t fd = h5::open("x.h5", H5F_ACC_TRUNC, h5::async);
    H5Gcreate2(fd, "/g", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);  // must fail to compile
}
```

CMake target asserts these fail with the expected diagnostic via
`try_compile(EXPECT_FAIL)` or a small driver script.

### 8.2 Runtime tests

- Concurrent writes to the same async_fd_t from 8 threads → readback verifies all data present
- Concurrent reads while writes are in flight (with explicit `flush` synchronization points)
- Executor shutdown ordering: drain on destruction, no leaked work
- Exception propagation: HDF5 error in worker → exception surfaces at caller
- TSAN clean across all multi-thread scenarios
- ASAN clean (executor allocations, command lifetimes)

### 8.3 Performance tests

- Single-thread baseline: async mode vs classic mode overhead per call (expected: ~5-50µs for small
  ops, <1% for chunk-sized ops)
- Multi-thread scaling: throughput vs thread count, identify executor saturation point
- Comparison vs `--enable-threadsafe` HDF5 build on the same workload

## 9. Trade-offs

| | |
|---|---|
| ✔ | Compile-time enforcement: mixing is structurally impossible. |
| ✔ | No HDF5 rebuild required. |
| ✔ | No VOL connector — saves 3-6 months. |
| ✔ | Reuses existing h5cpp type machinery. |
| ✔ | Classic mode entirely unchanged. Zero regression risk for existing users. |
| ✔ | Ships in weeks, not months. |
| ✔ | Self-documenting: `async_fd_t` in a signature is immediately clear. |
| ◇ | Doubles the constrained public API surface (one if-constexpr branch per operation). |
| ◇ | Raw C API blocked in async mode — the user signed up for this trade at open time. |
| ◇ | Users who need *both* thread-safety and raw C mixing simultaneously have no answer here. (Neither does HDF5.) |
| ◇ | Executor lifetime requires care — `std::shared_ptr` ownership through all derived descriptors. |

## 10. Positioning

This feature is **conference-talk material** the day it ships:

- "Compile-time mode separation for HDF5 thread safety" — CppCon, Meeting C++, ACCU
- Concrete demo: same code, two modes, compiler enforces the contract
- Comparison to HDF5's `--enable-threadsafe` build (global mutex, custom build) and to alternatives
  (kdb+ pricing, ArcticDB language constraints)
- Pitch: "h5cpp is the only C++ HDF5 library that makes thread safety a first-class, compile-time
  property"

Aligns with the broader Vargalabs positioning discussed in
[[h5cpp-product-positioning]] — H5CPP as the credibility-building loss leader, with each major
feature drop generating a fresh talk-and-blog cycle.

## 11. Open Questions for Steven

1. **Default tag name.** `h5::async` is concise but overloaded with other meanings (coroutines,
   futures). Alternatives: `h5::concurrent`, `h5::threaded`, `h5::mt`, `h5::thread_safe`. Each has
   trade-offs. Prefer `h5::async` for brevity; flag if there's a clash with an existing symbol.

2. **Mode escape hatch.** Should there be *any* way to obtain a raw `::hid_t` from an `async_fd_t`
   for special cases? An explicit `fd.unsafe_handle()` that returns `::hid_t` and documents the
   thread-safety contract is loud enough to be safe. Default position: provide it, name it clearly,
   document it as "you are now responsible for executor coordination."

3. **`H5ES` integration.** HDF5 ≥ 1.13 has native event-set async (`H5Dread_async`, etc.). Should
   async mode optionally route through these instead of our executor? Pro: less code we write. Con:
   HDF5 floor moves to 1.13, more complexity, dual code paths. Default position: skip for now,
   revisit if user demand surfaces.

4. **`pt_t<async_ds_t>` API symmetry.** Should `h5::append(async_pt, item)` block per-item, or
   batch locally and only block on chunk commit (like classic `pt_t`)? Strongly prefer the latter
   for performance. Confirm.

5. **Phase 1 PR scoping.** Pieces 1-3 (type plumbing + `open`/`create` only) ship as one PR for
   review of the core design, before investing in operation overloads. Confirms the type-system
   approach works in the real codebase before committing to the full surface.

6. **Executor reuse across files.** Multiple `async_fd_t` instances opened from the same path
   today get separate executors. Should they share? Probably not — file isolation is cleaner — but
   document the behavior.

---

## Decision

Recommended: **proceed with this approach.** It is the smallest, fastest, lowest-risk path to
shippable thread-safe h5cpp. It defers (without precluding) the VOL connector and the greenfield
storage-system options. It preserves classic h5cpp identity. It ships in 5-6 weeks.

Next step on Steven's approval: file the umbrella issue against `staging` and start phase 1 in a
fresh worktree once #239 lands.
