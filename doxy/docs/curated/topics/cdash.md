@page curated_topics_cdash CDASH

@brief Public dashboards of h5cpp build + test results across
platforms and toolchains, plus per-PR test-coverage tracking.
Open to community submissions — run the test suite on your own
architecture and your results appear on the same dashboards.

# CDash

H5CPP submits its CTest results — build status, test pass/fail
counts, timing breakdowns, and (where instrumented) line-coverage
data — to a public **CDash** dashboard. CDash is the Kitware-built
companion to CTest: a web aggregator that lines up results from
every submitting machine and renders them as a unified per-day grid.

→ **Live dashboard:** <https://my.cdash.org/index.php?project=h5cpp>

## What it shows

| Panel | Content |
| :---- | :------ |
| **Nightly / Continuous / Experimental** rows | Three submission tracks — `Nightly` (scheduled, full suite), `Continuous` (per-commit smoke), `Experimental` (ad-hoc runs from contributors) |
| Site column | One row per submitting machine — OS, compiler, HDF5 version, MPI flavour. Maintained by submitters, not centrally administered |
| Build warnings + errors | Compiler diagnostics aggregated from every submitting build, drill-down to source/line |
| Test pass / fail / timing | Per-test results across all submitting platforms in one grid; click any failing test for its captured output |
| Coverage (when instrumented) | Per-file line-coverage percentages from builds compiled with `-coverage` / `-fprofile-arcs -ftest-coverage` |
| Dynamic analysis | Valgrind / sanitizer reports if the submitter enabled them |

The dashboard cuts the matrix of *(platform × compiler × HDF5
version × test)* into a navigable surface — a regression that only
fires on Clang 16 + HDF5 1.14.4 + macOS-arm64 shows up as one cell
in the grid, exactly where you'd expect.

## Why this matters for an HDF5 library

HDF5 has a wide combinatorial surface:
- Compilers (gcc 8–14, clang 12–18, msvc 2019/2022, AppleClang)
- HDF5 versions (1.10.x, 1.12.x, 1.14.x — and a deprecated-symbols
  flag on top of each)
- Parallel mode (serial vs MPI; MPICH vs OpenMPI vs Intel MPI)
- Optional features (ROS3, MPI-IO, threadsafe, SZIP, parallel filters)
- Linear-algebra third parties (Armadillo, Eigen, Blaze, …)

No single CI runner can cover every cell of this matrix. CDash
solves the visibility half: contributors can each test the cells
they care about (or have access to), and the aggregated view tells
the maintainers which combinations are healthy and which are
flapping.

## Invitation — run h5cpp's test suite on your architecture

The submission flow is built into the repo's `CMakeLists.txt`. If
you have a build of h5cpp on a platform / configuration that isn't
currently represented on the dashboard (different OS, different
compiler version, exotic CPU architecture, custom HDF5 build), you
can submit your results with **three commands**:

```bash
# Configure the build (any flags you normally use)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DH5CPP_BUILD_TESTING=ON

# Build + run the test suite + submit to the dashboard
cd build
ctest -D Experimental --output-on-failure
```

The `-D Experimental` track is appropriate for one-off submissions
from contributors. (Maintainer machines run `Nightly` or
`Continuous` on a schedule.) `Experimental` builds appear under
their own row on the dashboard, tagged with the submitter's
hostname.

The `CTestConfig.cmake` at the repository root hard-codes:

| Setting              | Value                                          |
| :------------------- | :--------------------------------------------- |
| Project name         | `h5cpp`                                        |
| Submit URL           | `https://my.cdash.org/submit.php?project=h5cpp` |
| Nightly start time   | 01:00 UTC                                      |

— so the `ctest` submission lands at the right project automatically;
no per-user setup required.

### What submitters get back

- A row on the dashboard with your hostname / OS / compiler /
  HDF5 version visible to everyone
- A drill-down link from each test result back to its captured
  stdout/stderr — useful if your platform exposes a real
  regression
- Aggregated comparison with other machines — your build's
  warnings + failures lined up next to everyone else's

### What the project gets back

- Visibility into platform combinations the maintainers don't
  themselves run (ARM Linux, FreeBSD, IBM POWER, AIX, exotic HDF5
  builds, third-party-library combinations)
- Real-world build-time and test-time numbers across architectures
- Early signal on cross-platform regressions before they trip CI

This is genuinely community infrastructure — the dashboard is more
useful the more cells of the matrix get populated.

## Coverage — two paths

H5CPP exposes test coverage via **two complementary services**.
Submitters and reviewers can pick the one that matches their
workflow.

### 1. Codecov (CI-integrated, per-PR view)

The canonical coverage view runs through Codecov's GitHub
integration — every push and pull request to the `release` branch
triggers a coverage upload that lights up the corresponding PR
with a coverage delta comment, a coverage diff annotation, and a
branch-level dashboard:

→ **Codecov dashboard:** <https://app.codecov.io/gh/vargalabs/h5cpp/tree/release>

What you get on Codecov:
- Per-file and per-function coverage percentages for the
  `release` branch
- Per-PR coverage diffs — "this PR adds X% / loses Y% coverage"
- Sunburst + tree visualisations to locate cold zones
- Inline annotations on the GitHub PR diff page

Reviewers normally check Codecov first when evaluating a PR's
test impact — the coverage delta is the at-a-glance signal for
"did the test suite grow with the new code?"

### 2. CDash Coverage tab (per-submitter view)

When a contributor configures their own build with coverage
instrumentation, the same `ctest -D` submission flow pushes
coverage data to the **Coverage** tab of the CDash dashboard:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
               -DCMAKE_CXX_FLAGS="-coverage -fprofile-arcs -ftest-coverage"
cd build
ctest -D ExperimentalCoverage --output-on-failure
```

CDash's Coverage tab is more permissive than Codecov: any
submitter (not just CI) can push coverage from any
build / compiler / HDF5-version combination — useful for
validating coverage on platforms CI doesn't reach (ARM Linux,
exotic HDF5 builds, custom MPI flavours).

| Service | When to use | Granularity |
| :------ | :---------- | :---------- |
| **Codecov** | Reviewing a PR; tracking the `release` branch trendline | Per-file / per-function / per-PR diff on the canonical CI run |
| **CDash Coverage** | Validating an off-CI platform configuration | Per-submitter, per-build — same matrix as the test grid |

Coverage instrumentation is opt-in on the CDash side (the
`-coverage` flags above) and ~2× test-suite duration when active,
so the CDash Coverage submissions typically run separately from
the main build matrix. The Codecov uploads happen automatically
on every CI run — no submitter action required.

## Where to go next

- **CDash dashboard:** <https://my.cdash.org/index.php?project=h5cpp>
- **Codecov coverage:** <https://app.codecov.io/gh/vargalabs/h5cpp/tree/release>
- [CDash documentation (Kitware)](https://docs.cdash.org/)
- [CTest documentation (Kitware)](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [Codecov documentation](https://docs.codecov.com/docs)
- @ref curated_topics_mpi — the MPI variants are part of the
  submission matrix
- The `bench/` suite carries the comparative performance workloads,
  complementary to the CDash testing dashboard
