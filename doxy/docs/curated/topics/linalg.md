@page curated_topics_linalg LINEAR ALGEBRA

@brief Dense + sparse linear-algebra container support — Armadillo,
Eigen, Blaze, Blitz++, dlib, IT++, Boost uBLAS, xtensor. Eight
upstream libraries, one dispatch.

# Linear algebra integration

H5CPP recognises linear-algebra containers from eight C++ libraries as
first-class template arguments to `h5::read`, `h5::write`, and
`h5::create`. Each library is **opted in** by including a single mapper
header — until that include happens, the dispatch leaves the library's
types out of `storage_representation_v<T>` and a call site fails at
compile time with a clear diagnostic.

```cpp
#include <armadillo>
#include <h5cpp/H5Marma.hpp>      // opt in to arma::* support

arma::Mat<double> M(1000, 1000, arma::fill::randu);
h5::fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC);
h5::write(fd, "/grid/values", M);                          // one call
auto M2 = h5::read<arma::Mat<double>>(fd, "/grid/values"); // round-trip
```

## Supported libraries

| Library      | Mapper header              | Dense vectors / matrices                                         | Sparse                                       |
| :----------- | :------------------------- | :--------------------------------------------------------------- | :------------------------------------------- |
| Armadillo    | `h5cpp/H5Marma.hpp`        | `arma::Row<T>`, `Col<T>`, `Mat<T>`, `Cube<T>`                    | `arma::SpMat`, `SpRow`, `SpCol`              |
| Eigen        | `h5cpp/H5Meigen.hpp`       | `Eigen::Matrix<T,R,C,O>`, `Eigen::Array<T,R,C,O>`                | `Eigen::SparseMatrix`, `SparseVector` (CSC)  |
| Blitz++      | `h5cpp/H5Mblitz.hpp`       | `blitz::Array<T,N>` (rank-N, up to 11)                           | —                                            |
| Blaze        | `h5cpp/H5Mblaze.hpp`       | `blaze::DynamicVector<T,O>`, `DynamicMatrix<T,O>` (both orientations) | —                                       |
| dlib         | `h5cpp/H5Mdlib.hpp`        | `dlib::matrix<T,0,0,...,row_major_layout>`                       | —                                            |
| IT++         | `h5cpp/H5Mitpp.hpp`        | `itpp::Vec<T>`, `Mat<T>`                                         | —                                            |
| Boost uBLAS  | `h5cpp/H5Mublas.hpp`       | `boost::numeric::ublas::vector<T>`, `matrix<T>`                  | —                                            |
| xtensor      | `h5cpp/H5Mxtensor.hpp`     | `xt::xarray<T>` (dynamic rank), `xt::xtensor<T,N>` (static rank) | —                                            |

`std::valarray` (mapper at `h5cpp/H5Mvalarray.hpp`) follows the same
opt-in pattern but lives under @ref curated_topics_stl since it's
shipped with the standard library.

## Dense storage layout — what hits the disk

Every dense container is a **contiguous buffer of `T`** with one
pointer (`memptr()`, `data()`, `valuePtr()`, …) into a single heap
allocation. h5cpp passes that pointer straight to
`H5Dwrite`/`H5Dread` with no copy and no transpose, so the on-disk
layout matches the upstream container's in-memory layout
byte-for-byte.

- **Vectors** (rank-1) are layout-orientation invariant. Round-trip
  between `arma::Row`, `Eigen::VectorXd`, `blaze::DynamicVector`,
  `ublas::vector`, etc. on the same dataset is byte-exact.
- **Matrices** (rank-2) inherit the container's storage order:
  - **Column-major on disk**: `arma::Mat`, `Eigen::Matrix<…,ColMajor>`
    (default), `blaze::DynamicMatrix<…,columnMajor>`.
  - **Row-major on disk**: `dlib::matrix`, `Eigen::Matrix<…,RowMajor>`,
    `blaze::DynamicMatrix<…,rowMajor>`, `ublas::matrix` (default),
    `itpp::Mat`.

  Reading a column-major dataset into a row-major container does
  not transpose — it reinterprets, producing the mathematical
  transpose. **Cross-library reads must match orientation**.
- **Tensors / N-d arrays** (`xt::xarray`, `xt::xtensor`, `blitz::Array`,
  `arma::Cube`) follow the upstream's default storage order.

All dense linalg containers resolve to
`storage_representation_t::linear_value_dataset` — the same path
`std::vector<T>` takes.

## Sparse — canonical CSC group layout

`arma::SpMat` / `SpRow` / `SpCol` and `Eigen::SparseMatrix<T,ColMajor>`
land as an HDF5 **group** containing four datasets and two scalar
attributes:

```text
group/
    data    : 1-D, dtype = T,        length nnz
    indices : 1-D, dtype = uint32,   row indices, length nnz
    indptr  : 1-D, dtype = uint32,   column pointers, length n_cols+1
    shape   : 1-D, dtype = uint64,   [n_rows, n_cols]
  @format = "csc"
  @axis   = "column"
```

Byte-compatible with `scipy.sparse.csc_matrix`, Julia `HDF5.jl`, and
the 10x Genomics / Loompy on-disk convention.

**Preconditions** (h5cpp does not call them implicitly — both would
require mutating a `const &`):
- `arma::SpMat`: `SpMat::sync()` must have completed.
- `Eigen::SparseMatrix`: `makeCompressed()` must have been called;
  `ColMajor` is enforced via `static_assert` (RowMajor refuses at
  compile time).

```cpp
arma::SpMat<double> A(1000, 1000);
// ... populate A ...
A.sync();

h5::write(fd, "/A", A);                                   // CSC group at /A
auto B = h5::read<arma::SpMat<double>>(fd, "/A");         // round-trip
```

## Mixing libraries in one TU

Multiple mapper headers can be active in the same translation unit.
The dispatches are partitioned by type — `h5::write(fd, "a", arma_x)`
and `h5::write(fd, "b", eigen_y)` co-exist without conflict.

The historical foot-gun is Armadillo's LAPACK wrappers vs Blaze's
BLAS path. For mixed arma + blaze code, define `BLAZE_BLAS_MODE=0`
before including Blaze.

## How dispatch works

Each mapper header specialises three traits in `h5::meta`:

1. **`is_contiguous<T>`** — declares that `T::memptr()` / `data()` /
   `valuePtr()` returns a pointer to a contiguous buffer of
   `decay<T>::type` elements.
2. **`storage_representation_impl<T>`** — pins the value to
   `linear_value_dataset` (dense) or routes through
   `is_sparse<T>` for sparse.
3. **`access_traits_t<T>`** — exposes the rank, dimensions, and
   element pointer that `H5Dread.hpp` / `H5Dwrite.hpp` consume.

Once those three are in scope, the type joins the same generic
read/write/create dispatch as `std::vector<T>` — no per-library
special-case at the call site.

## Where to go next

- @ref link_linalg_template_types — full per-library reference
  (type aliases, exact storage layouts, sparse CSC layout in depth,
  mixing rules, dispatch traits)
- @ref curated_topics_stl — STL container support uses the same
  dispatch traits
- @ref example_guide_linalg — runnable per-library cookbook example
- @ref example_guide_sparse — sparse CSC round-trip example
