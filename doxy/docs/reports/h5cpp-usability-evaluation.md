@page reports_usability_evaluation h5cpp Usability Evaluation

> **Re-evaluation for h5cpp v1.12.6.** Earlier scorecard (post-#149–#153
> merge) is superseded — major gaps closed since then: STL non-contiguous
> containers, async I/O, references, sparse Eigen, C++20+ adoption,
> macOS packaging, h5cpp-compiler reflection, multithreaded filter
> pipeline.
>
> Legend: ✔ Full / ◇ Partial / ✘ Missing / **bold** = new since earlier eval

---

## 1. Core I/O Operations

| Feature | Status | Notes |
|---------|--------|-------|
| File create / open / close | ✔ | `h5::create`, `h5::open`, RAII `h5::fd_t` |
| Dataset create / open / read / write | ✔ | One-shot `h5::write`, `h5::read`; RAII `h5::ds_t` |
| **Streaming read (`h5::view<T>`)** | ✔ | C++20 ranges view over rank-1 chunked datasets — one chunk at a time, decompressed through the filter pipeline; works with `std::ranges` algorithms |
| Attribute create / open / read / write | ✔ | `h5::awrite`, `h5::aread`, `h5::adelete`; RAII `h5::at_t` |
| **Attribute bracket-syntax sugar** | ✔ | `parent["name"] = value` (write), `T v = parent["name"]` (implicit read) on `ds_t` / `gr_t` / `ob_t` |
| Group create / open | ✔ | RAII `h5::gr_t`, `h5::gcreate` / `h5::gopen` with intermediate-path auto-create via LCPL |
| Packet-table append (`h5::pt_t`) | ✔ | Chunked incremental append with `h5::append` / `h5::flush` / `h5::reset` |
| Object close / reference counting | ✔ | RAII wrappers with `H5*close`; `H5Iinc_ref` on copy |

---

## 2. Supported Data Types

| Type Category | Status | Notes |
|---------------|--------|-------|
| All C/C++ POD scalars | ✔ | `bool`, `char`, `short`, `int`, `long`, `long long`, `float`, `double`, `long double`, signed & unsigned variants |
| `std::string` (fixed-length) | ✔ | `char[N]` / `std::array<char,N>` → `H5T_C_S1 + H5Tset_size(N)` |
| `std::string` (variable-length) | ✔ | `H5T_C_S1, H5T_VARIABLE` |
| `char*` / `const char*` | ✔ | VLEN UTF-8 strings |
| `std::vector<T>` | ✔ | Requires contiguous `T`; `vector<bool>` excluded |
| `std::array<T,N>` | ✔ | Full rank support via `array_element` (top-level) or `array_dataset` (`vector<array<T,N>>`) |
| C-style arrays `T[N]` / `T[][]` | ✔ | Same dispatch as `std::array<T,N>` |
| Raw pointers `T*` | ✔ | Requires explicit `h5::count` |
| `std::complex<T>` | ✔ | Stored as `H5T_COMPLEX` native (HDF5 ≥ 2.0) or compound `{r,i}` fallback for HDF5 < 2.0 |
| `std::valarray<T>` | ✔ | Rank-1 via `h5cpp/H5Mvalarray.hpp` mapper |
| `std::tuple<Ts...>` / `std::pair<K,V>` | ✔ | Top-level: scalar compound; in `vector<>`: rank-1 of compound |
| **`std::map` / `std::unordered_map`** | ✔ | `key_value_dataset` — rank-1 of `H5T_COMPOUND { K key; V value; }` |
| **`std::set` / `std::unordered_set`** | ✔ | `linear_value_dataset` via iter-staging |
| **`std::list` / `std::deque` / `std::forward_list`** | ✔ | `linear_value_dataset` via iter-staging |
| **`std::vector<std::vector<T>>` (ragged)** | ✔ | `ragged_vlen_dataset` via `hvl_t` relay |
| **`std::vector<std::string>`** | ✔ | `vlen_text_dataset` via generic VLEN path (no longer special-cased) |
| **`std::vector<std::array<T,N>>`** | ✔ | `array_dataset` — rank-1 of `H5T_ARRAY[N]` |
| **`std::vector<std::array<char,N>>`** | ✔ | `fls_dataset` — rank-1 of fixed-length strings, no VLEN |
| **`std::unique_ptr<T[]>` / `std::shared_ptr<T[]>`** | ✔ | `h5cpp/H5Mmemory_io.hpp` mapper — forwarded to raw pointer overloads |
| **`std::mdspan<T,Extents>` (C++23)** | ✔ | Gated on `__cpp_lib_mdspan >= 202207L`; rank from `Extents` |
| **`std::initializer_list<T>`** | ✔ | Convenience: `h5::awrite(parent, "axes", {1,2,3})` |
| `half_float::half` (IEEE 754 binary16) | ◇ | Optional via `examples/half-float`; opt-in include |
| POD compound structs | ✔ | `H5CPP_REGISTER_STRUCT(T)` macro (POD) or **h5cpp-compiler** for non-POD |
| **Non-POD compound (vector / string / map fields)** | ✔ | **h5cpp-compiler** (Clang-based) emits scatter/gather + descriptor at pre-build; C++26 reflection roadmap covers the header-only future |
| `enum class E : T` | ✔ | Mapped to `H5T_ENUM` via explicit `H5CPP_REGISTER_DATATYPE(E, "E", H5Tenum_create(H5T_NATIVE_*), { H5Tenum_insert(handle, "Name", &val); … })` macro. Implicit use without registration falls to the underlying integer with no member names retained. |
| Bit-fields | ✘ | Not supported |

---

## 3. Linear Algebra Adapters

| Library | Status | Notes |
|---------|--------|-------|
| Armadillo (dense) | ✔ | `arma::Mat`, `Col`, `Row`, `Cube` via `h5cpp/H5Marma.hpp` |
| **Armadillo (sparse)** | ✔ | `arma::SpMat`, `SpRow`, `SpCol` — native CSC, `sync()` precondition |
| Eigen (dense) | ✔ | `Eigen::Matrix<T,R,C,O>`, `Eigen::Array<T,R,C,O>` — any rows/cols/options |
| **Eigen (sparse)** | ✔ | `Eigen::SparseMatrix<T,ColMajor>`, `SparseVector<T,ColMajor>` — `makeCompressed()` precondition, gated on `EIGEN_SPARSECORE_MODULE_H` |
| Blaze | ✔ | `blaze::DynamicVector`, `DynamicMatrix` (both orientations) |
| Blitz++ | ✔ | `blitz::Array<T,N>` rank-N |
| dlib | ✔ | `dlib::matrix<T,0,0,...,row_major_layout>` |
| IT++ | ✔ | `itpp::Vec`, `Mat` |
| uBLAS | ✔ | `boost::numeric::ublas::vector`, `matrix` |
| xtensor | ✔ | `xt::xarray<T>` (dynamic rank), `xt::xtensor<T,N>` (static rank) |
| xtensor-blas | ✔ | Companion mapper for xtensor-blas types |
| std::valarray | ✔ | header-only via `h5cpp/H5Mvalarray.hpp` |

**Sparse on-disk layout**: canonical CSC group with `data` / `indices` /
`indptr` / `shape` datasets + `@format` / `@axis` attributes. Byte-compatible
with `scipy.sparse.csc_matrix`, Julia `HDF5.jl`, 10x Genomics, Loompy.

**Total linalg libraries supported**: 11 dense + 2 sparse + valarray.

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
| **Scale-offset filter** | ✔ | `h5::scaleoffset{factor, offset}` |
| SZIP filter | ✔ | `h5::szip{opts, blocks}` (HDF5 1.10+ built-in) |
| Fill value | ✔ | `h5::fill_value<T>{value}` |
| **Gorilla time-series compression** | ✔ | `h5::gorilla` — Facebook delta-of-delta codec, ~10-20× ratio on smooth float streams |
| **Custom filter pipeline** | ✔ | `examples/custom-pipeline/` walkthrough; `H5Zregister` recipe in @ref curated_topics_filters |
| **High-throughput parallel filter pipeline** | ✔ | `h5::high_throughput{h5::threads{N}}` DAPL tag — pool-parallel decompression |
| **Region references** | ✔ | `h5::reference_t` (RAII rule-of-five) |
| **Object references** | ✔ | Same `h5::reference_t` (HDF5 1.12 unified token API) |
| Virtual datasets (VDS) | ◇ | `H5D_VIRTUAL` layout + DAPL props wrapped; **no high-level `h5::vds_map(...)` helper** |
| **Links (soft / hard / external)** | ✔ | `h5::link_soft` / `h5::link_hard` / `h5::link_external` in `H5Ialgorithm.hpp` — full templates over `is_valid_loc<Loc>` |
| **Move / copy / unlink** | ✔ | `h5::move` (H5Lmove), `h5::copy` (H5Ocopy cross-file), `h5::unlink` (H5Ldelete) |
| Named datatypes | ✘ | `H5Tcommit` not wrapped |
| Point selection | ✘ | `H5Sselect_elements` not wrapped |

---

## 5. Property List Wrapping

| Property List | Status | Notes |
|---------------|--------|-------|
| FAPL (file access) | ✔ | Drivers: `sec2`, `core`, `split`, `stdio`, `family`, `mpiio`, **`ros3`** (read-only S3), **`async`** |
| FCPL (file creation) | ✔ | `userblock`, `sizes`, `sym_k`, `istore_k` |
| DCPL (dataset creation) | ✔ | `chunk`, `deflate`, `shuffle`, `fletcher32`, `layout`, `fill_value`, `nbit`, `scaleoffset`, `szip`, **`gorilla`**, custom filters |
| DAPL (dataset access) | ✔ | `chunk_cache`, `virtual_view`, `virtual_printf_gap`, **`high_throughput{threads}`** for parallel filter pipeline |
| DXPL (data transfer) | ✔ | `mpiio` collective/independent modes |
| **LCPL (link creation)** | ✔ | UTF-8 encoding + intermediate-group auto-create (default), per-call override |
| **ACPL (attribute creation)** | ✔ | Character encoding |
| LAPL (link access) | ◇ | Partial — some properties wrapped |
| GCPL (group creation) | ◇ | Partial |
| OCPL (object create) | ✘ | Not wrapped |
| OCPYPL (object copy) | ✘ | Not wrapped |

---

## 6. Parallel I/O & Async Mode

| Feature | Status | Notes |
|---------|--------|-------|
| **MPI-IO FAPL high-level API** | ✔ | `h5::fapl{h5::driver::mpi{comm, info}}`; works on **any filesystem** the MPI runtime supports — no parallel FS required |
| **MPI-IO collective / independent DXPL** | ✔ | `h5::dxpl{} \| h5::collective` / `\| h5::independent`; test coverage in `examples/mpi/` |
| **FAPL-scoped worker pool (Phase I)** | ✔ | `h5::threads{N}` on FAPL — pool serves filter pipeline + I/O staging |
| **Async-mode descriptors (Phase II)** | ✔ | `h5::async::fd_t` / `ds_t` / `at_t` / `gr_t` / `ob_t` — compile-time mode discrimination; `operator ::hid_t() = delete` to prevent raw-CAPI escape |
| **Async-mode factories** | ✔ | `h5::async::create` / `h5::async::open` propagate the FAPL-scoped executor through descendant descriptors |
| **HDF5 1.13+ official async** | ◇ | `H5Dwrite_async` / `H5Dread_async` CAPI not yet directly wrapped; h5cpp's async-mode dispatch achieves the same outcome through the FAPL worker pool |
| **Threaded filter pipeline (sigma queue)** | ✔ | DAPL high-throughput tag activates pool-parallel decompression; design documented in @ref reports_threaded_pipeline_sigma_queue |

---

## 7. Iteration & Traversal

| Feature | Status | Notes |
|---------|--------|-------|
| List group contents (`h5::ls`) | ✔ | `H5Literate` wrapper in `H5Ialgorithm.hpp`; returns `std::vector<std::string>` |
| **Depth-first traversal (`h5::dfs`)** | ✔ | `H5Lvisit` with `H5_ITER_INC`, returns paths relative to start node |
| **Breadth-first traversal (`h5::bfs`)** | ✔ | `H5Lvisit` with breadth-first ordering |
| **Path existence (`h5::exists`)** | ✔ | `H5Lexists` wrapper, quiet-on-missing (returns `bool` rather than throwing) |
| Object visit (`H5Ovisit`) | ◇ | `H5Lvisit` covers most use cases via `dfs` / `bfs`; raw `H5Ovisit` not separately wrapped |
| Group info / metadata | ◇ | `exists` + `ls` + `dfs` / `bfs` cover most cases; `H5Gget_info` not wrapped |

---

## 8. Sparse Matrix Support

| Feature | Status | Notes |
|---------|--------|-------|
| **Canonical CSC group layout** | ✔ | `data` / `indices` / `indptr` / `shape` + `@format="csc"` / `@axis="column"` — scipy / Julia / 10x / Loompy compatible |
| **Armadillo `SpMat` / `SpRow` / `SpCol` I/O** | ✔ | Native CSC, `sync()` precondition |
| **Eigen sparse I/O** | ✔ | `SparseMatrix<T,ColMajor>`, `SparseVector<T,ColMajor>`; RowMajor refused at compile time |
| Sparse metadata classification (`csr_t` / `bcrs_t` etc.) | ✔ | Still defined in `H5Tmeta.hpp`; CSC is the canonical landed form |
| Generic CSR-on-disk path | ◇ | CSC is the primary path; CSR conversion happens at the call site if needed |
| Blaze sparse | ✘ | Blaze dense only; sparse adapter not implemented |

---

## 9. Error Handling

| Feature | Status | Notes |
|---------|--------|-------|
| **Exception hierarchy (48 leaves)** | ✔ | `h5::error::any` → `io::{file,dataset,attribute,group,packet_table}::*` → 36 op-specific leaves + `property_list::*` |
| **Context-sensitive catching** | ✔ | Catch by operation × object kind (e.g. `h5::error::io::attribute::open`) — distinguish failures by *what was being attempted* |
| `CHECK_*` macro family | ✔ | `H5CPP_CHECK_NZ` / `_PROP` / `_LT0` wrappers capture HDF5 error stack + source location into `.what()` |
| **Rollback semantics** | ✔ | `*::rollback` branch (not derived from `runtime_error`) signals "partial state cleaned up" |
| Mute / unmute CAPI errors | ✔ | `h5::mute` (RAII) / manual `h5::mute()` / `h5::unmute()` — thread-safe |
| `H5CPP_HARD_ERROR` | ✔ | Compile-time option to `exit(1)` on unrecoverable errors |
| HDF5 error stack introspection | ✔ | Folded into exception `.what()` via `H5Eget_current_stack` / `H5Ewalk2` |

---

## 10. STL & Container Support (non-contiguous) — **major change**

| Feature | Status | Notes |
|---------|--------|-------|
| **`std::map` / `std::unordered_map`** | ✔ | `key_value_dataset` → compound `{K key; V value}` |
| **`std::list` / `std::deque` / `std::forward_list`** | ✔ | `linear_value_dataset` via iter-staging through `vector<T>` buffer |
| **`std::set` / `std::unordered_set`** | ✔ | Same — iter-staging path |
| **`std::vector<std::vector<T>>` (ragged)** | ✔ | `ragged_vlen_dataset` via `hvl_t` relay |
| **`std::vector<std::string>`** | ✔ | `vlen_text_dataset` via generic VLEN path |
| **`std::vector<std::tuple<Ts...>>`** | ✔ | Per-element compound packing |
| **Third-party containers (Abseil, Boost.Container, Folly, EASTL, plf, robin-hood, phmap)** | ◇ | Should work via Walter Brown structural detection — see @ref curated_topics_stl § Third-party libraries (untested but structurally compatible) |

The earlier "blocked on #89" entry is **resolved** — the unified dispatch
goes through `meta::access_traits_t<T>` + `storage_representation_v<T>`
trait composition (Walter Brown feature detection).

---

## 11. Type System

| Feature | Status | Notes |
|---------|--------|-------|
| Automatic HDF5 type creation from C++ type | ✔ | `h5::dt_t<T>` + `meta::resolved_type_t<T>` |
| **Walter Brown feature-detection dispatch** | ✔ | `has_value_type` / `has_data_method` / `has_size_method` / `is_iterator_only` / `has_tuple_size` traits compose into `access_traits_t::kind` |
| **`storage_representation_t` taxonomy** | ✔ | 11-value enum classifies every supported type into a dispatch slot |
| Type conversion (read path) | ◇ | `H5Tconversion.hpp` covers the common cases; some edge cases still flagged TODO |
| Type conversion (write path) | ✔ | Implicit via HDF5; `h5::dt_t<T>` resolves to native types |
| Array types (`H5Tarray`) | ✔ | `array_element` / `array_dataset` / `fls_dataset` storage reps |
| **VLEN types (`H5Tvlen_create`)** | ✔ | `ragged_vlen_dataset` for `vector<vector<T>>`, VLEN strings for text |
| Opaque types (`H5Topaque`) | ✔ | Mapped via explicit `H5CPP_REGISTER_DATATYPE(T, "name", H5Tcreate(H5T_OPAQUE, n), { H5Tset_tag(handle, "…"); })` — same registration path as `H5T_ENUM`. |

---

## 12. Reflection — **new section**

| Feature | Status | Notes |
|---------|--------|-------|
| **POD struct macro (`H5CPP_REGISTER_STRUCT`)** | ✔ | One-line registration; in-memory == on-disk layout |
| **h5cpp-compiler (Clang LibTooling, external)** | ✔ | Walks AST, emits compound descriptor + `gather<T>` / `scatter<T>` for non-POD types with vector/string/map fields. Zero-copy on write; one-copy on read. Repo: <https://github.com/vargalabs/h5cpp-compiler> |
| **Multi-backend emission (HDF5 + Protobuf + JSON Schema + SQL DDL + Avro)** | ◇ | HDF5 producer landed; other producers on the multi-backend roadmap — design at @ref reports_compiler_multi_backend_architecture |
| **C++26 reflection (P2996 + P3394)** | ◇ | Roadmap document at @ref reports_reflection_cpp26_roadmap; header-only `h5cpp/reflection/` path planned for the first C++26-complete compiler |
| **Field-level annotations** | ✔ | Attribute syntax for h5cpp-compiler today (`[[h5::name("x")]]`, `[[h5::chunk(1024)]]`, `[[h5::on_missing("...")]]`); same vocabulary planned for C++26 annotations (`[[=h5::chunk{1024}]]`) |

---

## 13. Build & Tooling

| Feature | Status | Notes |
|---------|--------|-------|
| CMake build system | ✔ | `add_subdirectory(h5cpp)` friendly + `find_package` config |
| Header-only install | ✔ | All h5cpp code is header-only; h5cpp-compiler is a separate optional binary |
| **C++20 default** | ✔ | `CMAKE_CXX_STANDARD = 20`; **C++23 default in newer trees** |
| C++23 features used | ✔ | `std::mdspan` (gated), structural CTAD, designated initializers |
| Linux CI (Ubuntu 22.04 / 24.04 × gcc-13/14/15 × clang-17/18/19/20) | ✔ | Full matrix with badge generation |
| Windows CI (MSVC) | ✔ | Active matrix entry |
| **macOS CI / packaging** | ✔ | `macos-15 / arm64 / pkg` job in `.github/workflows/package.yml` |
| **HDF5 version matrix** | ✔ | 1.10.x / 1.12.x / 1.14.x tested in CI; gates feature-specific paths (e.g. SWMR requires ≥ 1.12.3) |
| Examples build on CI | ✔ | `thirdparty/` vendored dependencies for reproducible builds |
| **Public CDash dashboard** | ✔ | <https://my.cdash.org/index.php?project=h5cpp> — community submissions welcomed via `ctest -D Experimental`; see @ref curated_topics_cdash |
| **Codecov coverage** | ✔ | <https://app.codecov.io/gh/vargalabs/h5cpp/tree/release> — per-PR coverage diffs, per-file annotations, branch trendline |
| Conan / vcpkg packages | ✘ | Not available — build from source or use the macOS `.pkg` |

---

## Summary: Usability Scorecard

| Dimension | Was | Now | Rationale |
|-----------|-----|-----|-----------|
| **Basic I/O** | ●●●●● | ●●●●● | Seamless; bracket-syntax sugar for attributes added |
| **Type coverage** | ●●●●○ | ●●●●● | STL non-contiguous unblocked; mdspan; smart pointers; init-lists; ragged VLEN |
| **Linalg ecosystem** | ●●●●● | ●●●●● | 11 dense + 2 sparse (arma + eigen); CSC scipy/Julia/10x interop |
| **Advanced HDF5** | ●●●○○ | ●●●●○ | References ✔; high-throughput pipeline ✔; Gorilla ✔; custom filters ✔. Still missing: links, point selection, VDS mapping API |
| **Parallel I/O** | ●●○○○ | ●●●●○ | Async mode (Phase II) with type-level discrimination; FAPL worker pool; MPI collective/independent tested |
| **Sparse matrices** | ●●○○○ | ●●●●○ | Eigen + Armadillo sparse, canonical CSC group, scipy/Julia/10x interop |
| **Non-contiguous STL** | ●○○○○ | ●●●●● | #89 resolved — map / list / set / deque / ragged vlen all routed |
| **Error handling** | ●●●●○ | ●●●●● | 48 typed exceptions; error-stack folded into `.what()`; rollback branch |
| **Reflection** | n/a | ●●●●○ | New section — h5cpp-compiler today, C++26 reflection on roadmap |
| **Documentation** | ●●●○○ | ●●●●● | Curated Doxygen `@page` tree (I/O / TOPICS / COOKBOOK); 28 cookbook examples; multi-axis nav |
| **Build / CI / dashboard** | ●●●●○ | ●●●●● | macOS packaging; HDF5 version matrix; public CDash dashboard; CDash coverage |

---

## Top 5 Most Desirable Unsupported Features

> Each one of the original Top 5 has been addressed or substantially
> mitigated. Re-evaluated for v1.12.6:

| Rank | Feature | Impact | Status / Path Forward |
|------|---------|--------|------------------------|
| 1 | **Virtual Dataset (VDS) mapping API** | 🟡 Medium | `H5D_VIRTUAL` layout + DAPL wrapped; high-level `h5::vds_map(src, dst, src_ext, dst_ext)` helper still missing. Architectural prerequisites all in place. |
| 2 | **Point selection I/O** | 🟢 Low | `H5Sselect_elements` not wrapped. Useful for irregular read patterns; no architectural blocker. |
| 3 | **Named datatypes (`H5Tcommit`)** | 🟢 Low | Useful for cross-file consistency of compound types; rare in scientific computing. |
| 4 | **Native HDF5 1.13+ async API** | 🟢 Low | h5cpp's async-mode dispatch already achieves the outcome via FAPL worker pool; wrapping `H5Dwrite_async` / `H5Dread_async` directly would complement it but isn't gating any use case. |
| 5 | **Conan / vcpkg distribution** | 🟢 Low | macOS `.pkg` covers Apple; Linux distros are encouraged to package; cmake-`find_package` works from source. Lift if community contributors offer the packaging. |

---

## Honorable Mentions (Nice to Have)

| Feature | Impact | Notes |
|---------|--------|-------|
| **Named datatypes (`H5Tcommit`)** | 🟢 Low | Rare in scientific computing; useful for cross-file consistency. |
| **HDF5 1.14 dimension-scale wrappers** | 🟡 Medium | `H5DSset_scale` / `H5DSattach_scale` not wrapped; netCDF4 ↔ HDF5 interop is the primary motivator. |
| **Multi-backend emission rollout** | 🟡 Medium | h5cpp-compiler's HDF5 producer is the only landed backend; Protobuf / JSON Schema / SQL DDL / Avro on the roadmap. Significant LLM-tooling / RPC-schema upside when complete. |
| **C++26 reflection completion** | 🟢 Low (timed) | Waiting on GCC 16.1+ / Clang 21+ / MSVC 2026 — the path is designed (see roadmap) and will eliminate the external-tool dependency for non-POD types. |
| **Type conversion (read path) completion** | 🟡 Medium | `H5Tconversion.hpp` covers common cases; some explicit casts still flagged TODO. Hardening pass would close the last narrowing-cast edge cases. |

---

## What changed since the earlier evaluation

The headline shifts:

- **STL non-contiguous unblocked** — #89 resolved; `std::map`, `std::list`,
  `std::set`, ragged `vector<vector<T>>` all dispatched through the unified
  trait-based path (Walter Brown feature detection at the core).
- **Eigen sparse landed** — joined Armadillo on the canonical CSC group
  layout; scipy/Julia/10x interop established.
- **Async mode (Phase II)** — separate `h5::async::*` descriptor types
  with `operator ::hid_t() = delete` so the type system prevents
  raw-CAPI escape paths.
- **References full** — `h5::reference_t` covers both region and object
  references with RAII rule-of-five semantics.
- **macOS packaging** — `macos-15 / arm64 / pkg` job in CI produces a
  signed `.pkg` for Apple Silicon.
- **Public CDash dashboard** — submissions welcomed from the community;
  per-platform / per-compiler / per-HDF5-version matrix view.
- **h5cpp-compiler** — multi-backend reflection toolchain landed;
  HDF5 producer in production, four others on the roadmap.
- **Documentation overhaul** — curated Doxygen `@page` tree with three
  parallel axes (I/O / TOPICS / COOKBOOK); 28 cookbook examples; full
  Walter Brown idiom write-up; per-topic deep-dives.
- **C++20 default**, C++23 features used (mdspan); C++26 reflection on
  the roadmap.

The original 5-circle scorecard now sits at **7 dimensions at 5 circles,
4 at 4 circles** — the lowest ratings are on Advanced HDF5 (VDS mapping
helper, point selection still missing), Parallel I/O (native 1.13+
async CAPI not directly wrapped), Sparse (Blaze sparse not implemented),
and Reflection (multi-backend producers landed but C++26 path waits on
compiler support). No category sits below 4 circles.
