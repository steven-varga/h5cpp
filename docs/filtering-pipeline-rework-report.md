# H5CPP Filtering Pipeline Rework Report

## Scope

This report captures the current state of the H5CPP filtering pipeline and proposes a C++17-compatible path toward a SIMD- and multithreaded chunk filtering engine. The target platforms are Linux, macOS, and Windows. The design should keep the current C++17 surface stable while leaving room for later C++23 cleanup and eventual C++26 reflection integration.

## Current Architecture

H5CPP already has the right architectural interception point for high-throughput filtering:

1. Users express storage layout and filters through the existing property-list syntax, for example `h5::chunk{1024} | h5::shuffle | h5::gzip{6}`.
2. Dataset creation property lists keep standard HDF5-compatible filter metadata.
3. `h5::high_throughput` is intended to install a custom dataset access property-list entry.
4. `h5::write` and `h5::read` detect that DAPL entry and route transfers through `pipeline_t`.
5. `pipeline_t` tiles user memory into chunk-sized blocks and calls `H5Dwrite_chunk` / `H5Dread_chunk`.

That model is sound: keep HDF5 compatibility at the file boundary, but own the CPU-side transform chain inside H5CPP.

## Current Correctness Gaps

The current implementation is an experimental skeleton rather than a production replacement for HDF5's filter chain.

| Area | Current State | Required State |
|---|---|---|
| Activation | `h5::high_throughput` does not install its DAPL property on the current HDF5 1.10.9 baseline | Reliable cross-version activation or explicit unsupported-version diagnostics |
| Deflate/gzip | Compress-only callback | Encode and decode with correct zlib buffer sizing |
| Shuffle | Placeholder copy | Element-size-aware shuffle and unshuffle |
| Fletcher32 | Placeholder copy | Checksum generation and verification |
| N-bit / scale-offset | Placeholder copy | Correct implementation or explicit unsupported-filter error |
| Multi-filter read | Throws for more than one filter | Reverse-order decode through the complete filter plan |
| Buffer sizing | Uses chunk-sized scratch buffers | Encoded buffers must allow compression expansion |
| Filter mask | Partial handling | Preserve HDF5 chunk filter-mask semantics |
| Threading | `threaded_pipeline_t` is a placeholder | Worker-local state and bounded chunk scheduling (delivered in #250 as FAPL-scoped `pool_pipeline_t`) |
| Portability | Linux path is the only recently verified path | Linux, macOS, and Windows allocation/build behavior |

Focused baseline probes confirmed two important failures:

1. `h5::high_throughput` does not currently activate the DAPL property with HDF5 1.10.9.
2. The gzip callback can encode data, but it cannot decode using the reverse filter path.

## iex2h5 Vendored H5CPP Review

The vendored copy at `/home/steven/projects/iex2h5/thirdparty/h5cpp/v1.10.8/include/h5cpp/` was reviewed from the filtering pipeline angle. Its filter callbacks and chunk read/write logic are effectively the same experimental pipeline: gzip remains encode-only, shuffle/Fletcher32/N-bit/scale-offset remain placeholder copies, multi-filter read still throws, and `h5::high_throughput` remains gated to older HDF5 versions.

The useful difference is in scratch-buffer ownership: the vendored copy introduced a named aligned-allocation helper instead of open-coding `aligned_alloc` at the call site. That idea is worth keeping, but the vendored implementation is Linux-oriented and does not round allocation sizes or handle Windows. The #160 branch borrows the helper pattern and hardens it for C++17 portability:

- central `make_aligned(alignment, size)` helper,
- size rounded up to the requested alignment,
- `posix_memalign` on Linux/macOS,
- `_aligned_malloc` / `_aligned_free` on Windows,
- RAII deleter matched to the allocation API.

## Proposed Architecture

Replace the thin filter function-pointer model with a first-class filter plan:

```text
DCPL metadata
  -> filter_plan
  -> chunk_iterator
  -> execution_policy
  -> chunk_io_backend
```

Responsibilities:

| Layer | Responsibility |
|---|---|
| `filter_plan` | Decode DCPL filters into validated encode/decode operations |
| `chunk_iterator` | Walk chunk coordinates, handle edges, and map memory to chunks |
| `execution_policy` | Select serial, threaded, and later SIMD-aware execution |
| `chunk_io_backend` | Use HDF5 chunk APIs now; keep room for native storage later |

Each filter operation should carry:

- HDF5 filter id
- flags and client data values
- element size where required
- `max_encoded_size(input_size)`
- `encode(src, dst)`
- `decode(src, dst)`
- clear error behavior for unsupported filters

This avoids guessing filter direction from a single callback and gives the scheduler enough information to size scratch buffers safely.

## Implementation Phases

### Phase 1: Correct serial engine

Deliver a boring, testable serial implementation before adding SIMD or threads.

Initial #160 progress:

- Deflate/gzip callbacks now encode and decode zlib-compatible streams.
- libdeflate is used when configured/present; zlib remains the fallback because HDF5 already requires it.
- CMake exposes `H5CPP_USE_LIBDEFLATE`, `H5CPP_REQUIRE_LIBDEFLATE`, and `H5CPP_INSTALL_LIBDEFLATE`.
- The test thirdparty layer has a libdeflate interface target for doctest coverage.
- Pipeline scratch allocation now reserves deflate-bound capacity rather than raw chunk bytes.

Required work:

1. Restore reliable `h5::high_throughput` activation or emit a clear unsupported-version error.
2. Implement deflate/gzip encode and decode using `compressBound`, `compress2`, and inflate/uncompress. **Status: initial callback-level implementation complete, with libdeflate fast path and zlib fallback.**
3. Implement byte shuffle and unshuffle using the dataset element size.
4. Implement Fletcher32 generation and verification.
5. Make N-bit and scale-offset explicit: either implement them correctly or reject them loudly.
6. Implement reverse-order read for multi-filter chains.
7. Allocate scratch buffers by maximum encoded size, not raw chunk size.
8. Preserve and test filter-mask behavior.

Acceptance tests should prove:

- H5CPP pipeline write can be read through normal HDF5/H5CPP read.
- Normal HDF5 filtered write can be read through the H5CPP pipeline.
- `shuffle | gzip` round-trips.
- `shuffle | gzip | fletcher32` round-trips and detects corruption where feasible.
- Unsupported filters fail with useful diagnostics instead of silently copying.

### Phase 2: Threaded chunk scheduler

Once serial correctness is proven, add a bounded chunk scheduler:

```text
source memory
  -> chunk gather/tile
  -> prefilters
  -> compressor
  -> checksum
  -> H5Dwrite_chunk
```

Read path:

```text
H5Dread_chunk
  -> checksum verify
  -> decompressor
  -> reverse prefilters
  -> scatter to user memory
```

Design rules:

- No shared mutable scratch buffers across workers.
- Each worker owns aligned scratch memory.
- HDF5 calls should be isolated behind the backend so thread-safety policy is explicit.
- The scheduler should be bounded to prevent memory blowups on large datasets.
- Serial and threaded paths must share the same `filter_plan`.

### Phase 3: SIMD and advanced codecs

After correctness and scheduler boundaries are stable, add optimized filters:

- SIMD shuffle/unshuffle
- bitshuffle
- delta and XOR filters for numeric/time-series data
- zstd and lz4 backends
- blosc-style composed filter bundles
- optional adaptive codec selection by sampled chunk entropy

These should remain optional and feature-detected. The baseline must stay C++17 and portable.

## Cross-Platform Considerations

The pipeline should hide platform allocation differences behind one internal aligned-buffer helper:

- Linux/macOS: use a safe aligned allocation path that rounds size as required.
- Windows: use `_aligned_malloc` / `_aligned_free`.
- Never require raw chunk bytes to already be a multiple of alignment.

Threading should initially use C++17 standard library primitives. Avoid platform-specific thread pools until benchmarks prove the need.

## Recommendation

Start with correctness, not SIMD. The highest-value first milestone is a serial `filter_plan` that can round-trip standard HDF5 filters and reject unsupported filters explicitly. Once that foundation is correct, SIMD and multithreading become execution-policy improvements rather than a risky rewrite.

The strategic direction is to make H5CPP's filtering chain a modern CPU execution engine while preserving HDF5-compatible metadata and file interoperability.

## Status — Phase I (#250, FAPL worker pool)

Phase I of the threading workplan is delivered on PR #251.  The design and trade-offs are summarised in `tasks/h5cpp-fapl-multithreading-workplan.md`; the user-visible surface is one line in the file's FAPL:

```cpp
h5::fd_t fd = h5::create(
    "data.h5", H5F_ACC_TRUNC,
    h5::default_fcpl,
    h5::threads{N} | h5::backpressure{M});       // M default = 8 × N
```

When `h5::threads{N}` is installed, the FAPL allocates a `worker_pool_t` and parks a `shared_ptr<>` to it inside an `H5Pinsert2` slot.  Every dataset created/opened on that file inherits the pool via `H5Fget_access_plist`.  When a dataset's DAPL has `h5::high_throughput`, `h5::write` and `h5::read` construct a local `pool_pipeline_t` that submits per-chunk compression closures to the pool and drains in submission order; `H5Dwrite_chunk` still runs on the calling thread.  `pt_t` resolves the same pool in `init()` and uses `pool_pipeline_t` as a variant alternative.

Back-pressure is bounded by `h5::backpressure{M}`: the producer blocks on the front future once the in-flight deque hits `M`.  Default is 8 × worker count.

The legacy per-pt_t `h5::filter::threads{N}` constructor from #241 is removed in this cycle (see [Phase 1.4 commit message]).  Two parallel threading paths in the pipeline invite contention bugs and confuse the surface; the FAPL pool fully subsumes it.

Phase II (compile-time C-API blocking on `h5::async::fd_t`, full async mode) is tracked separately.

## Status — Phase II PR-A (#252, async descriptors + executor scaffold)

Phase II is delivered in two PRs.  PR-A (this work) lands the descriptor types, FAPL executor property, and executor thread; PR-B will wire the concept-constrained `h5::write` / `h5::read` / `h5::create(fd, "ds", …)` overloads that branch on `is_async_v<FD>`.

### User-visible surface (PR-A)

```cpp
// Namespace shape: nested h5::async::*, parallel to the classic h5::*.
namespace h5::async {
    using fd_t   = impl::async_hid_t<impl::fd_t,  H5Fclose>;
    using ds_t   = impl::async_did_t<impl::ds_t,  H5Dclose>;
    using gr_t   = impl::async_aid_t<impl::gr_t,  H5Gclose>;
    using at_t   = impl::async_aid_t<impl::at_t,  H5Aclose>;

    fd_t create(const std::string& path, unsigned flags,
                const h5::fcpl_t& fcpl = h5::default_fcpl,
                const h5::fapl_t& fapl = h5::default_fapl);
    fd_t open  (const std::string& path, unsigned flags,
                const h5::fapl_t& fapl = h5::default_fapl);
}

// Mode is declared exactly once — at h5::async::create / open.
// Every downstream operation deduces async-ness from the FD type via
// TAD (Phase II PR-B).
```

`h5::async::fd_t` has `operator ::hid_t() = delete`, so a stray `H5Gcreate2(async_fd, …)` is a clean compile error ("use of deleted function") rather than a silent thread-safety hazard.

### Mechanism (PR-A)

- `h5cpp/H5executor.hpp` — single worker thread per async fd, `std::packaged_task` in a `shared_ptr` wrapped in `std::function<void()>`, `submit_and_wait<Fn>(Fn&&)` blocking the caller via the future.  Exceptions propagate back through `future::get`; same-thread re-entry runs the callable inline.
- `h5cpp/H5Pfapl_async.hpp` — FAPL executor slot using the same `H5Pinsert2` + shared-ptr pattern as Phase I.  Kept as a defensive utility; the primary code path does not depend on it.
- `h5cpp/H5async.hpp` — the two factories.  Each constructs an `executor_t`, then calls `H5Fcreate` / `H5Fopen`, then returns `h5::async::fd_t{raw_hid, exec}`.

The executor lives **directly on the wrapper** (a `std::shared_ptr<executor_t> exec` field on the `false,false` `hid_t` specialization), not retrieved from the file's FAPL via `H5Fget_access_plist`.  HDF5 1.10.9 reconstructs the retrieved FAPL from standard properties only — `H5Pinsert2`-installed properties do not survive the round-trip.  (This also affects Phase I's pool resolution path; tracked as a follow-up.)

### Out of scope for PR-A (lands in PR-B)

- Concept-constrained overloads of `h5::write`, `h5::read`, `h5::append`, `h5::flush`, `h5::create(fd, "ds", …)`.
- Mode-transitive factory pattern (async fd → async ds → async at).
- `h5::pt_t` as a class template `template <class DS = h5::ds_t>` deduced via CTAD.
- Performance benchmarks vs. HDF5 `--enable-threadsafe`.
