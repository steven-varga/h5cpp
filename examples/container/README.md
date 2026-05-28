@page example_guide_container Supported container shapes

h5cpp routes C++ containers to HDF5 layouts by capability rather than by name. The dispatch is the Walter Brown detection idiom: at compile time, the type is probed for the expressions that should be valid on it (`value_type`, `iterator`, `begin`/`end`, `size`, `data`, `key_type`, `mapped_type`), and the matching storage representation is selected from the result. `std::vector<T>` writes as a rank-1 dataset because it exposes contiguous data with a known size and an element type the library can serialise — not because it is `std::vector`. Any type with the same surface routes the same way.

```cpp
h5::write(fd, "path", container);
auto x = h5::read<std::vector<int>>(fd, "path");
h5::append(packet_table, forward_list);
```

### Files

| File                  | Purpose                                                                      |
| --------------------- | ---------------------------------------------------------------------------- |
| `container.cpp`       | Round-trips the supported `std::` container families                         |
| `tiny_containers.hpp` | Header-only minimal custom containers: `vec`, `flist`, `set`, `dict`         |
| `detected.cpp`        | Demonstrates Walter Brown detection-idiom dispatch on non-`std::` containers |

Both examples are header-only from the user side:

```cpp
#include <h5cpp/all>
```

No generated compound descriptor header is needed here. The compound layouts used for `pair`, `tuple`, and map-like containers are handled by h5cpp traits.

---

### The Main Idea

The useful mental model is:

```text
C++ type surface      -> detected capabilities
detected capabilities -> storage representation
storage representation -> HDF5 dataset layout
```

For example:

```text
has value_type + iterator + size + data
    -> sequential-like contiguous container
    -> linear_value_dataset
    -> rank-1 HDF5 dataset

has key_type + mapped_type + iterator
    -> map-like container
    -> key_value_dataset
    -> rank-1 HDF5 compound dataset { key, value }

has key_type + value_type, no mapped_type
    -> set-like container
    -> linear_value_dataset
    -> rank-1 HDF5 dataset of keys
```

No inheritance, no virtual interfaces, no adapter layer — only compile-time shape recognition. The library detects the expressions that are valid for `T` and lets overload resolution pick the storage path.

---

### Minimal Example

```cpp
#include <h5cpp/all>
#include <deque>
#include <list>
#include <iostream>

int main() {
    auto fd = h5::create("containers.h5", H5F_ACC_TRUNC);
    auto data = h5::uniform<int>{1, 100} | h5::take(20);

    h5::write(fd, "sequence/vector", data);
    h5::write(fd, "sequence/deque", std::deque<int>(data.begin(), data.end()));
    h5::write(fd, "sequence/list",  std::list<int>(data.begin(), data.end()));

    auto v2 = h5::read<std::vector<int>>(fd, "sequence/vector");
    auto d2 = h5::read<std::deque<int>>(fd, "sequence/deque");
    auto l2 = h5::read<std::list<int>>(fd, "sequence/list");

    std::cout << "vector: " << v2 << "\n";
    std::cout << "deque:  " << d2 << "\n";
    std::cout << "list:   " << l2 << "\n";
}
```

`std::vector<T>` writes directly. `std::deque<T>` and `std::list<T>` are staged into a temporary contiguous buffer first. The on-disk result is identical: a rank-1 HDF5 dataset of `T`. The C++ container type does not appear in the file.

---

### Supported Container Families

`container.cpp` writes and reads:

* sequence containers: `std::vector<T>`, `std::deque<T>`, `std::list<T>`
* fixed-width row containers: `std::vector<std::array<T,N>>`
* sorted sets: `std::set<T>`, `std::multiset<T>`
* hash sets: `std::unordered_set<T>`, `std::unordered_multiset<T>`
* key-value containers: `std::map<K,V>`, `std::multimap<K,V>`
* hash maps: `std::unordered_map<K,V>`, `std::unordered_multimap<K,V>`
* composite element sequences: `std::vector<std::pair<K,V>>`, `std::vector<std::tuple<...>>`
* streaming input: `std::forward_list<T>` through `h5::append`
* variable-length text: `std::vector<std::string>`
* ragged arrays: `std::vector<std::vector<T>>`

The compact model:

```text
container<T>              -> HDF5 dataset over T
set<T>                    -> HDF5 dataset over T, unique by container semantics
map<K,V>                  -> HDF5 compound dataset { key, value }
pair<K,V>                 -> HDF5 compound { first, second }
tuple<...>                -> HDF5 compound packed from tuple fields
vector<string>            -> HDF5 variable-length text dataset
vector<vector<T>>         -> HDF5 ragged variable-length dataset
```

h5cpp does not store “a C++ container implementation.”
It stores the data model implied by the container.

---

### Sequence Containers

```cpp
std::vector<T> // direct contiguous write
std::deque<T>  // staged, then written
std::list<T>   // staged, then written
```

`std::vector<T>` is the fast path: contiguous memory, direct pointer, one `H5Dwrite`.

`std::deque<T>` and `std::list<T>` are not contiguous, so h5cpp copies them into a staging buffer first. The file layout is still a rank-1 dataset of `T`.

```cpp
h5::write(fd, "sequence/vector", data);
h5::write(fd, "sequence/deque", std::deque<int>(data.begin(), data.end()));
h5::write(fd, "sequence/list",  std::list<int>(data.begin(), data.end()));
```

---

### Vector of Arrays

```cpp
std::vector<std::array<int,4>>
```

A `vector<array<T,N>>` is flattened into a rectangular dataset. The outer vector gives the row count; the inner array gives the fixed column count.

```cpp
std::vector<std::array<int,4>> rows(5);
h5::write(fd, "sequence/vec_array4", rows);
```

Conceptually:

```text
std::vector<std::array<T,N>> -> rank-2 HDF5 dataset [rows, N]
```

Fixed-size rows, no variable-length machinery — the cleanest of the multi-dimensional paths.

---

### Sets

Sorted sets are stored in their iteration order:

```cpp
std::set<T>      // sorted, unique
std::multiset<T> // sorted, duplicates retained
```

```cpp
std::vector<int> src{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};

h5::write(fd, "sorted_set/set",
    std::set<int>(src.begin(), src.end()));

h5::write(fd, "sorted_set/multiset",
    std::multiset<int>(src.begin(), src.end()));
```

The result is a rank-1 dataset. For `set`, duplicates disappear. For `multiset`, duplicates remain.

---

### Hash Sets

Hash sets round-trip as containers, but their order is not meaningful:

```cpp
std::unordered_set<T>      // unique, unspecified order
std::unordered_multiset<T> // duplicates retained, unspecified order
```

```cpp
h5::write(fd, "hash_set/unordered_set",
    std::unordered_set<int>(src.begin(), src.end()));
```

The file records the observed iteration order, which depends on the hash table's internal state and is not portable across runs, compilers, or library versions. Do not depend on it.

---

### Streaming Containers

`std::forward_list<T>` has no `.data()` and is not a good match for ordinary contiguous writes.

Use an unlimited chunked dataset and append:

```cpp
std::forward_list<int> fl{7, 14, 21, 28, 35};

h5::pt_t pt = h5::create<int>(
    fd, "streaming/forward_list",
    h5::max_dims{H5S_UNLIMITED},
    h5::chunk{1});

h5::append(pt, fl);
```

Read it back as a normal sequence:

```cpp
auto back = h5::read<std::vector<int>>(fd, "streaming/forward_list");
```

This is the right pattern for single-pass input, packet streams, and append-style workflows.

---

### Key-Value Containers

Map-like containers are stored as rank-1 datasets of compound records:

```cpp
struct kv_t {
    K key;
    V value;
};
```

Supported examples:

```cpp
std::map<int, double>
std::multimap<short, int>
std::unordered_map<int, float>
std::unordered_multimap<int, int>
```

```cpp
std::map<int, double> m = {
    {1, 1.5},
    {2, 2.5},
    {3, 3.5}
};

h5::write(fd, "key_value/map", m);

auto m2 = h5::read<std::map<int, double>>(fd, "key_value/map");
```

Sorted maps are written in key order. Unordered maps are written in bucket iteration order. Round-trip reconstruction preserves the source container's semantics; the on-disk order reflects whatever the writer's container produced at the moment of write.

---

### Pair and Tuple Elements

Sequences of `pair` and `tuple` become compound datasets.

```cpp
std::vector<std::pair<int, double>> vp = {
    {1, 1.5},
    {2, 2.5},
    {3, 3.5}
};

h5::write(fd, "tuple_pair/vec_pair", vp);
```

The pair layout is:

```text
compound { first, second }
```

Tuples are packed into an internal C-struct mirror before writing:

```cpp
std::vector<std::tuple<int, double, float>> vt = {
    {1, 1.5, 2.5f},
    {2, 2.5, 3.5f},
    {3, 3.5, 4.5f}
};

h5::write(fd, "tuple_pair/vec_tuple", vt);
```

Conceptually:

```text
std::tuple<T0,T1,T2> -> compound { _0, _1, _2 }
```

The exact field naming is implementation-defined by h5cpp traits, but the model is simple: one tuple element becomes one compound record.

---

## Detection-Idiom Dispatch

`detected.cpp` is the important proof.

It shows that h5cpp’s container dispatch is **structural**, not name-based. The example uses custom containers from `tiny_containers.hpp`:

```cpp
tiny::vec<T>      // contiguous sequence
tiny::flist<T>    // iterator-only sequence
tiny::set<T>      // set-like container
tiny::dict<K,V>   // map-like container
```

None of these derive from `std::vector`, `std::list`, `std::set`, or `std::map`.

Yet h5cpp still routes them correctly because the Walter Brown detection idiom recognizes their type surface.

### Layer 1: Capability Detection

h5cpp probes the type:

```text
has_iterator<T>
has_value_type<T>
has_data<T>
has_size<T>
is_sequential_like<T>
is_set_like<T>
is_map_like<T>
```

The demo prints a trait card:

```text
tiny::vec<int>
  ✔ has_iterator
  ✔ has_value_type
  ✔ has_data
  ✔ has_size
  ✔ is_sequential_like
  ✘ is_set_like
  ✘ is_map_like
  storage_representation_v = linear_value_dataset

tiny::dict<int,double>
  ✔ has_iterator
  ✔ has_value_type
  ✘ has_data
  ✔ has_size
  ✘ is_sequential_like
  ✘ is_set_like
  ✔ is_map_like
  storage_representation_v = key_value_dataset
```

This is the key message:

```text
valid expressions define the category
category defines the storage
storage defines the HDF5 layout
```

The trait outputs are the dispatch's input: valid expressions define the category, the category selects the storage representation, the representation determines the HDF5 layout.

### Layer 2: Storage Representation

After detection, h5cpp picks the storage model:

| Custom type       | Detected shape      | Storage representation | HDF5 layout                              |
| ----------------- | ------------------- | ---------------------- | ---------------------------------------- |
| `tiny::vec<T>`    | contiguous sequence | `linear_value_dataset` | rank-1 dataset of `T`                    |
| `tiny::flist<T>`  | iterator sequence   | `linear_value_dataset` | rank-1 dataset of `T`                    |
| `tiny::set<T>`    | set-like            | `linear_value_dataset` | rank-1 dataset of `T`                    |
| `tiny::dict<K,V>` | map-like            | `key_value_dataset`    | rank-1 compound dataset `{ key, value }` |

The write path:

```cpp
tiny::vec<int>          v{1, 2, 3, 4, 5};
tiny::flist<int>        l{10, 20, 30, 40};
tiny::set<int>          s{3, 1, 4, 1, 5, 9, 2, 6};
tiny::dict<int, double> d{{1, 1.5}, {2, 2.5}, {3, 3.5}};

h5::write(fd, "tiny_vec",   v);
h5::write(fd, "tiny_flist", l);
h5::write(fd, "tiny_set",   s);
h5::write(fd, "tiny_dict",  d);
```

The read-back path:

```cpp
auto v2 = h5::read<tiny::vec<int>>(fd, "tiny_vec");

auto l2 = h5::read<std::vector<int>>(fd, "tiny_flist");
auto s2 = h5::read<std::set<int>>(fd, "tiny_set");
auto d2 = h5::read<std::map<int, double>>(fd, "tiny_dict");
```

Current asymmetry:

| Case                           | Current behavior                                           |
| ------------------------------ | ---------------------------------------------------------- |
| Contiguous custom vector-shape | Can write and read back into the custom type itself        |
| Iterator-only custom sequence  | Can write structurally; read back through `std::vector<T>` |
| Custom set-shape               | Can write structurally; read back through `std::set<T>`    |
| Custom map-shape               | Can write structurally; read back through `std::map<K,V>`  |

The write side is structural across all four shapes. The read side is structural for contiguous custom containers (they round-trip into themselves) but currently uses the matching `std::` counterpart for iterator-only, set-like, and map-like custom containers. This is a construction-policy gap in the dispatcher, not a file-format limitation — the HDF5 layout on disk is identical regardless of which C++ container reads it back.

---

## Third-Party Containers That Just Work

`tiny::vec`, `tiny::flist`, `tiny::set`, and `tiny::dict` are toy demos. The same detection-idiom dispatch picks up real-world third-party containers automatically — no specialization, no opt-in macro, no registration step.

### Vector-shape (round-trip into the custom type)

Any container with `.data()` + `.size()` + `value_type` + `T(size_t)` ctor:

| Library         | Type                                  |
| --------------- | ------------------------------------- |
| Abseil          | `absl::FixedArray<T>`                 |
| Abseil          | `absl::InlinedVector<T, N>`           |
| Folly           | `folly::small_vector<T, N>`           |
| Folly           | `folly::fbvector<T>`                  |
| Boost.Container | `boost::container::vector<T>`         |
| Boost.Container | `boost::container::small_vector<T,N>` |
| Boost.Container | `boost::container::static_vector<T,N>`|
| eve             | `eve::aligned_vector<T>`              |

These ride the read-side structural fallback added to `H5Dread.hpp` and round-trip into the custom type itself.

### Set-shape (write directly; read back through `std::set` today)

Any container with `key_type` + `value_type`, no `mapped_type`:

| Library         | Types                                                            |
| --------------- | ---------------------------------------------------------------- |
| Abseil          | `absl::flat_hash_set<T>`, `absl::node_hash_set<T>`, `absl::btree_set<T>` |
| TSL             | `tsl::robin_set<T>`, `tsl::hopscotch_set<T>`, `tsl::sparse_set<T>` |
| parallel-hashmap| `phmap::flat_hash_set<T>`, `phmap::parallel_flat_hash_set<T>`    |
| Boost           | `boost::container::flat_set<T>`, `boost::unordered_set<T>`       |
| Folly           | `folly::F14ValueSet<T>`, `folly::F14NodeSet<T>`                  |

These ride the new `is_set_like` fallback in `storage_representation_impl`. Before today they resolved to `unsupported` and triggered a `static_assert`.

### Map-shape (write directly; read back through `std::map` today)

Any container with `key_type` + `mapped_type` + `value_type`:

| Library         | Types                                                              |
| --------------- | ------------------------------------------------------------------ |
| Abseil          | `absl::flat_hash_map<K,V>`, `absl::node_hash_map<K,V>`, `absl::btree_map<K,V>` |
| TSL             | `tsl::robin_map<K,V>`, `tsl::hopscotch_map<K,V>`, `tsl::sparse_map<K,V>` |
| parallel-hashmap| `phmap::flat_hash_map<K,V>`, `phmap::parallel_flat_hash_map<K,V>` |
| Boost           | `boost::container::flat_map<K,V>`, `boost::unordered_map<K,V>`    |
| Folly           | `folly::F14ValueMap<K,V>`, `folly::F14NodeMap<K,V>`               |

These have been working since the original `is_map_like` fallback landed; listed here for completeness alongside their set counterparts.

### What does NOT just work

* Containers that allocate via a non-`size_t` constructor (e.g. only accept iterator pairs) won't round-trip on the structural read path — the dispatcher needs `T(std::size_t)`.
* Containers with non-standard-layout elements still fire the iter-staging guard.
* Linalg matrix types (Eigen, blaze) opt out of the iterable pretty-printer via the `Scalar` nested-type veto, which is desirable: those types have their own `operator<<`.

---

## Complete Write-Side Type Matrix

| Type                                                       | `kind`                   | `storage`                          | Write mechanism                                        |
| ---------------------------------------------------------- | ------------------------ | ---------------------------------- | ------------------------------------------------------ |
| `int`, `float`, enums                                      | `object`                 | `scalar`                           | direct `H5Dwrite`                                      |
| `std::string`, `std::string_view`                          | `text`                   | `vlen_text_dataset`                | direct write via variable-length text type             |
| `std::vector<T>`, `std::array<T,N>`, `T[N]`                | `contiguous`             | `linear_value_dataset` / `c_array` | direct `H5Dwrite`                                      |
| `std::vector<std::array<T,N>>`                             | `contiguous`             | `fixed_inner_extent_dataset`       | direct write as rows × `N`                             |
| `std::vector<std::complex<T>>`                             | `contiguous`             | `linear_value_dataset`             | direct `H5Dwrite`                                      |
| Linear algebra types                                       | `contiguous`             | `linear_value_dataset`             | direct `H5Dwrite`                                      |
| `std::vector<std::string>`                                 | `pointers`               | `vlen_text_dataset`                | `char*` relay + `H5T_VARIABLE`                         |
| `std::vector<std::vector<T>>`                              | `pointers`               | `ragged_vlen_dataset`              | `hvl_t` relay + `H5Tvlen_create`                       |
| `std::vector<NonTrivialPod>`                               | `pointers`               | `linear_value_dataset`             | `h5::gather` then flat write                           |
| `std::list<T>`, `std::deque<T>`, `std::forward_list<T>`    | `iterators`              | `linear_value_dataset`             | staging vector then flat write                         |
| `std::set<T>`, `std::multiset<T>`                          | `iterators`              | `linear_value_dataset`             | staging vector then flat write                         |
| `std::unordered_set<T>`, `std::unordered_multiset<T>`      | `iterators`              | `linear_value_dataset`             | staging vector then flat write                         |
| `std::map<K,V>`, `std::multimap<K,V>`                      | `iterators`              | `key_value_dataset`                | `kv_t` compound + `H5T_COMPOUND`                       |
| `std::unordered_map<K,V>`, `std::unordered_multimap<K,V>`  | `iterators`              | `key_value_dataset`                | `kv_t` compound + `H5T_COMPOUND`                       |
| `std::tuple<Ts...>`                                        | `composite`              | `scalar`                           | `traits::pack(ref, buf)` then compound scalar write    |
| `std::vector<std::tuple<Ts...>>`                           | `pointers`               | `linear_value_dataset`             | pack each tuple then rank-1 compound write             |
| `std::list<std::tuple<Ts...>>`                             | `iterators`              | `linear_value_dataset`             | pack each tuple through staging                        |
| `std::pair<K,V>`                                           | `object`                 | `scalar`                           | direct compound write via `dt_t<pair>`                 |
| `std::vector<std::pair<K,V>>`                              | `contiguous`             | `linear_value_dataset`             | direct compound rank-1 write                           |
| `std::complex<T>`                                          | `object`                 | `scalar`                           | direct write via `dt_t<complex>`                       |
| User aggregate registered via `H5CPP_REGISTER_STRUCT(Foo)` | `object`                 | `scalar`                           | direct compound write                                  |
| Compiler-reflected tier-2 type                             | —                        | —                                  | generated scatter path                                 |
| `tiny::vec<T>`, `absl::FixedArray<T>`, `folly::small_vector`, `boost::container::vector`, …  | detected contiguous      | `linear_value_dataset`             | structural write                                       |
| `tiny::flist<T>` and any custom iterator-only sequence     | detected sequential-like | `linear_value_dataset`             | structural write through staging                       |
| `tiny::set<T>`, `absl::flat_hash_set`, `tsl::robin_set`, `boost::flat_set`, `folly::F14ValueSet`, … | detected set-like        | `linear_value_dataset`             | structural write through staging                       |
| `tiny::dict<K,V>`, `absl::flat_hash_map`, `tsl::robin_map`, `boost::flat_map`, `folly::F14ValueMap`, … | detected map-like        | `key_value_dataset`                | structural write as `{ key, value }`                   |
| `std::vector<bool>`                                        | —                        | `unsupported`                      | rejected correctly                                     |
| deeply nested containers                                   | —                        | mostly unsupported                 | compile-time stopper                                   |
| unregistered POD aggregate                                 | —                        | `unsupported`                      | requires registration or compiler-generated descriptor |

---

## Complete Read-Side Type Matrix

| Type                                                       | `kind`                           | `storage`                          | Read mechanism                            |
| ---------------------------------------------------------- | -------------------------------- | ---------------------------------- | ----------------------------------------- |
| `int`, `float`, enums                                      | `object`                         | `scalar`                           | direct `H5Dread`                          |
| `std::string`, `std::string_view`                          | `text`                           | `vlen_text_dataset`                | direct variable-length text read          |
| `std::vector<T>`, `std::array<T,N>`, `T[N]`                | `contiguous`                     | `linear_value_dataset` / `c_array` | direct `H5Dread`                          |
| `std::vector<std::array<T,N>>`                             | `contiguous`                     | `fixed_inner_extent_dataset`       | direct read as rows × `N`                 |
| `std::vector<std::complex<T>>`                             | `contiguous`                     | `linear_value_dataset`             | direct `H5Dread`                          |
| Linear algebra types                                       | `contiguous`                     | `linear_value_dataset`             | direct `H5Dread`                          |
| `std::vector<std::string>`                                 | `pointers`                       | `vlen_text_dataset`                | `char*` relay + reclaim                   |
| `std::vector<std::vector<T>>`                              | `pointers`                       | `ragged_vlen_dataset`              | `hvl_t` relay + reclaim                   |
| `std::list<T>`, `std::deque<T>`, `std::forward_list<T>`    | `iterators`                      | `linear_value_dataset`             | read to staging vector, then assign/copy  |
| `std::set<T>`, `std::multiset<T>`                          | `iterators`                      | `linear_value_dataset`             | read to staging vector, then insert       |
| `std::unordered_set<T>`, `std::unordered_multiset<T>`      | `iterators`                      | `linear_value_dataset`             | read to staging vector, then insert       |
| `std::map<K,V>`, `std::multimap<K,V>`                      | `iterators`                      | `key_value_dataset`                | read `kv_t` compound records, then insert |
| `std::unordered_map<K,V>`, `std::unordered_multimap<K,V>`  | `iterators`                      | `key_value_dataset`                | read `kv_t` compound records, then insert |
| `std::tuple<Ts...>`                                        | `composite`                      | `scalar`                           | compound read then `traits::unpack`       |
| `std::vector<std::tuple<Ts...>>`                           | `pointers`                       | `linear_value_dataset`             | read packed compounds, unpack each        |
| `std::list<std::tuple<Ts...>>`, `set<...>`, `deque<...>`   | `iterators`                      | `linear_value_dataset`             | unpack each, then assign/insert           |
| `std::pair<K,V>`                                           | `object`                         | `scalar`                           | direct compound read                      |
| `std::vector<std::pair<K,V>>`                              | `contiguous`                     | `linear_value_dataset`             | direct compound rank-1 read               |
| `std::complex<T>`                                          | `object`                         | `scalar`                           | direct read via `dt_t<complex>`           |
| User aggregate registered via `H5CPP_REGISTER_STRUCT(Foo)` | `object`                         | `scalar`                           | direct compound read                      |
| Compiler-reflected tier-2 type                             | —                                | —                                  | generated gather path                     |
| `tiny::vec<T>`, `absl::FixedArray<T>`, `folly::small_vector`, `boost::container::vector`, …  | detected contiguous vector-shape | `linear_value_dataset`             | structural read into custom type          |
| `tiny::flist<T>` and any custom iterator-only sequence     | detected iterator-only sequence  | `linear_value_dataset`             | read back through `std::vector<T>` today  |
| `tiny::set<T>`, `absl::flat_hash_set`, `tsl::robin_set`, … | detected set-like                | `linear_value_dataset`             | read back through `std::set<T>` today     |
| `tiny::dict<K,V>`, `absl::flat_hash_map`, `tsl::robin_map`, … | detected map-like                | `key_value_dataset`                | read back through `std::map<K,V>` today   |
| `std::vector<bool>`                                        | —                                | `unsupported`                      | rejected correctly                        |
| deeply nested containers                                   | —                                | unsupported / guarded              | compile-time stopper                      |
| unregistered POD aggregate                                 | —                                | `unsupported`                      | requires descriptor                       |

---

### Variable-Length Storage

Two nested forms backed by HDF5 variable-length storage are wired and round-trip cleanly:

| C++ type                      | HDF5 model                     | Mechanism                              |
| ----------------------------- | ------------------------------ | -------------------------------------- |
| `std::vector<std::string>`    | variable-length text dataset   | `char*` relay array + `H5T_VARIABLE`   |
| `std::vector<std::vector<T>>` | ragged variable-length dataset | `hvl_t` relay array + `H5Tvlen_create` |

```cpp
std::vector<std::string> names = {"alpha", "beta", "gamma"};
h5::write(fd, "names", names);
auto back = h5::read<std::vector<std::string>>(fd, "names");

std::vector<std::vector<int>> jagged = {
    {1, 2, 3},
    {4, 5},
    {6, 7, 8, 9}
};

h5::write(fd, "jagged", jagged);
auto back2 = h5::read<std::vector<std::vector<int>>>(fd, "jagged");
```

---

### Pretty Printing

The example also demonstrates h5cpp’s STL stream output helpers:

```cpp
std::cout << "vector: " << v2 << "\n";
std::cout << "map:    " << m2 << "\n";
```

Containers can be inserted directly into `std::ostream`. Long containers are truncated according to `H5CPP_CONSOLE_WIDTH`, with a trailing `...`.

Useful for examples, tests, and sanity checks. Not a serialisation format — use `h5::write` for that.

The iterable pretty-printer vetoes types exposing a `Scalar` nested alias, so Eigen / blaze / xtensor matrices keep their own `operator<<` rather than getting hijacked by a generic begin/end print loop. Linalg libraries name their element `Scalar`; STL containers name it `value_type` — clean discriminator.

---

### Not Yet Supported

Deeper nesting still stops at compile time:

```cpp
std::vector<std::list<T>>
std::vector<std::set<T>>
std::vector<std::map<K,V>>
std::list<std::vector<T>>
```

HDF5 itself can represent many of these shapes with nested variable-length and compound types, but h5cpp intentionally rejects them until the recursive packer/unpacker path is explicit.

Other intentional stoppers:

| C++ type                        | Why                                                           |
| ------------------------------- | ------------------------------------------------------------- |
| `std::vector<bool>`             | bit-packing specialization; no real contiguous `bool*`        |
| `std::array<std::string, N>`    | fixed array of variable-length elements needs explicit policy |
| `std::array<std::vector<T>, N>` | array-of-container guard                                      |
| Unregistered POD aggregate      | needs `H5CPP_REGISTER_STRUCT(Foo)` or generated descriptor    |
| Arbitrary nested containers     | recursive VLEN/compound chain not wired yet                   |

---

### Final Mental Model

```text
vector<T>                 -> rank-1 dataset
vector<array<T,N>>        -> rank-2 rectangular dataset
vector<string>            -> variable-length text dataset
vector<vector<T>>         -> ragged variable-length dataset
set<T>                    -> rank-1 dataset, sorted, unique
multiset<T>               -> rank-1 dataset, sorted, duplicates kept
unordered_set<T>          -> rank-1 dataset, unordered, unique
map<K,V>                  -> rank-1 compound dataset { key, value }
vector<pair<K,V>>         -> rank-1 compound dataset { first, second }
vector<tuple<...>>        -> rank-1 compound dataset
forward_list<T>           -> append stream, read back as sequence

tiny::vec<T>              -> detected as sequential-like
tiny::flist<T>            -> detected as sequential-like
tiny::set<T>              -> detected as set-like
tiny::dict<K,V>           -> detected as map-like
```

The point of all this: h5cpp stores the data model implied by the container, not the container implementation. The detection idiom makes that possible — types with the same shape route to the same HDF5 layout, regardless of which library they come from. Hand-written overloads aren't needed for each new container type; the structural surface is the contract.

## Source

- [`container.cpp`](container_8cpp-example.html) — rendered with syntax highlighting
- [`detected.cpp`](detected_8cpp-example.html) — rendered with syntax highlighting
- [`tiny_containers.hpp`](tiny_containers_8hpp-example.html) — rendered with syntax highlighting
