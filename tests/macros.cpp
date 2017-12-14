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


#include <armadillo>
#include <hdf5.h>
#include <gtest/gtest.h>
#include <h5cpp/mock.hpp>
#include <h5cpp/macros.h>

#include "event_listener.hpp"
#include "struct.h"
#include "abstract.h"

//using namespace h5::utils;

/** test harness to open/close HDF5 file 
 *
 */ 
template <typename T> class STLVectorTest
					: public ::testing::Test {
public:
	void SetUp() {
		hsize_t count[] = {2,3,4};
		object = h5::utils::ctor<std::vector<T>>(3, count );
		rank =  h5::utils::base<std::vector<T>>::rank;
		size = h5::utils::get_size( object );
		auto dims = h5::utils::get_dims( object );
		ptr = h5::utils::get_ptr( object );
		type = h5::utils::type_name<T>();
		name = dir + "/" + type;
	}

	//typedef typename h5::utils::TypeTraits< std::vector<T>>::basetype basetype;
	int size;
	size_t rank;
	std::vector<T> object;
	T* ptr;

	std::string type;
	std::string name;
	std::string dir;
};

template <typename T> class ArmaColVectorTest
					: public ::testing::Test {
public:
	void SetUp() {
		hsize_t count[] = {2,3,4};
		object = h5::utils::ctor<arma::Col<T>>(3, count );
		rank =  h5::utils::base<arma::Col<T>>::rank;
		size = h5::utils::get_size( object );
		auto dims = h5::utils::get_dims( object );
		ptr = h5::utils::get_ptr( object );
	}

	typedef typename h5::utils::base< arma::Col<T>>::type basetype;
	int size;
	size_t rank;
	arma::Col<T> object;
	T* ptr;
};
template <typename T> class ArmaRowVectorTest
					: public ::testing::Test {
public:
	void SetUp() {
		hsize_t count[] = {2,3,4};
		object = h5::utils::ctor<arma::Row<T>>(3, count );
		rank =  h5::utils::base<arma::Row<T>>::rank;
		size = h5::utils::get_size( object );
		auto dims = h5::utils::get_dims( object );
		ptr = h5::utils::get_ptr( object );
	}

	typedef typename h5::utils::base< arma::Row<T>>::type basetype;
	int size;
	size_t rank;
	arma::Row<T> object;
	T* ptr;
};

template <typename T> class ArmaMatTest
					: public ::testing::Test {
public:
	void SetUp() {
		hsize_t count[] = {2,3,4};
		object = h5::utils::ctor<arma::Mat<T>>(3, count );
		rank =  h5::utils::base<arma::Mat<T>>::rank;
		size = h5::utils::get_size( object );
		auto dims = h5::utils::get_dims( object );
		ptr = h5::utils::get_ptr( object );
	}

	typedef typename h5::utils::base< arma::Mat<T>>::type basetype;
	int size;
	size_t rank;
	arma::Mat<T> object;
	T* ptr;
};

template <typename T> class ArmaCubeTest
					: public ::testing::Test {
public:
	void SetUp() {
		hsize_t count[] = {2,3,4};
		object = h5::utils::ctor<arma::Cube<T>>(3, count );
		rank =  h5::utils::base<arma::Cube<T>>::rank;
		size = h5::utils::get_size( object );
		auto dims = h5::utils::get_dims( object );
		ptr = h5::utils::get_ptr( object );
	}

	typedef typename h5::utils::base< arma::Cube<T>>::type basetype;
	int size;
	size_t rank;
	arma::Cube<T> object;
	T* ptr;
};



typedef ::testing::Types< H5CPP_TEST_STL_VECTOR_TYPES> StlTypes;
TYPED_TEST_CASE(STLVectorTest, StlTypes);
TYPED_TEST(STLVectorTest, stl_vector ) {
	if( this->rank != H5CPP_RANK_VEC ) ADD_FAILURE();
	if( this->size != this->object.size() ) ADD_FAILURE();
	if( this->ptr != this->object.data() ) ADD_FAILURE();
}

typedef ::testing::Types< H5CPP_TEST_ARMADILLO_TYPES> ArmaTypes;
TYPED_TEST_CASE(ArmaColVectorTest, ArmaTypes);
TYPED_TEST(ArmaColVectorTest, arma_col_vector ) {
	if( this->rank != H5CPP_RANK_VEC ) ADD_FAILURE();
	if( this->size != this->object.size() ) ADD_FAILURE();
	if( this->ptr != this->object.memptr() ) ADD_FAILURE();
}

TYPED_TEST_CASE(ArmaRowVectorTest, ArmaTypes);
TYPED_TEST(ArmaRowVectorTest, arma_row_vector ) {
	if( this->rank != H5CPP_RANK_VEC ) ADD_FAILURE();
	if( this->size != this->object.size() ) ADD_FAILURE();
	if( this->ptr != this->object.memptr() ) ADD_FAILURE();
}

TYPED_TEST_CASE(ArmaMatTest, ArmaTypes);
TYPED_TEST(ArmaMatTest, arma_matrix ) {
	if( this->rank != H5CPP_RANK_MAT ) ADD_FAILURE();
	if( this->size != this->object.size() ) ADD_FAILURE();
	if( this->ptr != this->object.memptr() ) ADD_FAILURE();
}
TYPED_TEST_CASE(ArmaCubeTest, ArmaTypes);
TYPED_TEST(ArmaCubeTest, arma_matrix ) {
	if( this->rank != H5CPP_RANK_CUBE ) ADD_FAILURE();
	if( this->size != this->object.size() ) ADD_FAILURE();
	if( this->ptr != this->object.memptr() ) ADD_FAILURE();
}

/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

