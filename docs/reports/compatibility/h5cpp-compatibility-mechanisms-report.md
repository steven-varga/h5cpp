@page reports_compatibility_mechanisms h5cpp Compatibility Mechanisms — Architectural Review

**Date:** 2026-05-13
**Scope:** HDF5 version, C++ compiler, and C++ standard compatibility checking across the h5cpp codebase
**Branch:** `origin/staging` (02b3e5eb)

---

## 1. HDF5 Compatibility

### 1.1 CMake-Level Checks (Coarse-Grained)

| Location | Mechanism | What it checks |
|----------|-----------|----------------|
| `CMakeLists.txt:83` | `find_package(HDF5 REQUIRED COMPONENTS C)` | Locates HDF5 C library and exports `HDF5_VERSION`, `HDF5_INCLUDE_DIRS`, `HDF5_LIBRARIES`, `HDF5_IS_PARALLEL`. |
| `CMakeLists.txt:85–88` | `if(HDF5_VERSION VERSION_LESS ${H5CPP_BASE_VERSION})` | **Fatal error** if installed HDF5 is older than the library's base version (currently `1.10.4`). |
| `CMakeLists.txt:95–97` | `if(HDF5_VERSION VERSION_GREATER 1.12.0)` | Informational message enabling "KITA support". |
| `CMakeLists.txt:100–103` | `if(MPI_FOUND AND HDF5_IS_PARALLEL)` | Detects parallel HDF5 (PHDF5) and emits status message. |
| `examples/CMakeLists.txt:12–18` | Same `find_package(HDF5)` + version check | Examples sub-project repeats the fatal version check. |
| `examples/CMakeLists.txt:163` | `if(MPI_FOUND AND HDF5_IS_PARALLEL)` | Gates MPI example compilation. |
| `docker/mpi-CMakeLists.txt:9` | `find_package(HDF5 REQUIRED COMPONENTS C HL)` | MPI docker build also requires HDF5 HL. |

### 1.2 Preprocessor Guards in Headers (Fine-Grained, Per-Property)

These come from `<hdf5.h>` (`H5_VERSION_GE` / `H5_VERSION_LE` macros).

| File | Macro | Feature gated |
|------|-------|---------------|
| `h5cpp/H5Pall.hpp:154` | `#if H5_VERSION_GE(1,8,0)` | FCPL properties: `sizes`, `sym_k`, `istore_k`, `shared_mesg_*` |
| `h5cpp/H5Pall.hpp:162` | `#if H5_VERSION_GE(1,10,0)` | FCPL `userblock` |
| `h5cpp/H5Pall.hpp:165` | `#if H5_VERSION_GE(1,10,1)` | FCPL `file_space_page_size`, `file_space_page_strategy` |
| `h5cpp/H5Pall.hpp:193` | `#if H5_VERSION_GE(1,8,2)` | FAPL `driver` |
| `h5cpp/H5Pall.hpp:196` | `#if H5_VERSION_GE(1,8,9)` | FAPL `file_image`, `elink_file_cache_size` |
| `h5cpp/H5Pall.hpp:201` | `#if H5_VERSION_GE(1,8,14)` | FAPL `core_write_tracking`, `fapl_log` |
| `h5cpp/H5Pall.hpp:205` | `#if H5_VERSION_GE(1,10,1)` | FAPL `page_buffer_size`, `evict_on_close`, `metadata_read_attempts`, `mdc_config`, `mdc_image_config`, `mdc_log_options` |
| `h5cpp/H5Pall.hpp:213` | `#if H5_VERSION_GE(1,14,0)` | FAPL `fapl_direct` (guarded additionally by `H5_HAVE_DIRECT`) |
| `h5cpp/H5Pall.hpp:261` | `#if H5_VERSION_GE(1,9,0)` | LACL `elink_fapl` |
| `h5cpp/H5Pall.hpp:270` | `#if H5_VERSION_GE(1,8,16)` | DCPL `chunk` |
| `h5cpp/H5Pall.hpp:273` | `#if H5_VERSION_GE(1,10,0)` | DCPL `layout`, `chunk_opts` |
| `h5cpp/H5Pall.hpp:311` | `#if H5_VERSION_GE(1,10,0)` | DXPL `type_conv_cb` |
| `h5cpp/H5Pall.hpp:222` | `#if H5_HAVE_WIN32_API` | FAPL `fapl_windows` (Windows-only) |
| `h5cpp/H5Pall.hpp:173` | `#ifdef H5_HAVE_PARALLEL` | FCPL `strategy_page` (commented out) |
| `h5cpp/H5Pall.hpp:346` | `#ifdef H5_HAVE_PARALLEL` | FAPL `fapl_mpiio`, `all_coll_metadata_ops`, `coll_metadata_write` |
| `h5cpp/H5Pdcpl.hpp:33` | `#if H5_VERSION_GE(1,8,16)` | DCPL `chunk` |
| `h5cpp/H5Pdcpl.hpp:36` | `#if H5_VERSION_GE(1,10,0)` | DCPL `layout`, `chunk_opts` |
| `h5cpp/H5Pdapl.hpp:16` | `#if H5_VERSION_LE(1,10,6)` | High-throughput pipeline insert disabled for HDF5 > 1.10.6 (crash workaround) |
| `h5cpp/H5Pdapl.hpp:31` | `#if H5_VERSION_GE(1,10,0)` | DAPL `efile_prefix`, `virtual_view`, `virtual_printf_gap` |
| `h5cpp/H5cout.hpp:14` | `#ifdef H5_HAVE_PARALLEL` | MPI I/O mode diagnostics in `operator<<` |
| `h5cpp/H5cout.hpp:64` | `#if H5_VERSION_GE(1,10,0)` | Empty block (placeholder) |
| `h5cpp/H5Zpipeline_basic.hpp:40` | `#if H5_VERSION_GE(2,0,0)` | `H5Dread_chunk` with `H5ES_NONE` (HDF5 2.0 async API) |
| `h5cpp/H5Zpipeline_basic.hpp:52` | `#if H5_VERSION_GE(2,0,0)` | Same for the read path in filtered pipeline |

### 1.3 Optional Compression / Filter Feature Macros

| File | Macro | How it's set |
|------|-------|--------------|
| `h5cpp/H5Zall.hpp:14–23` | `H5CPP_DISABLE_LIBDEFLATE` / `H5CPP_HAS_LIBDEFLATE` | CMake option `H5CPP_USE_LIBDEFLATE`; falls back to `__has_include(<libdeflate.h>)` |
| `h5cpp/H5Zall.hpp:25` | `H5CPP_HAS_LZ4` | Set by CMake when system `lz4` is found (`-DH5CPP_USE_LZ4=ON`) |
| `h5cpp/H5Zall.hpp:29` | `H5CPP_HAS_ZSTD` | Set by vendored `thirdparty/zstd/CMakeLists.txt` |
| `h5cpp/H5Zall.hpp:33` | `H5CPP_HAS_SZIP` | Set by vendored `thirdparty/szip/CMakeLists.txt` |

---

## 2. C++ Compiler Compatibility

### 2.1 CMake-Level Compiler / Platform Detection

| Location | Mechanism | What it checks |
|----------|-----------|----------------|
| `CMakeLists.txt:63–78` | `if(WIN32)`, `elseif(UNIX)`, `if(APPLE)`, `if(ANDROID)` | Platform messages and default install prefix (`C:/h5cpp` on Windows). |
| `test/CMakeLists.txt:96` | `if(MSVC)` | Adds `/STACK:67108864` linker flag for Windows stack-probe test. |
| `test/CMakeLists.txt:101` | `if(WIN32)` | Gates all Windows lifecycle/write-path/dapl probe tests. |
| `examples/CMakeLists.txt:29–34` | `if(WIN32)`, `if(APPLE)` | Warning messages about limited testing. |
| `examples/CMakeLists.txt:225` | `if(MSVC)` | Defines `_CRT_SECURE_NO_WARNINGS` for CSV example. |
| `.github/workflows/ci.yml` | Matrix: `gcc-13/14`, `clang-17/18/19/20`, `apple-clang`, `msvc` | CI coverage across compilers and OSs. |

**Notable absence:** There are zero `CMAKE_CXX_COMPILER_ID` checks, versioned compiler blacklists, or `try_compile` feature probes. h5cpp trusts `CMAKE_CXX_STANDARD` and `target_compile_features` entirely.

### 2.2 Preprocessor Guards in Headers

| File | Guard | Purpose |
|------|-------|---------|
| `h5cpp/H5Zpipeline.hpp:14` | `#ifdef _MSC_VER` | Includes `<malloc.h>` for `_aligned_malloc` / `_aligned_free`. |
| `h5cpp/H5Zpipeline.hpp:26` | `#ifdef _MSC_VER` | Uses `_aligned_free(ptr)` instead of `std::free`. |
| `h5cpp/H5Zpipeline.hpp:42` | `#ifdef _MSC_VER` | Uses `_aligned_malloc(...)` instead of `posix_memalign`. |
| `h5cpp/H5Dgather.hpp:126` | `#ifndef _MSC_VER` | **Disables `static_assert`** in unsupported fallback path because MSVC fails on it. |
| `h5cpp/H5Dgather.hpp:135` | `#ifndef _MSC_VER` | Same for `materialize` fallback. |
| `h5cpp/H5Dgather.hpp:143` | `#ifndef _MSC_VER` | Same for sized `materialize` fallback. |

### 2.3 Known Compiler-Bug Workarounds

| Location | Bug | Workaround |
|----------|-----|------------|
| `h5cpp/H5Dwrite.hpp:119–128` | MSVC partial-ordering bug: variadic forwarder ambiguous with inner `h5::write` for `(ds, sp_t, sp_t, dxpl_t, T*)`. | Inlines raw `H5Dwrite` CAPI call instead of calling `h5::write` recursively. |
| `h5cpp/H5Iall.hpp:126` | Clang crash with delegating constructor `using parent::hid_t`. | Explicitly inherits base constructors via `using hid_t<T,...>::hid_t` instead of `using parent::hid_t`. |
| `h5cpp/H5Pdapl.hpp:47–55` | Windows MSVC static-destruction-order fiasco with HDF5 atexit. | Heap-allocates singleton `dapl_t` objects intentionally leaked to avoid race. |

### 2.4 Windows-Specific Regression Tests

| Test File | CMake Registration | What it probes |
|-----------|-------------------|----------------|
| `test/win_stack_probe.cpp` | `test-win_stack_probe` (all platforms) | MSVC stack exhaustion / deep recursion. |
| `test/win_hdf5_lifecycle_probe.cpp` | Modes 1–8 under `if(WIN32)` | HDF5 object lifetime across create/open/write/read/exit. |
| `test/win_h5dwrite_path_probe.cpp` | Modes 1–8 under `if(WIN32)` | `h5::write` overload resolution for dataset vs file descriptor paths. |
| `test/win_dapl_probe.cpp` | Modes 1–10 under `if(WIN32)` | DAPL handle validity and singleton behavior. |

---

## 3. C++ Standard Compatibility

### 3.1 CMake-Level Standard Enforcement

| Location | Setting | Meaning |
|----------|---------|---------|
| `CMakeLists.txt:11–13` | `set(CMAKE_CXX_STANDARD 17)`, `REQUIRED ON`, `EXTENSIONS OFF` | **Baseline is C++17**, no GNU extensions. |
| `CMakeLists.txt:169` | `target_compile_features(h5cpp INTERFACE cxx_std_17)` | Consumers must have C++17. |
| `test/CMakeLists.txt:38` | `CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF` | Tests enforce C++17 per-target. |
| `examples/CMakeLists.txt:27` | `set(CMAKE_CXX_STANDARD 17)` | Examples enforce C++17. |
| `docker/mpi-CMakeLists.txt:4–6` | Same C++17 settings | MPI examples enforce C++17. |
| `.github/workflows/ci.yml:282,313` | `-DCMAKE_CXX_STANDARD=23` | **CI builds with C++23**, not the declared C++17 baseline. |
| `.github/workflows/ci.yml:486` | `-DCMAKE_CXX_STANDARD=17` | Coverage job correctly uses C++17. |

### 3.2 Preprocessor Standard Checks

**Finding: h5cpp's own headers do NOT check `__cplusplus`, `_MSVC_LANG`, or any C++ feature-test macros.** The library unconditionally assumes C++17 availability.

The only `__has_include` usage in core h5cpp:
- `h5cpp/H5Zall.hpp:17–18`: `#elif defined(__has_include)` → `#if __has_include(<libdeflate.h>)` for optional libdeflate.

All other `__cplusplus` / `_MSVC_LANG` checks are in **third-party** code (xtensor, blaze, zstd, ublas, half.hpp).

### 3.3 C++17 Features Used (Assumed Available)

| Feature | Where Used |
|---------|-----------|
| `if constexpr` | `H5Dwrite.hpp`, `H5Dcreate.hpp`, `H5Dappend.hpp`, `H5Tmeta.hpp`, `H5Tall.hpp` |
| `std::enable_if_t` | `H5Dappend.hpp`, `H5Awrite.hpp`, `H5Aread.hpp`, `H5Marma.hpp`, etc. |
| `std::is_same_v`, `std::is_convertible_v`, `std::disjunction_v`, `std::conjunction_v` | `H5Tmeta.hpp`, `H5meta.hpp` |
| `std::bool_constant` | `H5Tmeta.hpp` |
| `std::string_view` | `H5Tmeta.hpp` |
| Fold expressions | `H5meta.hpp` (`static_for` callback unpacking) |
| `auto` return type deduction | Widespread |

### 3.4 Legacy C++11/14 Back-Port Shims (Redundant Dead Weight)

| File | Shim | Status |
|------|------|--------|
| `h5cpp/compat.hpp` | `is_detected`, `detected_t`, `nonesuch` | **Keep** — not in C++17 standard (TS only). |
| `h5cpp/compat.hpp` (implied via `H5meta.hpp`) | `index_sequence`, `make_index_sequence`, `void_t` | **Redundant** — C++17 has `std::index_sequence` / `std::make_index_sequence` / `std::void_t`. |
| `h5cpp/H5meta.hpp:174–176` | Hand-rolled `h5::impl::conditional` | **Redundant** — `std::conditional` is in C++11. |
| `h5cpp/H5config.hpp` (per `docs/cpp-standard-compliance-report.md`) | `h5cpp__constexpr` / `h5cpp__assert` macros | **Redundant** — designed to degrade `if constexpr` to runtime `if` on C++14. |

---

## 4. Gaps and Inconsistencies

| # | Gap | Location | Risk | Severity |
|---|-----|----------|------|----------|
| 1 | **No C++17 feature-test macros** | Core headers | Consumer using C++14 gets 500-line template errors instead of `#error "C++17 required"` | Medium |
| 2 | **CI builds C++23, not C++17** | `.github/workflows/ci.yml` | C++20/23-isms could leak in unnoticed; library claims C++17 baseline but doesn't validate it broadly on GCC/Clang | Medium |
| 3 | **`static_assert` disabled on MSVC** | `H5Dgather.hpp` | Silent degradation of error quality on Windows | Low |
| 4 | **`std::is_pod` deprecated in C++20** (7 hits) | `H5Tmeta.hpp`, `H5Mstl.hpp`, `H5Aread.hpp`, `H5misc.hpp` | Will generate warnings on C++20+ builds | Low |
| 5 | **No `H5_HAVE_THREADSAFE` check** | `H5Eall.hpp` (`thread_local`) | Could crash with non-thread-safe HDF5 builds | Medium |
| 6 | **No compiler version blacklist** | CMake | Older MSVC/Clang that claim C++17 but have known bugs are not caught at configure time | Low |
| 7 | **Syntax bugs behind version macros** | `H5Pall.hpp:209`, `H5Pdcpl.hpp:19` | Missing `>` in template arg; `dapl` used instead of `dcpl` — only visible when the gated HDF5 version is hit | Low |

---

## 5. Bottom Line

h5cpp has a **solid HDF5 version-gating strategy** at both CMake and preprocessor levels, but its **C++ compatibility story is "trust me, it's C++17"** — no feature probes, no graceful degradation, and CI does not actually test the declared baseline across the matrix.

## Related examples

- [`examples/reference/reference.cpp`](../../../examples/reference/reference.cpp) — rule-of-five RAII against the H5R_ref_t lifecycle (1.12 reference path)
- [`examples/attributes/attributes.cpp`](../../../examples/attributes/attributes.cpp) — fixed-length vs VLEN string attribute handling
