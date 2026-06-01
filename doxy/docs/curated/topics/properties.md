@page curated_topics_properties PROPERTIES

@brief HDF5 property lists — typed RAII wrappers around the CAPI's
`hid_t` property machinery, with a `|` operator for composing
filter / chunk / tuning options into a single argument.

# Property lists

HDF5 has six creation / access "property list" types, each tuning a
different phase of an object's lifecycle. H5CPP wraps every one as a
typed RAII handle in the `h5::*_t` family, then provides a `|`
operator that lets you compose tunables fluently:

```cpp
h5::ds_t ds = h5::create<float>(fd, "stream",
    h5::current_dims{0}, h5::max_dims{H5S_UNLIMITED},
    h5::chunk{1024} | h5::gzip{9} | h5::fletcher32 | h5::shuffle);
```

That last argument is a `h5::dcpl_t` composed at compile time from
four tunables. No manual `H5Pset_chunk`, no
`H5Pset_filter(..., H5Z_FILTER_DEFLATE, …)`, no error-code dance.

## The six lists at a glance

| Property list | Tag       | Tunes                                           | Default value         |
| :------------ | :-------- | :---------------------------------------------- | :-------------------- |
| File creation | `fcpl_t`  | On-disk metadata layout, B-tree split ratios    | `h5::default_fcpl`    |
| File access   | `fapl_t`  | File driver (POSIX/MPI-IO/ROS3/family), cache, locking | `h5::default_fapl` |
| Dataset creation | `dcpl_t` | Chunking, fill value, filter pipeline (gzip/shuffle/fletcher32/…) | `h5::default_dcpl` |
| Dataset access | `dapl_t` | Per-chunk cache, virtual dataset behaviour, high-throughput pipeline | `h5::default_dapl` |
| Link creation | `lcpl_t`  | Character encoding (UTF-8), intermediate group auto-create | `h5::default_lcpl` |
| Attribute creation | `acpl_t` | Character encoding for attribute names         | `h5::default_acpl`    |

The full per-property surface (every `h5::*` tunable, what it maps
onto in the HDF5 CAPI) lives in:

→ @ref link_property_lists "Property Lists — full reference"

## The `|` composition operator

Each tunable is a tiny value type (`h5::chunk{...}`, `h5::gzip{9}`,
etc.). The `operator|(dcpl_t, dcpl_t)` overloads in
`h5cpp/H5Pdcpl.hpp` fold these into a single `dcpl_t` at compile time:

```cpp
auto dcpl = h5::chunk{4096}                 // chunk shape
          | h5::gzip{6}                     // deflate level
          | h5::fletcher32                  // checksum
          | h5::shuffle                     // byte-shuffle filter
          | h5::fill_value<float>{0.0f};    // fill value for unwritten chunks
```

This composes into a single argument the variadic create/write/etc.
calls pick up via `arg::get<dcpl_t>(args...)`. Order doesn't matter —
the SFINAE matches by type, not position.

## Default property lists

Every public API call accepts an optional property list with a sensible
default. The `h5::default_*` constants are `_t`-wrapped `H5P_DEFAULT`
sentinels:

```cpp
// All three of these are equivalent:
h5::fd_t fd = h5::open("data.h5", H5F_ACC_RDONLY);
h5::fd_t fd = h5::open("data.h5", H5F_ACC_RDONLY, h5::default_fapl);
h5::fd_t fd = h5::open("data.h5", H5F_ACC_RDONLY, h5::fapl{});
```

`h5::lcpl_t` is the one default worth noting: `h5::default_lcpl`
sets UTF-8 encoding AND enables intermediate-group auto-creation —
so `h5::gcreate(fd, "a/b/c")` works without pre-creating `a` and
`a/b` first.

## High-throughput dataset access pipeline

H5CPP's `dapl_t` carries an optional **pipeline tag**
(`H5CPP_DAPL_HIGH_THROUGHPUT`) that bypasses HDF5's chunk-cache
single-thread filter execution and runs filters across a
configurable worker pool. Activated implicitly when the DAPL
carries the tag:

```cpp
// Read a heavily-filtered dataset with parallel filter execution
auto dapl = h5::dapl{}
          | h5::high_throughput{h5::threads{N}};
h5::ds_t ds = h5::open(fd, "/giant/compressed", dapl);
auto v = h5::read<std::vector<float>>(ds);   // pool-parallel decompression
```

The pipeline initialises lazily inside `h5::open` from the dataset's
element size — see the H5Dopen detail page.

## Property lists as parents (file-io)

`h5::fapl_t` carries the file driver. Different drivers serve
different storage backends:

| Driver                | Use case                                       |
| :-------------------- | :--------------------------------------------- |
| POSIX (default)       | Local filesystem (single-process or NFS)       |
| MPI-IO                | Parallel I/O across ranks                      |
| ROS3                  | Read-only access to S3-hosted HDF5 files       |
| Family / Split        | Files larger than 2 GB on legacy filesystems   |
| Core (in-memory)      | Memory-resident HDF5 containers                |

The MPI driver setup is covered in the
@ref curated_topics_mpi "MPI" topic. ROS3 patterns are in
@ref example_guide_s3 "Cookbook: s3".

## Where to go next

- @ref link_property_lists — every tunable, every default, every
  call site
- @ref curated_topics_filters — gzip / fletcher32 / shuffle /
  Gorilla / custom — the most common DCPL composition target
- @ref curated_topics_mpi — FAPL setup for parallel I/O
