
/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_ARMA_HPP 
#define  H5CPP_ARMA_HPP

#if defined(ARMA_INCLUDES) || defined(H5CPP_USE_ARMADILLO)
namespace h5 { 	namespace arma {
		template<class T> using rowvec = ::arma::Row<T>;
		template<class T> using colvec = ::arma::Col<T>;
		template<class T> using colmat = ::arma::Mat<T>;
		template<class T> using cube   = ::arma::Cube<T>;

		// is_linalg_type := filter
		template <class Object, class T = typename impl::decay<Object>::type> using is_supported =
		std::integral_constant<bool, std::is_same<Object,h5::arma::cube<T>>::value || std::is_same<Object,h5::arma::colmat<T>>::value
			|| std::is_same<Object,h5::arma::rowvec<T>>::value ||  std::is_same<Object,h5::arma::colvec<T>>::value>;
}}

namespace h5 { namespace impl {
	// 1.) object -> H5T_xxx

	template <class T> struct decay<h5::arma::rowvec<T>>{ typedef T type; };
	template <class T> struct decay<h5::arma::colvec<T>>{ typedef T type; };
	template <class T> struct decay<h5::arma::colmat<T>>{ typedef T type; };
	template <class T> struct decay<h5::arma::cube<T>>{ typedef T type; };

	// get read access to datastaore
	template <class Object, class T = typename impl::decay<Object>::type> inline
	typename std::enable_if< h5::arma::is_supported<Object>::value,
	const T*>::type data( const Object& ref ){
			return ref.memptr();
	}

	// read write access
	template <class Object, class T = typename impl::decay<Object>::type> inline
	typename std::enable_if< h5::arma::is_supported<Object>::value,
	T*>::type data( Object& ref ){
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
	template <class T> inline std::array<size_t,2> size( const h5::arma::colmat<T>& ref ){ return {ref.n_cols,ref.n_rows};}
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
		static inline h5::arma::colmat<T> ctor( std::array<size_t,3> dims ){
			return h5::arma::colmat<T>( dims[2], dims[0], dims[1] );
	}};
}}
#endif
#endif
