/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include "utils/abstract.hpp"
#include "../h5cpp/H5Dcreate.hpp"
struct payload_t {
    char field[10];
};

template<> h5::dt_t<payload_t> inline h5::create<payload_t>(){
    h5::dt_t<payload_t> tid{H5Tvlen_create(H5Tcopy(H5T_NATIVE_UINT))};
    return tid;
}
template <> struct h5::name<payload_t> {
    static constexpr char const * value = "payload_t";
};


namespace ns = h5::test;
template <typename T> struct H5T :  public TestWithOpenHDF5<T>  {};
typedef ::testing::Types<payload_t, h5::test::pod_t, int, float, double, float[7][3][5]> element_t;
TYPED_TEST_SUITE(H5T, element_t, ns::element_names_t);

TYPED_TEST(H5T, create) {
    // with unlimited dimension you can't have compact size
    h5::ds_t ds = h5::create<TypeParam>(this->fd, "custom-" + this->name, h5::max_dims{10});
}

/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

