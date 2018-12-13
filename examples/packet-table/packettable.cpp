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
	try { // centrally used error handling
		std::vector<int> stream(83);
		std::iota(std::begin(stream), std::end(stream), 1);
		// the leading dimension is extended once chunk is full, chunk is filled in row major order
		// zero copy writes directly to chunk buffer then pushed through filter chain if specified
		// works up to H5CPP_MAX_RANK default to 7
		// last chunk if partial filled with h5::fill_value<T>( some_value )  
		h5::pt_t pt = h5::create<int>(fd, "stream of integral",
				 h5::max_dims{H5S_UNLIMITED,3,5}, h5::chunk{2,3,5} | h5::gzip{9} | h5::fill_value<int>(3) );
		for( auto record : stream )
			h5::append(pt, record);
		//auto M = h5::read<arma::mat>(fd,"stream of integral" );
	} catch ( const h5::error::any& e ){
		std::cerr << "ERROR:" << e.what();
	}

	// SCALAR: pod 
	try { // centrally used error handling
		std::vector<sn::example::Record> stream = h5::utils::get_test_data<sn::example::Record>(127);

		// implicit conversion from h5::ds_t to h5::pt_t makes it a breeze to create
		// packet_table from h5::open | h5::create calls,
		// The newly created h5::pt_t  stateful container caches the incoming data until
		// bucket filled. IO operations are at h5::chunk boundaries
		// or when resource is released. Last partial chunk handled as expected.
		//
		// compiler assisted introspection generates boilerplate, developer 
		// can focus on the idea, leaving boring details to machines 
		//h5::pt_t pt = h5::create<sn::example::Record>(fd, "stream of struct",
		h5::pt_t pt = h5::create<sn::example::Record>(fd, "stream of struct",
				 h5::max_dims{H5S_UNLIMITED,7}, h5::chunk{4,7} | h5::gzip{9} );
		for( auto record : stream )
			h5::append(pt, record);
	} catch ( const h5::error::any& e ){
		std::cerr << "ERROR:" << e.what();
	}
	/*
	// CONTAINERS
	{ 	// packet table for a collection of matrices modelling a HD resolution of gray scale images
		// c++ objects may be factored into:
		// 1. information available compile time: underlying type or element_type, ... 
		// 2. runtime information: actual place and size in memory
		// because of the latter, there is no 'type' safe way to declare a packet table but agree upon that 
		// the slowest growing dimension -- the left most in current_dims{ index, rows,cols } 
		// is the object index, the rest : [rows*cols] is the data content of the object being indexed

		// creates and transfers ownership to pt
		//size_t nrows = 1280, ncols=720, nframes=25*60*1;
		size_t nrows = 3, ncols=5, nframes=7;
		h5::pt_t pt = h5::create<double>(fd, "stream of matrices", // 1280 x 720
				h5::max_dims{H5S_UNLIMITED,nrows,ncols},
				h5::chunk{1,2,2}  // chunk size controls h5::append internal cache size
			   	| h5::gzip{9} );
		Matrix<double> M(nrows,ncols);
//		arma::mat M(3,5), b(5,1);
		std::cout << "\n[matrix]:\n";
		int k=0;
	   	for( int i=0; i<nrows; i++)
			for(int j=0; j<ncols; j++) M(i,j) = ++k;

		for( int i = 1; i < nframes; i++){ 	//
			h5::append( pt, M);  			// save it into file
		}
	}
	*/
	/*
	{
		h5::pt_t pt = h5::create<double>(fd, "stream of vectors", // 1280 x 720
				h5::max_dims{H5S_UNLIMITED,5},
				h5::chunk{2,5}  // chunk size controls h5::append internal cache size
			   	| h5::gzip{9} );
		arma::vec V(5); for(int i=0; i<5; i++) V(i) = i;
		for( int i = 1; i < 7; i++)
			h5::append( pt, V);  			// save it into file
	}*/

// 442'076'646.72
	return 0;
}
