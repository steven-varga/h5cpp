# #287 multithreaded pipeline profile — the global-mutex MT model

Profiles the multithreaded write pipeline (`pipeline.cpp`) against the established
single-threaded baselines, comparing the shipping **classic** build with the
**MT** build that enables concurrent-writer safety.

- **classic** = no `-DH5CPP_MULTITHREAD` (the shipping single-threaded path).
- **MT** = `-DH5CPP_MULTITHREAD`: a single **process-global recursive HDF5 lock**
  (HDF5's own `--enable-threadsafe` design — a *lock*, not a thread). Every HDF5
  C-API call (incl. handle refcount + property-list construction) runs under the
  one lock; in classic builds it compiles out to a no-op.

> Note: an earlier per-file `io_collector_t` **thread** model (and the
> `h5::async::` API that drove it) has been **retired and deleted**. This report
> describes the current global-mutex model only. The `single`/`multi`/`async`
> case labels below are benchmark cases in `pipeline.cpp`, not the old async API.

## Setup
- HDF5 1.12.3, gcc-14, `-O2` (RelWithDebInfo), 16 hw threads
- 64 MiB logical input, 512 KiB chunks, output on `/dev/shm`
- gzip level 6 unless a regime says otherwise; worker pool does compression only
- Throughput is MB/s, median of 3 runs

Porting note: `single`/`multi`/`async` pass **`h5::high_throughput`** — the
per-dataset DAPL opt-in that engages `pool_pipeline_t`. Without it the write
silently falls back to stock single-threaded HDF5 filters (the `HDF5 H5Dwrite +
gzip` row, ~56 MB/s).

## Data flow (the global-mutex model)

Legend: `⟦…⟧` = an HDF5 C-API call. Under MT, the process-global recursive lock
admits **one thread at a time** into HDF5 (uncontended for a single producer —
the lock is taken once per top-level op, with thread-local recursion depth).
`[w]` = a worker-pool thread (compression only — never touches HDF5, never takes
the lock). Arrows are data movement; `║` marks a thread boundary.

### Baselines — single thread, no pool

```
raw          caller ──split──► chunk ──────────────────────► ⟦H5Dwrite_chunk⟧ ──► file
             (no filter; chunk written straight from the source buffer)

direct-gzip  caller ──split──► chunk ──libdeflate gzip──────► ⟦H5Dwrite_chunk⟧ ──► file
             (compress AND write on the caller; tightest single-thread gzip)

hdf5-gzip    caller ──────────────────► ⟦H5Dwrite⟧ ─► [HDF5 deflate filter] ──► file
             (HDF5 runs the gzip filter itself, on the caller; no h5cpp pool)
```

### single / multi / async — pool pipeline (`h5::high_throughput`)

The producer thread fans compression out to the worker pool, then drives the
chunk I/O itself. Under MT it takes the global HDF5 lock around each C-API op; the
workers compress in parallel **off the lock**. `single` = 1 worker, `multi`/`async`
= N workers.

```
  producer thread                                 worker pool (N threads)
  ───────────────                                 ──────────────────────
  source buffer                                   ┌─────────────────────┐
      │  split per chunk                          │ [w1] gzip           │
      ▼                                           │ [w2] gzip  parallel │
   chunk ──────── submit ────────────────────────►│ [w3] gzip  compress │  (no HDF5,
      ▲                                           │ [..] gzip           │   no lock)
      │  drain (futures; bounded by backpressure) └──────────┬──────────┘
      ▼ ◄─────────────────── results ───────────────────────┘
  ⟦H5Dwrite_chunk⟧   ◄════ MT: under the one process-global HDF5 lock
      │
      ▼  file
```

Compression is parallel; chunk I/O is serialized under the global lock. Several
producers can write into one file concurrently and HDF5 never sees two threads at
once (TSan-clean — see `test/H5collector.cpp`):

```
  producer A ─┐
  producer B ─┤  ⟦HDF5 op⟧ ─► ║ global HDF5 lock ║ ─ serialized ─► ⟦HDF5⟧   one op
  producer C ─┘   (M threads)      (1 holder)                                 at a time
        distinct datasets into ONE file — HDF5 never sees two threads
```

### Filter variations — what the worker stage actually does

The pipeline shape is identical across filters; only the per-chunk worker cost
changes (this is why the numbers below split by filter):

```
  gzip-6    : [w] ─ deflate(level 6) ─►   heavy CPU  → ~9 cores busy, pool clearly wins
  gzip-0    : [w] ─ deflate framing   ─►   light CPU  → ~2 cores, pool ≈ break-even
  no-filter : [w] ─ passthrough       ─►   ~no CPU   → ~1 core; pool/queue is pure overhead
              (the chunk still rides the submit→drain handoff, but no compression
               is done — the machinery has nothing to amortize)
```

> Earlier-measurement footnote: a first MT re-run reported a ~2.4x gzip-6
> slowdown. That was a build artifact — the hand-compiled MT binary dropped
> `-DH5CPP_HAS_LIBDEFLATE=1` and fell back to slow zlib, so it compared
> libdeflate-classic vs zlib-MT. Rebuilt identically (both libdeflate), classic
> and MT match. The corrected numbers below are authoritative.

## Throughput — MB/s (median of 3), classic vs MT (both libdeflate)

### gzip-6 (real compression)
| case                         | classic | MT (lock) | MT / classic |
| ---------------------------- | ------: | --------: | -----------: |
| raw `H5Dwrite_chunk`         |  3833.1 |    3563.0 |        0.93x |
| direct gzip (libdeflate)     |   141.7 |     130.9 |        0.92x |
| HDF5 gzip (stock)            |    56.1 |      56.1 |        1.00x |
| single-thread pipeline       |   140.4 |     141.1 |        1.00x |
| multi-thread pipeline        |  1244.2 |    1220.4 |        0.98x |
| async pipeline               |  1218.9 |    1228.5 |        1.01x |

### gzip-0 (filter attached, no compression work)
| case                   | classic | MT (lock) | MT / classic |
| ---------------------- | ------: | --------: | -----------: |
| raw `H5Dwrite_chunk`   |  3939.2 |    3771.5 |        0.96x |
| direct gzip            |  3224.1 |    3280.0 |        1.02x |
| HDF5 gzip (stock)      |  1459.7 |    1479.2 |        1.01x |
| single-thread pipeline |  2209.0 |    2096.2 |        0.95x |
| multi-thread pipeline  |  1606.5 |    1474.5 |        0.92x |
| async pipeline         |  1633.3 |    1485.8 |        0.91x |

### no-filter
| case                   | classic | MT (lock) | MT / classic |
| ---------------------- | ------: | --------: | -----------: |
| raw `H5Dwrite_chunk`   |  3775.6 |    3979.8 |        1.05x |
| direct gzip            |  3736.0 |    3962.0 |        1.06x |
| HDF5 write (stock)     |  3149.6 |    3209.9 |        1.02x |
| single-thread pipeline |  2960.9 |    2994.7 |        1.01x |
| multi-thread pipeline  |  2828.6 |    2875.4 |        1.02x |
| async pipeline         |  2844.8 |    2912.6 |        1.02x |

## Thread sweep — gzip-6, multi-thread pipeline MB/s (libdeflate)
| threads | classic | MT (lock) | MT / classic |
| ------: | ------: | --------: | -----------: |
|       1 |   141.7 |     139.7 |        0.99x |
|       2 |   274.9 |     275.6 |        1.00x |
|       4 |   520.7 |     521.9 |        1.00x |
|       8 |   918.5 |     930.0 |        1.01x |
|      16 |  1123.1 |    1210.8 |        1.08x |

The MT path tracks classic within run-to-run noise at every worker count — the
same compression-bound scaling curve.

## Conclusions
1. **The global HDF5 mutex is throughput-neutral.** classic ≈ MT across *every*
   filter regime and *every* worker count (run-to-run noise, gzip-0 a touch
   noisier). Serialising all HDF5 access through one global lock costs nothing for
   write throughput, because the worker pool — which does the heavy compression —
   never touches HDF5 and runs fully in parallel either way.
2. **The lock is taken once per top-level op, uncontended for a single producer**
   (thread-local recursion depth), and the conversion-off handle backing is the
   same 8-byte handle — neither shows up in the numbers.
3. **Concurrent-writer safety comes free.** Multiple producers writing distinct
   datasets into one file are TSan-clean (`test/H5collector.cpp`: 62/62 + 40/40
   stress, clang-20) at no throughput cost vs the single-producer path.
4. The gap to the raw chunk-I/O ceiling (gzip-6 ≈ 1.22 GB/s vs ~3.6 GB/s raw) is
   **compression** throughput, not the pipeline or the lock — gzip-6 deflate
   dominates. The *pool* only earns its keep when there is real compression to
   parallelize: at gzip-0 (~2 CPUs) / no-filter (~1 CPU) the worker fan-out is
   mostly machinery cost, so the dispatcher should bypass the pool for
   no-op/no-filter datasets — still an open follow-up on 287.
5. **#16 (fine-grained locking) is NOT needed for throughput.** The coarse
   whole-`h5::write` lock was directly proven free here (env-bypass of the lock
   under MT changed nothing). Finer-grained locking would only help *concurrent
   producers* overlap their compression phases — a future nicety, not a
   correctness or single-writer-speed fix.
6. **Harness lesson:** build the benchmark via CMake (or pass the full
   `CXX_DEFINES`, incl. `-DH5CPP_HAS_LIBDEFLATE`) — a hand-compile that drops it
   silently halves compression throughput and produces misleading comparisons.

## Commands
```sh
DEF=$(grep 'CXX_DEFINES =' build-profile/examples/CMakeFiles/examples-multithreaded-pipeline.dir/flags.make | sed 's/CXX_DEFINES = //')   # carries -DH5CPP_HAS_LIBDEFLATE=1
g++ -O2 $DEF        <flags> pipeline.cpp -o all_classic <libs>     # classic
g++ -O2 $DEF -DH5CPP_MULTITHREAD <flags> pipeline.cpp -o all_mt <libs>   # MT (global lock)
export H5CPP_BENCH_DIR=/dev/shm
./all_classic ; ./all_mt
# regimes: H5CPP_BENCH_GZIP_LEVEL=0 ; H5CPP_BENCH_FILTERS=0 ; sweep: H5CPP_BENCH_THREADS=$n
```

---

# Addendum — `h5::append` (packet table) under the global lock

`h5::append` / `h5::pt_t` was the one write gateway that did **not** route through
`on_collector` (`H5Dappend.hpp` called `H5Dwrite` / `H5Dwrite_chunk` / `set_extent`
directly). It now does: every append-chunk flush, `flush()`'s pool drain, and the
`pt_t` open path run under the process-global HDF5 lock — so packet-table streaming
is concurrency-safe like `h5::write`. Same per-call discipline as the concurrent-
writers case: build the `pt_t` handles up front (single-threaded); producers
thereafter make no direct HDF5 calls.

> **See [`profile-all-cases.md`](profile-all-cases.md)** for the consolidated
> single-methodology profile of every case below (median of 9 + ranges + derived
> µs/chunk and speedups) — the baseline the `exp-vyukov` experiment is measured against.

Workload: 256 MB of `double` streamed via `h5::append`, chunk 8192, HDF5 1.12.3.
Box: i7-11700K — **8 physical cores, 16 SMT threads**. **Harness + bundled libdeflate
compiled `-O3 -DNDEBUG`**; best of 9 (median). The driver (`append_bench`) takes
`<producers> <total_doubles> <chunk> <gzip> <pool_threads>`, where `gzip < 0` means no
filter, `pool_threads = 0` means no pool (basic inline); see the Commands block.

> **Build-flags matter (two corrections, folded in).**
> 1. The first cut of this addendum linked the bundled libdeflate from a **non-optimized**
>    build (`build-mt` / `build-tsan`, whose C flags are just `-g` → `-O0`). That crippled
>    libdeflate to ~zlib speed and made the single-thread append path look like it merely
>    *matched* the C-API baseline.
> 2. Switching to `-O3`: the **codec** barely moves (libdeflate is already saturated at
>    `-O2` — 132 vs 134 MB/s), but the **h5cpp harness/memory-bound paths** gain 7–20%
>    (pool-8 gzip 714→**857**, no-filter ~1.6→~1.9 GB/s).
>
> All tables below are `-O3` with the optimized libdeflate. *Net: libdeflate is ~2.8× zlib
> at level 4, so h5cpp's single-thread append beats the stock C-API by that factor, and the
> pool stacks on top. A perf build must compile the bundled C codecs `-O3`; the
> default/debug CMake build does not.*

### 0. Baseline — stock HDF5 C-API (its own gzip/deflate plugin) vs h5cpp libdeflate, level 4, single thread
Baseline = no h5cpp, no libdeflate, no pool — `H5Pset_deflate(dcpl, 4)` (HDF5's
**built-in `H5Z_FILTER_DEFLATE` / gzip plugin**, backed by system zlib) + `H5Dwrite`.
`stream` mirrors `h5::append` (unlimited 1-D dataset, extend + hyperslab write per chunk).

| path | MB/s | vs baseline |
|---|--:|--:|
| C-API deflate-4 (HDF5 gzip plugin), stream | 43.1 | 1.0× |
| C-API deflate-4 (HDF5 gzip plugin), bulk (one `H5Dwrite`) | 42.9 | 1.0× |
| **h5cpp `h5::append`, basic / no pool (libdeflate)** | **118.6** | **2.75×** |

The single-thread h5cpp path is **2.75× the stock C-API floor** — purely the codec:
libdeflate beats zlib by that margin at level 4 on this input, at the **same ratio**.
Isolated codec microbench (one 64 KB chunk, no HDF5) confirms it — libdeflate **132
MB/s** vs zlib **47 MB/s** on the low-compressibility data, **735 vs 458** on
compressible data. (Built `-O0`, libdeflate collapses to ~44 MB/s ≈ zlib — that was
the bug.) On top of this codec win, the pool (§2) multiplies again: **857 MB/s at
`threads{8}` = 19.9× the C-API baseline.**

Workload note: this `sin()` data is genuinely **low-compressibility — 1.1:1, 256 MB →
248 MB (~8% off)** — so deflate-4 is compression-*scan* bound, the regime where
libdeflate's lead over zlib is largest.

### 1. Lock cost — classic vs MT  *(throughput-neutral)*
| pipeline | classic | MT (global lock) |
|---|--:|--:|
| pool `h5::threads{4}`, gzip-4 | 473.1 MB/s | 475.7 MB/s |
| pool `h5::threads{4}`, no filter | 1585.9 MB/s | 1616.1 MB/s |

The uncontended lock around each append-chunk flush is in the noise (MT is within ±1%,
both directions) at every compression level.

### 2. With compression (gzip-4), the **pool** is the throughput lever — single-producer sweep
| `h5::threads{N}` | MB/s | speedup vs 1 |
|--:|--:|--:|
| 1 (basic) | 118.6 | 1.0× |
| 2 | 246.1 | 2.1× |
| 4 | 474.9 | 4.0× |
| 8 | 857.5 | 7.2× |

Near-linear to **8 workers = the 8 physical cores** (7.2× ≈ 90% scaling efficiency);
gzip is CPU-bound, so it tops out there. One producer already keeps all 8 cores full.

### 3. Compression **disabled** — the pipeline/lock vanish; the pool turns into pure overhead
Single producer, no filter, chunk 8192:

| `h5::threads{N}` | no filter |
|--:|--:|
| 0 (basic) | 1875.4 MB/s |
| 1 | 1700.0 MB/s |
| 2 | 1655.1 MB/s |
| 4 | 1575.7 MB/s |
| 8 | 1598.7 MB/s |

Throughput is ~1.6–1.9 GB/s and **declines** the moment a pool is introduced: with
nothing to compress, the pool worker just allocates a chunk-sized buffer, memcpy's the
data in and back, and hops a thread — all cost, no benefit.

The fastest no-filter config is **no pool at all** (basic inline write) — and unlike
the pool path it *gains* from concurrent producers, because the per-element buffer
fills overlap and only `H5Dwrite_chunk` serializes under the lock:

| producers | no pool (basic) | `threads{4}` pool | pool penalty |
|--:|--:|--:|--:|
| 1 | 1875.4 MB/s | 1575.7 MB/s | −16% |
| 4 | 2149.7 MB/s | 1807.7 MB/s | −16% |

This is the concrete case for the still-open 287 follow-up: *the dispatcher should
bypass the pool for no-op / no-filter datasets* — the pool is a ~16% tax precisely
where it can do no useful work.

### 4. Concurrent appenders — safety, not extra speed  *(gzip-4, pool=8, 256 MB total)*
| producers | MB/s |
|--:|--:|
| 1 | 885.0 |
| 2 | 840.5 |
| 4 | 837.1 |
| 8 | 801.0 |

Flat (~840): a single appender already saturates the 8-core pool, so extra producers
only add global-lock contention. The win is that these M producers streaming into one
file are now **TSan-clean** (`test/H5collector.cpp` *"concurrent appenders — one file,
one pool"*, clang-20 `-fsanitize=thread`) — before this change that path was an
unlocked C-API race (UB).

### 5. 4 independent producers — compressor pool on vs off  *(the revealing pattern)*
4 threads each streaming into their own packet table in one file, 256 MB total:

| workload | 4 producers + `threads{4}` | 4 producers + no pool | 1 producer (ref) |
|---|--:|--:|--:|
| gzip-4    | 459.4 MB/s | **117.3 MB/s** | 475.8 (pool) / 118.6 (no pool) |
| no filter | 1807.7 MB/s | 2149.7 MB/s | 1875.4 (no pool) |

The gzip row is the finding: **4 no-pool producers (117.3) = 1 no-pool producer
(118.6).** The producer threads buy *nothing*. In the basic (no-pool) pipeline the
filter chain runs **inline, inside `on_collector`** (`H5Zpipeline_basic.hpp` — compress
then `H5Dwrite_chunk`), so the global lock serializes all four producers' gzip work.
And `459.4 ≈ 475.8`: the worker pool delivers the same ~4× whether 1 or 4 producers
feed it. **With compression, the pool is the parallelism — the producer threads are
not.** (No-filter has no compute to serialize, so the producers' lock-free buffer fills
overlap and the pool is a slight tax, as in §3.)

**Open optimization (consequence):** compression touches no HDF5 — only
`set_extent` / `H5Dwrite_chunk` do — and the basic pipeline already compresses into
per-`pt_t` buffers. Narrowing the lock so the inline filter chain runs *outside*
`on_collector` (lock only the C-API calls) would let N no-pool producers parallelize
gzip on their own threads — a pool-free ~Nx, and less lock contention for the pooled
path too. Currently the whole append-flush is locked; this is the next lever after
the no-filter pool-bypass.

### 6. Cost of the thread-safe dispatch machinery — and what it buys
*"How much does the multithreaded / thread-safe-queue path cost vs single-thread, and
what's the upside?"*

First, a correction to the premise: the live `worker_pool_t` does **not** use a
Vyukov queue. It dispatches through **`std::mutex` + `std::queue` + a futex doorbell`**
(`H5Pthreads.hpp`). The Vyukov bounded MPSC/SPMC/MPMC queues in `H5Qall.hpp` exist but
are **dead** — they were built for the retired `io_collector_t` and are now instantiated
by nothing. (They require C++20 — `std::bit_ceil`, `std::atomic::wait` — and the library
builds `-std=c++20`, so they **are** compiled and available; just unused. The
`exp-vyukov` branch wires them into the pool.)

**Cost per operation, in isolation** (2 M ops, `-O3`, single thread):

| operation | ns/op | notes |
|---|--:|---|
| inline call (no queue, no thread) | 4 | baseline |
| `std::mutex` + `std::queue` push+pop (the live queue) | 28 | uncontended |
| Vyukov MPSC push+pop (dead, but compiled — build is C++20) | 13 | what a lock-free queue *would* cost |
| **full `worker_pool::submit` + run, 1 worker** | **~500/task** | packaged_task alloc + `std::future` + doorbell + thread handoff |

The thread-safe **queue itself is ~28 ns — only ~6% of the ~500 ns per-task dispatch
cost.** The rest is the `std::make_shared<std::packaged_task>` heap allocation, the
`std::future` shared-state, and the cross-thread wake/handoff. So swapping in the
Vyukov queue would shave ~15 ns of ~500 — **~3%, not worth resurrecting**; the alloc +
future + handoff dominate, not the queue.

**Net cost vs single-thread, on the real workload** (pool=1 = full machinery, *zero*
parallelism, vs basic inline):

| workload | basic (inline) | pool=1 (1 worker, full queue) | Δ |
|---|--:|--:|--:|
| gzip-4 | 118.6 MB/s | 124.4 MB/s | **+4.9% (faster)** |
| no filter | 1875.4 MB/s | 1700.0 MB/s | **−9.4% (slower)** |

The sign flips with the per-task work size. A gzip chunk takes ~480 µs to compress, so
the ~500 ns dispatch is **0.1%** of it — invisible, and *more* than repaid because the
worker compresses chunk N while the main thread writes chunk N−1 and copies N+1 (a
**+5% pipelining win even with one worker**). A no-filter chunk is ~trivial (a memcpy),
so the same ~500 ns is **~5%** of it and there is nothing to overlap → a **~9% loss**.

**So, is multithreading "slower"?** Only when the per-chunk work is trivial (no/cheap
filter), where it costs ~9%. For its intended use — compression — it is **never net
slower**: +5% at one worker, and the real prize below.

**What the multithreaded path buys:**
1. **Parallel compression — the headline.** `threads{8}` = **7.2× single-thread** on
   gzip-4 (§2), ~90% scaling across the 8 physical cores. ~500 ns/chunk of overhead
   against ~480 µs/chunk of compress is a rounding error next to the 7× speedup.
2. **Compute/I/O overlap for free** even at `threads{1}` (+5%, above).
3. **Concurrent-writer safety.** M producer threads can stream into one file at once,
   TSan-clean (§4, `test/H5collector.cpp`) — impossible on the inline path against a
   Threadsafety-OFF HDF5 without the global lock.

Rule of thumb the numbers give: **engage the pool when a filter is set; skip it for
no-op / cheap-filter datasets** (the §3 / §5 bypass) — there the ~9% / ~16% queue tax
is the whole story and there is no compression to parallelize.

## Commands (append addendum)
The driver mirrors the `pt_t` paths exercised by `test/H5collector.cpp`
*"concurrent appenders"*: P producers each stream into their own packet table in one
file sharing the FAPL pool.
```sh
HV=/usr/local/HDF_Group/HDF5/1.12.3
# Build the bundled libdeflate at -O3 (the CMake build-mt/build-tsan dirs compile it at
# -O0 (C_FLAGS=-g) which cripples the codec to ~zlib speed; build-profile is only -O2).
LD=thirdparty/libdeflate/v1.25.0; mkdir -p /tmp/ld_o3
for s in adler32 crc32 deflate_compress deflate_decompress gzip_compress gzip_decompress \
         utils x86/cpu_features zlib_compress zlib_decompress; do
  gcc -O3 -DNDEBUG -I$LD -I$LD/lib -c $LD/lib/$s.c -o /tmp/ld_o3/$(echo $s|tr / _).o
done; ar rcs /tmp/ld_o3/libdeflate_o3.a /tmp/ld_o3/*.o
DEF=/tmp/ld_o3/libdeflate_o3.a

PRE="-O3 -std=c++17 -DH5CPP_HAS_LIBDEFLATE=1 -I. -I$LD -I$HV/include"
POST="$DEF -L$HV/lib -lhdf5 -lz -lpthread -Wl,-rpath,$HV/lib"
g++                  $PRE append_bench.cpp -o bench_classic $POST   # classic
g++ -DH5CPP_MULTITHREAD $PRE append_bench.cpp -o bench_mt   $POST   # MT (global lock)

# args: <producers> <total_doubles> <chunk> <gzip> <pool_threads>   (gzip<0 = no filter, pool=0 = no pool)
./bench_mt 1 33554432 8192  4 8     # gzip-4, single producer, 8-worker pool
./bench_mt 4 33554432 8192 -1 0     # no filter, 4 concurrent producers, no pool

# stock C-API baseline (§0): HDF5's own built-in deflate/gzip filter (H5Pset_deflate), no h5cpp
g++ -O3 -std=c++17 -I$HV/include capi_gzip_bench.cpp -o capi_gzip -L$HV/lib -lhdf5 -lz -Wl,-rpath,$HV/lib
./capi_gzip 33554432 8192 4 0       # args: <total_doubles> <chunk> <gzip> <bulk>  (bulk: 0=stream,1=one H5Dwrite)

# isolated codec microbench (§0): libdeflate vs zlib, no HDF5 — proves the 2.8× is the codec
g++ -O3 -std=c++17 -I$LD codec_micro.cpp -o codec_micro $DEF -lz && ./codec_micro

# dispatch-cost microbench (§6): worker_pool submit vs raw mutex-queue (+Vyukov needs -std=c++20)
g++ -O3 -std=c++17 -DH5CPP_HAS_LIBDEFLATE=1 -I. -I$LD -I$HV/include queue_micro.cpp -o queue_micro $POST
./queue_micro
```
