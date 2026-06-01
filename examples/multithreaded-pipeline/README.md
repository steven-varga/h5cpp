# Multithreaded Pipeline (#287)

A throughput harness comparing how the gzip filter chain is scheduled, and a
direct profile of the per-file **async collector** (`io_collector_t`) write path
against the sync pipeline.  See `profile-report.md` for numbers.

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

## Async (single per-file HDF5 thread)

`h5::async::create` stands up one `io_collector_t` thread per file — the sole
thread that calls into HDF5 for that file.  Compression still parallelizes across
the worker pool; chunk I/O and metadata run on the collector.  This makes
**concurrent writers to one file safe** (Threadsafety-OFF HDF5 never sees two
threads at once):

```cpp
h5::async::fd_t fd = h5::async::create("data.h5", H5F_ACC_TRUNC, h5::default_fcpl, fapl);
h5::write(fd, "dataset", data,
    h5::current_dims{rows}, h5::chunk{chunk} | h5::gzip{6}, h5::high_throughput);
// fd closes on the collector thread at scope exit
```

Note: for *concurrent* writers, share a pre-built DCPL rather than constructing
`h5::chunk|h5::gzip` per call (per-call `H5Pcreate`/`H5Pclose` runs on the producer
thread and races); see `test/H5collector.cpp`.

## Cases (`pipeline.cpp`)

| Case | What it measures |
| ---- | ---------------- |
| `raw` | `H5Dwrite_chunk`, no filter — the chunk-I/O ceiling |
| `direct-gzip` | hand-rolled libdeflate compress + `H5Dwrite_chunk` (tightest single-thread gzip) |
| `hdf5-gzip` | stock HDF5 deflate filter (no h5cpp pool) |
| `single` | h5cpp `high_throughput`, 1 worker |
| `multi` | h5cpp `high_throughput`, N workers (sync `pool_pipeline_t`) |
| `async` | `h5::async::create` + the collector (N workers) |

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
