@page reports_threaded_pipeline_sigma_queue h5cpp Multithreaded Filter Pipeline — sigma Queue Design Report

**Author:** Winston (System Architect)
**Scope:** Replace `impl::threaded_pipeline_t` stub with a real lock-free multithreaded filter
pipeline using `sigma/queue.hpp` and `sigma/ring.hpp` as the concurrency substrate.
**Related issues:** h5cpp #89 (type engine), h5cpp #160 (pipeline rework)

---

## 1. Current State

`H5Zpipeline.hpp` contains three pipeline variants as CRTP descendants of `pipeline_t<Derived>`:

| Type | Status | Description |
|---|---|---|
| `basic_pipeline_t` | Active | Serial, synchronous chunk decomposition |
| `threaded_pipeline_t` | **Empty stub** | `write_chunk_impl` / `read_chunk_impl` are no-ops |
| `romio_pipeline_t` | Empty stub | MPI-IO path, deferred |
| `hadoop_pipeline_t` | Empty stub | HDFS path, deferred |

`basic_pipeline_t` operates entirely on the calling thread:
1. `split_to_chunk_write` decomposes the user buffer into chunk-sized slabs via nested `memcpy` loops.
2. Each slab lands in `chunk0` (aligned buffer).
3. `write_chunk_impl` applies filters in-place (ping-pong between `chunk0`/`chunk1`), then calls
   `H5Dwrite_chunk` directly.
4. Read is the symmetric reverse.

Everything is serial and on the calling thread. There is no parallelism between filter stages or
between I/O and compute.

---

## 2. sigma Queue Infrastructure

`sigma/queue.hpp` provides four bounded, lock-free Vyukov-style queues:

| Namespace | Topology | Push contention | Pop contention | Doorbell |
|---|---|---|---|---|
| `bounded::spsc` | 1P–1C | None | None | `seq` atomic wait |
| `bounded::mpsc` | NP–1C | CAS on head | None | `doorbell` atomic wait |
| `bounded::spmc` | 1P–NC | None | CAS on tail | `doorbell` atomic wait |
| `bounded::mpmc` | NP–NC | CAS on head | CAS on tail | `doorbell` atomic wait |

All queues use C++20 `std::atomic::wait` / `notify_one` (Linux: futex, Windows: WaitOnAddress) as
their blocking primitive — **zero cost when not contended**, no condition variables, no mutexes.

`sigma::doorbell_t` is a standalone `atomic<uint32_t>` wrapper exposing `ring()` / `ring_all()`.

`sigma/ring.hpp` provides `bounded::ring::adaptor_t<queue_t, N_bytes, align>` — a **byte arena ring
buffer** on top of any of the above queues. The token type is `ctrl_t{type, length, position}`:
- `type` — user-defined tag (can encode filter stage, chunk index, padding)
- `length` — payload byte count (max 65535 per token; chunked datasets fit easily)
- `position` — byte offset into the ring arena
- PAD tokens (type==0) handle arena wrap-around transparently.

Convenience aliases: `bounded::ring::spsc_t<N_ctrl, N_bytes>` and `bounded::ring::mpsc_t<N_ctrl,
N_bytes>`.

### Key Properties
- Header-only, zero dependencies beyond `<atomic>` / `<array>` / `<stop_token>`.
- Cache-line padded cells (`cell_t`) prevent false sharing between producers and consumers.
- `wait_pop` accepts `std::stop_token` — cooperative cancellation is built-in.
- The ring buffer is a power-of-two static allocation; no heap involved in steady state.

---

## 3. Threading Model for the Pipeline

### 3.1 Write Path — Stage Topology

```
Calling thread                   Filter worker pool         I/O thread
─────────────────────────────    ────────────────────────   ─────────────────
split_to_chunk_write()           N worker threads           1 dedicated thread
  │                                │                          │
  ├─ memcpy chunk → input_ring ──→ pop ctrl_t                │
  │   (spsc: 1 decomposer)         apply filter chain         │
  │                                write result → output_ring ──→ pop ctrl_t
  │                                                            H5Dwrite_chunk()
  │                                                            release(ctrl)
  └─ wait for completion token ←──────────────────────────── push completion
```

### 3.2 Read Path — Stage Topology

```
Calling thread                   Filter worker pool         I/O thread
─────────────────────────────    ────────────────────────   ─────────────────
split_to_chunk_read()            N worker threads           1 dedicated thread
  │                                │                          │
  │   ┌── request ring ──────────→ H5Dread_chunk()           │
  │   │                            write raw → input_ring     │
  │   │                          pop ctrl_t                   │
  │   │                          apply inverse filter chain   │
  │   │                          write result → output_ring   │
  │   └── pop output ctrl_t                                   │
  │        memcpy chunk → user buffer                         │
  └─ done when all chunk tokens consumed
```

### 3.3 Queue Topology Selection

| Channel | Topology | Rationale |
|---|---|---|
| `input_ring` (decomposer → filter workers) | `bounded::ring::spsc_t` | One decomposer thread, multiple workers steal via separate `spmc` dispatch queue |
| `dispatch` (work tickets to filter workers) | `bounded::spmc` | 1 producer (decomposer), N consumers (workers) |
| `output_ring` (filter workers → reassembly) | `bounded::ring::mpsc_t` | N producers (workers), 1 consumer (I/O or reassembly thread) |
| `completion` (I/O → caller) | `bounded::spsc` | 1 I/O thread → 1 caller |

The ring buffers carry the actual chunk bytes; the dispatch / completion queues carry `ctrl_t`
tickets that reference positions in the ring buffers. This separates **scheduling** (cheap,
cache-hot) from **data movement** (bulk, arena-resident).

---

## 4. Token Design

`ctrl_t` has 16-bit `type`. Suggested encoding for h5cpp pipeline tokens:

```
type bits [15:8]  — stage number (0 = raw/unfiltered, 1..N = filter index, 0xFF = completion)
type bits [ 7:0]  — flags (0x01 = final chunk of dataset, 0x02 = edge chunk, 0x04 = PAD)
```

This lets a filter worker identify which filter to apply without a separate queue per stage,
and lets the reassembly consumer detect the final token without a separate out-of-band signal.

---

## 5. Chunk Buffer Sizing

The ring arenas must be sized at compile time. Recommended defaults:

```cpp
// Per pipeline instance — stored in threaded_pipeline_t
static constexpr size_t filter_ctrl_depth = 64;   // outstanding chunk tickets
static constexpr size_t ring_bytes        = 1 << 23; // 8 MB per ring arena (2 rings = 16 MB)
using input_ring_t  = bounded::ring::spsc_t<filter_ctrl_depth, ring_bytes>;
using output_ring_t = bounded::ring::mpsc_t<filter_ctrl_depth, ring_bytes>;
```

`ring_bytes` must be a power of two and ≥ `2 × max_chunk_compressed_size`. For typical HDF5
datasets (1 MB chunks, gzip level 6 ≈ 60% compression), 8 MB provides 8× headroom.
Both arenas together consume 16 MB on the stack (or the DAPL heap if dynamically allocated).

These should be tunable via `H5CPP_PIPELINE_RING_BYTES` and `H5CPP_PIPELINE_CTRL_DEPTH`
CMake defines, following the pattern of `H5CPP_MEM_ALIGNMENT` and `H5CPP_MAX_FILTER`.

---

## 6. C++ Standard Constraint

**This is the critical blocker.**

| Feature | Required by | Standard |
|---|---|---|
| `std::stop_token` / `std::jthread` | `wait_pop` cancellation | **C++20** |
| `std::atomic::wait` / `notify_one` | Blocking queue primitives | **C++20** |
| `std::bit_ceil` | `ceil_pow2` in queue.hpp | **C++20** |

h5cpp currently targets **C++17** (issue #89 explicitly preserves this). The sigma queues
are **C++20-only**.

### Options

| Option | Cost | Risk |
|---|---|---|
| A. Bump h5cpp to C++20 for the threaded pipeline only, guarded by `#ifdef H5CPP_USE_THREADING` | Requires issue + Steven approval; changes `CMakeLists.txt` minimum standard | Low — most modern compilers support C++20 |
| B. Backport the queues to C++17 (replace `std::atomic::wait` with `futex`/`WaitOnAddress` directly, `bit_ceil` with `__builtin_clz`) | ~1 day of work, no sigma dependency | Medium — platform ifdefs needed |
| C. Vendor a C++17-compatible SPSC/MPSC from a third party (e.g., `rigtorp/SPSCQueue`) | Smallest change, existing precedent (h5cpp already vendors xtensor, zstd) | Low |
| D. Accept C++20 as a build-time feature flag | Cleanest long-term; threaded_pipeline_t becomes a C++20 extension point | Low |

**Recommendation: Option D** — gate `threaded_pipeline_t` on `H5CPP_CXX_STANDARD >= 20`. The
type engine (#89) and warning cleanup (#174) work fine in C++17. The multithreaded pipeline is
explicitly a performance extension that users opt into.

---

## 7. IP / Include Strategy

`sigma/queue.hpp` and `sigma/ring.hpp` carry a Varga Labs proprietary copyright notice. They
cannot be vendored into the open-source h5cpp repository as-is.

### Options

| Option | Notes |
|---|---|
| A. Relicense sigma headers under Apache-2.0 / MIT | Cleanest for open source; requires Steven decision |
| B. Treat sigma as an optional found-package; h5cpp uses `find_package(sigma)` and feature-gates the threaded pipeline | No relicense needed; users who have sigma get threads |
| C. Reimplement the queue logic independently in h5cpp (the Vyukov algorithm is public domain) | No IP dependency; ~200 LOC |
| D. Vendor `rigtorp/SPSCQueue` (MIT, C++11-compatible SPSC) | Minimal; only SPSC, not MPSC/SPMC |

**Recommendation: Option C** — implement a minimal `h5::impl::spsc_queue_t` and
`h5::impl::mpsc_queue_t` directly in `H5Zpipeline.hpp` or a new `H5Zqueue.hpp`, using the Vyukov
algorithm. The sigma headers serve as the reference implementation and specification. ~200 lines,
no IP concerns, no external dependency.

---

## 8. Scatter/Gather Integration

This was the original motivating question. The answer after analyzing the ring buffer design:

**Yes, but only for contiguous-mapped scatter/gather, not for vlen strings.**

The ring arena (`adaptor_t`) stores flat byte sequences. A `vector<string>` write requires:
1. Gather: build `const char*[]` pointer array on the heap.
2. The pointer array itself is not the data — HDF5 dereferences each `char*` to variable-length
   storage; the ring buffer has no role here.

However, for `vector<T>` where T is a non-contiguous but regular structure (e.g., `list<float>`),
the gather step (copy elements into a contiguous staging buffer) **does** produce a flat byte
sequence that can be placed into the ring arena as a regular chunk. The filter workers then
operate on that contiguous image exactly as they would for a `vector<float>`.

**Split responsibilities:**
- `access_traits_t::pointers` / `access_traits_t::iterators` — scatter/gather is the **type engine's** job, performed before the chunk is handed to the ring buffer.
- The ring buffer and filter workers see only flat `void*` bytes — they are type-blind.
- vlen strings (`vector<string>`) bypass the ring entirely and use `H5Dread`/`H5Dwrite` with
  `H5T_VARIABLE` — the HDF5 library handles the allocation; no filter pipeline applies.

---

## 9. Work Breakdown

| Task | Scope | Blocking |
|---|---|---|
| T1 | Decide on C++ standard strategy (Option D) | Steven approval |
| T2 | Decide on queue IP strategy (Option C recommended) | Steven decision |
| T3 | Implement `H5Zqueue.hpp` — SPSC + MPSC, Vyukov, C++20 atomic::wait | T1, T2 |
| T4 | Implement `H5Zring.hpp` — byte arena adaptor (mirrors sigma ring) | T3 |
| T5 | Implement `threaded_pipeline_t::write_chunk_impl` using ring + worker pool | T4 |
| T6 | Implement `threaded_pipeline_t::read_chunk_impl` | T4 |
| T7 | Thread pool lifecycle management in `set_cache()` / destructor | T5, T6 |
| T8 | CMake: `H5CPP_PIPELINE_RING_BYTES`, `H5CPP_PIPELINE_CTRL_DEPTH`, `H5CPP_USE_THREADING` | T3 |
| T9 | Tests: `test/H5Zpipeline_threaded.cpp` round-trip with gzip | T5, T6 |
| T10 | Wire scatter/gather staging into `access_traits_t::iterators` path | #89 type engine |

T1 and T2 are architectural decisions that gate everything else. T10 is independent — it can
proceed in parallel with T3–T9 on the #89 branch.

---

## 10. Open Questions for Steven

1. **C++ standard:** Approve Option D (C++20 feature flag for threading)? Or should the queue
   implementation be backported to C++17?
2. **sigma IP:** Can `queue.hpp` / `ring.hpp` be relicensed for h5cpp, or should a clean-room
   reimplementation be done?
3. **Worker count:** Fixed compile-time pool size or runtime `std::thread::hardware_concurrency()`?
4. **Backpressure:** When the ring is full (slow I/O), should `write` block or throw? Current
   `basic_pipeline_t` never blocks. Blocking is safer; throwing breaks the API contract.
5. **Scope for #89:** Should the threaded pipeline be a separate issue (e.g., #178+), or is it
   in scope for #89? Given #89 is already the type engine refactor, a separate issue is cleaner.
