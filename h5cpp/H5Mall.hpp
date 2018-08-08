/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_MACROS_HPP 
#define  H5CPP_MACROS_HPP

	namespace h5 { namespace utils {

		template<typename T> struct base;
		template<typename T> inline T ctor(hsize_t rank, const hsize_t* dims ){}
		template<typename T> static inline hid_t h5type( ){ return 0; } // must apply 'H5Tclose' to return value to prevent resource leakage
		template<typename T> inline std::string type_name( ){ return "n/a"; }
		template<typename T> inline std::string h5type_name( ){ return "n/a"; }
		template<> inline std::string type_name<std::string>(){ return "std::string";}
		template<> inline hid_t h5type<char*>(){ // std::string is variable length
				hid_t type = H5Tcopy (H5T_C_S1);
				H5Tset_size (type,H5T_VARIABLE);
				return type;
		}
		template<> inline hid_t h5type<std::string>(){ // std::string is variable length
				return h5::utils::h5type<char*>();
		}
	}}
	// generate map from C/C++ typename to std::string
#define H5CPP_FUNDAMENTAL_TYPE( T, H )  \
	template<> inline std::string type_name<T>(){ return #T; }  	\
	template<> inline std::string h5type_name<T>(){ return #H; }   \
 	template<> inline hid_t h5type<T>(){ return  H5Tcopy(H); }     \

#include "H5Mbase.hpp"
#include "H5Mlinalg.hpp"

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
