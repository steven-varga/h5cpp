@page reports_type_system_map h5cpp Type-System Map — Today's Reality

> Comprehensive coverage of every C++ source type the dispatch matrix recognises and the HDF5 type / storage shape it currently lands on disk as. Compiled from the STL showdown (`examples/stl/`), the linalg showdown (`examples/linalg/`), the smart-pointer mapper (`examples/smart-ptr/`), the compound examples (`examples/compound/`, `examples/csv/`, `examples/multi-tu/`, `examples/packet-table/`), the architecture notes (`tasks/h5cpp-type-system-architecture-notes.md`), and direct inspection of the dispatch source in `h5cpp/H5T*.hpp`, `h5cpp/H5D*.hpp`, `h5cpp/H5M*.hpp`.

> **Canonical-mapping update landed.** Fixed-extent compile-time shape now lives in the *element type* (`H5T_ARRAY[N]`, `H5T_STRING(N)`), with a scalar (or rank-1) dataspace. The previous `c_array` / `fixed_inner_extent` collapsing paths have been replaced by `array_element`, `array_dataset`, and `fls_dataset`. The two rules driving the model:
>
> 1. **Compile-time fixed extent** (`std::array<T,N>`, `T[N]`) → HDF5 *element type* (`H5T_ARRAY[N]`), scalar dataspace.
> 2. **Runtime-variable extent** (`std::vector<T>`, `std::list<T>`, etc.) → HDF5 *simple dataspace* + hyperslab.
>
> The two compose orthogonally: `std::vector<std::array<T,N>>` is a rank-1 simple dataspace whose element type is `H5T_ARRAY[N]`. `std::array<std::array<T,N>, M>` is a scalar dataspace whose element type is `H5T_ARRAY[M] H5T_ARRAY[N]`. `char` is the documented exception — character arrays land as `H5T_STRING(N)` (fixed-length string) rather than `H5T_ARRAY[N] H5T_NATIVE_CHAR`, matching the NumPy / h5py convention.

**Status legend** (project convention):

| Symbol | Meaning |
| --- | --- |
| ✔ ok    | round-trips correctly today; on-disk shape and values both preserved |
| ✘ fail  | path is reachable but has a library-level defect (value or shape diverges, throws, or hard-faults) |
| ◇ na    | unsupported by design; the compile-time stopper fires correctly (no silent failure on disk) |

The table reports **what the code does now**. The proposed `H5T_ARRAY` / fixed-length-string-element model is documented separately in `tasks/h5cpp-type-system-architecture-notes.md`.

---

## 1. Native arithmetic types

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `bool` (scalar) | `H5T_NATIVE_HBOOL`, scalar dataspace | ✔ ok |
| Integer types — signed (`char`† / `signed char` / `short` / `int` / `long` / `long long`) | `H5T_NATIVE_CHAR` / `H5T_NATIVE_SCHAR`‡ / `H5T_NATIVE_SHORT` / `H5T_NATIVE_INT` / `H5T_NATIVE_LONG` / `H5T_NATIVE_LLONG`, scalar dataspace | ✔ ok |
| Integer types — unsigned (`unsigned char` / `unsigned short` / `unsigned int` / `unsigned long` / `unsigned long long`) | `H5T_NATIVE_UCHAR` / `H5T_NATIVE_USHORT` / `H5T_NATIVE_UINT` / `H5T_NATIVE_ULONG` / `H5T_NATIVE_ULLONG`, scalar dataspace | ✔ ok |
| Floating-point (`float` / `double` / `long double`) | `H5T_NATIVE_FLOAT` / `H5T_NATIVE_DOUBLE` / `H5T_NATIVE_LDOUBLE`, scalar dataspace | ✔ ok |
| Half-float — `half_float::half` (vendored, gated on `HALF_HALF_HPP`) | `H5Tcopy(H5T_NATIVE_FLOAT) + H5Tset_fields/H5Tset_size(2)` (16-bit IEEE half) | ✔ ok |
| Half-float — `OPENEXR::half` (gated on `WITH_OPENEXR_HALF`) | Same 16-bit IEEE half representation | ✔ ok |
| Half-float — `std::float16_t` (C++23, gated on `__STDCPP_FLOAT16_T__`) | Same 16-bit IEEE half representation | ✔ ok |
| `enum` / `enum class` (any underlying integer type) | `H5T_ENUM` over the underlying integer; named enumerators inserted via `H5Tenum_insert` (compiler-emitted) | ✔ ok |

† `char` has implementation-defined signedness; `dt_t<char>` is `H5T_NATIVE_CHAR` (the platform's choice). For text containers see §4.
‡ When `signed char` and `char` happen to coincide on a platform, `dt_t<signed char>` resolves to `H5T_NATIVE_SCHAR`; the two types remain distinct in the C++ type system regardless.

---

## 2. Other primitive / opaque elements

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `h5::reference_t` (object / region reference) | `H5T_STD_REF` (HDF5 ≥ 1.12) or `H5T_STD_REF_DSETREG` (older), scalar dataspace | ✔ ok |
| `std::complex<float>` / `std::complex<double>` / `std::complex<long double>` (scalar) | `H5T_COMPLEX` (HDF5 ≥ 1.16) or `H5T_COMPOUND { re, im }` fallback, scalar dataspace | ✔ ok |

---

## 3. Compound POD structs — Tier-1 (compiler-assisted)

POD struct registered via `H5CPP_REGISTER_STRUCT(T)`. The compiler emits a `register_struct<T>()` body that builds the `H5T_COMPOUND` with `H5Tinsert` for each field at its `HOFFSET`. Trivially copyable, standard-layout. See `examples/compound/`, `examples/csv/`, `examples/multi-tu/`, `examples/packet-table/`.

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| Registered POD struct (e.g. `sn::example::Record`) — top-level scalar | `H5T_COMPOUND { … fields … }`, scalar dataspace | ✔ ok |
| `std::vector<RegisteredPOD>` | rank-1 dataspace of `H5T_COMPOUND` elements | ✔ ok |
| Per-field `char[N]` inside a registered POD (e.g. `input_t::ReportedLocation` in csv) | `H5T_ARRAY[N] H5T_NATIVE_CHAR` within the compound | ✔ ok |
| Per-field nested POD struct (e.g. `sn::example::Record::field_03`) | Nested `H5T_COMPOUND` within the parent compound | ✔ ok |
| Per-field array of nested POD (e.g. `Record::field_05[3][8]`) | Nested `H5T_ARRAY` of `H5T_ARRAY` of `H5T_COMPOUND` | ✔ ok |
| Unregistered POD aggregate | — | ◇ na (storage = `unsupported`; static_assert directs caller to `H5CPP_REGISTER_STRUCT`) |

---

## 4. Text / string types

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `std::string` (scalar) | `H5T_C_S1 + H5Tset_size(H5T_VARIABLE) + H5Tset_cset(H5T_CSET_UTF8)`, scalar dataspace | ✔ ok |
| `std::string_view` (write-only, read into `std::string`) | Same variable-length string type, scalar dataspace | ✔ ok |
| `char*` / `const char*` | `H5T_C_S1 + H5T_VARIABLE`, scalar dataspace | ✔ ok |
| `char[N]` (top-level) | `H5T_C_S1 + H5Tset_size(N)` (fixed-length string), scalar dataspace | ✔ ok |
| `std::array<char, N>` (top-level) | `H5T_C_S1 + H5Tset_size(N)`, scalar dataspace (`fixed_length_string`) | ✔ ok |
| `signed char[N]` / `unsigned char[N]` (top-level, non-char element type) | rank-1 dataspace of N `H5T_NATIVE_SCHAR` / `H5T_NATIVE_UCHAR` (`c_array` storage) | ✔ ok (byte arrays as intended) |
| `wchar_t[N]`, `char16_t[N]`, `char32_t[N]` (wide-char strings) | — | ◇ na (HDF5 vlen string is 8-bit; no wide-char path) |
| `std::vector<std::string>` | rank-1 dataspace of `H5T_C_S1 + H5T_VARIABLE` elements (`vlen_text_dataset`) | ✔ ok |
| `std::list<std::string>` / `std::set<std::string>` / etc. | rank-1 dataspace of vlen strings via iterator staging | ✔ ok (structural fallback) |
| `std::basic_string<char16_t>` / `<char32_t>` / `<wchar_t>` | — | ◇ na |

---

## 5. STL fixed-extent containers (canonical model — H5T_ARRAY element)

Canonical Winston model: fixed-extent compile-time shape lives in the *element type* (`H5T_ARRAY[N]`), not in the dataspace. Top-level fixed-extent containers therefore land as a **scalar dataspace** carrying a single `H5T_ARRAY` element.

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `std::array<T, N>` (T non-char arithmetic) | scalar dataspace, `H5T_ARRAY[N] dt_t<T>` element (`array_element`) | ✔ ok |
| `T[N]` (T non-char arithmetic) | scalar dataspace, `H5T_ARRAY[N] dt_t<T>` element (`array_element`) | ✔ ok |
| `T[N][M]` / `T[N][M][P]` / … up to rank 7 | scalar dataspace, multi-dim `H5T_ARRAY` element | ✔ ok |
| `std::array<std::array<T, N>, M>` (nested fixed) | scalar dataspace, `H5T_ARRAY[M] H5T_ARRAY[N] dt_t<T>` (`array_element`) | ✔ ok |

---

## 6. STL sequence containers (variable-extent → rank-1 dataspace + hyperslab)

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `std::vector<T>` (T arithmetic / `complex` / POD) | rank-1 dataspace of `dt_t<T>` (`linear_value_dataset`) | ✔ ok |
| `std::deque<T>` (T arithmetic) | rank-1 dataspace via iterator-staging buffer (`linear_value_dataset`) | ✔ ok |
| `std::list<T>` (T arithmetic) | rank-1 dataspace via iterator-staging buffer | ✔ ok |
| `std::forward_list<T>` (T arithmetic) | rank-1 dataspace via iterator-staging buffer | ✔ ok |
| `std::vector<bool>` | — | ◇ na (bit-packed, no contiguous `bool*`; storage = `unsupported`) |

---

## 7. STL set-shaped containers

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `std::set<T>` / `std::multiset<T>` | rank-1 dataspace of `dt_t<T>` via iterator-staging (`linear_value_dataset`) | ✔ ok |
| `std::unordered_set<T>` / `std::unordered_multiset<T>` | rank-1 dataspace of `dt_t<T>` via iterator-staging | ✔ ok (insertion order not preserved across round-trip) |

---

## 8. STL map-shaped containers (compound key-value)

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `std::map<K, V>` / `std::multimap<K, V>` | rank-1 dataspace of `H5T_COMPOUND { K key; V value }` (`key_value_dataset`) | ✔ ok |
| `std::unordered_map<K, V>` / `std::unordered_multimap<K, V>` | Same `H5T_COMPOUND` compound, rank-1 dataspace | ✔ ok (insertion order not preserved) |

---

## 9. STL tuple / pair (scalar + container forms)

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `std::tuple<arith, arith, …>` (scalar; all arithmetic / trivially-copyable fields) | `H5T_COMPOUND { _0, _1, … }` with `tuple_layout` offsets, scalar dataspace (`composite` kind) | ✔ ok |
| `std::tuple<…, std::string, …>` (scalar; any non-trivial field) | `H5T_COMPOUND` with vlen-string slot at offset, but `tuple_layout::to_buffer` memcpys the std::string object — layout mismatch | ✘ fail (string fields land as garbage; documented in `examples/stl/README.md`) |
| `std::vector<std::tuple<…>>` (arithmetic fields) | rank-1 dataspace of `H5T_COMPOUND` (`linear_value_dataset`, composite element) | ✔ ok |
| `std::list<std::tuple<…>>` / `std::deque<…>` / `std::forward_list<…>` (arithmetic fields) | rank-1 dataspace of `H5T_COMPOUND` via iterator staging | ✔ ok |
| `std::set<std::tuple<…>>` / `std::multiset<…>` (arithmetic fields) | rank-1 dataspace of `H5T_COMPOUND` via iterator staging | ✔ ok |
| `std::pair<K, V>` (scalar) | `H5T_COMPOUND { first, second }` via `dt_t<pair>` + `offsetof`, scalar dataspace (`object` kind) | ✔ ok |
| `std::vector<std::pair<K, V>>` (trivially-copyable K, V) | rank-1 dataspace of `H5T_COMPOUND { first, second }` | ✔ ok |

---

## 10. STL nested containers

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `std::vector<std::vector<T>>` (ragged, T flat) | rank-1 dataspace of `H5T_VLEN dt_t<T>` (`ragged_vlen_dataset`) via `hvl_t` relay | ✔ ok |
| `std::vector<std::list<T>>` / `std::deque<T>` / `std::forward_list<T>` (T flat) | rank-1 dataspace of `H5T_VLEN dt_t<T>` via per-element scratch buffer + `hvl_t` relay | ✔ ok |
| `std::vector<std::set<T>>` / `std::multiset<T>` / `std::unordered_set<T>` (T flat) | Same ragged_vlen path | ✔ ok |
| `std::vector<std::array<T, N>>` (T non-char arithmetic) | rank-1 dataspace, `H5T_ARRAY[N] dt_t<T>` element (`array_dataset`) | ✔ ok |
| `std::vector<std::array<char, N>>` | rank-1 dataspace, `H5T_C_S1 + H5Tset_size(N)` element (`fls_dataset`) | ✔ ok |
| `std::list<std::array<T, N>>`, `std::deque<std::array<...>>`, `std::set<std::array<...>>`, etc. (T non-char) | rank-1 dataspace, `H5T_ARRAY[N] dt_t<T>` element (`array_dataset`, iterator-staged) | ✔ ok |
| Iterable outer container `Outer<std::array<char, N>>` | rank-1 dataspace, `H5T_C_S1 + H5Tset_size(N)` element (`fls_dataset`) | ✔ ok |
| `std::vector<std::list<std::list<T>>>` (3+ levels of nesting) | — | ◇ na (nested-container stopper) |
| `std::array<std::string, N>` / `std::array<std::vector<T>, N>` | — | ◇ na (array-of-container guard from `#274`) |

---

## 11. Walter Brown structural fallback (custom non-`std::` containers)

Detection idiom in `H5Tmeta.hpp` (`is_sequential_like`, `is_set_like`, `is_map_like`, `has_data_pointer`, `has_size`, `has_iterator`, `is_transport_contiguous_v`). Triggers for any user-defined container that exposes the relevant member typedefs and methods, and is NOT in `has_explicit_storage_repr<T>`.

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| Custom contiguous container with `.data() + .size() + value_type` (e.g. `absl::FixedArray<T>`, `folly::small_vector<T>`, `boost::container::vector<T>`, user-defined) | rank-1 dataspace of `dt_t<T>` via generic `access_traits_t` contiguous spec | ✔ ok (write); read works when the type has a `T(size_t)` ctor |
| Custom iterator-only sequence with `begin/end + value_type` (no `.data()`) | rank-1 dataspace via iterator-staging (`linear_value_dataset`) | ✔ ok (write); read assigns into a range-constructible target — generic non-`std::` reconstruction is best-effort today |
| Custom set-shape with `key_type + value_type + key_compare` (no `mapped_type`) | rank-1 dataspace via iterator-staging (`linear_value_dataset`) | ✔ ok (write); read inserts via `insert(iter, iter)` if available |
| Custom map-shape with `key_type + mapped_type + value_type` | rank-1 dataspace of `H5T_COMPOUND { key, value }` (`key_value_dataset`) | ✔ ok (write); read inserts pairs |
| `boost::container::flat_map`, `flat_set`, `ankerl::unordered_dense::map` | Same as their `std::` counterparts via structural detection | ✔ ok (untested in tree, expected by design) |

---

## 12. Smart pointers (added in this branch)

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `std::unique_ptr<T[]>` (with explicit `h5::count{...}`) | Forwards to raw-`T*` path; same as the underlying `T` array's HDF5 type | ✔ ok |
| `std::shared_ptr<T[]>` (with explicit `h5::count{...}`) | Same — forwards to raw-`T*` path | ✔ ok |
| `std::unique_ptr<T>` (single-element) | Synthetic `h5::count{1}`; same HDF5 type as `T` in a rank-1 size-1 dataset | ✔ ok |
| `std::shared_ptr<T>` (single-element) | Same as `unique_ptr<T>` (single) | ✔ ok |
| Auto-allocating return-style `h5::read<std::unique_ptr<T[]>>(fd, path)` | Sizes the unique_ptr to the dataset extent via `impl::get<>::ctor`; same HDF5 type as `T*` | ✔ ok |
| `std::unique_ptr<RegisteredPOD[]>` (compose with `H5CPP_REGISTER_STRUCT`) | rank-1 dataspace of `H5T_COMPOUND` elements | ✔ ok |

---

## 13. C++23 `std::mdspan` (gated on `__cpp_lib_mdspan`)

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `std::mdspan<T, std::extents<…>>` (compile-time static extents) | rank-N dataspace of `dt_t<T>` matching the extents | ✔ ok |
| `std::mdspan<T, std::dextents<…>>` (dynamic extents) | rank-N dataspace of `dt_t<T>` with runtime extents | ✔ ok |
| `std::mdspan` with non-default `LayoutPolicy` (e.g. `layout_left`, `layout_stride`) | Treated as layout_right (C-style row-major) | partial — non-default layouts coerced; documented in mapper |

---

## 14. Linear-algebra backends

### 14.1 Armadillo (gated on `ARMA_INCLUDES` / `H5CPP_USE_ARMADILLO`)

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `arma::Row<T>` (`arma::rowvec`) | rank-1 dataspace of N `dt_t<T>` | ✔ ok |
| `arma::Col<T>` (`arma::colvec` / `arma::vec`) | rank-1 dataspace of N `dt_t<T>` | ✔ ok |
| `arma::Mat<T>` (`arma::mat`) | rank-2 dataspace `(n_rows, n_cols)` of `dt_t<T>` (column-major on disk) | ✔ ok |
| `arma::Cube<T>` | rank-3 dataspace `(n_slices, n_cols, n_rows)` of `dt_t<T>` | ✔ ok |

### 14.2 Eigen3 (gated on `Eigen/Dense` include or `H5CPP_USE_EIGEN3`)

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `Eigen::Matrix<T, Dynamic, Dynamic, RowMajor>` | rank-2 dataspace `(rows, cols)` of `dt_t<T>` | ✔ ok |
| `Eigen::Matrix<T, Dynamic, Dynamic, ColMajor>` | rank-2 dataspace `(rows, cols)` of `dt_t<T>` | ✔ ok |
| `Eigen::Array<T, Dynamic, Dynamic, …>` | rank-2 dataspace `(rows, cols)` of `dt_t<T>` | ✔ ok |
| `Eigen::Matrix<T, Dynamic, 1, …>` (column vector) | rank-2 dataspace `(rows, 1)` of `dt_t<T>` | ✔ ok |
| Fixed-size Eigen matrices (`Eigen::Matrix<T, R, C>` with R,C compile-time) | — | ◇ na — pointer access path bypasses fixed-size; cast to `Dynamic` first |

### 14.3 Blaze (gated on `_BLAZE_MATH_MODULE_H_` or `_BLAZE_MATH_DYNAMICMATRIX_H_` or `H5CPP_USE_BLAZE`)

Compiled with `BLAZE_USE_PADDING=0` so `data()` exposes packed storage.

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `blaze::DynamicVector<T, rowVector>` (`blaze::rowvec`) | rank-1 dataspace of N `dt_t<T>` | ✔ ok |
| `blaze::DynamicVector<T, columnVector>` (`blaze::colvec`) | rank-1 dataspace of N `dt_t<T>` | ✔ ok |
| `blaze::DynamicMatrix<T, rowMajor>` (`blaze::rowmat`) | rank-2 dataspace `(rows, columns)` of `dt_t<T>` | ✔ ok |
| `blaze::DynamicMatrix<T, columnMajor>` (`blaze::colmat`) | rank-2 dataspace `(rows, columns)` of `dt_t<T>` | ✔ ok |

### 14.4 Blitz++ (gated on `BZ_NAMESPACE_START` or `H5CPP_USE_BLITZ`)

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `blitz::Array<T, 1>` | rank-1 dataspace of N `dt_t<T>` | ✔ ok |
| `blitz::Array<T, 2>` | rank-2 dataspace `(rows, cols)` of `dt_t<T>` | ✔ ok |
| `blitz::Array<T, 3>` | rank-3 dataspace `(extent_0, extent_1, extent_2)` of `dt_t<T>` | ✔ ok |
| `blitz::Array<T, N>` (N up to `H5CPP_MAX_RANK = 7`) | rank-N dataspace of `dt_t<T>` | ✔ ok |

### 14.5 Dlib (gated on `DLIB_MATRIx_HEADER` or `H5CPP_USE_DLIB`)

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `dlib::matrix<T>` (dynamic) | rank-2 dataspace `(nr, nc)` of `dt_t<T>` | ✔ ok |
| `dlib::matrix<T, R, C>` (compile-time fixed) | — | ◇ na — same constraint as Eigen fixed-size |

### 14.6 Boost.uBLAS (gated on `_BOOST_UBLAS_VECTOR_` / `_BOOST_UBLAS_MATRIX_` or `H5CPP_USE_UBLAS_VECTOR/MATRIX`)

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `boost::numeric::ublas::vector<T>` | rank-1 dataspace of N `dt_t<T>` | ✔ ok |
| `boost::numeric::ublas::matrix<T>` (row-major default) | rank-2 dataspace `(size1, size2)` of `dt_t<T>` | ✔ ok |

### 14.7 `std::valarray`

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `std::valarray<T>` | rank-1 dataspace of N `dt_t<T>` (`linear_value_dataset`) | ✔ ok |

### 14.8 xtensor / xtensor-blas (gated on `XTENSOR_ARRAY_HPP`, `XTENSOR_XARRAY_HPP`, `H5CPP_USE_XTENSOR`)

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `xt::xtensor<T, N>` (static rank) | rank-N dataspace of `dt_t<T>` | ✔ ok |
| `xt::xarray<T>` (dynamic rank) | rank-N dataspace of `dt_t<T>` (rank known at runtime) | ✘ fail — blocked on `traits::size` cross-tag conversion between `count_t` and `current_dims_t` (see `examples/linalg/README.md`) |
| `xt::xtensor_fixed<T, shape>` | — | ◇ na — fixed-shape, not exercised |
| xtensor-blas reductions (`xt::linalg::*` returning a `xt::xarray`) | Same as `xt::xarray<T>` | ✘ fail (same blocker) |

### 14.9 IT++ (vendored, currently disabled in CMake)

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| `itpp::Mat<T>`, `itpp::Vec<T>` | rank-2 / rank-1 dataspace of `dt_t<T>` | ◇ na — IT++ v4.3.1 isn't C++17-clean (uses removed `register` keyword); CMake target commented out |

---

## 15. Compiler-assisted Tier-2 — scatter/gather for compound w/ vlen fields

Tier-2 path for POD structs containing **non-POD fields** (`std::string`, `std::vector<double>`, etc.). The compiler emits a `scatter<T>` write specialization and a `gather<T>` read specialization that handle the per-field vlen serialisation. Routed via `h5::scatter(fd, path, ref)` / `h5::gather(fd, path, ref)`, OR — when the user calls `h5::write(fd, path, ref)` / `h5::read(fd, path, ref)` and `has_scatter<T>::value` — auto-dispatched through the fd-gateway. See `examples/compound/compound.cpp` (`sn::sensor::timeseries_t`).

| C++ source type | HDF5 type | Status |
| --- | --- | --- |
| POD struct with `std::string` field (compiler-registered) | `H5T_COMPOUND` with `H5T_C_S1 + H5T_VARIABLE` field per std::string; scatter writes per-record | ✔ ok (via `h5::scatter` / `h5::gather`) |
| POD struct with `std::vector<T>` field | `H5T_COMPOUND` with `H5T_VLEN dt_t<T>` field; scatter writes vlen blob per record | ✔ ok |
| POD struct with `[[h5::ignore]]` annotated field | Field omitted from the compound; not persisted (e.g. `timeseries_t::internal_id`) | ✔ ok |
| Calling `h5::write(ds, ref)` (ds-overload) on a scatter type | — | ◇ na — stopper fires: `scatter types must use h5::write(fd, path, ref)` to reach the generated specialization |
| Tier-2 type containing nested non-POD field (e.g. `std::vector<std::string>` field) | Per-field VLEN with inner vlen-string compound type | ✔ ok (compiler emits the nested wiring) |

---

## 16. Unsupported by design (compile-time stoppers fire correctly)

These are NOT bugs; they're the design boundary. The static_assert at `H5Dwrite.hpp` / `H5Dread.hpp` / `H5Awrite.hpp` / `H5Aread.hpp` names the offending type and routes the developer to the right path.

| C++ source type | Stopper message |
| --- | --- |
| `std::vector<bool>` | `storage_representation_v<T> resolved to 'unsupported'` (bit-packed; no contiguous `bool*`) |
| `std::array<std::string, N>` / `std::array<std::vector<T>, N>` | `storage_representation_v<T> resolved to 'unsupported'` (array-of-container guard from #274) |
| `std::vector<std::list<std::list<T>>>` / 3+-level nesting | `containers of containers are only supported for vector<string> (vlen_text_dataset) or vector<vector<T>> (ragged_vlen_dataset)` |
| `std::list<std::array<T, N>>` / `std::set<std::array<T, N>>` | Same nested-container stopper (only `vector<array<T,N>>` is wired) |
| Unregistered POD aggregate (no `H5CPP_REGISTER_STRUCT`) | `storage_representation_v<T> resolved to 'unsupported'. Check: unregistered POD aggregate (use H5CPP_REGISTER_STRUCT)` |
| Scatter type via `h5::write(ds, ref)` (ds-overload, not fd-gateway) | `scatter types must use h5::write(fd, path, ref) so the compiler-generated h5::scatter<T> specialization can dispatch` |
| Iterator-staging path with non-standard-layout element_t | `std::is_standard_layout_v<element_t>` static_assert |
| `wchar_t`, `char16_t`, `char32_t` strings | — (no path registered; resolves to `unsupported`) |
| `std::basic_string<signed char>` / `<unsigned char>` | — (not idiomatic; falls to `unsupported`) |

---

## 17. h5cpp-compiler reflection attributes

The Clang-based h5cpp compiler tool scans user headers and emits `generated.h` containing the `H5CPP_REGISTER_STRUCT(T)` bodies and (for Tier-2) the `scatter<T>` / `gather<T>` specialisations. Per-struct and per-field behaviour is steered by `[[h5::*]]` attributes on the source declaration. See `examples/reflection/` for end-to-end demos.

The attribute column distinguishes **struct-level** (applied to the type declaration) from **field-level** (applied to one data member); they're not interchangeable.

| Attribute | Scope | Effect on emitted code | Status |
| --- | --- | --- | --- |
| `[[h5::doc("text")]]` | struct | Documentation string propagated into `generated.h` as a top-of-block comment | ✔ ok |
| `[[h5::alias("name")]]` | struct | Logical / namespace name for the generated symbol — decouples the C++ identifier from the on-disk schema name | ✔ ok |
| `[[h5::version("x.y.z")]]` | struct | Schema-version metadata recorded next to the registration; useful for migration tooling | ✔ ok |
| `[[h5::name("disk_name")]]` | field | Renames one field on disk; overrides any `name_all` prefix/suffix | ✔ ok |
| `[[h5::name_all("prefix_", "_suffix")]]` | struct | Applies a prefix and/or suffix to every field's on-disk name uniformly | ✔ ok |
| `[[h5::ignore]]` | field | Omit the field from persistence entirely; reads zero-initialize it. Use for runtime-only state (cached handles, debug counters) | ✔ ok |
| `[[h5::chunk(N)]]` | struct | Default chunk size for the Tier-2 streaming dataset (each scatter call appends one row) | ✔ ok |
| `[[h5::compress("gzip", level)]]` | struct | Compression filter on the Tier-2 dataset's DCPL; level 0–9 | ✔ ok |
| `[[h5::on_missing("create")]]` | struct | When `h5::scatter` finds no dataset at the path, create it from the embedded schema | ✔ ok |
| `[[h5::on_missing("ignore")]]` | struct | When the dataset is missing, return early without throwing — quiet pre-flight pattern | ✔ ok |
| `[[h5::serialize_full]]` | struct | Force Tier-1 (POD-only) emission even when the struct has non-POD fields; non-POD fields are silently skipped. Useful when only the POD slice is meaningful on disk | ✔ ok |

Attribute combinations are independent: `[[h5::alias("dev"), h5::version("1.0.0"), h5::chunk(64), h5::compress("gzip", 6)]]` on a struct is valid and composes cleanly. Per-field `[[h5::name("...")]]` overrides any struct-level `name_all`.

### Tier-1 vs. Tier-2 emission

The compiler classifies each registered struct by inspecting its fields:

| If the struct contains… | Tier emitted | What `generated.h` looks like |
| --- | --- | --- |
| Only trivially-copyable / standard-layout fields (POD) | **Tier-1** | `register_struct<T>()` body with `H5Tinsert` per field. The dispatch uses `dt_t<T>` directly and writes/reads via the linear-value path. |
| At least one non-POD field (`std::string`, `std::vector<T>`, etc.) | **Tier-2** | `scatter<T>(fd, path, ref)` + `gather<T>(fd, path, ref)` specialisations. Each non-POD field is serialised through its dedicated vlen mechanism (vlen string, `hvl_t`, …). User code calls `h5::write(fd, path, ref)` / `h5::read(fd, path, ref)` and the gateway routes through scatter/gather. |
| Mixed POD + non-POD but `[[h5::serialize_full]]` present | **Tier-1 with silent skip** | Non-POD fields are dropped from the compound. Useful when the POD slice is the canonical on-disk shape and the non-POD fields are scratch. |

---

## 18. C++26 reflection — *pending*

C++26 (P2996 *Reflection for C++26* + P3394 *Annotations for Reflection*) makes the h5cpp-compiler tool optional: the same field enumeration and attribute reading move into the language as `std::meta::*` + `[[=value]]` annotation syntax.

| Mechanism | Today (C++17 / clang-based tool) | Under C++26 reflection |
| --- | --- | --- |
| Field enumeration | h5cpp-compiler AST walks the struct | `template for (constexpr auto m : std::meta::nonstatic_data_members_of(^T))` at compile time |
| Field type extraction | AST `clang::FieldDecl::getType()` | `std::meta::type_of(m)` |
| Field offset | `offsetof(T, name)` emitted into `generated.h` | `std::meta::offset_of(m)` |
| Field name | AST `getName()` emitted as `"name"` literal | `std::meta::name_of(m)` |
| Annotation value | h5cpp-compiler scans `[[h5::name("...")]]` from the AST | `std::meta::annotations_of(m, ^h5::name)` |
| Annotation syntax | `[[h5::name("label")]] float tag;` | `[[=h5::name{"label"}]] float tag;` (P3394 — the `=` marks an *annotation* expression) |
| Code emission | `H5CPP_REGISTER_STRUCT(T)` body written to `generated.h`, included after `<h5cpp/all>` | `dt_t<T>` and `scatter<T>` specialisations synthesised inline at the point of use; no `generated.h` file needed |
| Build step | CMake `GENERATED` line invokes the h5cpp compiler before C++ compilation | Single C++ compilation pass; the compiler **is** the C++ compiler |

The attribute **vocabulary** stays identical across both worlds — the migration is at the implementation layer, not the user-facing surface. A struct annotated for the C++17 tool also works under C++26 reflection once the library lands the `consteval` `dt_t<T>` generators.

| Status of each piece | Today | Notes |
| --- | --- | --- |
| `[[h5::*]]` attribute parser (clang tool) | ✔ ok — production today | `examples/reflection/` and every `generated.h` in tree |
| `std::meta::*` field walker | ◇ pending | Library has no C++26 dispatch path yet; awaits P2996 acceptance and a stable compiler implementation |
| `[[=h5::name{"..."}]]` P3394 annotation reader | ◇ pending | Depends on P3394 (annotations) advancing — currently in WG21 Evolution review |
| `consteval dt_t<T>` synthesis (compile-time `H5Tinsert` recording) | ◇ pending | Will replace the `register_struct<T>()` body emission once the standard reflection API is usable |
| Single-step C++26 build (no external compiler tool) | ◇ pending | The endpoint — once the above three land, `examples/reflection/` builds with zero external tooling |
| Backwards-compatibility shim | ◇ pending | Same `[[h5::*]]` attributes must keep working through the transition; the C++17 tool stays as the bootstrap path while early C++26 implementations are unstable |

When the C++26 path lands, the rows in §15 (Tier-2 scatter/gather) get a parallel "synthesised inline at use" alternative; nothing in this map changes semantically, only *where* the code lives. This section will be re-evaluated once a tier-1 compiler implementation (GCC 16+ / Clang 22+ / MSVC vNext) ships with usable P2996 + P3394 support.

---

## 19. Cross-references

- **`tasks/h5cpp-type-system-architecture-notes.md`** — full architectural rationale, the dispatch matrix, and the proposed `H5T_ARRAY` / FLS-element model that this map should converge to.
- **`examples/stl/README.md`** — runtime status table for all 32 STL paths with sample output.
- **`examples/linalg/README.md`** — per-backend linalg runtime status table.
- **`examples/smart-ptr/README.md`** — smart-pointer integration story.
- **`examples/reflection/README.md`** — every `[[h5::*]]` attribute exercised end-to-end with annotated source structs, Tier-1 / Tier-2 dispatch demos, and the C++26 migration sketch.
- **`examples/compound/`** — Tier-1 (POD-only) and Tier-2 (with vlen fields) compound demos.
- **`examples/csv/`**, **`examples/multi-tu/`**, **`examples/packet-table/`** — Tier-1 compound examples in different organisational shapes.
- **`h5cpp/H5Tall.hpp`** — `H5CPP_REGISTER_TYPE_` macro, primitive registrations, `dt_t<>` specialisations.
- **`h5cpp/H5Tmeta.hpp`** — `storage_representation_t` enum, `access_traits_t` family, structural-detection traits.
- **`h5cpp/H5Mmemory.hpp`**, `H5Mmemory_io.hpp` — smart-pointer mapper.
- **`h5cpp/H5M{arma,blaze,blitz,dlib,eigen,ublas,valarray,xtensor,mdspan}.hpp`** — per-backend linear-algebra mappers.
- **WG21 P2996** *Reflection for C++26* — language reflection API the §18 path depends on.
- **WG21 P3394** *Annotations for Reflection* — `[[=value]]` annotation syntax that pairs with P2996 for the C++26 migration.
