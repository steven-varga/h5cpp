@page example_guide_datasets Datasets

A dataset is the unit HDF5 stores typed multi-dimensional arrays in. The point of this example is simple: everything you'd reach for the HDF5 C API for — `H5Dcreate`, `H5Dwrite`, `H5Dread`, `H5Sselect_hyperslab`, `H5Pset_chunk`, `H5Pset_deflate`, `H5Pset_fill_value`, `H5Dextend` — has a small composable C++ surface in h5cpp.

The whole vocabulary fits on one slide:

```cpp
h5::create<T>(fd, path, ...)        // create with explicit shape and policy
h5::write(fd, path, data, ...)      // one-shot create-or-write
h5::read<T>(fd, path, ...)          // typed read into T
h5::append(pt, value)               // packet-table row appender
```

## Files

| File           | Purpose                                                  |
| -------------- | -------------------------------------------------------- |
| `datasets.cpp` | Ten sections that exercise the full dataset surface (incl. `std::mdspan` if available) |
| `datasets.h5` | Output container (`./datasets-h5dump -pH datasets.h5`)    |

## Includes

```cpp
#include <h5cpp/all>
```

Pure STL. The dataset surface itself is type-agnostic — if you want `arma::mat`, `xt::xarray`, `Eigen::MatrixXd`, or `std::mdspan` (C++23) instead of `std::vector`, the same `h5::write` / `h5::read` calls accept them. See `examples/attributes` for the linalg variants and the note at the bottom of this README about `std::mdspan`.

## Anatomy

Every dataset has four pieces of header information plus the data array. h5cpp exposes them as composable arguments to `h5::create` / `h5::write`:

| HDF5 concept     | h5cpp                                       | Notes                                            |
| ---------------- | ------------------------------------------- | ------------------------------------------------ |
| Name             | the `path` string                           | `/group/subgroup/dataset` — missing groups can be auto-created with `h5::create_path` |
| Datatype         | template parameter `T`                      | Scalar, compound (via `H5CPP_REGISTER_STRUCT`), string, complex, fixed array |
| Dataspace        | `h5::current_dims{...}`, `h5::max_dims{...}` | Use `H5S_UNLIMITED` for extendable axes          |
| Storage layout   | `h5::chunk{...}` + filters                  | Contiguous by default; chunking required for filters or unlimited dimensions |

Hyperslab selection (partial I/O) is the orthogonal vocabulary:

| HDF5 concept | h5cpp           | What it means                                |
| ------------ | --------------- | -------------------------------------------- |
| `start`      | `h5::offset{}`  | First selected cell                          |
| `count`      | `h5::count{}`   | How many `(block, block)` groups            |
| `stride`     | `h5::stride{}`  | Distance between successive group starts    |
| `block`      | `h5::block{}`   | Shape of each group                          |

With no `stride` / `block`, `count` is the simple "size of the selection".

## 1. One-shot create + write

When you don't need explicit control over chunk size or filters, hand a value to `h5::write` and h5cpp picks shape and policy from the value:

```cpp
std::vector<double> v = {1.0, 2.0, 3.0, 4.0, 5.0};
h5::write(fd, "/one_shot/vec", v);
```

Result: contiguous storage, fixed shape `{5}`, no filters.

## 2. Explicit create, then write

When you want chunking, compression, attributes, or unlimited dimensions, create the dataset first, then write into it:

```cpp
h5::ds_t ds = h5::create<double>(
    fd, "/explicit/mat", h5::current_dims{4, 5}, h5::chunk{2, 5} | h5::gzip{6});

ds["units"]    = "meters";      // attributes go on the ds_t
ds["captured"] = "2026-05-27";

std::vector<double> M(4 * 5);
std::iota(M.begin(), M.end(), 0.0);
h5::write(ds, M);
```

`h5::create<T>` returns an `h5::ds_t` — a managed dataset handle. Attributes attach to it; the packet-table view `h5::pt_t` is the same handle from a different angle.

## 3. Reading back — three reader shapes

Same dataset, three reader shapes. `h5::read<T>` dispatches on `T`:

```cpp
auto v = h5::read<std::vector<double>>(fd, "/explicit/mat");   // h5cpp allocates

std::vector<double> buf(20);                                   // raw memory
h5::read<double>(fd, "/explicit/mat", buf.data(), h5::count{4, 5});

std::vector<double> col0(4);                                   // partial read
h5::read<double>(fd, "/explicit/mat", col0.data(),
    h5::offset{0, 0}, h5::count{4, 1});
```

The first form lets h5cpp allocate. The second hands h5cpp a pre-sized buffer + `count` describing the shape on disk. The third uses `offset + count` to read just a sub-region — same hyperslab vocabulary as the write side.

## 4. Chunking + filter chain

Chunking is **required** for compression, fletcher32 checksums, or unlimited dimensions. The filter chain runs per chunk:

| Filter                       | What it does                          |
| ---------------------------- | ------------------------------------- |
| `h5::chunk{r, c}`            | rectangular chunk shape               |
| `h5::gzip{N}`                | DEFLATE level N (1..9)                |
| `h5::shuffle`                | byte-shuffle before compression        |
| `h5::fletcher32`             | per-chunk checksum                    |
| `h5::nbit`                   | strip insignificant bits              |
| `h5::fill_value<T>{v}`       | pre-fill value for uninitialised cells |

```cpp
h5::ds_t ds = h5::create<double>(fd, "/chunked/sine",
    h5::current_dims{100, 100}, h5::chunk{20, 20} | h5::shuffle | h5::gzip{6} | h5::fletcher32);
h5::write(ds, v);
hsize_t storage = H5Dget_storage_size(static_cast<hid_t>(ds));
```

Compression ratio depends on the data. Slowly-varying signals (sine, images, structured records) get 3-10x; high-entropy data (already-compressed payloads, random noise) gets ~1x and the pipeline overhead dominates.

## 5. Fill values

Pre-create with a fill value, then read before writing — every cell shows the fill. Common idioms: `NaN` for floats, sentinel integers for indices.

```cpp
h5::create<double>(fd, "/fill/preset",
    h5::current_dims{3, 4}, h5::chunk{3, 4} | h5::fill_value<double>{std::nan("")});
auto buf = h5::read<std::vector<double>>(fd, "/fill/preset");
// buf is all NaN
```

## 6. Unlimited dimensions + append (packet table)

Set `max_dims` to `H5S_UNLIMITED` on the axis you want to grow. Chunking is mandatory. `h5::pt_t` is the packet-table view of the dataset — it buffers appends and flushes them as chunks.

```cpp
{   // Inner scope so the pt destructor flushes before we read.
    h5::pt_t pt = h5::create<int>(fd, "/stream/values",
        h5::max_dims{H5S_UNLIMITED}, h5::chunk{20} | h5::gzip{4});
    for (int i = 0; i < 100; ++i) h5::append(pt, i * i);
}
auto out = h5::read<std::vector<int>>(fd, "/stream/values");
// out.size() == 100, out.back() == 9801
```

Two gotchas:

- **Flush before read.** The pt buffers writes; reading before the `pt_t` destructor runs returns the dataset as last flushed. Scope the pt explicitly or call its flush.
- **Pick a chunk size that divides your expected count.** Partial trailing chunks may be zero-padded in the current bank.

## 7. Hyperslab selection — offset / count / stride / block

Write a small block into a larger dataset. Background is `0.0` (fill value), patch is `9.0`:

```cpp
h5::ds_t ds = h5::create<double>(fd, "/hyperslab/grid",
    h5::current_dims{6, 8}, h5::chunk{3, 4} | h5::fill_value<double>{0.0});

std::vector<double> patch(2 * 3, 9.0);
h5::write(ds, patch.data(), h5::offset{1, 1}, h5::count{2, 3});
```

Result:

```text
0  0  0  0  0  0  0  0
0  9  9  9  0  0  0  0    ← row 1, cols 1..3
0  9  9  9  0  0  0  0    ← row 2, cols 1..3
0  0  0  0  0  0  0  0
0  0  0  0  0  0  0  0
0  0  0  0  0  0  0  0
```

The raw-buffer form (`patch.data()` + explicit `h5::count`) is the unambiguous way to write a sub-region: source layout is row-major, destination layout is row-major. Writing from `arma::mat` (column-major) also works, but the round-trip through `h5::read<arma::mat>` will appear transposed unless you compensate.

## 8. Partial read

The same hyperslab vocabulary on the read side. Request a window of shape `count` starting at `offset`:

```cpp
std::vector<double> sub(3 * 4);
h5::read<double>(fd, "/hyperslab/grid", sub.data(),
    h5::offset{0, 0}, h5::count{3, 4});
```

Reads the upper-left 3×4 corner — the patch from section 7 is visible at its (1,1) corner.

## 9. Reusable property lists

Property-list fragments compose with `|`. The result is a real `dcpl_t` / `lcpl_t` you can store, reuse, and pass to many `h5::create` calls:

```cpp
h5::dcpl_t fast_chunked = h5::chunk{64, 64} | h5::shuffle | h5::gzip{6};
h5::lcpl_t deep_path    = h5::create_path | h5::utf8;

for (int i = 0; i < 3; ++i) {
    std::string path = "/group/depth/" + std::to_string(i) + "/data";
    h5::create<float>(fd, path,
        h5::current_dims{128, 128}, deep_path, fast_chunked);
}
```
`h5::create_path` auto-creates missing intermediate groups. `h5::utf8` marks link names as UTF-8.

## Build Notes

Wired into CMake as `examples-datasets`. Pure STL — no linalg dependency. Running the binary writes `datasets.h5` in the current directory:

```sh
cd <build-dir>
./examples-datasets
h5dump -pH datasets.h5
```

## 10. `std::mdspan` (C++23)

`std::mdspan<T, Extents, LayoutPolicy, AccessorPolicy>` (P0009, `<mdspan>` since C++23) is a non-owning multi-dimensional view over a contiguous buffer. Structurally it's exactly what h5cpp passes around internally: pointer + extents.

Wired in `h5cpp/H5Mmdspan.hpp`, gated on the `__cpp_lib_mdspan` feature-test macro. The mapper provides:

- `access_traits_t<std::mdspan<...>>` with `kind = contiguous`
- `storage_representation_impl` resolving to `linear_value_dataset`
- `impl::data`, `impl::size`, `impl::rank` for the legacy raw paths

mdspan is **non-owning**, so the read path always uses a caller-owned buffer:

```cpp
constexpr std::size_t rows = 3, cols = 4;

// Source view over an owned buffer.
std::vector<double> storage(rows * cols);
std::iota(storage.begin(), storage.end(), 100.0);
std::mdspan<double, std::dextents<std::size_t, 2>>
    view(storage.data(), rows, cols);

// Write the view directly — shape comes from extents, data from .data_handle().
h5::write(fd, "/mdspan/view", view);

// Read back into a fresh buffer + view.
std::vector<double> back_buf(rows * cols);
std::mdspan<double, std::dextents<std::size_t, 2>>
    back(back_buf.data(), rows, cols);
h5::read<double>(fd, "/mdspan/view", back.data_handle(),
    h5::count{rows, cols});
```

### Availability

The mapper is a no-op if the standard library doesn't ship `<mdspan>`. `H5CPP_HAS_MDSPAN` is defined only when `__cpp_lib_mdspan >= 202207L` is. Section 10 of `datasets.cpp` reflects that — when mdspan isn't available, it prints `skipped: this TU was not built with __cpp_lib_mdspan` instead of failing the build.

| Toolchain                      | Ships `<mdspan>` | Section 10 runs |
| ------------------------------ | ---------------- | --------------- |
| libstdc++ 15+                  | yes              | yes             |
| libstdc++ ≤ 14                 | no               | skipped         |
| libc++ 17+                     | yes              | yes             |
| libc++ ≤ 16                    | no               | skipped         |

The `examples-datasets` target is built at C++23 (`target_compile_features(... cxx_std_23)`) so the gate trips automatically when the toolchain catches up — no CMake-level toggle needed.

### Caveats

- `h5::read<std::mdspan<...>>(fd, path)` (allocating return form) is not supported — mdspan is non-owning. Use the buffer-out overload with `view.data_handle()` as shown above.
- The mapper supports static (`std::extents<std::size_t, N, M>`), dynamic (`std::dextents<std::size_t, R>`), and mixed extents. Layout policy is `layout_right` (row-major) by default, which matches HDF5's on-disk layout. `layout_left` (column-major) round-trips correctly but the on-disk shape will appear transposed in `h5dump`.
- Custom `AccessorPolicy` is accepted but only the default accessor is exercised in the example.

## Mental Model

```text
type T  +  path  +  dataspace  +  policy   →   managed dataset
                                              (h5::ds_t)
                                                  │
       value  ──── h5::write(ds, value, ...)   ───┤
                                                  │
                  hyperslab args                  │
                  (offset/count/stride/block)     │
                                                  ▼
                                          chunked / filtered
                                          / unlimited disk layout
```

The pieces are orthogonal. Type and path are mandatory. Dataspace is the shape. Policy is the property-list bundle. Hyperslab args are the per-call selection. You compose only what you need; defaults cover the rest.

## Source

- [`datasets.cpp`](datasets_8cpp-example.html) — rendered with syntax highlighting
