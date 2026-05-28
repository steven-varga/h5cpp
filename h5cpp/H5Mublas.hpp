/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include <hdf5.h>

#if defined(_BOOST_UBLAS_MATRIX_) || defined(H5CPP_USE_UBLAS_MATRIX)
namespace h5::ublas {
		template<class T> using rowmat 	= ::boost::numeric::ublas::matrix<T>;
		template <class Object, class T = typename impl::decay<Object>::type>
			using is_supportedm = std::bool_constant<std::is_same_v<Object,h5::ublas::rowmat<T>>>;
}
namespace h5::meta {
		template <class T> struct is_contiguous<h5::ublas::rowmat<T>> : std::true_type {};

		// Register types so generic access_traits_t fallbacks don't create ambiguous partial specializations
		template <class T> struct detail::has_explicit_access_traits<h5::ublas::rowmat<T>> : std::true_type {};
}
// ublas::matrix doesn't have plain begin()/end() (uses begin1/end1), so the
// structural is_sequential_like fallback won't reach it. Register an explicit
// storage_representation so h5::awrite's static_assert(storage != unsupported)
// passes and dispatch routes through the contiguous (linear_value_dataset) path.
namespace h5::meta::detail_capabilities {
		template <class T> struct has_explicit_storage_repr<h5::ublas::rowmat<T>> : std::true_type {};
		template <class T> struct storage_representation_impl<h5::ublas::rowmat<T>>
			: std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
}
namespace h5::impl {
	// 1.) object -> H5T_xxx
	template <class T> struct detail::has_explicit_decay<h5::ublas::rowmat<T>> : std::true_type {};
	template <class T> struct decay<h5::ublas::rowmat<T>>{ using type = T; };
	// get read access to datastaore
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::ublas::is_supportedm<Object>::value,
	const T*> data(const Object& ref ){
			return ref.data().begin();
	}
	// read write access
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::ublas::is_supportedm<Object>::value,
	T*> data( Object& ref ){
			return ref.data().begin();
	}
	template<class T> struct rank<h5::ublas::rowmat<T>> : public std::integral_constant<size_t,2>{};
	template <class T> inline std::array<size_t,2> size( const h5::ublas::rowmat<T>& ref ){ return {(hsize_t)ref.size2(),(hsize_t)ref.size1()};}
	template <class T> struct get<h5::ublas::rowmat<T>> {
		static inline h5::ublas::rowmat<T> ctor( std::array<size_t,2> dims ){
			return h5::ublas::rowmat<T>( dims[1], dims[0] );
	}};
}
namespace h5::meta {
		template <class T> struct access_traits_t<h5::ublas::rowmat<T>> {
			using element_t = T;
			static constexpr access_t kind = access_t::contiguous;
			static constexpr bool is_trivially_packable = true;
			static auto data(const h5::ublas::rowmat<T>& c) noexcept { return h5::impl::data(c); }
			static auto data(h5::ublas::rowmat<T>& c)       noexcept { return h5::impl::data(c); }
			static auto size(const h5::ublas::rowmat<T>& c) noexcept { return h5::impl::size(c); }
			static std::size_t bytes(const h5::ublas::rowmat<T>& c) noexcept {
				auto s = size(c); std::size_t n = 1;
				for (std::size_t i = 0; i < s.size(); ++i) n *= s[i];
				return n * sizeof(element_t);
			}
		};
}
#endif

#if defined(_BOOST_UBLAS_VECTOR_) || defined(H5CPP_USE_UBLAS_VECTOR)
namespace h5::ublas {
		template<class T> using rowvec = ::boost::numeric::ublas::vector<T>;
		template <class Object, class T = typename impl::decay<Object>::type>
			using is_supportedv = std::bool_constant<std::is_same_v<Object,h5::ublas::rowvec<T>>>;
}
namespace h5::meta {
		template <class T> struct is_contiguous<h5::ublas::rowvec<T>> : std::true_type {};

		// Register types so generic access_traits_t fallbacks don't create ambiguous partial specializations
		template <class T> struct detail::has_explicit_access_traits<h5::ublas::rowvec<T>> : std::true_type {};
}
namespace h5::meta::detail_capabilities {
		template <class T> struct has_explicit_storage_repr<h5::ublas::rowvec<T>> : std::true_type {};
		template <class T> struct storage_representation_impl<h5::ublas::rowvec<T>>
			: std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
}
namespace h5::impl {
	template <class T> struct detail::has_explicit_decay<h5::ublas::rowvec<T>> : std::true_type {};
	template <class T> struct decay<h5::ublas::rowvec<T>>{ using type = T; };
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::ublas::is_supportedv<Object>::value,
	const T*> data(const Object& ref ){
			return ref.data().begin();
	}
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::ublas::is_supportedv<Object>::value,
	T*> data( Object& ref ){
			return ref.data().begin();
	}
	template<class T> struct rank<h5::ublas::rowvec<T>> : public std::integral_constant<size_t,1>{};
	template <class T> inline std::array<size_t,1> size( const h5::ublas::rowvec<T>& ref ){ return { (hsize_t)ref.size() };}
	template <class T> struct get<h5::ublas::rowvec<T>> {
		static inline h5::ublas::rowvec<T> ctor( std::array<size_t,1> dims ){
			return h5::ublas::rowvec<T>( dims[0] );
	}};
}
namespace h5::meta {
		template <class T> struct access_traits_t<h5::ublas::rowvec<T>> {
			using element_t = T;
			static constexpr access_t kind = access_t::contiguous;
			static constexpr bool is_trivially_packable = true;
			static auto data(const h5::ublas::rowvec<T>& c) noexcept { return h5::impl::data(c); }
			static auto data(h5::ublas::rowvec<T>& c)       noexcept { return h5::impl::data(c); }
			static auto size(const h5::ublas::rowvec<T>& c) noexcept { return h5::impl::size(c); }
			static std::size_t bytes(const h5::ublas::rowvec<T>& c) noexcept {
				auto s = size(c); std::size_t n = 1;
				for (std::size_t i = 0; i < s.size(); ++i) n *= s[i];
				return n * sizeof(element_t);
			}
		};
}
#endif
