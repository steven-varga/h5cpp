
/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_BLITZ_HPP 
#define  H5CPP_BLITZ_HPP
/**************************************************************************************************************************************/
/* BLITZ                                                                                                                              */
/**************************************************************************************************************************************/
#if defined(BZ_ARRAY_ONLY_H) || defined(H5CPP_USE_BLITZ)

namespace h5 { 	namespace blitz {
		template<class T> using rowvec = ::blitz::Array<T,1>;
		template<class T> using colvec = ::blitz::Array<T,1>;
		template<class T> using rowmat = ::blitz::Array<T,2>;
		template<class T> using colmat = ::blitz::Array<T,2>;
		template<class T> using cube   = ::blitz::Array<T,3>;

		// is_linalg_type := filter
		template <class Object, class T = typename impl::decay<Object>::type> using is_supported =
		std::integral_constant<bool, std::is_same<Object,rowmat<T>>::value || std::is_same<Object,colmat<T>>::value
			|| std::is_same<Object,rowvec<T>>::value ||  std::is_same<Object,colvec<T>>::value>;
}}
namespace h5 { namespace impl {
	// 1.) object -> H5T_xxx
	template <class T> struct decay<h5::blitz::rowvec<T>>{ typedef T type; };
	template <class T> struct decay<h5::blitz::rowmat<T>>{ typedef T type; };

	// get read access to datastaore
	template <class Object, class T = typename impl::decay<Object>::type> inline
	typename std::enable_if< h5::blitz::is_supported<Object>::value,
	const T*>::type data(const Object& ref ){
			return ref.data();
	}
	// read write access
	template <class Object, class T = typename impl::decay<Object>::type> inline
	typename std::enable_if< h5::blitz::is_supported<Object>::value,
	T*>::type data( Object& ref ){
			return ref.data();
	}
	// rank
	template<class T> struct rank<h5::blitz::rowvec<T>> : public std::integral_constant<size_t,1>{};
	template<class T> struct rank<h5::blitz::rowmat<T>> : public std::integral_constant<size_t,2>{};
	template<class T> struct rank<h5::blitz::cube<T>> : public std::integral_constant<size_t,3>{};
	// determine rank and dimensions
	template <class T> inline std::array<size_t,1> size( const h5::blitz::rowvec<T>& ref ){ return {(hsize_t) ref.size()};}
	template <class T> inline std::array<size_t,2> size( const h5::blitz::rowmat<T>& ref ){ return {(hsize_t) ref.cols(), (hsize_t) ref.rows()};}
	template <class T> inline std::array<size_t,3> size( const h5::blitz::cube<T>& ref ){ return {(hsize_t)ref.depth(),(hsize_t)ref.cols(),(hsize_t)ref.rows()};}


	template <class T> struct get<h5::blitz::rowvec<T>> {
		static inline  h5::blitz::rowvec<T> ctor( std::array<size_t,1> dims ){
			return h5::blitz::rowvec<T>( dims[0] );
	}};
	template <class T> struct get<h5::blitz::rowmat<T>> {
		static inline h5::blitz::rowmat<T> ctor( std::array<size_t,2> dims ){
			return h5::blitz::rowmat<T>( dims[1], dims[0] );
	}};
	template <class T> struct get<h5::blitz::cube<T>> {
		static inline h5::blitz::cube<T> ctor( std::array<size_t,3> dims ){
			return h5::blitz::cube<T>(dims[2], dims[1], dims[0]);
	}};

}}


#endif
#endif
