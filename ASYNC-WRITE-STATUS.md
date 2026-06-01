# #287 async write — status

Branch `287-refactor-fapl-pipeline-context` on top of staging `f7847201`.

Build/verify (HDF5 1.12.3):
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DH5CPP_BUILD_TESTS=ON -DBUILD_TESTING=ON \
      -DHDF5_DIR=/usr/local/HDF_Group/HDF5/1.12.3/cmake
cmake --build build -j4 && (cd build && ctest)          # 62/62
# TSan (clang-20): cmake -S . -B build-tsan -G Ninja -DCMAKE_C/CXX_COMPILER=clang(-20)
#   -DCMAKE_C/CXX_FLAGS="-fsanitize=thread …" … then build-tsan/test-h5collector  → clean
```

## Done & working (TSan-clean)
- **`io_collector_t`** (`h5cpp/H5collector.hpp`) — single per-file HDF5 thread.
  `submit_and_wait` (metadata, reentrancy-inline) + `io_enqueue` (streaming, built
  but dormant) + cached `fileno()`. Mutex+condvar.
- **Registry** — `attach_async` returns the shared collector (one per *physical*
  file); `close_async` routes closes through the carried collector, detaching by
  the collector's cached fileno (no off-thread HDF5).
- **FAT HANDLE** (your call): `async::fd_t` carries `shared_ptr<io_collector_t>`,
  sourced once at open. Write/close resolve it with a **member read** — no
  `H5Fget_fileno` off-thread. `operator ::hid_t()` stays deleted (CAPI use-site
  compat preserved via `.handle`); binary-layout compat was already gone.
- **Async write** — `h5::write(async_fd, …)` runs the sync gateway on the collector
  thread (parallel compression + serialized I/O). Pass `h5::high_throughput` to
  engage the pool.
- **CONCURRENT writers to one file: TSan-clean** (`test/H5collector.cpp`) — M
  threads, distinct datasets, no race, all round-trip.
- Profiling: `examples-async-pipeline` — gzip-6 ~355 MB/s (pool engaged) vs ~56
  (stock), no-filter ~1.9 GB/s on /tmp.

## Documented constraint (real, with a follow-up)
Concurrent producers must **not construct/destruct HDF5 property lists per call**:
`h5::chunk{..} | h5::gzip{..}` builds a DCPL via `H5Pcreate`/`H5Pset` and frees it
via `H5Pclose` *in the caller's expression* on the producer thread — which races
(the write can't move those calls onto the collector; they bracket it). Fix:
**build the plist once and pass by reference** (the concurrent test does this).
Lifting it (defer arg-plist lifecycle onto the collector) is a follow-up.

## Deferred (not blockers)
- Streaming `io_enqueue` chunk path (finer interleave across writers) — built, dormant.
- Delete `H5executor.hpp` — kept as dead code (only `H5Pfapl_async` builds it).
- Async write returns `void` (creates+writes+closes the ds on the collector); a
  later slice returns an async `ds_t` + propagates the collector to it.
- Read path through the collector.
