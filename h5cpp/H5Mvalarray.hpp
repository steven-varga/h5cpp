/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

#include <valarray>

namespace h5::valarray {
		template<class T> using array = ::std::valarray<T>;
		template <class Object, class T = typename impl::decay<Object>::type>
			using is_supported = std::bool_constant<std::is_same_v<Object,h5::valarray::array<T>>>;
}

namespace h5::meta {
    template <class T> struct is_contiguous<h5::valarray::array<T>> : std::true_type {};

    template <class T> struct detail::has_explicit_access_traits<h5::valarray::array<T>> : std::true_type {};
}

// Explicit storage_representation: rank-1 contiguous → linear_value_dataset.
// Without this, std::valarray falls through to 'unsupported' and the dispatch
// static_assert fires (mirrors the arma/eigen/blitz/... mappers).
namespace h5::meta::detail_capabilities {
    template <class T> struct has_explicit_storage_repr<h5::valarray::array<T>> : std::true_type {};
    template <class T> struct storage_representation_impl<h5::valarray::array<T>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
}

namespace h5::impl {
	// std::valarray carries value_type, so it must be registered to suppress the
	// catch-all SFINAE decay<T, has_value_type<T>...> in H5Mstl.hpp; otherwise
	// decay<std::valarray<T>> has two viable specialisations and instantiation
	// is ambiguous.
	template <class T> struct detail::has_explicit_decay<h5::valarray::array<T>> : std::true_type {};

	// 1.) object -> H5T_xxx
	template <class T> struct decay<h5::valarray::array<T>>{ using type = T; };
	// get read access to datastaore
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::valarray::is_supported<Object>::value,
	const T*> data(const Object& ref ){
			return std::begin(ref);
	}
	// read write access
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::valarray::is_supported<Object>::value,
	T*> data( Object& ref ){
			return std::begin(ref);
	}
	template<class T> struct rank<h5::valarray::array<T>> : public std::integral_constant<size_t,1>{};
	template <class T> inline std::array<size_t,1> size( const h5::valarray::array<T>& ref ){ return { (hsize_t)ref.size() };}
	template <class T> struct get<h5::valarray::array<T>> {
		static inline h5::valarray::array<T> ctor( std::array<size_t,1> dims ){
			return h5::valarray::array<T>( dims[0] );
	}};
}

namespace h5::meta {
	template <class T> struct access_traits_t<h5::valarray::array<T>> {
		using element_t = T;
		static constexpr access_t kind = access_t::contiguous;
		static constexpr bool is_trivially_packable = true;
		static auto data(const h5::valarray::array<T>& c) noexcept { return h5::impl::data(c); }
		static auto data(h5::valarray::array<T>& c)       noexcept { return h5::impl::data(c); }
		static auto size(const h5::valarray::array<T>& c) noexcept { return h5::impl::size(c); }
		static std::size_t bytes(const h5::valarray::array<T>& c) noexcept {
			auto s = size(c); std::size_t n = 1;
			for (std::size_t i = 0; i < s.size(); ++i) n *= s[i];
			return n * sizeof(element_t);
		}
	};
}
