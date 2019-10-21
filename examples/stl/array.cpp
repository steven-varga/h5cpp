/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

/** @example vector.cpp
 * A description of the example file, causes the example file to show up in 
 * Examples */
#include <array>
#include <vector>
#include <string>
#include <algorithm>

#include <cstdint>
#include "struct.h"
#include <h5cpp/core>
	// compound type descriptor must be sandwiched between core and io 
	// to satisfy template dependencies in <h5cpp/io>  
	#include "generated.h"
#include <h5cpp/io>
#include "utils.hpp"

#define CHUNK_SIZE 5
#define NROWS 4*CHUNK_SIZE
#define NCOLS 1*CHUNK_SIZE

int main(){
	constexpr size_t N = 10;
	h5::fd_t fd = h5::create("example.h5",H5F_ACC_TRUNC);
	{
		std::array<double,N> object; std::fill(std::begin(object), std::end(object), 2e0 );
		h5::write(fd,"stl/array/full.dat", object);
		h5::create<double>(fd, "stl/array/partial.dat",
				h5::current_dims{4,7}, h5::max_dims{H5S_UNLIMITED,7}, h5::chunk{1,7} | h5::gzip{9} );
		h5::write<double>(fd,"stl/array/partial.dat",  h5::impl::data(object), h5::count{2,5}, h5::offset{1,1} );
	}
	{
		std::array<sn::example::Record,N> vec = h5::utils::get_test_data<sn::example::Record,N>();
		h5::write(fd, "orm/partial/vector one_shot", vec );
	}
	{ // READ: pass the correct type information, which in this case contains size as well
		using T = std::array<sn::example::Record,N>;
		auto data = h5::read<T>(fd,"/orm/partial/vector one_shot");
		for( auto r:data )
			std::cerr << r.idx <<" ";
		std::cerr << std::endl;
	}
}
