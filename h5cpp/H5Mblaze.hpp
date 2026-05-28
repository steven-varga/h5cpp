/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

// Either gate fires for the matrix/vector specializations:
//   _BLAZE_MATH_MODULE_H_        — defined by <blaze/Math.h> (umbrella; pulls everything
//                                   including blaze/math/lapack/clapack/* which conflicts
//                                   with Armadillo's Fortran LAPACK declarations).
//   _BLAZE_MATH_DYNAMICMATRIX_H_ — defined by <blaze/math/DynamicMatrix.h> (narrow include;
//                                   skips the LAPACK wrappers, so Blaze + Armadillo can
//                                   coexist in one TU as long as the user prefers the
//                                   narrow header.)
#if defined(_BLAZE_MATH_MODULE_H_) || defined(_BLAZE_MATH_DYNAMICMATRIX_H_) || defined(H5CPP_USE_BLAZE)
namespace h5::blaze {
		template<class T> using rowvec = ::blaze::DynamicVector<T,::blaze::rowVector>;
		template<class T> using colvec = ::blaze::DynamicVector<T,::blaze::columnVector>;
		template<class T> using rowmat = ::blaze::DynamicMatrix<T,::blaze::rowMajor>;
		template<class T> using colmat = ::blaze::DynamicMatrix<T,::blaze::columnMajor>;

		// is_linalg_type := filter
		template <class Object, class T = typename impl::decay<Object>::type> using is_supported =
		std::bool_constant<std::is_same_v<Object,rowmat<T>> || std::is_same_v<Object,colmat<T>>
			|| std::is_same_v<Object,rowvec<T>> ||  std::is_same_v<Object,colvec<T>>>;
}

namespace h5::meta {
		template <class T> struct is_contiguous<h5::blaze::rowvec<T>> : std::true_type {};
		template <class T> struct is_contiguous<h5::blaze::colvec<T>> : std::true_type {};
		template <class T> struct is_contiguous<h5::blaze::rowmat<T>> : std::true_type {};
		template <class T> struct is_contiguous<h5::blaze::colmat<T>> : std::true_type {};

		// Register types so generic access_traits_t fallbacks don't create ambiguous partial specializations
		template <class T> struct detail::has_explicit_access_traits<h5::blaze::rowmat<T>> : std::true_type {};
		template <class T> struct detail::has_explicit_access_traits<h5::blaze::colmat<T>> : std::true_type {};
		template <class T> struct detail::has_explicit_access_traits<h5::blaze::rowvec<T>> : std::true_type {};
		template <class T> struct detail::has_explicit_access_traits<h5::blaze::colvec<T>> : std::true_type {};
}
// Explicit storage_representation so h5::awrite's static_assert(storage != unsupported)
// always passes for vendored blaze types.
namespace h5::meta::detail_capabilities {
		template <class T> struct has_explicit_storage_repr<h5::blaze::rowmat<T>> : std::true_type {};
		template <class T> struct has_explicit_storage_repr<h5::blaze::colmat<T>> : std::true_type {};
		template <class T> struct has_explicit_storage_repr<h5::blaze::rowvec<T>> : std::true_type {};
		template <class T> struct has_explicit_storage_repr<h5::blaze::colvec<T>> : std::true_type {};
		template <class T> struct storage_representation_impl<h5::blaze::rowmat<T>>
			: std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
		template <class T> struct storage_representation_impl<h5::blaze::colmat<T>>
			: std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
		template <class T> struct storage_representation_impl<h5::blaze::rowvec<T>>
			: std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
		template <class T> struct storage_representation_impl<h5::blaze::colvec<T>>
			: std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
}

namespace h5::impl {
	// 1.) object -> H5T_xxx
	// Register Blaze types in has_explicit_decay (Blaze vectors/matrices have value_type).
	template <class T> struct detail::has_explicit_decay<h5::blaze::rowvec<T>> : std::true_type {};
	template <class T> struct detail::has_explicit_decay<h5::blaze::colvec<T>> : std::true_type {};
	template <class T> struct detail::has_explicit_decay<h5::blaze::rowmat<T>> : std::true_type {};
	template <class T> struct detail::has_explicit_decay<h5::blaze::colmat<T>> : std::true_type {};

	template <class T> struct decay<h5::blaze::rowvec<T>>{ using type = T; };
	template <class T> struct decay<h5::blaze::colvec<T>>{ using type = T; };
	template <class T> struct decay<h5::blaze::rowmat<T>>{ using type = T; };
	template <class T> struct decay<h5::blaze::colmat<T>>{ using type = T; };

	// get read access to datastaore
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::blaze::is_supported<Object>::value,
	const T*> data(const Object& ref ){
			return ref.data();
	}
	// read write access
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::blaze::is_supported<Object>::value,
	T*> data( Object& ref ){
			return ref.data();
	}
	// rank
	template<class T> struct rank<h5::blaze::rowvec<T>> : public std::integral_constant<size_t,1>{};
	template<class T> struct rank<h5::blaze::colvec<T>> : public std::integral_constant<size_t,1>{};
	template<class T> struct rank<h5::blaze::rowmat<T>> : public std::integral_constant<size_t,2>{};
	template<class T> struct rank<h5::blaze::colmat<T>> : public std::integral_constant<size_t,2>{};
	// determine rank and dimensions
	template <class T> inline std::array<size_t,1> size( const h5::blaze::rowvec<T>& ref ){ return {ref.size()};}
	template <class T> inline std::array<size_t,1> size( const h5::blaze::colvec<T>& ref ){ return {ref.size()};}
	// row-major blaze matrix stores rows-by-rows, so the HDF5 row-major shape is {rows, columns}.
	// column-major stores cols-by-cols, so it also presents {rows, columns} to HDF5 (the storage
	// order is invisible to size() — only the conceptual shape matters).
	template <class T> inline std::array<size_t,2> size( const h5::blaze::rowmat<T>& ref ){ return {ref.rows(),ref.columns()};}
	template <class T> inline std::array<size_t,2> size( const h5::blaze::colmat<T>& ref ){ return {ref.rows(),ref.columns()};}


	template <class T> struct get<h5::blaze::rowvec<T>> {
		static inline  h5::blaze::rowvec<T> ctor( std::array<size_t,1> dims ){
			return h5::blaze::rowvec<T>( dims[0] );
	}};
	template <class T> struct get<h5::blaze::colvec<T>> {
		static inline h5::blaze::colvec<T> ctor( std::array<size_t,1> dims ){
			return h5::blaze::colvec<T>( dims[0] );
	}};
	template <class T> struct get<h5::blaze::rowmat<T>> {
		// dims == {rows, columns} now that size() reports the rationalised order.
		static inline h5::blaze::rowmat<T> ctor( std::array<size_t,2> dims ){
			return h5::blaze::rowmat<T>( dims[0], dims[1] );
	}};
	template <class T> struct get<h5::blaze::colmat<T>> {
		static inline h5::blaze::colmat<T> ctor( std::array<size_t,2> dims ){
			return h5::blaze::colmat<T>( dims[0], dims[1] );
	}};
}

namespace h5::meta {
		template <class T> struct access_traits_t<h5::blaze::rowmat<T>> {
			using element_t = T;
			static constexpr access_t kind = access_t::contiguous;
			static constexpr bool is_trivially_packable = true;
			static auto data(const h5::blaze::rowmat<T>& c) noexcept { return h5::impl::data(c); }
			static auto data(h5::blaze::rowmat<T>& c)       noexcept { return h5::impl::data(c); }
			static auto size(const h5::blaze::rowmat<T>& c) noexcept { return h5::impl::size(c); }
			static std::size_t bytes(const h5::blaze::rowmat<T>& c) noexcept {
				auto s = size(c); std::size_t n = 1;
				for (std::size_t i = 0; i < s.size(); ++i) n *= s[i];
				return n * sizeof(element_t);
			}
		};
		template <class T> struct access_traits_t<h5::blaze::colmat<T>> {
			using element_t = T;
			static constexpr access_t kind = access_t::contiguous;
			static constexpr bool is_trivially_packable = true;
			static auto data(const h5::blaze::colmat<T>& c) noexcept { return h5::impl::data(c); }
			static auto data(h5::blaze::colmat<T>& c)       noexcept { return h5::impl::data(c); }
			static auto size(const h5::blaze::colmat<T>& c) noexcept { return h5::impl::size(c); }
			static std::size_t bytes(const h5::blaze::colmat<T>& c) noexcept {
				auto s = size(c); std::size_t n = 1;
				for (std::size_t i = 0; i < s.size(); ++i) n *= s[i];
				return n * sizeof(element_t);
			}
		};
		// Vectors: rowvec / colvec route through the same contiguous path. Without
		// these specs access_traits_t<DynamicVector<...>> falls through to the
		// primary template (no element_t) and dispatch fails to compile.
		template <class T> struct access_traits_t<h5::blaze::rowvec<T>> {
			using element_t = T;
			static constexpr access_t kind = access_t::contiguous;
			static constexpr bool is_trivially_packable = true;
			static auto data(const h5::blaze::rowvec<T>& c) noexcept { return h5::impl::data(c); }
			static auto data(h5::blaze::rowvec<T>& c)       noexcept { return h5::impl::data(c); }
			static auto size(const h5::blaze::rowvec<T>& c) noexcept { return h5::impl::size(c); }
			static std::size_t bytes(const h5::blaze::rowvec<T>& c) noexcept {
				auto s = size(c); std::size_t n = 1;
				for (std::size_t i = 0; i < s.size(); ++i) n *= s[i];
				return n * sizeof(element_t);
			}
		};
		template <class T> struct access_traits_t<h5::blaze::colvec<T>> {
			using element_t = T;
			static constexpr access_t kind = access_t::contiguous;
			static constexpr bool is_trivially_packable = true;
			static auto data(const h5::blaze::colvec<T>& c) noexcept { return h5::impl::data(c); }
			static auto data(h5::blaze::colvec<T>& c)       noexcept { return h5::impl::data(c); }
			static auto size(const h5::blaze::colvec<T>& c) noexcept { return h5::impl::size(c); }
			static std::size_t bytes(const h5::blaze::colvec<T>& c) noexcept {
				auto s = size(c); std::size_t n = 1;
				for (std::size_t i = 0; i < s.size(); ++i) n *= s[i];
				return n * sizeof(element_t);
			}
		};
}

#endif
