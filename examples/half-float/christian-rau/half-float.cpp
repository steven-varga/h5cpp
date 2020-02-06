/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include "half.hpp"  // this should be obtained from http://half.sourceforge.net/
#include <h5cpp/all>
#include "utils.hpp"

#define CHUNK_SIZE 5
#define NROWS 4*CHUNK_SIZE
#define NCOLS 1*CHUNK_SIZE

int main(){
	using half_float::half;
	using namespace half_float::literal;

	h5::fd_t fd = h5::create("example.h5",H5F_ACC_TRUNC);
	{
		std::cout <<"Floating point formats: \n";
		std::cout << h5::dt_t<half>() << std::endl;
		std::cout << h5::dt_t<float>() << std::endl;
		std::cout << h5::dt_t<double>() << std::endl;
		std::cout << h5::dt_t<long double>() << std::endl;
		std::cout <<"Half Float numerical limits: ";

		std::cout << "[min] " << std::numeric_limits<half>::min() <<" [max] " << std::numeric_limits<half>::max() << std::endl;
		std::cout << std::endl;
	}
	{
		std::vector<half> vec = h5::utils::get_test_data<half>(20);
		h5::write(fd,"data", vec);
		auto data = h5::read<std::vector<half>>(fd, "data");

		for( int i=0; i<20; i++ )
			std::cout << "[" << i << ": " << vec[i] << " " <<data[i] <<"]";
		std::cout << "\n\ncomputing difference ||saved - read|| expecting norm be close to zero:\n";
		for( int i=0; i<20; i++ )
			std::cout << abs(vec[i] - data[i]) <<" ";
	}
}

