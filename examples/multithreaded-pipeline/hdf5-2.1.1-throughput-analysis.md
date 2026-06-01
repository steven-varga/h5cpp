# HDF5 2.1.1 — does it provide true async I/O, and how to squeeze maximum throughput

Source examined: `~/src/hdf5/src/` (v2.1.1) + the installed build at
`/usr/local/HDF_Group/HDF5/2.1.1`. This is the "final destination" for h5cpp's async
pipeline. Five subsystems audited; all claims below are backed by file:line in the 2.1.1
tree.

## TL;DR
**No true async I/O, and no parallel I/O, on a stock 2.1.1 build.** The `_async` API is a
façade over a synchronous native VOL; the new concurrency mode still serializes every call;
the filter pipeline is still single-threaded; and every advanced VFD is MPI-bound or OFF.
This *confirms* the experiment series from the source: the single-threaded-I/O ceiling is
**intrinsic to HDF5**, not an artifact of the global lock. What 2.1.1 *does* give is a set
of **FAPL/DCPL/DAPL tuning knobs** that recover real throughput without changing the
architecture — and `H5Dwrite_chunk`, which h5cpp already uses.

---

## 1. True async I/O? — NO (on a stock build)

The full `*_async` family exists — `H5Dwrite_async`, `H5Dread_async`, `H5Dwrite_multi_async`,
`H5Fcreate_async`, … (`H5Dpublic.h:979,1172,1186`; `H5Fpublic.h:391…`) — with event sets for
completion (`H5EScreate`/`H5ESwait`/`H5ESget_count`, `H5ESpublic.h:165,198,233`). But it is a
**pass-through, not async**:

- `H5Dwrite_async` only inserts a request token into the event set **if the VOL created
  one** (`H5D.c`: `if (NULL != token) H5ES_insert(...)`).
- The **native (terminal, on-disk) VOL never creates one**: `H5VL__native_dataset_write`
  takes `void H5_ATTR_UNUSED **req` and just calls the **blocking** `H5D__write`
  (`H5VLnative_dataset.c:397`); its request callbacks are all NULL
  (`H5VLnative.c:157-165`: `NULL /*wait*/, NULL /*notify*/, …`).
- So with the native VOL, `H5Dwrite_async` == synchronous `H5Dwrite` + a NULL check; the
  event set stays empty; **zero overlap.**
- The core library spawns **no** I/O thread for `_async` (`H5public.h:792`: "the library …
  not internally multi-threaded"). The real **Async VOL connector** (Argobots-based) is
  **external / out-of-tree** — the only "async" in 2.1.1's tree is a `fake_async` *test*
  stub (`test/vol.c:313`).

**To get true overlap** you must: rebuild HDF5 `--enable-threadsafe`, build **Argobots**,
build the **external Async VOL connector**, stack it via `H5Pset_vol`, then use
`H5Dwrite_async` + event sets. The installed build is **threadsafe-OFF** (`H5pubconf.h`),
so none of this is available today. And note: even then, the Async VOL runs ops on **one**
background user-thread over the same POSIX backend — it overlaps **compute with I/O** (which
h5cpp's worker pool already does for compression) but **not I/O with I/O** on local storage.

## 2. Parallel / multi-threaded I/O? — NO (still a global lock)

2.1.1 adds a second threadsafe mode, `H5_HAVE_CONCURRENCY` (rwlock-based, vs the classic
recursive-mutex `H5_HAVE_THREADSAFE`) — but at the API boundary it **always takes the rwlock
in WRITE (exclusive) mode** (`H5TSint.c:356` `H5TS_rwlock_wrlock`; **no** `rdlock` anywhere on
the API path). So it degenerates to a global mutex — **no two API calls ever run
concurrently, not even reads.** The concurrency mode is groundwork only: 2-file footprint
(`H5TSint.c`, `H5.c`), OFF by default (`CMakeBuildOptions.cmake:61`), undocumented, no
per-object/lock-free paths wired in. The installed build defines **neither**
`H5_HAVE_THREADSAFE` nor `H5_HAVE_CONCURRENCY` → **h5cpp must keep its own process-global
mutex**, and even a rebuilt threadsafe HDF5 would just relocate the same serialization.

## 3. Batched I/O (multi-dataset / selection / vector) — no help for this workload

- **`H5Dwrite_multi`** (`H5Dpublic.h:1163`, since 1.14): writes **N distinct datasets** in
  one call — wrong shape; cannot batch repeated appends to ONE growing dataset.
- **Selection I/O** (`H5Pset_selection_io`, AUTO by default): **disabled by any filter**
  (`H5Dchunk.c:2849`: "Don't use selection I/O if there are filters") and bypassed by
  `H5Dwrite_chunk` — so irrelevant to compressed chunked append.
- **Vector I/O** (`H5FD_write_vector`): the default **sec2 driver doesn't implement it**
  (`H5FDsec2.c:147-148` → NULL), so it falls back to a per-chunk `pwrite` loop
  (`H5FDint.c:740-779`). Only a win with MPI-IO or a custom VFD that supplies `write_vector`.

## 4. VFDs — none is a single-process throughput multiplier

| VFD | single-process verdict | evidence |
|---|---|---|
| **sec2** (default) | the baseline: one `pwrite`/chunk | `H5FDsec2.c:754` |
| **subfiling** | ✘ MPI-required (`MPI_THREAD_MULTIPLE`); IOC pool is MPI-fed; write blocks to durability (`MPI_Waitall`) | `H5FDsubfiling.h:360-367`, `H5subfiling_common.h:43` `#error MPI 3 required`, `H5FDioc.c:1418` |
| **mirror** | ✘ TCP replication to a remote host — *slower* | `H5FDmirror.c:1357-1367` |
| **direct (O_DIRECT)** | ✘ cache-bypass tuning, single-threaded; usually *worse* for compressed appends | `H5FDdirect.c:42-47` |
| **core (+ backing store)** | ◇ the only realistic win — RAM-speed staging + one bulk flush; RAM-bounded, flush still serial | `H5FDcore.c:52,126,209` |

All advanced VFDs are **OFF** in the installed build (`libhdf5.settings`: Parallel/Direct/
Mirror/Subfiling/Threadsafety all OFF).

## 5. Filter pipeline — still single-threaded (h5cpp's pool is still necessary)

`H5Z_pipeline` is a serial `for` loop over one chunk buffer (`H5Z.c:1359,1392`), invoked one
chunk at a time from `H5D__chunk_flush_entry` (`H5Dchunk.c:5367`). **Zero** `pthread`/`omp` in
`H5Z.c`/`H5Dchunk.c`. So HDF5 2.1.1 does **not** parallelize compression — h5cpp's
worker-pool + libdeflate + **`H5Dwrite_chunk`** (which bypasses the serial pipeline,
`H5Dpublic.h:1242-1248`) remains the correct and necessary design. Nothing in 2.x supersedes it.

---

## What actually squeezes more throughput in 2.1.1 (single-process, available today)

Ranked by likely impact for compressed chunked append. These are **FAPL/DCPL/DAPL knobs
h5cpp can set** — recoverable throughput with no architecture change:

1. **Right-size the chunk cache + W0 = 1.0** — `H5Pset_chunk_cache(dapl, nslots, nbytes, 1.0)`
   (`H5Pdapl.c:771`). Default is ~1 MB / 521 slots; if a chunk exceeds the cache, every
   partial write triggers a decompress→modify→recompress cycle. For pure sequential append,
   `w0 = 1.0` (full preempt on flush) stops the cache holding finished chunks. **Biggest,
   most common untapped win.** (h5cpp's `pt_t` already opens with a *zero* cache to avoid
   arena bloat — worth revisiting per-workload.)
2. **`H5Dwrite_chunk` + app-side parallel compression** — already h5cpp's path; sidesteps the
   serial H5Z pipeline. Keep it.
3. **Page buffer + paged file-space strategy** —
   `H5Pset_file_space_strategy(fcpl, H5F_FSPACE_STRATEGY_PAGE, false, thr)` **and**
   `H5Pset_page_buffer_size(fapl, N·page, meta%, raw%)` (`H5Pfcpl.c:1215`, `H5Pfapl.c:5754`).
   Batches many small chunk/metadata writes into page-sized I/Os. **Inert without the PAGE
   strategy** (`H5Fint.c:2122`). Largest gains on high-latency / network FS.
4. **Metadata block size** — `H5Pset_meta_block_size(fapl, 1–8 MB)` (default **2 KB**,
   `H5Fprivate.h:349`). Reduces metadata fragmentation as the chunk index grows during append.
5. **`H5D_CHUNK_DONT_FILTER_PARTIAL_CHUNKS`** — `H5Pset_chunk_opts(dcpl, …)` (`H5Pdcpl.c:2615`).
   Skips re-filtering the growing partial edge chunk on each append flush — directly relevant
   to filtered append along the extend dimension.
6. **Alignment** — `H5Pset_alignment(fapl, thr, align)` (default off). Modest single-process;
   helps mainly on parallel FS.
7. **Core VFD + backing store** — RAM-speed staging when the dataset fits in memory.

---

## Synthesis — what this means for h5cpp and the async destination

1. **The experiment series is vindicated by the source.** exp-coordinator measured that no
   userspace coordinator beats the lock because HDF5 I/O is serial — and the 2.1.1 source
   confirms the library is *intrinsically* single-threaded and serial: no internal async, no
   parallel API, no threaded filtering. The single-thread-I/O ceiling is HDF5's, not h5cpp's.

2. **The global lock decision stands.** 2.1.1's own threadsafety is still a global exclusive
   lock; rebuilding against it would only relocate h5cpp's mutex, not parallelize anything.

3. **h5cpp's worker-pool / libdeflate / `H5Dwrite_chunk` design is the right one** and is *not*
   superseded by anything in 2.x. It already extracts the only available parallelism
   (compression overlapped with serialized I/O).

4. **The true-async "final destination" is a deployment, not an h5cpp code change**: rebuild
   HDF5 `--enable-threadsafe` + Argobots + the external Async VOL connector, stack via
   `H5Pset_vol`, use `H5Dwrite_async` + event sets. Worth it only when I/O (not compression)
   is the bottleneck AND the storage backend can actually overlap I/O — on plain local POSIX
   the Async VOL's single background thread still serializes the writes, so the gain over
   h5cpp's current overlap is limited. The bigger I/O-parallelism wins live in **parallel
   HDF5 / subfiling (MPI)**, a different deployment target.

5. **Cheapest real next step:** expose the **tuning knobs** (chunk-cache + W0, page buffer +
   PAGE strategy, meta-block-size, `DONT_FILTER_PARTIAL_CHUNKS`) through h5cpp's FAPL/DCPL/DAPL
   property surface and sweep them on the append workload. That's recoverable throughput with
   zero architectural risk — and the only lever in 2.1.1 that doesn't require an MPI/Argobots
   rebuild.
</content>
