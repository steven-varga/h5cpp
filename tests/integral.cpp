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
#include <h5cpp/create.hpp>
#include <h5cpp/read.hpp>
#include <h5cpp/write.hpp>

#include "event_listener.hpp"
#include "abstract.h"

template <typename T> class IntegralTest : public AbstractTest<T>{};
typedef ::testing::Types<H5CPP_TEST_PRIMITIVE_TYPES> PrimitiveTypes;
TYPED_TEST_CASE(IntegralTest, PrimitiveTypes);

TYPED_TEST(IntegralTest,IntegralCreate) {

	// PLAIN
	if( h5::create<TypeParam>(this->fd,this->name + " 1d",{3}) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 2d",{2,3} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 3d",{1,2,3}) < 0 ) ADD_FAILURE();
	// CHUNKED
	if( h5::create<TypeParam>(this->fd,this->name + " 1d chunk",{3000},{100} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 2d chunk",{2000,3},{100,1} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 3d chunk",{1000,2,3},{100,1,1} ) < 0 ) ADD_FAILURE();
	// CHUNKED COMPRESSION: GZIP 9
	if( h5::create<TypeParam>(this->fd,this->name + " 1d chunk gzip",{3000},{100},9 ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 2d chunk gzip",{2000,3},{100,1},9 ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 3d chunk gzip",{1000,2,3},{100,1,1},9 ) < 0 ) ADD_FAILURE();
}

TYPED_TEST(IntegralTest,IntegralPointerWrite) {
	TypeParam data[100]={0};
	hid_t ds = h5::create<TypeParam>(this->fd, this->name+".ptr",{100},{} );
		h5::write(ds, data,{0},{100} );
	H5Dclose(ds);
}
TYPED_TEST(IntegralTest,IntegralPointerRead) {
	TypeParam data[100]={0};
	hid_t ds = h5::create<TypeParam>(this->fd, this->name+".ptr",{100},{} );
		h5::write(ds, data,{0},{100} );
		h5::read<TypeParam>(ds,data,{5},{20} );
	H5Dclose(ds);
	h5::read<TypeParam>(this->fd,this->name+".ptr",data,{5},{20} );
}

/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

