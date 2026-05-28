@page example_guide_s3 S3 example — Read-Only S3 VFD (ROS3)

Opens HDF5 files hosted on AWS S3 (or any S3-compatible object store) read-only through HDF5's ROS3 Virtual File Driver. Same h5cpp call shape as a local file — only the URL and the FAPL change.

## What ROS3 is

| Layer | What it does |
| --- | --- |
| VFD swap | `H5Pset_fapl_ros3(fapl, &H5FD_ros3_fapl_t{...})` replaces the default `H5FD_SEC2` (POSIX) driver. Subsequent `H5Fopen` / `H5Dread` calls issue HTTP `Range:` GETs against S3 instead of `pread()` against a local file. |
| Auth | Three modes — unauthenticated (public bucket), long-term AWS credentials (v1 FAPL, HDF5 ≥ 1.10.6, SigV4-signed), and temporary STS / IAM-role credentials with a session token (v2 FAPL, HDF5 ≥ 1.14.x). |
| Caching | HDF5 maintains a page buffer; small reads are coalesced. Tuneable in HDF5 ≥ 1.14 via `H5Pset_fapl_ros3_token` family. |
| Scope | **Read-only.** No `H5Dwrite`, no append, no create. See *Write counterpart* below. |

## URL forms (cross-version trap)

| HDF5 version | `s3://bucket/key` | `https://bucket.s3.region.amazonaws.com/key` |
| --- | :---: | :---: |
| ≤ 1.12.x | ✘ rejected by `H5FD_s3comms_parse_url` | ✔ |
| ≥ 1.14.x | ✔ | ✔ |

This example uses the HTTPS form so it works on both. If you can hard-require HDF5 ≥ 1.14 use the shorter `s3://` form.

## Build

```sh
cd <build-dir>
cmake ..   # auto-detects ROS3 via HDF5_HAVE_ROS3_VFD in H5pubconf.h
```

CMake prints one of:

```
-- H5CPP: ROS3 VFD detected — S3 read-only support enabled
```

or:

```
-- H5CPP: ROS3 VFD not found — S3 support disabled
```

If the second message shows up your HDF5 was built without `--enable-ros3-vfd` (and/or without libcurl + OpenSSL). The example is skipped silently.

```sh
cmake --build . --target s3
```

## Run

```sh
./examples/s3/s3
```

No arguments. Block 1 (unauthenticated public bucket) always runs. Blocks 2 and 3 only run if their corresponding AWS env vars are set:

| Block | Env vars required | Runs against |
| --- | --- | --- |
| 1 — public | (none) | `https://rhdf5-public.s3.eu-central-1.amazonaws.com/h5ex_t_array.h5` |
| 2 — long-term creds | `AWS_REGION`, `AWS_ACCESS_KEY_ID`, `AWS_SECRET_ACCESS_KEY` | a private bucket of yours — edit the URL in `s3.cpp:43` |
| 3 — STS / session | also `AWS_SESSION_TOKEN`; HDF5 ≥ 1.14.x at compile time | a private bucket of yours — edit the URL in `s3.cpp:65` |

Expected output (no creds set, against the Bioconductor `rhdf5-public` test file):

```
Public bucket (no auth):
  /DS1
  /DS1: read 60 int64s (480 bytes) from S3
  element[0]:
    0    0    0    0    0
    0   -1   -2   -3   -4
    0   -2   -4   -6   -8
Set AWS_REGION, AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY to test authenticated access.
```

The element-0 grid matches what `h5dump` of the file shows — those 480 bytes really did traverse S3 → ROS3 → h5cpp.

## What the example demonstrates

1. **VFD selection via FAPL** — `h5::ros3{authenticate?, region, key, secret[, token]}` is the only S3-specific call. After `h5::open` returns, every subsequent h5cpp operation (`h5::ls`, `h5::open(fd, "/DS1")`, etc.) is identical to a local-file workflow.
2. **Real data transit, not just metadata** — block 1 reads `/DS1` and prints its first element, proving bytes (not just the superblock + header chain) crossed the wire.
3. **Auth-mode skip without creds** — blocks 2 and 3 silently do nothing if their env vars aren't set, so the example is safe to run on any machine.

## Read path used in the example (workaround note)

The on-disk type of `/DS1` is `H5T_ARRAY { [3][5] H5T_STD_I64LE }` — flat 2D array. The canonical h5cpp mapping `std::vector<std::array<std::array<i64,5>,3>>` produces *nested* `H5T_ARRAY{[3] H5T_ARRAY{[5] i64}}`, which HDF5's type-conversion engine refuses to bridge against the flat form even though the memory layout is identical. The example therefore drops to a raw `H5Dread` with the file's own datatype to prove S3 transit — the type-system gap is tracked separately as [vargalabs/h5cpp#279](https://github.com/vargalabs/h5cpp/issues/279) and out of scope for this example.

## Write counterpart — there isn't one

ROS3 is exactly what its name says — **R**ead-**O**nly **S3**. Upstream HDF5 ships no writable S3 VFD, in 1.12.x or 1.14.x. The VFD abstraction (POSIX-like `pwrite` at byte offsets, many small flushes) and S3's semantics (whole-object PUT, no append, no lock) mismatch badly enough that a naive writable VFD would either re-PUT the file on every flush (catastrophic) or buffer everything until close (no streaming, no crash safety).

The conventional patterns for S3 writes are:

| Pattern | What it is | Native C++? |
| --- | --- | --- |
| **Local write → `aws s3 cp`** | Write the `.h5` to local disk, sync to S3 on close. One shell line, full HDF5 feature set, crash-safe. | n/a — works with any HDF5 program |
| **HSDS + REST VOL** | HDFGroup's REST service shards HDF5 data into per-chunk S3 objects; HDF5's VOL layer translates ops to HTTP. Full R/W. | Yes via the REST VOL connector — a real setup chore |
| **HSDS + h5pyd** | Same backend, Python client | No — Python only |
| **Cloud-Optimised HDF5 / kerchunk** | Treat the `.h5` on S3 as immutable, mint a JSON sidecar of chunk byte-ranges, read via direct HTTP range-GETs without ROS3 | Conceptual, mostly Python ecosystem |

For h5cpp users, the right default is **write locally, upload as a separate step**. Reach for HSDS only when you genuinely need in-process writes to object storage and can absorb the operational complexity.

## Cross-references

- **`examples/mpi/`** — the other large-file / cluster I/O story; parallel HDF5 with MPI-IO. Complementary, not overlapping: ROS3 is read-from-cloud, parallel HDF5 is write-to-cluster-filesystem.
- **HDF5 manual: ROS3** — https://docs.hdfgroup.org/hdf5/develop/group___f_a_p_l.html#gaaf2a4d0b1c81dbc59f3df50b7d6d3e44 (search "ros3")
- **HSDS** — https://github.com/HDFGroup/hsds
- **REST VOL connector** — https://github.com/HDFGroup/vol-rest

## Build State (as of HEAD)

| Target | Status | Notes |
| --- | --- | --- |
| `s3` (block 1, public bucket) | ✔ ok | HDF5 1.12.3 at `/usr/local/HDF_Group/HDF5/1.12.3/` with `--enable-ros3-vfd` |
| `s3` (block 2, long-term creds) | ◇ na | Needs a private bucket + AWS creds — code path is wired |
| `s3` (block 3, STS / session) | ◇ na | Gated out on HDF5 ≤ 1.13 via `H5FD_CURR_ROS3_FAPL_T_VERSION >= 2` — needs HDF5 ≥ 1.14 to even compile in |

Gated on `H5CPP_HAVE_ROS3_VFD` (set by CMake when HDF5's `H5pubconf.h` defines `H5_HAVE_ROS3_VFD`). When ROS3 isn't present the example is skipped at CMake configure time with a clear status message.

## Source

- [`s3.cpp`](s3_8cpp-example.html) — rendered with syntax highlighting
