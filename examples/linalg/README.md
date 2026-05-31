@page example_guide_linalg Linear-Algebra Containers

This directory shows how h5cpp talks to the major C++ linear-algebra libraries. The point is simple: write your matrix or tensor with the same `h5::write` / `h5::read` you'd use for `std::vector` — h5cpp picks the right path for each library's memory layout.

```cpp
arma::mat M(2, 3);
h5::write(fd, "/path", M);          // same call, any library
auto back = h5::read<arma::mat>(fd, "/path");
```

That's the whole interface. The mapper headers (`H5Marma.hpp`, `H5Meigen.hpp`, etc.) carry the per-library access traits — h5cpp knows how to find the raw pointer behind each library's matrix wrapper, what its element type is, and what its row/column layout is.

## Files

| File                | Library                    | Backing types in the mapper                                     |
| ------------------- | -------------------------- | --------------------------------------------------------------  |
| `arma.cpp`          | Armadillo                  | `arma::Row<T>`, `arma::Col<T>`, `arma::Mat<T>`, `arma::Cube<T>` |
| `eigen3.cpp`        | Eigen3                     | `Eigen::Matrix<T, Dynamic, Dynamic, ...>`, `Eigen::Array<T,...>`|
| `blaze.cpp`         | Blaze                      | `blaze::DynamicMatrix<T,...>`, `blaze::DynamicVector<T,...>`    |
| `blitz.cpp`         | Blitz++                    | `blitz::Array<T, N>` (any rank N)                               |
| `dlib.cpp`          | Dlib                       | `dlib::matrix<T>`                                               |
| `ublas.cpp`         | Boost.uBLAS                | `boost::numeric::ublas::{matrix,vector}<T>`                     |
| `valarray.cpp`      | `std::valarray`            | `std::valarray<T>`                                              |
| `xtensor.cpp`       | xtensor                    | `xt::xtensor<T, N>`, `xt::xarray<T>`                            |
| `xtensor-blas.cpp`  | xtensor-blas               | xtensor + BLAS-backed linalg ops                                |
| `itpp.cpp`          | IT++                       | `itpp::Mat<T>`, `itpp::Vec<T>` *(disabled — not C++17-clean)*   |

## Build State (as of HEAD)

Each `✔ ok` line below corresponds to a runtime check inside the example itself: the matrix is written, read back, and compared element-by-element. The example prints `✔ ok` for a clean round-trip or `✘ failed` if the readback disagrees with the original. No more "shape preserved" prose — the example fails its own check if the round-trip doesn't match.

| Target                  | CMake target                  | Verified shapes                                                      |
| ----------------------- | ----------------------------- | -------------------------------------------------------------------- |
| `examples-arma`         | wired                         | ✔ ok — `vec(8)`, `mat(3x4)`, `cube(2x3x4)`                            |
| `examples-eigen3`       | wired                         | ✔ ok — `Matrix(3x4)`, `Array(2x5)`, column-vec `(8x1)`                |
| `examples-blaze`        | wired                         | ✔ ok — `DynamicVector(8)`, `DynamicMatrix(3x4)`                       |
| `examples-blitz`        | wired                         | ✔ ok — 1D `(8)`, 2D `(3x4)`, 3D `(2x3x4)`                              |
| `examples-dlib`         | wired                         | ✔ ok — `matrix(3x4)`, `matrix(5x1)`                                   |
| `examples-ublas`        | wired (gated on Boost.config) | ✔ ok — `vector(8)`, `matrix(3x4)`                                     |
| `examples-valarray`     | wired                         | ✔ ok — `valarray(10)`, chunk+gzip `valarray(20)`                      |
| `examples-xtensor`      | wired                         | ✘ failed — blocked on `traits::size` cross-tag conversion (see below) |
| `examples-xtensor-blas` | wired (gated on BLAS)         | ✘ failed — same root cause as xtensor                                  |
| `examples-itpp`         | **not wired**                 | ◇ cancelled — ITPP v4.3.1 isn't C++17-clean; CMake block commented out |

Sample output (`./examples-blaze`):

```
✔ ok    blaze::DynamicVector<double>(8)    shape + values
✔ ok    blaze::DynamicMatrix<short>(3x4)   shape + values
```

### The xtensor blocker

`xt::xarray<T>` has dynamic rank, so its `traits::size(ref)` can't return `std::array<size_t, N>` (the rank-known-at-compile-time shape every other mapper uses). The current xtensor mapper returns `h5::current_dims_t`, but the write dispatch at `H5Dwrite.hpp:647` does `h5::count_t count = traits::size(ref);` — and `h5::impl::array<TagX>` types don't implicitly convert across tags. The narrowly-symmetric problem appears at `H5Awrite.hpp:111` where `current_dims_t` is what's expected.

The proper fix unifies the cross-tag conversion across `count_t` / `current_dims_t` / `max_dims_t` / etc. (they share the same struct shape, only the tag differs). Two attempted fixes broke other paths — full unification needs a focused architectural pass with the dispatch code, not a per-example tweak. Pending that, the static-rank `xt::xtensor<T, N>` half of the example works (it uses `std::array<size_t, N>` like the others); only `xt::xarray<T>` is blocked.

## The Pattern (Same Across Every Library)

```cpp
#include <armadillo>          // your linalg library
#include <h5cpp/all>

int main() {
    auto fd = h5::create("file.h5", H5F_ACC_TRUNC);

    arma::mat M(rows, cols);
    h5::write(fd, "/path", M);                      // write

    auto back = h5::read<arma::mat>(fd, "/path");   // read into the same type
}
```

Swap `arma::mat` for `Eigen::Matrix<T,...>` / `blaze::DynamicMatrix<T,...>` / `xt::xtensor<T,2>` / etc. — the call shape doesn't change. The mapper header for each library is auto-included by `h5cpp/core` (which `<h5cpp/all>` pulls in), gated on whether the library's own header was seen earlier in the TU.

## Library × Shape Support Matrix

| Library      | Vector | Matrix | Cube/3-tensor | N-tensor |
| ------------ | ------ | ------ | ------------- | -------- |
| Armadillo    | ✔ Row/Col | ✔ Mat | ✔ Cube         | —        |
| Eigen3       | ✔     | ✔     | —             | —        |
| Blaze        | ✔     | ✔     | —             | —        |
| Blitz++      | ✔     | ✔     | ✔            | ✔ via `Array<T,N>` |
| Dlib         | —     | ✔     | —             | —        |
| Boost.uBLAS  | ✔     | ✔     | —             | —        |
| `std::valarray` | ✔  | —     | —             | —        |
| xtensor      | ✔ xarray | ✔ xtensor<T,2> | ✔ xtensor<T,3> | ✔ xtensor<T,N> |

The empty cells aren't h5cpp limitations — they're the library's own coverage. Most libraries are matrix-first and add rank-3+ as an afterthought.

## Hyperslab + Partial I/O

Several of these examples include a `h5::write(file, path, V, h5::current_dims{40,50}, h5::offset{5,0}, h5::count{1,1}, h5::stride{3,5}, h5::block{2,4}, h5::max_dims{40, H5S_UNLIMITED})` line. That's a hyperslab selection — placing a small linalg object inside a larger dataset.

The same `offset` / `count` / `stride` / `block` vocabulary is documented in detail under `examples/datasets/` section 7. The linalg examples reproduce it without prose; treat it as "this works for linalg containers too", not as the primary teaching point.

## Which Library Should I Use?

A short guide:

- **Armadillo** if you want a MATLAB-flavoured C++ API and don't mind a compiled dependency (LAPACK).
- **Eigen3** if you want header-only, template-expression-heavy, fast-compile-against-large-codebases. Most popular C++ linalg library today.
- **Blaze** if you want aggressive SIMD/multi-threading and don't mind heavy template metaprogramming. Compiles slowly.
- **Blitz++** if you want arbitrary-rank arrays and you're already in a scientific-computing codebase that uses it.
- **Boost.uBLAS** if you're already in a Boost-heavy codebase.
- **`std::valarray`** if you want zero external dependencies and only need vectors.
- **xtensor** if you want NumPy-flavoured tensor expressions with lazy evaluation.
- **xtensor-blas** if you want xtensor with BLAS-backed linalg primitives.
- **Dlib** if you're already using Dlib's image/ML stack.
- **IT++** — not currently supported; example exists for reference but is disabled.

h5cpp's stance: it doesn't care which one you pick. Every supported library round-trips through `h5::write` / `h5::read` with the same call shape.

## Output

Most variants in this directory write the round-trip but don't print anything — by design, since the libraries differ in what their "print" looks like. To verify a write, inspect the file with `h5dump`:

```sh
cd <build-dir>
./examples-arma
h5dump -pH arma.h5
```

The exception is `valarray.cpp`, which prints the readback values.

## Cross-References

- **`examples/datasets/`** — the hyperslab `offset`/`count`/`stride`/`block` vocabulary with prose
- **`examples/attributes/`** — single TU that exercises every supported linalg library at once (compile-time proof that they coexist; the `BLAS_H` define escape hatch handles the Armadillo + Blaze LAPACK conflict)
- **`examples/container/`** — the same dispatch surface for STL and custom containers
- **`h5cpp/H5M*.hpp`** — per-library mapper headers; these are where new linalg support gets added

## Library Bugs Fixed in This Pass

In the course of getting these examples to actually run, several real h5cpp bugs surfaced and were fixed. Adding the round-trip + element-equality verification flushed out two more (the blaze shape order + ctor swap, and the missing non-const `access_traits_t::data` overloads).

| File | Fix |
| --- | --- |
| `h5cpp/H5Marma.hpp` | `get<arma::cube<T>>::ctor` returned a `colmat` (a Mat!) instead of a `cube`; corrected return type + dim mapping |
| `h5cpp/H5Mblaze.hpp` | Added missing `access_traits_t` specs for `rowvec` / `colvec` — they had `is_contiguous` and `has_explicit_access_traits` markers but no `access_traits_t`, leaving DynamicVector with no `element_t` |
| `h5cpp/H5Mvalarray.hpp` | Added `has_explicit_storage_repr` + `storage_representation_impl` → `linear_value_dataset`; without these `std::valarray<T>` resolved to `unsupported` |
| `h5cpp/H5Meigen.hpp` | `get<ColMajor>::ctor` dim-swap removed — matches the rationalized `access_traits_t::size = {rows, cols}` convention used by every other mapper |
| `h5cpp/H5Marma.hpp`, `H5Mblaze.hpp`, `H5Mblitz.hpp`, `H5Mdlib.hpp`, `H5Mublas.hpp`, `H5Mvalarray.hpp` | `access_traits_t::data` was missing the non-const overload, so non-const reads dispatched through `h5::impl::data(const T&)` and returned `const T*` — `H5Dread.hpp:225` then failed with `cannot convert 'const T*' to 'T*'`. Added `static auto data(T& c) noexcept` to every contiguous-mapper spec. Eigen and xtensor were already correct. Surfaced by `examples/optimized`, which does an in-place `h5::read(ds, M, offset)`. |
| `h5cpp/H5Mblaze.hpp` + `thirdparty/blaze/CMakeLists.txt` | `size(rowmat)` returned `{columns, rows}` instead of `{rows, columns}`; the row-major `get`/`ctor` mapper matched it with a swap. Both corrected to the rationalized `{rows, columns}` convention. Additionally, Blaze's default SIMD row padding made `data()` non-contiguous: a `(3 × 4)` matrix of `short` had row spacing 8, so the first 12 elements read out of `data()` interleaved zeros between the rows. Setting `BLAZE_USE_PADDING=0` on the `libblaze` interface target disables that padding so `data()` is honestly packed. |

## Known Remaining Issues

1. **`xt::xarray<T>` (dynamic rank) read/write blocked** on the `traits::size` cross-tag conversion question. See "The xtensor blocker" above. The static-rank `xt::xtensor<T, N>` works fine; only the dynamic-rank `xt::xarray<T>` path is blocked.
2. **`itpp.cpp` is orphaned.** Disabled in CMake (`examples/CMakeLists.txt:202-205`) because ITPP v4.3.1 isn't C++17-clean. File still ships for reference.
3. **Some mapper conventions still differ on shape order.** Dlib + uBLAS use a `{cols, rows}` legacy convention in their `impl::size`; arma + eigen + blitz + blaze use `{rows, cols}`. Explicit `h5::chunk{r, c}` only works when its order matches the mapper's. The linalg examples sidestep this by not passing chunk specs.
4. **Code duplication across files.** All seven working files share ~70% identical scaffolding. A future cleanup could trim each to ~15 lines and have a single "universal pattern" code block in this README. Not actively harmful.

## Source

- [`arma.cpp`](arma_8cpp-example.html) — rendered with syntax highlighting
- [`blaze.cpp`](blaze_8cpp-example.html) — rendered with syntax highlighting
- [`blitz.cpp`](blitz_8cpp-example.html) — rendered with syntax highlighting
- [`dlib.cpp`](dlib_8cpp-example.html) — rendered with syntax highlighting
- [`eigen3.cpp`](eigen3_8cpp-example.html) — rendered with syntax highlighting
- [`itpp.cpp`](itpp_8cpp-example.html) — rendered with syntax highlighting
- [`ublas.cpp`](ublas_8cpp-example.html) — rendered with syntax highlighting
- [`valarray.cpp`](valarray_8cpp-example.html) — rendered with syntax highlighting
- [`xtensor-blas.cpp`](xtensor_blas_8cpp-example.html) — rendered with syntax highlighting
- [`xtensor.cpp`](xtensor_8cpp-example.html) — rendered with syntax highlighting
