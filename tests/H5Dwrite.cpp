/*
 * Copyright (c) 2018-2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include "utils/abstract.hpp"
#include "../h5cpp/H5Dcreate.hpp"

namespace ns = h5::test;
template <typename T> struct H5D : public TestWithOpenHDF5<T> {};
typedef ::testing::Types<
//char, int, long
unsigned char, unsigned short, unsigned int, unsigned long long int,
		char, short, int, long long int, float, double, h5::test::pod_t
> element_t;
TYPED_TEST_SUITE(H5D, element_t, ns::element_names_t);

H5D_layout_t get_layout(const h5::ds_t& ds ){
    h5::dcpl_t dcpl{H5Dget_create_plist(ds)};
    return H5Pget_layout(dcpl);
}

TYPED_TEST(H5D,write_basic) {
    h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name, h5::max_dims{10,H5S_UNLIMITED});
    ASSERT_EQ(H5D_CHUNKED, get_layout(ds));
}


/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/
