# Multithreaded Pipeline (#287)

A throughput harness comparing how the gzip filter chain is scheduled, and a
profile of the **`-DH5CPP_MULTITHREAD`** build mode (global HDF5 mutex) write
path against the default build.  See `profile-report.md` for numbers.

## Policy surface

The worker pool is a **file-level** policy (FAPL); the parallel pipeline is a
**per-dataset** opt-in (DAPL):

```cpp
// FAPL — "is there a pool, how many workers, how deep the in-flight window"
h5::fapl_t fapl = h5::threads{hw} | h5::backpressure{32};

auto fd = h5::create("data.h5", H5F_ACC_TRUNC, h5::default_fcpl, fapl);

// h5::high_throughput (DAPL) is what actually engages pool_pipeline_t — without
// it the write falls back to stock single-threaded HDF5 filters.
h5::write(fd, "dataset", data,
    h5::current_dims{rows}, h5::chunk{chunk} | h5::gzip{6}, h5::high_throughput);
```

`h5::threads{N}` creates the shared worker pool (resolved per-file via the #286
fileno registry, since `H5Fget_access_plist` strips it off the file id);
`h5::backpressure{M}` bounds in-flight chunks.  Chunk I/O stays on the caller
thread; gzip/zstd fan out across the worker pool.

## `-DH5CPP_MULTITHREAD` (global HDF5 mutex)

Building with `-DH5CPP_MULTITHREAD` engages one process-global recursive mutex
that serializes every HDF5 C-API call (the same design as HDF5's own
`--enable-threadsafe` — a lock, not a dedicated thread).  Compression still
parallelizes across the worker pool and never touches HDF5, so the lock is
throughput-neutral on that path.  The lock is a no-op / zero-cost in a classic
build.  This makes **concurrent writers to one file safe** (a Threadsafety-OFF
HDF5 never sees two threads inside the C library at once):

```cpp
auto fd = h5::create("data.h5", H5F_ACC_TRUNC, h5::default_fcpl, fapl);
h5::write(fd, "dataset", data,
    h5::current_dims{rows}, h5::chunk{chunk} | h5::gzip{6}, h5::high_throughput);
```

Note: for *concurrent* writers, share a pre-built DCPL rather than constructing
`h5::chunk|h5::gzip` per call (per-call `H5Pcreate`/`H5Pclose` runs on the producer
thread; the global mutex serializes them but a shared DCPL avoids the churn).

## Cases (`pipeline.cpp`)

| Case | What it measures |
| ---- | ---------------- |
| `raw` | `H5Dwrite_chunk`, no filter — the chunk-I/O ceiling |
| `direct-gzip` | hand-rolled libdeflate compress + `H5Dwrite_chunk` (tightest single-thread gzip) |
| `hdf5-gzip` | stock HDF5 deflate filter (no h5cpp pool) |
| `single` | h5cpp `high_throughput`, 1 worker |
| `multi` | h5cpp `high_throughput`, N workers (sync `pool_pipeline_t`) |
| `async` | the `multi` pipeline compiled with `-DH5CPP_MULTITHREAD` (global lock engaged, N workers) |

`H5CPP_BENCH_CASE=H5CPP_BENCH_CASE_<NAME>` builds a single-case executable for
clean `perf` profiling (`examples-multithreaded-pipeline-<name>`); the default
target runs all cases and prints the comparison table.

Env knobs: `H5CPP_BENCH_{GZIP_LEVEL,THREADS,CHUNK,N,FILTERS,DIR}`.

## Build

```sh
cmake --build <build> --target examples-multithreaded-pipeline   # all-cases table
# per-case: examples-multithreaded-pipeline-{raw,direct-gzip,hdf5-gzip,single,multi,async}
```
Links `Threads::Threads`; the direct-gzip baseline uses the vendored libdeflate.
