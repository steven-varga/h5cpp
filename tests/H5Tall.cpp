/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include "utils/abstract.hpp"
#include "../h5cpp/H5Dappend.hpp"
#include <cstdlib>
#include <random>
namespace ns = h5::test;

template <typename T> class H5TallTest : public ::testing::Test {};
typedef ::testing::Types<
#ifdef H5CPP_TEST_SHORT
    h5::fd_t
#else
    h5::fd_t, h5::ds_t, h5::pt_t, h5::at_t, h5::gr_t, h5::ob_t, h5::sp_t, h5::dt_t<int>,
    h5::acpl_t, h5::dapl_t, h5::dcpl_t, h5::tapl_t, h5::tcpl_t,
    h5::gapl_t, h5::gcpl_t, h5::lapl_t, h5::lcpl_t, h5::ocpl_t,
    h5::ocrl_t, h5::scpl_t
#endif
    > H5TallTypes;
TYPED_TEST_SUITE(H5TallTest, H5TallTypes, ns::element_names_t);
/*
::herr_t h5cpp_free_type( void* ptr){
    std::cout <<"free...";
    return 0; // failure -1
}
::H5I_type_t h5cpp_register_type(){
    return = H5Iregister_type(100,1, );
}
*/
//checks if ctor retuns valid hdf5 identifier
TYPED_TEST(H5TallTest, default_ctor) {
	h5::dt_t<TypeParam> dt;
	ASSERT_EQ( static_cast<::hid_t>(dt), H5I_UNINIT);
}
//check explicit conversion to hid
TYPED_TEST(H5TallTest, explicit_to_hid) {
	h5::dt_t<TypeParam> dt;
    ::hid_t id = static_cast<::hid_t>(dt);
	ASSERT_EQ(id, H5I_UNINIT);
}

//check implicit conversion to hid
TYPED_TEST(H5TallTest, implicit_to_hid) {
	h5::dt_t<TypeParam> dt;
	ASSERT_EQ(dt, H5I_UNINIT);
}

//check hid to h5::dt_t
TYPED_TEST(H5TallTest, from_hid) {
	h5::dt_t<TypeParam> dt{H5Tcopy(H5T_NATIVE_UINT)};
	ASSERT_EQ(H5Iget_ref(dt),1);
	ASSERT_TRUE(H5Iis_valid(dt));
}

// check if h5::copy maintains correct reference count
/*
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
H5CPP_ERROR_HANDLER_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/
