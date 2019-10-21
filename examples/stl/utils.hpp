
/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include "struct.h"
#include <h5cpp/H5misc.hpp>

namespace h5 { namespace utils {
	// template specialization 
	template <> inline  std::vector<sn::example::Record> get_test_data( size_t n ){
		std::vector<sn::example::Record> vec (n);
		for(int i=0; i<n; i++ )
			vec[i].idx = i;
		return vec;
	}
	template <class T, size_t N> inline
		std::array<sn::example::Record,N> get_test_data( ){
	std::array<sn::example::Record,N> vec;
	for(int i=0; i<N; i++ )
		vec[i].idx = i;
	return vec;
	}
}}
