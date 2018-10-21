/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_MACROS_HPP 
#define  H5CPP_MACROS_HPP

#include "H5Mfundamental.hpp"
#include "H5Mbase.hpp"
#include "H5Mlinalg.hpp"
#include "H5Meigen.hpp"

#define H5CPP_STL_TEMPLATE_SPEC(T) 																			\
	H5CPP_BASE_TEMPLATE_SPEC(T, std::vector, ref.data(), ref.size(), H5CPP_RANK_VEC,  {ref.size()})  		\
	H5CPP_CTOR_SPEC(T, std::vector, H5CPP_RANK_VEC, (dims[0]))												\

/* BEGIN:
 * template specialization group for all linear algebra package object types */
#define H5CPP_LINALG_SPECIALIZE(T)  H5CPP_ARMA_TEMPLATE_SPEC(T)  H5CPP_EIGEN_TEMPLATE_SPEC(T) \
	H5CPP_UBLAS_TEMPLATE_SPEC(T)  H5CPP_ITPP_TEMPLATE_SPEC(T) H5CPP_BLITZ_TEMPLATE_SPEC(T) H5CPP_BLAZE_TEMPLATE_SPEC(T) \
	H5CPP_DLIB_TEMPLATE_SPEC(T) \

/* END */

#define H5CPP_REGISTER_TYPE( T, H )     H5CPP_STL_TEMPLATE_SPEC(T)  H5CPP_LINALG_SPECIALIZE(T)  H5CPP_FUNDAMENTAL_TYPE(T,H)
#define H5CPP_REGISTER_STL_TYPE( T, H ) H5CPP_STL_TEMPLATE_SPEC(T)  H5CPP_FUNDAMENTAL_TYPE(T,H)
#define H5CPP_REGISTER_STRUCT( T ) 	    namespace h5{ namespace utils { H5CPP_STL_TEMPLATE_SPEC(T); }}

namespace h5 { namespace utils {
	// stl::string has nonstandard mapping from char to hdf5
	H5CPP_STL_TEMPLATE_SPEC(std::string)

	//H5CPP_REGISTER_STL_TYPE(bool,H5T_NATIVE_HBOOL) // FIXME: stl::vector<bool> is a special case
	H5CPP_REGISTER_STL_TYPE(long double,H5T_NATIVE_LDOUBLE)

	H5CPP_REGISTER_TYPE(unsigned char, H5T_NATIVE_UCHAR) 			H5CPP_REGISTER_TYPE(char, H5T_NATIVE_CHAR)
	H5CPP_REGISTER_TYPE(unsigned short, H5T_NATIVE_USHORT) 			H5CPP_REGISTER_TYPE(short, H5T_NATIVE_SHORT)
	H5CPP_REGISTER_TYPE(unsigned int, H5T_NATIVE_UINT) 				H5CPP_REGISTER_TYPE(int, H5T_NATIVE_INT)
	H5CPP_REGISTER_TYPE(unsigned long int, H5T_NATIVE_ULONG) 		H5CPP_REGISTER_TYPE(long int, H5T_NATIVE_LONG)
	H5CPP_REGISTER_TYPE(unsigned long long int, H5T_NATIVE_ULLONG)  H5CPP_REGISTER_TYPE(long long int, H5T_NATIVE_LLONG)
	H5CPP_REGISTER_TYPE(float, H5T_NATIVE_FLOAT) 					H5CPP_REGISTER_TYPE(double, H5T_NATIVE_DOUBLE)
}}
#endif
