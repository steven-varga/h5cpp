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
#include <blaze/Math.h>

#include <h5cpp/core>
#include <h5cpp/mock.hpp>
#include <h5cpp/create.hpp>
#include <h5cpp/read.hpp>
#include <h5cpp/write.hpp>

#include "event_listener.hpp"
#include "abstract.h"


template<class T> using rowmat = blaze::DynamicMatrix<T,blaze::rowMajor>;
template<class T> using colmat = blaze::DynamicMatrix<T,blaze::columnMajor>;
template<class T> using rowvec = blaze::DynamicVector<T,blaze::rowVector>;
template<class T> using colvec = blaze::DynamicVector<T,blaze::columnVector>;

template <typename T> class BlazeMatrixTest : public AbstractTest<T>{};

typedef ::testing::Types<H5CPP_TEST_PRIMITIVE_TYPES> PrimitiveTypes;
TYPED_TEST_CASE(BlazeMatrixTest, PrimitiveTypes);

TYPED_TEST(BlazeMatrixTest, BlazeMatrixWrite) {

	colvec<TypeParam> C(10);
	rowvec<TypeParam> R(10);
	rowmat<TypeParam> RM(10,50);
	colmat<TypeParam> CM(10,50);

	h5::write(this->fd,this->name+".col", C);
	h5::write(this->fd,this->name+".row", R);
	h5::write(this->fd,this->name+".rmat", RM);
	h5::write(this->fd,this->name+".cmat", CM);
}

TYPED_TEST(BlazeMatrixTest, BlazeMatrixRead) {

	colvec<TypeParam> C(1000);
	rowvec<TypeParam> R(1000);
	rowmat<TypeParam> RM(1000,50);
	colmat<TypeParam> CM(1000,50);

	h5::write(this->fd,this->name+".col", C);
	h5::write(this->fd,this->name+".row", R);
	h5::write(this->fd,this->name+".rmat", RM);
	h5::write(this->fd,this->name+".cmat", CM);

	{ // READ ENTIRE DATASET
	auto c = h5::read<colvec<TypeParam>>(this->fd,this->name+".col");
	auto r = h5::read<rowvec<TypeParam>>(this->fd,this->name+".row");

	auto rm = h5::read<rowmat<TypeParam>>(this->fd,this->name+".rmat");
	auto cm = h5::read<rowmat<TypeParam>>(this->fd,this->name+".rmat");
	}
	{ //READ PARTIAL DATASET
	auto c = h5::read<colvec<TypeParam>>(this->fd,this->name+".col",{100},{10});
	auto r = h5::read<rowvec<TypeParam>>(this->fd,this->name+".row",{100},{10});

	auto rm = h5::read<rowmat<TypeParam>>(this->fd,this->name+".rmat",{0,0},{10,10});
	auto cm = h5::read<rowmat<TypeParam>>(this->fd,this->name+".rmat",{0,0},{10,10});
	}
}

/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

