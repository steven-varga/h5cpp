/* Copyright (c) 2017-2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <gtest/gtest.h>
namespace h5::test{
	template <class T, class ...> struct name {
		static constexpr char const * value = "T";
	};
}

#include "../linalg/all"
#include "../../h5cpp/core"
#include "types.hpp"
#include "pod_t.hpp"
#include "names.hpp"
#include "listener.hpp"

template <typename T>
class TestWithOpenHDF5
	: public ::testing::Test {
protected:
	void SetUp() override {
        h5::mute();
		dir = ::testing::UnitTest::GetInstance()->current_test_info()->name();
		type = h5::name<T>::value;
		name = type + ":" + dir;
        try {
		    this->fd = h5::open("test.h5", H5F_ACC_RDWR );
        } catch (const h5::error::any& err) {
            this->fd = h5::create("test.h5", H5F_ACC_TRUNC);
        }
	}
	void TearDown() override {
        h5::unmute();
	}
	std::string type;
	std::string name;
	std::string dir;
	h5::fd_t fd; //< file descriptor
};



herr_t gtest_hdf5_error_handler (long int a, void *unused) {
	hid_t es = H5Eget_current_stack();
	H5Eclear( es );
	ADD_FAILURE();
	return es;
}
#define H5CPP_ERROR_HANDLER_RUNNER( ARGC, ARGV ) 															\
int main( ARGC, ARGV) { 		 																	\
	testing::InitGoogleTest(&argc, argv); 															\
	testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners(); 		\
	delete listeners.Release(listeners.default_result_printer()); 									\
  	listeners.Append(new MinimalistPrinter( argv[0] ) ); 											\
	hid_t es = H5Eget_current_stack(); 														        \
	H5Eset_auto(H5E_DEFAULT,gtest_hdf5_error_handler, nullptr );                                    \
	return RUN_ALL_TESTS(); 																		\
}                                                                                                   \


#define H5CPP_BASIC_RUNNER( ARGC, ARGV ) 															\
int main( ARGC, ARGV) { 		 																	\
	testing::InitGoogleTest(&argc, argv); 															\
	testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners(); 		\
	delete listeners.Release(listeners.default_result_printer()); 									\
  	listeners.Append(new MinimalistPrinter( argv[0] ) ); 											\
	return RUN_ALL_TESTS(); 																		\
}                                                                                                   \

