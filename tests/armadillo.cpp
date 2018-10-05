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
#define GOOGLE_STRIP_LOG 1

#include <gtest/gtest.h>
#include <armadillo>
#include <h5cpp/all>

#include "event_listener.hpp"
#include "abstract.h"



using namespace h5::utils;
template <typename T> class ArmadilloTest : public AbstractTest<T>{};
//typedef ::testing::Types<H5CPP_TEST_PRIMITIVE_TYPES> PrimitiveTypes;
typedef ::testing::Types<short> PrimitiveTypes;
TYPED_TEST_CASE(ArmadilloTest, PrimitiveTypes);
/*
TYPED_TEST(ArmadilloTest, ArmadilloWrite) {

	arma::Col<TypeParam>  C(10); 	 C.ones();
	arma::Row<TypeParam>  R(10);
	arma::Mat<TypeParam>  M(3,5);    M.ones();
	arma::Cube<TypeParam> Q(3,4,5);  Q.ones();
	// PLAIN
	h5::write(this->fd,this->name+".col", C);
	h5::write(this->fd,this->name+".row", R);

	h5::write(this->fd,this->name+".col unlim", C, h5::max_dims{H5S_UNLIMITED});
	h5::write(this->fd,this->name+".col unlim, gzip | chunk", C, h5::max_dims{H5S_UNLIMITED}, h5::gzip{9} | h5::chunk{10});

	h5::write(this->fd,this->name+".mat", Q, h5::count{3,5,1}, h5::stride{2,2,1}, h5::offset{2,0,0} );
	h5::write(this->fd,this->name+".custom ", C, h5::count{1,10}, h5::current_dims{4,12}, h5::offset{2,2}, h5::max_dims{12,H5S_UNLIMITED} );
}

TYPED_TEST(ArmadilloTest, ReadArmadillo) {

	arma::Col<TypeParam>  C(3);        C.ones();
	arma::Row<TypeParam>  R(3);        R.ones();
	arma::Mat<TypeParam>  M(20,20);    for(int i=0; i < M.size(); i++ ) M[i] = i;
	arma::Cube<TypeParam> Q(3,5,7);    Q.ones();

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
	auto c = h5::read<arma::Col<TypeParam>>(this->fd,this->name+".col",h5::count{2}, h5::offset{1} );
	auto r = h5::read<arma::Row<TypeParam>>(this->fd,this->name+".row", h5::offset{0}, h5::count{3});
	//auto m = h5::read<arma::Mat<TypeParam>>(this->fd,this->name+".mat",
	//		h5::offset{0,0}, h5::stride{2,2}, h5::count{3,4}, h5::block{2,1});
	arma::Mat<TypeParam> m(8,6);
	h5::read(this->fd,this->name+".mat", m.memptr(), h5::stride{4,3}, h5::count{4,3}, h5::block{4,3} );
	m.print();
	}
}
*/
TYPED_TEST(ArmadilloTest, BlockReadArmadillo) {

	arma::Mat<TypeParam>  M(20,20);    for(int i=0; i < M.size(); i++ ) M[i] = i;
	arma::Mat<TypeParam> m(8,6);

	h5::write(this->fd,this->name+".mat", M);
	auto m_ = h5::read<arma::imat>(this->fd,this->name+".mat", h5::stride{3,3}, h5::count{2,2}, h5::block{2,2} );
	h5::read(this->fd,this->name+".mat", m.memptr(), h5::stride{3,3}, h5::count{2,2}, h5::block{2,2} );
	m.print();
}

/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

