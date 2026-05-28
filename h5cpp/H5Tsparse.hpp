/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

// Sparse matrix / vector meta-traits.
//
// The h5cpp public sparse API lives in H5Dsparse.hpp; this header only declares
// the trait surface that per-library mappers (H5Marma, H5Meigen, ...) specialize.
//
// On-disk layout is canonical Compressed Sparse Column (CSC):
//   group/
//     data    : 1-D, dtype=T,            non-zeros, length nnz
//     indices : 1-D, uint32,             row indices, length nnz
//     indptr  : 1-D, uint32,             column pointers, length n_cols+1
//     shape   : 1-D, uint64, length=2,  [n_rows, n_cols]
//   @format = "csc"
//   @axis   = "column"
//
// Index width is fixed uint32 on disk (10x Genomics / Loompy convention) with
// an overflow guard at write time: if any of {nnz, n_rows, n_cols} exceeds
// 2^32-1 the write throws. The trait writes converted uint32 buffers directly
// into caller-supplied storage so per-library precision is preserved up to
// the file boundary.
//
// Sparse vectors (Arma SpRow / SpCol, Eigen SparseVector) are promoted to
// 1xN / Nx1 CSC matrices so files round-trip with scipy.sparse.csc_matrix
// and 10x-style consumers unchanged.

namespace h5::meta {

    // Primary template: T is not a sparse linalg type.
    // Library mappers specialize this to std::true_type.
    template <class T, class = void>
    struct is_sparse : std::false_type {};

    template <class T>
    inline constexpr bool is_sparse_v = is_sparse<T>::value;

    // sparse_traits<Sparse>: per-library accessor contract.
    //
    // Specializations must provide:
    //   using value_type = T;                              // element scalar type
    //   static std::size_t       nnz   (const Sparse&);
    //   static std::size_t       rows  (const Sparse&);
    //   static std::size_t       cols  (const Sparse&);
    //   static const value_type* values(const Sparse&);    // non-zero values, length nnz
    //
    //   // fill caller-provided uint32 buffers with the CSC indices.
    //   // outer length = cols()+1, inner length = nnz().
    //   static void to_u32_outer(const Sparse&, std::uint32_t* dst);
    //   static void to_u32_inner(const Sparse&, std::uint32_t* dst);
    //
    //   // construct from disk-decoded uint32 CSC buffers + values.
    //   static Sparse construct(
    //       std::size_t rows, std::size_t cols, std::size_t nnz,
    //       const std::uint32_t* outer,        // length cols+1
    //       const std::uint32_t* inner,        // length nnz
    //       const value_type*    data);        // length nnz
    //
    // Precondition for write: the source matrix must be in compressed/synced
    // CSC form (Arma: post-sync(); Eigen: makeCompressed() && !RowMajor).
    template <class T>
    struct sparse_traits; // intentionally undefined; specializations live with each library mapper.

    // Stable on-disk dataset / attribute names.
    namespace sparse {
        inline constexpr const char* k_data    = "data";
        inline constexpr const char* k_indices = "indices";
        inline constexpr const char* k_indptr  = "indptr";
        inline constexpr const char* k_shape   = "shape";
        inline constexpr const char* k_format  = "format";
        inline constexpr const char* k_axis    = "axis";
        inline constexpr const char* v_csc     = "csc";
        inline constexpr const char* v_column  = "column";
    }
}
