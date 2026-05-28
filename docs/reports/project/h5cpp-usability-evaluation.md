@page reports_usability_evaluation h5cpp Usability Evaluation

> Compiled from codebase survey at staging tip `72408d2b` (post-#149–#153 merge).
> Legend: ✔ Full / ◇ Partial / ✘ Missing

---

## 1. Core I/O Operations

| Feature | Status | Notes |
|---------|--------|-------|
| File create / open / close | ✔ | `h5::create`, `h5::open`, RAII `h5::fd_t` |
| Dataset create / open / read / write | ✔ | One-shot `h5::write`, `h5::read`; RAII `h5::ds_t` |
| Attribute create / open / read / write | ✔ | `h5::awrite`, `h5::aread`; RAII `h5::at_t` |
| Group create / open | ✔ | RAII `h5::gr_t` |
| Packet-table append (`h5::pt_t`) | ✔ | Chunked incremental append with `h5::append` |
| Object close / reference counting | ✔ | RAII wrappers with `H5*close` |

---

## 2. Supported Data Types

| Type Category | Status | Notes |
|---------------|--------|-------|
| All C/C++ POD scalars | ✔ | `bool`, `char`, `short`, `int`, `long`, `long long`, `float`, `double`, `long double`, signed & unsigned variants |
| `std::string` (fixed-length) | ✔ | Stored as fixed-size char array |
| `std::string` (variable-length) | ✔ | HDF5 variable-length string type |
| `char*` / `const char*` | ✔ | Variable-length UTF-8 strings |
| `std::vector<T>` | ✔ | Requires contiguous `T`; `vector<bool>` excluded |
| `std::array<T,N>` | ✔ | Full rank support |
| C-style arrays `T[N]` / `T[][]` | ✔ | Decay to pointer + explicit `h5::count` |
| Raw pointers `T*` | ✔ | Requires explicit `h5::count` |
| `std::complex<T>` | ✔ | Stored as compound `{re,im}` |
| `std::valarray<T>` | ✔ | **New (#149)** — rank-1 only |
| `std::tuple` (sparse triplets) | ✔ | Used for sparse CSR/CSC representation |
| `half_float::half` (OpenEXR half) | ◇ | Only when `HALF_HALF_HPP` or `WITH_OPENEXR_HALF` defined; manual type construction |
| POD structs / compound types | ◇ | **Reflective compound types** via template specialization (`is_reflected_compound_t`); no automatic field introspection |
| Enums | ✘ | No native enum type mapping |
| Bit-fields | ✘ | Not supported |

---

## 3. Linear Algebra Adapters

| Library | Status | Notes |
|---------|--------|-------|
| Armadillo | ✔ | `arma::Mat`, `arma::Col`, `arma::Row`, `arma::Cube`, `arma::SpMat` |
| Eigen | ✔ | Fixed & dynamic matrices, vectors, arrays, tensors |
| Blaze | ✔ | Dense & sparse matrices |
| Blitz++ | ✔ | **New (#150)** — vendored headers |
| Dlib | ✔ | `dlib::matrix` |
| IT++ | ✔ | `itpp::Mat`, `itpp::Vec` |
| uBLAS | ✔ | `boost::numeric::ublas::matrix`, `vector` |
| xtensor | ✔ | **New (#146)** — `xt::xtensor`, `xt::xarray` |
| xtensor-blas | ✔ | **New (#147)** — alias wrapper, no extra code needed |
| OpenCV | ✔ | **New (#148)** — `cv::Mat` with `isContinuous()` guard |
| NT2 | ✔ | **New (#151)** — wrapper types `table1d<T>`, `table2d<T>` for rank disambiguation |
| Newmat | ✔ | **New (#152)** — minimal vendored headers |
| GMTL | ✔ | **New (#153)** — `gmtl::Matrix`, `gmtl::Vec` |
| std::valarray | ✔ | **New (#149)** — header-only, always available |

**Total linalg libraries supported: 14**

---

## 4. Advanced HDF5 Features

| Feature | Status | Notes |
|---------|--------|-------|
| Hyperslab I/O (offset/stride/count/block) | ✔ | Full partial read/write via `h5::offset`, `h5::stride`, `h5::count`, `h5::block` |
| Chunked datasets | ✔ | `h5::chunk{...}` with arbitrary rank |
| Contiguous / compact layout | ✔ | `h5::layout` property |
| Unlimited dimensions + extend | ✔ | `h5::max_dims{H5S_UNLIMITED}` |
| Gzip / deflate compression | ✔ | `h5::gzip{N}` / `h5::deflate{N}` (0–9) |
| Shuffle filter | ✔ | `h5::shuffle` |
| Fletcher32 checksum | ✔ | `h5::fletcher32` |
| N-bit filter | ✔ | `h5::nbit` |
| Fill value | ✔ | `h5::fill_value<T>{value}` |
| Region references | ◇ | `h5::reference_t` + `hdset_reg_ref_t` storage; **experimental** |
| Object references | ✘ | `TODO` in `H5Rall.hpp` |
| Virtual datasets (VDS) | ◇ | `H5D_VIRTUAL` layout constant + DAPL props; **no mapping API** |
| Links (soft / hard / external) | ✘ | Not implemented |
| Named datatypes | ✘ | Not implemented |
| Point selection | ✘ | Not implemented |
| Custom filters (HDF5 plugin API) | ✘ | No C++ wrapper |

---

## 5. Property List Wrapping

| Property List | Status | Notes |
|---------------|--------|-------|
| FAPL (file access) | ✔ | Most common properties wrapped (`sec2`, `core`, `split`, `stdio`, `mpiio`, etc.) |
| FCPL (file creation) | ✔ | `userblock`, `sizes`, `sym_k`, `istore_k` |
| DCPL (dataset creation) | ✔ | `chunk`, `deflate`, `shuffle`, `fletcher32`, `layout`, `fill_value`, `nbit` |
| DAPL (dataset access) | ✔ | `chunk_cache`, `virtual_view`, `virtual_printf_gap` |
| DXPL (data transfer) | ✔ | Most common properties; `mpiio` collective/independent modes |
| LAPL (link access) | ◇ | Partial — some properties wrapped |
| GCPL (group creation) | ◇ | Partial |
| OCPL (object copy) | ✘ | Not wrapped |
| OCPYPL (object copy) | ✘ | Not wrapped |

---

## 6. Parallel I/O

| Feature | Status | Notes |
|---------|--------|-------|
| MPI-IO FAPL (`H5Pset_fapl_mpio`) | ◇ | Property list wrapper exists; **no high-level API** |
| MPI-IO DXPL (collective / independent) | ◇ | Wrappers exist; **examples only, no test coverage** |
| Thread-safe HDF5 | ✘ | Not explicitly supported / tested |
| Async I/O (HDF5 1.13+) | ✘ | Not implemented |

---

## 7. Iteration & Traversal

| Feature | Status | Notes |
|---------|--------|-------|
| List group contents (`h5::ls`) | ✔ | `H5Literate` wrapper in `H5Ialgorithm.hpp` |
| BFS / DFS traversal | ✘ | Stubs present (`H5Ialgorithm.hpp:35 //FIXME: to be implemented`) |
| Object visit (`H5Ovisit`) | ✘ | Not wrapped |
| Group info / metadata | ◇ | Partial — basic HID wrappers only |

---

## 8. Sparse Matrix Support

| Feature | Status | Notes |
|---------|--------|-------|
| Sparse metadata classification | ✔ | `csr_t`, `csc_t`, `bcrs_t`, `cds_t`, `jds_t`, `ss_t` tuples defined in `H5Tmeta.hpp` |
| Armadillo `SpMat` I/O | ◇ | **One example only** (`examples/sparse/arma.cpp`); no generic sparse adapter |
| Blaze sparse | ✘ | Blaze adapter only covers dense types |
| Eigen sparse | ✘ | Not implemented |
| Generic CSR/CSC I/O | ✘ | Classification done, no generic I/O path |

---

## 9. Error Handling

| Feature | Status | Notes |
|---------|--------|-------|
| Exception hierarchy | ✔ | `h5::error::io::*` with granular sub-types (file, dataset, attribute, etc.) |
| Mute / unmute CAPI errors | ✔ | `h5::mute()`, `h5::unmute()` — thread-safe |
| `H5CPP_HARD_ERROR` | ✔ | Compile-time option to `exit(1)` on unrecoverable errors |
| HDF5 error stack introspection | ✘ | Not wrapped |

---

## 10. STL & Container Support (Non-Contiguous)

| Feature | Status | Notes |
|---------|--------|-------|
| `std::map`, `std::unordered_map` | ✘ | Classification done in `H5Tmeta.hpp`; I/O blocked on structural I/O refactor (#89) |
| `std::list`, `std::deque` | ✘ | Same — blocked on #89 |
| `std::set`, `std::unordered_set` | ✘ | Same — blocked on #89 |
| `std::vector<std::vector<T>>` (ragged) | ✘ | Classification done (`ragged_vlen_dataset`); I/O blocked on #89 |
| `std::vector<std::string>` | ◇ | Special-cased in read/write; not via generic VLEN path |

---

## 11. Type System

| Feature | Status | Notes |
|---------|--------|-------|
| Automatic HDF5 type creation from C++ type | ✔ | `h5::impl::detail::hid_t<T,H5Tclose,...>` |
| Type conversion (read path) | ◇ | `H5Tconversion.hpp` present but marked **debug/incomplete** |
| Type conversion (write path) | ◇ | Implicit via HDF5; no explicit C++ casting layer |
| Array types (`H5Tarray`) | ✘ | Not supported |
| Opaque types (`H5Topaque`) | ✘ | Not supported |

---

## 12. Build & Tooling

| Feature | Status | Notes |
|---------|--------|-------|
| CMake build system | ✔ | `add_subdirectory(h5cpp)` friendly |
| Header-only install | ✔ | All code is header-only |
| C++17 required | ✔ | No C++23 concepts |
| CI (Ubuntu 22.04/24.04 × gcc-13/14/15 × clang-17/18/19/20) | ✔ | Full matrix with badge generation |
| Windows CI (MSVC) | ✔ | **Fixed (#141)** — NA badges for non-applicable combos |
| Examples build on Linux CI | ✔ | **Enabled (#144)** — `thirdparty/` vendored deps |
| macOS CI | ✘ | On hold — no GitHub macOS runners |
| Codecov integration | ◇ | **Fix pushed (#140)** but not merged to staging |
| Conan / vcpkg packages | ✘ | Not available |

---

## Summary: Usability Scorecard

| Dimension | Score | Rationale |
|-----------|-------|-----------|
| **Basic I/O** | ⭐⭐⭐⭐⭐ | Read/write/create/open for files, datasets, attributes is seamless |
| **Type coverage** | ⭐⭐⭐⭐☆ | All scalars, strings, vectors, arrays, complex; enums & bit-fields missing |
| **Linalg ecosystem** | ⭐⭐⭐⭐⭐ | 14 libraries — best-in-class for C++ HDF5 |
| **Advanced HDF5** | ⭐⭐⭐☆☆ | Hyperslabs, chunking, filters are great; VDS, links, refs, point sel missing |
| **Parallel I/O** | ⭐⭐☆☆☆ | MPI property wrappers exist but no high-level API or test coverage |
| **Sparse matrices** | ⭐⭐☆☆☆ | Only Armadillo `SpMat` example; no generic sparse adapter |
| **Non-contiguous STL** | ⭐☆☆☆☆ | Maps, lists, ragged arrays classified but I/O blocked on #89 |
| **Error handling** | ⭐⭐⭐⭐☆ | Good exception hierarchy; lacks error stack introspection |
| **Documentation** | ⭐⭐⭐☆☆ | Doxygen + examples; some areas (sparse, MPI) under-documented |
| **Build / CI** | ⭐⭐⭐⭐☆ | Strong Linux+Windows CI; missing macOS, package managers |

---

## Top 5 Most Desirable Unsupported Features

| Rank | Feature | Impact | Blocker / Path Forward |
|------|---------|--------|------------------------|
| 1 | **Generic non-contiguous STL I/O** (maps, lists, ragged arrays) | 🔴 High | Blocked on #89 (structural I/O refactor). Would unlock `std::map`, `std::list`, `vector<vector<T>>`, etc. |
| 2 | **Virtual Dataset (VDS) mapping API** | 🔴 High | Layout constant exists; need `h5::vds_map(src_file, src_dset, dst_dset, src_ext, dst_ext)` helpers. No fundamental blocker. |
| 3 | **Object & soft/hard/external links** | 🟡 Medium | Core HDF5 feature for graph-like file structures. HID wrappers exist; need `h5::link(src, dst, type)` API. |
| 4 | **Generic sparse matrix adapter** (CSR/CSC for all linalg libs) | 🟡 Medium | Classification tuples (`csr_t`, `csc_t`) exist in `H5Tmeta.hpp`; need I/O specializations per library or a generic path. |
| 5 | **Point selection I/O** | 🟡 Medium | Useful for irregular read patterns. `H5Sselect_elements` wrapper needed; no architectural blocker. |

---

## Honorable Mentions (Nice to Have)

| Feature | Impact | Notes |
|---------|--------|-------|
| **Async I/O** (HDF5 1.13+) | 🟢 Low | Emerging standard; not widely needed yet |
| **Custom filter plugins** | 🟢 Low | Niche use case; can use CAPI directly |
| **Named datatypes** | 🟢 Low | Rarely used in scientific computing |
| **Conan / vcpkg packages** | 🟡 Medium | Distribution convenience; build from source works fine |
| **macOS CI** | 🟢 Low | No runner access; code is portable |
| **Enum type mapping** | 🟡 Medium | Common request; straightforward to add via macro or reflection |
| **Type conversion layer** | 🟡 Medium | `H5Tconversion.hpp` exists but incomplete; finish & test |
