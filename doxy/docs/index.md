@mainpage H5CPP — Modern C++ for HDF5

> **Read and write any STL type, linear-algebra container, registered
> POD, or `std::tuple` to HDF5 in one line of C++.**
> &nbsp; &nbsp; *No manual `H5Tcreate`, no offset arithmetic, no error-code dance.*

```cpp
#include <h5cpp/all>

h5::fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC);

h5::write(fd, "/grid/values", arma::Mat<double>(1024, 1024, arma::fill::randu));
h5::write(fd, "/labels",      std::vector<std::string>{"alpha","beta","gamma"});
h5::write(fd, "/metadata",    std::map<std::string,double>{{"version", 1.4}});

auto M = h5::read<arma::Mat<double>>(fd, "/grid/values");
auto v = h5::read<std::vector<std::string>>(fd, "/labels");
auto m = h5::read<std::map<std::string,double>>(fd, "/metadata");
```

RAII handles close on scope exit. The on-disk format is canonical HDF5 —
readable by Python, R, MATLAB, Julia, Fortran, or any other
HDF5-capable tool — and for sparse matrices it's byte-compatible with
`scipy.sparse.csc_matrix`, Julia `HDF5.jl`, and the 10x Genomics /
Loompy convention out of the box.

---

# Why H5CPP exists

H5CPP began from a practical requirement: **efficient storage for
large numerical datasets with both indexed block access and sequential
streaming**. Existing serialisation systems handled streams reasonably
well, but not the kind of partial, multidimensional, file-backed access
needed in numerical computing and financial engineering. HDF5 already
had most of the right storage primitives — partial I/O, extendable
datasets, compression, broad interoperability across operating systems
and scientific environments. **What was missing was a modern C++
interface.**

|  | Direct HDF5 in C/C++ | H5CPP |
| :-- | :-- | :-- |
| **Datatype + dataspace** | Manual `H5Tcreate` / `H5Screate_simple` per type | `h5::write(fd, "x", value)` — type deduced from `T` |
| **Shape bookkeeping** | Hand-tracked `hsize_t[]` arrays | Derived from the value's `access_traits_t::size` |
| **Resource lifetime** | Paired `H5Fopen` / `H5Fclose`, error-prone on early return | `h5::fd_t` — RAII close on scope exit |
| **Container interop** | Memcpy into intermediate buffers | Direct on `std::vector`, `arma::Mat`, `Eigen::SparseMatrix`, … |
| **Error handling** | Negative return codes + global error stack | 48-leaf typed exception hierarchy with HDF5 stack folded in |
| **Parallel I/O** | Separate FAPL setup; raw MPI driver calls | `h5::fapl{ h5::driver::mpi{comm, info} }` — same call shape |
| **User structs** | Hand-written `H5T_COMPOUND` per field | `H5CPP_REGISTER_STRUCT(T)` (POD) or h5cpp-compiler (non-POD); C++26 reflection on the roadmap |

---

# Where to start

This documentation site is organised along three axes plus a project
scorecard. Pick the one that matches what you're trying to do:

| Axis | Answers | Start here |
| :--- | :------ | :--------- |
| **@ref curated_io_api "I/O"** | "I have a `FILE` / `DATASET` / `ATTRIBUTE` / `GROUP` — what API do I call?" | API reference organised by HDF5 object kind |
| **@ref curated_topics "TOPICS"** | "What does h5cpp *support*, and how does it work under the hood?" | Capability reference: properties, linalg, STL, reflection, filters, MPI, error handling, type system, CDash, reports |
| **@ref examples_guides_index "COOKBOOK"** | "Just show me a runnable example." | 28 worked examples — basics, datasets, attributes, groups, compound, linalg, sparse, mdspan, transform, optimised, packet table, custom pipeline, references, smart pointers, reflection, multi-TU, half-float, CSV, S3, MPI, … |
| **@ref reports_usability_evaluation "USABILITY"** | "What's actually supported in v1.14.0?" | Per-category feature scorecard (✔ / ◇ / ✘) — core I/O, type coverage, linalg, advanced HDF5, parallel & async, sparse, error handling, STL, type system, reflection, build & CI |

---

# What you can do

A few of the differentiators worth knowing about up front:

| Capability | One-liner | Reference |
| :--------- | :-------- | :-------- |
| **Linear algebra** | Armadillo, Eigen, Blaze, Blitz++, dlib, IT++, Boost uBLAS, xtensor — same `h5::write` / `h5::read` on every library | @ref curated_topics_linalg |
| **STL** (sequence + associative + ragged) | `std::vector`, `std::map`, `std::list`, `std::vector<std::vector<T>>`, `std::vector<std::string>` — all routed via the unified dispatch | @ref curated_topics_stl |
| **Sparse matrices** | `arma::SpMat` and `Eigen::SparseMatrix<T, ColMajor>` round-trip through canonical CSC group layout (scipy / Julia / 10x / Loompy interop) | @ref curated_topics_linalg |
| **Streaming I/O (C++20 ranges)** | `for (auto v : h5::view<float>(ds)) …` — iterate multi-GB datasets one chunk at a time, no materialisation | @ref curated_io_api_dataset_view |
| **Compiler-assisted reflection** | Pre-build Clang tool emits compound descriptors + scatter/gather for *any* non-POD struct — no intrusive macros. C++26 reflection on the roadmap. | @ref curated_topics_reflection |
| **Walter Brown feature detection** | The dispatch asks structural questions about `T` (does it have `data()`? `value_type`? `tuple_size`?) — third-party container libraries (Abseil, Folly, EASTL, Boost.Container) work via the same path | @ref curated_topics_stl |
| **Filters** | gzip, shuffle, fletcher32, nbit, **Gorilla** (Facebook time-series codec), custom plugins — composed via the `\|` operator | @ref curated_topics_filters |
| **Parallel I/O** | MPI-IO collective and independent transfer; works without a parallel filesystem (laptop testing, NFS-backed cluster) | @ref curated_topics_mpi |
| **Async mode** | `h5::async::create` returns type-level-discriminated descriptors — `operator ::hid_t() = delete`'d so raw-CAPI escape fails at compile time | @ref handle_ref_async |
| **Context-sensitive exceptions** | 48 typed leaves — `catch(h5::error::io::attribute::open&)` distinguishes from `catch(h5::error::io::dataset::read&)` | @ref curated_topics_error |

---

# Install

Pre-built packages for **v1.14.0**, one row per platform — the
**H5CPP** column is the header-only library (required); the
**COMPILER** column is the optional Clang LibTooling-based reflection
toolchain (needed only for non-POD struct persistence — see
@ref curated_topics_reflection).

| Platform | Arch | Format | H5CPP (lib) | COMPILER (optional) |
| :------- | :--- | :----- | :---------- | :------------------ |
| **Ubuntu / Debian** | x86_64 / amd64 | `.deb` | [download][100] (22 MB) | [download][200] (17 MB) |
| **Ubuntu / Debian** | aarch64 / arm64 | `.deb` | [download][101] (22 MB) | [download][201] (17 MB) |
| **RHEL / Fedora / openSUSE** | x86_64 | `.rpm` | [download][102] (22 MB) | [download][202] (17 MB) |
| **RHEL / Fedora / openSUSE** | aarch64 | `.rpm` | [download][103] (22 MB) | [download][203] (17 MB) |
| **Linux** (musl, distro-agnostic) | x86_64 | `.tar.gz` | — | [download][204] (33 MB) |
| **macOS** | Apple Silicon (arm64) | `.pkg` | [download][104] (23 MB) | [download][205] (2 KB ⚠) |
| **Windows** | x64 | `.exe` | [download][105] (25 MB) | [download][206] (9 MB) |

> ⚠ The macOS COMPILER `.pkg` is currently a 2 KB stub — full
> macOS package in the works. Build from source via the
> [h5cpp-compiler repo][src-compiler] if you need it on Apple
> Silicon today.

SHA-256 checksums on each release page:
[h5cpp v1.14.0][rel-h5cpp] · [h5cpp-compiler v1.12.6][rel-compiler]
&middot; browse [all h5cpp releases][all-h5cpp] /
[all compiler releases][all-compiler] for older versions and
pre-release builds.

```bash
# H5CPP — install once downloaded:
sudo apt install ./h5cpp-dev-v1.14.0-linux-x86_64.deb        # Ubuntu / Debian
sudo dnf install ./h5cpp-dev-v1.14.0-linux-x86_64.rpm        # RHEL / Fedora
sudo installer -pkg h5cpp-dev-v1.14.0-darwin-arm64.pkg \
                -target /                                      # macOS arm64
# Windows: double-click the .exe — NSIS installer with Start menu entries

# COMPILER — same commands, h5cpp-compiler-1.12.6-* package name.
```

## From source

```bash
git clone https://github.com/vargalabs/h5cpp.git && cd h5cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --install build
```

## Use from your CMake project

```cmake
find_package(h5cpp REQUIRED)
target_link_libraries(my_app PRIVATE h5cpp::h5cpp)
```

**Write your first program** — see the snippet at the top of this page.

---

# Project status

CI matrix, ASan / UBSan / TSan, code coverage, and the public CDash
dashboard:

[![CI](https://github.com/vargalabs/h5cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/vargalabs/h5cpp/actions/workflows/ci.yml)
[![ASan](badges/asan.svg)](https://github.com/vargalabs/h5cpp/actions/workflows/ci.yml)
[![UBSan](badges/ubsan.svg)](https://github.com/vargalabs/h5cpp/actions/workflows/ci.yml)
[![TSan](badges/tsan.svg)](https://github.com/vargalabs/h5cpp/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/vargalabs/h5cpp/branch/release/graph/badge.svg)](https://app.codecov.io/gh/vargalabs/h5cpp/tree/release)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.20123216.svg)](https://doi.org/10.5281/zenodo.20123216)

| OS / Compiler | GCC 13 | GCC 14 | Clang 17–20 | Apple Clang | MSVC |
| :------------ | :----: | :----: | :---------: | :---------: | :--: |
| Ubuntu 22.04 / 24.04 | ✔ | ✔ | ✔ | ∅ | ∅ |
| macOS 15 arm64 | ∅ | ∅ | ∅ | ✔ | ∅ |
| Windows | ∅ | ∅ | ∅ | ∅ | ✔ |

- **Live test dashboard** → <https://my.cdash.org/index.php?project=h5cpp>
- **Coverage** → <https://app.codecov.io/gh/vargalabs/h5cpp/tree/release>
- **HDF5 version matrix** — tested against 1.10.x / 1.12.x / 1.14.x
- **C++ standard** — C++20 default, C++23 features used (`std::mdspan`);
  C++26 reflection on the roadmap

---

# What's in v1.14.0

The @ref reports_usability_evaluation "v1.14.0 usability scorecard"
has the full picture — highlights since the earlier evaluation:

- **STL non-contiguous unblocked** — `std::map`, `std::list`,
  `std::set`, ragged `vector<vector<T>>` all routed through the
  unified Walter Brown trait-based dispatch
- **Eigen sparse landed** — joins Armadillo on the canonical CSC
  group layout
- **Async mode (Phase II)** — separate `h5::async::*` descriptor
  types with `operator ::hid_t() = delete` so the type system
  prevents raw-CAPI escape paths
- **Streaming `h5::view<T>`** — C++20 ranges view over rank-1
  chunked datasets, decompressed through the filter pipeline
- **h5cpp-compiler** multi-backend roadmap — one AST walker, many
  artifacts (HDF5 + Protobuf + JSON Schema + SQL + Avro + CBOR +
  BSON + MessagePack + RLP)
- **macOS packaging** — signed `.pkg` for Apple Silicon
- **Public CDash dashboard** — community submissions welcomed
- **Documentation overhaul** — this site, with three parallel nav
  axes and 28 cookbook examples

---

# Reading by background

| Coming from… | Start at | Why |
| :----------- | :------- | :-- |
| **HDF5 CAPI** | @ref curated_io_api | One-to-one mapping from HDF5 verbs (`H5Fcreate`, `H5Dwrite`, `H5Aopen`) to h5cpp's typed equivalents |
| **h5py** (Python) | @ref curated_io_api | Side-by-side syntax comparison |
| **Boost.Serialization / Cereal / nlohmann::json** | @ref curated_topics_reflection | How h5cpp's compiler-assisted reflection avoids intrusive macros |
| **HighFive** | the `bench/` suite | Methodology used to compare h5cpp against HighFive and raw CAPI |
| **A new C++ project** | The quickstart above + @ref example_guide_basics | 60 seconds to a working `h5::write` / `h5::read` round-trip |
| **A linear-algebra-heavy codebase** | @ref curated_topics_linalg | Per-library mapper headers and storage-order rules |
| **An LLM-tooling pipeline** (RPC schemas, JSON Schema) | @ref reports_compiler_multi_backend_architecture | One struct → HDF5 + Protobuf + JSON Schema + SQL + Avro from a single source of truth |

---

# Acknowledgements

H5CPP began as a small collection of templates. Early contact with The HDF Group helped shape the first H5CPP11 project, and later informed the C++17 version. I am especially grateful to Gerd Heber for his sustained guidance, generosity, and friendship over the years; to Elena Pourmal and David Pareah for their encouragement, support on the project’s direction; and to Mark Paterno and Chris at Fermilab for their thoughtful input. Many of H5CPP’s stronger ideas were sharpened through those discussions. Any mistakes, omissions, or rough edges remain entirely my own.

H5CPP [has been presented](https://steven-varga.ca/site/talks/) in HDF5 and C++ community venues over multiple years, including HUG sessions, HDF Group events, C++ community talks, and ISC-related
material. Topics covered include compiler-assisted reflection, POD introspection, MPI/parallel I/O, throughput/latency trade-offs, and practical HDF5 workflows.

---

# Project info

| | |
| :-- | :-- |
| **Source** | <https://github.com/vargalabs/h5cpp> |
| **h5cpp-compiler** | <https://github.com/vargalabs/h5cpp-compiler> |
| **License** | MIT |
| **Citation** | [Zenodo DOI 10.5281/zenodo.20123216](https://doi.org/10.5281/zenodo.20123216) |
| **Contact** | Steven Varga &middot; <steven@vargalabs.com> &middot; <https://steven-varga.ca/> |

---

<small>Documentation built with Doxygen + the
[doxygen-awesome-css](https://github.com/jothepro/doxygen-awesome-css)
theme.</small>

<!-- ----------------------------------------------------------------------
     Download URL definitions — kept at the bottom to keep the install
     table readable. Reference convention:
       100-105  → h5cpp library      (release: vargalabs/h5cpp v1.14.0)
       200-206  → h5cpp-compiler     (release: vargalabs/h5cpp-compiler v1.12.6)
       rel-*    → per-release pages with SHA-256 checksums
       all-*    → "all releases" landing pages
       src-*    → source repository roots
     The two products version independently: bump each block to its own
     latest release tag (library is on v1.14.0; h5cpp-compiler trails at
     v1.12.6 until its next release). The table above embeds no URLs directly.
     ---------------------------------------------------------------------- -->

<!-- h5cpp library (header-only) — v1.14.0 -->
[100]: https://github.com/vargalabs/h5cpp/releases/download/v1.14.0/h5cpp-dev-v1.14.0-linux-x86_64.deb
[101]: https://github.com/vargalabs/h5cpp/releases/download/v1.14.0/h5cpp-dev-v1.14.0-linux-aarch64.deb
[102]: https://github.com/vargalabs/h5cpp/releases/download/v1.14.0/h5cpp-dev-v1.14.0-linux-x86_64.rpm
[103]: https://github.com/vargalabs/h5cpp/releases/download/v1.14.0/h5cpp-dev-v1.14.0-linux-aarch64.rpm
[104]: https://github.com/vargalabs/h5cpp/releases/download/v1.14.0/h5cpp-dev-v1.14.0-darwin-arm64.pkg
[105]: https://github.com/vargalabs/h5cpp/releases/download/v1.14.0/h5cpp-dev-v1.14.0-windows-amd64.exe

<!-- h5cpp-compiler (Clang LibTooling reflection toolchain) — v1.12.6 -->
[200]: https://github.com/vargalabs/h5cpp-compiler/releases/download/v1.12.6/h5cpp-compiler-1.12.6-Linux-amd64.deb
[201]: https://github.com/vargalabs/h5cpp-compiler/releases/download/v1.12.6/h5cpp-compiler-1.12.6-Linux-arm64.deb
[202]: https://github.com/vargalabs/h5cpp-compiler/releases/download/v1.12.6/h5cpp-compiler-1.12.6-Linux-x86_64.rpm
[203]: https://github.com/vargalabs/h5cpp-compiler/releases/download/v1.12.6/h5cpp-compiler-1.12.6-Linux-aarch64.rpm
[204]: https://github.com/vargalabs/h5cpp-compiler/releases/download/v1.12.6/h5cpp-compiler-1.12.6-Linux-musl-x86_64.tar.gz
[205]: https://github.com/vargalabs/h5cpp-compiler/releases/download/v1.12.6/h5cpp-compiler-1.12.6-Darwin.pkg
[206]: https://github.com/vargalabs/h5cpp-compiler/releases/download/v1.12.6/h5cpp-compiler-1.12.6-win64.exe

<!-- Release pages (SHA-256 checksums + release notes) -->
[rel-h5cpp]:    https://github.com/vargalabs/h5cpp/releases/tag/v1.14.0
[rel-compiler]: https://github.com/vargalabs/h5cpp-compiler/releases/tag/v1.14.0

<!-- "All releases" landing pages -->
[all-h5cpp]:    https://github.com/vargalabs/h5cpp/releases
[all-compiler]: https://github.com/vargalabs/h5cpp-compiler/releases

<!-- Source repositories -->
[src-h5cpp]:    https://github.com/vargalabs/h5cpp
[src-compiler]: https://github.com/vargalabs/h5cpp-compiler
