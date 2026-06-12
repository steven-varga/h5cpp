@page example_guide_custom_pipeline Custom Pipelines

How a chunk flows from `h5::write` to disk is a **policy chosen at the write site** — and h5cpp picks one of three paths automatically from the call's arguments and the dataset's layout. This example walks all three on the same dataset, timed and verified.

A pipeline is the layer between `h5::write` / `h5::read` and `H5Dwrite_chunk` / `H5Dread_chunk`: it owns the per-chunk scratch buffers and runs the filter chain (`gzip`, `shuffle`, `fletcher32`, …), either on the calling thread or fanned out across a worker pool.

## Files

| File           | Purpose                                                            |
| -------------- | ------------------------------------------------------------------ |
| `pipeline.cpp` | Three sections, one per write path; same dataset, timed + verified |

## The three write paths

| # | Path | Engaged when | Mechanism |
| - | ---- | ------------ | --------- |
| 1 | **direct chunk** *(default)* | a plain, no-hyperslab chunked write | `basic_pipeline_t` → `H5Dwrite_chunk`, h5cpp's filter chain on the calling thread |
| 2 | **CAPI hyperslab** | `h5::offset`/`stride`/`block` present (compile-time), a contiguous dataset, or an HDF5-applied filter (`nbit`/`scaleoffset`) | stock `H5Dwrite` — HDF5's own chunk processor + filter pipeline; the most flexible, usually the slowest |
| 3 | **parallel** | `h5::threads{N}` on the dataset's DAPL | `pool_pipeline_t` — the gzip stage fans out across the process-global worker pool |

Selection between #1 and #2 is **compile-time** (the presence of a hyperslab tag); #3 is a per-dataset **DAPL** opt-in. A hyperslab write can never reach the pool — the exclusion is structural.

> `h5::high_throughput` (the old per-dataset opt-in) is **deprecated** — direct-chunk is the default now, so the tag is a no-op kept only for source compatibility, removed in v2.x.y.

## Test data

```cpp
constexpr std::size_t k_rows  = 1024;
constexpr std::size_t k_cols  = 2048;   // 1024 × 2048 × 8B ≈ 16 MiB
constexpr std::size_t k_chunk = 64;     // 64 × 2048 doubles per chunk

std::vector<double> data = h5::normal<double>{0.0, 1.0} | h5::take(k_rows * k_cols);
```

Gaussian noise compresses poorly (~1:1 with gzip), so these timings mostly compare pipeline overhead, not filter throughput.

## 1. Direct chunk — the default

No flag. A no-hyperslab chunked write goes straight through h5cpp's pipeline.

```cpp
auto fd = h5::create("pipeline_default.h5", H5F_ACC_TRUNC);
h5::write(fd, "dataset", data,
    h5::current_dims{k_rows, k_cols},
    h5::chunk{k_chunk, k_cols} | h5::gzip{4});
```

(A 1-D container written to an N-D chunked dataset is tiled by the dataset's actual dimensions — multidimensional writes round-trip correctly.)

## 2. CAPI hyperslab — HDF5's own pipeline

A hyperslab selection (here `h5::offset{0,0}`) is a compile-time signal that routes the write through stock `H5Dwrite`, letting HDF5 run its own chunk processor and filters. The same path is taken for contiguous datasets and for `nbit`/`scaleoffset` filters (which h5cpp leaves to the C library).

```cpp
auto ds = h5::create<double>(fd, "dataset",
    h5::current_dims{k_rows, k_cols},
    h5::chunk{k_chunk, k_cols} | h5::gzip{4});
h5::write(ds, data.data(), h5::count{k_rows, k_cols}, h5::offset{0, 0});
```

## 3. `h5::threads{N}` — parallel pool pipeline

`h5::threads{N}` is a per-dataset **DAPL** property (it survives the `H5Dget_access_plist` round-trip, so it is read back at the write site directly — no registry). It fans the gzip stage out across one **process-global** `worker_pool_t`; `H5Dwrite_chunk` stays on the caller thread, so only compression is parallel.

```cpp
h5::dapl_t dapl = h5::threads{hw} | h5::backpressure{32};

auto fd = h5::create("pipeline_threads.h5", H5F_ACC_TRUNC);
h5::write(fd, "dataset", data,
    h5::current_dims{k_rows, k_cols},
    h5::chunk{k_chunk, k_cols} | h5::gzip{4}, dapl);
```

Notes:

- `h5::threads{}` with no argument fans out across `std::thread::hardware_concurrency()`.
- `h5::backpressure{M}` caps in-flight chunks at `M`; without `h5::threads{N}` it is a silent no-op (no pool, no queue to bound).
- One pool backs every dataset — under the single-producer model (HDF5 is driven from one thread) datasets are written sequentially and never contend, so a per-file pool would only oversubscribe.

## Build & run

Wired into CMake as `examples-custom-pipeline`, linked against `Threads::Threads`. Running it writes three `.h5` files in the current directory:

```sh
cd <build-dir>
./examples-custom-pipeline
ls pipeline_*.h5
```

## Mental model

```text
h5::write
   │   hyperslab tag?  ── yes ─→ [2] CAPI hyperslab  → H5Dwrite          (HDF5's pipeline)
   │        │ no
   │   chunked + supported filters?
   │        │ yes
   │   h5::threads{N}? ── yes ─→ [3] pool_pipeline_t → H5Dwrite_chunk    (parallel filters)
   │        │ no
   │        └─────────→ [1] basic_pipeline_t → H5Dwrite_chunk            (default direct chunk)
   ▼
disk
```

## Source

- [`pipeline.cpp`](pipeline_8cpp-example.html) — rendered with syntax highlighting
