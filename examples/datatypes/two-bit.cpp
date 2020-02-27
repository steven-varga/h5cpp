/* Copyright (c) 2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <h5cpp/core> // include this before custom type definition
	#include "two-bit.hpp"
#include <h5cpp/io> // IO operators become aware of your custom type

int main(){
	namespace nm = bitstring;

	h5::fd_t fd = h5::create("example.h5",H5F_ACC_TRUNC);
	// prints out type info, eases on debugging
	std::cout << h5::dt_t<nm::two_bit>() << std::endl;

	std::vector<nm::two_bit> vec = {0xff,0x0f,0xf0,0x00,0b0001'1011};

	/* H5CPP operators are aware of your dataype, will do the right thing
	 */
	h5::write(fd,"data", vec); // single shot write
	auto data = h5::read<std::vector<nm::two_bit>>(fd, "data");

	for( int i=0; i<vec.size(); i++ )
		std::cout << "[" << i << ": " << vec[i] << " "  <<"]";
	std::cout << "\n\ncomputing difference ||saved - read|| expecting norm to be zero:\n";
	for( int i=0; i<vec.size(); i++ )
		std::cout << abs(vec[i].value - data[i].value) <<" ";
}

