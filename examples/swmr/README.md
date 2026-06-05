@page example_guide_swmr SWMR — Single-Writer / Multiple-Reader

SWMR lets one writer process append to a dataset while several reader processes
see the growth **live**, on the same host, with no locks and no coordination
beyond explicit `h5::flush` (writer) and `h5::refresh` (reader). This cookbook is
three small standalone programs — one writer and two interchangeable readers.

**Linux-only** this release: each source opens with an `#ifndef __linux__` guard
and the CMake targets are registered only on `CMAKE_SYSTEM_NAME STREQUAL "Linux"`.

## Files

| File | Purpose |
|------|---------|
| `swmr-write.cpp` | writer — create/append/flush, cold+warm start, clean Ctrl-C |
| `swmr-read.cpp`  | reader — follows via `h5::read<std::vector<T>>` (materializes the extent) |
| `swmr-view.cpp`  | reader — follows via `h5::view<T>` (C++20 ranges, constant-memory streaming) |

## Run

SWMR is a cross-**process** feature, so use two terminals — writer first, then a
reader:

```
./examples-swmr-write     # terminal 1
./examples-swmr-read      # terminal 2 — watches the count climb live
# or, interchangeably:
./examples-swmr-view      # terminal 2 — same, via the h5::view ranges API
```

## What you should know about SWMR

1. **Writer-first, cross-process.** One writer + N readers on one host. The writer
   must exist before readers attach; the only coordination is `h5::flush` on the
   writer and `h5::refresh` on the reader. No polling helper is provided — cadence
   is your policy.
2. **You can't enable SWMR at create time.** `h5::create` makes a *normal*
   latest-format file; `h5::start_swmr_write(fd)` flips it into SWMR mode **after**
   the datasets exist. The call is idempotent — a no-op if the file is already
   SWMR-write (e.g. a warm restart).
3. **Restart resumes, never truncates.** Cold start (no file) creates; warm start
   (file exists) reopens and continues from `h5::get_extent(ds)[0]`. Re-running the
   writer picks up the stream where it left off.
4. **Close cleanly or the file is left incomplete.** A writer killed without
   `H5Fclose` leaves the file in a dirty "write-in-progress" state. The writer
   installs a signal handler that just sets a flag; the loop exits and **RAII**
   closes the file — never call HDF5 from a signal handler.
5. **File locking blocks a warm reattach while readers are live.** That's HDF5's
   file-*lock* (not the SWMR protocol). The writer turns it off on its own FAPL:
   `H5Pset_file_locking(fapl, /*use=*/false, /*ignore_disabled=*/true)`. Trade-off:
   single-writer discipline is then yours to enforce.
6. **Reader type must match the writer's element type.** This stream is
   `std::uint64_t`, so readers use `h5::view<std::uint64_t>` /
   `h5::read<std::vector<std::uint64_t>>` — a type mismatch reads garbage.
7. **Filesystem matters.** SWMR needs local POSIX page-cache coherence; NFS and
   other network/layered filesystems break reader visibility. h5cpp warns at open
   (opt out with `H5CPP_SWMR_NO_FS_CHECK=1`).
8. **CAPI ↔ h5cpp.** The demos pass raw `H5F_ACC_*` flags to `h5::open`/`h5::create`
   and reach for raw `H5Pset_file_locking` through an `h5::fapl_t` — the C-API is
   always one `static_cast<hid_t>` away to fill any gap.

## Writer

```cpp
// Warm restarts reattach while readers are live: disable the file lock on the FAPL.
h5::fapl_t nolock{H5Pcreate(H5P_FILE_ACCESS)};
H5Pset_file_locking(static_cast<hid_t>(nolock), false, true);

h5::fd_t fd = std::filesystem::exists(container)
    ? h5::open(container,   H5F_ACC_RDWR | H5F_ACC_SWMR_WRITE, nolock)   // warm: resume
    : h5::create(container, H5F_ACC_RDWR | H5F_ACC_SWMR_WRITE);          // cold: create
h5::ds_t ds = h5::exists(fd, dataset)
    ? h5::open(fd, dataset)
    : h5::create<std::uint64_t>(fd, dataset,
          h5::current_dims{0}, h5::max_dims{H5S_UNLIMITED}, h5::chunk{batch});

h5::start_swmr_write(fd);                       // idempotent: transitions cold, no-op warm
std::uint64_t count = h5::get_extent(ds)[0];    // 0 on cold, the resume point on warm

h5::pt_t pt(ds);
for (int b = 0; b < batches && !stop; ++b) {    // !stop → clean Ctrl-C shutdown
    for (int i = 0; i < batch; ++i) h5::append(pt, count++);
    h5::flush(pt);                              // appended data is visible to readers now
}
```

Note: pass the raw `H5F_ACC_SWMR_WRITE` flag and h5cpp does the right thing — it
creates a normal latest-format file (it can't enable SWMR at create), so you then
call `h5::start_swmr_write`. The packet table's `flush(pt)` issues the SWMR metadata
flush itself, so the writer needs no separate `h5::ds_t` to flush.

## Readers — two interchangeable styles

Both open `H5F_ACC_RDONLY | H5F_ACC_SWMR_READ` and loop on `h5::refresh`; they
differ only in how they pull the newly-visible tail.

### `swmr-read` — materialize the extent

```cpp
h5::fd_t fd = h5::open(container, H5F_ACC_RDONLY | H5F_ACC_SWMR_READ);
h5::ds_t ds = h5::open(fd, "stream");
std::size_t seen = 0;
for (;;) {
    h5::refresh(ds);                                      // pull the latest flushed state
    auto data = h5::read<std::vector<std::uint64_t>>(ds); // current contents
    if (data.size() > seen) { /* use data[seen .. end) */ seen = data.size(); }
}
```

Simple, fine for modest streams — but it reads the whole dataset every poll.

### `swmr-view` — stream in constant memory

`h5::view<std::uint64_t>(ds)` is a **C++20 ranges** input-view that streams a
rank-1 chunked dataset one chunk at a time and never materializes a full vector —
the right tool for a large stream. It **snapshots the extent when constructed**, so
the discipline is: `refresh` first, then build a *fresh* view each poll;
`std::views::drop(seen)` streams only the new tail.

```cpp
h5::refresh(ds);                                       // pull latest flushed state FIRST
auto snap = h5::view<std::uint64_t>(ds);               // snapshot of the current extent
for (std::uint64_t v : snap | std::views::drop(seen))  // stream only the newly-visible tail
    { /* use v */ }
```

## See also

- @ref example_guide_packet_table — the `h5::append` packet-table surface this builds on
