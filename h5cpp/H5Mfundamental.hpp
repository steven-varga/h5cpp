/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef H5CPP_MFUNDAMENTAL_HPP
#define H5CPP_MFUNDAMENTAL_HPP

namespace h5 { namespace utils {

	template<class T> struct base {
		using type = T;
		constexpr static const size_t rank = 1;
	};

	template<class T> inline
	typename std::enable_if<std::is_integral<T>::value || std::is_same<T,std::string>::value,
	h5::count_t>::type get_count( const T& ref ){
		return h5::count{1};
	}
	template <class T> inline
	typename std::enable_if<std::is_integral<T>::value || std::is_same<T,std::string>::value,
	hsize_t>::type get_size( const T& ref ){
			return 1;
	}
	template <class T> inline
	typename std::enable_if<std::is_integral<T>::value,
	const T*>::type get_ptr( const T& ref ){
			return &ref;
	}
;

// std::string

	template<typename T> inline T ctor(hsize_t rank, const hsize_t* dims ){}
	template<typename T> static inline hid_t h5type( ){ return 0; } // must apply 'H5Tclose' to return value to prevent resource leakage
	template<typename T> inline std::string type_name( ){ return "n/a"; }
	template<typename T> inline std::string h5type_name( ){ return "n/a"; }
	/*specializations for std::string*/
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

#endif
