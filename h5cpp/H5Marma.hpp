
/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_ARMA_HPP 
#define  H5CPP_ARMA_HPP

namespace h5 { namespace impl {
#if defined(ARMA_INCLUDES) || defined(H5CPP_USE_ARMADILLO)
	// 1.) object -> H5T_xxx
	template <class T> struct decay<arma::Mat<T>>{ typedef T type; };
	template <class T> struct decay<arma::Col<T>>{ typedef T type; };
	template <class T> struct decay<arma::Row<T>>{ typedef T type; };
	template <class T> struct decay<arma::Cube<T>>{ typedef T type; };
	// is_linalg_type := filter
	template <class Arma, class T = typename impl::decay<Arma>::type> using is_arma_type =
		std::integral_constant<bool, std::is_same<Arma,arma::Cube<T>>::value || std::is_same<Arma,arma::Mat<T>>::value
			|| std::is_same<Arma,arma::Col<T>>::value ||  std::is_same<Arma,arma::Row<T>>::value>;

	// get read access to datastaore
	template <class Arma, class T = typename impl::decay<Arma>::type> inline
	typename std::enable_if< is_arma_type<Arma>::value,
	const T*>::type data(const Arma& ref ){
			return ref.memptr();
	}
	// read write access
	template <class Arma, class T = typename impl::decay<Arma>::type> inline
	typename std::enable_if< is_arma_type<Arma>::value,
	T*>::type data( Arma& ref ){
			return ref.memptr();
	}
	// rank
	template<class T> struct rank<arma::Row<T>> : public std::integral_constant<size_t,1>{};
	template<class T> struct rank<arma::Col<T>> : public std::integral_constant<size_t,1>{};
	template<class T> struct rank<arma::Mat<T>> : public std::integral_constant<size_t,2>{};
	template<class T> struct rank<arma::Cube<T>>: public std::integral_constant<size_t,3>{};
	// determine rank and dimensions
	template <class T> inline std::array<size_t,1> size( const arma::Row<T>& ref ){ return {ref.n_elem};}
	template <class T> inline std::array<size_t,1> size( const arma::Col<T>& ref ){ return {ref.n_elem};}
	template <class T> inline std::array<size_t,2> size( const arma::Mat<T>& ref ){ return {ref.n_cols,ref.n_rows};}
	template <class T> inline std::array<size_t,3> size( const arma::Cube<T>& ref){ return {ref.n_slices,ref.n_cols,ref.n_rows};}


	template <class T> struct get<arma::Row<T>> {
		static inline  arma::Row<T> ctor( std::array<size_t,1> dims ){
			return arma::Row<T>( dims[0] );
	}};
	template <class T> struct get<arma::Col<T>> {
		static inline  arma::Col<T> ctor( std::array<size_t,1> dims ){
			return arma::Col<T>( dims[0] );
	}};
	template <class T> struct get<arma::Mat<T>> {
		static inline  arma::Mat<T> ctor( std::array<size_t,2> dims ){
			return arma::Mat<T>( dims[0], dims[1] );
	}};
	template <class T> struct get<arma::Cube<T>> {
		static inline  arma::Cube<T> ctor( std::array<size_t,3> dims ){
			return arma::Cube<T>( dims[2], dims[0], dims[1] );
	}};
#else
	template <class Arma> using is_arma_type = std::integral_constant<bool,false>;
#endif
}}
#endif
