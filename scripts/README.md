# `scripts/`

Developer tooling for h5cpp. Each script is self-contained and documented in
its own header; this file is the quick reference.

---

## `cdash` — off-CI CDash test + coverage submission

Builds h5cpp, runs the test suite, collects coverage, and uploads the results
to [my.cdash.org](https://my.cdash.org/index.php?project=h5cpp) from any
machine — desktop, laptop, or an HPC cluster under SLURM. It is a thin wrapper
over `ctest -S CTestDashboard.cmake` (the CTest dashboard script that does the
actual configure → build → test → coverage → submit pipeline).

### Quick start

```bash
git submodule update --init --recursive      # vendored thirdparty libs must be present
CXX=g++-14 GCOV=gcov-14 scripts/cdash         # coverage + examples, then submit
```

The result appears on CDash under the **Experimental** group, tagged with the
host's site name and an auto build name like `Linux-x86_64-g++-14-Debug`.

### Defaults

| Option | Default | Notes |
|--------|---------|-------|
| `COVERAGE` | `ON` | forces a `Debug -O0 -g --coverage` build + gcov collection |
| `BUILD_EXAMPLES` | `ON` | also builds the `examples/` tree |
| `SUBMIT` | `ON` | upload to CDash; set `OFF` for a local dry run |
| `TRACK` | `Experimental` | CDash group — `Nightly` for scheduled cron runs |
| `BUILD_TYPE` | `Debug` (when `COVERAGE=ON`) | otherwise `Release` |
| `JOBS` | SLURM allocation, else logical cores | see SLURM section |

> **Track vs build type.** `Experimental`/`Nightly`/`Continuous` is the CDash
> *group* (when/why a build ran). It is **not** an alternative to
> `Release`/`Debug`, which is the *build type* (how it was compiled). Coverage
> requires `Debug` instrumentation, so coverage runs are always `Debug`.

### Common invocations

```bash
scripts/cdash                                   # coverage + examples, submit (defaults)
scripts/cdash SUBMIT=OFF                         # dry run: build + test + coverage, no upload
scripts/cdash COVERAGE=OFF BUILD_TYPE=Release    # plain Release build, no coverage
scripts/cdash HDF5_DIR=/opt/hdf5/cmake           # pin HDF5 (module installs / discovery trap)
CXX=g++-14   GCOV=gcov-14        scripts/cdash    # match gcov to the compiler
CXX=clang++-18 GCOV='llvm-cov gcov' scripts/cdash
```

All `KEY=value` arguments are forwarded to `CTestDashboard.cmake` as `-DKEY=value`.
Full option list: `HDF5_ROOT`, `HDF5_DIR`, `CTEST_SITE`, `CTEST_BUILD_NAME`,
`JOBS`, `TRACK`, `SUBMIT`, `COVERAGE`, `BUILD_EXAMPLES`, `BUILD_TYPE`.

### Prerequisites

- `cmake` ≥ 3.17, `ctest`, and `ninja` (preferred) or `make`
- A compiler with `--coverage` support **and a matching `gcov`** — e.g. `g++-14`
  needs `gcov-14`, not the default `gcov`. Set both via `CXX=` and `GCOV=`.
  (`lcov` is **not** needed — CTest drives `gcov` directly.)
- HDF5 ≥ 1.12.3 — auto-detected, or pinned with `HDF5_DIR=.../cmake`
- thirdparty submodules initialised (the build won't configure without them)
- outbound HTTPS to `my.cdash.org` (open submission, no credentials)

### Running under SLURM

No special flags are required — the script adapts to the job automatically:

- **`JOBS`** is derived from `SLURM_CPUS_PER_TASK` → `SLURM_CPUS_ON_NODE` →
  logical core count, so the build never oversubscribes a shared node.
- **Site** defaults to `SLURM_CLUSTER_NAME` (so every node groups under one
  cluster on the dashboard); override with `CTEST_SITE`.
- **Build name** is suffixed with the compute-node hostname so concurrent
  per-node submissions stay distinct.

```bash
#!/bin/bash
#SBATCH -N1 -c16 -t00:30:00
module load hdf5 gcc                        # site-specific
srun scripts/cdash HDF5_DIR="$HDF5_DIR/cmake"
```

> **Compute-node networking.** Many HPC sites block direct outbound HTTPS from
> compute nodes, so `SUBMIT=ON` may fail at the upload step. Options: run on a
> login/interactive node, export `HTTPS_PROXY`, or use `SUBMIT=OFF` on the
> compute node and re-run the submit from a login node.

### Troubleshooting

| Symptom | Cause / fix |
|---------|-------------|
| `gcov` version-mismatch / zeroed counts | `gcov` doesn't match the compiler → set `GCOV=gcov-NN` for `g++-NN` |
| HDF5 not found / wrong version | pass `HDF5_DIR=.../cmake` (an `h5cc` on `PATH` can shadow the intended install) |
| upload fails on a cluster | compute node has no outbound HTTPS — see the SLURM note above |
| stale build | safe to `rm -rf build-cdash` between runs |

---

## `amalgamate.py` — single-header generator

Concatenates the modular h5cpp headers into one self-contained `h5cpp.hpp`,
stripping `#pragma once` and internal `#include "H5*.hpp"` / `compat.hpp`
directives so the result compiles as a single translation unit. Useful for
drop-in distribution or quick experimentation without the full include tree.

### Usage

```bash
python3 scripts/amalgamate.py <input-dir> <output-file>

# example: amalgamate the in-tree headers into a build artifact
python3 scripts/amalgamate.py h5cpp build/h5cpp.hpp
```

- `<input-dir>` — the `h5cpp/` header directory (must contain the `core` and
  `io` umbrella files; their pre-`#ifndef` preambles are preserved).
- `<output-file>` — destination path for the generated single header.

Requires Python 3; no third-party packages.
