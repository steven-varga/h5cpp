@page curated_io_api_file_create h5::create

@brief Create a new HDF5 file at `path`. RAII-managed `h5::fd_t`
returned; no explicit close.

```cpp
h5::fd_t h5::create(const std::string& path,
                    unsigned flags,
                    const h5::fcpl_t& fcpl = h5::default_fcpl,
                    const h5::fapl_t& fapl = h5::default_fapl);
```

Creates a new HDF5 container at `path`. The `flags` argument controls
overwrite behaviour; `fcpl` controls the on-disk meta-data layout;
`fapl` controls the file driver and parallel I/O settings.

**Parameters**

| Name       | Type                  | Description                                                                                                                |
| :--------- | :-------------------- | :------------------------------------------------------------------------------------------------------------------------- |
| `path`     | `const std::string&`  | OS file system path to the new HDF5 container.                                                                             |
| `flags`    | `unsigned`            | One of `H5F_ACC_TRUNC` (overwrite if exists), `H5F_ACC_EXCL` (fail if exists), `H5F_ACC_DEBUG` (verbose CAPI diagnostics). |
| `fcpl`     | `const h5::fcpl_t&`   | File-creation property list. Defaults to `h5::default_fcpl`.                                                               |
| `fapl`     | `const h5::fapl_t&`   | File-access property list. Defaults to `h5::default_fapl`. For parallel I/O, pass an MPI-aware FAPL.                       |

**Returns** — `h5::fd_t` RAII handle; closes via `H5Fclose` on scope exit.

**Throws**

| Exception                                | When                                                  |
| :--------------------------------------- | :---------------------------------------------------- |
| `h5::error::io::file::create`            | `H5Fcreate` failed (path unwritable, EXCL conflict, …) |
| `h5::error::property_list::misc`         | Invalid `fcpl` or `fapl` was supplied.                |

**Example**

```cpp
// Trivial overwrite — most common pattern.
h5::fd_t fd = h5::create("example.h5", H5F_ACC_TRUNC);

// Fail-if-exists semantics.
h5::fd_t fresh = h5::create("first-run.h5", H5F_ACC_EXCL);

// With a tuned FAPL — e.g., MPI-IO for parallel writes.
h5::fapl_t fapl = h5::fapl{
    h5::driver::mpi{MPI_COMM_WORLD, MPI_INFO_NULL}};
h5::fd_t pfd = h5::create("parallel.h5", H5F_ACC_TRUNC,
                          h5::default_fcpl, fapl);
```

> **DO** capture the result into `h5::fd_t` (or `auto`), then pass
> through `static_cast<::hid_t>(fd)` when interoping with raw CAPI.
> **DON'T** assign directly to a `::hid_t` variable — the temporary
> `h5::fd_t` closes the handle as it goes out of scope, invalidating
> the raw id:
> ```cpp
> ::hid_t bad = h5::create("x.h5", H5F_ACC_TRUNC);   // BAD: handle dead immediately
> ```

@see @ref curated_io_api_file_open, @ref curated_io_api_file
