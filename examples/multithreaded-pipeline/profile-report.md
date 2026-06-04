# #287 Multithreaded Pipeline — Profile & Analysis

Combined report (the former `profile-report.md`, `profile-all-cases.md`, and
`hdf5-2.1.1-throughput-analysis.md`), with throughput **refreshed against the
current examples** — `pipeline-write.cpp` (a block of random floats written to a
chunked gzip dataset through the worker-pool pipeline) and `pipeline-read.cpp`
(read back through the parallel reader).

---

## 1. The model — global-mutex MT

The parallel write path:

- A plain chunked write uses h5cpp's **direct-chunk** pipeline (`basic_pipeline_t` →
  `H5Dwrite_chunk`) by default — no opt-in.
- **`h5::threads{N}`** — a per-dataset **DAPL** property (survives `H5Dget_access_plist`,
  so no `#286` registry) that fans the gzip stage across one process-global worker pool
  (`pool_pipeline_t`); `h5::backpressure{M}` bounds in-flight chunks.

gzip compression fans out across the pool; the `H5Dwrite_chunk` calls stay on the caller
thread. Building with **`-DH5CPP_MULTITHREAD`** additionally wraps every HDF5 C-API call in
one process-global recursive mutex (HDF5-threadsafe style — a lock, not a thread), making
concurrent writers to one file safe. Compression never touches HDF5, so the lock is
throughput-neutral on that path and a no-op in a classic build.

---

## 2. Results — `pipeline-write.cpp` / `pipeline-read.cpp` (this machine: 16 cores, HDF5 1.12.3, libdeflate)

16 M random `float`s (64 MiB), 1 M-float chunks. **Build with `-DH5CPP_HAS_LIBDEFLATE`** —
a hand-compile that drops it silently falls back to zlib and ~halves compression throughput.
`pipeline-write` writes the dataset; `pipeline-read` reads it back through the parallel
reader (`h5::threads{N}` → `pool_pipeline_t`, parallel inflate).  Run write first.

### Thread sweep — gzip-6, worker count
| workers | write MiB/s | write Mfloat/s | read MiB/s | read Mfloat/s |
| ------: | ----------: | -------------: | ---------: | ------------: |
|       1 |       103.5 |           27.1 |      452.3 |         118.6 |
|       2 |       197.8 |           51.9 |      797.8 |         209.1 |
|       4 |       340.2 |           89.2 |     1261.9 |         330.8 |
|       8 |       528.5 |          138.5 |     1354.5 |         355.1 |
|      16 |       617.5 |          161.9 |     1311.0 |         343.7 |

**Both stages parallelise.**  Write scales ~6× (1→16) — it is deflate-bound and each chunk is
one large compression task, so it keeps scaling to 16 workers.  Read scales ~3× and
**saturates at ~8 workers**: inflate is far cheaper than deflate, and with only 16 chunks
(1 M-float each) the serial `H5Dread_chunk` I/O and the chunk-count granularity — not inflate
throughput — become the limit past 8 workers.  With finer chunks the read scales further:
64 K-float chunks (256 chunks) reach **~3.2 GB/s at 16 workers** (read 479 → 3253 MiB/s, 6.8×).

> Engaging the parallel reader requires the `threads{N}` tag to reach the read dispatch via the
> dataset's real access plist (`H5Dget_access_plist`), e.g. `h5::open(fd, "data", h5::threads{N})`.

### Filter-level sweep — 16 workers, 1 M-float chunks
| gzip | write MiB/s | read MiB/s |
| ---: | ----------: | ---------: |
|    0 |       867.6 |     1756.4 |
|    4 |       538.8 |      998.5 |
|    6 |       548.2 |     1292.9 |
|    9 |       562.1 |     1228.7 |

> Note: random floats are near-incompressible, so the output barely shrinks and the absolute
> numbers reflect **deflate's CPU effort on random data**.  16-way parallel inflate keeps the
> read *above* the write at every level — within ~1.4× of the gzip-0 chunk-I/O ceiling — and
> read is faster than write throughout because inflate is cheaper than deflate.  Both stages
> are genuine parallelism wins.

---

## 3. Conclusions

1. **The global HDF5 mutex is throughput-neutral.** Classic ≈ MT across every filter regime
   and every worker count (original profiling: within run-to-run noise). Serializing all
   HDF5 access through one lock costs nothing for write throughput, because the worker pool —
   which does the compression — never touches HDF5 and runs fully in parallel either way.
2. **Concurrent-writer safety comes free.** Multiple producers writing distinct datasets into
   one file are TSan-clean (`test/H5collector.cpp`) at no throughput cost vs the single-producer
   path.
3. **Compression is the bottleneck, and the pool is the lever.** The gap to the raw chunk-I/O
   ceiling is deflate, not the pipeline or lock. The pool earns its keep only when there is
   real compression to parallelize — at gzip-0/no-filter the fan-out is mostly machinery, so
   the dispatcher should bypass the pool for no-op/no-filter datasets (open follow-up).
4. **Fine-grained locking (`#16`) is not needed for throughput.** The coarse whole-`h5::write`
   lock was proven free (env-bypass under MT changed nothing). Finer locking would only let
   *concurrent producers* overlap their compression phases — a future nicety, not a
   correctness or single-writer-speed fix.
5. **Harness lesson:** build via CMake or pass `-DH5CPP_HAS_LIBDEFLATE`; a hand-compile that
   drops it silently halves compression throughput and produces misleading comparisons.

### Reference numbers (original append-harness profiling)
| metric | value |
|---|--:|
| codec: libdeflate vs zlib (L4, isolated) | 132 vs 47 MB/s (2.8×) |
| single-thread append vs C-API baseline | 119 vs 43 MB/s (2.74×) |
| pool scaling, gzip-4, 8 cores | 7.4× (~93% efficiency) |
| global-lock cost | −0.5..−1.0% (free) |
| live queue op (mutex + std::queue) | 28 ns |
| Vyukov MPSC op (isolated) | 12.5 ns |
| full per-task dispatch cost | ~500 ns |

---

## 4. HDF5 2.1.1 — does it provide true async / parallel I/O?

**No true async, and no parallel I/O, on a stock 2.1.1 build.** The `_async` API is a façade
over a synchronous native VOL; the new concurrency mode still serializes every call; the
filter pipeline is still single-threaded; every advanced VFD is MPI-bound or OFF. This
**confirms the experiment series from the source**: the single-threaded-I/O ceiling is
*intrinsic to HDF5*, not an artifact of the global lock.

**Synthesis:**
1. **The experiment series is vindicated by the source.** No userspace coordinator beats the
   lock because HDF5 I/O is serial — and the 2.1.1 source confirms the library is intrinsically
   single-threaded: no internal async, no parallel API, no threaded filtering.
2. **The global-lock decision stands.** 2.1.1's own threadsafety is still a global exclusive
   lock; rebuilding against it would relocate h5cpp's mutex, not parallelize anything.
3. **h5cpp's worker-pool / libdeflate / `H5Dwrite_chunk` design is the right one** and is not
   superseded by anything in 2.x — it already extracts the only available parallelism
   (compression overlapped with serialized I/O).
4. **True async is a deployment, not a code change:** rebuild HDF5 `--enable-threadsafe` +
   Argobots + the external Async VOL, stack via `H5Pset_vol`, use `H5Dwrite_async` + event
   sets. Worth it only when I/O (not compression) is the bottleneck *and* the backend can
   overlap I/O — on plain local POSIX the Async VOL's single background thread still serializes.
   The larger I/O-parallelism wins live in **parallel HDF5 / subfiling (MPI)** — a different
   deployment target.
5. **Cheapest real next step:** expose the **tuning knobs** (chunk-cache + W0, page buffer +
   PAGE strategy, meta-block-size, `DONT_FILTER_PARTIAL_CHUNKS`) through h5cpp's
   FAPL/DCPL/DAPL surface and sweep them — recoverable throughput with zero architectural risk.

---

## 5. The `exp-vyukov` question — dispatch cost

The pool's `mutex + std::queue` op costs **28 ns**; an isolated bounded **Vyukov MPSC** op is
**12.5 ns**; full per-task dispatch is **~500 ns**. Swapping the queue therefore saves ~15 ns
of ~500 ns — **predicted ≤3% of dispatch, i.e. invisible end-to-end**. `exp-vyukov` exists to
measure whether reality matches that prediction.

---

## 6. Reproduce

```sh
cmake --build <build> --target examples-multithreaded-pipeline-write examples-multithreaded-pipeline-read
# thread sweep (worker count) — write then read each point:
for t in 1 2 4 8 16; do
  H5CPP_BENCH_THREADS=$t ./examples-multithreaded-pipeline-write
  H5CPP_BENCH_THREADS=$t ./examples-multithreaded-pipeline-read
done
# filter sweep — write at each level, read it back:
for g in 0 4 6 9; do
  H5CPP_BENCH_GZIP=$g H5CPP_BENCH_THREADS=16 ./examples-multithreaded-pipeline-write
  H5CPP_BENCH_THREADS=16 ./examples-multithreaded-pipeline-read
done
```

`H5CPP_BENCH_THREADS` sizes the global worker pool (write: deflate fan-out; read: inflate
fan-out). Other write knobs: `H5CPP_BENCH_{N, CHUNK, GZIP}`. A hand-compile must pass
`-DH5CPP_HAS_LIBDEFLATE` (and link libdeflate) or compression throughput halves.
