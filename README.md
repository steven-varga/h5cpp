# H5CPP
[![CI](https://github.com/vargalabs/h5cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/vargalabs/h5cpp/actions/workflows/ci.yml)
[![ASan](https://vargalabs.github.io/h5cpp/badges/asan.svg)](https://github.com/vargalabs/h5cpp/actions/workflows/ci.yml)
[![UBSan](https://vargalabs.github.io/h5cpp/badges/ubsan.svg)](https://github.com/vargalabs/h5cpp/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/vargalabs/h5cpp/branch/release/graph/badge.svg)](https://app.codecov.io/gh/vargalabs/h5cpp/tree/release)
[![MIT License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.20123216.svg)](https://doi.org/10.5281/zenodo.20123216)
[![GitHub release](https://img.shields.io/github/v/release/vargalabs/h5cpp.svg)](https://github.com/vargalabs/h5cpp/releases)
[![Documentation](https://img.shields.io/badge/docs-stable-blue)](https://vargalabs.github.io/h5cpp)

Modern C++ for HDF5
--------------------
**H5CPP** is a modern C++ template library for serial and parallel HDF5 I/O. It provides type-safe RAII wrappers, high-level `create` / `read` / `write` / `append` operations, and seamless interoperability with the native HDF5 C API. Chunked and compressed datasets, extendable packet-table streams, hyperslab selection, custom datatypes, and MPI parallel I/O are all supported. HDF5 files written by H5CPP are readable from Python, R, MATLAB, Fortran, Julia, and any other HDF5-capable environment.

| Layer | Role |
|---|---|
| **H5CPP** | Header-only C++ HDF5 I/O library |
| [h5cpp-compiler][compiler] | Optional build-time reflection tool for non-POD struct persistence |

## Supported platforms

| OS / Compiler | GCC 13        | GCC 14        | GCC 15    | Clang 17     | Clang 18     | Clang 19     | Clang 20     | Apple Clang | MSVC         |
|---------------|---------------|---------------|-----------|--------------|--------------|--------------|--------------|-------------|--------------|
| Ubuntu 22.04  | ![gcc13][200] | ![NA][NA]     | ![NA][NA] | ![cl17][250] | ![cl18][251] | ![cl19][252] | ![cl20][253] | ![NA][NA]   | ![NA][NA]    |
| Ubuntu 24.04  | ![gcc13][300] | ![gcc14][301] | ![NA][NA] | ![NA][NA]    | ![cl18][351] | ![cl19][352] | ![cl20][353] | ![NA][NA]   | ![NA][NA]    |
| macOS 15      | ![NA][NA]     | ![NA][NA]     | ![NA][NA] | ![NA][NA]    | ![NA][NA]    | ![NA][NA]    | ![NA][NA]    | ![ac][400]  | ![NA][NA]    |
| Windows       | ![NA][NA]     | ![NA][NA]     | ![NA][NA] | ![NA][NA]    | ![NA][NA]    | ![NA][NA]    | ![NA][NA]    | ![NA][NA]   | ![msvc][500] |

## Quick Start

```cpp
#include <h5cpp/all>
#include <vector>

namespace sn::sensor {
	struct [[h5::doc("Time-series sensor reading with variable-length fields"),
			h5::chunk(128), h5::compress("gzip", 6)]] timeseries_t {
		unsigned long long timestamp_ns;
		[[h5::name("label")]] std::string tag;
		[[h5::ignore]] int internal_id;
		std::vector<double> readings;
	};
}


int main() {
    auto fd = h5::create("example.h5", H5F_ACC_TRUNC);

    auto fd = h5::create("storage.h5");
    std::vector<timeseries_t> samples(100);
    h5::write(fd, "samples", samples);

    auto result = h5::read<std::vector<timeseries_t>>(fd, "samples");
}
```

```cmake
find_package(HDF5 REQUIRED)
find_package(h5cpp REQUIRED)
target_link_libraries(my_app PRIVATE h5cpp::h5cpp)
```

## Requirements

| Requirement | Minimum | Tested ceiling |
|---|---|---|
| C++ standard | C++17 | C++23 |
| HDF5 | 1.10.x | 1.14.6 |
| CMake | 3.22 | — |

C++20 enables `h5::view<T>` streaming ranges.
C++23 adds `std::float16_t` dataset support.

## Installation

**From GitHub Releases** — pre-built packages for each tagged release:

| Platform | Package |
|---|---|
| Ubuntu / Debian (amd64, arm64) | `.deb` via [Releases][releases] |
| RHEL / Fedora (x86\_64, aarch64) | `.rpm` via [Releases][releases] |
| macOS 15 arm64 | `.pkg` via [Releases][releases] |
| Windows x64 | NSIS `.exe` via [Releases][releases] |

**From source:**

```bash
git clone https://github.com/vargalabs/h5cpp.git
cmake -B build -DCMAKE_BUILD_TYPE=Release -DH5CPP_BUILD_TESTS=OFF
cmake --install build
```

## Supported Types

| Category | Types |
|---|---|
| Numeric | `bool`, `int8_t`–`int64_t`, `uint8_t`–`uint64_t`, `float`, `double`, `long double`, `std::complex<T>`, `std::float16_t` (C++23) |
| Strings | `std::string`, `char[]`, variable-length HDF5 strings |
| STL sequences | `std::vector`, `std::valarray`, `std::array`, `std::deque` |
| STL node-based | `std::list`, `std::forward_list`, `std::set`, `std::multiset`, `std::unordered_set`, `std::unordered_multiset` |
| Linear algebra | Armadillo, Eigen, Blaze, Blitz++, Boost uBLAS, IT++, dlib |
| Structs | POD / C / C++ structs via [h5cpp-compiler][compiler] |
| Arrays | Up to rank 7 |

## v1.14.0

v1.14.0 closes the v1.12.x line — a cycle organized around eliminating the performance and concurrency objections that push teams from HDF5 toward ad-hoc formats.

- **STL non-contiguous and sequence containers** — `std::valarray`, `std::list`, `std::deque`, `std::set`, `std::multiset`, `std::unordered_set`, `std::unordered_multiset` all route through the unified Walter Brown trait-based dispatch. `std::complex<T>` and `std::float16_t` (C++23 half-precision) datasets added.
- **Rank-7 arrays** — up to seven-dimensional arrays supported, matching the HDF5 C library limit.
- **Expanded attribute coverage** — all scalar, string, and compound attribute types reachable through the same `h5::awrite` / `h5::aread` surface.
- **SSSE3 rank-1 write fast path** — chunked 1-D writes use a bump-pointer arena, prefetch, nontemporal stores, and SIMD shuffle/unshuffle for element sizes 2, 4, and 8 bytes. No API changes; existing code picks it up automatically on x86 targets. Non-x86 targets use the scalar path unchanged.
- **In-place filter pipeline** — shuffle, Fletcher-32, scale-offset, and nbit run without heap allocation, cutting per-chunk overhead on write-heavy workloads.
- **Transparent concurrent compression** — the filter pipeline (gzip/zstd) runs across a per-file worker pool internally; the write API stays synchronous and single-threaded. Activate at file open with `h5::create(..., h5::threads{N} | h5::backpressure{M})`; no THREAD_SAFE HDF5 build required.
- **Parallel decompression** — rank-1 chunked reads decompress across the same per-file pool, scaling with available threads.
- **`h5::view<T>` streaming ranges** — C++20 range view over chunked datasets; `for (auto chunk : h5::view<std::vector<float>>(ds))` iterates multi-GB datasets one chunk at a time without materialising the full dataset in memory. Any container satisfying Walter Brown's detection idiom (`data()`, `value_type`, `size()`) works as the element type, so Abseil, Folly, EASTL, and Boost.Container all participate without registration.
- **Gorilla XOR filter** *(experimental)* — delta-of-delta XOR codec for `float32`/`float64` time-series (Facebook Gorilla algorithm). Interoperability with h5py and the HDF5 C library for Gorilla-compressed datasets is not yet tested.
- **Scatter/gather dispatch** — `H5CPP_REGISTER_SCATTER` lets any third-party container opt into `h5::read` / `h5::write` without modifying the library.
- **Compiler-assisted reflection** — `h5cpp-compiler`, a Clang LibTooling pre-build tool, emits HDF5 compound descriptors and scatter/gather helpers for any non-POD struct without intrusive macros. Struct members are annotated with C++26-style attributes; the tool walks the AST at build time and generates the descriptors. C++26 static reflection is on the roadmap as the macro-free successor.
- **SWMR** — `swmr_write_t` / `swmr_read_t` tags enable live readers to observe an active writer without closing the file. Requires HDF5 ≥ 1.12.3; `H5CPP_HAS_SWMR` is set automatically by CMake.
- **HDF5 2.x compatibility** — `H5Dread_chunk2` buffer-sizing fix; existing code runs correctly on HDF5 2.x without source changes.
- **Doxygen API reference** — full reference live at [vargalabs.github.io/h5cpp][docs], with I/O API, topics, and cookbook navigation axes.
- **28 CMake cookbook examples** — basics through MPI, S3, sparse matrices, half-float, and custom pipelines; enable with `-DH5CPP_BUILD_EXAMPLES=ON`.
- **Public CDash dashboard** — CI results and sanitiser status at <https://my.cdash.org/index.php?project=h5cpp>; community submissions welcomed.
- **Coverage > 95%**, TSan-clean on Clang 20, ASan and UBSan clean across the full matrix.

## Documentation

Full API reference, examples, and architecture notes: [vargalabs.github.io/h5cpp][docs]

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for issue naming, branch conventions, commit format, and the pull request workflow.

## License

MIT — see [LICENSE](LICENSE).

[hdf5]:     https://www.hdfgroup.org/solutions/hdf5/
[docs]:     https://vargalabs.github.io/h5cpp
[compiler]: https://github.com/vargalabs/h5cpp-compiler
[releases]: https://github.com/vargalabs/h5cpp/releases

[NA]: https://vargalabs.github.io/h5cpp/badges/na.svg

<!-- Ubuntu 22.04 -->
[200]: https://vargalabs.github.io/h5cpp/badges/ubuntu-22.04-gcc-13.svg
[250]: https://vargalabs.github.io/h5cpp/badges/ubuntu-22.04-clang-17.svg
[251]: https://vargalabs.github.io/h5cpp/badges/ubuntu-22.04-clang-18.svg
[252]: https://vargalabs.github.io/h5cpp/badges/ubuntu-22.04-clang-19.svg
[253]: https://vargalabs.github.io/h5cpp/badges/ubuntu-22.04-clang-20.svg

<!-- Ubuntu 24.04 -->
[300]: https://vargalabs.github.io/h5cpp/badges/ubuntu-24.04-gcc-13.svg
[301]: https://vargalabs.github.io/h5cpp/badges/ubuntu-24.04-gcc-14.svg
[351]: https://vargalabs.github.io/h5cpp/badges/ubuntu-24.04-clang-18.svg
[352]: https://vargalabs.github.io/h5cpp/badges/ubuntu-24.04-clang-19.svg
[353]: https://vargalabs.github.io/h5cpp/badges/ubuntu-24.04-clang-20.svg

<!-- macOS 15 -->
[400]: https://vargalabs.github.io/h5cpp/badges/macos-15-apple-clang.svg

<!-- Windows -->
[500]: https://vargalabs.github.io/h5cpp/badges/windows-latest-msvc.svg
