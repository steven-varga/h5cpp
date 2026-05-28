@page example_guide_custom_pipeline Custom Pipelines

This example walks the three pipeline opt-in surfaces h5cpp ships today. The point is simple: how chunks flow from `h5::write` to disk is a policy choice — single-threaded, per-dataset scratch, or worker-pool — and each policy is one property-list flag away.

A pipeline is the layer between `h5::write` / `h5::read` and `H5Dwrite_chunk` / `H5Dread_chunk`: it owns the per-chunk scratch buffers, runs the filter chain (`gzip`, `shuffle`, `fletcher32`, …), and decides whether the filter work runs on the calling thread or on a pool.

## Files

| File           | Purpose                                                            |
| -------------- | ------------------------------------------------------------------ |
| `pipeline.cpp` | Three sections, one per pipeline mode; same dataset, timed + verified |

## The Three Pipelines

| Pipeline                  | Opt-in                                       | What it does                                                  |
| ------------------------- | -------------------------------------------- | ------------------------------------------------------------- |
| `basic_pipeline_t`        | default — no flag                            | Single-threaded chunk tiling + filter chain                   |
| `basic_pipeline_t` w/ scratch | `h5::high_throughput` on the DAPL        | Per-dataset opt-in; keeps the pipeline scratch buffer alive across writes (fix for issue #242) |
| `pool_pipeline_t`         | `h5::threads{N}` (+ `h5::backpressure{M}`) on the FAPL | Worker pool shared across all datasets in the file; filter work parallelises across N workers |

Three other pipelines are declared but **not yet implemented** — `threaded_pipeline_t`, `romio_pipeline_t`, `hadoop_pipeline_t`. Those are stubs in `H5Zpipeline.hpp` (see architecture notes line 133-135).

## Includes

```cpp
#include <h5cpp/all>
```

Everything in this example is already in `<h5cpp/all>`. No `generated.h`.

## Test Data

```cpp
constexpr std::size_t k_rows  = 1024;
constexpr std::size_t k_cols  = 2048;   // 1024 × 2048 × 8B ≈ 16 MiB
constexpr std::size_t k_chunk = 64;     // 64 × 2048 doubles per chunk

std::vector<double> data = h5::normal<double>{0.0, 1.0} | h5::take(k_rows * k_cols);
```

`h5::normal<T>{mean, stddev} | h5::take(n)` is the same generator pipe used in the container example. Gaussian noise compresses poorly (~1:1 with gzip), so the timings here mostly compare pipeline overhead. For real filter throughput tests, switch the generator — `h5::uniform<int>{0, 15}` cast to `double` gives gzip something to chew on (mostly-zero high bytes).

## 1. Default Pipeline

No flag, no fuss. Chunk tiling and gzip happen on the calling thread.

```cpp
auto fd = h5::create("pipeline_default.h5", H5F_ACC_TRUNC);
h5::write(fd, "dataset", data,
    h5::current_dims{k_rows, k_cols},
    h5::chunk{k_chunk, k_cols} | h5::gzip{4});

auto back = h5::read<std::vector<double>>("pipeline_default.h5", "dataset");
```

This is the baseline.

## 2. `h5::high_throughput` — Per-Dataset DAPL

The flag attaches a `basic_pipeline_t` to the dataset's DAPL. The pipeline keeps its scratch buffer alive across writes instead of reallocating it on every `H5Dwrite`. Issue #242's copy callback ensures the scratch buffer is freshly allocated whenever HDF5 internally copies the DAPL — no double-free.

```cpp
auto fd = h5::create("pipeline_high_throughput.h5", H5F_ACC_TRUNC);

// Pre-create the dataset with the DAPL so the high_throughput property
// is attached at construction time, then write into it.
auto ds = h5::create<double>(fd, "dataset",
    h5::current_dims{k_rows, k_cols},
    h5::chunk{k_chunk, k_cols} | h5::gzip{4},
    h5::high_throughput);
h5::write(ds, data.data(), h5::count{k_rows, k_cols});

// Re-apply the same flag on read so H5Pexist sees the property.
auto back = h5::read<std::vector<double>>(
    "pipeline_high_throughput.h5", "dataset", h5::high_throughput);
```

Important: `h5::high_throughput` is a flag (`flag::high_throughput`), not an `h5::dapl_t`. Passing it as a vararg to `h5::write(fd, path, data, …)` *does not* attach it to the dataset — `arg::get<dapl_t>` cannot pick it up by type. The pattern shown above (`h5::create<T>(fd, path, …, h5::high_throughput)`) is the working route.

## 3. `h5::threads{N}` — Pool Pipeline (FAPL)

The FAPL owns a `worker_pool_t` shared by every dataset in the file. When you write a chunked dataset, h5cpp instantiates a `pool_pipeline_t` against that pool and dispatches per-chunk filter work to it. `h5::backpressure{M}` caps in-flight chunks at `M` so memory doesn't blow up on streaming workloads.

```cpp
const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
h5::fapl_t fapl   = h5::threads{hw} | h5::backpressure{32};

auto fd = h5::create("pipeline_threads.h5", H5F_ACC_TRUNC,
                     h5::default_fcpl, fapl);
h5::write(fd, "dataset", data,
    h5::current_dims{k_rows, k_cols},
    h5::chunk{k_chunk, k_cols} | h5::gzip{4});

// Re-apply the same FAPL on read so the pool is available.
auto fd_r = h5::open("pipeline_threads.h5", H5F_ACC_RDONLY, fapl);
auto back = h5::read<std::vector<double>>(fd_r, "dataset");
```

Notes:

- `h5::threads{}` with no argument uses `std::thread::hardware_concurrency()`.
- `h5::backpressure{N}` without `h5::threads{N}` is a silent no-op — without a pool, there's no queue to bound.
- The pool's lifetime is tied to the FAPL's shared-ownership refcount. When the last FAPL copy drops, the pool joins its workers cleanly.

## Sample Output

16-core machine, 16 MiB of Gaussian noise, gzip{4}:

```text
1. default (basic_pipeline_t, single-threaded)
----------------------------------------------
  write+read:   562.1 ms   roundtrip ok: yes

2. h5::high_throughput (DAPL, basic_pipeline_t with per-DAPL scratch)
---------------------------------------------------------------------
  write+read:   368.9 ms   roundtrip ok: yes

3. h5::threads + h5::backpressure (FAPL, pool_pipeline_t)
---------------------------------------------------------
  hardware_concurrency() = 16
  write+read:   553.1 ms   roundtrip ok: yes
```

Takeaways for *this* shape:

- `high_throughput` wins by ~30% — scratch buffer reuse pays off when the filter chain runs many times.
- The pool pipeline shows parity with default. At 16 chunks of mostly-incompressible data, the thread coordination cost cancels the parallelism gain. **The pool wins on workloads with many heavy chunks** (large datasets, expensive compressors, dictionary-coded data).
- Timings are noisy and machine-dependent. Treat them as ordering hints, not absolutes.

## Composability

`h5::high_throughput` (DAPL, per-dataset) and `h5::threads{N}` (FAPL, per-file) sit on orthogonal axes. They can be combined on the same dataset:

```cpp
h5::fapl_t fapl    = h5::threads{8} | h5::backpressure{32};
h5::dapl_t dapl_ht = h5::high_throughput;

auto fd = h5::create("combined.h5", H5F_ACC_TRUNC, h5::default_fcpl, fapl);
auto ds = h5::create<double>(fd, "dataset",
    h5::current_dims{N},
    h5::chunk{C} | h5::gzip{4},
    dapl_ht);
```

## Build Notes

Wired into CMake as `examples-custom-pipeline`, linked against `Threads::Threads`. Running the binary writes three `.h5` files in the current directory:

```sh
cd <build-dir>
./examples-custom-pipeline
ls pipeline_*.h5
```

## Mental Model

```text
h5::write / h5::read
        │
        ▼
[pipeline]  ──── default      ──→ basic_pipeline_t (calling thread)
            ──── high_throughput  ─→ basic_pipeline_t + per-DAPL scratch
            ──── h5::threads{N}   ─→ pool_pipeline_t (worker pool)
        │
        ▼
H5Dwrite_chunk / H5Dread_chunk
```

User code chooses the pipeline once via a property-list flag. The dispatch and filter chain stay the same. The differences are *where* the work runs and *what* gets reused across calls.

## Source

- [`pipeline.cpp`](pipeline_8cpp-example.html) — rendered with syntax highlighting
