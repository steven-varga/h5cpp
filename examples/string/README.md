# Strings — VLEN Round-Trip Across Datasets and Attributes

HDF5 strings are not a single thing — the format has four orthogonal axes
(length, character set, padding, element type). h5cpp picks one canonical
setting on each axis so the call shape stays the same as for any other type:

```cpp
std::string s = "hello, world";
h5::write(fd, "/path", s);                          // VLEN H5T_C_S1, scalar
auto back = h5::read<std::string>(fd, "/path");     // round-trips bit-exact

std::vector<std::string> rows = ...;
h5::write(fd, "/rows", rows);                       // rank-1 VLEN dataset
h5::awrite(ds, "author", std::string("Steven"));    // VLEN string attribute
```

This example wires every C++ string type h5cpp binds to HDF5 into one
self-checking demo. Eight stages, each prints `✔ ok` or `✘ failed`, and
the final tally returns non-zero from `main` if any check disagrees with
its source value.

## HDF5 file-format axes vs h5cpp's choices

| Axis              | HDF5 options                                                              | h5cpp's pick                                                                                              |
| ----------------- | ------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------- |
| Length            | fixed (`H5Tset_size(N)`) or variable (`H5Tset_size(H5T_VARIABLE)`)         | **VLEN** for `std::string` / `std::string_view` / `const char*`; **fixed** for `char[N]` inside compounds |
| Character set     | `H5T_CSET_ASCII` (default) / `H5T_CSET_UTF8`                              | **UTF-8** on the attribute path; **ASCII** on the dataset-creation path (both accept raw UTF-8 bytes)     |
| Padding           | `H5T_STR_NULLTERM` (C) / `H5T_STR_NULLPAD` (Fortran) / `H5T_STR_SPACEPAD` | **NULLTERM** (HDF5 default; h5cpp never calls `H5Tset_strpad`)                                            |
| Element type      | char / wchar / 16 / 32 on disk                                            | **char only** — `wchar_t` / `char16_t` / `char32_t` strings are intentionally unsupported (portability)   |

The cset asymmetry between datasets and attributes is a known minor
inconsistency — the on-disk byte content is identical either way because
ASCII is a subset of UTF-8, but a strict downstream tool checking the
H5Tget_cset bit would see different values across the two paths.

## C++ types h5cpp wires up

| C++ type                              | HDF5 mapping                                | Dataset | Attribute | Inside compound |
| ------------------------------------- | ------------------------------------------- | ------- | --------- | --------------- |
| `std::string` / `std::basic_string<char,…>` | `H5T_C_S1` + `H5T_VARIABLE`, scalar dataspace | ✔ ok    | ✔ ok      | ◇ partial — `dt_t<string>` is registered but content round-trip through `std::tuple<…,string,…>` returns empty strings on read |
| `std::string_view`                    | same VLEN encoding                          | ✔ ok    | ✔ ok      | ◇ partial — same caveat as `std::string` |
| `const char*` / `char*`               | VLEN UTF-8 via `H5CPP_REGISTER_TYPE_(char*, H5T_C_S1)` | ○ na — top-level write/read needs explicit `h5::count{}` because the dispatcher routes pointers through the count-required overload | ✔ ok | ✔ ok |
| `char[N]` literal                     | fixed-length `H5T_C_S1` size N              | ○ na — same pointer-decay reason; use `std::string(literal)` instead | ✔ ok | ✔ ok |
| `std::vector<std::string>`            | rank-1 VLEN-string dataset (`storage_representation_t::vlen_text_dataset`); partial IO via `h5::offset` / `h5::count` honored at the file_space selection | ✔ ok    | ✔ ok      | n/a               |
| `std::wstring` / `std::u16string` / `std::u32string` | named in `is_string` but no `H5CPP_REGISTER_TYPE_` for `wchar_t*` / `char16_t*` / `char32_t*` | ✘ unsupported | ✘ unsupported | ✘ unsupported |

The wide-char case is the one bear-trap. The `is_string` trait at
`H5Tmeta.hpp:69-76` claims to match `std::basic_string<wchar_t>` and the
UTF-16/UTF-32 variants, but the actual dispatch path only has type
registrations for `char*` / `const char*` — a `std::wstring` write today
resolves to `storage_representation_t::unsupported` and trips the
generic "unregistered POD aggregate" static_assert, not a string-specific
diagnostic. Fixable with two more `H5CPP_REGISTER_TYPE_(wchar_t*, …)`
lines, but HDF5's own story on `wchar_t` is non-portable (sizeof differs
on Windows vs Linux), so the current "char-only" position is defensible
boring-tech. UTF-8 in a `std::string` covers every Unicode codepoint
without the portability tax.

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-string
./examples-string
```

Expected output:

```
✔ ok      std::string scalar (VLEN)
✔ ok      non-ASCII UTF-8 content (Greek / Japanese / emoji / math)
✔ ok      std::vector<std::string> dataset (20 rows)
✔ ok      partial IO via offset=5, count=8 (contiguous slice)
✔ ok      scalar std::string attribute
✔ ok      ASCII attribute under UTF-8 cset
✔ ok      std::vector<std::string> attribute (4 tags)
✔ ok      ds["name"] = string operator-= sugar

pretty-print of /bulk/rows via h5cpp's STL streamer:
  [DlFDlMyNCqEd,PCvqbWNVdeORS, … vEgXuUMGOGWGjzMcZ]

✔ all checks passed, errors=0
```

Exit code is the number of failed checks; the example fails its own gate
if any string round-trip disagrees with the source.

## Files

| File         | What it covers                                                              |
| ------------ | --------------------------------------------------------------------------- |
| `string.cpp` | Eight stages, each a self-checking round-trip: scalar `std::string`, non-ASCII UTF-8, `std::vector<std::string>` dataset + attribute, partial-slice idiom, two scalar attributes, the `ds["name"]=value` operator sugar, and the STL pretty-printer. |

## Known limitations

- **`std::string` inside `std::tuple` or registered POD compounds** compiles and writes, but the string fields read back empty. `dt_t<std::string>` is registered (`H5Tall.hpp:192-202`) so the compound type-creation step succeeds, but the runtime data path doesn't serialize VLEN-string content into compound elements correctly. Numeric fields in the same tuple round-trip fine — `std::tuple<int, double>` works.
- **`const char*` / `char[N]` at the top-level write/read boundary** route to the pointer overload that requires `h5::count{}`. Pass a `std::string(literal)` instead at the dataset level; both work transparently inside attributes and compounds.

### Partial-IO semantics (h5cpp convention vs HDF5 hyperslab)

h5cpp's `h5::count{N}` means "I want N elements total"; the wrapper at `H5capi.hpp:136` then expresses that to HDF5 as `block=N, count=1` (one block of N contiguous elements). To pick a true strided / non-contiguous slice you have to pass `h5::block{1}` explicitly and let `h5::count{N}` be the HDF5 count — the same caveat the numeric pointer-read path has had since 2018. The VLEN-string read path now goes through the same wrapper, so its semantics match the numeric path exactly:

```cpp
// 8 contiguous strings starting at index 5
auto sub = h5::read<std::vector<std::string>>(fd, "/rows",
              h5::offset{5}, h5::count{8});
```

For strided picks across all paths, a unified `h5::stride{S}, h5::count{N}, h5::block{1}` UX would need to land in the wrapper itself; that's a separate cleanup.

## CMake Wiring

```cmake
add_h5cpp_example(string string/string.cpp)
```

Lives in `examples/CMakeLists.txt:434`. No library dependencies — only the
`<h5cpp/all>` umbrella and standard library.

## Build State (as of HEAD)

| Target             | Status                                                                |
| ------------------ | --------------------------------------------------------------------- |
| `examples-string`  | ✔ ok — eight string round-trip checks pass, exit 0                    |

## Cross-References

- **`h5cpp/H5Tall.hpp:182-213`** — `H5CPP_REGISTER_TYPE_(char*, H5T_C_S1)` macro expansions and the explicit `dt_t<std::basic_string>` spec that sets `H5T_VARIABLE` + `H5T_CSET_UTF8`.
- **`h5cpp/H5Tmeta.hpp:69-76`** — `is_string` trait (claims wide-char support; see caveat above).
- **`h5cpp/H5Tmeta.hpp:104-116`** — `is_fixed_text_like` / `is_vl_text_like` / `is_text_like` — the per-purpose narrower traits the dispatcher actually uses.
- **`h5cpp/H5Tmeta.hpp:230-241`** — `storage_representation_impl` for `std::basic_string<char,…>` (vlen_text_dataset) and `char*` / `const char*` (vlen_text_dataset).
- **`h5cpp/H5Dwrite.hpp:361-388`** — scalar text branch in the ds-overload (`H5Screate(H5S_SCALAR)` + `H5Tset_size(H5T_VARIABLE)`).
- **`h5cpp/H5Dwrite.hpp:780-792`** — scalar text branch in the fd-overload (dataset creation, scalar dataspace).
- **`h5cpp/H5Dread.hpp:701-726`** — scalar text return-style read (`char*` relay + `H5Treclaim`).
- **`h5cpp/H5Awrite.hpp:92-107`** — scalar text attribute write (uses `dt_t<char*>` with UTF-8 cset).
- **`h5cpp/H5Uall.hpp`** — the `operator<<` overloads that pretty-print `std::vector<std::string>` to stdout.
- **`examples/compound/`** — strings inside POD aggregates via the `H5CPP_REGISTER_STRUCT` macro (the workaround for the tuple-with-string limitation).
- **`examples/datasets/`** — full `offset` / `count` / `stride` / `block` hyperslab vocabulary that works on numeric datasets, blocked here on VLEN strings.
