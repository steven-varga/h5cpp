/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <armadillo>
#include <cstdint>
#include "struct.h"
#include <h5cpp/core>
	// generated file must be sandwiched between core and io 
	// to satisfy template dependencies in <h5cpp/io>  
	#include "tu_02.h"
	// multiple inclusion is on purpose to test 
	// include guards in generated file: tu_02.h 
	#include "tu_02.h"
#include <h5cpp/io>
#include "utils.hpp"

#define CHUNK_SIZE 5
#define NROWS 4*CHUNK_SIZE
#define NCOLS 1*CHUNK_SIZE


void test_03( const h5::fd_t& fd ){ // creates + writes entire object tree

	std::vector<sn::example::Record> vec = h5::utils::get_test_data<sn::example::Record>(20);
	h5::write(fd, "orm/partial/vector one_shot", vec );
	// dimensions and other properties specified additional argument 
	h5::write(fd, "orm/partial/vector custom_dims", vec,
		h5::current_dims{100}, h5::max_dims{H5S_UNLIMITED}, h5::gzip{9} | h5::chunk{20} );
	// you don't need to remember order, compiler will do it for you without runtime penalty:
	h5::write(fd, "orm/partial/vector custom_dims different_order", vec,
		h5::chunk{20} | h5::gzip{9}, 
		h5::block{2}, h5::max_dims{H5S_UNLIMITED}, h5::stride{2}, h5::current_dims{100}, h5::offset{3} );
}

void test_04( const h5::fd_t& fd ){ // read entire dataset back

	using T = std::vector<sn::example::Record>;
	std::cerr<< "reading data: \n";
	auto data = h5::read<T>(fd,"/orm/partial/vector one_shot");
	std::cerr <<"reading back data previously written:\n\t";
	for( auto r:data )
		std::cerr << r.idx <<" ";

	std::cerr << std::endl;
}

