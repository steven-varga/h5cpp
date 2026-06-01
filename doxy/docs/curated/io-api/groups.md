@page curated_io_api_groups GROUPS

@brief Create, open, traverse, and link HDF5 groups — the
directory-like structure inside an HDF5 container. Includes the
`h5::ls` / `h5::dfs` / `h5::bfs` traversal helpers and the soft /
hard / external link API.

# Group operations

Groups are HDF5's namespace mechanism. They behave like POSIX
directories: each group can hold other groups, datasets,
attributes, and committed datatypes, addressed by `/`-separated
paths inside the file. The file's root group `/` is implicit and
always present.

Groups in H5CPP are also one of the standard
[attribute parents](@ref curated_io_api_attributes) — anything you
can write as an attribute on a dataset, you can also write on a
group (useful for per-group metadata: schema, source, time-range,
etc.).

## At a glance

| Operation                | Function                                                         | Returns                       |
| :----------------------- | :--------------------------------------------------------------- | :---------------------------- |
| Create new group         | `h5::gcreate(parent, path, lcpl?)`                                | `h5::gr_t`                    |
| Open existing group      | `h5::gopen(parent, path)`                                         | `h5::gr_t`                    |
| Close                    | RAII (let `h5::gr_t` destruct)                                    | —                             |
| **Check existence**      | `h5::exists(loc, path)`                                           | `bool`                        |
| **List children**        | `h5::ls(fd, path)`                                                | `std::vector<std::string>`    |
| **Depth-first traversal** | `h5::dfs(fd, path)`                                              | `std::vector<std::string>`    |
| **Breadth-first traversal** | `h5::bfs(fd, path)`                                            | `std::vector<std::string>`    |
| **Create soft link**     | `h5::link_soft(target_path, loc, link_name)`                      | `void`                        |
| **Create hard link**     | `h5::link_hard(target_loc, target_path, loc, link_name)`          | `void`                        |
| **Create external link** | `h5::link_external(target_file, target_path, loc, link_name)`     | `void`                        |
| **Move / rename**        | `h5::move(src_loc, src_name, dst_loc, dst_name)`                  | `void`                        |
| **Copy**                 | `h5::copy(src_loc, src_name, dst_loc, dst_name)`                  | `void`                        |
| **Unlink (delete)**      | `h5::unlink(loc, path)`                                           | `void`                        |
| Attribute on a group     | `h5::awrite(gr, "name", value)` or `gr["name"] = …`               | `h5::at_t`                    |

`gcreate` and `gopen` are templated on the parent type via
`h5::impl::is_valid_group_parent` (file `h5::fd_t` or group
`h5::gr_t`). Nested groups compose naturally —
`h5::gopen(gr, "sub")` returns a `gr_t` you can pass back as the
parent of another `gopen` / `gcreate`. The traversal and link APIs
take any `Loc` satisfying `h5::impl::is_valid_loc<Loc>` (file,
group, or other location handle).

---

@anchor curated_io_api_groups_gcreate
## `h5::gcreate` — create a new group

```cpp
template<class HID_T>
std::enable_if_t<h5::impl::is_valid_group_parent<HID_T>::value, h5::gr_t>
h5::gcreate(const HID_T& parent, const std::string& path,
            const h5::lcpl_t& lcpl = h5::default_lcpl);
```

With the default `h5::default_lcpl`, intermediate groups along
the path are created automatically (e.g. `gcreate(fd, "a/b/c")`
creates `a`, then `a/b`, then `a/b/c`) and link names are UTF-8
encoded.

**Throws** — `h5::error::io::group::create` on `H5Gcreate2` failure.

```cpp
h5::fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC);
h5::gr_t imu = h5::gcreate(fd, "sensors/imu");   // intermediates created
h5::gr_t ch0 = h5::gcreate(imu, "channel_0");    // nested
```

---

@anchor curated_io_api_groups_gopen
## `h5::gopen` — open an existing group

```cpp
template<class HID_T>
std::enable_if_t<h5::impl::is_valid_group_parent<HID_T>::value, h5::gr_t>
h5::gopen(const HID_T& parent, const std::string& path);
```

**Throws** — `h5::error::io::group::open` if the group does not exist.

```cpp
h5::fd_t fd = h5::open("data.h5", H5F_ACC_RDONLY);
h5::gr_t gr = h5::gopen(fd, "/sensors/imu");
```

---

@anchor curated_io_api_groups_exists
## `h5::exists` — does a path exist?

```cpp
template<class Loc>
std::enable_if_t<h5::impl::is_valid_loc<Loc>::value, bool>
h5::exists(const Loc& loc, const std::string& path);
```

Returns `true` if `path` resolves to **any** object (group,
dataset, datatype, link) relative to `loc`. Quiet on missing —
returns `false` rather than throwing.

```cpp
if (h5::exists(fd, "/config/version"))
    auto v = h5::aread<double>(fd, "/config/version");
else
    use_default();
```

The lightweight pattern for "does this child exist?" — avoids the
exception cost of the call-fail probe.

---

@anchor curated_io_api_groups_ls
## `h5::ls` — list children of a group

```cpp
inline std::vector<std::string> h5::ls(const h5::fd_t& fd,
                                        const std::string& directory);
```

Returns the names of every direct child of `directory` (one level
deep) in `H5_ITER_INC` order — same as `ls /path` on POSIX.

**Throws** — `std::runtime_error` on `H5Literate` failure (parent
not present, bad path).

```cpp
h5::fd_t fd = h5::open("data.h5", H5F_ACC_RDONLY);
for (auto& name : h5::ls(fd, "/sensors")) {
    std::cout << "  " << name << "\n";
}
```

---

@anchor curated_io_api_groups_traversal
## `h5::dfs` / `h5::bfs` — recursive traversal

```cpp
inline std::vector<std::string> h5::dfs(const h5::fd_t& fd,
                                         const std::string& directory);
inline std::vector<std::string> h5::bfs(const h5::fd_t& fd,
                                         const std::string& directory);
```

Recursive equivalents of `ls`: return every object's path
**relative to `directory`**, walked depth-first (`dfs`) or
breadth-first (`bfs`). Built on `H5Lvisit` with the matching
iteration order.

| Variant | When it wins                                                                |
| :------ | :-------------------------------------------------------------------------- |
| `dfs`   | "Process the whole subtree, leaf-by-leaf" — log analyzers, validators       |
| `bfs`   | "Find this object by name without descending unnecessarily" — search-style  |

```cpp
// Print every dataset under /experiments
for (auto& path : h5::dfs(fd, "/experiments")) {
    if (h5::exists(fd, "/experiments/" + path))   // skip soft-link dangling
        std::cout << path << "\n";
}
```

---

@anchor curated_io_api_groups_link_soft
## `h5::link_soft` — symbolic link

```cpp
template<class Loc>
std::enable_if_t<h5::impl::is_valid_loc<Loc>::value, void>
h5::link_soft(const std::string& target_path,
              const Loc& loc, const std::string& link_name,
              const h5::lcpl_t& lcpl = h5::default_lcpl);
```

Creates `loc/link_name` as a **symbolic link** to `target_path`.
Resolved at access time — if the target doesn't exist when the
link is created (or is later deleted), the link **dangles** silently
until accessed.

```cpp
// Make /current/data point to /experiments/2026-05-29/data
h5::link_soft("/experiments/2026-05-29/data", fd, "/current/data");
```

Soft links can point to anywhere — including paths that don't yet
exist, and across H5O_TYPE boundaries.

**Throws** — `std::runtime_error` on `H5Lcreate_soft` failure.

---

@anchor curated_io_api_groups_link_hard
## `h5::link_hard` — hard link (alias)

```cpp
template<class L1, class L2>
std::enable_if_t<h5::impl::is_valid_loc<L1>::value && h5::impl::is_valid_loc<L2>::value, void>
h5::link_hard(const L1& target_loc, const std::string& target_path,
              const L2& loc, const std::string& link_name,
              const h5::lcpl_t& lcpl = h5::default_lcpl);
```

Creates `loc/link_name` as a **second name** for the object at
`target_loc/target_path`. Both names refer to the **same** HDF5
object — no copy, no indirection, no dangling. Deleting one name
leaves the object reachable through the other.

```cpp
// Give /shared/spectra a second name at /experiment_1/data
h5::link_hard(fd, "/shared/spectra", fd, "/experiment_1/data");
```

Hard links can't span files (use external links for that) and
can't point at groups in some HDF5 versions.

---

@anchor curated_io_api_groups_link_external
## `h5::link_external` — cross-file reference

```cpp
template<class Loc>
std::enable_if_t<h5::impl::is_valid_loc<Loc>::value, void>
h5::link_external(const std::string& target_file, const std::string& target_path,
                  const Loc& loc, const std::string& link_name,
                  const h5::lcpl_t& lcpl = h5::default_lcpl);
```

Creates `loc/link_name` as a reference to an object in **another
HDF5 file**. The link is resolved by re-opening the target file at
access time. Useful for splitting large datasets across files
while keeping a unified `fd_t` view.

```cpp
// Reference an external file's dataset as if it were local
h5::link_external("shared.h5", "/global/calibration",
                  fd, "/local/cal");
auto cal = h5::read<arma::vec>(fd, "/local/cal");   // transparently reads from shared.h5
```

---

@anchor curated_io_api_groups_move
## `h5::move` / `h5::copy` — rename and duplicate

```cpp
template<class L1, class L2>
std::enable_if_t<h5::impl::is_valid_loc<L1>::value && h5::impl::is_valid_loc<L2>::value, void>
h5::move(const L1& src_loc, const std::string& src_name,
         const L2& dst_loc, const std::string& dst_name);

template<class L1, class L2>
std::enable_if_t<h5::impl::is_valid_loc<L1>::value && h5::impl::is_valid_loc<L2>::value, void>
h5::copy(const L1& src_loc, const std::string& src_name,
         const L2& dst_loc, const std::string& dst_name);
```

`h5::move` is `H5Lmove` — the link is moved without copying the
underlying object. Atomic on a single file.

`h5::copy` is `H5Ocopy` — the **object itself** is duplicated.
Suitable for cross-file copies; respects HDF5's `H5P_OBJECT_COPY`
property list for filter/chunk preservation.

```cpp
h5::move(fd, "/scratch/temp", fd, "/results/run_42");        // rename
h5::copy(src_fd, "/calibration", dst_fd, "/imported_cal");   // cross-file
```

---

@anchor curated_io_api_groups_unlink
## `h5::unlink` — remove a link

```cpp
template<class Loc>
std::enable_if_t<h5::impl::is_valid_loc<Loc>::value, void>
h5::unlink(const Loc& loc, const std::string& path);
```

Removes the link at `loc/path`. If it was the **last** reference
to the underlying object (no hard links pointing at it elsewhere),
the object itself becomes unreachable and HDF5 reclaims its space
on the next file repack.

```cpp
h5::unlink(fd, "/scratch/stale_run");
```

Note: HDF5 doesn't truly delete data on `H5Ldelete` — the bytes
remain in the file until you repack with `h5repack`. Plan
accordingly for sensitive data.

---

## Groups as attribute parents

Anything you can hang on a dataset, you can hang on a group:

```cpp
h5::gr_t cfg = h5::gcreate(fd, "/config");

h5::awrite(cfg, "schema_version", 2);
h5::awrite(cfg, "source", std::string{"sensor-7"});
cfg["units"]   = std::string{"m/s"};
cfg["axes"]    = {1, 2, 3};

auto v = cfg["schema_version"];                  // implicit-conversion read
auto s = h5::aread<std::string>(cfg, "source");
```

This is the canonical pattern for **per-section metadata**: create
a logical group, attach descriptive attributes, then put the
heavyweight datasets underneath.

@see @ref curated_io_api_attributes "ATTRIBUTES" — full attribute API

---

## Cross-references

- @ref link_handle_reference "Handles, Descriptors & Property Lists" — `h5::gr_t` taxonomy
- @ref link_property_lists "Property Lists" — `h5::lcpl_t` reference
- @ref link_error_handler "Error Handling" — `h5::error::io::group::*` hierarchy
- @ref curated_io_api_file "FILE" — `fd_t` the typical parent of a top-level group
- @ref curated_io_api_dataset "DATASET" — what you put inside a group
- @ref curated_io_api_attributes "ATTRIBUTES" — metadata on a group (same surface as on a dataset)
