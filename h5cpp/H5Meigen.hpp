/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include <hdf5.h>
#include "H5Tmeta.hpp"
#include "H5Tsparse.hpp"
#include <tuple>
#include <type_traits>
#include <array>
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

#if defined(EIGEN_CORE_H) || defined(H5CPP_USE_EIGEN3)
/*
	Matrix<typename Scalar,
       int RowsAtCompileTime,
       int ColsAtCompileTime,
       int Options = 0,          // ColMajor | RowMajor
       int MaxRowsAtCompileTime = RowsAtCompileTime,
       int MaxColsAtCompileTime = ColsAtCompileTime>
*/

namespace h5::meta {

    template<class T,int R,int C, int O> struct is_contiguous<::Eigen::Matrix<T,R,C,O>> : std::true_type {};
    template<class T,int R,int C, int O> struct is_contiguous<::Eigen::Array<T,R,C,O>> : std::true_type {};
}
// Explicit storage_representation so h5::awrite's static_assert(storage != unsupported)
// always passes for vendored Eigen matrices/arrays.
namespace h5::meta::detail_capabilities {
    template<class T,int R,int C, int O> struct has_explicit_storage_repr<::Eigen::Matrix<T,R,C,O>> : std::true_type {};
    template<class T,int R,int C, int O> struct has_explicit_storage_repr<::Eigen::Array<T,R,C,O>>  : std::true_type {};
    template<class T,int R,int C, int O> struct storage_representation_impl<::Eigen::Matrix<T,R,C,O>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    template<class T,int R,int C, int O> struct storage_representation_impl<::Eigen::Array<T,R,C,O>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
}

namespace h5::impl {
	// 1.) object -> H5T_xxx
	// Register Eigen types in has_explicit_decay to prevent ambiguity with the
	// structural decay fallback (Eigen matrices expose value_type = Scalar).
	template<class T,int R,int C, int O>
	struct detail::has_explicit_decay<::Eigen::Matrix<T,R,C,O>> : std::true_type {};
	template<class T,int R,int C, int O>
	struct detail::has_explicit_decay<::Eigen::Array<T,R,C,O>>  : std::true_type {};

	// 1.) object -> H5T_xxx
	template<class T,int R,int C, int O> struct decay<::Eigen::Matrix<T,R,C,O>>{ using type = T; };
	template<class T,int R,int C, int O> struct decay<::Eigen::Array<T,R,C,O>>{ using type = T; };
	    

	// get read access to datastaore
	template<class T,int R,int C,int O, int MR=R,int MC=C>
	const T* data(const ::Eigen::Matrix<T,R,C,O,MR,MC>& ref ){
			return ref.data();
	}
	// read write access
	template<class T,int R,int C,int O, int MR=R,int MC=C>
	T* data(::Eigen::Matrix<T,R,C,O,MR,MC>& ref ){
			return ref.data();
	}
	// get read access to datastaore
	template<class T,int R,int C,int O, int MR=R,int MC=C>
	const T* data(const ::Eigen::Array<T,R,C,O,MR,MC>& ref ){
			return ref.data();
	}
	// read write access
	template<class T,int R,int C,int O, int MR=R,int MC=C>
	T* data(::Eigen::Array<T,R,C,O,MR,MC>& ref ){
			return ref.data();
	}
	// determine rank and dimensions
	// MATRICES
	template<class T,int R,int C,int MR=R,int MC=C>
	inline std::array<size_t,2> size( const ::Eigen::Matrix<T,R,C,::Eigen::RowMajor,MR,MC>& ref ){
		return {(hsize_t)ref.rows(),(hsize_t)ref.cols()};
	}
	template<class T,int R,int C,int MR=R,int MC=C>
	inline std::array<size_t,2> size( const ::Eigen::Matrix<T,R,C,::Eigen::ColMajor,MR,MC>& ref ){
		return {(hsize_t)ref.cols(), (hsize_t)ref.rows()};
	}
	// ARRAYS
	template<class T,int R,int C,int MR=R,int MC=C>
	inline std::array<size_t,2> size( const ::Eigen::Array<T,R,C,::Eigen::RowMajor,MR,MC>& ref ){
		return {(hsize_t)ref.rows(),(hsize_t)ref.cols()};
	}

	template<class T,int R,int C,int MR=R,int MC=C>
	inline std::array<size_t,2> size( const ::Eigen::Array<T,R,C,::Eigen::ColMajor,MR,MC>& ref ){
		return {(hsize_t)ref.cols(), (hsize_t)ref.rows()};
	}

	// rank
	template<class T,int R,int C,int O,int MR,int MC>
	struct rank<::Eigen::Matrix<T,R,C,O,MR,MC>> : public std::integral_constant<size_t,2>{};
	template<class T,int R,int C,int O,int MR,int MC>
	struct rank<::Eigen::Array<T,R,C,O,MR,MC>>  : public std::integral_constant<size_t,2>{};
	// CTOR-s
	// MATRICES
	template<class T,int R,int C>
	struct get<::Eigen::Matrix<T,R,C,::Eigen::RowMajor>> {
		static inline ::Eigen::Matrix<T,R,C,::Eigen::RowMajor> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Matrix<T,R,C,::Eigen::RowMajor>( dims[0], dims[1] );
	}};
	template<class T,int R,int C, int MR, int MC>
	struct get<::Eigen::Matrix<T,R,C,::Eigen::RowMajor,MR,MC>> {
		static inline ::Eigen::Matrix<T,R,C,::Eigen::RowMajor,MR,MC> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Matrix<T,R,C,::Eigen::RowMajor,MR,MC>( dims[0], dims[1] );
	}};
	// access_traits_t::size returns {rows, cols} canonically (see line 202-203).
	// Constructors here must read dims in that same order; the historical swap
	// to dims[1],dims[0] paired with a legacy {cols,rows} impl::size convention
	// that the access_traits_t rationalization replaced.
	template<class T,int R,int C>
	struct get<::Eigen::Matrix<T,R,C,::Eigen::ColMajor>> {
		static inline ::Eigen::Matrix<T,R,C,::Eigen::ColMajor> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Matrix<T,R,C,::Eigen::ColMajor>( dims[0], dims[1] );
	}};
	template<class T,int R,int C, int MR, int MC>
	struct get<::Eigen::Matrix<T,R,C,::Eigen::ColMajor,MR,MC>> {
		static inline ::Eigen::Matrix<T,R,C,::Eigen::ColMajor,MR,MC> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Matrix<T,R,C,::Eigen::ColMajor,MR,MC>( dims[0], dims[1] );
	}};
	// ARRAYS
	template<class T,int R,int C>
	struct get<::Eigen::Array<T,R,C,::Eigen::RowMajor>> {
		static inline ::Eigen::Array<T,R,C,::Eigen::RowMajor> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Array<T,R,C,::Eigen::RowMajor>( dims[0], dims[1] );
	}};
	template<class T,int R,int C, int MR, int MC>
	struct get<::Eigen::Array<T,R,C,::Eigen::RowMajor,MR,MC>> {
		static inline ::Eigen::Array<T,R,C,::Eigen::RowMajor,MR,MC> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Array<T,R,C,::Eigen::RowMajor,MR,MC>( dims[0], dims[1] );
	}};
	template<class T,int R,int C>
	struct get<::Eigen::Array<T,R,C,::Eigen::ColMajor>> {
		static inline ::Eigen::Array<T,R,C,::Eigen::ColMajor> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Array<T,R,C,::Eigen::ColMajor>( dims[0], dims[1] );
	}};
	template<class T,int R,int C, int MR, int MC>
	struct get<::Eigen::Array<T,R,C,::Eigen::ColMajor,MR,MC>> {
		static inline ::Eigen::Array<T,R,C,::Eigen::ColMajor,MR,MC> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Array<T,R,C,::Eigen::ColMajor,MR,MC>( dims[0], dims[1] );
	}};
}

namespace h5::meta {
    template<class T,int R,int C,int O, int MR=R,int MC=C>
    T* data(const ::Eigen::Matrix<T,R,C,O,MR,MC>& ref ){
            return const_cast<T*>( ref.data() );
    }
    template<class T,int R,int C,int O, int MR=R,int MC=C>
    T* data(const ::Eigen::Array<T,R,C,O,MR,MC>& ref ){
            return const_cast<T*>( ref.data() );
    }
    template<class T,int R,int C,int MR=R,int MC=C>
    inline std::array<size_t,2> size( const ::Eigen::Matrix<T,R,C,::Eigen::RowMajor,MR,MC>& ref ){
        return {(hsize_t)ref.rows(),(hsize_t)ref.cols()};
    }
    template<class T,int R,int C,int MR=R,int MC=C>
    inline std::array<size_t,2> size( const ::Eigen::Matrix<T,R,C,::Eigen::ColMajor,MR,MC>& ref ){
        return {(hsize_t)ref.cols(), (hsize_t)ref.rows()};
    }
    template<class T,int R,int C,int MR=R,int MC=C>
    inline std::array<size_t,2> size( const ::Eigen::Array<T,R,C,::Eigen::RowMajor,MR,MC>& ref ){
        return {(hsize_t)ref.rows(),(hsize_t)ref.cols()};
    }
    template<class T,int R,int C,int MR=R,int MC=C>
    inline std::array<size_t,2> size( const ::Eigen::Array<T,R,C,::Eigen::ColMajor,MR,MC>& ref ){
        return {(hsize_t)ref.cols(), (hsize_t)ref.rows()};
    }
}

namespace h5::meta {
    template<class T,int R,int C, int O> struct decay<::Eigen::Matrix<T,R,C,O>> : h5::impl::decay<::Eigen::Matrix<T,R,C,O>> {};
    template<class T,int R,int C, int O> struct decay<::Eigen::Array<T,R,C,O>> : h5::impl::decay<::Eigen::Array<T,R,C,O>> {};
    template<class T,int R,int C,int O,int MR,int MC> struct rank<::Eigen::Matrix<T,R,C,O,MR,MC>> : h5::impl::rank<::Eigen::Matrix<T,R,C,O,MR,MC>> {};
    template<class T,int R,int C,int O,int MR,int MC> struct rank<::Eigen::Array<T,R,C,O,MR,MC>> : h5::impl::rank<::Eigen::Array<T,R,C,O,MR,MC>> {};
    template<class T,int R,int C> struct get<::Eigen::Matrix<T,R,C,::Eigen::RowMajor>> : h5::impl::get<::Eigen::Matrix<T,R,C,::Eigen::RowMajor>> {};
    template<class T,int R,int C, int MR, int MC> struct get<::Eigen::Matrix<T,R,C,::Eigen::RowMajor,MR,MC>> : h5::impl::get<::Eigen::Matrix<T,R,C,::Eigen::RowMajor,MR,MC>> {};
    template<class T,int R,int C> struct get<::Eigen::Matrix<T,R,C,::Eigen::ColMajor>> : h5::impl::get<::Eigen::Matrix<T,R,C,::Eigen::ColMajor>> {};
    template<class T,int R,int C, int MR, int MC> struct get<::Eigen::Matrix<T,R,C,::Eigen::ColMajor,MR,MC>> : h5::impl::get<::Eigen::Matrix<T,R,C,::Eigen::ColMajor,MR,MC>> {};
    template<class T,int R,int C> struct get<::Eigen::Array<T,R,C,::Eigen::RowMajor>> : h5::impl::get<::Eigen::Array<T,R,C,::Eigen::RowMajor>> {};
    template<class T,int R,int C, int MR, int MC> struct get<::Eigen::Array<T,R,C,::Eigen::RowMajor,MR,MC>> : h5::impl::get<::Eigen::Array<T,R,C,::Eigen::RowMajor,MR,MC>> {};
    template<class T,int R,int C> struct get<::Eigen::Array<T,R,C,::Eigen::ColMajor>> : h5::impl::get<::Eigen::Array<T,R,C,::Eigen::ColMajor>> {};
    template<class T,int R,int C, int MR, int MC> struct get<::Eigen::Array<T,R,C,::Eigen::ColMajor,MR,MC>> : h5::impl::get<::Eigen::Array<T,R,C,::Eigen::ColMajor,MR,MC>> {};

    // Explicit access_traits_t for Eigen Matrix/Array: must report rank-2 dimensions
    // {rows, cols}.  Without these, the generic contiguous fallback reports `{c.size()}`
    // (rank-1 element count), causing the attribute to be created as a flat 1-D blob.
    // On read-back, impl::get<MatrixXd>::ctor would then read uninitialised dims[1]
    // and construct an (n, 0) matrix whose .data() is null, tripping H5Aread.
    template<class T,int R,int C,int O,int MR,int MC>
    struct detail::has_explicit_access_traits<::Eigen::Matrix<T,R,C,O,MR,MC>> : std::true_type {};
    template<class T,int R,int C,int O,int MR,int MC>
    struct detail::has_explicit_access_traits<::Eigen::Array<T,R,C,O,MR,MC>>  : std::true_type {};

    template<class T,int R,int C,int O,int MR,int MC>
    struct access_traits_t<::Eigen::Matrix<T,R,C,O,MR,MC>> {
        using element_t = T;
        using pointer_t = const T*;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static const T* data(const ::Eigen::Matrix<T,R,C,O,MR,MC>& c) noexcept { return c.data(); }
        static T*       data(::Eigen::Matrix<T,R,C,O,MR,MC>& c)       noexcept { return c.data(); }
        static std::array<std::size_t,2> size(const ::Eigen::Matrix<T,R,C,O,MR,MC>& c) noexcept {
            return { static_cast<std::size_t>(c.rows()), static_cast<std::size_t>(c.cols()) };
        }
        static std::size_t bytes(const ::Eigen::Matrix<T,R,C,O,MR,MC>& c) noexcept {
            return static_cast<std::size_t>(c.rows()) * static_cast<std::size_t>(c.cols()) * sizeof(T);
        }
    };
    template<class T,int R,int C,int O,int MR,int MC>
    struct access_traits_t<::Eigen::Array<T,R,C,O,MR,MC>> {
        using element_t = T;
        using pointer_t = const T*;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static const T* data(const ::Eigen::Array<T,R,C,O,MR,MC>& c) noexcept { return c.data(); }
        static T*       data(::Eigen::Array<T,R,C,O,MR,MC>& c)       noexcept { return c.data(); }
        static std::array<std::size_t,2> size(const ::Eigen::Array<T,R,C,O,MR,MC>& c) noexcept {
            return { static_cast<std::size_t>(c.rows()), static_cast<std::size_t>(c.cols()) };
        }
        static std::size_t bytes(const ::Eigen::Array<T,R,C,O,MR,MC>& c) noexcept {
            return static_cast<std::size_t>(c.rows()) * static_cast<std::size_t>(c.cols()) * sizeof(T);
        }
    };
}

// ---------------------- Sparse linalg bindings ---------------------------
// Eigen sparse types live in the SparseCore module, which is not pulled in by
// <Eigen/Core>. Guard so users who only include Core don't pay for the sparse
// trait specializations.
#ifdef EIGEN_SPARSECORE_MODULE_H
namespace h5::meta {

    // Only ColMajor (CSC) is supported on disk. RowMajor SparseMatrix would
    // need a transpose on write; refuse at compile time so users get a clear
    // signal instead of a silently mislabelled file.
    template <class T, int O, class I>
    struct is_sparse<::Eigen::SparseMatrix<T,O,I>>
        : std::bool_constant<(O & ::Eigen::RowMajorBit) == 0> {};

    template <class T, int O, class I>
    struct is_sparse<::Eigen::SparseVector<T,O,I>>
        : std::bool_constant<(O & ::Eigen::RowMajorBit) == 0> {};

    namespace detail {
        // SparseMatrix accessor — CSC only. innerIndex = row indices,
        // outerIndex = column pointers (length cols+1).
        // Precondition: src.makeCompressed() has been called. h5::write does
        // not call it implicitly to avoid mutating a `const SparseMatrix&`.
        template <class T, int O, class I>
        struct eigen_spmat_accessor {
            using value_type  = T;
            using sparse_type = ::Eigen::SparseMatrix<T,O,I>;
            static_assert((O & ::Eigen::RowMajorBit) == 0,
                "h5cpp: only ColMajor Eigen::SparseMatrix can be written as CSC.");
            static const value_type* values(const sparse_type& m) { return m.valuePtr(); }
            static std::size_t nnz (const sparse_type& m) { return static_cast<std::size_t>(m.nonZeros()); }
            static std::size_t rows(const sparse_type& m) { return static_cast<std::size_t>(m.rows()); }
            static std::size_t cols(const sparse_type& m) { return static_cast<std::size_t>(m.cols()); }

            static void to_u32_outer(const sparse_type& m, std::uint32_t* dst) {
                const std::size_t n = static_cast<std::size_t>(m.cols()) + 1;
                const I* src = m.outerIndexPtr();
                for (std::size_t i = 0; i < n; ++i)
                    dst[i] = static_cast<std::uint32_t>(src[i]);
            }
            static void to_u32_inner(const sparse_type& m, std::uint32_t* dst) {
                const std::size_t n = static_cast<std::size_t>(m.nonZeros());
                const I* src = m.innerIndexPtr();
                for (std::size_t i = 0; i < n; ++i)
                    dst[i] = static_cast<std::uint32_t>(src[i]);
            }

            static sparse_type construct(
                std::size_t n_rows, std::size_t n_cols, std::size_t nnz,
                const std::uint32_t* outer, const std::uint32_t* inner,
                const value_type* data)
            {
                // Stage uint32 buffers into the matrix's native StorageIndex
                // type, then build via Eigen::Map (read-only) and assign.
                std::vector<I> outer_buf(n_cols + 1);
                for (std::size_t i = 0; i <= n_cols; ++i) outer_buf[i] = static_cast<I>(outer[i]);
                std::vector<I> inner_buf(nnz);
                for (std::size_t i = 0; i < nnz; ++i)     inner_buf[i] = static_cast<I>(inner[i]);
                // Map<const SparseMatrix> needs const-correct pointers; cast away
                // const on data — Map only reads, then `mat = map` copies out.
                ::Eigen::Map<const sparse_type> view(
                    static_cast<typename sparse_type::Index>(n_rows),
                    static_cast<typename sparse_type::Index>(n_cols),
                    static_cast<typename sparse_type::Index>(nnz),
                    outer_buf.data(), inner_buf.data(),
                    const_cast<value_type*>(data));
                return sparse_type(view);
            }
        };

        // SparseVector accessor — Eigen stores it as a single inner vector,
        // so outerIndexPtr() is null. On-disk we synthesize a 2-element
        // column pointer [0, nnz] for an Nx1 CSC matrix.
        template <class T, int O, class I>
        struct eigen_spvec_accessor {
            using value_type  = T;
            using sparse_type = ::Eigen::SparseVector<T,O,I>;
            static_assert((O & ::Eigen::RowMajorBit) == 0,
                "h5cpp: only ColumnVector Eigen::SparseVector is supported.");
            static const value_type* values(const sparse_type& v) { return v.valuePtr(); }
            static std::size_t nnz (const sparse_type& v) { return static_cast<std::size_t>(v.nonZeros()); }
            static std::size_t rows(const sparse_type& v) { return static_cast<std::size_t>(v.size()); }
            static std::size_t cols(const sparse_type& /*v*/) { return 1; }

            static void to_u32_outer(const sparse_type& v, std::uint32_t* dst) {
                // synthesized [0, nnz] for an Nx1 column.
                dst[0] = 0;
                dst[1] = static_cast<std::uint32_t>(v.nonZeros());
            }
            static void to_u32_inner(const sparse_type& v, std::uint32_t* dst) {
                const std::size_t n = static_cast<std::size_t>(v.nonZeros());
                const I* src = v.innerIndexPtr();
                for (std::size_t i = 0; i < n; ++i)
                    dst[i] = static_cast<std::uint32_t>(src[i]);
            }

            static sparse_type construct(
                std::size_t n_rows, std::size_t n_cols, std::size_t nnz,
                const std::uint32_t* /*outer*/, const std::uint32_t* inner,
                const value_type* data)
            {
                if (n_cols != 1)
                    throw std::runtime_error(
                        "h5::read<Eigen::SparseVector>: on-disk matrix has more than "
                        "one column; cannot be loaded as a vector.");
                sparse_type v(static_cast<typename sparse_type::Index>(n_rows));
                v.reserve(static_cast<typename sparse_type::Index>(nnz));
                for (std::size_t i = 0; i < nnz; ++i)
                    v.insertBack(static_cast<typename sparse_type::Index>(inner[i])) = data[i];
                v.finalize();
                return v;
            }
        };
    }

    template <class T, int O, class I>
    struct sparse_traits<::Eigen::SparseMatrix<T,O,I>>
        : detail::eigen_spmat_accessor<T,O,I> {};

    template <class T, int O, class I>
    struct sparse_traits<::Eigen::SparseVector<T,O,I>>
        : detail::eigen_spvec_accessor<T,O,I> {};
}
#endif // EIGEN_SPARSECORE_MODULE_H

#endif
