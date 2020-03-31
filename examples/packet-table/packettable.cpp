/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#define ARMA_DONT_USE_WRAPPER
#include <armadillo>
#include <Eigen/Dense> // must include Eigen before <h5cpp/core>

#include <cstdint>
#include "struct.h"
#include <h5cpp/core>
	// generated file must be sandwiched between core and io 
	// to satisfy template dependencies in <h5cpp/io>  
	#include "generated.h"
#include <h5cpp/io>
#include "utils.hpp"

template<class T> using Matrix   = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

int main(){

	h5::fd_t fd = h5::create("example.h5",H5F_ACC_TRUNC);

	// SCALAR: integral
	// The motivation behind this example to allow 2D frames be recorded into a stream
	// 3x5 is the frame or image size, with 2 planes. 
	try { // centrally used error handling
		std::vector<int> stream(83);
		std::iota(std::begin(stream), std::end(stream), 1);
		// the leading dimension is extended once chunk is full, chunk is filled in row major order
		// zero copy writes directly to chunk buffer then pushed through filter chain if specified
		// works up to H5CPP_MAX_RANK default to 7
		// last chunk if partial filled with h5::fill_value<T>( some_value )  
		h5::pt_t pt = h5::create<int>(fd, "stream of integral 01",
				 h5::max_dims{H5S_UNLIMITED,3,5}, h5::chunk{2,3,5} | h5::gzip{9} | h5::fill_value<int>(3) );
		for( auto record : stream )
			h5::append(pt, record);
		//auto M = h5::read<arma::mat>(fd,"stream of integral" );
	} catch ( const h5::error::any& e ){
		std::cerr << "ERROR:" << e.what();
	}

	try { // centrally used error handling
		std::vector<int> stream(83);
		std::iota(std::begin(stream), std::end(stream), 1);
		// the leading dimension is extended once chunk is full, chunk is filled in row major order
		// zero copy writes directly to chunk buffer then pushed through filter chain if specified
		// works up to H5CPP_MAX_RANK default to 7
		// last chunk if partial filled with h5::fill_value<T>( some_value )  
		h5::pt_t pt = h5::create<int>(fd, "stream of integral 02",
									  h5::max_dims{H5S_UNLIMITED}, h5::chunk{6} | h5::gzip{9} | h5::fill_value<int>(3) );
		for( auto record : stream )
			h5::append(pt, record);
	} catch ( const h5::error::any& e ){
		std::cerr << "ERROR:" << e.what();
	}
	

	// SCALAR: pod 
	try { //
		std::vector<sn::example::Record> stream = h5::utils::get_test_data<sn::example::Record>(127);

		// implicit conversion from h5::ds_t to h5::pt_t makes it a breeze to create
		// packet_table from h5::open | h5::create calls,
		// The newly created h5::pt_t  stateful container caches the incoming data until
		// bucket filled. IO operations are at h5::chunk boundaries
		// or when resource is released. Last partial chunk handled as expected.
		//
		// compiler assisted introspection generates boilerplate, developer 
		// can focus on the idea, leaving boring details to machines 
		h5::pt_t pt = h5::create<sn::example::Record>(fd, "stream of struct",
				 h5::max_dims{H5S_UNLIMITED,7}, h5::chunk{4,7} | h5::gzip{9} );
		for( auto record : stream )
			h5::append(pt, record);
	} catch ( const h5::error::any& e ){
		std::cerr << "ERROR:" << e.what();
	}

	{ 	// packet table for a collection of matrices modelling a HD resolution of gray scale images
		size_t nrows = 2, ncols=256, nframes=100;
		h5::pt_t pt = h5::create<double>(fd, "stream of matrices",
				h5::max_dims{H5S_UNLIMITED,nrows,ncols}, h5::chunk{1,nrows,ncols} );
		Matrix<double> M(nrows,ncols);
		int k=0;
	   	for( int i=0; i<nrows; i++) for(int j=0; j<ncols; j++) M(i,j) = ++k;
		// actual code, you may insert arbitrary number of frames: nrows x ncols
		for( int i = 0; i < nframes; i++)
			h5::append( pt, M);
	}

	return 0;
}
