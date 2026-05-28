/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <half.h>
#include <h5cpp/all>

// Register Imath::half as an HDF5 datatype via the new H5CPP_REGISTER_DATATYPE
// macro. The CREATE_EXPR + BODY together describe IEEE-754 binary16 layout:
// 5-bit exponent at bit 10, 10-bit mantissa at bit 0, sign at bit 15.
H5CPP_REGISTER_DATATYPE(Imath::half, "Imath::half",
    H5Tcopy(H5T_NATIVE_FLOAT),
    {
        H5Tset_fields(handle, 15, 10, 5, 0, 10);
        H5Tset_precision(handle, 16);
        H5Tset_ebias(handle, 15);
        H5Tset_size(handle, 2);
    })

namespace h5 { namespace utils {

	template <> inline  std::vector<Imath::half> get_test_data( size_t n ){
		std::random_device rd;
		std::mt19937 eng(rd());
		std::uniform_real_distribution<float> distr(-1e2, 1e2);

		std::vector<Imath::half> vec (n);
		for(int i=0; i<n; i++ ) vec[i] = distr(eng);
		return vec;
	}
}}
