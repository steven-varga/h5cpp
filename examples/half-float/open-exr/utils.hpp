
/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <h5cpp/H5misc.hpp>

namespace h5 { namespace utils {

	template <> inline  std::vector<half> get_test_data( size_t n ){
		std::random_device rd;
		std::mt19937 eng(rd());
		std::uniform_real_distribution<float> distr(-1e2, 1e2);

		std::vector<half> vec (n);
		for(int i=0; i<n; i++ ) vec[i] = distr(eng);
		return vec;
	}
}}
