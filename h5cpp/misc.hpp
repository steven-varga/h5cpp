/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <hdf5.h>
#include <string>
#include <iostream>

#ifndef  H5CPP_MISC_H
#define H5CPP_MISC_H

#define H5CPP_supported_elementary_types "supported elementary types ::= pod_struct | float | double |  [signed](int8 | int16 | int32 | int64)"

namespace h5 { namespace utils {
	template <class T>
	static constexpr bool is_supported = std::is_class<T>::value | std::is_arithmetic<T>::value;
	//static constexpr bool is_supported = std::is_pod<T>::value && std::is_class<T>::value | std::is_arithmetic<T>::value;
}}

namespace h5{
	using cx_double =  std::complex<double>; /**< scientific type */
	using cx_float = std::complex<float>;    /**< scientific type */
	using herr_t = ::herr_t;

	static thread_local herr_t (*error_stack_callback)(::hid_t, void*);
	static thread_local void * error_stack_client_data;
}
#endif

