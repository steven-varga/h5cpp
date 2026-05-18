[![CI](https://github.com/vargalabs/h5cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/vargalabs/h5cpp/actions/workflows/ci.yml)
[![ASan](https://vargalabs.github.io/h5cpp/badges/asan.svg)](https://github.com/vargalabs/h5cpp/actions/workflows/ci.yml)
[![UBSan](https://vargalabs.github.io/h5cpp/badges/ubsan.svg)](https://github.com/vargalabs/h5cpp/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/vargalabs/h5cpp/branch/release/graph/badge.svg)](https://app.codecov.io/gh/vargalabs/h5cpp/tree/release)
[![MIT License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.20123216.svg)](https://doi.org/10.5281/zenodo.20123216)
[![GitHub release](https://img.shields.io/github/v/release/vargalabs/h5cpp.svg)](https://github.com/vargalabs/h5cpp/releases)
[![Documentation](https://img.shields.io/badge/docs-stable-blue)](https://vargalabs.github.io/h5cpp)

# H5CPP — High-Performance [HDF5][hdf5] for Modern C++

| OS / Compiler | GCC 13        | GCC 14        | GCC 15    | Clang 17     | Clang 18     | Clang 19     | Clang 20     | Apple Clang | MSVC         |
|---------------|---------------|---------------|-----------|--------------|--------------|--------------|--------------|-------------|--------------|
| Ubuntu 22.04  | ![gcc13][200] | ![NA][NA]     | ![NA][NA] | ![cl17][250] | ![cl18][251] | ![cl19][252] | ![cl20][253] | ![NA][NA]   | ![NA][NA]    |
| Ubuntu 24.04  | ![gcc13][300] | ![gcc14][301] | ![NA][NA] | ![NA][NA]    | ![cl18][351] | ![cl19][352] | ![cl20][353] | ![NA][NA]   | ![NA][NA]    |
| macOS 15      | ![NA][NA]     | ![NA][NA]     | ![NA][NA] | ![NA][NA]    | ![NA][NA]    | ![NA][NA]    | ![NA][NA]    | ![ac][400]  | ![NA][NA]    |
| Windows       | ![NA][NA]     | ![NA][NA]     | ![NA][NA] | ![NA][NA]    | ![NA][NA]    | ![NA][NA]    | ![NA][NA]    | ![NA][NA]   | ![msvc][500] |

H5CPP is a modern C++ template library for serial and parallel HDF5 I/O. It provides type-safe RAII wrappers, high-level `create` / `read` / `write` / `append` operations, and seamless interoperability with the native HDF5 C API. Chunked and compressed datasets, extendable packet-table streams, hyperslab selection, custom datatypes, and MPI parallel I/O are all supported. HDF5 files written by H5CPP are readable from Python, R, MATLAB, Fortran, Julia, and any other HDF5-capable environment.

## Quick Start

```cpp
#include <h5cpp/all>
#include <vector>

int main() {
    auto fd = h5::create("example.h5", H5F_ACC_TRUNC);

    std::vector<float> data = {1.f, 2.f, 3.f, 4.f, 5.f};
    h5::write(fd, "sensor/readings", data);

    auto result = h5::read<std::vector<float>>(fd, "sensor/readings");
    // result == {1.f, 2.f, 3.f, 4.f, 5.f}
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
| HDF5 | 1.10.x | 1.12.2 |
| CMake | 3.22 | — |

C++20 upgrades the I/O pipeline to lock-free queues and enables `h5::view<T>` streaming ranges.
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

## v1.12 Highlights

- **`std::valarray<T>`** read/write support
- **Iterator containers** — `std::list`, `std::deque`, `std::set`, `std::unordered_set` and their multi- variants
- **`std::complex<T>`** datasets
- **`std::float16_t`** (C++23 IEEE 754 half-precision)
- **Rank-7** array support
- **Expanded attribute** type coverage
- **Threaded I/O pipeline** for filter chains
- **HDF5 1.12.2 ceiling** — tested and verified; `H5Dvlen_reclaim` / reference API compatibility
- **Windows MSVC** in the CI matrix
- **ASan + UBSan** clean on Clang 20

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
