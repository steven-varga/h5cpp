@page example_guide_utf UTF-8 — Unicode Throughout the Identifier Surface

HDF5 1.8+ supports UTF-8 across every place an object can be *named* — file
names, group names, dataset names, attribute names — plus the byte content of
string-typed attributes and datasets. h5cpp opts into UTF-8 by default on
every link and attribute it creates, so the call shape is the same regardless
of script:

```cpp
h5::fd_t fd = h5::create(u8"こんにちは世界.h5", H5F_ACC_TRUNC,
    h5::default_fcpl,
    h5::libver_bounds({H5F_LIBVER_V18, H5F_LIBVER_V18}));

arma::mat M = arma::ones(3, 4);
h5::write(fd, u8"温度/مجموعة/données", M);   // mixed-script nested path
```

This example exercises UTF-8 in the *naming* surface across 14 scripts plus
a three-script nested group path. For UTF-8 *content* round-trip inside
string datasets and string attribute values see `examples/string/`.

## What HDF5 supports / what h5cpp wires up

HDF5 carries a character-set bit on each link and attribute (`H5T_CSET_ASCII`
or `H5T_CSET_UTF8`), set via the link-creation and attribute-creation property
lists (LCPL / ACPL). h5cpp ships UTF-8 as the default on both:

| Property list       | h5cpp default (`H5Pall.hpp`)                                        |
| ------------------- | -------------------------------------------------------------------- |
| `h5::default_lcpl`  | `H5T_CSET_UTF8` + `create_intermediate_group{1}`                     |
| `h5::default_acpl`  | `H5T_CSET_UTF8`                                                      |

The alternative `H5T_CSET_ASCII` exists in HDF5 and is the C-library default,
but h5cpp never selects it — every link h5cpp creates carries the UTF-8 bit,
so downstream tools that branch on `H5Lget_info` / `H5Aget_info` cset see
UTF-8 consistently.

## Identifier surface covered

| Surface                              | Example from the demo                                                |
| ------------------------------------ | -------------------------------------------------------------------- |
| File name                            | `こんにちは世界.h5`                                                    |
| Top-level dataset name               | 14 scripts (`مرحبا بالعالم`, `Բարեւ աշխարհ`, `你好，世界`, etc.)        |
| Attribute name                       | each dataset has an attribute with the same UTF-8 name as the dataset |
| Attribute string value               | each attribute's value equals its own name — three identical UTF-8 byte sequences across dataset name, attribute name, and attribute content |
| Nested group path with mixed scripts | `温度/مجموعة/données` (Chinese / Arabic / French)                     |

The dataset payload is `arma::ones(3, 4)`; the UTF-8 features themselves are
agnostic to the payload type. The nested-path stage uses an `arma::Col<int>`
to keep the readback distinguishable from the matrix datasets.

## `libver_bounds(V18, V18)`

The demo pins the file format to HDF5 1.8 on both lower and upper bound:

```cpp
h5::libver_bounds({H5F_LIBVER_V18, H5F_LIBVER_V18})
```

UTF-8 in identifiers has been supported since HDF5 1.8 itself, so this
constraint is inert for what the example exercises. It matters for
downstream interop: pinning to 1.8 keeps the resulting file readable by
1.8-era tools (h5py rebuilt against ≥1.8, h5dump 1.8+, MATLAB ≥ R2011a,
Julia `HDF5.jl`). Newer file-format features (`H5R_ref_t`, complex variable-
length encodings, etc.) would be rejected at write-time under this bound —
intentional, the example trades them away for portability.

`h5::libver_bounds` is defined in `h5cpp/H5Pall.hpp`.

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-utf
./examples-utf
```

Expected output:

```
✔ ok      file opens by UTF-8 filename
✔ ok      hello world
✔ ok      مرحبا بالعالم
✔ ok      Բարեւ աշխարհ
✔ ok      Здравей свят
✔ ok      Прывітанне Сусвет
✔ ok      မင်္ဂလာပါကမ္ဘာလောက
✔ ok      你好，世界
✔ ok      Γειά σου Κόσμε
✔ ok      હેલ્લો વિશ્વ
✔ ok      Helló Világ
✔ ok      こんにちは世界
✔ ok      안녕 세상
✔ ok      سلام دنیا
✔ ok      העלא וועלט
✔ ok      UTF-8 nested group path: 温度/مجموعة/données

✔ all checks passed, errors=0
```

Exit code is the number of failed checks; the example fails its own gate if
any UTF-8 identifier disagrees on round-trip.

## Files

| File      | What it covers                                                                                                                                                                       |
| --------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `utf.cpp` | A 14-script loop that writes one dataset + one same-named attribute per phrase, plus a mixed-script nested group path (`温度/مجموعة/données`). Reopens the file by its UTF-8 filename and round-trips every identifier. |

## Interop notes

- **`h5dump`** shows the UTF-8 names correctly when stdout is a UTF-8 terminal. When piped or non-TTY, it escapes the multi-byte sequences as octal (`\343\201\223...`) — this is h5dump's display choice, not a corruption; the bytes on disk are still raw UTF-8.
- **h5py** round-trips UTF-8 identifiers natively: `f[u"مرحبا بالعالم"][...]` works on a file produced by this example without conversion.
- **Julia `HDF5.jl`** opens UTF-8-named files via `h5open("こんにちは世界.h5", "r")` and accesses datasets by their UTF-8 path with no extra step.
- **Windows caveat**: HDF5 1.12 on Windows expects UTF-8 passed to `H5Fcreate`, but the OS layer may convert the filename to the active code page before it reaches the filesystem. Projects targeting Windows interop should round-trip a non-ASCII filename on the target box before relying on it.

## CMake Wiring

```cmake
add_h5cpp_example(utf utf/utf.cpp LIBRARIES libarmadillo)
```

Lives in `examples/CMakeLists.txt:446-447`. The Armadillo dependency is
incidental — only `arma::mat` and `arma::Col<int>` are used as the payload
containers. The UTF-8 features themselves require nothing beyond `<h5cpp/all>`
and HDF5 ≥ 1.8.

## Build State (as of HEAD)

| Target          | Status                                                       |
| --------------- | ------------------------------------------------------------ |
| `examples-utf`  | ✔ ok — 16 UTF-8 round-trip checks pass, exit 0               |

## Cross-References

- **`h5cpp/H5Pall.hpp`** — `h5::default_lcpl` / `h5::default_acpl` set `H5T_CSET_UTF8`; `h5::libver_bounds` for file-format pinning.
- **`examples/string/`** — string *content* round-trip (this example covers the naming surface; that one covers the byte content of values, including non-ASCII UTF-8 in scalar and vector string datasets and attributes).
- **`examples/groups/`** — group creation and intermediate-path semantics; the nested mixed-script path here exercises the same `create_intermediate_group{1}` machinery from the default LCPL.
- **HDF5 file-creation property reference** — https://support.hdfgroup.org/documentation/hdf5/latest/group___f___c_p_l.html

## Source

- [`utf.cpp`](utf_8cpp-example.html) — rendered with syntax highlighting
