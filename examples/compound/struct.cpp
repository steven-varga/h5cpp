/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <armadillo>
#include <cstdint>
#include "struct.h"
#include <h5cpp/core>
	// generated file must be sandwiched between core and io 
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
	
	{// LINARG:=[armaidllo|eigen3|blaze|blitz|it++|dlib|ublas] supported
		arma::imat M(NROWS,NCOLS);              // define a linalg object
		h5::write(fd, "/linalg/armadillo",M);   // save it somewhere, partial and full read|write and append supported
	}
	{// create a Matrix of STRUCT with chunked and GZIP compressed properties ready for partial read|write
	 // upto 7 dimensions/extents are supported
		h5::create<sn::example::Record>(fd, "/orm/chunked_2D", 
				h5::current_dims{NROWS,NCOLS}, h5::chunk{1,CHUNK_SIZE} | h5::gzip{8} );
		//FIXME: defaults to unit chunk, which may not the best setting, yet chunking is required for unlimted
		// should we have some plausable value: 1024 instead?
		h5::create<sn::typecheck::Record>(fd, "/orm/typecheck",	h5::max_dims{H5S_UNLIMITED} );
	}
	{ // creates + writes entire object tree
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
	{ // read entire dataset back
		using T = std::vector<sn::example::Record>;
		auto data = h5::read<T>(fd,"/orm/partial/vector one_shot");
		std::cerr <<"reading back data previously written:\n\t";
		for( auto r:data )
			std::cerr << r.idx <<" ";
		std::cerr << std::endl;
	}
}
