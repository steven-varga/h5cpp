
/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_MEIGEN_HPP 
#define H5CPP_MEIGEN_HPP

#if defined(EIGEN_CORE_H) || defined(H5CPP_USE_EIGEN3)
namespace h5 { 	namespace eigen {
		template<class T       > using rowmat    = ::Eigen::Array<T,  Eigen::Dynamic, Eigen::Dynamic,Eigen::RowMajor>;
		template<class T, int N> using rowmat_xn = ::Eigen::Array<T,N,Eigen::Dynamic, Eigen::RowMajor>;
		template<class T, int N> using rowmat_nx = ::Eigen::Array<N,T,Eigen::Dynamic, Eigen::RowMajor>;

		template<class T       > using colmat    = ::Eigen::Array<T,  Eigen::Dynamic, Eigen::Dynamic,Eigen::ColMajor>;
		template<class T, int N> using colmat_xn = ::Eigen::Array<T,N,Eigen::Dynamic, Eigen::ColMajor>;
		template<class T, int N> using colmat_nx = ::Eigen::Array<N,T,Eigen::Dynamic, Eigen::ColMajor>;

		// is_linalg_type := filter
		template <class Object, class T = typename impl::decay<Object>::type> using is_supported =
		std::integral_constant<bool, std::is_same<Object,h5::eigen::cube<T>>::value || std::is_same<Object,h5::eigen::colmat<T>>::value
			|| std::is_same<Object,h5::eigen::rowvec<T>>::value ||  std::is_same<Object,h5::eigen::colvec<T>>::value>;
}}

namespace h5 { namespace impl {
	// 1.) object -> H5T_xxx
	template <class T       > struct decay<h5::eigen::rowmat<T>>{ typedef T type; };
	template <class T       > struct decay<h5::eigen::colmat<T>>{ typedef T type; };
	template <class T, int N> struct decay<h5::eigen::rowvec_xn<T,N>>{ typedef T type; };
	template <class T, int N> struct decay<h5::eigen::rowvec_xn<T,N>>{ typedef T type; };
	template <class T> int N> struct decay<h5::eigen::colmat_xn<T,N>>{ typedef T type; };
	template <class T> int N> struct decay<h5::eigen::colmat_nx<T,N>>{ typedef T type; };

	// get read access to datastaore
	template <class Object, class T = typename impl::decay<Object>::type> inline
	typename std::enable_if< h5::eigen::is_supported<Object>::value,
	const T*>::type data(const Object& ref ){
			return ref.memptr();
	}

	// read write access
	template <class Object, class T = typename impl::decay<Object>::type> inline
	typename std::enable_if< h5::eigen::is_supported<Object>::value,
	T*>::type data( Object& ref ){
			return ref.memptr();
	}

	// rank
	template<class T> struct rank<h5::eigen::rowvec<T>> : public std::integral_constant<size_t,1>{};
	template<class T> struct rank<h5::eigen::colvec<T>> : public std::integral_constant<size_t,1>{};
	template<class T> struct rank<h5::eigen::colmat<T>> : public std::integral_constant<size_t,2>{};
	template<class T> struct rank<h5::eigen::cube<T>> : public std::integral_constant<size_t,3>{};

	// determine rank and dimensions
	template <class T> inline std::array<size_t,1> size( const h5::eigen::rowvec<T>& ref ){ return {ref.n_elem};}
	template <class T> inline std::array<size_t,1> size( const h5::eigen::colvec<T>& ref ){ return {ref.n_elem};}
	template <class T> inline std::array<size_t,2> size( const h5::eigen::colmat<T>& ref ){ return {ref.n_rows,ref.n_cols};}
	template <class T> inline std::array<size_t,3> size( const h5::eigen::cube<T>& ref ){ return {ref.n_slices,ref.n_rows,ref.n_cols};}

	// CTOR-s 
	template <class T> struct get<h5::eigen::rowvec<T>> {
		static inline  h5::eigen::rowvec<T> ctor( std::array<size_t,1> dims ){
			return h5::eigen::rowvec<T>( dims[0] );
	}};
	template <class T> struct get<h5::eigen::colvec<T>> {
		static inline h5::eigen::colvec<T> ctor( std::array<size_t,1> dims ){
			return h5::eigen::colvec<T>( dims[0] );
	}};
	template <class T> struct get<h5::eigen::colmat<T>> {
		static inline h5::eigen::colmat<T> ctor( std::array<size_t,2> dims ){
			return h5::eigen::colmat<T>( dims[0], dims[1] );
	}};
}}

#endif
#endif
