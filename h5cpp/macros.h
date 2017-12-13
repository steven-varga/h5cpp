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
	template<typename T> struct TypeTraits;
	template<typename T> T ctor(hsize_t rank, const hsize_t* dims ){}
	template<typename T> hid_t h5type( ){}
	template<> hid_t h5type<std::string>(){ // std::string is variable length
			hid_t type = H5Tcopy (H5T_C_S1);
	   		H5Tset_size (type,H5T_VARIABLE);
			return type;
	}
}}

#define H5CPP_REGISTER_FUNDAMENTAL_TYPE( T )									\
			template <> struct TypeTraits<T> { 									\
				static const char* 	name; 										\
			}; 																	\
			const char*  TypeTraits<T>::name = #T; 								\


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
		template<> container<T> ctor<container<T>>(hsize_t hdf5_rank, const hsize_t* hdf5_dims ){ 			\
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
		hsize_t get_size( const container<T>&ref ){															\
			return ref.n_elem; 																				\
		}; 																									\
		std::array<hsize_t,R> get_dims( const container<T>& ref ){											\
			return __VA_ARGS__; 																			\
		}; 						 																			\
		T* get_ptr( container<T>& ref ){																	\
			return ref.address;  																			\
		}; 																									\
		const T* get_ptr( const container<T>& ref ){														\
			return ref.address;  																			\
		}; 																									\


#define H5CPP_STL_TEMPLATE_SPEC(T) 																			\
	H5CPP_BASE_TEMPLATE_SPEC(T, std::vector, data(), size(), H5CPP_RANK_VEC,  {ref.size()})  				\
	H5CPP_CTOR_SPEC(T, std::vector, H5CPP_RANK_VEC, (dims[0]))												\
	H5CPP_REGISTER_FUNDAMENTAL_TYPE(T) \


#ifdef ARMA_INCLUDES
/* definitions for armadillo containers */
	#define  H5CPP_ARMA_TEMPLATE_SPEC(T) 																				\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Row, memptr(), n_elem, H5CPP_RANK_VEC,  {ref.n_elem} ) 							\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Col, memptr(), n_elem, H5CPP_RANK_VEC,  {ref.n_elem} ) 							\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Mat, memptr(), n_elem, H5CPP_RANK_MAT,  {ref.n_rows, ref.n_cols} ) 				\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Cube,memptr(), n_elem, H5CPP_RANK_CUBE, {ref.n_rows, ref.n_cols, ref.n_slices} ) 	\
	H5CPP_CTOR_SPEC(T, arma::Row,  H5CPP_RANK_VEC,  (dims[0]) )															\
	H5CPP_CTOR_SPEC(T, arma::Col,  H5CPP_RANK_VEC,  (dims[0]) )															\
	H5CPP_CTOR_SPEC(T, arma::Mat,  H5CPP_RANK_MAT,  (dims[0],dims[1]) )													\
	H5CPP_CTOR_SPEC(T, arma::Cube, H5CPP_RANK_CUBE, (dims[0],dims[1],dims[2]) )											\

#else
	#define H5CPP_ARMA_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif


#define H5CPP_POD2H5T(POD_TYPE,H_TYPE) 	template<> hid_t h5type<POD_TYPE>(){ return  H5Tcopy(H_TYPE); }
#define H5CPP_SPECIALIZE(T)  H5CPP_ARMA_TEMPLATE_SPEC(T)  H5CPP_STL_TEMPLATE_SPEC(T)
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
#endif
