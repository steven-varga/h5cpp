/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */


#include <cstdint>
// this TU only needs to know of h5::fd_t 
// no `sandwitching` between <h5cpp/all> and <h5cpp/io> needed
#include <h5cpp/all>

// of course the function prototype definitions needs to be here, preferably factored out:

void test_01( const h5::fd_t& fd );
void test_02( const h5::fd_t& fd );
void test_03( const h5::fd_t& fd );
void test_04( const h5::fd_t& fd );


int main(){

	h5::fd_t fd = h5::create("example.h5",H5F_ACC_TRUNC);

	test_01( fd );
	test_02( fd );
	test_03( fd );
	test_04( fd );
}
