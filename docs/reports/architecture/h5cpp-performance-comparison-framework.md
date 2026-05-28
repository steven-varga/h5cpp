@page reports_performance_comparison_framework h5cpp I/O System — Comparative Evaluation Framework

**Version:** 1.0-draft  
**Date:** 2026-05-25  
**Author:** Winston (System Architect)  
**Scope:** Performance and feature comparison of h5cpp against HDF5 C API baseline, official HDF5 C++ API, and HighFive  
**Status:** Framework specification — awaiting instrumentation and measurement

---

## 1. Executive Summary

This document specifies a rigorous, evidence-based framework for comparing h5cpp's I/O system against three baselines:

| Baseline | Description | Role in Comparison |
|---|---|---|
| **HDF5 C API** | Raw HDF5 C library (libhdf5) | Absolute performance ceiling; control baseline |
| **Official HDF5 C++ API** | HDF5 Group's bundled C++ wrapper (`hdf5_cpp`) | Legacy abstraction baseline |
| **HighFive** | Modern header-only C++14 wrapper | Direct ergonomic competitor |
| **h5cpp** | C++17 header-only template library | Target of evaluation |

**Core principles:**

1. **No speculation.** Compile-time cost, memory overhead, and throughput claims require measured evidence.
2. **Controlled variables.** Hardware, OS, HDF5 library version, compiler, and build flags are locked across all APIs.
3. **Stakeholder-weighted scoring.** Feature comparisons are weighted by Jobs-to-be-Done, not checkbox counts.
4. **Statistical rigor.** Performance claims require non-overlapping 95% confidence intervals.

**Key correction from prior analysis:** `h5::high_throughput` is an **opt-in** data-transfer path that trades hyperslab flexibility for direct-chunk bandwidth. The standard h5cpp I/O path retains full hyperslab, stride, block, and scatter/gather support. Comparisons must evaluate the standard path and the high-throughput path as separate configurations.

---

## 2. Performance Comparison Framework

### 2.1 First Principles

- Lock the environment, or the data is noise.
- Report median and interquartile range (IQR), never mean alone.
- Minimum *n* = 30 repetitions for throughput; discard first 5 as warm-up.
- Use paired Wilcoxon signed-rank test for significance testing (non-parametric, robust to outliers).
- No performance claim is valid unless 95% confidence intervals do not overlap.

### 2.2 Controlled Variables

| Layer | Variable | Control |
|---|---|---|
| **Hardware** | CPU | Single-socket x86_64, fixed clock (turbo disabled), model documented |
| | Memory | DDR4/DDR5 configuration documented, no swap pressure |
| | Storage | Local NVMe for baseline; separate parallel-filesystem test if relevant |
| **Software** | OS | Same kernel version, same page cache state (drop caches between runs) |
| | HDF5 | Exact same version (e.g., 1.14.x), same build flags, thread-safe setting documented |
| | VFD | `sec2` driver for baseline; MPI-IO separately |
| **Compiler** | Toolchain | Same GCC or Clang version, same `-O3`, same C++ standard (C++17) |
| | LTO | Off for baseline; separate LTO-on test if requested |
| **Build** | Linking | Static vs. dynamic documented; stripped binaries for size tests |
| | ccache | Disabled for compile-time measurements |

### 2.3 Benchmark Categories

| # | Category | What We Measure | Controlled Variables |
|---|---|---|---|
| 1 | **Contiguous Throughput** | Read/write GB/s for 1D–4D contiguous arrays | Array size (1 MB → 10 GB), dimensionality, datatype (int32, float64, compound) |
| 2 | **Chunked Throughput** | Read/write GB/s for chunked datasets | Chunk size (small / matched / large), dataset shape |
| 3 | **Filtered I/O** | Throughput with deflate, shuffle, fletcher32 | Compression level (1, 6, 9), filter pipeline combinations |
| 4 | **Direct-Chunk Access** | Raw chunk bypass throughput | Pre-filtered vs. post-filtered data; chunk dimensions |
| 5 | **Hyperslab Selection** | Offset, stride, block, count on **standard path** | Selection complexity, selection size, dimensionality |
| 6 | **Scatter/Gather** | Multiple selections per read/write call | Number of selections, total bytes, fragmentation pattern |
| 7 | **Small-Object Latency** | Dataset create, attribute read/write, group traversal | Object count (1 → 10,000), attribute size, nesting depth |
| 8 | **Memory Overhead** | Peak RSS, temporary allocations, handle footprint | Massif or `/proc/pid/status`; report delta vs. C API baseline |
| 9 | **Binary Size Impact** | Text/data segment increase from wrapping code | Static vs. dynamic linking, stripped, with/without LTO |
| 10 | **Compile-Time Cost** | Clean build, incremental build, template instantiation bloat | `clang -ftime-report`; instantiate for *N* = {10, 50, 100} types; **measure, do not guess** |

### 2.4 Test Matrix

| Axis | Values |
|---|---|
| **Data sizes** | 1 KB, 1 MB, 100 MB, 1 GB, 10 GB |
| **Datatypes** | float64, int32, compound (3 fields), variable-length, fixed-length string |
| **Dimensionality** | 1D, 2D, 3D, 4D |
| **Access patterns** | Sequential, strided (various strides), block/subset, multi-point, random |
| **Chunk scenarios** | Unchunked, aligned single chunk, misaligned partial chunk, cross-chunk boundary |
| **Filters** | None, deflate (1, 6, 9), shuffle, shuffle+deflate, fletcher32 |
| **Concurrency** | Single-threaded, multi-threaded (with/without HDF5 thread-safety build) |
| **I/O backend** | POSIX (`sec2`), MPI-IO (if applicable) |

### 2.5 Metrics to Capture

| Metric | Unit | Reporting |
|---|---|---|
| Throughput (read / write) | bytes/second | Median + IQR, reported separately for read and write |
| Latency to first byte | microseconds | p50, p99 |
| CPU utilization | % of single core | During I/O phase only |
| Memory high-water mark | bytes | Delta vs. C API baseline |
| Temporary allocations | count + bytes | Via `malloc_count` or equivalent |
| Binary segment sizes | bytes | text, data, BSS (stripped) |
| Build wall-clock time | seconds | Median of *n* = 10 clean builds |
| Build user time | seconds | Median of *n* = 10 clean builds |

### 2.6 Statistical Methodology

1. **Warm-up:** Discard first 5 iterations.
2. **Repetitions:** *n* ≥ 30 for throughput; *n* ≥ 100 for latency.
3. **Reporting:** Median, standard deviation, coefficient of variation.
4. **Significance:** Paired Wilcoxon signed-rank test between APIs.
5. **Decision gate:** No claim of superiority unless 95% CIs are disjoint.
6. **Reproducibility:** Every benchmark committed as code; CI job fails if benchmark drifts > 5% from baseline.

---

## 3. Feature Comparison Framework

### 3.1 First Principles

- Feature checklists lie. Score on a gradient (0–3), not binary presence.
- Weight by stakeholder criticality and Jobs-to-be-Done.
- Evidence required: working code in a public repo or documented API.
- Performance-linked features require benchmark proof to score "optimal."

### 3.2 Stakeholder Segments & Jobs-to-be-Done

| Stakeholder | Primary Job-To-Be-Done | Critical Dimensions |
|---|---|---|
| **HPC Engineer** | "Move terabytes between memory and storage without becoming an HDF5 C API programmer." | Throughput, direct-chunk access, parallel I/O, compression control |
| **Quant Dev** | "Serialize my Eigen/Armadillo objects with one call and get them back fast." | Container adaptors, type safety, small-object latency, compile-time checks |
| **Data Scientist** | "Read a slab of my 4D data without loading the whole cube." | Hyperslab ergonomics, partial I/O, dataset slicing |
| **Maintainer / Release Engineer** | "Upgrade HDF5 underneath without rewriting our I/O layer; debug production crashes." | ABI stability, error handling, test coverage, documentation |

### 3.3 Scoring Rubric (0–3)

| Score | Label | Definition | Evidence Required |
|---|---|---|---|
| **0** | Not supported | No API surface; or explicitly documented as unsupported | Link to docs or absence in API |
| **1** | Supported, manual | Requires > 5 lines of boilerplate or escape to C API; no compile-time safety | Working code sample |
| **2** | Supported, ergonomic | Single-function call; compile-time type checking where applicable | Working code sample + compile verification |
| **3** | Supported, optimal | Single call **and** within 10% of C API throughput **or** provides safety/ergonomics impossible in C | Working code + benchmark proof |

### 3.4 Dimensions of Comparison

#### Core I/O (Weight: 30% — all stakeholders)

| Feature | Assessment Criteria |
|---|---|
| Contiguous read/write | 0–3 based on ergonomics and performance |
| Chunked read/write | 0–3 based on ergonomics and performance |
| Hyperslab selection (offset, stride, block, count) | **h5cpp standard path evaluated separately from `h5::high_throughput`** |
| Attribute read/write | 0–3 based on type coverage and ergonomics |
| String handling (fixed vs. variable length) | 0–3 based on UTF-8 support and zero-copy potential |

#### Performance Specialization (Weight: 25% — HPC / Quant)

| Feature | Assessment Criteria |
|---|---|
| Direct-chunk I/O bypass | Only score if API exposes `H5Dread_chunk` / `H5Dwrite_chunk`; otherwise N/A |
| High-throughput / batched path | Document as opt-in trade-off; measure bandwidth vs. flexibility |
| Filter pipeline control (compression, shuffle, checksum) | 0–3 based on composability and runtime control |
| Zero-copy / memory-mapped interfaces | 0–3 based on evidence of avoidance of intermediate buffers |

#### Container & Type System Integration (Weight: 20% — Quant / Data Science)

| Feature | Assessment Criteria |
|---|---|
| STL containers (vector, array, deque, list, set, map) | 0–3 per container; time to first successful write |
| Linear algebra backends (Eigen, Armadillo, Blaze, xtensor) | 0–3 per backend; evidence of zero-copy I/O |
| Custom type registration (structs / PODs) | 0–3 based on manual vs. compiler-assisted reflection |
| Compound / nested types | 0–3 based on depth supported and ergonomics |

#### Robustness & Observability (Weight: 15% — Maintainer / Quant)

| Feature | Assessment Criteria |
|---|---|
| Error handling model | Exception hierarchy vs. error codes vs. both; HDF5 error stack preservation |
| RAII resource management | Handle lifecycle, move semantics, double-close prevention |
| Thread-safety guarantees | Documentation + sanitizer-clean evidence (TSan) |
| Debuggability | Template error message quality, sanitizer compatibility (ASan/UBSan) |

#### Ecosystem & Lifecycle (Weight: 10% — Maintainer)

| Feature | Assessment Criteria |
|---|---|
| Build system integration | CMake `find_package`, pkg-config, Conan, vcpkg |
| Packaging | Pre-built binaries (deb, rpm, pkg, exe) vs. source-only |
| Documentation | API reference, examples, migration guide, architecture docs |
| Maintenance | Test coverage %, CI matrix breadth, release cadence, issue response time |

### 3.5 Weighting by Persona

Do **not** publish a single winner. Publish weighted scorecards:

| Persona | Performance | Core I/O | Type System | Robustness | Ecosystem |
|---|---|---|---|---|---|
| HPC Engineer | 40% | 30% | 10% | 10% | 10% |
| Quant Dev | 20% | 20% | 35% | 15% | 10% |
| Data Scientist | 15% | 35% | 25% | 10% | 15% |
| Library Maintainer | 10% | 20% | 15% | 25% | 30% |

### 3.6 Decision-Weighted Scoring (Cross-Check)

For each feature, ask: *"Would a user choose Library X over Library Y solely because of this?"*

| Weight Class | Points | Definition |
|---|---|---|
| **Blocking** | 3 | Missing this prevents the job from being done at all |
| **Differentiating** | 2 | Does the job materially better (faster, simpler, more robust) |
| **Nice-to-have** | 1 | Better, but not a decision driver |
| **Checkbox** | 0 | Technically supported, but irrelevant to real usage |

**Sensitivity analysis:** Re-run with inverted weights to ensure no API wins only because of a single dimension.

### 3.7 Evidence Thresholds

1. A feature scores > 0 only if supported by **working code in a public repository** or **documented public API**.
2. "Experimental" or "undocumented" usage caps the score at 1.
3. Performance-linked features (e.g., "Eigen support") score 3 only with benchmark proof of within 10% of raw pointer throughput. Otherwise cap at 2.
4. Compile-time features (e.g., "template type resolution") score 3 only if accompanied by measured compile-time data.

---

## 4. Comparative Matrix Template

The following template is populated with measured evidence as benchmarks and feature audits are completed.

### 4.1 Performance Results

| Benchmark | C API | HDF5 C++ API | HighFive | h5cpp (standard) | h5cpp (high_throughput) |
|---|---|---|---|---|---|
| Contiguous 1D write (1 GB float64) | — | — | — | — | — |
| Contiguous 1D read (1 GB float64) | — | — | — | — | — |
| Chunked write (1 GB, 1 MB chunks) | — | — | — | — | — |
| Filtered write (deflate-6, 1 GB) | — | — | — | — | — |
| Direct-chunk write (1 GB) | — | — | N/A | — | — |
| Hyperslab strided read (2D, stride=2) | — | — | — | — | N/A |
| Scatter/gather (10 selections) | — | — | — | — | N/A |
| Small-object create (10K datasets) | — | — | — | — | — |
| Attribute write latency (p99) | — | — | — | — | — |
| Peak RSS delta (1 GB write) | — | — | — | — | — |
| Binary size (minimal executable) | — | — | — | — | — |
| Clean build time (simple project) | — | — | — | — | — |
| Incremental build (one header touch) | — | — | — | — | — |

*Units: throughput in GB/s, latency in μs, memory in MB, size in KB, time in seconds.*

### 4.2 Feature Scorecard

| Dimension | Weight | C API | HDF5 C++ API | HighFive | h5cpp |
|---|---|---|---|---|---|
| **Core I/O** | 30% | | | | |
| ├─ Contiguous read/write | | | | | |
| ├─ Chunked read/write | | | | | |
| ├─ Hyperslab selection | | | | | |
| ├─ Attribute handling | | | | | |
| ├─ String support | | | | | |
| **Performance Specialization** | 25% | | | | |
| ├─ Direct-chunk bypass | | | | | |
| ├─ High-throughput path | | | | | |
| ├─ Filter control | | | | | |
| ├─ Zero-copy potential | | | | | |
| **Type System** | 20% | | | | |
| ├─ STL containers | | | | | |
| ├─ Linear algebra backends | | | | | |
| ├─ Custom struct registration | | | | | |
| ├─ Compound / nested types | | | | | |
| **Robustness** | 15% | | | | |
| ├─ Error handling | | | | | |
| ├─ RAII / move semantics | | | | | |
| ├─ Thread safety | | | | | |
| ├─ Debuggability | | | | | |
| **Ecosystem** | 10% | | | | |
| ├─ Build system support | | | | | |
| ├─ Packaging | | | | | |
| ├─ Documentation | | | | | |
| ├─ Maintenance / CI | | | | | |
| **Weighted Total** | **100%** | | | | |

---

## 5. Decision Gates

Before any comparison claim is published:

| # | Gate | Criterion |
|---|---|---|
| 1 | **Completeness** | All four APIs tested on identical hardware, OS, HDF5 version, and compiler |
| 2 | **Significance** | Performance differences are statistically significant (non-overlapping 95% CIs) |
| 3 | **Reproducibility** | All benchmark code committed; CI can re-run the full suite with one command |
| 4 | **Stakeholder alignment** | Weighting reflects the target user persona; sensitivity analysis performed |
| 5 | **Honesty** | All "N/A" entries explicitly marked; no omissions by design |
| 6 | **Compile-time evidence** | Build-time measurements exist; no compile-time claims without `-ftime-report` or `time -p` data |
| 7 | **High-throughput clarity** | `h5::high_throughput` documented as opt-in; standard path and high-throughput path scored separately |

---

## 6. Implementation Plan

### Phase 1: Instrumentation (Estimated: 2–3 weeks)

1. **Benchmark harness:** Extend existing h5cpp bench suite (`worktrees/h5cpp/staging/bench/`) to execute equivalent workloads across all four APIs.
2. **Compile-time harness:** Create representative translation units of increasing template complexity; measure with `time -p` and `clang -ftime-report`.
3. **Feature audit:** Implement the same 5 representative programs in each API; measure lines of code, time-to-correctness, and compile-error quality.

### Phase 2: Measurement (Estimated: 1–2 weeks)

1. Run full benchmark matrix on controlled hardware.
2. Collect *n* = 30+ repetitions per configuration.
3. Run statistical analysis (median, IQR, Wilcoxon test).

### Phase 3: Reporting (Estimated: 1 week)

1. Populate Comparative Matrix Template (Section 4).
2. Produce persona-weighted scorecards.
3. Publish with full methodology, raw data, and reproduction instructions.

---

## 7. Open Questions

1. **Compile-time measurement protocol:** Who builds the controlled harness? Do we measure total build time, template instantiation time only, or incremental build time after a single header change?
2. **Feature weight validation:** Do we have telemetry, issue frequency, or user survey data to justify the persona weights? If not, we flag them as "team assumption — needs validation."
3. **HighFive version pinning:** Which HighFive release? Latest stable (as of measurement date) or a specific tagged version?
4. **MPI-IO scope:** Is MPI-IO comparison in scope for Phase 1, or deferred to Phase 2?
5. **Direct-chunk for HighFive:** Does HighFive expose `H5Dread_chunk` / `H5Dwrite_chunk`? If not, mark N/A rather than 0.

---

## 8. References

- h5cpp bench suite: `worktrees/h5cpp/staging/bench/README.md`
- h5cpp architecture docs: `worktrees/h5cpp/staging/docs/architecture.md`
- h5cpp I/O operators: `worktrees/h5cpp/staging/h5cpp/H5Dwrite.hpp`, `H5Dread.hpp`
- HDF5 C API reference: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_d.html
- HighFive repository: https://github.com/BlueBrain/HighFive

---

*This framework is a living document. Revisions require passing all seven Decision Gates (Section 5).*
