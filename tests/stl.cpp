/*
 * Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this  software  and associated documentation files (the "Software"), to deal in
 * the Software  without   restriction, including without limitation the rights to
 * use, copy, modify, merge,  publish,  distribute, sublicense, and/or sell copies
 * of the Software, and to  permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE  SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT  SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY  CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER
 * IN  AN  ACTION  OF  CONTRACT, TORT OR  OTHERWISE, ARISING  FROM,  OUT  OF  OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <gtest/gtest.h>
#include <armadillo>
#include <h5cpp/core>
#include <h5cpp/mock.hpp>

#include "event_listener.hpp"
#include "abstract.h"
#include <h5cpp/create.hpp>
#include <h5cpp/read.hpp>
#include <h5cpp/write.hpp>

template <typename T> class STLTest : public AbstractTest<T>{};
typedef ::testing::Types<H5CPP_TEST_STL_VECTOR_TYPES> StlTypes;

// instantiate for listed types
TYPED_TEST_CASE(STLTest, StlTypes);
TYPED_TEST(STLTest, WriteSTL) {

	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(100);
	// PLAIN
	h5::write(this->fd,this->name, vec);
}

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

TYPED_TEST(STLTest, 2DD_no_chunk) {
	hid_t ds = h5::create<TypeParam>(this->fd, this->name, {20,200}  );
	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(20);

	h5::write(ds, vec, {0,0},{1,20} );

	H5Dclose(ds);
}

TYPED_TEST(STLTest, MultiDim_no_chunk) {
	hid_t ds = h5::create<TypeParam>(this->fd, this->name, {20,200,40}  );
	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(20);

	h5::write(ds, vec, {0,0,0},{1,20,1} );

	H5Dclose(ds);
}

TYPED_TEST(STLTest, MultiDim_yes_chunk_gzip) {
	hid_t ds = h5::create<TypeParam>(this->fd, this->name, {20,200,40},{20,10,10}, 9 );
	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(20);

	h5::write(ds, vec, {0,0,0},{1,20,1} );

	H5Dclose(ds);
}


/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

