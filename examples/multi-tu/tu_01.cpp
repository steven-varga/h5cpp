/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <armadillo>
#include <cstdint>
#include "struct.h"
#include <h5cpp/core>
	// generated file must be sandwiched between core and io 
	// to satisfy template dependencies in <h5cpp/io>  
	#include "tu_01.h"
#include <h5cpp/io>
#include "utils.hpp"

#define CHUNK_SIZE 5
#define NROWS 4*CHUNK_SIZE
#define NCOLS 1*CHUNK_SIZE

void test_01( const h5::fd_t& fd ){// LINARG:=[armaidllo|eigen3|blaze|blitz|it++|dlib|ublas] supported

	arma::imat M(NROWS,NCOLS);              // define a linalg object
	h5::write(fd, "/linalg/armadillo",M);   // save it somewhere, partial and full read|write and append supported
}

void test_02( const h5::fd_t& fd ){// create a Matrix of STRUCT with chunked and GZIP compressed properties ready for partial read|write

	// upto 7 dimensions/extents are supported
	h5::create<sn::example::Record>(fd, "/orm/chunked_2D", 
		h5::current_dims{NROWS,NCOLS}, h5::chunk{1,CHUNK_SIZE} | h5::gzip{8} );
	h5::create<sn::typecheck::Record>(fd, "/orm/typecheck",	h5::max_dims{H5S_UNLIMITED} );
}

