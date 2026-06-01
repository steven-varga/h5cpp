@page curated_io_api_attributes ATTRIBUTES

@brief Templated attribute I/O on any HDF5 parent that can carry
metadata — file, group, dataset, opaque object, committed datatype.

# Attribute operations

Attributes are small named metadata items attached to a parent HDF5
object. They live in the parent's object header (not as standalone
datasets), do not chunk, and do not support partial I/O — each
attribute is read and written in a single shot.

Use attributes for things that **describe** the data (units, labels,
provenance, schema versions, small calibration constants) — not for
the data itself. If the value is large, changes over time, or needs
slicing, use a dataset instead.

## At a glance

| Operation              | Function / syntax                                                | Returns        |
| :--------------------- | :--------------------------------------------------------------- | :------------- |
| Allocate               | `h5::create<T>(parent, name, args...)`                           | `h5::at_t`    |
| Open existing          | `h5::open(parent, name, acpl?)`                                  | `h5::at_t`    |
| Read into `T`          | `h5::aread<T>(parent, name, acpl?)`                              | `T`           |
| Write a value          | `h5::awrite(parent, name, value, acpl?)`                         | `h5::at_t`    |
| Write a brace-list     | `h5::awrite(parent, name, {a, b, c}, acpl?)`                     | `h5::at_t`    |
| Delete by name         | `h5::adelete(parent, name)`                                      | `void`        |
| Bracket-syntax write   | `parent["name"] = value`                                         | `h5::at_t`    |
| Bracket-syntax read    | `T v = parent["name"]`                                           | `T`           |

All free functions accept any parent satisfying
`h5::impl::is_valid_attr<P>`: raw `::hid_t`, `h5::gr_t`, `h5::ds_t`,
`h5::ob_t`, or `h5::dt_t<T>`. Typed `h5::fd_t` is **not** in the trait
— pass `static_cast<::hid_t>(fd)` to attach to the file root.

The bracket-syntax forms live on `h5::ds_t`, `h5::gr_t`, and
`h5::ob_t`.

---

@anchor curated_io_api_attributes_create
## `h5::create<T>` — allocate a new attribute

```cpp
template<class T, class HID_T, class... args_t>
std::enable_if_t<h5::impl::is_valid_attr<HID_T>::value, h5::at_t>
h5::create(const HID_T& parent, const std::string& path, args_t&&... args);
```

Allocates the on-disk slot for an attribute of element type `T`. The
value itself is later deposited with `h5::awrite` or read with
`h5::aread<T>`. Most call sites skip `h5::create` and let `h5::awrite`
create-on-demand; explicit `h5::create` is useful when the shape is
non-default or the slot must exist before any value is written.

**Parameters**

| Name              | Type                                                  | Description                                                                  |
| :---------------- | :---------------------------------------------------- | :--------------------------------------------------------------------------- |
| `parent`          | `::hid_t` / `gr_t` / `ds_t` / `ob_t` / `dt_t<T>`      | Open parent handle (compile-time enforced via `is_valid_attr`).              |
| `path`            | `const std::string&`                                  | Attribute name (UTF-8).                                                      |
| `args...`         | variadic                                              | `h5::current_dims` (shape; defaults to scalar) and/or `h5::acpl_t` (ACPL).   |

**Returns** — `h5::at_t` RAII handle.

**Throws** — `h5::error::io::attribute::create` on `H5Acreate2`
failure; `h5::error::property_list::misc` on invalid ACPL.

**Example**

```cpp
h5::fd_t fd = h5::open("file.h5", H5F_ACC_RDWR);
h5::ds_t ds = h5::open(fd, "/grid/data");

// rank-1 of 3 floats; deposit the value later
h5::create<float>(ds, "spacing", h5::current_dims{3});

// scalar on file root — fd_t needs an explicit cast
h5::create<double>(static_cast<::hid_t>(fd), "schema_version");
```

---

@anchor curated_io_api_attributes_open
## `h5::open` — open an existing attribute

```cpp
template<class HID_T>
std::enable_if_t<h5::impl::is_valid_attr<HID_T>::value, h5::at_t>
h5::open(const HID_T& parent, const std::string& path,
         const h5::acpl_t& acpl = h5::default_acpl);
```

Returns an RAII-managed `h5::at_t` suitable for passing to
`h5::aread<T>` or the low-level `h5::awrite(at_t, T*)` form.

**Throws** — `h5::error::io::attribute::open` (not present, parent
invalid, bad ACPL).

**Example**

```cpp
h5::ds_t ds = h5::open(fd, "/grid/data");
h5::at_t att = h5::open(ds, "spacing");
```

---

@anchor curated_io_api_attributes_aread
## `h5::aread<T>` — read attribute value as `T`

```cpp
template <class T, class HID_T>
std::enable_if_t<h5::impl::is_valid_attr<HID_T>::value, T>
h5::aread(const HID_T& parent, const std::string& name,
          const h5::acpl_t& acpl = h5::default_acpl);
```

`T` determines how the on-disk value is materialised — see
@ref link_base_template_types "Supported Types". HDF5 has no
fixed-length ↔ VLEN string conversion, so a fixed-length string
attribute reads into `std::string` or `char[N]` but not into
`std::vector<std::string>` unless the on-disk type is VLEN.

**Throws** — `h5::error::io::attribute::open` if the attribute is
not present; `h5::error::io::attribute::read` on `H5Aread` failure.

**Example**

```cpp
h5::ds_t ds = h5::open(fd, "/grid/data");

auto version = h5::aread<double>(ds, "schema_version");           // scalar
auto spacing = h5::aread<std::vector<float>>(ds, "spacing");      // rank-1
auto label   = h5::aread<std::string>(ds, "label");               // VLEN string

// Bracket-syntax sugar — implicit conversion picks T from the LHS
std::string label2 = ds["label"];
```

---

@anchor curated_io_api_attributes_awrite
## `h5::awrite` — write a value as an attribute

```cpp
// Create-on-demand — creates the attribute if it doesn't exist.
template <class T, class HID_T, class... args_t>
std::enable_if_t<h5::impl::is_valid_attr<HID_T>::value, h5::at_t>
h5::awrite(const HID_T& parent, const std::string& name,
           const T& ref, const h5::acpl_t& acpl = h5::default_acpl);

// Brace-list convenience.
template<class T, class HID_T>
std::enable_if_t<h5::impl::is_valid_attr<HID_T>::value, h5::at_t>
h5::awrite(const HID_T& parent, const std::string& name,
           std::initializer_list<T> ref,
           const h5::acpl_t& acpl = h5::default_acpl);

// Low-level — operates on a pre-opened h5::at_t. Rarely called directly.
template <class T>
void h5::awrite(const h5::at_t& attr, const T* ptr);
```

Shape is derived from `ref` via `access_traits_t<T>::size`. Attributes
do not chunk and do not support partial I/O.

**Throws** — `h5::error::io::attribute::write` on `H5Awrite` failure;
`h5::error::io::attribute::create` if create-on-demand failed.

**Example**

```cpp
h5::ds_t ds = h5::open(fd, "/grid/data");

h5::awrite(ds, "schema_version", 1.4);                              // double
h5::awrite(ds, "spacing", std::vector<float>{0.5f, 0.5f, 1.0f});    // 3 floats
h5::awrite(ds, "label", std::string{"u32"});                        // VLEN string
ds["axes"] = {1, 2, 3};                                             // bracket sugar
```

---

@anchor curated_io_api_attributes_adelete
## `h5::adelete` — remove an attribute

```cpp
template<class HID_T>
std::enable_if_t<h5::impl::is_valid_attr<HID_T>::value, void>
h5::adelete(const HID_T& parent, const std::string& name);
```

Removes the named attribute from `parent`. The operation is final —
there is no rollback once `H5Adelete` has succeeded.

**Throws** — `h5::error::io::attribute::delete_` on failure.

**Example**

```cpp
h5::ds_t ds = h5::open(fd, "/grid/data");
h5::adelete(ds, "stale_marker");
```

---

@anchor curated_io_api_attributes_bracket
## Bracket-syntax sugar — Python-dict-like API

`h5::ds_t`, `h5::gr_t`, and `h5::ob_t` expose
`operator[](const char*)` returning a transient `h5::at_t` carrying
the parent handle and attribute name. Combined with
`at_t::operator=(V)` (write) and the templated
`at_t::operator V() const` (read), this gives a compact dict-like API:

```cpp
// Write
gr["title"]   = std::string{"hello"};
gr["count"]   = 42;
gr["values"]  = std::vector<double>{1.0, 2.0, 3.0};

// Read — implicit conversion picks T from the LHS
std::string title  = gr["title"];
int         count  = gr["count"];
```

**Caveats**

- `auto v = gr["x"]` deduces `at_t`, **not** the attribute value. The
  implicit conversion needs a concrete LHS type to fire.
- The transient `at_t` carries only `ds` + `name`; if the parent
  handle is closed before the conversion runs, the read throws
  `h5::error::io::attribute::read`.

@see @ref handle_ref_indexer

---

## Cross-references

- @ref link_base_template_types "Supported Types" — element-type dispatch matrix
- @ref link_handle_reference "Handles, Descriptors & Property Lists" — `h5::at_t`, parent handles, ACPL
- @ref link_error_handler "Error Handling" — `h5::error::io::attribute::*` hierarchy
- @ref curated_io_api_file "FILE" — `fd_t` is one of the valid parents (with cast)
- @ref curated_io_api_dataset "DATASET" — `ds_t` is the most common attribute parent
- @ref curated_io_api_groups "GROUPS" — `gr_t` is the other common attribute parent
