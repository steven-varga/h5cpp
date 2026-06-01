@page curated_io_api_dataset DATASET

@brief Templated dataset I/O — create, open, read, write, append (packet
table), and the sparse-matrix CSC group form.

# Dataset operations

The dataset API is the bulk of H5CPP's surface. All operations take an
element type `T` from the
@ref link_base_template_types "Supported Types" matrix and return /
populate that type directly — no manual buffer management, no shape
bookkeeping when the destination is a value or container.

Datasets live below an open `h5::fd_t` and are addressed by POSIX-style
path. Lifetime is RAII via `h5::ds_t`. Optional `h5::offset` /
`h5::stride` / `h5::count` / `h5::block` arguments select a hyperslab
for partial I/O; omitting them touches the whole extent.

## At a glance

| Operation                  | Function                                                                | Returns        |
| :------------------------- | :---------------------------------------------------------------------- | :------------- |
| Allocate                   | `h5::create<T>(fd, path, args...)`                                      | `h5::ds_t`     |
| Open existing              | `h5::open(fd, path, dapl?)`                                             | `h5::ds_t`     |
| Read — return by value     | `auto v = h5::read<T>(ds, args...)`                                     | `T`            |
| Read — into reference      | `h5::read(ds, T& ref, args...)`                                         | `void`         |
| Read — into raw pointer    | `h5::read(ds, T* ptr, h5::count{...}, args...)`                         | `void`         |
| **Streaming read (C++20 ranges view)** | `for (auto v : h5::view<T>(ds)) …`                          | `std::ranges::input_range` |
| Write a value              | `h5::write(ds, value, args...)`                                         | `h5::ds_t`     |
| Write + create on demand   | `h5::write(fd, path, value, args...)`                                   | `h5::ds_t`     |
| Append (packet table)      | `h5::append(pt, value)` + `h5::flush(pt)`                               | `void`         |
| Write sparse (CSC)         | `h5::write(parent, path, sparse_src)`                                   | `h5::gr_t`     |
| Read sparse                | `auto m = h5::read<arma::SpMat<T>>(parent, path)`                       | `T` (sparse)   |

Every overload follows the same `[fd, path]` / `[ds]` parent dispatch
matrix — pick the one that matches what you already have open.

---

@anchor curated_io_api_dataset_create
## `h5::create` — allocate a new dataset

```cpp
template<class T, class... args_t>
h5::ds_t h5::create(const h5::fd_t& fd, const std::string& dataset_path,
                    args_t&&... args);

// Convenience — opens the file (H5F_ACC_RDWR), creates dataset, returns.
template<class T, class... args_t>
h5::ds_t h5::create(const std::string& file_path,
                    const std::string& dataset_path, args_t&&... args);
```

Allocates a new HDF5 dataset of element type `T`. Shape comes from
`h5::current_dims`; extendable dimensions from `h5::max_dims`; chunking,
compression, and filter pipelines from `h5::chunk{...}` / `h5::gzip{N}` /
`h5::fletcher32` / `h5::shuffle` etc. (see
@ref link_property_lists "Property Lists").

**Parameters**

| Name               | Type                  | Description                                                                                        |
| :----------------- | :-------------------- | :------------------------------------------------------------------------------------------------- |
| `fd`               | `const h5::fd_t&`     | Open file descriptor.                                                                              |
| `dataset_path`     | `const std::string&`  | POSIX-style path inside the file; intermediate groups are created when `h5::default_lcpl` is used. |
| `args...`          | variadic              | Optional: `h5::current_dims`, `h5::max_dims`, `h5::chunk`, filter pipeline, `h5::dcpl_t`, `h5::lcpl_t`, `h5::dapl_t`, explicit `h5::dt_t<T>`. |

**Returns** — `h5::ds_t` RAII handle.

**Throws** — `h5::error::io::dataset::create` on `H5Dcreate2` failure
(path conflict, invalid type, insufficient permissions).

**Example**

```cpp
// Simple contiguous dataset.
h5::ds_t ds = h5::create<double>(fd, "/grid/values", h5::current_dims{1024, 1024});

// Extendable, chunked, gzip-compressed.
h5::ds_t logs = h5::create<int>(fd, "/stream/samples",
    h5::current_dims{0},
    h5::max_dims{H5S_UNLIMITED},
    h5::chunk{4096} | h5::gzip{9} | h5::fletcher32);

// One-shot convenience: file + dataset in a single call.
h5::ds_t out = h5::create<float>("output.h5", "/result", h5::current_dims{N});
```

@see @ref curated_io_api_dataset_write, @ref curated_io_api_dataset_open

---

@anchor curated_io_api_dataset_open
## `h5::open` — open an existing dataset

```cpp
h5::ds_t h5::open(const h5::fd_t& fd, const std::string& path,
                  const h5::dapl_t& dapl = h5::default_dapl);
```

If the dataset's access property list carries the H5CPP high-throughput
pipeline tag, the pipeline's per-chunk cache is initialised here from
the dataset's element size — subsequent reads / writes pick up the
pre-warmed filter chain transparently.

**Throws** — `h5::error::io::dataset::open` (not present, no read
permission, invalid DAPL).

**Example**

```cpp
h5::fd_t fd = h5::open("data.h5", H5F_ACC_RDONLY);
h5::ds_t ds = h5::open(fd, "/grid/values");
auto v = h5::read<arma::Mat<double>>(ds);
```

---

@anchor curated_io_api_dataset_read
## `h5::read` — read into a value, container, or buffer

Three flavours, chosen by what you have on hand:

```cpp
// (1) Return-by-value — most convenient
template<class T, class... args_t>
T h5::read(const h5::ds_t& ds, args_t&&... args);

// (2) Into a pre-allocated container or value
template<class T, class... args_t>
void h5::read(const h5::ds_t& ds, T& ref, args_t&&... args);

// (3) Into a raw pointer — caller owns the memory; requires h5::count
template<class T, class... args_t>
void h5::read(const h5::ds_t& ds, T* ptr, args_t&&... args);
```

Each form has an `(fd, path, ...)` and `(file_path, dataset_path, ...)`
convenience overload that opens the dataset (or file + dataset) and
forwards. Nine overloads in total — pick whichever matches your call
context.

| Form                       | Element count derived from… | Use when                                              |
| :------------------------- | :-------------------------- | :---------------------------------------------------- |
| Return-by-value            | dataset's on-disk shape     | One-shot read; you want the right `T` instance back   |
| By-reference (`T&`)        | `ref`'s container size      | You already have a target object (avoids allocation)  |
| Raw pointer (`T*`)         | `h5::count{...}` (required) | Interop with C buffers, scatter/gather pipelines      |

`T` follows @ref link_base_template_types "Supported Types".
Optional `h5::offset` / `h5::stride` / `h5::block` arguments select a
hyperslab.

**Throws** — `h5::error::io::dataset::read` on `H5Dread` failure
(type-conversion error, rank mismatch, invalid hyperslab).

**Examples**

```cpp
// (1) Return-by-value
auto vec = h5::read<std::vector<float>>(ds);                          // whole extent
auto mat = h5::read<arma::Mat<double>>(fd, "/grid", h5::count{10,10}, h5::offset{5,0});
auto label = h5::read<std::string>(ds);                                // VLEN string

// (2) Into pre-allocated container
arma::Mat<double> m(10, 10);
h5::read(ds, m, h5::offset{5,0});

// (3) Into raw pointer (note the required count)
std::vector<float> buf(100);
h5::read(ds, buf.data(), h5::count{10,10});
```

@see @ref curated_io_api_dataset_write — symmetric write side,
     @ref curated_io_api_dataset_view — streaming alternative for large datasets

---

@anchor curated_io_api_dataset_view
## `h5::view<T>` — C++20 ranges streaming read

```cpp
template<typename T>
[[nodiscard]] view_range<impl::iterator_t<T>> h5::view(h5::ds_t ds);
```

A **streaming view** over a rank-1 chunked dataset. Returns a
`view_range` satisfying `std::ranges::input_range` — usable in any
range-for loop or `std::ranges` algorithm — that walks the dataset
**one chunk at a time** through the standard h5cpp filter pipeline.
The whole dataset is never materialised in memory.

The iterator pulls a chunk on demand, decompresses it through the
configured filter chain (gzip / shuffle / Gorilla / custom — see
@ref curated_topics_filters), and yields elements one at a time
until exhausted. When the next chunk is needed, the iterator
fetches it; the previous chunk is released. Memory footprint is
**one chunk + the iterator's small bookkeeping**, regardless of
the dataset's total size.

**Constraints**

| Constraint                                | Reason                                                            |
| :---------------------------------------- | :---------------------------------------------------------------- |
| Dataset must be **rank-1**                | The view yields scalars in storage order — higher ranks would need indexing semantics |
| Dataset must be **chunked**               | The streaming model requires chunk-boundary I/O — contiguous and compact layouts can't be streamed by chunk |
| C++ standard ≥ **C++20**                  | Uses `<ranges>` + concept-constrained iterators; gated on `__cplusplus >= 202002L` |
| Element type `T` must be HDF5-native       | Pulled into the iterator's value buffer via the standard `dt_t<T>` pipeline |

**Filter compatibility**

Any filter chain that `h5::impl::basic_pipeline_t` supports is
transparently handled — uncompressed, gzip / deflate, LZ4, Zstd,
Gorilla, shuffle, fletcher32, custom filters. Filtered datasets
benefit the most from the streaming view because they're typically
the ones that don't fit in memory.

**Throws** — `std::runtime_error` on `h5::view` construction if
the dataset isn't rank-1; `h5::error::io::dataset::read` on
chunk-fetch failures during iteration.

**Examples**

```cpp
// Simple — print every element one at a time
h5::ds_t ds = h5::open(fd, "/sensor/samples");        // rank-1 chunked float dataset
for (auto v : h5::view<float>(ds))
    std::cout << v << '\n';

// Pipe through std::ranges algorithms — no intermediate vector
auto sum = std::ranges::fold_left(h5::view<double>(ds), 0.0, std::plus{});

// Filter + count without materialising
auto over_threshold = std::ranges::count_if(
    h5::view<float>(ds),
    [](float v) { return v > 100.0f; });

// Streaming reduction on a multi-gigabyte dataset that wouldn't fit in RAM
double max_seen = std::numeric_limits<double>::lowest();
for (auto v : h5::view<double>(huge_ds)) {
    max_seen = std::max(max_seen, v);
}
```

### When to use `view` vs. ordinary `read`

| Use case                                                          | Choose          |
| :---------------------------------------------------------------- | :-------------- |
| Dataset fits comfortably in memory                                | `h5::read<T>`  |
| Dataset is multi-GB / unbounded                                   | `h5::view<T>`  |
| Need random access by index                                       | `h5::read<T>` with hyperslab `offset`/`count` |
| Need sequential pass + reduction (sum / count / max / fold)       | `h5::view<T>`  |
| Want to pipe through `std::ranges` algorithms                     | `h5::view<T>`  |
| Need rank-2+ dataset                                              | `h5::read<T>` — `view` is rank-1 only |
| Non-chunked (contiguous / compact) dataset                        | `h5::read<T>` — `view` requires chunked layout |

`h5::view` complements rather than replaces the by-value /
by-reference / by-pointer overloads — they handle the "load into a
container" use case; `view` handles the "iterate without loading"
use case.

@see @ref curated_io_api_dataset_read — load-into-container reads;
     @ref curated_topics_filters — filter chain the streaming view runs through;
     @ref curated_io_api_dataset_append — write-side streaming counterpart

---

@anchor curated_io_api_dataset_write
## `h5::write` — deposit a value into a dataset

```cpp
// Low-level — explicit mem/file spaces; rare in user code.
template<class T>
void h5::write(const h5::ds_t& ds, const h5::sp_t& mem_space,
               const h5::sp_t& file_space, const h5::dxpl_t& dxpl,
               const T* ptr);

// Standard — raw pointer into an open dataset.
template<class T, class... args_t>
h5::ds_t h5::write(const h5::ds_t& ds, const T* ptr, args_t&&... args);

// Standard — value or container into an open dataset.
template<class T, class... args_t>
h5::ds_t h5::write(const h5::ds_t& ds, const T& ref, args_t&&... args);

// Create-on-demand — opens dataset if it exists, creates if not.
template<class T, class... args_t>
h5::ds_t h5::write(const h5::fd_t& fd, const std::string& path,
                   const T& ref, args_t&&... args);

// One-shot — opens file + creates/writes in a single call.
template<class T, class... args_t>
h5::ds_t h5::write(const std::string& file_path,
                   const std::string& dataset_path,
                   const T& ref, args_t&&... args);
```

The dispatch is compile-time SFINAE on `T`'s storage representation —
contiguous container, ragged VLEN, fixed-length string, sparse matrix,
etc. all route through dedicated branches.

**Throws** — `h5::error::io::dataset::write` on `H5Dwrite` failure;
`h5::error::io::dataset::create` if create-on-demand failed.

**Examples**

```cpp
// Whole dataset write — shape derived from the value.
h5::write(ds, my_vector);
h5::write(ds, arma::Mat<double>(10, 10, arma::fill::randu));

// Partial write — hyperslab.
h5::write(ds, small_block, h5::offset{5, 0}, h5::count{4, 4});

// Create-on-demand with chunking + compression.
h5::ds_t out = h5::write(fd, "/result", data,
    h5::max_dims{H5S_UNLIMITED}, h5::chunk{1024} | h5::gzip{9});

// One-shot — file + dataset in one call.
h5::write("snapshot.h5", "/state", state_vec);
```

@see @ref curated_io_api_dataset_read, @ref curated_io_api_dataset_append

---

@anchor curated_io_api_dataset_append
## `h5::append` / `h5::flush` / `h5::reset` — streaming append

```cpp
template<class T>
void h5::append(h5::pt_t& pt, const T& ref);     // buffered, per-element
template<class T>
void h5::append(h5::pt_t& pt, const T* ptr);     // raw chunk-sized buffer

void h5::flush(h5::pt_t& pt);                    // explicit flush
void h5::reset(h5::pt_t& pt);                    // reuse the same pt
```

`h5::pt_t` is a packet-table descriptor wrapping an extendable chunked
dataset. `h5::append` buffers values into the active in-memory chunk;
when it fills, the chunk flushes to disk along the first
(slowest-growing) dimension. Multi-rank packet tables write a
hyperplane per call — `ref`'s shape must match
`chunk_dims[1..rank-1]`.

Create the packet table via `h5::create<T>(fd, path,
max_dims{H5S_UNLIMITED}, chunk{N})`.

**Throws** — `h5::error::io::dataset::write` on a flush failure;
`h5::error::io::dataset::close` if the destructor's implicit flush
fails.

**Example — streaming loop**

```cpp
h5::fd_t fd = h5::create("stream.h5", H5F_ACC_TRUNC);
h5::pt_t pt = h5::create<float>(fd, "/stream",
    h5::max_dims{H5S_UNLIMITED}, h5::chunk{1024});

for (float sample : live_stream())
    h5::append(pt, sample);

h5::flush(pt);   // make the trailing partial chunk visible
```

---

@anchor curated_io_api_dataset_sparse
## Sparse — CSC group layout

```cpp
template <class T, class LOC>
h5::gr_t h5::write(const LOC& parent, const std::string& path, const T& src);

template <class T, class LOC>
T h5::read(const LOC& parent, const std::string& path);
```

SFINAE-gated on `is_sparse_v<T>` — the dense and sparse overloads do
not conflict. Source types: `arma::SpMat` / `SpRow` / `SpCol`, Eigen
`SparseMatrix<T, ColMajor, I>` / `SparseVector<T, ColMajor, I>`.

On-disk layout (see
@ref link_linalg_template_types "Supported Linear Algebra Types"
§ Sparse storage layout):

```text
group/
    data    : 1-D dataset, dtype = T,        length nnz
    indices : 1-D dataset, dtype = uint32,   row indices, length nnz
    indptr  : 1-D dataset, dtype = uint32,   column pointers, length n_cols+1
    shape   : 1-D dataset, dtype = uint64,   [n_rows, n_cols]
  @format = "csc"
  @axis   = "column"
```

Byte-compatible with `scipy.sparse.csc_matrix`, Julia `HDF5.jl`, and
the 10x Genomics / Loompy convention.

**Preconditions** (not enforced implicitly — both would require
mutating a `const &`):
- `arma::SpMat`: `SpMat::sync()` must have completed.
- `Eigen::SparseMatrix`: `makeCompressed()` must have been called;
  `ColMajor` is enforced via `static_assert`.

**Example**

```cpp
arma::SpMat<double> A(1000, 1000);
// ... populate A ...
A.sync();

h5::fd_t fd = h5::create("sparse.h5", H5F_ACC_TRUNC);
h5::write(fd, "/A", A);

// Round-trip
auto B = h5::read<arma::SpMat<double>>(fd, "/A");
```

---

## Cross-references

- @ref link_base_template_types "Supported Types" — element-type dispatch matrix
- @ref link_linalg_template_types "Supported Linear Algebra Types" — sparse layout deep-dive
- @ref link_property_lists "Property Lists" — DCPL / DAPL / chunking / filters
- @ref link_error_handler "Error Handling" — `h5::error::io::dataset::*` hierarchy
- @ref curated_io_api_file "FILE" — `fd_t` the parent of every dataset
- @ref curated_io_api_attributes "ATTRIBUTES" — metadata on the dataset
- @ref curated_io_api_groups "GROUPS" — directory structure within a file
