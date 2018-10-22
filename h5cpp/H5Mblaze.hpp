
/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_BLAZE_HPP 
#define  H5CPP_BLAZE_HPP

#if defined(_BLAZE_MATH_MODULE_H_) || defined(H5CPP_USE_BLAZE)
namespace h5 { 	namespace blaze {
		template<class T> using rowvec = ::blaze::DynamicVector<T,::blaze::rowVector>;
		template<class T> using colvec = ::blaze::DynamicVector<T,::blaze::columnVector>;
		template<class T> using rowmat = ::blaze::DynamicMatrix<T,::blaze::rowMajor>;
		template<class T> using colmat = ::blaze::DynamicMatrix<T,::blaze::columnMajor>;

		// is_linalg_type := filter
		template <class Object, class T = typename impl::decay<Object>::type> using is_supported =
		std::integral_constant<bool, std::is_same<Object,rowmat<T>>::value || std::is_same<Object,colmat<T>>::value
			|| std::is_same<Object,rowvec<T>>::value ||  std::is_same<Object,colvec<T>>::value>;
}}

namespace h5 { namespace impl {
	// 1.) object -> H5T_xxx
	template <class T> struct decay<h5::blaze::rowvec<T>>{ typedef T type; };
	template <class T> struct decay<h5::blaze::colvec<T>>{ typedef T type; };
	template <class T> struct decay<h5::blaze::rowmat<T>>{ typedef T type; };
	template <class T> struct decay<h5::blaze::colmat<T>>{ typedef T type; };

	// get read access to datastaore
	template <class Object, class T = typename impl::decay<Object>::type> inline
	typename std::enable_if< h5::blaze::is_supported<Object>::value,
	const T*>::type data(const Object& ref ){
			return ref.data();
	}
	// read write access
	template <class Object, class T = typename impl::decay<Object>::type> inline
	typename std::enable_if< h5::blaze::is_supported<Object>::value,
	T*>::type data( Object& ref ){
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
	template <class T> inline std::array<size_t,2> size( const h5::blaze::rowmat<T>& ref ){ return {ref.columns(),ref.rows()};}
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
		static inline h5::blaze::rowmat<T> ctor( std::array<size_t,2> dims ){
			return h5::blaze::rowmat<T>( dims[1], dims[0] );
	}};
	template <class T> struct get<h5::blaze::colmat<T>> {
		static inline h5::blaze::colmat<T> ctor( std::array<size_t,2> dims ){
			return h5::blaze::colmat<T>( dims[0], dims[1] );
	}};
}}

#endif
#endif
