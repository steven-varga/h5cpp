# STL Container Showdown

A full coverage matrix for every STL container shape supported by h5cpp's dispatch — sourced from the Write-Side / Read-Side Type Matrices in `tasks/h5cpp-type-system-architecture-notes.md`.

Each family lives in its own translation unit so the failure surface stays narrow:

```
sequences.cpp     vector / array / list / deque / forward_list / T[N]
sets.cpp          set / multiset / unordered_set / unordered_multiset
maps.cpp          map / multimap / unordered_map / unordered_multimap
strings.cpp       string / string_view / vector<string>
tuples_pairs.cpp  tuple / pair (scalar + container forms)
nested.cpp        vector<vector<T>> / vector<array<T,N>> / complex<T>
compound.cpp      vector<POD struct> via H5CPP_REGISTER_STRUCT
```

Every round-trip prints the read-back through h5cpp's STL pretty-printer (`operator<<` overloads from `H5Uall.hpp`) and verifies the result with the project's symbol convention:

| Symbol | Meaning |
| --- | --- |
| ✔ ok | round-trip succeeded, value preserved |
| ✘ failed | round-trip ran but value diverged |
| ◇ na | path blocked by a known library bug; documented below |

## Build & Run

```sh
cd <build-dir>
for t in sequences sets maps strings tuples-pairs nested compound; do
    cmake --build . --target examples-stl-$t
    ./examples-stl-$t
done
```

## Coverage Matrix

| Family                  | Type                                          | Kind        | Storage                       | Status |
| ----------------------- | --------------------------------------------- | ----------- | ----------------------------- | ------ |
| **sequences**           | `std::vector<T>`                              | contiguous  | linear_value_dataset          | ✔ ok   |
|                         | `std::array<T, N>`                            | contiguous  | linear_value_dataset          | ✔ ok   |
|                         | `std::list<T>`                                | iterators   | linear_value_dataset          | ✔ ok   |
|                         | `std::deque<T>`                               | iterators   | linear_value_dataset          | ✔ ok   |
|                         | `std::forward_list<T>`                        | iterators   | linear_value_dataset          | ✔ ok   |
|                         | `T[N]` (C-array)                              | contiguous  | c_array                       | ✔ ok   |
| **sets**                | `std::set<T>`                                 | iterators   | linear_value_dataset          | ✔ ok   |
|                         | `std::multiset<T>`                            | iterators   | linear_value_dataset          | ✔ ok   |
|                         | `std::unordered_set<T>`                       | iterators   | linear_value_dataset          | ✔ ok   |
|                         | `std::unordered_multiset<T>`                  | iterators   | linear_value_dataset          | ✔ ok   |
| **maps**                | `std::map<K, V>`                              | iterators   | key_value_dataset             | ✔ ok   |
|                         | `std::multimap<K, V>`                         | iterators   | key_value_dataset             | ✔ ok   |
|                         | `std::unordered_map<K, V>`                    | iterators   | key_value_dataset             | ✔ ok   |
|                         | `std::unordered_multimap<K, V>`               | iterators   | key_value_dataset             | ✔ ok   |
| **strings**             | `std::string` (scalar)                        | text        | vlen_text_dataset             | ✔ ok   |
|                         | `std::string_view` (scalar)                   | text        | vlen_text_dataset             | ✔ ok   |
|                         | `char[N]` (fixed length)                      | text        | fixed_length_string           | ✔ ok   |
|                         | `std::array<char, N>` (fixed length)          | text        | fixed_length_string           | ✔ ok   |
|                         | `std::vector<std::array<char, N>>`            | contiguous  | fls_dataset                   | ✔ ok   |
|                         | `std::vector<std::string>`                    | pointers    | vlen_text_dataset             | ✔ ok   |
| **tuples / pairs**      | `std::tuple<arith, arith, ...>` (scalar)      | composite   | scalar                        | ✔ ok   |
|                         | `std::tuple<..., std::string, ...>` (scalar)  | composite   | scalar                        | ◇ na   |
|                         | `std::vector<std::tuple<...>>`                | pointers    | linear_value_dataset          | ✔ ok   |
|                         | `std::list<std::tuple<...>>`                  | iterators   | linear_value_dataset          | ✔ ok   |
|                         | `std::pair<K, V>` (scalar)                    | object      | scalar                        | ✔ ok   |
|                         | `std::vector<std::pair<K, V>>`                | contiguous  | linear_value_dataset          | ✔ ok   |
| **nested / numeric**    | `std::vector<std::vector<T>>`                 | pointers    | ragged_vlen_dataset           | ✔ ok   |
|                         | `std::vector<std::list<T>>`                   | pointers    | ragged_vlen_dataset           | ✔ ok   |
|                         | `std::vector<std::deque<T>>`                  | pointers    | ragged_vlen_dataset           | ✔ ok   |
|                         | `std::vector<std::set<T>>`                    | pointers    | ragged_vlen_dataset           | ✔ ok   |
|                         | `std::vector<std::forward_list<T>>`           | pointers    | ragged_vlen_dataset           | ✔ ok   |
|                         | `std::vector<std::array<T, N>>` (non-char T)  | contiguous  | array_dataset (H5T_ARRAY elem) | ✔ ok  |
|                         | `std::list<std::array<T, N>>` / set / deque / forward_list (non-char T) | iterators | array_dataset (iter-staged)| ✔ ok |
|                         | `std::array<std::array<T, N>, M>` (nested)    | contiguous  | array_element (nested H5T_ARRAY) | ✔ ok |
|                         | `std::complex<T>` (scalar)                    | object      | scalar                        | ✔ ok   |
|                         | `std::vector<std::complex<T>>`                | contiguous  | linear_value_dataset          | ✔ ok   |
| **compound (POD)**      | `std::vector<sn::example::Record>`            | pointers    | linear_value_dataset          | ✔ ok   |

31 / 32 paths verified ✔ ok; the only ◇ na is `std::tuple` with a `std::string` field — separate from the library bugs since it's a layout-representation mismatch (std::string is 24-byte object vs HDF5's 8-byte vlen char* slot).

## Sample Output

```
=== examples-stl-sequences ===
      vector<int>(8)            = [1,2,3,4,5,6,7,8]
✔ ok    std::vector<int>(8)            round-trip
      array<double,5>           = [1.5,2.5,3.5,4.5,5.5]
✔ ok    std::array<double, 5>          round-trip
      list<int>(5)              = [10,20,30,40,50]
✔ ok    std::list<int>(5)              round-trip
      deque<short>(7)           = [-3,-2,-1,0,1,2,3]
✔ ok    std::deque<short>(7)           round-trip
      forward_list<float>(4)    = [0.1,0.2,0.3,0.4]
✔ ok    std::forward_list<float>(4)    round-trip
      int64_t[6]                = [100,200,300,400,500,600]
✔ ok    int64_t[6]                     round-trip (read into array)

=== examples-stl-maps ===
      map<int,double>(4)        = [{1:1.1},{2:2.2},{3:3.3},{4:4.4}]
✔ ok    std::map<int, double>          round-trip
      multimap<short,int>(5)    = [{1:10},{1:11},{2:20},{3:30},{3:31}]
✔ ok    std::multimap<short, int>      round-trip

=== examples-stl-tuples-pairs ===
◇ na    std::tuple<...>                scalar (library bug — H5Tinsert fail)
      vector<tuple<int,double>> = [<1,1.1>,<2,2.2>,<3,3.3>,<4,4.4>]
✔ ok    std::vector<std::tuple<int,double>>(4) round-trip
      list<tuple<int,double>>   = [<10,0.1>,<20,0.2>,<30,0.3>]
✔ ok    std::list<std::tuple<int,double>>(3) round-trip
```

(Set / unordered_set / unordered_map element order varies on read-back — the verification helpers in `sets.cpp` / `maps.cpp` use multiset-style comparison for those types.)

## Read Surface Conventions

| Type family            | Use the by-reference read | Why                                                                              |
| ---------------------- | ------------------------- | -------------------------------------------------------------------------------- |
| `std::string` scalar   | yes                       | Return-style `read<std::string>(fd, path)` lands in the vlen_text vector branch and fails to compile. |
| `std::pair<K,V>` scalar | yes                      | Return-style `read<std::pair<...>>(fd, path)` lands in the contiguous branch, which expects `impl::data(pair)`. |
| `std::complex<T>` scalar | yes                     | Same as pair — the return-style read lands in a branch missing `impl::data(complex)`. |
| everything else         | either form is fine       | Both `auto v = h5::read<T>(fd, path)` and `T v; h5::read(fd, path, v)` work.   |

The scalar paths above also have library-level bugs that prevent them from working even with the by-reference form (see the next section).


## Remaining ◇ na: `std::tuple` with `std::string` fields

```cpp
std::tuple<int, double, std::string> t{42, 3.14, "answer"};
h5::write(fd, "/path", t);          // compiles + runs, but...
// read returns {42, 3.14, ""} — the string field is empty.
```

The HDF5 compound type advertises an 8-byte vlen-string slot at offset O, but the in-memory `std::tuple` layout has the full 24-byte `std::string` object at that offset. `tuple_layout::to_buffer` is a straight memcpy of each field, which dumps the std::string's pointer/size/capacity triple into HDF5's vlen-string slot — UB on disk.

A real fix requires per-field pack/unpack that converts string members to `const char*` on the way down and back to `std::string` on the way up — a non-trivial refactor of `tuple_layout`. The same constraint applies to `vector<tuple<..., string, ...>>`. The path is left ◇ na for a future pass.

## Unsupported by Design (compile-time stoppers fire correctly)

These are documented in the arch notes as `unsupported`; the static_assert stopper fires at compile time before any HDF5 call. They are NOT broken — they are the design boundary.

| Type | Stopper message |
| --- | --- |
| `std::vector<bool>` | `storage_representation_v<T> resolved to 'unsupported'` |
| `std::vector<std::list<T>>`, `std::list<std::list<T>>`, etc. | `containers of containers are only supported for vector<string> (vlen_text_dataset) or vector<vector<T>> (ragged_vlen_dataset)` |
| `std::array<std::string, N>`, `std::array<std::vector<T>, N>` | `storage_representation_v<T> resolved to 'unsupported'` (array-of-container guard) |
| Unregistered POD struct | `storage_representation_v<T> resolved to 'unsupported'` — Check: unregistered POD aggregate (use H5CPP_REGISTER_STRUCT) |

To see one fire, uncomment a `h5::write(fd, "/v", std::vector<bool>{1,0,1});` line — the compile error names the offending type and points at `H5Dwrite.hpp` (and the equivalent in the other three dispatch headers).

## Pretty-Print Format

Container output uses the `operator<<` overloads from `H5Uall.hpp`. Format conventions:

| Container shape           | Renders as                              |
| ------------------------- | --------------------------------------- |
| iterable (`vector`, `list`, `set`, ...) | `[a,b,c,d,...]`                  |
| `std::pair<K, V>`         | `{k:v}`                                 |
| `std::tuple<Ts...>`       | `<v0,v1,...,vN>`                        |
| `std::map<K, V>` (and friends) | `[{k1:v1},{k2:v2},...]`            |
| stack / queue adaptors    | `[top,...,bottom]` (non-destructive)    |

Long containers truncate at `H5CPP_CONSOLE_WIDTH` (default 10, this build uses 30) with a trailing `", ..."`. See `examples/pprint/README.md` for the full inserter story.

## Cross-References

- **`tasks/h5cpp-type-system-architecture-notes.md`** — the source of truth for the dispatch matrix (Section "Complete Write-Side Type Matrix" / "Complete Read-Side Type Matrix").
- **`examples/pprint/`** — focused demo of the `operator<<` inserters used by every verification line here.
- **`examples/compound/`** — the single-file version of the compound POD path that `compound.cpp` reuses.
- **`examples/container/`** — the same I/O dispatch surface for STL and *custom* (non-`std::`) containers.
- **`h5cpp/H5Mstl.hpp`** — the per-container access-traits specialisations.
