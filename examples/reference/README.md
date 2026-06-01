@page example_guide_reference References — Region & Object Pointers Across Datasets

An HDF5 *reference* is a portable pointer-like value that names either a whole object (object reference) or a hyperslab inside a dataset (region reference). h5cpp wraps the HDF5 1.12+ generic reference type `H5R_ref_t` in `h5::reference_t`, and the same `h5::write` / `h5::read` shape used everywhere else applies:

```cpp
h5::reference_t ref = h5::reference(fd, "/01",
    h5::offset{2,2}, h5::count{4,4});            // region ref to a 4x4 hyperslab
h5::write(fd, "single reference", ref);          // scalar reference value
auto idx = h5::read<std::vector<h5::reference_t>>(ds);   // vector of references
```

A reference dataset is a normal HDF5 dataset whose element type is `H5T_STD_REF` (1.12+) — `h5cpp/H5Tall.hpp` declares that type via `dt_t<h5::reference_t>`, and the `access_traits_t<h5::reference_t>` block teaches the write/read dispatcher to treat a `reference_t` as a scalar object.

## Files

| File             | What it covers                                                                          |
| ---------------- | --------------------------------------------------------------------------------------- |
| `reference.cpp`  | Create a 10x20 chunked dataset; write one scalar region ref + a `vector<reference_t>` index; reopen, dereference each entry, write a constant through the region; reopen again, dereference, read each region back as `arma::fmat`. |

## RAII Lifecycle (HDF5 1.12+)

Each successful `h5::reference(...)` (which calls `H5Rcreate_region`) allocates an internal handle inside the `H5R_ref_t` blob. **Every** such handle must be released with `H5Rdestroy` or HDF5 spins forever in its library-shutdown path (`HDF5: infinite loop closing library`). `reference_t` carries the rule-of-five for this:

| Member                          | Behavior                                                                   |
| ------------------------------- | -------------------------------------------------------------------------- |
| `reference_t()`                 | zero-initializes the H5R_ref_t blob (empty)                                |
| `~reference_t()`                | `H5Rdestroy(&value)` if non-empty                                          |
| `reference_t(const reference_t&)`  | `H5Rcopy` — deep duplicate with its own internal handle                  |
| `operator=(const reference_t&)`    | destroy-then-copy                                                        |
| `reference_t(reference_t&&)`       | steals the bytes, zeroes the source (noexcept — vector-reallocation safe) |
| `operator=(reference_t&&)`         | destroy-then-move                                                        |
| `is_empty()` *(private)*           | `memcmp` against an all-zero `H5R_ref_t` sentinel                        |

The opaque `H5R_ref_t` blob remains memcpy-safe for HDF5's *on-disk* serializer (H5Dread / H5Dwrite go through `H5T_STD_REF` and re-allocate fresh handles per read element). The rule-of-five governs only *in-memory* C++ copy/move; the disk path is unaffected.

The pre-1.12 path (`hdset_reg_ref_t`) is a plain POD with no internal handle — no destructor needed; that branch of `reference_t` stays an aggregate.

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-reference
./examples-reference
```

Expected output (the final dataset is truncated by `arma`'s default printer):

```
✔ ok    write scalar region reference
✔ ok    write index of 4 region references
✔ ok    dereference + write color through each region
✔ ok    dereference + read each region back as arma::fmat

[9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9]    // 4x4 region, all 9
[9]                                  // 1x1 region
[9,9,9,9,9,9,9,9,9]                  // 3x3 region
[9,9]                                // 2x1 region

[1,1,1,1, ... ]                      // full 10x20 dataset, mostly fill_value=1
```

The example exits cleanly (exit 0). With the RAII pass on `reference_t`, the previous `HDF5: infinite loop closing library` shutdown crash is gone.

## Known h5dump / h5ls Limitation

`h5dump` and `h5ls` crash or report `*ERROR*` on the reference datasets:

```
$ h5ls -r ref.h5
/01                      Dataset {10, 20}
/index                   Dataset *ERROR*
/single\ reference       Dataset *ERROR*
$ h5dump -H ref.h5
h5dump error: internal error (file h5dump.c:line 1432)
```

This is an **HDF5 1.12 tool limitation**, not an issue with the file or with h5cpp. The HDF5 command-line tools weren't updated for the `H5R_ref_t` opaque type until HDF5 1.14. The data itself is structurally correct — `examples-reference` reads its own references back successfully, and `h5py` (rebuilt against ≥1.12) can load them too.

If `h5dump` interop is required, the choices are:
1. Run h5dump from HDF5 ≥ 1.14
2. Use the deprecated `H5R_DATASET_REGION` API (which 1.12's h5dump *does* understand) — incompatible with this project's policy of building with compatibility macros removed.

## CMake Wiring

```cmake
add_h5cpp_example(reference reference/reference.cpp LIBRARIES libarmadillo)
```

Lives in `examples/CMakeLists.txt:440`. Single source file, single library dependency (Armadillo, only for the `arma::fmat` readback type at the end).

## Build State (as of HEAD)

| Target                | Status                                                                |
| --------------------- | --------------------------------------------------------------------- |
| `examples-reference`  | ✔ ok — region-reference round-trip works end-to-end, exits cleanly    |

The previous failure modes — `static_assert: storage_representation_v<reference_t> resolved to 'unsupported'` at compile time and `HDF5: infinite loop closing library` at runtime — are both resolved in `h5cpp/H5Tall.hpp` (storage-rep registration + rule-of-five).

## Cross-References

- **`h5cpp/H5Tall.hpp`** — `reference_t` struct, its rule-of-five, the `H5T_STD_REF` datatype binding, and the `access_traits_t` / `storage_representation_impl` registrations.
- **`h5cpp/H5Rreference.hpp`** — low-level `h5::impl::reference::*` helpers (`create_region`, `open_object`, `open_region`, `copy`, `destroy`, `reclaim`) over the HDF5 C API.
- **`h5cpp/H5Rregion.hpp`** — public `h5::reference(...)` factory and the experimental `h5::exp::write` / `h5::exp::read<T>` paths that drive partial I/O through a reference.
- **`examples/datasets/`** — the same `offset` / `count` / `stride` / `block` hyperslab vocabulary used to define the regions a reference points at.

## Source

- [`reference.cpp`](reference_8cpp-example.html) — rendered with syntax highlighting
