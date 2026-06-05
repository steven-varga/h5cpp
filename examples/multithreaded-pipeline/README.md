# Multithreaded Pipeline (#287)

Two examples around the h5cpp **worker-pool pipeline**, both reporting throughput:
`pipeline-write` writes a **block of random floats** to a chunked, gzip dataset, and
`pipeline-read` reads it back through the **parallel reader**.

## What it shows

A plain chunked write already uses h5cpp's **direct-chunk** pipeline by default;
one per-dataset **DAPL** tag turns on the parallel stage:

```cpp
auto fd = h5::create("out.h5", H5F_ACC_TRUNC);

h5::write(fd, "data", data,                                // a std::vector<float>
    h5::current_dims{n},
    h5::chunk{C} | h5::gzip{6},                            // chunked + deflate
    h5::threads{N});                                       // per-dataset DAPL → pool_pipeline_t
```

- A no-hyperslab chunked write goes through `basic_pipeline_t` → `H5Dwrite_chunk`
  (h5cpp's filter chain) **by default** — no opt-in.
- **`h5::threads{N}`** (a per-dataset **DAPL** property; `h5::threads{}` = hardware_concurrency)
  fans the gzip stage out across the **process-global worker pool** (`pool_pipeline_t`). A
  DAPL property survives the `H5Dget_access_plist` round-trip, so it's read back at the write
  site directly — no fileno registry. `h5::backpressure{M}` bounds in-flight chunks.
- `H5Dwrite_chunk` stays on the caller thread; only compression is parallel.
- **Read** (`pipeline-read`): `h5::open(fd, "data", h5::threads{N})` puts the same DAPL tag on
  the read handle, so `h5::read` routes through `pool_pipeline_t::read` — `H5Dread_chunk` on the
  caller, gzip **inflate** fanned across the pool. (`H5Dread_chunk` also bypasses HDF5's chunk
  cache, so a direct-chunk write + read on one handle does not leak it.) The read parallelises
  too — inflate is cheaper than deflate, so it saturates at fewer workers / needs more chunks,
  but it scales (479 → 3.2 GB/s at 16 workers with 256 chunks). See [`profile-report.md`](profile-report.md).

Building with **`-DH5CPP_MULTITHREAD`** additionally engages one process-global recursive
mutex around every HDF5 C-API call (HDF5-threadsafe-style — a lock, not a thread), making
concurrent writers to one file safe. Compression never touches HDF5, so the lock is
throughput-neutral on that path; it is a no-op in a classic build.

## Run

```sh
cmake --build <build> --target examples-multithreaded-pipeline-write examples-multithreaded-pipeline-read
./examples-multithreaded-pipeline-write     # writes multithreaded-pipeline.h5
./examples-multithreaded-pipeline-read      # reads it back (run write first)
```

Each prints `done: <s>  <MiB/s>  <Mfloat/s>`. `H5CPP_BENCH_THREADS` sizes the worker pool
(write: compression fan-out; read: inflate fan-out).

Env knobs (all optional): write `H5CPP_BENCH_{N, THREADS, CHUNK, GZIP}`; read `H5CPP_BENCH_THREADS`.

Links `Threads::Threads`.

---
*See [`profile-report.md`](profile-report.md) for the full throughput profile and the
HDF5-2.1.1 / global-mutex / `exp-vyukov` analysis, refreshed against this example.*
