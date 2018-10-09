/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
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

int main(){

	try { // centrally used error handling
		std::vector<sn::example::Record> stream = h5::utils::get_test_data<sn::example::Record>(200);
		h5::fd_t fd = h5::create("example.h5",H5F_ACC_TRUNC);

		// implicit conversion from h5::ds_t to h5::pt_t makes it a breeze to create
		// packet_table from h5::open | h5::create calls,
		// The newly created h5::pt_t  stateful container caches the incoming data until
		// bucket filled. IO operations are at h5::chunk boundaries
		// or when resource is released. Last partial chunk handled as expected.
		//
		// compiler assisted introspection generates boilerplate, developer 
		// can focus on the idea, leaving boring details to machines 
		h5::pt_t pt = h5::create<sn::example::Record>(fd, "stream of struct",
				h5::max_dims{H5S_UNLIMITED}, h5::chunk{20} | h5::gzip{9} );

		for( auto record : stream )
			h5::append(pt, record);

	} catch ( const h5::error::any& e ){
		std::cerr << "ERROR:" << e.what();
	}
}
