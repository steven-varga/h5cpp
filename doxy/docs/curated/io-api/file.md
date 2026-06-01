@page curated_io_api_file FILE

@brief Open or create HDF5 files. RAII-managed handles close
automatically on scope exit.

# File operations

H5CPP exposes two free functions for the HDF5 file lifecycle —
`h5::open` and `h5::create` — both returning an `h5::fd_t` RAII
handle. There is no explicit `h5::close(fd)`: let the handle go
out of scope and `H5Fclose` runs automatically.

`h5::fd_t` is the entry point for everything else — datasets,
attributes, and groups all open relative to an `fd_t`. See
@ref handle_ref_objects "Object handles" for the broader handle
taxonomy and the conversion rules to/from raw `::hid_t`.

## Operations

- @subpage curated_io_api_file_create — create a new file
- @subpage curated_io_api_file_open — open an existing file
- Close: RAII (let `h5::fd_t` destruct, or `static_cast<::hid_t>` to consume)

Both calls take optional [property lists](@ref link_property_lists)
to tune low-level behaviour (cache size, MPI communicator, file
driver, …); the defaults work for the common cases.

## Notes on lifetime

`h5::fd_t` follows the rule-of-five RAII contract from
@ref handle_ref_objects. Copies share the underlying HDF5 reference
(via `H5Iinc_ref`); moves transfer ownership without bumping the
refcount; destruction calls `H5Fclose` only when the last reference
goes away.

Practical implication: passing an `h5::fd_t` by value to a function
is cheap (refcount bump, no copy of file data) and the file stays
open for as long as any handle exists.

```cpp
void process(h5::fd_t fd) {                 // by value: refcount bump
    auto v = h5::read<std::vector<float>>(fd, "/grid");
}                                            // refcount decrements; file stays open if caller still holds

h5::fd_t outer = h5::open("data.h5", H5F_ACC_RDONLY);
process(outer);                              // outer still valid here
```

## Cross-references

- @ref handle_ref_objects "Object handles" — the broader `h5::*_t` taxonomy
- @ref link_conversion_policy "Conversion Policy" — `h5::fd_t` ↔ `::hid_t`
- @ref link_property_lists "Property Lists" — `fcpl_t` / `fapl_t` reference
- @ref link_error_handler "Error Handling" — the `h5::error::io::file::*` hierarchy
- @ref curated_io_api_dataset "DATASET" — read/write/append on an open file
- @ref curated_io_api_attributes "ATTRIBUTES" — metadata attached to file / groups / datasets
- @ref curated_io_api_groups "GROUPS" — directory-like structure within a file
