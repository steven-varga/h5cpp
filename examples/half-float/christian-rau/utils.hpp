
/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include "struct.h"
#include <h5cpp/H5misc.hpp>

namespace h5 { namespace utils {
	using namespace half_float::literal;
	using half_float::half;
	// template specialization 
	template <> inline  std::vector<sn::example::Record> get_test_data( size_t n ){
		std::vector<sn::example::Record> vec (n);
		for(int i=0; i<n; i++ )
			vec[i].idx = i;
		return vec;
	}
	template <> inline  std::vector<half> get_test_data( size_t n ){
		std::random_device rd;
		std::mt19937 eng(rd());
		std::uniform_real_distribution<float> distr(-1e2, 1e2);

		std::vector<half> vec (n);
		for(int i=0; i<n; i++ ) vec[i] = distr(eng);
		return vec;
	}
}}
