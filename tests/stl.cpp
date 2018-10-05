/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#define GOOGLE_STRIP_LOG 1

#include <gtest/gtest.h>
#include <armadillo>
#include <h5cpp/core>

#include "event_listener.hpp"
#include "abstract.h"
#include <h5cpp/io>

template <typename T> class STLTest : public AbstractTest<T>{};
typedef ::testing::Types<H5CPP_TEST_STL_VECTOR_TYPES> StlTypes;

// instantiate for listed types
TYPED_TEST_CASE(STLTest, StlTypes);
TYPED_TEST(STLTest, stl_write) {

	std::vector<TypeParam> vec(10); // = h5::utils::get_test_data<TypeParam>(10);
	std::iota( std::begin(vec), std::end(vec), 1);
	// PLAIN: create + write
	h5::write(this->fd,this->name, vec,  h5::stride{3}, h5::max_dims{H5S_UNLIMITED}  );
}
/*
TYPED_TEST(STLTest, ReadSTL) {

	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(100);
	h5::write(this->fd,this->name, vec);
	{  // READ
		std::vector<TypeParam> data =  h5::read<std::vector<TypeParam>>(this->fd,this->name );
	}
	{ // PARTIAL READ 
		std::vector<TypeParam> data =  h5::read<std::vector<TypeParam>>(this->fd,this->name,{50},{10} );
	}
}
*/

TYPED_TEST(STLTest, stl_create_write_continous) {
	auto ds = h5::create<TypeParam>(this->fd, this->name, h5::current_dims{20}  );
	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(20);
	h5::write(ds, vec );
}
TYPED_TEST(STLTest, stl_create_write_dcpl) {
	auto ds = h5::create<TypeParam>(this->fd, this->name, h5::current_dims{20},
		 h5::chunk{2} | h5::gzip{4} ) ;
	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(20);
	h5::write(ds, vec );
}
TYPED_TEST(STLTest, stl_create_write) {
	auto ds = h5::create<TypeParam>(this->fd, this->name, h5::current_dims{20},
		 h5::chunk{2} | h5::gzip{4} ) ;
	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(20);
	h5::write(ds, vec );
}

TYPED_TEST(STLTest, stl_partial) {
	auto ds = h5::create<TypeParam>(this->fd, this->name, h5::current_dims{20,200,40}, h5::chunk{20,1,1} );
	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(20);
	h5::write(ds, vec, h5::offset{0,0,0}, h5::count{1,20,1}, h5::stride{1,1,1} );
}

TYPED_TEST(STLTest, stl_multidim_gzip) {
	// block write with offset 
	auto ds = h5::create<TypeParam>(this->fd, this->name, h5::current_dims{20,200,40}, h5::chunk{20,10,10} | h5::gzip{9} );
	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(20);

	h5::write(ds, vec, h5::offset{2,3,0}, h5::count{4,5,1});

	H5Dclose(ds);
}


/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

