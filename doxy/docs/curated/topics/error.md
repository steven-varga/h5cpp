@page curated_topics_error ERROR

@brief Context-sensitive C++ exception hierarchy — every HDF5 CAPI
failure surfaces as a typed exception that names **which operation
failed on which object kind**, so callers can catch at the narrowest
useful level.

# Error model

HDF5's CAPI signals failure by negative return codes and pushes a
diagnostic onto an internal error stack. Useful information lives in
the stack; useful control flow comes from the return code. h5cpp
collapses both into one mechanism: every failed CAPI call throws a
**typed C++ exception** derived from `h5::error::any : std::runtime_error`,
named after **the operation that failed on the object kind it failed
on**.

The handler at your call site catches what it cares about; the rest
propagates.

## The "context-sensitive" pitch

A C++ application calling HDF5 wants to distinguish failures by
*what was being attempted*, not by a global error code. h5cpp's
hierarchy is shaped exactly so:

```
std::runtime_error
└─ h5::error::any                            ← root; catch this to swallow ALL h5cpp errors

   ├─ h5::error::io::file::any              ← any file-level failure
   │  ├─ h5::error::io::file::open          ← H5Fopen failed
   │  ├─ h5::error::io::file::create        ← H5Fcreate failed
   │  ├─ h5::error::io::file::close
   │  ├─ h5::error::io::file::read          ← rare; raised by file-level read primitives
   │  └─ h5::error::io::file::write

   ├─ h5::error::io::dataset::any
   │  ├─ h5::error::io::dataset::open
   │  ├─ h5::error::io::dataset::create
   │  ├─ h5::error::io::dataset::close
   │  ├─ h5::error::io::dataset::read       ← H5Dread / hyperslab / type-conversion failures
   │  ├─ h5::error::io::dataset::write
   │  ├─ h5::error::io::dataset::append     ← packet table flush failed
   │  └─ h5::error::io::dataset::misc

   ├─ h5::error::io::attribute::any
   │  ├─ h5::error::io::attribute::open
   │  ├─ h5::error::io::attribute::create
   │  ├─ h5::error::io::attribute::close
   │  ├─ h5::error::io::attribute::read
   │  ├─ h5::error::io::attribute::write
   │  ├─ h5::error::io::attribute::delete_
   │  └─ h5::error::io::attribute::misc

   ├─ h5::error::io::group::any
   │  ├─ h5::error::io::group::open
   │  ├─ h5::error::io::group::create
   │  └─ h5::error::io::group::close

   ├─ h5::error::io::packet_table::any
   │  └─ h5::error::io::packet_table::append

   └─ h5::error::property_list::any
      ├─ h5::error::property_list::misc      ← invalid FAPL/FCPL/DCPL/DAPL/LCPL/ACPL
      └─ h5::error::property_list::argument
```

Every leaf is its own type. Every intermediate is the "any" parent
that catches a whole category at once.

## Catching at the right level

```cpp
try {
    h5::fd_t fd = h5::open(path, H5F_ACC_RDONLY);
    auto v = h5::read<std::vector<float>>(fd, "/grid/data");
    process(v);
}
catch (h5::error::io::file::open&) {
    // File-specific recovery — bad path, missing file, magic byte mismatch
    use_default_config();
}
catch (h5::error::io::dataset::read&) {
    // Read-specific recovery — type conversion, hyperslab, rank mismatch
    log_and_skip();
}
catch (h5::error::any&) {
    // h5cpp catch-all — anything else h5cpp threw
    abort_gracefully();
}
catch (std::runtime_error&) {
    // Non-h5cpp library failures (and the h5cpp parent is one of these too)
}
```

The hierarchy is a real C++ inheritance chain — `catch` resolution
follows standard rules. **Catch the narrowest type you actually need
to distinguish; let the rest propagate**.

### Context-sensitive recipes

| Situation                                                  | Catch type                                  | Reason                                                       |
| :--------------------------------------------------------- | :------------------------------------------ | :----------------------------------------------------------- |
| "File may not exist yet" — create on first run             | `h5::error::io::file::open`                 | Just file-open; let other failures propagate                 |
| "Attribute may not be present" — use default                | `h5::error::io::attribute::open`            | Most callers want this exact type when reading optional metadata |
| "Read into too-small buffer" / "incompatible on-disk type" | `h5::error::io::dataset::read`              | Distinguish from open-time failures (file/dataset)           |
| Streaming append failure mid-flush                         | `h5::error::io::dataset::append`            | Different recovery than a one-shot write                     |
| "Either dataset or attribute" partial-state recovery       | `h5::error::io::dataset::any`, `h5::error::io::attribute::any` | Catch the kind, sub-distinguish at debug time     |
| Library-level "anything went wrong"                         | `h5::error::any`                            | Last-resort handler before letting it propagate to main      |
| Need the original HDF5 diagnostic text                     | Any leaf, then call `.what()`                | h5cpp puts the HDF5 stack trace + the wrapping `H5CPP_CHECK_NZ` location into the message |

### Anti-pattern — catching too wide

```cpp
// ✘ Loses all context. Don't do this:
try {
    h5::fd_t fd = h5::open(...);
    auto v = h5::read<...>(fd, "...");
} catch (std::exception&) {
    log("something went wrong");
    return -1;
}
```

You can't tell whether the file was missing, the dataset was the
wrong type, or you ran out of memory. **Catch by exception type, not
by hand-coded error-code switching**.

## How the typed exceptions get raised — the `CHECK_*` macros

Every HDF5 CAPI call in h5cpp goes through a small family of macros
that wrap the return code, inspect the HDF5 error stack, and throw
the contextual type:

```cpp
// h5cpp/H5capi.hpp — paraphrased
#define H5CPP_CHECK_NZ(expr, exc_type, msg)              \
    do { if ((expr) < 0) {                               \
        throw exc_type(H5CPP_ERROR_MSG(msg));            \
    }} while(0)

#define H5CPP_CHECK_PROP(prop, exc_type, msg)            \
    do { if (!H5Iis_valid(prop)) {                       \
        throw exc_type(H5CPP_ERROR_MSG(msg));            \
    }} while(0)

#define H5CPP_ERROR_MSG(msg) \
    (std::string(__FILE__ ":") + std::to_string(__LINE__) \
        + ": " + msg + "\n" + capture_hdf5_error_stack())
```

`capture_hdf5_error_stack()` reads HDF5's internal error stack
(via `H5Eget_current_stack` / `H5Ewalk2`) and folds it into the
exception's `.what()` string before letting the exception fly. So
you get:

- **The C++ source location** (`H5Acreate.hpp:97`) where the call
  failed
- **The h5cpp-level message** ("couldn't create attribute")
- **The full HDF5 error stack** (file/line/major/minor diagnostics
  from inside libhdf5 itself)

— all in a single `.what()` you can log.

### Wrapping your own HDF5 CAPI calls

If you drop down to raw HDF5 inside h5cpp-based code, use the same
macros so failures land in the same typed hierarchy:

```cpp
#include <h5cpp/H5capi.hpp>

::hid_t my_custom_thing = H5I_UNINIT;
H5CPP_CHECK_NZ(
    (my_custom_thing = H5Aopen_by_idx(static_cast<::hid_t>(parent),
                                       ".", H5_INDEX_NAME, H5_ITER_NATIVE,
                                       0, H5P_DEFAULT, H5P_DEFAULT)),
    h5::error::io::attribute::open,        // ← the exception type to throw
    "couldn't open first attribute by index");
```

This integrates cleanly: the application catches `h5::error::io::attribute::open`
without needing to know whether it came from h5cpp's `h5::open` or
your custom `H5Aopen_by_idx` wrapper.

## Rollback semantics — a separate branch

A small set of exceptions signal **"a partial-state operation was
successfully rolled back"**. These are NOT derived from
`std::runtime_error` (so they don't get caught by `catch(std::exception&)`
in non-h5cpp-aware code) — instead they're a separate
`h5::error::*::rollback` family:

```
h5::error::io::file::rollback
h5::error::io::dataset::rollback
h5::error::io::attribute::rollback
```

These are raised by the few operations that perform multi-step
modifications and can clean up after themselves on failure (file
creation across a flush boundary, dataset append after a partial
chunk flush, attribute create when followed by a write that fails).
The callee guarantees that on-disk state is consistent with
pre-call state when a `rollback` is thrown.

```cpp
try {
    h5::awrite(ds, "version", 2);   // create + write — atomic
} catch (h5::error::io::attribute::rollback&) {
    // On-disk state == pre-call state. Safe to retry, or fall through.
}
```

For details on which operations are rollback-safe, see
@ref link_error_handler "Error Handling — full reference".

## Suppressing the HDF5 error stack — `mute` / `unmute`

By default the HDF5 CAPI **also** prints the error stack to stderr
on every failure (separately from the typed exception h5cpp throws).
This is helpful during development, noisy in production, and
catastrophic in tight loops where failure is information:

```cpp
// "Does this attribute exist?" — the call-fail pattern
auto guard = h5::mute{};                          // RAII: re-enables on scope exit
try {
    h5::open(ds, "optional_attr");                // throws if not present — quiet
} catch (h5::error::io::attribute::open&) {
    // attribute not present; treated as "not set"
}
// `guard` destructs here, HDF5 stderr printing resumes
```

`h5::mute` and `h5::unmute` work as RAII guards (recommended) or as
manual switches. They suppress the **HDF5 native stack-trace
printing**, not h5cpp's typed exceptions — those still fly.

## Where to go next

- @ref link_error_handler — full exception-type reference with the
  CHECK_* macro inventory, every leaf type's trigger condition, and
  the rollback contract per operation
- @ref curated_io_api_file — `h5::error::io::file::*` leaves are the
  throws table for every file op
- @ref curated_io_api_dataset — `h5::error::io::dataset::*` leaves
- @ref curated_io_api_attributes — `h5::error::io::attribute::*` leaves
