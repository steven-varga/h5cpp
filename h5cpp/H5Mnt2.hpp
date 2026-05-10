/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

#if defined(NT2_TABLE_HPP_INCLUDED) || defined(H5CPP_USE_NT2)
namespace h5::nt2 {
		template<class T> struct table1d : ::nt2::table<T> {
			using ::nt2::table<T>::table;
		};
		template<class T> struct table2d : ::nt2::table<T> {
			using ::nt2::table<T>::table;
		};

		// is_linalg_type := filter
		template <class Object, class T = typename impl::decay<Object>::type> using is_supported =
		std::bool_constant<std::is_same_v<Object,h5::nt2::table1d<T>> || std::is_same_v<Object,h5::nt2::table2d<T>>>;
}

namespace h5::meta {
    template <class T> struct is_contiguous<h5::nt2::table1d<T>> : std::true_type {};
    template <class T> struct is_contiguous<h5::nt2::table2d<T>> : std::true_type {};
}

namespace h5::impl {
	// 1.) object -> H5T_xxx
	template <class T> struct decay<h5::nt2::table1d<T>>{ using type = T; };
	template <class T> struct decay<h5::nt2::table2d<T>>{ using type = T; };

	// get read access to datastore
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::nt2::is_supported<Object>::value,
	const T*> data( const Object& ref ){
			return ref.data();
	}

	// read write access
	template <class Object, class T = typename impl::decay<Object>::type> inline
	std::enable_if_t< h5::nt2::is_supported<Object>::value,
	T*> data( Object& ref ){
			return ref.data();
	}

	// rank
	template<class T> struct rank<h5::nt2::table1d<T>> : public std::integral_constant<size_t,1>{};
	template<class T> struct rank<h5::nt2::table2d<T>> : public std::integral_constant<size_t,2>{};

	// determine rank and dimensions
	template <class T> inline std::array<size_t,1> size( const h5::nt2::table1d<T>& ref ){ return {ref.extent()[0]};}
	template <class T> inline std::array<size_t,2> size( const h5::nt2::table2d<T>& ref ){ return {ref.extent()[1],ref.extent()[0]};}

	// CTOR-s 
	template <class T> struct get<h5::nt2::table1d<T>> {
		static inline  h5::nt2::table1d<T> ctor( std::array<size_t,1> dims ){
			return h5::nt2::table1d<T>( nt2::of_size(dims[0]) );
	}};
	template <class T> struct get<h5::nt2::table2d<T>> {
		static inline h5::nt2::table2d<T> ctor( std::array<size_t,2> dims ){
			return h5::nt2::table2d<T>( nt2::of_size(dims[1], dims[0]) );
	}};
}
#endif
