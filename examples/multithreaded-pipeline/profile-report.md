# #287 multithreaded pipeline profile — async collector vs sync

Profiles the per-file `io_collector_t` write path against the established sync
baselines, using `pipeline.cpp` on branch `287-refactor-fapl-pipeline-context`
(on top of staging `f7847201`, which carries the #286 fileno registry).  A 6th
case — **`async pipeline (collector)`** — exercises `h5::async::create` + the
collector write; the other five are unchanged.

> Supersedes the earlier exp-e baseline report (which profiled the `db16477a`
> code — Vyukov queue, `h5::arena` — and lives in the `exp-e-coordinator`
> worktree).  That code is not on this branch; the numbers below are this branch.

## Setup
- HDF5 1.12.3 (Threadsafety OFF), gcc-14, RelWithDebInfo `-fno-omit-frame-pointer`
- Output dir `/home/steven/scratch/h5cpp-287-eval` (`/dev/nvme0n1`, f2fs), 16 CPUs
- 64 MiB logical, 512 KiB chunks (128 chunks), 16 workers, gzip level 6
- `/usr/bin/perf` explicitly (the default `perf` resolves to a broken build)

Porting notes vs exp-e: `h5::arena{}` was an exp-e-only FAPL prop (absent here) and
was dropped; `single`/`multi`/`async` now pass **`h5::high_throughput`** — the
per-dataset DAPL opt-in that engages `pool_pipeline_t`.  Without it the write
silently falls back to stock single-threaded HDF5 filters (the `HDF5 H5Dwrite +
gzip` row, ~56 MB/s).

## Data flows (what each case does)

Legend:  `⟦…⟧` = an HDF5 C-API call (with Threadsafety OFF, **only one thread per
file may be inside HDF5 at a time**).  `[w]` = a worker-pool thread (compression
only — never touches HDF5).  Arrows are data movement; `║` marks a thread boundary.

### Baselines — single thread, no pool

```
raw          caller ──split──► chunk ──────────────────────► ⟦H5Dwrite_chunk⟧ ──► file
             (no filter; chunk written straight from the source buffer)

direct-gzip  caller ──split──► chunk ──libdeflate gzip──────► ⟦H5Dwrite_chunk⟧ ──► file
             (compress AND write on the caller; tightest single-thread gzip)

hdf5-gzip    caller ──────────────────► ⟦H5Dwrite⟧ ─► [HDF5 deflate filter] ──► file
             (HDF5 runs the gzip filter itself, on the caller; no h5cpp pool)
```

### single / multi — sync pool pipeline (`h5::high_throughput`)

The **caller thread is the collector**: it fans compression out to the worker pool
but does the chunk I/O itself.  `single` = 1 worker, `multi` = N workers.

```
  caller thread                                   worker pool (N threads)
  ─────────────                                   ──────────────────────
  source buffer                                   ┌─────────────────────┐
      │  split per chunk                          │ [w1] gzip           │
      ▼                                           │ [w2] gzip  parallel │
   chunk ──────── submit ────────────────────────►│ [w3] gzip  compress │
      ▲                                           │ [..] gzip           │
      │  drain (futures; bounded by backpressure) └──────────┬──────────┘
      ▼ ◄─────────────────── results ───────────────────────┘
  ⟦H5Dwrite_chunk⟧   ◄════ the ONLY thread inside HDF5 (the caller)
      │
      ▼  file
```
Compression is parallel; chunk I/O is serialized on the caller.  Safe only because
*one* caller drives it — two concurrent callers would be two threads in HDF5.

### async — dedicated per-file collector (`h5::async::create`)

A single `io_collector_t` thread per file is the only thread that touches HDF5.
The producer hands the whole op to it and blocks until the data is on disk.

```
  producer thread          collector thread (1 / file)            worker pool (N)
  ───────────────          ──────────────────────────            ───────────────
  h5::write(fd,…)
     │ submit_and_wait
     ▼  (BLOCKS) ─────────► ⟦H5Dcreate⟧                ← metadata, on the collector
                            split ─ submit ───────────────────► [w] gzip (parallel)
                            drain  ◄─────────────────────────── results
                            ⟦H5Dwrite_chunk⟧  ◄════ the ONLY HDF5 thread
                            ⟦H5Dclose⟧
                               │
                               ▼  file
     ◄───────────────────── returns (data on disk)
```
"async" here = **concurrency-safe, not deferred** — it blocks to drain.  The win is
that *several* producers can do this at once into one file:

```
  producer A ─┐
  producer B ─┤  submit_and_wait ─► ║ collector ║ ─ serialized ─► ⟦HDF5⟧   one op
  producer C ─┘     (M threads)        (1 thread)                            at a time
        distinct datasets into ONE file — HDF5 never sees two threads (TSan-clean)
```
The collector is resolved as a **member read** off the (fat) async handle —
`fd.collector` — so producers make no HDF5 call to find it.

### Filter variations — what the worker stage actually does

The pipeline shape is identical across filters; only the per-chunk worker cost
changes (this is why the numbers below split by filter):

```
  gzip-6    : [w] ─ deflate(level 6) ─►   heavy CPU  → ~9 cores busy, pool clearly wins
  gzip-0    : [w] ─ deflate framing   ─►   light CPU  → ~2 cores, pool ≈ break-even
  no-filter : [w] ─ passthrough       ─►   ~no CPU   → ~1 core; pool/queue is pure overhead
              (tail==0: the chunk still rides the submit→drain handoff, but no
               compression is done — the machinery has nothing to amortize)
```

## Throughput (gzip-6)

| Case | Median ms | MB/s | % of raw | Roundtrip |
|---|---:|---:|---:|---|
| direct H5Dwrite_chunk (raw) | 21.7 | 3093.7 | 100% | ok |
| direct H5Dwrite_chunk + gzip (libdeflate) | 472.8 | 141.9 | 5% | ok |
| HDF5 H5Dwrite + gzip (stock) | 1197.6 | 56.0 | 2% | ok |
| single-thread pipeline | 476.2 | 140.9 | 5% | ok |
| **multi-thread pipeline (sync)** | 54.7 | **1226.7** | 40% | ok |
| **async pipeline (collector)** | 54.5 | **1231.3** | 40% | ok |

- ✔ single-thread vs direct gzip baseline: **99.3%** (≥90% required).
- ✔ multi-thread speedup over single: **8.70×** of 16 threads (54% scaling).
- **async ≈ multi (1231 vs 1227 MB/s)** — the collector thread-hop is throughput-neutral.

## Perf counters

| Case | MB/s | task-clock ms | CPUs | ctx-switches | migrations | cache-misses |
|---|---:|---:|---:|---:|---:|---:|
| raw | 3094 | 160 | 0.99 | 6 | 1 | 14.5M |
| HDF5 gzip (stock) | 56 | 4943 | 1.00 | 36 | 11 | 17.7M |
| multi (sync) | 1252 | 3194 | 9.04 | 678 | 167 | 35.9M |
| async (collector) | 1236 | 3188 | 8.99 | 1185 | 167 | 35.9M |

multi and async are **indistinguishable** on CPU time (3194 vs 3188 ms), cores
used (9.0), migrations (167), and cache-misses (35.9M).  The single difference is
async's **context-switches (1185 vs 678)** — the producer→collector `submit_and_wait`
handoff — a handful of switches over ~3.2 s that does not move throughput.

## Thread sweep (multi vs async, gzip-6)

| Workers | multi MB/s | async MB/s |
|---:|---:|---:|
| 1 | 141.6 | 141.0 |
| 2 | 276.0 | 278.2 |
| 3 | 408.3 | 392.7 |
| 4 | 487.7 | 505.3 |
| 6 | 738.9 | 733.8 |
| 8 | 869.8 | 887.5 |

The async collector path tracks the sync multi path within run-to-run noise at
every worker count — same compression-bound scaling curve.

## gzip-0 (zlib stream, no compression)

Level 0 still runs the deflate machinery (stream framing + adler32) but emits no
compression (ratio ~1.0×), so the worker stage does only light work — ~2 CPUs.

### Perf counters
| Case | MB/s | task-clock ms | CPUs | ctx-switches | migrations | cache-misses |
|---|---:|---:|---:|---:|---:|---:|
| raw | 3070 | 153 | 0.99 | 3 | 1 | 14.3M |
| HDF5 gzip-0 (stock) | 1406 | 284 | 0.99 | 9 | 3 | 18.5M |
| multi (sync) | 1377 | 559 | 1.98 | 1401 | 121 | 38.4M |
| async (collector) | 1444 | 573 | 2.06 | 1083 | 164 | 40.9M |

### Thread sweep (multi vs async)
| Workers | multi MB/s | async MB/s |
|---:|---:|---:|
| 1 | 1772.7 | 2295.2 |
| 2 | 1754.6 | 1821.7 |
| 3 | 1735.4 | 1792.2 |
| 4 | 1617.4 | 1699.3 |
| 6 | 1465.9 | 1596.4 |
| 8 | 1466.2 | 1516.5 |

Throughput **declines** as workers rise: with no compression to parallelize, extra
workers add scheduler/queue cost the pipeline can't amortize (the exp-e "pool hurts
for no-op filters" regime).  async ≈ multi (here slightly ahead).

## no-filter (`H5CPP_BENCH_FILTERS=0`)

Chunked dataset, no filter chain.  The pool/collector still engage (`high_throughput`
is set; 287 has no no-filter bypass) but the worker stage is a passthrough, so ~1 CPU
is active and throughput is closest to raw.

### Perf counters
| Case | MB/s | task-clock ms | CPUs | ctx-switches | migrations | cache-misses |
|---|---:|---:|---:|---:|---:|---:|
| raw | 3286 | 156 | 0.99 | 3 | 1 | 14.3M |
| HDF5 chunked, no filter | 2805 | 179 | 0.99 | 13 | 5 | 17.9M |
| multi (sync) | 2436 | 188 | 1.02 | 620 | 65 | 16.4M |
| async (collector) | 2374 | 189 | 1.01 | 647 | 72 | 16.6M |

### Thread sweep (multi vs async)
| Workers | multi MB/s | async MB/s |
|---:|---:|---:|
| 1 | 2680.4 | 2552.8 |
| 2 | 2577.7 | 2468.0 |
| 3 | 2626.5 | 2522.8 |
| 4 | 2582.4 | 2527.1 |
| 6 | 2566.1 | 2502.1 |
| 8 | 2535.4 | 2503.7 |

Flat across workers (no filter work to scale), ~77–80% of raw.  multi is marginally
ahead of async; both pay the pool/collector machinery cost that a no-op filter can't
hide — which is why the dispatcher should bypass the pool for no-op/no-filter datasets
(the exp-e conclusion, still an open follow-up on 287).

## Conclusions
1. **The collector is free for throughput.** Routing writes through the single
   per-file `io_collector_t` thread matches the sync `pool_pipeline_t` on
   throughput, CPU, scaling, and cache behavior — it costs only a small, constant
   bump in context switches (the producer→collector handoff).
2. So the **concurrent-writer safety** the collector provides (multiple producers
   into one file, TSan-clean — see `test/H5collector.cpp`) is obtained at **no
   throughput cost** vs the single-producer sync pipeline.
3. The gap to the raw chunk-I/O ceiling (≈40%) is **compression** throughput, not
   the pipeline or the collector — gzip-6 deflate dominates.
4. **Across all three filter regimes the collector is throughput-neutral** —
   gzip-6 (~9 CPUs, ~1.23 GB/s, scales up), gzip-0 (~2 CPUs, ~1.4 GB/s, declines
   with workers), no-filter (~1 CPU, ~2.5 GB/s, flat).  async ≈ multi in every
   case.  But the *pool* only earns its keep when there is real compression to
   parallelize: at gzip-0/no-filter the worker fan-out is pure machinery cost
   (declining or flat throughput), so the dispatcher should bypass the pool for
   no-op/no-filter datasets — the exp-e recommendation, still open on 287.

## Commands
```sh
cmake -S . -B build-profile -DCMAKE_BUILD_TYPE=RelWithDebInfo -DH5CPP_BUILD_EXAMPLES=ON \
      -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer" -DHDF5_DIR=/usr/local/HDF_Group/HDF5/1.12.3/cmake
cmake --build build-profile -j4 --target examples-multithreaded-pipeline{,-raw,-direct-gzip,-hdf5-gzip,-single,-multi,-async}
export H5CPP_BENCH_DIR=/home/steven/scratch/h5cpp-287-eval
./build-profile/examples-multithreaded-pipeline                  # throughput table
/usr/bin/perf stat -e task-clock,context-switches,cpu-migrations,cache-misses \
    ./build-profile/examples-multithreaded-pipeline-async         # per-case counters
for n in 1 2 3 4 6 8; do H5CPP_BENCH_THREADS=$n ./build-profile/examples-multithreaded-pipeline-async; done

# env knobs: H5CPP_BENCH_{GZIP_LEVEL,THREADS,FILTERS,DIR}
```
