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
#include <dlib/matrix.h>
#include <h5cpp/core>
#include <h5cpp/mock.hpp>
#include <h5cpp/create.hpp>
#include <h5cpp/read.hpp>
#include <h5cpp/write.hpp>

#include "event_listener.hpp"
#include "abstract.h"

template<class T> using Matrix = dlib::matrix<T>;

using namespace h5::utils;

template <typename T> class DlibMatrixTest : public AbstractTest<T>{};

typedef ::testing::Types<H5CPP_TEST_PRIMITIVE_TYPES> PrimitiveTypes;
TYPED_TEST_CASE(DlibMatrixTest, PrimitiveTypes);

TYPED_TEST(DlibMatrixTest, DlibMatrixWrite) {
	Matrix<TypeParam> M(10,50);
	h5::write(this->fd,this->name+".mat", M);
}

TYPED_TEST(DlibMatrixTest, DlibMatrixRead) {

	Matrix<TypeParam> M(1000,50);
	h5::write(this->fd,this->name+".mat", M);

	{ // READ ENTIRE DATASET
	auto m = h5::read<Matrix<TypeParam>>(this->fd,this->name+".mat");
	}
	{ //READ PARTIAL DATASET
	auto m = h5::read<Matrix<TypeParam>>(this->fd,this->name+".mat",{0,0},{10,10});
	}
}

/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

