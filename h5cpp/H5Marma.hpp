/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

#include "H5Tsparse.hpp"

#if defined(ARMA_INCLUDES) || defined(H5CPP_USE_ARMADILLO)
namespace h5::arma {
		template<class T> using rowvec = ::arma::Row<T>;
		template<class T> using colvec = ::arma::Col<T>;
		template<class T> using colmat = ::arma::Mat<T>;
		template<class T> using cube   = ::arma::Cube<T>;

		// is_linalg_type := filter
		template <class Object, class T = typename impl::decay<Object>::type> using is_supported =
		std::bool_constant<std::is_same_v<Object,h5::arma::cube<T>> || std::is_same_v<Object,h5::arma::colmat<T>>
			|| std::is_same_v<Object,h5::arma::rowvec<T>> ||  std::is_same_v<Object,h5::arma::colvec<T>>>;
}

namespace h5::meta {
    template <class T> struct is_contiguous<h5::arma::rowvec<T>> : std::true_type {};
    template <class T> struct is_contiguous<h5::arma::colvec<T>> : std::true_type {};
    template <class T> struct is_contiguous<h5::arma::colmat<T>> : std::true_type {};
    template <class T> struct is_contiguous<h5::arma::cube<T>> : std::true_type {};

    // Register types so generic access_traits_t fallbacks don't create ambiguous partial specializations
    template <class T> struct detail::has_explicit_access_traits<h5::arma::rowvec<T>> : std::true_type {};
    template <class T> struct detail::has_explicit_access_traits<h5::arma::colvec<T>> : std::true_type {};
    template <class T> struct detail::has_explicit_access_traits<h5::arma::colmat<T>> : std::true_type {};
    template <class T> struct detail::has_explicit_access_traits<h5::arma::cube<T>> : std::true_type {};
}
// Explicit storage_representation so h5::awrite's static_assert(storage != unsupported)
// always passes for these vendored linalg types, independent of structural fallbacks.
namespace h5::meta::detail_capabilities {
    template <class T> struct has_explicit_storage_repr<h5::arma::rowvec<T>> : std::true_type {};
    template <class T> struct has_explicit_storage_repr<h5::arma::colvec<T>> : std::true_type {};
    template <class T> struct has_explicit_storage_repr<h5::arma::colmat<T>> : std::true_type {};
    template <class T> struct has_explicit_storage_repr<h5::arma::cube<T>>   : std::true_type {};
    template <class T> struct storage_representation_impl<h5::arma::rowvec<T>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    template <class T> struct storage_representation_impl<h5::arma::colvec<T>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    template <class T> struct storage_representation_impl<h5::arma::colmat<T>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    template <class T> struct storage_representation_impl<h5::arma::cube<T>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
}

namespace h5::impl {
	// 1.) object -> H5T_xxx

	// Register Armadillo types so the structural decay fallback in H5Mstl.hpp
	// does not create an ambiguous partial-specialisation with these explicit specs.
	template <class T> struct detail::has_explicit_decay<h5::arma::rowvec<T>> : std::true_type {};
	template <class T> struct detail::has_explicit_decay<h5::arma::colvec<T>> : std::true_type {};
	template <class T> struct detail::has_explicit_decay<h5::arma::colmat<T>> : std::true_type {};
	template <class T> struct detail::has_explicit_decay<h5::arma::cube<T>>   : std::true_type {};

	template <class T> struct decay<h5::arma::rowvec<T>>{ using type = T; };
	template <class T> struct decay<h5::arma::colvec<T>>{ using type = T; };
	template <class T> struct decay<h5::arma::colmat<T>>{ using type = T; };
	template <class T> struct decay<h5::arma::cube<T>>{ using type = T; };

	// get read access to datastaore
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::arma::is_supported<Object>::value,
	const T*> data( const Object& ref ){
			return ref.memptr();
	}

	// read write access
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::arma::is_supported<Object>::value,
	T*> data( Object& ref ){
			return ref.memptr();
	}

	// rank
	template<class T> struct rank<h5::arma::rowvec<T>> : public std::integral_constant<size_t,1>{};
	template<class T> struct rank<h5::arma::colvec<T>> : public std::integral_constant<size_t,1>{};
	template<class T> struct rank<h5::arma::colmat<T>> : public std::integral_constant<size_t,2>{};
	template<class T> struct rank<h5::arma::cube<T>> : public std::integral_constant<size_t,3>{};

	// determine rank and dimensions
	template <class T> inline std::array<size_t,1> size( const h5::arma::rowvec<T>& ref ){ return {ref.n_elem};}
	template <class T> inline std::array<size_t,1> size( const h5::arma::colvec<T>& ref ){ return {ref.n_elem};}
	template <class T> inline std::array<size_t,2> size( const h5::arma::colmat<T>& ref ){ return {ref.n_rows,ref.n_cols};}
	template <class T> inline std::array<size_t,3> size( const h5::arma::cube<T>& ref ){ return {ref.n_slices,ref.n_cols,ref.n_rows};}

	// CTOR-s 
	template <class T> struct get<h5::arma::rowvec<T>> {
		static inline  h5::arma::rowvec<T> ctor( std::array<size_t,1> dims ){
			return h5::arma::rowvec<T>( dims[0] );
	}};
	template <class T> struct get<h5::arma::colvec<T>> {
		static inline h5::arma::colvec<T> ctor( std::array<size_t,1> dims ){
			return h5::arma::colvec<T>( dims[0] );
	}};
	template <class T> struct get<h5::arma::colmat<T>> {
		static inline h5::arma::colmat<T> ctor( std::array<size_t,2> dims ){
			return h5::arma::colmat<T>( dims[0], dims[1] );
	}};
	template <class T> struct get<h5::arma::cube<T>> {
		// h5::impl::size() returns {n_slices, n_cols, n_rows} for arma::cube,
		// so on read dims has the same layout — invert to construct.
		static inline h5::arma::cube<T> ctor( std::array<size_t,3> dims ){
			return h5::arma::cube<T>( dims[2], dims[1], dims[0] );
	}};
}

namespace h5::meta {
    template <class T> struct access_traits_t<h5::arma::rowvec<T>> {
        using element_t = T;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static auto data(const h5::arma::rowvec<T>& c) noexcept { return h5::impl::data(c); }
        static auto data(h5::arma::rowvec<T>& c)       noexcept { return h5::impl::data(c); }
        static auto size(const h5::arma::rowvec<T>& c) noexcept { return h5::impl::size(c); }
        static std::size_t bytes(const h5::arma::rowvec<T>& c) noexcept {
            auto s = size(c); std::size_t n = 1;
            for (std::size_t i = 0; i < s.size(); ++i) n *= s[i];
            return n * sizeof(element_t);
        }
    };
    template <class T> struct access_traits_t<h5::arma::colvec<T>> {
        using element_t = T;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static auto data(const h5::arma::colvec<T>& c) noexcept { return h5::impl::data(c); }
        static auto data(h5::arma::colvec<T>& c)       noexcept { return h5::impl::data(c); }
        static auto size(const h5::arma::colvec<T>& c) noexcept { return h5::impl::size(c); }
        static std::size_t bytes(const h5::arma::colvec<T>& c) noexcept {
            auto s = size(c); std::size_t n = 1;
            for (std::size_t i = 0; i < s.size(); ++i) n *= s[i];
            return n * sizeof(element_t);
        }
    };
    template <class T> struct access_traits_t<h5::arma::colmat<T>> {
        using element_t = T;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static auto data(const h5::arma::colmat<T>& c) noexcept { return h5::impl::data(c); }
        static auto data(h5::arma::colmat<T>& c)       noexcept { return h5::impl::data(c); }
        static auto size(const h5::arma::colmat<T>& c) noexcept { return h5::impl::size(c); }
        static std::size_t bytes(const h5::arma::colmat<T>& c) noexcept {
            auto s = size(c); std::size_t n = 1;
            for (std::size_t i = 0; i < s.size(); ++i) n *= s[i];
            return n * sizeof(element_t);
        }
    };
    template <class T> struct access_traits_t<h5::arma::cube<T>> {
        using element_t = T;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static auto data(const h5::arma::cube<T>& c) noexcept { return h5::impl::data(c); }
        static auto data(h5::arma::cube<T>& c)       noexcept { return h5::impl::data(c); }
        static auto size(const h5::arma::cube<T>& c) noexcept { return h5::impl::size(c); }
        static std::size_t bytes(const h5::arma::cube<T>& c) noexcept {
            auto s = size(c); std::size_t n = 1;
            for (std::size_t i = 0; i < s.size(); ++i) n *= s[i];
            return n * sizeof(element_t);
        }
    };
}

// ---------------------- Sparse linalg bindings ---------------------------
// Armadillo SpMat<T> / SpRow<T> / SpCol<T> are stored in canonical CSC form
// natively: values + row_indices + col_ptrs. SpRow and SpCol derive from
// SpMat and reuse the same storage layout; vectors round-trip through the
// generic CSC group layout as 1xN and Nx1 matrices respectively.
//
// Precondition: SpMat::sync() must have completed before the matrix is
// handed to h5::write. The arma docs require this for any direct access to
// values / row_indices / col_ptrs; h5cpp does not call sync() implicitly to
// avoid the const_cast on a user-supplied `const SpMat&`.
namespace h5::meta {

    template <class T> struct is_sparse<::arma::SpMat<T>> : std::true_type {};
    template <class T> struct is_sparse<::arma::SpRow<T>> : std::true_type {};
    template <class T> struct is_sparse<::arma::SpCol<T>> : std::true_type {};

    namespace detail {
        // Shared accessor body: SpRow and SpCol derive from SpMat, so a single
        // SpMat-typed accessor is reusable for all three via base-class slicing.
        template <class T>
        struct arma_spmat_accessor {
            using value_type = T;
            static const value_type* values(const ::arma::SpMat<T>& m) noexcept { return m.values; }
            static std::size_t nnz (const ::arma::SpMat<T>& m) noexcept { return static_cast<std::size_t>(m.n_nonzero); }
            static std::size_t rows(const ::arma::SpMat<T>& m) noexcept { return static_cast<std::size_t>(m.n_rows); }
            static std::size_t cols(const ::arma::SpMat<T>& m) noexcept { return static_cast<std::size_t>(m.n_cols); }

            static void to_u32_outer(const ::arma::SpMat<T>& m, std::uint32_t* dst) {
                const std::size_t n = static_cast<std::size_t>(m.n_cols) + 1;
                for (std::size_t i = 0; i < n; ++i)
                    dst[i] = static_cast<std::uint32_t>(m.col_ptrs[i]);
            }
            static void to_u32_inner(const ::arma::SpMat<T>& m, std::uint32_t* dst) {
                const std::size_t n = static_cast<std::size_t>(m.n_nonzero);
                for (std::size_t i = 0; i < n; ++i)
                    dst[i] = static_cast<std::uint32_t>(m.row_indices[i]);
            }

            static ::arma::SpMat<T> construct(
                std::size_t n_rows, std::size_t n_cols, std::size_t nnz,
                const std::uint32_t* outer, const std::uint32_t* inner,
                const T* data)
            {
                // Arma's CSC constructor takes arma::Col<uword> for rowind/colptr.
                ::arma::Col<::arma::uword> rowind(nnz);
                for (std::size_t i = 0; i < nnz; ++i) rowind[i] = inner[i];
                ::arma::Col<::arma::uword> colptr(n_cols + 1);
                for (std::size_t i = 0; i <= n_cols; ++i) colptr[i] = outer[i];
                ::arma::Col<T> vals(nnz);
                for (std::size_t i = 0; i < nnz; ++i) vals[i] = data[i];
                return ::arma::SpMat<T>(rowind, colptr, vals,
                    static_cast<::arma::uword>(n_rows),
                    static_cast<::arma::uword>(n_cols),
                    /*check_for_zeros=*/false);
            }
        };
    }

    template <class T>
    struct sparse_traits<::arma::SpMat<T>> : detail::arma_spmat_accessor<T> {};

    // SpRow and SpCol inherit SpMat layout; same accessor body, but construct
    // a row / column variant so users get the type they asked for.
    template <class T>
    struct sparse_traits<::arma::SpRow<T>> : detail::arma_spmat_accessor<T> {
        static ::arma::SpRow<T> construct(
            std::size_t n_rows, std::size_t n_cols, std::size_t nnz,
            const std::uint32_t* outer, const std::uint32_t* inner,
            const T* data)
        {
            return ::arma::SpRow<T>(detail::arma_spmat_accessor<T>::construct(
                n_rows, n_cols, nnz, outer, inner, data));
        }
    };

    template <class T>
    struct sparse_traits<::arma::SpCol<T>> : detail::arma_spmat_accessor<T> {
        static ::arma::SpCol<T> construct(
            std::size_t n_rows, std::size_t n_cols, std::size_t nnz,
            const std::uint32_t* outer, const std::uint32_t* inner,
            const T* data)
        {
            return ::arma::SpCol<T>(detail::arma_spmat_accessor<T>::construct(
                n_rows, n_cols, nnz, outer, inner, data));
        }
    };
}
#endif
