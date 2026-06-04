# h5cpp SWMR — Single-Writer/Multiple-Reader

**Issue:** h5cpp#267
**Scope:** Linux-only, HDF5 ≥ 1.12.3, local POSIX filesystems.

SWMR lets one writer process append to a dataset while multiple reader
processes concurrently read it, with no locks and no reader/writer
coordination beyond explicit flush/refresh. This document is the operational
guide and the failure-mode catalog (report §8); every mode below was
re-verified against this implementation.

---

## 1. Quick start

```cpp
#include <h5cpp/all>

// ---------- writer process ----------
auto fd = h5::create("stream.h5", h5::swmr_write, h5::latest_version);
auto ds = h5::create<float>(fd, "stream",
    h5::current_dims{0}, h5::max_dims{H5S_UNLIMITED}, h5::chunk{4096});
h5::start_swmr_write(fd);          // activate SWMR — readers may attach AFTER this

// Build the packet table AFTER start_swmr_write: HDF5's transition must
// refresh-close every open object and cannot do so if the pt's re-opened
// dataset handle is already open. From here the writer keeps no ds_t.
h5::pt_t pt(ds);
for (;;) {
    h5::append(pt, sample);
    h5::flush(pt);                 // the complete writer-side SWMR call:
                                   // pushes any partial chunk AND issues the
                                   // SWMR metadata flush. No h5::flush(ds) needed.
}

// ---------- reader process ----------
auto fd = h5::open("stream.h5", h5::swmr_read);
auto ds = h5::open(fd, "stream");
for (;;) {
    h5::refresh(ds);               // pull the writer's latest flushed state
    // ... read the newly-visible tail ...
}
```

## 2. API surface

| Call | Side | Backed by |
|---|---|---|
| `h5::create(path, h5::swmr_write, h5::latest_version)` | writer | `H5Fcreate` (latest bounds) |
| `h5::open(path, h5::swmr_write)` | writer | `H5Fopen(... \| H5F_ACC_SWMR_WRITE)` |
| `h5::open(path, h5::swmr_read)` | reader | `H5Fopen(... \| H5F_ACC_SWMR_READ)` |
| `h5::start_swmr_write(fd)` | writer | `H5Fstart_swmr_write` |
| `h5::flush(pt)` | writer (packet table) | `H5Dflush` on the pt's own dataset — see below |
| `h5::flush(ds)` | writer (raw `h5::write`) | `H5Dflush` |
| `h5::refresh(ds)` | reader | `H5Drefresh` |

### Flush the handle you write through

- **Packet-table writers** call **`h5::flush(pt)`** — it pushes any buffered
  partial chunk *and*, when the file is SWMR-write, issues the `H5Dflush` on the
  packet table's own dataset. The pt owns the dataset, so the user has no
  separate `h5::ds_t` to flush; `flush(pt)` is the single, complete call. (It
  detects SWMR-write from the file's access intent and is a cheap no-op flush
  for non-SWMR packet tables.)
- **Raw writers** (`h5::write` + manual `h5::set_extent`) hold their own `ds`
  and call **`h5::flush(ds)`**.

### Two distinct writer entry paths (do not confuse them)

- **`h5::create(path, h5::swmr_write, h5::latest_version)`** — creates a *new*
  file with the latest on-disk format, returns a normal RDWR handle. HDF5
  cannot enable SWMR at create time because no datasets exist yet; you create
  your datasets, then call `h5::start_swmr_write(fd)` to activate SWMR. This is
  deliberate (see failure mode #3).
- **`h5::open(path, h5::swmr_write)`** — opens an *already SWMR-prepared* file
  directly in SWMR-write mode (no transition needed).
- **`h5::start_swmr_write(fd)`** — transitions an already-open RDWR file into
  SWMR mode mid-stream (the long-running-writer workflow).

### Type-level safety

`h5::create(..., h5::swmr_write, ...)` **requires** `h5::latest_version` in the
property chain. Omitting it is a *compile error*, not a runtime surprise — a
file created with default library-version bounds can never host SWMR.

## 3. Gating & platform

- **Feature gate:** keys on the HDF5 feature macros
  `H5F_ACC_SWMR_WRITE` / `H5F_ACC_SWMR_READ` → defines `H5CPP_HAS_SWMR`
  (mirrors the ROS3-v2 FAPL gating pattern; not `H5_VERSION_GE`).
- **Linux-only:** every SWMR header `#error`s on non-Linux. CMake sets the
  feature on `CMAKE_SYSTEM_NAME STREQUAL "Linux"` and `FATAL_ERROR`s when
  `HDF5_VERSION VERSION_LESS 1.12.3`.
- **Filesystem detection:** `statfs(2)` at open classifies the backing FS and
  warns (does not refuse) on network/layered/parallel filesystems. Opt out
  with `H5CPP_SWMR_NO_FS_CHECK=1`.

## 4. Filesystem support

| FS | Verdict | Behaviour |
|---|---|---|
| ext4, xfs, btrfs, tmpfs, f2fs | ✔ ok | silent |
| nfs | ✘ failed | loud warning, proceeds |
| smb, cifs | ✘ failed | loud warning, proceeds |
| overlayfs | ◇ cancelled | warning, proceeds |
| lustre, gpfs, beegfs | ◇ cancelled | warning ("requires correct mount options"), proceeds |
| unknown / fuse | ○ na | silent (informational), proceeds |

statfs magic numbers absent from `<linux/magic.h>` on most distros (CIFS,
Lustre, GPFS, BeeGFS) are vendored in `h5cpp/H5Fswmr_fs.hpp`.

---

## 5. Failure-mode catalog (report §8) — verified verdicts

| # | Failure mode | Mitigation in this impl | Verdict |
|---|---|---|---|
| 1 | Missing `(LATEST,LATEST)` library bounds at create | `h5::create(swmr_write,...)` `static_assert`s on `h5::latest_version`; forces latest bounds on the FAPL | ✔ ok — compile-time prevented |
| 2 | Contiguous/compact storage layout | SWMR-appendable data is necessarily extensible: `h5::create<T>` with `max_dims{H5S_UNLIMITED}` auto-applies a chunked layout (`H5Pset_chunk`), so the idiomatic SWMR dataset is always chunked. Note HDF5 1.12.3 does *not* reject `start_swmr_write` on a contiguous fixed-size dataset — but such a dataset cannot be appended to, so it is not a real SWMR target | ✔ ok — idiomatic auto-chunking (HDF5 does not enforce) |
| 3 | Mode confusion: create-with-SWMR vs `H5Fstart_swmr_write` | Distinct entry points; `create(swmr_write)` routes through `start_swmr_write` *after* datasets exist instead of passing `H5F_ACC_SWMR_WRITE` to `H5Fcreate` (which silently produced torn reader metadata — "bad object header version number") | ✔ ok — root cause fixed during impl |
| 4 | No flush between writes → stale readers | No hidden flushes; visibility is explicit via `h5::flush(ds)` (and `h5::flush(pt)` to push the partial chunk) | ✔ ok — explicit by design |
| 5 | Dense-attribute SWMR bug (pre-1.14) | 1.12.3 floor mitigates; doc warning for many-attribute objects on the SWMR path | ◇ cancelled — mitigated, not eliminated (keep attribute count low) |
| 6 | NFS / network FS | `statfs` detection emits a loud warning at open | ✔ ok — surfaced, not hidden |
| 7 | Reader opens before writer's first flush | Documented writer-first ordering; reader retry is application policy. Harness proves it via a POSIX-semaphore gate + bounded open retry | ✔ ok — documented + test-proven |
| 8 | HDF5 built without thread-safety, multi-threaded writer | h5cpp serializes at the file-handle level; if you bypass h5cpp and call HDF5 directly from multiple threads, you own the lock. This build's HDF5 has `Threadsafety: OFF` — keep the writer single-threaded | ○ na — out of scope; documented |
| 9 | VFD mismatch (split/multi/family) | SWMR works only with the default sec2 VFD on local FS; documented. Type-level VFD enforcement considered, deferred (rare misuse) | ◇ cancelled — documented, not enforced |

### Re-verification notes

- **Mode #3 was the real prior-attempt root cause.** Passing
  `H5F_ACC_SWMR_WRITE` to `H5Fcreate` is *accepted* by HDF5 1.12.3 (returns a
  valid handle) but produces a file whose datasets, created afterward, are not
  SWMR-safe: readers hit "bad object header version number" on refresh. The fix
  is to create normally with latest bounds and transition via
  `H5Fstart_swmr_write` once datasets exist. The cross-process harness fails
  without this fix and passes with it.
- **fork() + HDF5 handles (report §12).** Sharing an open HDF5 handle across
  `fork()` corrupts library state in the child and silently breaks SWMR
  visibility (reader sees 0 growth). The test harness opens **all** HDF5 handles
  *after* fork — never inherits one. This is a hard requirement for any
  fork-based SWMR program.

## 6. Test coverage (report §10)

`test/H5Fswmr.cpp` (ctest label `swmr`): 1 writer + 1 reader, 1 writer + 3
readers, writer-crash mid-stream, reader-before-first-flush, `start_swmr_write`
transition, tmpfs/local-FS round-trip, and filesystem-class classification incl.
the env opt-out. Run in isolation:

```
ctest --test-dir build -L swmr --output-on-failure
```
