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
TYPED_TEST(H5D,create_maxdims_unlimited) {
    // with unlimited dimension you can't have compact size
    h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name, h5::max_dims{10,H5S_UNLIMITED});
    ASSERT_EQ(H5D_CHUNKED, get_layout(ds));
}
TYPED_TEST(H5D,create_compact) {
    h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name, h5::current_dims{10});
    ASSERT_EQ(H5D_COMPACT, get_layout(ds));
}
TYPED_TEST(H5D,create_non_compact) {
    // this should be contiguous
    h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name, h5::current_dims{65535});
    ASSERT_EQ(H5D_CONTIGUOUS, get_layout(ds));
}

TYPED_TEST(H5D,create_current_and_max_dims) {
    h5::unmute();
        h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name, h5::current_dims{10}, h5::max_dims{100});
    h5::mute();
}
TYPED_TEST(H5D,create_chunked) {
    h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name,
        h5::current_dims{10}, h5::max_dims{100}, h5::chunk{10});
}

TYPED_TEST(H5D,create_fill_value) {
    h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name,
        h5::current_dims{10}, h5::max_dims{100}, h5::chunk{10});
}

TYPED_TEST(H5D,create_gzip) {
    h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name,
        h5::current_dims{10}, h5::max_dims{100}, h5::chunk{10} | h5::gzip{9});
}
TYPED_TEST(H5D,create_inf_chunk_is_current_dims) {
    h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name,
        h5::current_dims{13}, h5::max_dims{H5S_UNLIMITED});
        hid_t plist = H5Dget_create_plist(ds);
        h5::dims_t dims{0};
        H5Pget_chunk(plist, H5CPP_MAX_RANK, *dims );
        EXPECT_EQ(dims[0], 13); //chunk size must match to current dims
    H5Pclose(plist);
}
TYPED_TEST(H5D,create_inf_chunk_is_current_dims_2D) {
    h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name,
        h5::current_dims{50,100}, h5::max_dims{H5S_UNLIMITED,H5S_UNLIMITED});
        hid_t plist = H5Dget_create_plist(ds);
        h5::dims_t dims{0,0};
        H5Pget_chunk(plist, H5CPP_MAX_RANK, *dims );
        EXPECT_EQ(dims[0],  50); //chunk size must match to current dims
        EXPECT_EQ(dims[1], 100); //chunk size must match to current dims
    H5Pclose(plist);
}

TYPED_TEST(H5D, create_with_group_id) {
    // must be unique per type, or throws an error which will fail the test
    std::string group_name = "group-" + this->name;
    h5::gr_t gr{
                H5Gcreate(this->fd, group_name.data(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT )};
    h5::ds_t ds = h5::create<TypeParam>(gr, this->name,
        h5::current_dims{1000}, h5::max_dims{10000}, h5::chunk{100} );
}




/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/
