
/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_ITPLUS_HPP 
#define  H5CPP_ITPLUS_HPP

#if defined(DLIB_MATRIx_HEADER) || defined(H5CPP_USE_DLIB)
namespace h5 { 	namespace dlib {
		template<class T> using rowmat = ::dlib::matrix<T>;
		template <class Object, class T = typename impl::decay<Object>::type>
			using is_supported = std::integral_constant<bool, std::is_same<Object,h5::dlib::rowmat<T>>::value>;
}}
namespace h5 { namespace impl {
	// 1.) object -> H5T_xxx
	template <class T> struct decay<h5::dlib::rowmat<T>>{ typedef T type; };

	// get read access to datastaore
	template <class Object, class T = typename impl::decay<Object>::type> inline
	typename std::enable_if< h5::dlib::is_supported<Object>::value,
	const T*>::type data(const Object& ref ){
			return &ref.(0,0);
	}
	// read write access
	template <class Object, class T = typename impl::decay<Object>::type> inline
	typename std::enable_if< h5::dlib::is_supported<Object>::value,
	T*>::type data( Object& ref ){
			return &ref.(0,0);
	}
	template<class T> struct rank<h5::dlib::rowmat<T>> : public std::integral_constant<size_t,2>{};
	template <class T> inline std::array<size_t,2> size( const h5::dlib::rowmat<T>& ref ){ return {(hsize_t)ref.nc(),(hsize_t)ref.nr()};}
	template <class T> struct get<h5::dlib::rowmat<T>> {
		static inline h5::dlib::rowmat<T> ctor( std::array<size_t,2> dims ){
			return h5::dlib::rowmat<T>( dims[1], dims[0] );
	}};
}}
#endif
#endif
