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
#include "event_listener.hpp"
#include "abstract.h"

template <typename T> class StructTypeTest : public AbstractTest<T>{};
typedef ::testing::Types<sn::PodType> StructType;
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
	int n = 100;
	std::vector<sn::PodType> vec;
	for(unsigned int i; i<n; i++)
		vec.push_back(	sn::PodType( {i,0.0,0,0} ));

	h5::write(this->fd,this->name, vec);
	hid_t ds = h5::create<sn::PodType>(this->fd, this->name+".ptr",{100},{0} );
		h5::write(ds, vec.data(),{0},{10} );
	H5Dclose(ds);
}


TYPED_TEST(StructTypeTest, StructTypeRead) {
	int n = 100;
	std::vector<sn::PodType> vec;
	for(unsigned int i; i<n; i++)
		vec.push_back(	sn::PodType( {i,0.0,0,0} ));
	h5::write(this->fd,this->name, vec);
//	auto v = h5::read<std::vector<sn::PodType>>(this->fd,this->name);
//	for(int i=0; i<n; i++  ) v[i].field1 = 100;
}

/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

