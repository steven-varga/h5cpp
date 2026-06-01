@page example_guide_groups Groups

A group is HDF5's directory. The point of this example is simple: building hierarchies, attaching attributes to them, listing what's inside, linking objects together, and handling the errors all fit on one page of h5cpp.

The whole vocabulary fits on one slide:

```cpp
h5::gcreate(parent, path [, lcpl, gcpl, gapl])     // create a group
h5::gopen(parent, path)                             // open an existing group

h5::ls(fd, path)                                    // immediate children
h5::dfs(fd, path)                                   // recursive: depth-first
h5::bfs(fd, path)                                   // recursive: breadth-first

h5::exists(loc, path)                               // bool: object/link exists?
h5::link_soft(target, link_loc, link_name)          // POSIX-symlink semantics
h5::link_hard(target_loc, target, link_loc, name)   // second name, same file
h5::link_external(target_file, target, loc, name)   // soft link to another file

h5::move(src_loc, src, dst_loc, dst)                // rename or relocate
h5::copy(src_loc, src, dst_loc, dst)                // deep copy
h5::unlink(loc, path)                               // delete a link

h5::awrite(parent, name, value)                     // attach an attribute
```

Every entry above is now wrapped — no raw `H5L*` / `H5O*` calls needed.

## Files

| File         | Purpose                                                            |
| ------------ | ------------------------------------------------------------------ |
| `groups.cpp` | Seven sections exercising the group surface                         |
| `groups.h5`  | Output container (`h5dump -n groups.h5` shows the full tree)       |

## Includes

```cpp
#include <h5cpp/all>
```

Nothing else needed. Groups are part of the core h5cpp surface.

## Anatomy

An HDF5 group has two pieces of header data plus attributes:

| HDF5 concept            | What it is                                                 |
| ----------------------- | ---------------------------------------------------------- |
| Header                  | Name + attribute list                                      |
| Symbol table            | Names of the objects that belong to the group (datasets, sub-groups, links) |
| Attributes              | Small named scalars/vectors attached to the group itself   |

A group's symbol table is what `h5::ls` returns. The header carries the attributes.

## 1. Creating groups

`h5cpp`'s `gcreate` takes any parent that can own children (an `fd_t` or another `gr_t`), a path string, and optional property lists.

```cpp
// Implicit: h5::default_lcpl already has create_intermediate_group set,
// so missing parents (/sensors/imu) get created automatically.
h5::gr_t a = h5::gcreate(fd, "/sensors/imu/left");

// Explicit: build the lcpl by hand if you want different defaults.
h5::lcpl_t lcpl = h5::char_encoding{H5T_CSET_UTF8}
                | h5::create_intermediate_group{1};
h5::gr_t b = h5::gcreate(fd, "/sensors/lidar", lcpl);
```

## 2. Nested groups (group as parent)

`gr_t` is a valid parent too. Use relative paths to extend a sub-tree without retyping the absolute prefix:

```cpp
h5::gr_t imu_left = h5::gopen(fd, "/sensors/imu/left");
h5::gcreate(imu_left, "calibration");   // → /sensors/imu/left/calibration
h5::gcreate(imu_left, "raw");           // → /sensors/imu/left/raw
```

This is how you build hierarchies incrementally.

## 3. Open + existence check

`h5::exists(loc, path)` returns `bool` (no exception on missing path). Check before opening to avoid catching exceptions in normal control flow.

```cpp
for (const auto& path : {"/sensors/imu/left", "/sensors/imu/right", "/sensors/lidar"}) {
    if (h5::exists(fd, path))
        h5::gr_t gr = h5::gopen(fd, path);  // safe — present
}
```

`loc` can be any h5cpp handle (file, group, raw `hid_t`); `path` is resolved relative to it.

## 4. Group attributes

`h5::awrite(parent, name, value)` accepts scalars, strings, vectors, and initializer lists. Works on any handle that can carry attributes — groups, datasets, named datatypes.

```cpp
h5::gr_t gr = h5::gopen(fd, "/sensors/imu/left");

h5::awrite(gr, "temperature_C", 42.0);
h5::awrite(gr, "unit", std::string("celsius"));
h5::awrite(gr, "channels", std::vector<int>{0, 1, 2, 3, 4, 5});
h5::awrite(gr, "axes", std::initializer_list<const char*>{"x", "y", "z"});
```

The bracket-assignment shorthand `gr["name"] = value` is a `ds_t`-only convenience — for `gr_t`, use `awrite` directly.

## 5. Listing group contents

Three traversal flavours, all returning `std::vector<std::string>` and streamable via the STL pretty-printer:

| Function                   | Scope                | Order                    |
| -------------------------- | -------------------- | ------------------------ |
| `h5::ls(fd, path)`         | immediate children   | name-sorted              |
| `h5::dfs(fd, path)`        | recursive            | depth-first, name-sorted |
| `h5::bfs(fd, path)`        | recursive            | breadth-first            |

```cpp
std::cout << "ls(/sensors)  -> " << h5::ls(fd, "/sensors")  << "\n";
std::cout << "dfs(/sensors) -> " << h5::dfs(fd, "/sensors") << "\n";
std::cout << "bfs(/sensors) -> " << h5::bfs(fd, "/sensors") << "\n";
```

Sample output:

```text
ls(/sensors)  -> [imu,lidar]
dfs(/sensors) -> [imu,imu/left,imu/left/calibration,imu/left/raw,lidar]
bfs(/sensors) -> [imu,lidar,imu/left,imu/left/calibration,imu/left/raw]
```

`dfs` is `H5Lvisit` directly. `bfs` is `dfs` then a stable-sort by slash-count — same set of paths, BFS order. Use `bfs` when you want shallower paths first (e.g. depth-limited output).

## 6. Links — soft, hard, external

HDF5 has three link flavours, all wrapped:

| Link type   | Semantics                                                   | h5cpp call              |
| ----------- | ----------------------------------------------------------- | ----------------------- |
| **soft**    | POSIX-symlink: stores a path string, resolved lazily        | `h5::link_soft`         |
| **hard**    | Additional name pointing at the same object, same file      | `h5::link_hard`         |
| **external**| Soft link whose target lives in another HDF5 file           | `h5::link_external`     |

```cpp
// soft: /shortcuts/imu_l -> /sensors/imu/left
h5::link_soft("/sensors/imu/left", fd, "/shortcuts/imu_l");

// hard: /shortcuts/imu_l_alias is a second name for the same group
h5::link_hard(fd, "/sensors/imu/left", fd, "/shortcuts/imu_l_alias");

// external: /external/peer -> /sensors/imu/left in external.h5
//           (target file doesn't have to exist at creation time)
h5::link_external("external.h5", "/sensors/imu/left", fd, "/external/peer");
```

All three accept an optional trailing `lcpl` argument; the default is `h5::default_lcpl`, which has intermediate-group creation enabled — so missing parents (`/shortcuts`, `/external`) auto-create.

Soft links are followed transparently — `h5::gopen(fd, "/shortcuts/imu_l")` opens the target group.

Dangling soft links (broken paths) and missing external files don't fail at link-creation time. They fail at open time. Use `H5Lget_info` directly if you need to distinguish link type without opening the target.

## 7. Move, copy, unlink

Three more wrappers, same shape:

```cpp
// Rename (or move within the file): /tmp/sketch -> /archive/v1
h5::move(fd, "/tmp/sketch", fd, "/archive/v1");

// Deep copy: clones the object plus all attributes and nested children.
// Source and destination locations may be in different files.
h5::copy(fd, "/sensors/imu/left", fd, "/archive/imu_snapshot");

// Drop a link. Target object survives if other names still reach it.
h5::unlink(fd, "/shortcuts/imu_l_alias");
```

Behavior worth knowing:

- **`h5::move`** with `src_loc == dst_loc` is a rename. Cross-loc moves are HDF5's `H5Lmove`; cross-*file* moves aren't supported by HDF5 (use copy + unlink).
- **`h5::copy`** copies the object's group hierarchy and attributes by default. Layout customisation (shallow, expand soft links, etc.) is available via `h5::shallow_hierarchy` / `h5::expand_soft_link` / … property-list flags from `H5Pall.hpp` — pass them through `H5Ocopy`'s ocpl directly if you need them (not exposed in the wrapper yet).
- **`h5::unlink`** is HDF5's `H5Ldelete`. The object is reclaimed when its last name drops. Hard-linked objects survive as long as one name remains.

## 8. Error handling — specific vs umbrella exceptions

h5cpp's exception hierarchy lets you catch precisely or broadly:

```cpp
h5::mute();   // suppress CAPI error stack for deliberate failures

try {                                          // specific
    h5::gcreate(fd, "/sensors/imu/left");      // already exists
} catch (const h5::error::io::group::create& e) {
    std::cerr << "specific: " << e.what() << "\n";
}

try {                                          // umbrella
    h5::gopen(fd, "/does/not/exist");
} catch (const h5::error::any& e) {            // base class
    std::cerr << "umbrella: " << e.what() << "\n";
}

h5::unmute();
```

`h5::mute()` / `h5::unmute()` suppress HDF5's CAPI diagnostic stack so intentional failure tests don't spray noise onto `stderr`.

## Surface Map

| Operation                | h5cpp call                   | Underlying HDF5         |
| ------------------------ | ---------------------------- | ----------------------- |
| Create group             | `h5::gcreate`                | `H5Gcreate2`            |
| Open group               | `h5::gopen`                  | `H5Gopen`               |
| List immediate children  | `h5::ls`                     | `H5Literate`            |
| Recursive listing (DFS)  | `h5::dfs`                    | `H5Lvisit`              |
| Recursive listing (BFS)  | `h5::bfs`                    | `H5Lvisit` + sort       |
| Attach attribute         | `h5::awrite`                 | `H5Awrite`              |
| Existence check          | `h5::exists`                 | `H5Lexists`             |
| Soft link                | `h5::link_soft`              | `H5Lcreate_soft`        |
| Hard link                | `h5::link_hard`              | `H5Lcreate_hard`        |
| External link            | `h5::link_external`          | `H5Lcreate_external`    |
| Rename / move object     | `h5::move`                   | `H5Lmove`               |
| Copy object              | `h5::copy`                   | `H5Ocopy`               |
| Delete link              | `h5::unlink`                 | `H5Ldelete`             |

Every link/move/copy operation accepts `h5::fd_t`, `h5::gr_t`, or a raw `hid_t` as the location parameter (via the `impl::is_valid_loc<L>` constraint). Default lcpl carries `create_intermediate_group{1}` so missing parents auto-create on link writes.

Wrappers not yet exposed (use raw C API for now):

- `H5Ldelete_by_idx`, `H5Lmove_by_idx` — index-based variants
- `H5Lget_info`, `H5Lget_val` — link metadata
- `H5Ocopy` with custom `ocpy_plist` — fine-grained copy options

## Build Notes

CMake target `examples-groups`. No dependencies beyond h5cpp itself.

```sh
cd <build-dir>
./examples-groups
h5dump -n groups.h5     # full tree, names only
h5dump groups.h5        # full tree with attribute values
```

## Mental Model

```text
file (fd_t)
  │
  └── group (gr_t) ───── attributes (awrite)
        │
        ├── group (gr_t)
        │     └── datasets, sub-groups, links, attributes
        │
        ├── dataset (ds_t)
        │
        └── link (soft/hard/external)
              │
              ▼
        another object (same or different file)
```

Groups are HDF5's namespace mechanism: they organise objects, carry annotations, and let you build cross-references via links. Everything else in HDF5 lives inside one.

## Source

- [`groups.cpp`](groups_8cpp-example.html) — rendered with syntax highlighting
