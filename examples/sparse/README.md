@page example_guide_sparse Sparse Matrices & Vectors — CSC Round-Trip

A sparse matrix is written as an HDF5 *group* containing four datasets in canonical Compressed Sparse Column form. The call shape is the same as for dense linalg, but it returns `h5::gr_t` instead of `h5::ds_t`:

```cpp
arma::SpMat<double> A = ...; A.sync();
h5::gr_t g = h5::write(fd, "/matrix/A", A);          // writes a CSC group
auto A_back = h5::read<arma::SpMat<double>>(fd, "/matrix/A");
```

The on-disk layout is the convention scipy / 10x Genomics / Loompy / h5sparse already use, so the file round-trips with Python consumers without an out-of-band schema. `is_sparse<T>` selects this overload via SFINAE; non-sparse types stay on the dense `h5::write` path.

## Files

| File         | Library  | What it covers                                                                  |
| ------------ | -------- | ------------------------------------------------------------------------------- |
| `arma.cpp`   | Armadillo | `arma::SpMat<double>` matrix + `arma::SpCol<double>` vector round-trip          |
| `eigen.cpp`  | Eigen3    | `Eigen::SparseMatrix<double>` + `Eigen::SparseVector<double>` round-trip        |
| `Makefile`   | —         | Standalone build that resolves `h5cpp/` + vendored arma/eigen from the worktree |

Sparse traits live in the per-library mapper headers (`H5Marma.hpp`, `H5Meigen.hpp`); the I/O dispatch is in `H5Dsparse.hpp`; the on-disk constants and the `is_sparse<T>` / `sparse_traits<T>` contract are in `H5Tsparse.hpp`.

## On-Disk Layout

```
group/
  data    : 1-D, dtype = T,        length = nnz       — non-zero values
  indices : 1-D, uint32,           length = nnz       — row indices (CSC)
  indptr  : 1-D, uint32,           length = cols + 1  — column pointers
  shape   : 1-D, uint64,           length = 2         — [n_rows, n_cols]
@format = "csc"
@axis   = "column"
```

Indices are fixed `uint32` (10x Genomics / Loompy convention). `h5::write` throws if `nnz`, `n_rows`, or `n_cols` exceeds `2^32 - 1`. Vectors (Arma `SpRow`/`SpCol`, Eigen `SparseVector`) are promoted to `1xN` / `Nx1` CSC matrices so a scipy reader sees a normal `csc_matrix`.

## Build & Run

Via the project's CMake build:

```sh
cd <build-dir>
cmake --build . --target examples-sparse-arma examples-sparse-eigen
./examples-sparse-arma
./examples-sparse-eigen
```

Or standalone via the local `Makefile` (does not need the CMake build tree):

```sh
cd examples/sparse
make test      # builds arma + eigen, runs both
```

Expected output:

```
✔ ok    wrote  SpMat: 8x12 nnz=16
✔ ok    read   SpMat: 8x12 nnz=16
        round-trip residual (max|A - A'| on nonzeros): 0
✔ ok    vec round-trip: nnz=3, v(17)=-2.25 (expected -2.25)

✔ ok    wrote  SparseMatrix: 10x15 nnz=23
✔ ok    read   SparseMatrix: 10x15 nnz=23
        round-trip residual (max|A - A'|): 0
✔ ok    vec round-trip: nnz=3, v.coeff(11)=-3.14 (expected -3.14)
```

Inspecting the result with `h5dump arma.h5` shows the four datasets and two attributes per CSC group exactly as the layout block above describes.

The arma example sticks to a hand-rolled `max(abs(...))` over `arma::SpMat::const_iterator` rather than `arma::norm(...)`, because `norm` pulls in the BLAS `dnrm2_` symbol and we want the example to link without an external BLAS dependency.

## Preconditions

Both libraries lazy-cache inserts; h5cpp does **not** call `sync()` / `makeCompressed()` implicitly to avoid mutating a user's `const SpMat&`.

| Library    | Required call before `h5::write`         |
| ---------- | ---------------------------------------- |
| Armadillo  | `M.sync()` after `M(i,j) = x` insertions |
| Eigen3     | `M.makeCompressed()` (or use `setFromTriplets` which leaves it compressed) |

A non-compressed Eigen matrix passed to `h5::write` will produce a wrong file — the runtime check is on the trait's `valuePtr()` access path. The arma `sync()` precondition is identical to what direct `values` / `row_indices` / `col_ptrs` access requires per the Arma docs.

## Library × Shape Support

| Library    | Sparse matrix                              | Sparse vector                          | Storage order |
| ---------- | ------------------------------------------ | -------------------------------------- | ------------- |
| Armadillo  | ✔ `arma::SpMat<T>`                          | ✔ `arma::SpRow<T>`, `arma::SpCol<T>`    | CSC (native)  |
| Eigen3     | ✔ `Eigen::SparseMatrix<T, ColMajor, I>`     | ✔ `Eigen::SparseVector<T, ColMajor, I>` | CSC (native)  |
| Eigen3     | ✘ `Eigen::SparseMatrix<T, RowMajor, I>`     | ✘ row-vector form                        | rejected at compile time — would need a transpose on write |

`is_sparse<T>` evaluates `false` for RowMajor Eigen types, so the dense `h5::write` path is selected instead — which then fails its own `unsupported` static_assert with a clear message. If you need CSR on disk, transpose to ColMajor first (`SpMat tmp = src.transpose(); tmp.makeCompressed();`).

## Interop with Python and Julia

### Python — works out of the box

```python
import h5py, scipy.sparse as sp
with h5py.File("arma.h5") as f:
    g = f["/matrix/A"]
    A = sp.csc_matrix(
        (g["data"][:], g["indices"][:], g["indptr"][:]),
        shape=tuple(g["shape"][:]))
```

scipy accepts the `uint32` indices without conversion. The file also matches the **10x Genomics** layout (`/matrix/{data, indices, indptr, shape}`), so scanpy / cellranger / Loompy-adjacent tooling that targets 10x format reads it directly.

The one caveat is the **h5sparse** Python library — it dispatches on an attribute named `h5sparse_format`, not `format`, so its auto-detection won't fire. The group still reads fine with the snippet above.

### Julia — needs one `.+ 1`

```julia
using HDF5, SparseArrays
h5open("arma.h5", "r") do f
    g       = f["/matrix/A"]
    shape   = read(g, "shape")             # [n_rows, n_cols]
    data    = read(g, "data")
    indices = read(g, "indices") .+ 1      # 0-based -> 1-based
    indptr  = read(g, "indptr")  .+ 1      # 0-based -> 1-based
    A = SparseMatrixCSC(shape[1], shape[2], indptr, indices, data)
end
```

The two `.+ 1` are intrinsic to crossing the C/Python ↔ Julia boundary — scipy and the C convention are 0-based, `SparseMatrixCSC` is 1-based. Writing both 0-based and 1-based copies to disk would double the index storage for one ecosystem's convenience, so the file stays 0-based and the Julia reader adjusts.

`JLD.jl` / `JLD2.jl` won't auto-recognize the file — they use their own ad-hoc layout (the historical `_refs` directory the earlier sparse note complained about). For Julia, route through `HDF5.jl` directly as above.

### Interop matrix

| Reader                                         | Reads our files | Notes                                                                        |
| ---------------------------------------------- | --------------- | ---------------------------------------------------------------------------- |
| Python `h5py` + `scipy.sparse.csc_matrix`      | ✔ ok            | 4-line snippet above; no conversion needed                                   |
| Python `scanpy` / `cellranger` (10x layout)    | ✔ ok            | Same dataset names as 10x Genomics matrix format                             |
| Python `h5sparse`                              | ◇ partial       | Auto-dispatch keyed off `h5sparse_format` not `format`; manual read works    |
| Julia `HDF5.jl` + `SparseArrays`               | ✔ ok            | After `.+ 1` to convert 0-based indices                                      |
| Julia `JLD.jl` / `JLD2.jl`                     | ✘ na            | Uses its own non-standard layout; bypass via `HDF5.jl`                       |

`@format` and `@axis` are advisory attributes — they let format-aware readers like Loompy dispatch correctly; plain h5py / HDF5.jl ignore them and read the four datasets directly.

## CMake Wiring

```cmake
if(${ARMADILLO_FOUND})
    add_h5cpp_example(sparse-arma sparse/arma.cpp LIBRARIES libarmadillo)
endif()
add_h5cpp_example(sparse-eigen sparse/eigen.cpp LIBRARIES Eigen3::Eigen)
```

Lives in `examples/CMakeLists.txt` right after the linalg block. The `sparse-arma` target is gated on `ARMADILLO_FOUND` (same gate as the dense `arma` example); `sparse-eigen` has no gate because Eigen3 is required by the wider build. Output binaries land in `<build>/examples-sparse-arma` and `<build>/examples-sparse-eigen`.

## Build State (as of HEAD)

| Target                    | Status                                                                |
| ------------------------- | --------------------------------------------------------------------- |
| `examples-sparse-arma`    | ✔ ok — SpMat 8x12 round-trip + SpCol vector round-trip, residual = 0  |
| `examples-sparse-eigen`   | ✔ ok — SparseMatrix 10x15 + SparseVector round-trip, residual = 0     |

Both targets also build via the local `Makefile` in this directory (resolves headers from `../../h5cpp` plus the vendored `thirdparty/armadillo` and `thirdparty/eigen3` trees) for standalone iteration outside the CMake build tree.

## Cross-References

- **`examples/linalg/`** — dense linalg-container round-trip; same `h5::write` / `h5::read<T>` call shape, returns `h5::ds_t`.
- **`h5cpp/H5Tsparse.hpp`** — `is_sparse<T>` / `sparse_traits<T>` contract, on-disk name constants.
- **`h5cpp/H5Dsparse.hpp`** — `h5::write(parent, name, spmat)` returning `h5::gr_t`, plus the symmetric `h5::read<Sparse>`.
- **`h5cpp/H5Marma.hpp`** / **`h5cpp/H5Meigen.hpp`** — per-library `sparse_traits` specializations.

## Source

- [`arma.cpp`](arma_8cpp-example.html) — rendered with syntax highlighting
- [`eigen.cpp`](eigen_8cpp-example.html) — rendered with syntax highlighting
