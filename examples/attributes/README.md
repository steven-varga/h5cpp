@page example_guide_attributes Attributes — Metadata on Groups and Datasets

HDF5 attributes are small metadata values attached to groups, datasets, or named datatypes. They live in the object header, not as standalone datasets — no chunking, no compression, no partial I/O, no streaming. Each attribute is read and written in a single shot.

Use attributes for things that describe the data, not the data itself:

| Use case | Example |
|----------|---------|
| Units and labels | `"units" = "m/s"`, `"axis_label" = "time"` |
| Provenance | `"source" = "sensor-7"`, `"acquired_at" = "2026-05-25T20:31:06Z"` |
| Calibration | `"gain" = 1.02`, `"offset" = -0.003` |
| Schema version | `"schema_version" = "2.1.0"` |
| Small lookup tables | A compact vector of thresholds or category names |

If the value is large, changes over time, or needs slicing, use a dataset instead.

## Two APIs

h5cpp exposes two write surfaces for attributes; both accept the same set of types and dispatch through the same backend:

```cpp
// Compact style — looks like a map
ds["temperature_units"] = "celsius";
auto t = h5::aread<double>(ds, "temperature");

// Explicit style — clearer in complex code
h5::awrite(ds, "temperature_units", "celsius");
auto t = h5::aread<double>(ds, "temperature");
```

`h5::aread<T>(parent, name)` is **parent-generic**: `parent` may be an `h5::ds_t`, `h5::gr_t`, or any other handle that satisfies the `is_valid_attr<P>` SFINAE constraint. Section 6 below exercises the group-as-parent path.

## Files

| File | Purpose |
|---|---|
| `attributes.cpp` | One TU exercising every attribute kind the dispatch supports |
| `struct.h` | POD test struct: `sn::example::record_t` |
| `generated.h` | Compiler-emitted compound descriptor for `record_t`, committed in tree |

`generated.h` is **not regenerated** in this target's CMake — the h5cpp-compiler tool currently segfaults on the canonical-mapping type mix (`std::complex`, `std::array<char,N>`, etc. appearing in the same TU). The committed file covers the only registration the example needs (`sn::example::record_t`); the rest of the types resolve through the runtime dispatch directly.

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-attributes
./examples-attributes
```

## Coverage

The example is organised into eight numbered sections, each printing a `=== ... ===` banner on stdout. Every line is a real round-trip — the value is written, read back via `h5::aread<T>`, and compared with `==` or a library-specific `same()` helper (e.g. `arma::approx_equal` for `arma::mat`). 27 distinct verifications run end-to-end.

| Section | Group | Verifications |
|---|---|---|
| 1 | Linear-algebra backends | 7 |
| 2 | Canonical fixed-extent (Winston model) | 5 |
| 3 | Scalar object kinds (`complex`, `pair`, `tuple`) | 3 |
| 4 | Compound POD (`record_t` scalar + vector) | 2 |
| 5 | STL strings and containers | 7 |
| 6 | Explicit `h5::awrite` on a group target | 1 |
| 7 | Pretty-print readback (no asserts; 10 rendered lines) | — |
| 8 | Rewrite (same type/shape) + type-mismatch (must throw) | 2 |

### 1. Linear-algebra backends (7)

Seven libraries write a 3×4 matrix (or a length-5 vector for `std::valarray`) as an attribute on the host dataset:

| Library | Type | Mapper |
|---|---|---|
| Armadillo | `arma::mat` | `H5Marma.hpp` |
| Eigen3 | `Eigen::MatrixXd` | `H5Meigen.hpp` |
| Blitz++ | `blitz::Array<double, 2>` | `H5Mblitz.hpp` |
| dlib | `dlib::matrix<double>` | `H5Mdlib.hpp` |
| Boost.uBLAS | `boost::numeric::ublas::matrix<double>` | `H5Mublas.hpp` |
| xtensor | `xt::xarray<double>` | `H5Mxtensor.hpp` |
| `std::valarray` | `std::valarray<double>` | `H5Mvalarray.hpp` |

**Blaze is excluded by design.** Blaze ships its own LAPACK Fortran prototypes, which clash with Armadillo's when both headers land in the same TU. The Blaze attribute surface is exercised in isolation by `examples/linalg/blaze.cpp`.

### 2. Canonical fixed-extent attributes (Winston model)

Five canonical mappings cover fixed-extent storage. Storage-tag names match `tasks/h5cpp-type-system-map.md`:

| C++ source type | HDF5 datatype | Storage tag |
|---|---|---|
| `char[N]` | `H5T_C_S1` + `H5Tset_size(N)` | `fixed_length_string` |
| `std::array<char, N>` | `H5T_C_S1` + `H5Tset_size(N)` | `fixed_length_string` |
| `std::array<T, N>` (non-char) | `H5T_ARRAY[N]` of `dt_t<T>` | `array_element` (scalar) |
| `std::vector<std::array<T, N>>` | rank-1 of `H5T_ARRAY[N]` | `array_dataset` |
| `std::vector<std::array<char, N>>` | rank-1 of `H5T_C_S1+size(N)` | `fls_dataset` |

### 3. Scalar object kinds

`std::complex<double>`, `std::pair<int, double>`, and `std::tuple<int, double, char>` are written as scalar attributes. Each lands in the compound/scalar path and survives `==` comparison on readback.

### 4. Compound POD

`sn::example::record_t` is written both as a scalar attribute (`ds["pod"] = rec`) and as a rank-1 vector (`ds["pods"] = records`, length 8). The comparison checks the `idx` field, sufficient to detect a field-offset drift.

### 5. STL strings and containers

Seven container shapes, each round-tripped and compared:

| Type | Storage tag |
|---|---|
| `std::string` (scalar) | `vlen_text_dataset` |
| `std::vector<std::string>` | `vlen_text_dataset` |
| `std::vector<std::vector<double>>` (ragged) | `ragged_vlen_dataset` |
| `std::list<int>` | `linear_value_dataset` (iterators) |
| `std::set<int>` | `linear_value_dataset` (iterators) |
| `std::map<int, double>` | `key_value_dataset` |
| `std::vector<std::tuple<int, float>>` | `linear_value_dataset` (composite element) |

### 6. Explicit `h5::awrite` on a group target

```cpp
h5::gr_t gr = h5::gcreate(fd, "/group");

h5::awrite(gr, "int",    42);
h5::awrite(gr, "double", 3.14);
h5::awrite(gr, "str",    std::string("via h5::awrite"));
h5::awrite(gr, "arma",   host);
```

All four read back with `h5::aread<T>(gr, name)`. The parent type is a `gr_t`, not a `ds_t` — `aread` accepts any handle satisfying `is_valid_attr<P>`.

### 7. Pretty-print readback

Ten reads are forwarded straight to `std::cout`, using the `operator<<` overloads from `H5Uall.hpp`. No assertion — visual confirmation that the STL pretty-printer reaches the attribute reads.

### 8. Rewrite + type-mismatch

```cpp
ds["scalar_int"] = 42;       // create
ds["scalar_int"] = 99;       // rewrite, same type → OK
```

Rewriting with a different HDF5 type throws `h5::error::io::attribute::any`:

```cpp
h5::awrite(ds, "scalar_int", std::string("not an int"));  // throws
```

The example wraps the throwing call in `h5::mute()` / `h5::unmute()` so the expected diagnostic does not pollute stdout.

## Sample Output

```
=== linear-algebra attributes ===
✔ ok    arma::mat(3x4)               attribute
✔ ok    Eigen::MatrixXd(3x4)         attribute
✔ ok    blitz::Array<double,2>(3x4)  attribute
✔ ok    dlib::matrix<double>(3x4)    attribute
✔ ok    ublas::matrix<double>(3x4)   attribute
✔ ok    xt::xarray<double>(3x4)      attribute
✔ ok    std::valarray<double>(5)     attribute

=== canonical fixed-extent attributes ===
✔ ok    char[16]                     attribute (FLS)
✔ ok    std::array<char,16>          attribute (FLS)
✔ ok    std::array<int,5>            attribute (H5T_ARRAY)
✔ ok    std::vector<std::array<int,3>>(3) attribute (rank-1 of H5T_ARRAY)
✔ ok    std::vector<std::array<char,8>>(2) attribute (rank-1 of FLS)

=== pretty-print readback ===
  vec_s    = [alpha,beta,gamma,delta]
  ragged   = [[1],[2,3],[4,5,6]]
  list     = [10,20,30,40]
  set      = [1,2,3,4,5]
  map      = [{1:1.1},{2:2.2},{3:3.3}]
  vtups    = [<1,1.1>,<2,2.2>,<3,3.3>]
  tuple    = <42,3.14,x>
  pair     = {42:3.14}
  ai       = [10,20,30,40,50]
  v_ai     = [[1,2,3],[4,5,6],[7,8,9]]
```

## Supported Types

| Category | Types | Storage tag |
|---|---|---|
| Scalar arithmetic | `int`, `double`, `float`, `short`, `long long`, ... | native scalar |
| C strings | `"literal"`, `const char*` | `vlen_text_dataset` |
| C++ strings | `std::string`, `std::string_view` | `vlen_text_dataset` |
| Fixed-length C strings | `char[N]` | `H5T_C_S1` + `H5Tset_size(N)` — `fixed_length_string` |
| Fixed-length char arrays | `std::array<char, N>` | `H5T_C_S1` + `H5Tset_size(N)` — `fixed_length_string` |
| Fixed-length array (scalar) | `std::array<T, N>` (non-char) | `array_element` (`H5T_ARRAY`) |
| Vector of fixed array | `std::vector<std::array<T, N>>` | `array_dataset` (rank-1) |
| Vector of fixed char array | `std::vector<std::array<char, N>>` | `fls_dataset` |
| Vectors | `std::vector<T>` (scalar / string / compound / tuple element) | `linear_value_dataset` / `vlen_text_dataset` |
| Ragged | `std::vector<std::vector<T>>` | `ragged_vlen_dataset` |
| Maps and sets | `std::map<K,V>`, `std::set<T>`, `std::unordered_map<K,V>` | `key_value_dataset` / `linear_value_dataset` |
| Scalar objects | `std::complex<T>`, `std::pair<K,V>`, `std::tuple<Ts...>` | compound scalar |
| Vector of tuples | `std::vector<std::tuple<Ts...>>` | `linear_value_dataset` (composite element) |
| POD compounds | Tier-1 structs with `H5CPP_REGISTER_STRUCT` or compiler-emitted descriptor | `H5T_COMPOUND` |
| Linear algebra | Armadillo, Eigen3, Blitz++, dlib, uBLAS, xtensor, `std::valarray` | per-mapper |
| Unsupported | Tier-2 structs with `std::string` / `std::vector<T>` fields | (see below) |

The attribute dispatch has parity with the dataset dispatch: `H5Awrite.hpp` and `H5Aread.hpp` now carry the canonical `fixed_length_string`, `array_element`, `array_dataset`, and `fls_dataset` branches, alongside the vlen-text / key-value / ragged-vlen branches that were already there. If `h5::write` accepts `T` as a dataset element, `h5::awrite` accepts it as an attribute value — with the one exception below.

### Why tier-2 structs are excluded

Compiler-reflected structs with `std::string` or `std::vector<T>` fields (tier-2) require `h5::scatter<T>` / `h5::gather<T>`. That machinery needs:

- a dataset path to open or create;
- chunked, extendable dataset creation (`H5Pset_chunk`, `H5S_UNLIMITED`);
- row-by-row append logic (`h5::detail::write_one_row`);
- `hvl_t` relay objects per field;
- HDF5 memory reclamation (`H5Treclaim`).

Attributes are single-shot metadata stored in object headers — no dataset, no chunks, no rows, no append semantics. The tier-2 path has nowhere to run, so `h5::awrite` rejects these types at compile time:

```cpp
struct event_t {
    unsigned long long timestamp_ns;
    std::string source_id;           // makes it tier-2
    std::vector<double> readings;    // makes it tier-2
};

ds["event"] = event_t{...};        // static_assert: unsupported storage
```

If you need to persist a tier-2 struct, call `h5::scatter(fd, "/metadata/event", event)` against a dataset path. If only the POD fields matter, mark the struct with `[[h5::serialize_full]]` and the compiler emits a tier-1 compound that skips the non-POD members.

## v2 Object Headers — Load-Bearing Here

```cpp
h5::fd_t fd = h5::create("attributes.h5", H5F_ACC_TRUNC, h5::default_fcpl,
    h5::libver_bounds({H5F_LIBVER_V18, H5F_LIBVER_V18}));
```

This is not decorative. The demo packs ~25 attributes (matrices, compounds, containers) onto the single host dataset. The default v1 object-header caps total attribute size at 64 KB; the cumulative payload here overflows that ceiling. `H5F_LIBVER_V18` enables the v2 object header, which has no such cap. Removing the `libver_bounds` line breaks the example.

## CMake Wiring

```cmake
add_h5cpp_example(attributes attributes/attributes.cpp
    LIBRARIES libarmadillo Eigen3::Eigen libblaze libblitz libdlib
              libublas libxtensor xtl::xtl
    COMPILER_INCLUDES ...)
```

No `GENERATED` line — `generated.h` is committed alongside `struct.h`. The h5cpp-compiler tool currently segfaults on the canonical-mapping type mix this example pulls in (`std::complex` / `std::array<char,N>` co-occurring with the linalg backends); the committed `generated.h` carries the `sn::example::record_t` registration the runtime path needs.

## Build State

| Target | Status |
|---|---|
| `examples-attributes` | ✔ ok — 27 attribute round-trips verified across linalg / canonical / scalar / compound / STL / explicit-awrite / rewrite groups |

## Cross-References

- `examples/compound/` — the POD struct definitions used here as attribute values.
- `examples/linalg/` — the per-library linalg coverage in isolated TUs; Blaze is exercised there.
- `examples/stl/` — full STL container dispatch matrix on the dataset side.
- `examples/datasets/` — when your metadata outgrows the attribute payload model, switch to a dataset.
- `tasks/h5cpp-type-system-architecture-notes.md` — write-side / read-side type matrices.
- `tasks/h5cpp-type-system-map.md` — canonical storage-tag names referenced in the tables above.
- `h5cpp/H5Awrite.hpp` / `h5cpp/H5Aread.hpp` — attribute dispatch implementation.
- `h5cpp/H5Uall.hpp` — `operator<<` overloads driving section 7's pretty-print readback.

## Source

- [`attributes.cpp`](attributes_8cpp-example.html) — rendered with syntax highlighting
- [`generated.h`](generated_8h-example.html) — rendered with syntax highlighting
- [`struct.h`](struct_8h-example.html) — rendered with syntax highlighting
- [`utils.hpp`](utils_8hpp-example.html) — rendered with syntax highlighting
