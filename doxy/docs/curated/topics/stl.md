@page curated_topics_stl STL

@brief Standard-library container, string, tuple, smart-pointer and
view support — what `T` you can pass to `h5::read<T>` / `h5::write` /
`h5::create<T>` and how each maps to an HDF5 datatype.

# STL types in h5cpp

Every standard-library type that has a sensible HDF5 representation
is a first-class template argument across the H5CPP API. No opt-in
header, no registration — they're recognised by the same compile-time
dispatch that handles linear-algebra containers and registered
compound PODs.

The dispatch is built on **Walter E. Brown's compile-time feature
detection** (CppCon 2014, *"Modern Template Metaprogramming: A
Compendium"*) — h5cpp asks "does `T` have a `.data()` method? a
`.size()` method? a nested `value_type` typedef? a
`std::tuple_size` specialisation?" and routes accordingly. No `T`
is special-cased by name; everything goes through the same trait
machinery.

## Full support matrix

Each row maps a C++ type to its `storage_representation_t` enum
value (defined in `h5cpp/H5Tmeta.hpp`) and the resulting HDF5
on-disk shape.

### Sequence containers

| C++ type                                | `storage_representation_t`      | HDF5 on-disk shape                                                |
| :-------------------------------------- | :------------------------------ | :---------------------------------------------------------------- |
| `std::vector<T>`                        | `linear_value_dataset`          | rank-1 of `T`'s native HDF5 type                                  |
| `std::deque<T>`                         | `linear_value_dataset`          | rank-1 of `T`'s native HDF5 type (staged through vector)          |
| `std::list<T>`                          | `linear_value_dataset`          | rank-1 of `T`'s native HDF5 type (staged through vector)          |
| `std::forward_list<T>`                  | `linear_value_dataset`          | rank-1 of `T`'s native HDF5 type (staged through vector)          |
| `std::valarray<T>`                      | `linear_value_dataset`          | rank-1 of `T`'s native HDF5 type                                  |
| `std::array<T, N>` (non-`char` T)       | `array_element`                 | scalar dataspace with `H5T_ARRAY[N]` element type                 |
| `T[N]` (C array, non-`char` T)          | `array_element`                 | scalar dataspace with `H5T_ARRAY[N]` element type                 |

### Associative containers

| C++ type                                | `storage_representation_t`      | HDF5 on-disk shape                                                |
| :-------------------------------------- | :------------------------------ | :---------------------------------------------------------------- |
| `std::set<T>` / `std::multiset<T>`      | `linear_value_dataset`          | rank-1 of `T` (iter-staging through vector buffer)                |
| `std::unordered_set<T>` / `multiset`    | `linear_value_dataset`          | rank-1 of `T`                                                     |
| `std::map<K, V>` / `multimap`           | `key_value_dataset`             | rank-1 of `H5T_COMPOUND { K key; V value; }`                      |
| `std::unordered_map<K, V>` / `multimap` | `key_value_dataset`             | rank-1 of `H5T_COMPOUND { K key; V value; }`                      |

### Tuples and pairs

| C++ type                                | `storage_representation_t`      | HDF5 on-disk shape                                                |
| :-------------------------------------- | :------------------------------ | :---------------------------------------------------------------- |
| `std::pair<K, V>` (top-level)           | `scalar`                        | scalar dataspace, `H5T_COMPOUND { K first; V second; }`           |
| `std::tuple<Ts...>` (top-level)         | `scalar`                        | scalar dataspace, `H5T_COMPOUND { Ts...; }`                       |
| `std::vector<std::pair<K, V>>`          | `key_value_dataset`             | rank-1 of `H5T_COMPOUND { K first; V second; }`                   |
| `std::vector<std::tuple<Ts...>>`        | `linear_value_dataset` (composite element) | rank-1 of `H5T_COMPOUND { Ts...; }`                    |

### Strings

| C++ type                                | `storage_representation_t`      | HDF5 on-disk shape                                                |
| :-------------------------------------- | :------------------------------ | :---------------------------------------------------------------- |
| `std::string` / `std::string_view`      | `vlen_text_dataset` (scalar form) | scalar dataspace with `H5T_C_S1, H5T_VARIABLE`                  |
| `char*` / `const char*` (top-level)     | `vlen_text_dataset`             | scalar `H5T_C_S1, H5T_VARIABLE`                                   |
| `char[N]` / `std::array<char, N>`       | `fixed_length_string`           | scalar `H5T_C_S1` with `H5Tset_size(N)`                           |
| `std::vector<std::string>`              | `vlen_text_dataset`             | rank-1 of `H5T_C_S1, H5T_VARIABLE` (VLEN string per element)      |
| `std::vector<std::array<char, N>>`      | `fls_dataset`                   | rank-1 of `H5T_C_S1` + `H5Tset_size(N)` (no VLEN — flat bytes)    |

### Ragged / vlen

| C++ type                                | `storage_representation_t`      | HDF5 on-disk shape                                                |
| :-------------------------------------- | :------------------------------ | :---------------------------------------------------------------- |
| `std::vector<std::vector<T>>`           | `ragged_vlen_dataset`           | rank-1 of `H5T_VLEN { T }` (hvl_t relay)                          |

### Containers of arrays

| C++ type                                | `storage_representation_t`      | HDF5 on-disk shape                                                |
| :-------------------------------------- | :------------------------------ | :---------------------------------------------------------------- |
| `std::vector<std::array<T, N>>` (non-`char`) | `array_dataset`            | rank-1 of `H5T_ARRAY[N]` of `T`                                   |
| `std::vector<T[N]>` — equivalent        | `array_dataset`                 | rank-1 of `H5T_ARRAY[N]` of `T`                                   |

### Numeric

| C++ type                                | `storage_representation_t`      | HDF5 on-disk shape                                                |
| :-------------------------------------- | :------------------------------ | :---------------------------------------------------------------- |
| `std::complex<T>` (top-level)           | `scalar`                        | scalar `H5T_COMPOUND { T real; T imag; }`                         |
| `std::vector<std::complex<T>>`          | `linear_value_dataset`          | rank-1 of `H5T_COMPOUND { T real; T imag; }`                      |

### Smart pointers (memory-region overloads)

| C++ type                                | `storage_representation_t`      | HDF5 on-disk shape                                                |
| :-------------------------------------- | :------------------------------ | :---------------------------------------------------------------- |
| `std::unique_ptr<T[]>`                  | (forwarded to raw pointer)      | rank-N (caller-supplied `h5::count`); element type = `T`'s native |
| `std::shared_ptr<T[]>`                  | (forwarded to raw pointer)      | same — `h5cpp/H5Mmemory_io.hpp` mapper                            |

### Views (C++23)

| C++ type                                | `storage_representation_t`      | HDF5 on-disk shape                                                |
| :-------------------------------------- | :------------------------------ | :---------------------------------------------------------------- |
| `std::mdspan<T, Extents, …>`            | (forwarded via view)            | rank from `Extents`; element type = `T`'s native                  |

Gated on `__cpp_lib_mdspan >= 202207L` (libstdc++ ≥ 15 / libc++ ≥ 19).

### Brace-list convenience

| C++ type                                | `storage_representation_t`      | HDF5 on-disk shape                                                |
| :-------------------------------------- | :------------------------------ | :---------------------------------------------------------------- |
| `std::initializer_list<T>` (via `h5::awrite(parent, name, {a,b,c})`) | `linear_value_dataset` | rank-1 of `T` (materialised internally) |

### NOT supported (intentional stoppers)

| C++ type / pattern                                                | Why not                                                                 |
| :--------------------------------------------------------------- | :---------------------------------------------------------------------- |
| `std::vector<bool>`                                              | Not addressable — `data()` does not return `bool*`                      |
| Unregistered POD aggregates                                      | Compound dispatch needs `H5CPP_REGISTER_STRUCT` (or compiler-emitted reflection) |
| `std::vector<std::vector<std::vector<T>>>` (3-deep nesting)      | Only `vector<vector<T>>` (ragged) and `vector<string>` (vlen text) supported |
| `std::array<std::string, N>`                                     | Fixed-extent array of VLEN strings — neither `array_element` nor `array_dataset` applies |
| Containers without a `T(size_t)` constructor for the read path   | Iter-staging needs to size the target before insertion                  |
| Map keys / set elements that aren't HDF5-storable                | Trait fallback bottoms out at `unsupported` → compile-time `static_assert` |

Each unsupported case fails at compile time with a clear
`static_assert` diagnostic — never silently.

## Walter Brown feature detection — the dispatch engine

The whole table above is built without ever switching on `typeid(T)`
or `if constexpr (std::is_same_v<T, std::vector<int>>) …`. Instead,
h5cpp asks structural questions about `T` using the **detection
idiom**:

```cpp
// h5cpp/H5Tmeta.hpp (paraphrased)

// "Does T have a member typedef called value_type?"
template<class T, class = void>
struct has_value_type : std::false_type {};

template<class T>
struct has_value_type<T, std::void_t<typename T::value_type>>
    : std::true_type {};

// "Does T have a .data() method?"
template<class T, class = void>
struct has_data_method : std::false_type {};

template<class T>
struct has_data_method<T, std::void_t<decltype(std::declval<T>().data())>>
    : std::true_type {};

// "Does T have a .size() method?"
template<class T, class = void>
struct has_size_method : std::false_type {};

template<class T>
struct has_size_method<T, std::void_t<decltype(std::declval<T>().size())>>
    : std::true_type {};
```

Each detector is a class template specialisation that **succeeds via
SFINAE** when the probed expression is well-formed, falling back to
the unspecialised primary template (`std::false_type`) when it isn't.

The pattern was popularised by Walter E. Brown's *"Modern Template
Metaprogramming"* CppCon talk and codified in C++17 as
`std::void_t`. Pre-C++17 the same pattern needed a hand-rolled
`make_void`.

### From detectors to a full dispatch matrix

`access_traits_t<T>` composes the detectors into a single trait
that asks all the questions at once:

```cpp
// (heavily simplified)
template<class T>
struct access_traits_t {
    static constexpr access_t kind =
        is_text_like_v<T>            ? access_t::text          :
        is_composite_v<T>            ? access_t::composite     :  // tuple / pair
        has_data_method_v<T>         ? access_t::contiguous    :  // vector / array / linalg
        is_iterator_only_v<T>        ? access_t::iterators     :  // list / set / map
        has_pointer_indirection_v<T> ? access_t::pointers      :  // vector<string>
                                       access_t::object;          // scalar / registered POD
    // ... rank, dims, data ptr accessors ...
};
```

The dispatch in `h5cpp/H5Dwrite.hpp` then becomes:

```cpp
if constexpr (kind == access_t::composite)        { /* tuple pack */         }
else if constexpr (kind == access_t::text)        { /* string branch */      }
else if constexpr (kind == access_t::contiguous)  { /* H5Dwrite ptr */       }
else if constexpr (kind == access_t::iterators)   { /* iter-staging */       }
else if constexpr (kind == access_t::pointers)    { /* relay through ptr */  }
else if constexpr (kind == access_t::object)      { /* scalar / compound */  }
```

No type name ever appears in the dispatch. The same code path
handles `std::vector<int>`, `arma::Mat<double>`, and any user-defined
container that satisfies the structural contract.

### What this buys you

1. **Open extension** — any user-defined STL-like container with
   `data()` + `size()` + `value_type` automatically works. No
   registration required.
2. **Compile-time validation** — wrong `T` fails at the
   `static_assert` stopper with a readable diagnostic, never at
   runtime.
3. **Zero virtual dispatch / RTTI cost** — every branch is selected
   at compile time. The generated assembly is the same as if you'd
   written the right `H5Dwrite` call by hand.

## Third-party container libraries

Because the dispatch asks *structural* questions about `T` rather
than matching by name, containers from third-party libraries that
mirror the STL's contract work transparently — usually with no
extra include beyond the library's own headers and the matching
`h5cpp/H5Mxxx.hpp` mapper (if one ships) or none at all.

The table below maps common third-party container types to the
dispatch slot they would hit. **Bold rows** are validated by the
h5cpp test suite or have a dedicated mapper header; the rest
should work via structural detection but have not been
exhaustively tested.

| Library | Container type | Structural shape | Expected dispatch | Status |
| :------ | :------------- | :--------------- | :---------------- | :----- |
| **Abseil** | `absl::InlinedVector<T, N>` | `data()` + `size()` + `value_type`, contiguous | `linear_value_dataset` | ◇ should work via detection |
| **Abseil** | `absl::FixedArray<T>` | same | `linear_value_dataset` | ◇ should work |
| **Abseil** | `absl::flat_hash_map<K, V>` | iter-only + `value_type = pair<const K, V>` | `key_value_dataset` (via iter-staging) | ◇ should work |
| **Abseil** | `absl::flat_hash_set<T>` | iter-only + `value_type = T` | `linear_value_dataset` (via iter-staging) | ◇ should work |
| **Abseil** | `absl::btree_map<K, V>` / `btree_set<T>` | iter-only, sorted | as `map<K,V>` / `set<T>` | ◇ should work |
| **Abseil** | `absl::string_view` | text-like + `data()` + `size()` | `vlen_text_dataset` (scalar) | ◇ should work |
| **Abseil** | `absl::Cord` | non-contiguous (rope) — no flat `data()` | not directly supported | ✘ requires manual conversion to `std::string` |
| **Boost.Container** | `boost::container::vector<T>` | same as `std::vector` | `linear_value_dataset` | ◇ should work |
| **Boost.Container** | `boost::container::small_vector<T, N>` | same | `linear_value_dataset` | ◇ should work |
| **Boost.Container** | `boost::container::static_vector<T, N>` | same | `linear_value_dataset` | ◇ should work |
| **Boost.Container** | `boost::container::stable_vector<T>` | iter-only — no flat `data()` | `linear_value_dataset` (iter-staging path) | ◇ should work |
| **Boost.Container** | `boost::container::flat_map<K, V>` / `flat_set<T>` | iter-only or contiguous | `key_value_dataset` / `linear_value_dataset` | ◇ should work |
| **Boost.Container** | `boost::container::deque<T>` | as `std::deque` | `linear_value_dataset` | ◇ should work |
| **Folly** (Meta) | `folly::fbvector<T>` | as `std::vector` | `linear_value_dataset` | ◇ should work |
| **Folly** | `folly::fbstring` | as `std::string` (SSO + `data()`) | `vlen_text_dataset` | ◇ should work |
| **Folly** | `folly::F14ValueMap<K, V>` / `F14FastMap` | iter-only + pair value_type | `key_value_dataset` | ◇ should work |
| **EASTL** | `eastl::vector<T>` | as `std::vector` | `linear_value_dataset` | ◇ should work |
| **EASTL** | `eastl::hash_map<K, V>` | iter-only + pair value_type | `key_value_dataset` | ◇ should work |
| **plf** | `plf::colony<T>` | iter-only, non-contiguous | `linear_value_dataset` (iter-staging) | ◇ should work |
| **plf** | `plf::stack<T>` / `plf::list<T>` | iter-only | `linear_value_dataset` | ◇ should work |
| **martinus/robin-hood** | `robin_hood::unordered_map<K, V>` | iter-only + pair value_type | `key_value_dataset` | ◇ should work |
| **greg7mdp/parallel-hashmap** | `phmap::flat_hash_map<K, V>` | iter-only + pair value_type | `key_value_dataset` | ◇ should work |
| **C++23 std** | `std::flat_map<K, V>` / `std::flat_set<T>` | iter-only / contiguous | `key_value_dataset` / `linear_value_dataset` | ◇ should work once C++23 lands |
| **Eigen / Armadillo / Blaze / Blitz / dlib / xtensor** | dense + sparse matrices/vectors | dedicated mapper headers | see @ref curated_topics_linalg | ✔ validated |
| **std::valarray** | `std::valarray<T>` | dedicated mapper header | `linear_value_dataset` | ✔ validated |

### What "should work via detection" really means

Three structural contracts cover most container libraries:

1. **Contiguous + sized** — `T::data()` returns a pointer; `T::size()`
   returns count; `T::value_type` is the element type. → routes to
   `linear_value_dataset`.
2. **Iter-only + sized + insertable** — `begin()` / `end()` exist;
   `size()` exists; `value_type` exists; the container has a
   `T(size_t)` constructor (or `T()` + `.insert()`) so the read
   side can size+populate. → routes to the iter-staging path
   (`linear_value_dataset` via vector buffer, or `key_value_dataset`
   for pair-valued).
3. **Text-like** — `data()` returns `char*` / `const char*`;
   `size()` returns the length. → routes to `vlen_text_dataset`.

Anything outside these three contracts (e.g. `absl::Cord`, which
has no flat byte view; rope-style strings; multi-dimensional
non-tensor containers) requires either a dedicated mapper header
or manual conversion at the call site.

### Adding a new library mapper

If a container library doesn't work via structural detection (or
you want explicit control over the dispatch), drop a small
`h5cpp/H5MyourLib.hpp` header alongside `h5cpp/H5Marma.hpp` —
typically 30–50 lines specialising three traits:

- `meta::is_contiguous<T>` (or leave default for iter-only)
- `meta::storage_representation_impl<T>` — pin the dispatch slot
- `meta::access_traits_t<T>` — rank / dims / data pointer accessors

See the `h5cpp/H5Marma.hpp` mapper as the worked reference.

## Where to go next

- @ref link_base_template_types — comprehensive Supported Types
  reference (this page is a subset focused on STL)
- @ref curated_topics_linalg — same dispatch traits, applied to
  arma / Eigen / xtensor / etc.
- @ref reports_type_system_map — design rationale
  for the trait system (Walter Brown idiom + h5cpp's extensions)
- @ref example_guide_stl — runnable per-type STL examples
- @ref example_guide_container — fixed-extent container patterns
