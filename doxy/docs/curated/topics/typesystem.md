@page curated_topics_typesystem TYPESYSTEM

@brief Full reference of h5cpp's C++ ↔ HDF5 type mapping —
elementary scalars, strings, compounds, containers, the
`storage_representation_t` taxonomy, and the trait machinery that
drives the dispatch.

# The h5cpp type system

H5CPP's job is to take any C++ type and figure out the right
HDF5 on-disk shape automatically — no manual `H5Tcreate` /
`H5Tinsert` calls, no per-field offset arithmetic. The mechanism
is a three-layer mapping:

```
       C++ type T (arbitrary user type)
              │
              ▼
   ┌──────────────────────────────────┐
   │  Layer 1 — recognition           │  Walter Brown feature detection +
   │  meta::access_traits_t<T>        │  static specialisations for known types
   └──────────┬───────────────────────┘
              │
              ▼
   ┌──────────────────────────────────┐
   │  Layer 2 — classification        │  storage_representation_t enum picks
   │  storage_representation_v<T>     │  one of ~10 dispatch slots
   └──────────┬───────────────────────┘
              │
              ▼
   ┌──────────────────────────────────┐
   │  Layer 3 — HDF5 type emission    │  dt_t<T> produces the actual
   │  dt_t<T> + resolved_type_t<T>    │  H5T_NATIVE_* / H5T_COMPOUND / H5T_VLEN / …
   └──────────────────────────────────┘
                                          → fed into H5Dwrite / H5Dread / H5Acreate
```

Every public call site (`h5::write`, `h5::read<T>`, `h5::create<T>`,
`h5::awrite`, `h5::aread<T>`) consults this chain at **compile
time** via `if constexpr` / SFINAE. The generated assembly is the
same as if you'd hand-written the right `H5Dwrite` call.

## Layer 1 — Elementary types

The leaf of every compound, container, or array — what HDF5 calls
*atomic* types. h5cpp ships a `dt_t<T>` specialisation per
elementary C++ type that returns the matching HDF5 native type id.

### Integer types

| C++ type           | HDF5 native type      | Width   | Notes                                  |
| :----------------- | :-------------------- | :------ | :------------------------------------- |
| `signed char`      | `H5T_NATIVE_SCHAR`    | 8 bit   | Treated as integer, not text           |
| `unsigned char`    | `H5T_NATIVE_UCHAR`    | 8 bit   | "                                      |
| `char`             | `H5T_NATIVE_CHAR`     | 8 bit   | Sign depends on platform — h5cpp pins it via traits |
| `int8_t`           | `H5T_STD_I8LE`        | 8 bit   | Endian-pinned form (little-endian)     |
| `int16_t`          | `H5T_STD_I16LE`       | 16 bit  | "                                      |
| `int32_t`          | `H5T_STD_I32LE`       | 32 bit  | "                                      |
| `int64_t`          | `H5T_STD_I64LE`       | 64 bit  | "                                      |
| `uint8_t`          | `H5T_STD_U8LE`        | 8 bit   | "                                      |
| `uint16_t`         | `H5T_STD_U16LE`       | 16 bit  | "                                      |
| `uint32_t`         | `H5T_STD_U32LE`       | 32 bit  | "                                      |
| `uint64_t`         | `H5T_STD_U64LE`       | 64 bit  | "                                      |
| `short` / `int` / `long` / `long long` | `H5T_NATIVE_SHORT` / `H5T_NATIVE_INT` / `H5T_NATIVE_LONG` / `H5T_NATIVE_LLONG` | platform-dep | h5cpp uses the *native* form here; on-disk width follows the platform |

For portability **always prefer the `intN_t` / `uintN_t` aliases**
— they pin the width and endianness on disk, so a file written on
x86_64 reads correctly on ARM big-endian and vice-versa.

### Floating-point types

| C++ type           | HDF5 native type      | Width   | Notes                                  |
| :----------------- | :-------------------- | :------ | :------------------------------------- |
| `float`            | `H5T_IEEE_F32LE`      | 32 bit  | IEEE 754 binary32, little-endian       |
| `double`           | `H5T_IEEE_F64LE`      | 64 bit  | IEEE 754 binary64                      |
| `long double`      | `H5T_NATIVE_LDOUBLE`  | 80–128 bit | Width is platform-dependent — file may not round-trip across systems |
| `half` (custom)    | h5cpp half-float type | 16 bit  | IEEE 754 binary16 via `examples/half-float` — opt-in |

### Boolean and enums

| C++ type                    | HDF5 type                | Notes                                          |
| :-------------------------- | :----------------------- | :--------------------------------------------- |
| `bool`                      | 8-bit enum {false, true} | Persisted as a 2-value enum, not as 1-bit     |
| `enum class E : T` (scoped) | `H5T_ENUM` over `T`      | Each named value becomes an enum member        |
| Unscoped `enum`             | Treated as the underlying integer | (No `H5T_ENUM` introspection — use `enum class`) |

### Complex

| C++ type            | HDF5 type                                                     |
| :------------------ | :------------------------------------------------------------ |
| `std::complex<T>`   | `H5T_COMPOUND { T real; T imag; }` — 2-field compound         |

## Layer 2 — `storage_representation_t` taxonomy

Every type that h5cpp recognises resolves at compile time to one of
ten values in the `storage_representation_t` enum (defined in
`h5cpp/H5Tmeta.hpp`). The value picks which dispatch branch in
`H5Dwrite.hpp` / `H5Dread.hpp` / `H5Awrite.hpp` runs.

| Enum value                       | Triggers for…                                              | HDF5 on-disk shape                                              |
| :------------------------------- | :--------------------------------------------------------- | :-------------------------------------------------------------- |
| `unsupported`                    | Anything no trait recognises                               | Hard `static_assert` — never reaches I/O                        |
| `scalar`                         | Single arithmetic value, `std::pair`, `std::tuple`, `std::complex`, registered POD | Scalar dataspace + native or `H5T_COMPOUND`             |
| `c_array`                        | (Legacy/internal — see `array_element` for the user-facing path) | —                                                          |
| `linear_value_dataset`           | `std::vector<T>` / `std::deque<T>` / `std::list<T>` / `std::forward_list<T>` / `std::valarray<T>` / `std::set<T>` / dense linalg containers | rank-1 dataspace of `T`'s native type                  |
| `key_value_dataset`              | `std::map<K, V>` / `std::multimap` / `unordered_*` variants / `std::vector<std::pair<K, V>>` | rank-1 of `H5T_COMPOUND { K key; V value; }`     |
| `ragged_vlen_dataset`            | `std::vector<std::vector<T>>`                              | rank-1 of `H5T_VLEN { T }` (hvl_t relay)                        |
| `fixed_inner_extent_dataset`     | `std::vector<std::array<T, N>>` with fixed inner extent     | (canonical mapping — see `array_dataset` below)                 |
| `vlen_text_dataset`              | `std::string` (scalar), `std::vector<std::string>`         | scalar or rank-1 of `H5T_C_S1, H5T_VARIABLE`                    |
| `fixed_length_string`            | `char[N]` / `std::array<char, N>` (top-level)              | scalar `H5T_C_S1` with `H5Tset_size(N)`                         |
| `array_element`                  | `std::array<T, N>` / `T[N]` (top-level, non-`char`)        | scalar dataspace + `H5T_ARRAY[N]` of `T`                        |
| `array_dataset`                  | `std::vector<std::array<T, N>>` (non-`char`)               | rank-1 of `H5T_ARRAY[N]` of `T`                                 |
| `fls_dataset`                    | `std::vector<std::array<char, N>>`                          | rank-1 of `H5T_C_S1 + H5Tset_size(N)` — flat bytes, no VLEN     |

The full enum (`h5cpp/H5Tmeta.hpp` line 164) is the single source of
truth for what shapes h5cpp speaks.

## Layer 3 — Compound types

When `T` is a `pair`, `tuple`, `complex`, or a registered struct,
h5cpp emits an HDF5 `H5T_COMPOUND` whose fields are laid out by
`offsetof(T, member)`. For each field, the dispatch recurses:
field type → its own `dt_t<F>` → its native or compound HDF5 type.

### Standard composites

| C++ type                                | HDF5 compound layout                                              |
| :-------------------------------------- | :---------------------------------------------------------------- |
| `std::pair<K, V>`                       | `H5T_COMPOUND { K first; V second; }`                             |
| `std::tuple<T0, T1, ..., Tn>`           | `H5T_COMPOUND { T0 _0; T1 _1; ...; Tn _n; }` (positional names)   |
| `std::complex<T>`                       | `H5T_COMPOUND { T real; T imag; }`                                |

### User-defined POD structs — via macro

```cpp
struct sample {
    int      ts;
    double   value;
    uint32_t flags;
};
H5CPP_REGISTER_STRUCT(sample);        // expands to dt_t<sample> specialisation
```

After registration, `sample` is treated like any built-in type
across the API:

```cpp
std::vector<sample> data = collect();
h5::write(fd, "/samples", data);
auto round = h5::read<std::vector<sample>>(fd, "/samples");
```

On-disk shape: rank-1 of `H5T_COMPOUND { int ts; double value;
uint32_t flags; }` with field offsets computed via `offsetof`.

POD-eligible means trivially-copyable + standard-layout + no
vlen-bearing fields. See @ref curated_topics_reflection for the
non-POD path (h5cpp-compiler) and the C++26 reflection roadmap.

### Nested compounds

Compounds can contain other compounds. Walking is recursive:

```cpp
struct sample { int ts; double value; };
struct chunk  { sample s; uint32_t flags; };

H5CPP_REGISTER_STRUCT(sample);
H5CPP_REGISTER_STRUCT(chunk);    // chunk.s resolves to the registered sample type

h5::write(fd, "/chunks", std::vector<chunk>(N));
// On disk: H5T_COMPOUND { H5T_COMPOUND { int ts; double value; } s;
//                        uint32_t flags; }
```

## Container shapes — the complete taxonomy

| User code                              | `storage_representation_t`    | HDF5 dataspace        | HDF5 element type            |
| :------------------------------------- | :---------------------------- | :-------------------- | :--------------------------- |
| `T` (arithmetic / pair / tuple / POD)  | `scalar`                      | scalar                | native or compound           |
| `std::vector<T>`                       | `linear_value_dataset`        | rank-1, length N      | native(T)                    |
| `std::deque<T>` / `list<T>` / `set<T>` | `linear_value_dataset`        | rank-1, length N      | native(T) — staged via vector |
| `std::array<T, N>` (non-char)          | `array_element`               | scalar                | `H5T_ARRAY[N]` of native(T)  |
| `std::vector<std::array<T, N>>` (non-char) | `array_dataset`           | rank-1, length M      | `H5T_ARRAY[N]` of native(T)  |
| `std::vector<std::vector<T>>`          | `ragged_vlen_dataset`         | rank-1, length M      | `H5T_VLEN { T }` (hvl_t)     |
| `std::string`                          | `vlen_text_dataset` (scalar)  | scalar                | `H5T_C_S1, H5T_VARIABLE`     |
| `std::vector<std::string>`             | `vlen_text_dataset`           | rank-1, length M      | `H5T_C_S1, H5T_VARIABLE`     |
| `char[N]` / `std::array<char, N>`      | `fixed_length_string`         | scalar                | `H5T_C_S1 + H5Tset_size(N)`  |
| `std::vector<std::array<char, N>>`     | `fls_dataset`                 | rank-1, length M      | `H5T_C_S1 + H5Tset_size(N)`  |
| `std::map<K, V>`                       | `key_value_dataset`           | rank-1, length entries | `H5T_COMPOUND { K key; V value; }` |
| `arma::Mat<T>` / `Eigen::Matrix<T>`    | `linear_value_dataset`        | rank-2                | native(T) — see LINEAR ALGEBRA |
| `arma::SpMat<T>` / `Eigen::SparseMatrix<T>` | `is_sparse_v<T>` route    | **CSC group**         | (see LINEAR ALGEBRA § sparse) |

## How dispatch picks a path — the trait machinery

The dispatch is built on **Walter E. Brown's feature detection**
(detection idiom + `std::void_t`). Each candidate type is probed
structurally:

| Question asked                                       | Detector                       | Picks…                                              |
| :--------------------------------------------------- | :----------------------------- | :-------------------------------------------------- |
| Does `T` have a `value_type` member typedef?         | `has_value_type<T>`            | Containers / strings / smart pointers               |
| Does `T` have a `data()` method?                     | `has_data_method<T>`           | Contiguous containers → `linear_value_dataset`      |
| Does `T` have `begin()` + `end()` but no `data()`?   | `is_iterator_only<T>`          | `list` / `set` / `map` → iter-staging path          |
| Is `T` a `tuple_size`-specialised type?              | `has_tuple_size<T>`            | `std::tuple` / `std::pair` → compound               |
| Is `T::value_type` itself container-like?            | (composition of above)         | `vector<vector<T>>` / `vector<string>` → vlen        |
| Does `is_sparse<T>` specialise to `true_type`?       | (per-library mapper opts in)   | Sparse → CSC group path                             |

The full trait set lives in `h5cpp/H5Tmeta.hpp`. Walter Brown's
contribution is the *mechanism* — `std::void_t` + SFINAE detectors;
h5cpp's contribution is the *vocabulary* — which questions to ask
to fully classify a C++ type for HDF5 storage.

See @ref curated_topics_stl for the worked C++ code of the
detectors and the dispatch composition.

## Extending the type system

Three doors for getting your own types into the dispatch:

| Mechanism | When to use | Cost |
| :-------- | :---------- | :--- |
| `H5CPP_REGISTER_STRUCT(T)` | POD struct, no vlen fields | one macro per type |
| h5cpp-compiler (Clang AST) | Non-POD with vector / string / map fields | pre-build step |
| C++26 reflection (future) | Any type, no external tools | C++26 compiler |
| Add `dt_t<MyType>` specialisation | One-off custom integer / opaque type | hand-written ~10 lines |
| Add `meta::storage_representation_impl<MyContainer>` + `access_traits_t<MyContainer>` | Custom container library (write your own h5cpp mapper) | ~30–50 lines, mirrors `h5cpp/H5Marma.hpp` |

The first three cover the common cases. The last two are for type
authors writing a new mapper (or porting a custom container into
the h5cpp dispatch).

## Where to go next

- @ref link_base_template_types — comprehensive Supported Types
  reference (object-axis view; this page is type-system-axis)
- @ref curated_topics_stl — STL coverage + Walter Brown feature
  detection in working C++ code
- @ref curated_topics_linalg — linalg dispatch (uses the same
  `linear_value_dataset` path as `std::vector`)
- @ref curated_topics_reflection — extending the type system for
  user-defined types via macro, h5cpp-compiler, or C++26 reflection
- @ref reports_type_system_map — the design
  rationale + the bootstrap problem (`H5CPP_BUILDING_TYPE_INFO`)
- @ref reports_type_system_map — exhaustive map of every
  storage_representation case with worked examples
