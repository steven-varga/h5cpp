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



using namespace h5::utils;
template <typename T> class ArmadilloTest : public AbstractTest<T>{};
typedef ::testing::Types<H5CPP_TEST_PRIMITIVE_TYPES> PrimitiveTypes;
TYPED_TEST_CASE(ArmadilloTest, PrimitiveTypes);



TYPED_TEST(ArmadilloTest, ArmadilloWrite) {

	arma::Col<TypeParam>  C(10);
	arma::Row<TypeParam>  R(10);
	arma::Mat<TypeParam>  M(10,50);
	arma::Cube<TypeParam> Q(10,50,30);

	// PLAIN
	h5::write(this->fd,this->name+".col", C);
	h5::write(this->fd,this->name+".row", R);
	h5::write(this->fd,this->name+".mat", M);
	h5::write(this->fd,this->name+".cube", Q);
}

TYPED_TEST(ArmadilloTest, ReadArmadillo) {

	arma::Col<TypeParam>  C(1000);
	arma::Row<TypeParam>  R(1000);
	arma::Mat<TypeParam>  M(1000,50);
	arma::Cube<TypeParam> Q(1000,50,30);

	// PLAIN
	h5::write(this->fd,this->name+".col", C);
	h5::write(this->fd,this->name+".row", R);
	h5::write(this->fd,this->name+".mat", M);
	h5::write(this->fd,this->name+".cube", Q);
	{ // READ ENTIRE DATASET
	auto c = h5::read<arma::Col<TypeParam>>(this->fd,this->name+".col");
	auto r = h5::read<arma::Row<TypeParam>>(this->fd,this->name+".row");
	auto m = h5::read<arma::Mat<TypeParam>>(this->fd,this->name+".mat");
	auto q = h5::read<arma::Cube<TypeParam>>(this->fd,this->name+".cube");
	}
	{ //READ PARTIAL DATASET
	auto c = h5::read<arma::Col<TypeParam>>(this->fd,this->name+".col",{100},{10} );
	auto r = h5::read<arma::Row<TypeParam>>(this->fd,this->name+".row",{0},{10});
	auto m = h5::read<arma::Mat<TypeParam>>(this->fd,this->name+".mat",{0,0},{10,10});
	auto q = h5::read<arma::Cube<TypeParam>>(this->fd,this->name+".cube",{0,0,0},{10,10,10});
	}

}


/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

