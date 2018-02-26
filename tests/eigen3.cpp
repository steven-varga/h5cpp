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
#include <Eigen/Dense>

#include <h5cpp/core>
#include <h5cpp/mock.hpp>
#include <h5cpp/create.hpp>
#include <h5cpp/read.hpp>
#include <h5cpp/write.hpp>

#include "event_listener.hpp"
#include "abstract.h"



using namespace h5::utils;

template <typename T> class EigenArrayTest  : public AbstractTest<T>{};
template <typename T> class EigenMatrixTest : public AbstractTest<T>{};
template <typename T> class EigenMacroTest  : public AbstractTest<T>{};

typedef ::testing::Types<H5CPP_TEST_PRIMITIVE_TYPES> PrimitiveTypes;
TYPED_TEST_CASE(EigenMatrixTest, PrimitiveTypes);
TYPED_TEST_CASE(EigenArrayTest, PrimitiveTypes);
TYPED_TEST_CASE(EigenMacroTest, PrimitiveTypes);

TYPED_TEST(EigenMacroTest, EigenMacroTest) {

	Eigen::Matrix<TypeParam,Eigen::Dynamic,1> C(10);
	Eigen::Matrix<TypeParam,1,Eigen::Dynamic> R(10);
	Eigen::Matrix<TypeParam,Eigen::Dynamic,Eigen::Dynamic> M(10,50);
}

TYPED_TEST(EigenMatrixTest, EigenMatrixWrite) {

	Eigen::Matrix<TypeParam,Eigen::Dynamic,1> C(10);
	Eigen::Matrix<TypeParam,1,Eigen::Dynamic> R(10);
	Eigen::Matrix<TypeParam,Eigen::Dynamic,Eigen::Dynamic> M(10,50);

	h5::write(this->fd,this->name+".col", C);
	h5::write(this->fd,this->name+".row", R);
	h5::write(this->fd,this->name+".mat", M);
}

TYPED_TEST(EigenArrayTest, EigenArrayWrite) {
	Eigen::Array<TypeParam,Eigen::Dynamic,1> C(10);
	Eigen::Array<TypeParam,1,Eigen::Dynamic> R(10);
	Eigen::Array<TypeParam,Eigen::Dynamic,Eigen::Dynamic> M(10,50);

	// PLAIN
	h5::write(this->fd,this->name+".col", C);
	h5::write(this->fd,this->name+".row", R);
	h5::write(this->fd,this->name+".mat", M);
}


TYPED_TEST(EigenMatrixTest, EigenMatrixRead) {

	Eigen::Matrix<TypeParam,Eigen::Dynamic,1> C(1000);
	Eigen::Matrix<TypeParam,1,Eigen::Dynamic> R(1000);
	Eigen::Matrix<TypeParam,Eigen::Dynamic,Eigen::Dynamic> M(1000,50);

	h5::write(this->fd,this->name+".col", C);
	h5::write(this->fd,this->name+".row", R);
	h5::write(this->fd,this->name+".mat", M);

	{ // READ ENTIRE DATASET
	auto c = h5::read<Eigen::Matrix<TypeParam,Eigen::Dynamic,1>>(this->fd,this->name+".col");
	auto r = h5::read<Eigen::Matrix<TypeParam,Eigen::Dynamic,1>>(this->fd,this->name+".row");
	auto m = h5::read<Eigen::Matrix<TypeParam,Eigen::Dynamic,Eigen::Dynamic>>(this->fd,this->name+".mat");
	}
	{ //READ PARTIAL DATASET
	auto c = h5::read<Eigen::Matrix<TypeParam,Eigen::Dynamic,1>>(this->fd,this->name+".col",{100},{10});
	auto r = h5::read<Eigen::Matrix<TypeParam,Eigen::Dynamic,1>>(this->fd,this->name+".row",{0},{10});
	auto m = h5::read<Eigen::Matrix<TypeParam,Eigen::Dynamic,Eigen::Dynamic>>(this->fd,this->name+".mat",{0,0},{10,10});
	}
}
TYPED_TEST(EigenArrayTest, EigenArrayRead) {

	Eigen::Array<TypeParam,Eigen::Dynamic,1> C(1000);
	Eigen::Array<TypeParam,1,Eigen::Dynamic> R(1000);
	Eigen::Array<TypeParam,Eigen::Dynamic,Eigen::Dynamic> M(1000,50);

	h5::write(this->fd,this->name+".col", C);
	h5::write(this->fd,this->name+".row", R);
	h5::write(this->fd,this->name+".mat", M);

	{ // READ ENTIRE DATASET
	auto c = h5::read<Eigen::Array<TypeParam,Eigen::Dynamic,1>>(this->fd,this->name+".col");
	auto r = h5::read<Eigen::Array<TypeParam,Eigen::Dynamic,1>>(this->fd,this->name+".row");
	auto m = h5::read<Eigen::Array<TypeParam,Eigen::Dynamic,Eigen::Dynamic>>(this->fd,this->name+".mat");
	}
	{ //READ PARTIAL DATASET
	auto c = h5::read<Eigen::Array<TypeParam,Eigen::Dynamic,1>>(this->fd,this->name+".col",{100},{10});
	auto r = h5::read<Eigen::Array<TypeParam,Eigen::Dynamic,1>>(this->fd,this->name+".row",{0},{10});
	auto m = h5::read<Eigen::Array<TypeParam,Eigen::Dynamic,Eigen::Dynamic>>(this->fd,this->name+".mat",{0,0},{10,10});
	}
}



/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

