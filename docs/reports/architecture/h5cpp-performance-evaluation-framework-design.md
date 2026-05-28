@page reports_performance_evaluation_framework_design h5cpp Performance Evaluation Framework — Design Report

**Date:** 2026-05-13  
**Authors:** Winston (Architecture), John (Product), Mary (Business Analysis)  
**Status:** Design ready for MVP implementation  

---

## 1. Executive Summary

h5cpp is at a **credibility inflection point**. The README promises "high-performance persistence" and "fast I/O with direct chunk write/read," yet the repository contains **zero performance benches**. The filtering pipeline rework (#160) is production-ready at the callback level, but the `h5::high_throughput` activation path is broken — meaning most users silently fall back to standard HDF5 C filter execution. We cannot fix what we cannot measure.

This report designs a **performance evaluation framework** that:
- Replaces hand-wavy claims with reproducible, versioned numbers
- Protects against performance regressions in CI
- Creates marketing/academic artifacts (badges, dashboard, release notes)
- Guides architectural decisions (chunk sizes, filter chains, container adapters)

**Recommendation:** Build the MVP in a single sprint. It requires no new external dependencies and delivers immediate value.

---

## 2. Business Context (Mary)

### 2.1 Why Claims Matter Now

h5cpp competes in a crowded space:

| Competitor | Strength | Weakness vs h5cpp |
|------------|----------|-------------------|
| **HDF5 C API** | Ubiquitous, stable | Verbose, error-prone, no C++ type safety |
| **HighFive** | Simple, C++14, École polytechnique pedigree | No custom filter pipeline, less template metaprogramming |
| **h5py** | Pythonic, huge community | 10–100× slower, GIL-bound |
| **ADIOS2** | MPI-optimized, HPC-focused | Heavy dependency, different abstraction (not HDF5-native) |
| **xtensor-io** | Native xtensor integration | Narrow scope, no general HDF5 feature set |

**The buying audience is not end-users — it is technical evaluators** (postdocs choosing a stack, quant engineers vetting dependencies, PI grant writers justifying a C++ choice). These people benchmark before they adopt. If h5cpp has no numbers, it loses to HighFive by default.

### 2.2 Highest-Impact Claims

| Priority | Claim | Target | Evidence Required |
|----------|-------|--------|-------------------|
| P0 | **"h5cpp direct-chunk I/O is N× faster than standard HDF5 C for filtered writes"** | 2–5× | A/B bench: `h5::high_throughput` vs standard path |
| P0 | **"h5cpp container adapters are zero-overhead abstractions"** | <5% overhead | Same workload via raw C API vs h5cpp high-level API |
| P1 | **"libdeflate backend outperforms HDF5 built-in zlib by N×"** | 2–3× | deflate throughput bench with both backends |
| P1 | **"Multi-filter chains (shuffle+gzip) decode at line-rate"** | >1 GB/s | Filter chain decode bandwidth on representative data |
| P2 | **"Compile-time type mapping eliminates runtime type errors"** | N/A | Not a performance claim, but a correctness differentiator |

### 2.3 Claims That Would Backfire

- **"h5cpp is faster than ADIOS2"** — ADIOS2 targets a different use case (MPI collectives, staging). False equivalence.
- **"h5cpp is faster than h5py"** — True but trivial; h5py is Python. This claim signals insecurity.
- **Unqualified "fastest HDF5 wrapper"** — Without specifying workload, compiler, and HDF5 version, this is legally and reputationally dangerous.

**Rule:** Every claim must be qualified by workload shape, filter chain, container type, and HDF5 version.

---

## 3. Product Requirements (John)

### 3.1 User Personas

| Persona | Goal | Frequency | Priority |
|---------|------|-----------|----------|
| **Prospective User** | "Is h5cpp faster than alternatives for my workload?" | Once during evaluation | P0 |
| **CI / Release Engineer** | Detect regressions before they hit `staging` | Every PR | P0 |
| **Library Developer** | Validate refactors don't degrade hot paths | During active dev | P1 |
| **Paper Author** | Reproduce published throughput claims | During peer review | P1 |
| **HPC Systems Engineer** | Tune chunk sizes and filter chains | Pre-deployment | P2 |

### 3.2 Deliverable Tiers

```
Tier 1: Reproducible Benchmark Suite (MVP)
        └── CMake target `h5cpp-bench`
        └── JSON output + console table
        └── Runs locally in <5 minutes

Tier 2: CI Regression Dashboard
        └── GitHub Actions job (advisory, not merge gate)
        └── Compares PR vs `main` baseline
        └── Posts markdown comment on regression >10%

Tier 3: Public Performance Page
        └── GitHub Pages with absolute numbers
        └── Compiler comparison matrix
        └── Filter chain latency breakdowns
        └── Updated on every release
```

### 3.3 Metrics Priority

#### P0 — Headline Throughput (The "Why Should I Care?" Numbers)

| Metric | Workloads |
|--------|-----------|
| Sequential Write Bandwidth (MB/s) | 1D `std::vector<double>`, 2D `arma::mat`, 3D `xtensor` |
| Sequential Read Bandwidth (MB/s) | Same shapes, cold-cache read |
| Chunked Write Bandwidth (MB/s) | Various chunk sizes (cache-line, page-aligned, oversized) |
| Filtered Write Bandwidth (MB/s) | gzip{6}, zstd, lz4, shuffle+gzip |
| `h5::high_throughput` Speedup | Direct chunk vs standard HDF5 path |

#### P1 — Overhead & Latency (The "Is It Zero-Cost?" Numbers)

| Metric | Workloads |
|--------|-----------|
| Container Adapter Overhead | `(h5cpp_time - raw_hdf5_time) / raw_hdf5_time` for identical shapes |
| Small-Object Write Latency | Single 4KB chunk, single struct, small `std::vector` |
| Compile-Time Proxy | Binary size and build time: h5cpp vs raw C API for same I/O |

#### P2 — Scalability & Edge Cases

| Metric | Workloads |
|--------|-----------|
| Large Dataset Streaming | Datasets >> RAM, measuring sustained throughput without OOM |
| Memory Pressure | Peak RSS during datasets >> RAM |
| Filter Chain Decode | Read-back bandwidth for compressed data |

#### P3 — Future (v2)

- Threaded pipeline throughput (blocked on `threaded_pipeline_t` implementation)
- Packet-table append latency
- Extendable dataset amortized growth cost

---

## 4. Architectural Design (Winston)

### 4.1 Principles

1. **No new external dependencies** — h5cpp is header-heavy and dependency-averse. The harness must be vendored or custom.
2. **CMake-native** — `cmake --build build --target h5cpp-bench && ctest --benchmark` must work.
3. **Deterministic on pinned hardware** — CI numbers are only comparable on identical runners. Use GitHub Actions `ubuntu-24.04` with pinned specs.
4. **JSON-first** — All output is machine-readable. Pretty tables are renderings of JSON.
5. **Minimal intrusion** — Benchmarks live in `bench/`; they do not pollute `test/` or `src/`.

### 4.2 Directory Structure

```
bench/
├── CMakeLists.txt              # bench target definitions
├── README.md                   # how to run, how to interpret
├── harness/
│   ├── nanobench.h           # vendored nanobench single-header harness
│   ├── reporter.hpp            # JSON + console formatters
│   └── suite.hpp               # auto-discovery of bench cases
├── workloads/
│   ├── sequential.cpp          # 1D/2D/3D vector/matrix sequential I/O
│   ├── chunked.cpp             # chunked write/read with parameter sweeps
│   ├── filtered.cpp            # filter chain benches
│   ├── containers.cpp          # STL vs Eigen vs Armadillo vs xtensor overhead
│   ├── throughput.cpp          # h5::high_throughput vs standard path
│   └── small_object.cpp        # latency-focused microbenches
└── fixtures/
    └── synthetic.hpp           # deterministic data generators (noise, gradient, zeros)
```

### 4.3 Harness Choice

| Option | Pros | Cons | Verdict |
|--------|------|------|---------|
| **google/benchmark** | Mature, statistical rigor, fixtures | External dependency, heavy, Bazel-centric | v2 only |
| **nanobench** | Header-only, single file, no deps, MIT license | Less statistical depth than google/benchmark | **Locked in** — Steven has already validated it in `libdecimal` |
| **Custom `<chrono>`** | Zero dependencies, full control | Must implement outlier rejection, warming | Not needed; nanobench is proven |

**MVP:** Vendored `nanobench` (single header, MIT license). If it causes build issues on any CI target, fall back to a 200-line custom harness.

### 4.4 Benchmark Methodology

Every bench case follows this protocol:

```
for each (workload, parameter_set):
    generate synthetic data (deterministic seed)
    warm-up: 1 untimed iteration
    measure: 10 timed iterations
    report: median, stddev, min, max, bandwidth (MB/s)
    teardown: delete temp HDF5 file
```

**Critical controls:**
- **File on tmpfs** (where CI permits) to eliminate disk variance
- **Process affinity** (`taskset -c 0` on Linux) to reduce scheduler noise
- **Drop caches** before read benches (`echo 3 > /proc/sys/vm/drop_caches` where permitted)
- **Fixed seed** for synthetic data so compression ratios are reproducible

### 4.5 CI Integration

```yaml
# .github/workflows/benchmark.yml (new file)
name: Benchmark
on:
  push:
    branches: [staging, release]
  workflow_dispatch:
  pull_request:          # advisory only — never blocks merge
    branches: [staging]

jobs:
  benchmark:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4
      - name: Build benchmarks
        run: |
          cmake -B build -DH5CPP_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
          cmake --build build --target h5cpp-bench
      - name: Run benchmarks
        run: ./build/bench/h5cpp-bench --json=results.json
      - name: Download baseline
        uses: actions/download-artifact@v4
        with: {name: bench-baseline, path: baseline/}
        continue-on-error: true
      - name: Compare vs baseline
        run: python3 bench/scripts/compare.py results.json baseline/baseline.json
      - name: Upload results
        uses: actions/upload-artifact@v4
        with: {name: bench-results, path: results.json}
```

**Rules:**
- Benchmarks run in **Release** mode (`-O3 -DNDEBUG`). Debug builds are meaningless.
- **Not a merge gate.** A 12% regression does not block the PR. It posts an advisory comment.
- Baseline is the latest `release` branch artifact. Staging is compared against release, not against itself.

### 4.6 Release Integration

Before tagging a release:
1. Run benches on the release candidate
2. Copy the console summary into the GitHub release notes
3. Archive the JSON as a release asset
4. Trigger a GitHub Pages rebuild that includes the new numbers

This creates a **versioned performance contract**.

---

## 5. MVP Scope

### 5.1 What Gets Built in Sprint 1

| Component | Detail |
|-----------|--------|
| **Harness** | Vendored `nanobench.h` (single header, MIT license) |
| **Workloads** | 5 bench groups (see below) |
| **Metrics** | Bandwidth (MB/s), wall time, stddev over 10 runs |
| **Output** | JSON file + console table |
| **CI** | Single job on `ubuntu-24.04 / gcc-14`, runs on `workflow_dispatch` and `push` to `release` |
| **Docs** | `bench/README.md` |

### 5.2 The Five MVP Benchmark Groups

1. **`sequential_write_read`**  
   1D `std::vector<double>` (1M, 10M, 100M elements). Write then cold-cache read. Baseline for "how fast is h5cpp at simple I/O."

2. **`container_overhead`**  
   2D `arma::mat` (1024×1024) written via: (a) raw HDF5 C API, (b) h5cpp high-level API. Reports `(h5cpp - raw) / raw` overhead ratio.

3. **`chunked_filtered_write`**  
   3D `xtensor` (256×256×256) with `h5::chunk{64,64,64}`. Tests: no filter, gzip{6}, zstd, lz4, shuffle+gzip.

4. **`high_throughput_speedup`**  
   Same data as #3, comparing standard HDF5 write path vs `h5::high_throughput` direct chunk write. **This is the headline number.**

5. **`small_object_latency`**  
   Single `double`, 4KB `std::vector`, single struct with 8 fields. Measures write latency (μs) for tiny payloads.

### 5.3 What Is Explicitly Out of MVP

- Object-store backend benches (S3 greenfield, deferred until backend exists)
- Compile-time metrics (needs build-system instrumentation)
- Threaded pipeline (blocked on implementation)
- macOS / Windows bench CI (Linux is the stable reference)
- Trend graphs / GitHub Pages (needs artifact storage infrastructure)

---

## 6. Risks & Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| GitHub Actions runners are noisy | High | Medium | Run 10 iterations, report median; pin to `ubuntu-24.04`; use tmpfs |
| `h5::high_throughput` is broken | Certain | High | Benchmark the standard path first. Once #160 activation is fixed, add the A/B comparison. |
| Benchmarks add build time | Medium | Low | `H5CPP_BUILD_BENCH` defaults to `OFF`; CI job is separate |
| Claims attract scrutiny | Medium | High | Every claim is qualified by workload, compiler, HDF5 version, and commit hash |
| Competitors improve | Low | Medium | Re-run benches before each release; archive versioned JSON |

---

## 7. Open Questions for Steven

1. **Hardware baseline:** Do you have a dedicated benchmark machine (e.g., an on-prem server or cloud instance) that would give more stable numbers than GitHub Actions? If so, we should run canonical benches there and use CI for regression detection only.

2. **Claim target:** What speedup would make h5cpp's `h5::high_throughput` path compelling? 2×? 5×? This determines whether we pitch it as a marginal improvement or a game-changer.

3. **Competitor comparison:** Should we vendor HighFive and raw HDF5 C benches in the same repo for head-to-head comparison, or keep h5cpp-only and let users run their own comparisons?

4. ~~Nanobench vs custom~~ **Resolved:** nanobench (already validated in `libdecimal`)

---

## Appendix A: Benchmarking Against the Underlying Filesystem

*Steven raised a critical question: what if we bench against raw filesystem I/O?*

### A.1 Why This Is a Game-Changer

Every competitor comparison (HighFive, h5py, ADIOS2) is relative and debatable. But the **filesystem is an absolute ceiling**. No HDF5 wrapper — not h5cpp, not HighFive, not the C API itself — can exceed `fwrite` bandwidth to the same disk. By measuring against raw POSIX I/O, we transform claims from:

> "h5cpp is faster than HighFive" *(debatable, workload-dependent)*

into:

> "h5cpp achieves 85% of raw disk bandwidth; HDF5 C API achieves 72%; HighFive achieves 68%" *(quantified, reproducible, hard to dispute)*

This also answers the **most common objection to HDF5**: *"Why not just dump raw binary?"* We can now quantify exactly what the "HDF5 tax" buys you (self-describing format, cross-platform portability, compression, partial I/O) and what it costs you in bandwidth.

### A.2 Reference Baselines to Implement

| Baseline | Implementation | What It Measures |
|----------|---------------|------------------|
| **Raw POSIX `fwrite`/`fread`** | `fopen`, `fwrite(buf, 1, size, fp)`, `fread` | Absolute disk bandwidth ceiling; no format overhead at all |
| **Memory-mapped POSIX (`mmap`)** | `mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0)` | Zero-copy ceiling; useful for comparing against HDF5's memory mapping (if any) |
| **`tmpfs`/`ramfs` POSIX** | Same as above, file on `/dev/shm` or `tmpfs` mount | Pure library overhead with disk eliminated; isolates h5cpp/HDF5 from storage subsystem |
| **`memcpy` in-memory** | `std::memcpy(dst, src, size)` | RAM bandwidth ceiling; shows how much headroom exists before hitting memory bus limits |

### A.3 The "HDF5 Tax" Decomposition

With filesystem baselines, we can decompose overhead into layers:

```
memcpy           ──────► 100%  (RAM bandwidth ceiling)
   │
   │ - memory bus / cache effects
   ▼
fwrite (tmpfs)   ──────►  ~95%  (pure syscall overhead)
   │
   │ - kernel VFS / page cache
   ▼
fwrite (ext4)    ──────►  ~80%  (filesystem + disk overhead)
   │
   │ - metadata (superblock, inode, journaling)
   ▼
HDF5 C API       ──────►  ~70%  (HDF5 "tax": B-tree updates, datatype encoding, property lists)
   │
   │ - C++ wrapper overhead (templates, type dispatch)
   ▼
h5cpp std path   ──────►  ~68%  (h5cpp standard-path overhead)
   │
   │ - direct chunk bypasses HDF5 filter pipeline
   ▼
h5cpp high_thrpt ──────►  ~78%  (h5cpp direct-chunk path, closer to C API)
```

*(Percentages are illustrative — the framework will measure actual numbers.)*

This decomposition is **marketing gold**. It lets us say:
- "h5cpp adds only 2% overhead over raw HDF5 C API for standard writes"
- "h5cpp direct-chunk path recovers 10 percentage points of bandwidth by bypassing the HDF5 filter pipeline"
- "The total cost of self-describing, portable, compressed I/O is 22% of raw disk bandwidth"

### A.4 New Benchmark Groups (Filesystem-Aware MVP)

The 5-group MVP should expand to include filesystem baselines:

**Group 6: `filesystem_ceiling`**  
Run identical payload sizes through `fwrite`, `fread`, `mmap`, and `memcpy`. Report bandwidth for each. This is the **reference bar** against which all HDF5 numbers are normalized.

**Group 7: `hdf5_tax_by_size`**  
Write payloads from 1 KB to 1 GB in powers of 10. For each size, run: `fwrite` → HDF5 C API → h5cpp standard → h5cpp high_throughput. Plot the **overhead ratio** `(hdf5_time - fwrite_time) / fwrite_time` vs payload size. This reveals where HDF5 metadata overhead dominates (small writes) and where it amortizes away (large writes).

**Group 8: `tmpfs_isolation`**  
Run the full bench suite (groups 1–5) twice: once on `ext4` and once on `tmpfs`. The difference isolates **storage subsystem variance** from **library overhead**. If numbers are nearly identical on `tmpfs`, the bottleneck is in h5cpp/HDF5, not the disk.

### A.5 Claim Refinement With Filesystem Baselines

| Original Claim (Relative) | Refined Claim (Absolute, with Filesystem Baseline) |
|---------------------------|---------------------------------------------------|
| "h5cpp is faster than HighFive" | "h5cpp achieves 680 MB/s vs HighFive's 620 MB/s on the same workload, against a raw `fwrite` ceiling of 950 MB/s" |
| "h5cpp has zero-overhead container adapters" | "h5cpp Eigen adapter adds 3% overhead vs raw HDF5 C API, and 29% overhead vs raw `fwrite`" |
| "h5::high_throughput is fast" | "h5::high_throughput achieves 82% of raw disk bandwidth; standard HDF5 path achieves 68%" |

### A.6 Methodology Notes

- **Block alignment:** Raw `fwrite` should use the same buffer alignment as HDF5 (usually 4 KB or 8 KB page-aligned) to make comparisons fair.
- **Sync behavior:** Call `fflush` + `fsync` after `fwrite` to match HDF5's default durability semantics. Or benchmark both sync and async paths.
- **`tmpfs` size:** Ensure `/dev/shm` is large enough for the biggest payload (1 GB test needs 1 GB + overhead of free RAM).
- **Cold-cache reads:** For read benchmarks, drop OS page caches (`echo 3 > /proc/sys/vm/drop_caches`) before both `fread` and `h5::read` to ensure neither benefits from warm cache.
- **File deletion:** Unlink immediately after open to eliminate filesystem directory-update overhead where possible (`O_TMPFILE` on Linux, or `unlink` after `fopen`).

### A.7 Deliverable Impact

Adding filesystem baselines turns the bench suite from a **marketing tool** into a **systems analysis tool**. It becomes useful for:
- **Users deciding "HDF5 vs raw binary"** — quantified tradeoff
- **Developers optimizing h5cpp** — identifies whether slowdowns are in h5cpp, HDF5, or the disk
- **Academic reviewers** — rigorous, absolute baseline methodology
- **h5cpp maintainers** — catch regressions that push h5cpp further from the filesystem ceiling

**Recommendation:** Include `filesystem_ceiling` and `hdf5_tax_by_size` in the MVP. They add ~200 lines of C++ but multiply the credibility of every other benchmark.

---

## 8. Recommended Next Steps

1. **Create GitHub issue:** `perf, establish MVP bench suite for I/O throughput claims`
2. **Branch:** `NNN-perf-bench-mvp` (where NNN is the next available issue number)
3. **Sprint scope:**
   - Day 1–2: Set up `bench/` directory, CMake integration, harness
   - Day 3–4: Implement the 5 MVP bench groups
   - Day 5: CI job, JSON output, `bench/README.md`
4. **Target merge:** After `h5::high_throughput` activation fix (post-#160 follow-up)

---

*End of report.*
