
/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_EIGEN_HPP 
#define H5CPP_EIGEN_HPP

#if defined(EIGEN_CORE_H) || defined(H5CPP_USE_EIGEN3)
/*
	Matrix<typename Scalar,
       int RowsAtCompileTime,
       int ColsAtCompileTime,
       int Options = 0,          // ColMajor | RowMajor
       int MaxRowsAtCompileTime = RowsAtCompileTime,
       int MaxColsAtCompileTime = ColsAtCompileTime>
*/
namespace h5::impl {
}

namespace h5 { namespace impl {
	// 1.) object -> H5T_xxx
	template<class T,int R,int C, int O> struct decay<::Eigen::Matrix<T,R,C,O>>{ typedef T type; };
	template<class T,int R,int C, int O> struct decay<::Eigen::Array<T,R,C,O>>{ typedef T type; };
	// TODO: remove const_cast
	// get read access to datastaore
	template<class T,int R,int C,int O, int MR=R,int MC=C>
	T* data(const ::Eigen::Matrix<T,R,C,O,MR,MC>& ref ){
			return const_cast<T*>( ref.data() );
	}
	template<class T,int R,int C,int O, int MR=R,int MC=C>
	T* data(const ::Eigen::Array<T,R,C,O,MR,MC>& ref ){
			return const_cast<T*>( ref.data() );
	}
	// determine rank and dimensions
	// MATRICES
	template<class T,int R,int C,int MR=R,int MC=C>
	inline std::array<size_t,2> size( const ::Eigen::Matrix<T,R,C,::Eigen::RowMajor,MR,MC>& ref ){
		return {(hsize_t)ref.rows(),(hsize_t)ref.cols()};
	}
	template<class T,int R,int C,int MR=R,int MC=C>
	inline std::array<size_t,2> size( const ::Eigen::Matrix<T,R,C,::Eigen::ColMajor,MR,MC>& ref ){
		return {(hsize_t)ref.cols(), (hsize_t)ref.rows()};
	}
	// ARRAYS
	template<class T,int R,int C,int MR=R,int MC=C>
	inline std::array<size_t,2> size( const ::Eigen::Array<T,R,C,::Eigen::RowMajor,MR,MC>& ref ){
		return {(hsize_t)ref.rows(),(hsize_t)ref.cols()};
	}

	template<class T,int R,int C,int MR=R,int MC=C>
	inline std::array<size_t,2> size( const ::Eigen::Array<T,R,C,::Eigen::ColMajor,MR,MC>& ref ){
		return {(hsize_t)ref.cols(), (hsize_t)ref.rows()};
	}
	// RANK: define the exceptions, then the general case
	template <class T,int Major> // scalar
	   	struct rank<Eigen::Matrix<T, 1,1, Major>> : public std::integral_constant<int,0> {};
	template <class T,int Major> // scalar
	   	struct rank<Eigen::Array<T, 1,1, Major>> : public std::integral_constant<int,0> {};

	template <class T,int N, int Major> // row vector
	   	struct rank<Eigen::Matrix<T, 1,N, Major>> : public std::integral_constant<int,1> {};
	template <class T,int M, int Major> // col vector
	   	struct rank<Eigen::Matrix<T, M,1, Major>> : public std::integral_constant<int,1> {};
	template <class T,int M, int N, int Major> // matrices
	   	struct rank<Eigen::Matrix<T, M, N, Major>> : public std::integral_constant<int,2> {};
	template <class T,int N, int Major> // row vector
	   	struct rank<Eigen::Array<T, 1,N, Major>> : public std::integral_constant<int,1> {};
	template <class T,int M, int Major> // col vector
	   	struct rank<Eigen::Array<T, M,1, Major>> : public std::integral_constant<int,1> {};
	template <class T,int M, int N, int Major> // matrices
	   	struct rank<Eigen::Array<T, M, N, Major>> : public std::integral_constant<int,2> {};

	// DENSE-SPARSE


	// CTOR-s
	// MATRICES
	template<class T,int R,int C>
	struct get<::Eigen::Matrix<T,R,C,::Eigen::RowMajor>> {
		static inline ::Eigen::Matrix<T,R,C,::Eigen::RowMajor> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Matrix<T,R,C,::Eigen::RowMajor>( dims[0], dims[1] );
	}};
	template<class T,int R,int C, int MR, int MC>
	struct get<::Eigen::Matrix<T,R,C,::Eigen::RowMajor,MR,MC>> {
		static inline ::Eigen::Matrix<T,R,C,::Eigen::RowMajor,MR,MC> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Matrix<T,R,C,::Eigen::RowMajor,MR,MC>( dims[0], dims[1] );
	}};
	template<class T,int R,int C>
	struct get<::Eigen::Matrix<T,R,C,::Eigen::ColMajor>> {
		static inline ::Eigen::Matrix<T,R,C,::Eigen::ColMajor> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Matrix<T,R,C,::Eigen::ColMajor>( dims[1], dims[0] );
	}};
	template<class T,int R,int C, int MR, int MC>
	struct get<::Eigen::Matrix<T,R,C,::Eigen::ColMajor,MR,MC>> {
		static inline ::Eigen::Matrix<T,R,C,::Eigen::ColMajor,MR,MC> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Matrix<T,R,C,::Eigen::ColMajor,MR,MC>( dims[1], dims[0] );
	}};
	// ARRAYS
	template<class T,int R,int C>
	struct get<::Eigen::Array<T,R,C,::Eigen::RowMajor>> {
		static inline ::Eigen::Array<T,R,C,::Eigen::RowMajor> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Array<T,R,C,::Eigen::RowMajor>( dims[0], dims[1] );
	}};
	template<class T,int R,int C, int MR, int MC>
	struct get<::Eigen::Array<T,R,C,::Eigen::RowMajor,MR,MC>> {
		static inline ::Eigen::Array<T,R,C,::Eigen::RowMajor,MR,MC> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Array<T,R,C,::Eigen::RowMajor,MR,MC>( dims[0], dims[1] );
	}};
	template<class T,int R,int C>
	struct get<::Eigen::Array<T,R,C,::Eigen::ColMajor>> {
		static inline ::Eigen::Array<T,R,C,::Eigen::ColMajor> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Array<T,R,C,::Eigen::ColMajor>( dims[1], dims[0] );
	}};
	template<class T,int R,int C, int MR, int MC>
	struct get<::Eigen::Array<T,R,C,::Eigen::ColMajor,MR,MC>> {
		static inline ::Eigen::Array<T,R,C,::Eigen::ColMajor,MR,MC> ctor( std::array<size_t,2> dims ){
			return ::Eigen::Array<T,R,C,::Eigen::ColMajor,MR,MC>( dims[1], dims[0] );
	}};
}}

#endif
#endif
