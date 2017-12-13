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
#include "event_listener.hpp"
#include <armadillo>
#include <h5cpp/core>
#include <h5cpp/mock.hpp>

namespace SomeNameSpace {
	typedef struct PodType {
		unsigned int   field1;
		double 			field2;
		float 			field3;
		char 			field4;
	} pod_type;
}
namespace sn = SomeNameSpace;
namespace h5 { namespace utils {
	template <> std::vector<sn::PodType> get_test_data( size_t n ){
		std::vector<sn::PodType> data;
		for(unsigned int i; i<n; i++)
			data.push_back(	sn::PodType( {i,0.0,0,0} ));
		return data;
	}
}}
H5CPP_REGISTER_STRUCT( sn::pod_type );
namespace h5{ namespace utils {
	// specialize template and create a compound data type
	template<> hid_t h5type<sn::pod_type>(){
			hid_t type = H5Tcreate(H5T_COMPOUND, sizeof (sn::pod_type));
			// layout
			H5Tinsert(type, "field1", 	HOFFSET(sn::pod_type, field1), H5T_NATIVE_UINT16);
			H5Tinsert(type, "field2", 	HOFFSET(sn::pod_type, field2), H5T_NATIVE_DOUBLE);
			H5Tinsert(type, "field3", 	HOFFSET(sn::pod_type, field3), H5T_NATIVE_FLOAT);
			H5Tinsert(type, "field4", 	HOFFSET(sn::pod_type, field4), H5T_NATIVE_CHAR);
            return type;
	};
}}

#include <h5cpp/create.hpp>
#include <h5cpp/read.hpp>
#include <h5cpp/write.hpp>

#define H5CPP_TEST_PRIMITIVE_TYPES unsigned char, unsigned short, unsigned int, unsigned long long int, char,short, int, long long int, float, double
#define H5CPP_TEST_STL_VECTOR_TYPES H5CPP_TEST_PRIMITIVE_TYPES, std::string
#define H5CPP_TEST_ARMADILLO_TYPES H5CPP_TEST_PRIMITIVE_TYPES


using namespace h5::utils;

/** test harness to open/close HDF5 file 
 *
 */ 
template <typename T> class AbstractTest
					: public ::testing::Test {
public:
	void SetUp() {
		dir = ::testing::UnitTest::GetInstance()->current_test_info()->name();
		type = std::string( TypeTraits<T>::name);
		name = dir + "/" + type;
		this->fd = h5::open("test.h5", H5F_ACC_RDWR );
	}
	void TearDown() {
		H5Fclose(fd);
	}
	std::string type;
	std::string name;
	std::string dir;
	hid_t fd; //< file descriptor
};
template <typename T> class ArmadilloTest : public AbstractTest<T>{};
template <typename T> class STLTest : public AbstractTest<T>{};
template <typename T> class STLTest_nostring : public AbstractTest<T>{};
template <typename T> class StructTypeTest : public AbstractTest<T>{};

typedef ::testing::Types<H5CPP_TEST_STL_VECTOR_TYPES> StlTypes;
typedef ::testing::Types<H5CPP_TEST_PRIMITIVE_TYPES> PrimitiveTypes;
typedef ::testing::Types<sn::PodType> StructType;

// instantiate for listed types
TYPED_TEST_CASE(STLTest, StlTypes);
TYPED_TEST_CASE(STLTest_nostring, PrimitiveTypes);
TYPED_TEST_CASE(ArmadilloTest, PrimitiveTypes);
TYPED_TEST_CASE(StructTypeTest, StructType);
TYPED_TEST(StructTypeTest, StructTypeCreate) {
	// PLAIN
	if( h5::create<TypeParam>(this->fd,this->name + " 1d",{3}) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 2d",{2,3} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 3d",{1,2,3}) < 0 ) ADD_FAILURE();
}
TYPED_TEST(StructTypeTest, StructTypeCreateChunked) {
	if( h5::create<TypeParam>(this->fd,this->name + " 1d chunk",{3000},{100} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 2d chunk",{2000,3},{100,1} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 3d chunk",{1000,2,3},{100,1,1} ) < 0 ) ADD_FAILURE();
}
TYPED_TEST(StructTypeTest, StructTypeCreateChunkedCompressed) {
	if( h5::create<TypeParam>(this->fd,this->name + " 1d chunk gzip",{3000},{100},9 ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 2d chunk gzip",{2000,3},{100,1},9 ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 3d chunk gzip",{1000,2,3},{100,1,1},9 ) < 0 ) ADD_FAILURE();
}
TYPED_TEST(StructTypeTest, StructTypeWrite) {
	std::vector<TypeParam> vec = get_test_data<TypeParam>(100);
	h5::write(this->fd,this->name, vec);
}
TYPED_TEST(StructTypeTest, StructTypeRead) {
	int n = 100;
	std::vector<TypeParam> vec = get_test_data<TypeParam>(n);
	h5::write(this->fd,this->name, vec);
	auto v = h5::read<std::vector<TypeParam>>(this->fd,this->name);
	for(int i=0; i<n; i++  ) EXPECT_EQ(v[i].field1, i);
}

TYPED_TEST(STLTest, CreateIntegral) {

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

TYPED_TEST(ArmadilloTest, WriteArmadillo) {

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


TYPED_TEST(STLTest, WriteSTL) {

	std::vector<TypeParam> vec = get_test_data<TypeParam>(100);
	// PLAIN
	h5::write(this->fd,this->name, vec);
}

TYPED_TEST(STLTest, ReadSTL) {

	std::vector<TypeParam> vec = get_test_data<TypeParam>(100);
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
	std::vector<TypeParam> vec = get_test_data<TypeParam>(20);

	h5::write(ds, vec, {0,0},{1,20} );

	H5Dclose(ds);
}

TYPED_TEST(STLTest, MultiDim_no_chunk) {
	hid_t ds = h5::create<TypeParam>(this->fd, this->name, {20,200,40}  );
	std::vector<TypeParam> vec = get_test_data<TypeParam>(20);

	h5::write(ds, vec, {0,0,0},{1,20,1} );

	H5Dclose(ds);
}

TYPED_TEST(STLTest, MultiDim_yes_chunk_gzip) {
	hid_t ds = h5::create<TypeParam>(this->fd, this->name, {20,200,40},{20,10,10}, 9 );
	std::vector<TypeParam> vec = get_test_data<TypeParam>(20);

	h5::write(ds, vec, {0,0,0},{1,20,1} );

	H5Dclose(ds);
}

/**
 * @brief clears error stack and sets FAILURE for google test framework
 *
 */
herr_t gtest_hdf5_error_handler (int a, void *unused) {
	hid_t es = H5Eget_current_stack();
	H5Eclear( es );
	ADD_FAILURE();
}


int main(int argc, char **argv) {
	
	testing::InitGoogleTest(&argc, argv);

	testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
	delete listeners.Release(listeners.default_result_printer());
  	listeners.Append(new MinimalistPrinter( argv[0] ) );
  	
	// make sure dataset exists, and is zapped
  	hid_t fd = h5::create("test.h5");   h5::close(fd);
	//install error handler that captures any h5 error
	hid_t es = H5Eget_current_stack();
	H5Eset_auto(H5E_DEFAULT,gtest_hdf5_error_handler, nullptr );

  	// will create/read/write/append to datasets in fd
	return RUN_ALL_TESTS();
}
