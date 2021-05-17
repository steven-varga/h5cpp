/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */


/** @example vector.cpp
 * A description of the example file, causes the example file to show up in 
 * Examples */
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
	//RAII will close resource, noo need H5Fclose( any_longer ); 
	h5::fd_t fd = h5::create("example.h5",H5F_ACC_TRUNC);
	{
		std::vector<double> v(10, 1.0);
		h5::write(fd,"stl/vector/full.dat", v); // simplest example

		//An elaborate example to demonstrate how to use H5CPP when you know the details, but no time/budget
		//to code it. The performance must be on par with the best C implementation -- if not: shoot an email and I fix it
		h5::create<double>(fd,"stl/vector/partial.dat",
				// arguments can be written any order without loss of performance thanks to compile time parsing
				h5::current_dims{21,10},h5::max_dims{H5S_UNLIMITED,10}, h5::chunk{1,10} | h5::gzip{9} );

		// you have some memory region you liked to read/write from, and H5CPP doesn't know of your object + no time to
		// fiddle around you want it done:
		// SOLUTION: write/read from/to memory region, NOTE the type cast: h5::write<DOUBLE>( ... );
		h5::write(fd,"stl/vector/partial.dat",  v, h5::offset{2,3}, h5::count{2,5});
	}

	{ // creates + writes entire POD STRUCT tree
		std::vector<sn::example::Record> vec = h5::utils::get_test_data<sn::example::Record>(20);
		h5::write(fd, "orm/partial/vector one_shot", vec );
		// dimensions and other properties specified additional argument 
		h5::write(fd, "orm/partial/vector custom_dims", vec,
			h5::max_dims{H5S_UNLIMITED}, h5::gzip{9} | h5::chunk{20} );
		// you don't need to remember order, compiler will do it for you without runtime penalty:
		h5::write(fd, "orm/partial/vector custom_dims different_order", vec,
			h5::chunk{20} | h5::gzip{9}, 
			h5::block{2}, h5::max_dims{H5S_UNLIMITED}, h5::stride{4}, h5::current_dims{100}, h5::offset{3} );
	}
	{ // read entire dataset back
		using T = std::vector<sn::example::Record>;
		auto data = h5::read<T>(fd,"/orm/partial/vector one_shot");
		std::cerr <<"reading back data previously written:\n\t";
		for( auto r:data )
			std::cerr << r.idx <<" ";
		std::cerr << std::endl;
	}
}
