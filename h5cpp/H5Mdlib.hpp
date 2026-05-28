/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include <hdf5.h>

#if defined(DLIB_MATRIx_HEADER) || defined(H5CPP_USE_DLIB)
namespace h5::dlib {
// dlib template:
// const dlib::matrix<short int, 0, 0, dlib::memory_manager_stateless_kernel_1<char>, dlib::row_major_layout>&
		template<class T> using rowmat = ::dlib::matrix<T, 0, 0,
			::dlib::memory_manager_stateless_kernel_1<char>,
			::dlib::row_major_layout>;
		template <class Object, class T = typename impl::decay<Object>::type>
			using is_supported = std::bool_constant<std::is_same_v<Object,h5::dlib::rowmat<T>>>;
}
namespace h5::meta {
		template <class T> struct is_contiguous<h5::dlib::rowmat<T>> : std::true_type {};

		// Register types so generic access_traits_t fallbacks don't create ambiguous partial specializations
		template <class T> struct detail::has_explicit_access_traits<h5::dlib::rowmat<T>> : std::true_type {};
}
// Explicit storage_representation so h5::awrite's static_assert(storage != unsupported)
// always passes for vendored dlib matrices.
namespace h5::meta::detail_capabilities {
		template <class T> struct has_explicit_storage_repr<h5::dlib::rowmat<T>> : std::true_type {};
		template <class T> struct storage_representation_impl<h5::dlib::rowmat<T>>
			: std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
}

namespace h5::impl {
	// 1.) object -> H5T_xxx
	template <class T> struct detail::has_explicit_decay<h5::dlib::rowmat<T>> : std::true_type {};
	template <class T> struct decay<h5::dlib::rowmat<T>>{ using type = T; };

	// get read access to datastaore
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::dlib::is_supported<Object>::value,
	const T*> data(const Object& ref ){
			return &ref(0,0);
	}
	// read write access
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::dlib::is_supported<Object>::value,
	T*> data( Object& ref ){
			return &ref(0,0);
	}
	
	template<class T> struct rank<h5::dlib::rowmat<T>> : public std::integral_constant<size_t,2>{};
	template<class T>
	inline std::array<size_t,2> size( const dlib::rowmat<T>& ref ){
				return { (hsize_t)ref.nc(),(hsize_t)ref.nr()};
	}
	
	template <class T> struct get<h5::dlib::rowmat<T>> {
		static inline h5::dlib::rowmat<T> ctor( std::array<size_t,2> dims ){
			return h5::dlib::rowmat<T>( dims[1], dims[0] );
	}};
}

namespace h5::meta {
		template <class T> struct access_traits_t<h5::dlib::rowmat<T>> {
			using element_t = T;
			static constexpr access_t kind = access_t::contiguous;
			static constexpr bool is_trivially_packable = true;
			static auto data(const h5::dlib::rowmat<T>& c) noexcept { return h5::impl::data(c); }
			static auto data(h5::dlib::rowmat<T>& c)       noexcept { return h5::impl::data(c); }
			static auto size(const h5::dlib::rowmat<T>& c) noexcept { return h5::impl::size(c); }
			static std::size_t bytes(const h5::dlib::rowmat<T>& c) noexcept {
				auto s = size(c); std::size_t n = 1;
				for (std::size_t i = 0; i < s.size(); ++i) n *= s[i];
				return n * sizeof(element_t);
			}
		};
}
#endif
