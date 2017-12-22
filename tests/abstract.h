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
#include <h5cpp/core>
#include "event_listener.hpp"


template <typename T> class AbstractTest
					: public ::testing::Test {
public:
	void SetUp() {
		dir = ::testing::UnitTest::GetInstance()->current_test_info()->name();
		type = h5::utils::type_name<T>();
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

herr_t gtest_hdf5_error_handler (int a, void *unused) {
	hid_t es = H5Eget_current_stack();
	H5Eclear( es );
	ADD_FAILURE();
}

#ifdef H5CPP_CHRONO
	#define H5CPP_TEST_CHRONO_TYPES h5::chrono::duration, h5::chrono::ptime, h5::chrono::period, h5::chrono::iterator
#else
	#define H5CPP_TEST_CHRONO_TYPES 
#endif

#define H5CPP_TEST_PRIMITIVE_TYPES 								\
	unsigned char, unsigned short, unsigned int, unsigned long 	\
	long int, char,short, int, long long int, float, double 	\

#define H5CPP_TEST_STL_VECTOR_TYPES H5CPP_TEST_PRIMITIVE_TYPES, std::string
#define H5CPP_TEST_ARMADILLO_TYPES H5CPP_TEST_PRIMITIVE_TYPES


#define H5CPP_TEST_RUNNER( ARGC, ARGV ) 															\
int main( ARGC, ARGV) { 		 																	\
																									\
	testing::InitGoogleTest(&argc, argv); 															\
																									\
	testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners(); 		\
	delete listeners.Release(listeners.default_result_printer()); 									\
  	listeners.Append(new MinimalistPrinter( argv[0] ) ); 											\
																									\
	/* make sure dataset exists, and is zapped */ 													\
  	hid_t fd = h5::create("test.h5");   h5::close(fd); 												\
	/* install error handler that captures any h5 error 	*/ 										\
	hid_t es = H5Eget_current_stack(); 																\
	H5Eset_auto(H5E_DEFAULT,gtest_hdf5_error_handler, nullptr ); 									\
																									\
  	/* will create/read/write/append to datasets in fd */ 											\
	return RUN_ALL_TESTS(); 																		\
} 																									\


