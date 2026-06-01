@page curated_io_api_file_open h5::open

@brief Open an existing HDF5 file at `path`. RAII-managed `h5::fd_t`
returned.

```cpp
h5::fd_t h5::open(const std::string& path,
                  unsigned flags,
                  const h5::fapl_t& fapl = h5::default_fapl);
```

Opens an existing HDF5 container at `path`. The `flags` argument
controls the access mode (read-only vs read-write); `fapl` selects the
file driver and tunes access-level behaviour.

**Parameters**

| Name      | Type                  | Description                                                                       |
| :-------- | :-------------------- | :-------------------------------------------------------------------------------- |
| `path`    | `const std::string&`  | OS file system path to an existing HDF5 container.                                |
| `flags`   | `unsigned`            | `H5F_ACC_RDONLY` (read-only) or `H5F_ACC_RDWR` (read-write).                      |
| `fapl`    | `const h5::fapl_t&`   | File-access property list. Defaults to `h5::default_fapl`.                        |

**Returns** — `h5::fd_t` RAII handle.

**Throws**

| Exception                                | When                                          |
| :--------------------------------------- | :-------------------------------------------- |
| `h5::error::io::file::open`              | File does not exist, permission denied, magic-byte mismatch, unsupported on-disk format. |
| `h5::error::property_list::misc`         | Invalid `fapl` was supplied.                  |

**Example**

```cpp
// Read-only — safe default for inspection / analysis.
h5::fd_t fd = h5::open("dataset.h5", H5F_ACC_RDONLY);
auto data = h5::read<std::vector<float>>(fd, "/grid/values");

// Read-write — required if you'll be writing or appending.
h5::fd_t rw = h5::open("log.h5", H5F_ACC_RDWR);
h5::awrite(rw, "schema_version", 1.4);
```

@see @ref curated_io_api_file_create, @ref curated_io_api_file
