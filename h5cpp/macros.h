/*
 * Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this  software  and associated documentation files (the "Software"), to deal in
 * the Software  without   restriction, including without limitation the rights to
 * use, copy, modify, merge,  publish,  distribute, sublicense, and/or sell copies
 * of the Software, and to  permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE  SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT  SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY  CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER
 * IN  AN  ACTION  OF  CONTRACT, TORT OR  OTHERWISE, ARISING  FROM,  OUT  OF  OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <hdf5.h>
#include "config.h"
#include <array>
#include <vector>

#ifndef  H5CPP_MACROS_H 
#define H5CPP_MACROS_H
namespace h5 { namespace utils {

	template<typename T> struct base;
	template<typename T> inline T ctor(hsize_t rank, const hsize_t* dims ){}
	template<typename T> static inline hid_t h5type( ){} // must apply 'H5Tclose' to return value to prevent resource leakage
	template<typename T> inline std::string type_name( ){ return "n/a"; }
	template<> inline hid_t h5type<std::string>(){ // std::string is variable length
			hid_t type = H5Tcopy (H5T_C_S1);
	   		H5Tset_size (type,H5T_VARIABLE);
			return type;
	}  // ditto! call H5Tclose on returned type
}}

#define H5CPP_REGISTER_FUNDAMENTAL_TYPE( T )  template<> inline std::string type_name<T>(){ return #T; };

// macros which specialize on the 'base' template detect underlying properties such as type at compile type, as well define 
// general accessors so differences in properties can be mitigated
//
// modify the macro if new container type, or property accessor as added,  

/* -------------------------------------------------------------------------------------------------*/

/* ----------------------------- BEGIN META TEMPLATE -----------------------------------------*/
/* meta templates to leverage differences among containers: armadillo, stl, etc...
 * the variadic macro argument '...' is to escape commas inside: '{rows,cols,slices}'   
 */

#define H5CPP_CTOR_SPEC( T,container, container_rank, ... ) 										\
		template<> inline container<T> ctor<container<T>>(hsize_t hdf5_rank, const hsize_t* hdf5_dims ){ 	\
			/* it possible to have an HDF5 file space higher rank then of the container  					\
			 * we're mapping it to. let's collapse dimensions which exceed rank of output container 
			 *  
			 * */																							\
			hsize_t dims[container_rank];																	\
			int i=0; 																						\
			/* branch off if indeed the ranks mismatch, otherwise just a straight copy */ 					\
			if( container_rank < hdf5_rank ){ 																\
				dims[container_rank-1]=1; 																	\
				switch( container_rank ){ 																	\
					case H5CPP_RANK_VEC: /*easy: multiply them all*/										\
							for(;i<hdf5_rank;i++) dims[0] *= hdf5_dims[i]; 									\
						break; 																				\
					case H5CPP_RANK_MAT: /*return the first dimensions which are not equal to 1 \
										   then collapse the tail                        */					\
							for(; hdf5_dims[i] <= 1 && i<hdf5_rank;i++); dims[0] = hdf5_dims[i++]; 			\
							for(;i<hdf5_rank;i++) dims[1] *= hdf5_dims[i]; 									\
						break; 																				\
					case H5CPP_RANK_CUBE: /* ditto */														\
							for(; hdf5_dims[i] <= 1 && i<hdf5_rank;i++); dims[0] = hdf5_dims[i++]; 			\
							for(; hdf5_dims[i] <= 1 && i<hdf5_rank;i++); dims[1] = hdf5_dims[i++]; 			\
							for(;i<hdf5_rank;i++) dims[2] *= hdf5_dims[i]; 									\
						break; 																				\
				} 																							\
			} else 																							\
				for(; i<container_rank; i++) dims[i] = hdf5_dims[i]; 										\
			/*expands to constructor of the form: arma::Mat<int>(dims[0],dims[1]);  */ 						\
			return container<T> __VA_ARGS__ ; 																\
		}; 																									\


/* ----------------------------- END META TEMPLATE -----------------------------------------*/ 				\
#define H5CPP_BASE_TEMPLATE_SPEC( T, container, address, n_elem, R, ... )									\
		template<> struct base<container<T>> {																\
			typedef T type; 																				\
			static const size_t rank=R; 																	\
		}; 																									\
		inline hsize_t get_size( const container<T>&ref ){													\
			return n_elem; 																				\
		}; 																									\
		inline std::array<hsize_t,R> get_dims( const container<T>& ref ){									\
			return __VA_ARGS__; 																			\
		}; 						 																			\
		inline T* get_ptr( container<T>& ref ){																\
			return ref.address;  																			\
		}; 																									\
		inline const T* get_ptr( const container<T>& ref ){													\
			return ref.address;  																			\
		}; 																									\


#define H5CPP_STL_TEMPLATE_SPEC(T) 																			\
	H5CPP_BASE_TEMPLATE_SPEC(T, std::vector, data(), ref.size(), H5CPP_RANK_VEC,  {ref.size()})  				\
	H5CPP_CTOR_SPEC(T, std::vector, H5CPP_RANK_VEC, (dims[0]))												\
	H5CPP_REGISTER_FUNDAMENTAL_TYPE(T) \

/* ARMADILLO */
#if defined(ARMA_INCLUDES) || defined(H5CPP_USE_ARMADILLO)
/* definitions for armadillo containers */
	#define  H5CPP_ARMA_TEMPLATE_SPEC(T) 																				\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Row, memptr(), ref.n_elem, H5CPP_RANK_VEC,  {ref.n_elem} ) 							\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Col, memptr(), ref.n_elem, H5CPP_RANK_VEC,  {ref.n_elem} ) 							\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Mat, memptr(), ref.n_elem, H5CPP_RANK_MAT,  {ref.n_cols, ref.n_rows} ) 				\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Cube,memptr(), ref.n_elem, H5CPP_RANK_CUBE, {ref.n_slices, ref.n_cols, ref.n_rows} ) 	\
	H5CPP_CTOR_SPEC(T, arma::Row,  H5CPP_RANK_VEC,  (dims[0]) )															\
	H5CPP_CTOR_SPEC(T, arma::Col,  H5CPP_RANK_VEC,  (dims[0]) )															\
	H5CPP_CTOR_SPEC(T, arma::Mat,  H5CPP_RANK_MAT,  (dims[1],dims[0]) )													\
	H5CPP_CTOR_SPEC(T, arma::Cube, H5CPP_RANK_CUBE, (dims[2],dims[1],dims[0]) )											\

#else
	#define H5CPP_ARMA_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif
/* EIGEN3: doesn use versioning  */
#if defined(EIGEN_CORE_H) || defined(H5CPP_USE_EIGEN3)
	namespace eigen {
		template<class T> using amat 	= Eigen::Array<T,Eigen::Dynamic,Eigen::Dynamic>;
		template<class T> using arowvec = Eigen::Array<T,1,Eigen::Dynamic>;
		template<class T> using acolvec = Eigen::Array<T,Eigen::Dynamic,1>;

		template<class T> using mat    = Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic>;
		template<class T> using rowvec = Eigen::Matrix<T,1,Eigen::Dynamic>;
		template<class T> using colvec = Eigen::Matrix<T,Eigen::Dynamic,1>;
	}
	#define H5CPP_EIGEN_TEMPLATE_SPEC(T) \
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::rowvec, data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size()} )						\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::colvec, data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size()} )						\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::mat,    data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )	\
	H5CPP_CTOR_SPEC(T, eigen::rowvec,  H5CPP_RANK_VEC,  (dims[0]) )																\
	H5CPP_CTOR_SPEC(T, eigen::colvec,  H5CPP_RANK_VEC,  (dims[0]) )																\
	H5CPP_CTOR_SPEC(T, eigen::mat,     H5CPP_RANK_MAT,  (dims[1], dims[0]) )													\
	\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::arowvec, data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size()} )						\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::acolvec, data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size()} )						\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::amat,    data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )	\
	H5CPP_CTOR_SPEC(T, eigen::arowvec,  H5CPP_RANK_VEC,  (dims[0]) )																\
	H5CPP_CTOR_SPEC(T, eigen::acolvec,  H5CPP_RANK_VEC,  (dims[0]) )																\
	H5CPP_CTOR_SPEC(T, eigen::amat,     H5CPP_RANK_MAT,  (dims[1], dims[0]) )														\

#else
	#define H5CPP_EIGEN_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif
/* BOOST MATRIX  */
#if defined(_BOOST_UBLAS_MATRIX_) || defined(H5CPP_USE_UBLAS_MATRIX)
	namespace ublas {
		template<class T> using matrix 	= boost::numeric::ublas::matrix<T>;
	}
	#define H5CPP_UBLASM_TEMPLATE_SPEC(T) \
	H5CPP_BASE_TEMPLATE_SPEC(T, ::ublas::matrix, data().begin(), ref.size1()*ref.size2(), H5CPP_RANK_MAT,  { (hsize_t) ref.size2(), (hsize_t) ref.size1()} )	\
	H5CPP_CTOR_SPEC(T, ::ublas::matrix,     H5CPP_RANK_MAT,  (dims[1], dims[0]) )														\

#else
	#define H5CPP_UBLASM_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif
#if defined(_BOOST_UBLAS_VECTOR_) || defined(H5CPP_USE_UBLAS_VECTOR)
	namespace ublas {
		template<class T> using vector 	= boost::numeric::ublas::vector<T>;
	}
	#define H5CPP_UBLASV_TEMPLATE_SPEC(T) \
	H5CPP_BASE_TEMPLATE_SPEC(T, ::ublas::vector, data().begin(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size() } )	\
	H5CPP_CTOR_SPEC(T, ::ublas::vector,     H5CPP_RANK_VEC,  (dims[0]) ) 													\

#else
	#define H5CPP_UBLASV_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif
#define H5CPP_UBLAS_TEMPLATE_SPEC(T) H5CPP_UBLASM_TEMPLATE_SPEC(T) H5CPP_UBLASV_TEMPLATE_SPEC(T)

/* IT++  */
#if defined(MAT_H) || defined(H5CPP_USE_ITPP_MATRIX)
	#define H5CPP_ITPPM_TEMPLATE_SPEC(T) \
	H5CPP_BASE_TEMPLATE_SPEC(T,itpp::Mat,_data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )	\
	H5CPP_CTOR_SPEC(T, itpp::Mat,     H5CPP_RANK_MAT,  (dims[1], dims[0]) )												\

#else
	#define H5CPP_ITPPM_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif
#if defined(VEC_H) || defined(H5CPP_USE_ITPP_VECTOR)
	#define H5CPP_ITPPV_TEMPLATE_SPEC(T) \
	H5CPP_BASE_TEMPLATE_SPEC(T, itpp::Vec, _data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size() } )	\
	H5CPP_CTOR_SPEC(T, itpp::Vec,     H5CPP_RANK_VEC,  (dims[0]) ) 													\

#else
	#define H5CPP_ITPPV_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif
#define H5CPP_ITPP_TEMPLATE_SPEC(T) H5CPP_ITPPM_TEMPLATE_SPEC(T) H5CPP_ITPPV_TEMPLATE_SPEC(T)
/* BLITZ  */
#if defined(BZ_ARRAY_ONLY_H) || defined(H5CPP_USE_BLITZ)
	namespace blitz {
		template<class T> using vector 	= blitz::Array<T,1>;
		template<class T> using matrix 	= blitz::Array<T,2>;
		template<class T> using qube 	= blitz::Array<T,3>;
	}

	#define H5CPP_BLITZ_TEMPLATE_SPEC(T) \
	H5CPP_BASE_TEMPLATE_SPEC(T,::blitz::vector,data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t)ref.size() } )	\
	H5CPP_BASE_TEMPLATE_SPEC(T,::blitz::matrix,data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t)ref.cols(), (hsize_t)ref.rows()} )	\
	H5CPP_BASE_TEMPLATE_SPEC(T,::blitz::qube,data(),   ref.size(), H5CPP_RANK_CUBE, { (hsize_t)ref.depth(),(hsize_t)ref.cols(),(hsize_t)ref.rows()} )	\
	H5CPP_CTOR_SPEC(T, ::blitz::vector,   H5CPP_RANK_VEC,  (dims[0]) )												\
	H5CPP_CTOR_SPEC(T, ::blitz::matrix,   H5CPP_RANK_MAT,  (dims[1], dims[0]) )												\
	H5CPP_CTOR_SPEC(T, ::blitz::qube,     H5CPP_RANK_CUBE, (dims[2], dims[1], dims[0]) )										\

#else
	#define H5CPP_BLITZ_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif


/* END DEF */


#define H5CPP_POD2H5T(POD_TYPE,H_TYPE) 	template<> inline hid_t h5type<POD_TYPE>(){ return  H5Tcopy(H_TYPE); }
/* BEGIN */
#define H5CPP_SPECIALIZE(T)  H5CPP_ARMA_TEMPLATE_SPEC(T)  H5CPP_STL_TEMPLATE_SPEC(T) H5CPP_EIGEN_TEMPLATE_SPEC(T) \
	H5CPP_UBLAS_TEMPLATE_SPEC(T)  H5CPP_ITPP_TEMPLATE_SPEC(T) H5CPP_BLITZ_TEMPLATE_SPEC(T) \

/* END */
#define H5CPP_REGISTER_STL_TYPE( T, H ) H5CPP_POD2H5T(T,H) H5CPP_STL_TEMPLATE_SPEC(T)
#define H5CPP_REGISTER_TYPE( T, H ) H5CPP_POD2H5T(T,H) H5CPP_SPECIALIZE(T)
//TODO: recursive template
//http://www.boost.org/doc/libs/1_65_1/libs/preprocessor/doc/index.html
#define H5CPP_REGISTER_STRUCT( T ) 	namespace h5{ namespace utils { H5CPP_STL_TEMPLATE_SPEC(T); }}

namespace h5 { namespace utils {
	// stl::string has nonstandard mapping from char to hdf5
	H5CPP_STL_TEMPLATE_SPEC(std::string)

	//H5CPP_REGISTER_STL_TYPE(bool,H5T_NATIVE_HBOOL) // FIXME: stl::vector<bool> is a special case
	H5CPP_REGISTER_STL_TYPE(long double,H5T_NATIVE_LDOUBLE)

	H5CPP_REGISTER_TYPE(unsigned char, H5T_NATIVE_UCHAR); 			H5CPP_REGISTER_TYPE(char, H5T_NATIVE_CHAR);
	H5CPP_REGISTER_TYPE(unsigned short, H5T_NATIVE_USHORT); 		H5CPP_REGISTER_TYPE(short, H5T_NATIVE_SHORT);
	H5CPP_REGISTER_TYPE(unsigned int, H5T_NATIVE_UINT); 			H5CPP_REGISTER_TYPE(int, H5T_NATIVE_INT);
	H5CPP_REGISTER_TYPE(unsigned long int, H5T_NATIVE_ULONG); 		H5CPP_REGISTER_TYPE(long int, H5T_NATIVE_LONG);
	H5CPP_REGISTER_TYPE(unsigned long long int, H5T_NATIVE_ULLONG); H5CPP_REGISTER_TYPE(long long int, H5T_NATIVE_LLONG);
	H5CPP_REGISTER_TYPE(float, H5T_NATIVE_FLOAT); 					H5CPP_REGISTER_TYPE(double, H5T_NATIVE_DOUBLE);
}}

// H5CPP_ASSERT_POINTER( T ) ""

/*
#undef H5CPP_REGISTER_FUNDAMENTAL_TYPE
#undef H5CPP_REGISTER_TYPE
#undef H5CPP_REGISTER_STL_TYPE
#undef H5CPP_SPECIALIZE
#undef H5CPP_POD2H5T
#undef H5CPP_STL_TEMPLATE_SPEC
#undef H5CPP_ARMA_TEMPLATE_SPEC
#undef H5CPP_BASE_TEMPLATE_SPEC
#undef H5CPP_CTOR_SPEC
*/

#define H5CPP_DOC_LINKS 													 \
	 /* [1]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Create \
	 * [2]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Open   \
	 * [3]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Close  \
	 * [4]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Open \
	 * [5]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Close \
	 * [10]: https://support.hdfgroup.org/HDF5/Tutor/compress.html \
	 */ 

#endif
