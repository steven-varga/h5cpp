/*
 * Copyright (c) 2018-2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include "utils/abstract.hpp"
#include <cstdlib>
#include <random>
namespace ns = h5::test;

// Tests factorial of 0.
TEST(H5F, open_existing_rdwr) {
	try {
        {
		h5::fd_t fd = h5::create( "fopen_rdrw.h5", H5F_ACC_TRUNC);
        }
        h5::fd_t fd = h5::open("fopen_rdrw.h5", H5F_ACC_RDWR);
		EXPECT_GT( static_cast<hid_t>( fd ), -1 );
        hid_t id = H5Gcreate( static_cast<hid_t>(fd), "datatset", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT );
		EXPECT_GT( id, -1 );
        H5Gclose(id);
	} catch ( const h5::error::any& err ){
		ADD_FAILURE();
	}
}
TEST(H5F, open_existing_rdonly) {
	try {
        {
		h5::fd_t fd = h5::create( "fopen_rdonly.h5", H5F_ACC_TRUNC);
        }
        h5::fd_t fd = h5::open("fopen_rdonly.h5", H5F_ACC_RDONLY);
		EXPECT_GT( static_cast<hid_t>( fd ), -1 );
        h5::mute(); // next should fail
            hid_t id = H5Gcreate( static_cast<hid_t>(fd), "datatset", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT );
        h5::unmute();
		EXPECT_LT( id, 0 );
	} catch ( const h5::error::any& err ){
		ADD_FAILURE();
	}
}


/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/
