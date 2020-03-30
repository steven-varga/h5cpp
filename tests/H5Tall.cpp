/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#define GOOGLE_STRIP_LOG 1

#include <gtest/gtest.h>
// redefine to check private/protected members
#define private public
#define protected public

#include <h5cpp/core>
#include "struct.h"
#include "event_listener.hpp"
#include "abstract.h"
#include <h5cpp/io>
#include <sstream>

template <typename T> class H5TallTest : public AbstractTest<T>{};
typedef ::testing::Types<H5CPP_TEST_ALL_TYPES>H5TallTypes;

TYPED_TEST_CASE(H5TallTest, H5TallTypes);


//checks if ctor retuns valid hdf5 identifier
TYPED_TEST(H5TallTest, default_ctor) {
	h5::dt_t<TypeParam> dt;
	ASSERT_TRUE(H5Iis_valid(dt.handle));
}
//check explicit conversion to hid
TYPED_TEST(H5TallTest, explicit_to_hid) {
	h5::dt_t<TypeParam> dt;
	ASSERT_TRUE(static_cast<hid_t>(H5Iis_valid(dt)));
}
//check implicit conversion to hid
TYPED_TEST(H5TallTest, implicit_to_hid) {
	h5::dt_t<TypeParam> dt;
	ASSERT_TRUE(H5Iis_valid(dt));
}
//check hid to h5::dt_t
TYPED_TEST(H5TallTest, from_hid) {
	h5::dt_t<TypeParam> dt{H5Tcopy(H5T_NATIVE_UINT)};
	ASSERT_EQ(H5Iget_ref(dt),1);
	ASSERT_TRUE(H5Iis_valid(dt));
}

// check if h5::copy maintains correct reference count
TYPED_TEST(H5TallTest, copy) {
	h5::dt_t<TypeParam> dt;

	int ref_dt = H5Iget_ref(dt);
	hid_t hid = h5::copy(dt);
	ASSERT_TRUE(H5Iis_valid(hid));
	ASSERT_EQ(ref_dt + 1, H5Iget_ref(hid)); // must hold
}
// check basic ostream funcitonality
TYPED_TEST(H5TallTest, info) {
	h5::dt_t<TypeParam> dt;
	std::stringstream ss;
	ss << dt;
	ASSERT_GT(ss.str().size(), 0);
}


/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

