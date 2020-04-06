/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#define DEBUG

#include "utils/abstract.hpp"
#include "../h5cpp/H5Dappend.hpp"
#include <cstdlib>
#include <random>
namespace ns = h5::test;

template <typename T> class H5TallTest : public ::testing::Test {};
typedef ::testing::Types<
#ifdef H5CPP_TEST_SHORT
    h5::fd_t
//h5::dt_t<int>
#else // dt_t<T> is tested in H5Tall.cpp
    h5::fd_t, h5::ds_t, h5::pt_t, h5::at_t, h5::gr_t, h5::ob_t, h5::sp_t,
    h5::acpl_t, h5::dapl_t, h5::dcpl_t, h5::tapl_t, h5::tcpl_t,
    h5::gapl_t, h5::gcpl_t, h5::lapl_t, h5::lcpl_t, h5::ocpl_t,
    h5::ocrl_t, h5::scpl_t
#endif
    > H5TallTypes;
TYPED_TEST_SUITE(H5TallTest, H5TallTypes, ns::element_names_t);
/*
herr_t h5cpp_free_type( void* ptr){
    std::cout <<"free...";
    return 0; // failure -1
}
H5I_type_t h5cpp_register_type(){
    return = H5Iregister_type(100,1, );
}
*/
//default ctors are different for ds_t<T> 
// as they are initialized with H5T_NATIVE_???
template <class T, class... > struct check {
    static void default_ctor() {
        T id;
        ASSERT_EQ( static_cast<hid_t>(id), H5I_UNINIT);
    }
};
template <class T> struct check<h5::dt_t<T>> {
    static void default_ctor() {
        h5::dt_t<T> id;
        ASSERT_NE( static_cast<hid_t>(id), H5I_UNINIT);
    }
};
//checks if ctor returns valid hdf5 identifier
TYPED_TEST(H5TallTest, default_ctor) {
    check<TypeParam>::default_ctor();
}

//check explicit conversion to hid
TYPED_TEST(H5TallTest, explicit_to_hid) {
	TypeParam dt;
    ::hid_t id = static_cast<::hid_t>(dt);
	ASSERT_EQ(id, dt.handle);
}

//check implicit conversion to hid
TYPED_TEST(H5TallTest, implicit_to_hid) {
	TypeParam dt;
	ASSERT_EQ(dt, dt.handle);
}

/*

// check if h5::copy maintains correct reference count
TYPED_TEST(H5TallTest, copy_ctor) {
	TypeParam id_1;
	int ref_1 = H5Iget_ref(id_1);
    TypeParam id_2(id_1);
	int ref_3 = H5Iget_ref(id_1);
	int ref_2 = H5Iget_ref(id_2);
    std::cout << ref_1 << " " << ref_2 <<" "<<ref_3<<"\n";
    ASSERT_EQ(ref_2, ref_3);
    ASSERT_EQ(ref_1+1, ref_3);
}
// check basic ostream funcitonality
TYPED_TEST(H5TallTest, info) {
	h5::dt_t<TypeParam> dt;
	std::stringstream ss;
	ss << dt;
	ASSERT_GT(ss.str().size(), 0);
}
*/
/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_ERROR_HANDLER_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/
