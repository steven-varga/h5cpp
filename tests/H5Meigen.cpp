/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#define H5CPP_WRITE_DISPATCH_TEST__(T, msg){ \
    std::cout << "macro: " << msg << "\n"; \
}; \

#include <Eigen/Dense>
#define H5CPP_USE_EIGEN3

#include "utils/abstract.hpp"
#include "../h5cpp/H5Dcreate.hpp"
#include "../h5cpp/H5Dopen.hpp"
#include "../h5cpp/H5Dwrite.hpp"
#include "../h5cpp/H5Meigen.hpp"

namespace ns = h5::test;
template <typename T> struct H5Meigen : public TestWithOpenHDF5<T> {};
template <typename T> struct H5Meigen_write : public TestWithOpenHDF5<T> {};
template <typename T> struct H5Meigen_read : public TestWithOpenHDF5<T> {};

using element_t = ::testing::Types<short,int,long, unsigned short, unsigned int, unsigned long, float,double>;


TYPED_TEST_SUITE(H5Meigen, element_t, ns::element_names_t);
TYPED_TEST_SUITE(H5Meigen_write, element_t, ns::element_names_t);
TYPED_TEST_SUITE(H5Meigen_read, element_t, ns::element_names_t);

H5D_layout_t get_layout(const h5::ds_t& ds ){
    h5::dcpl_t dcpl{H5Dget_create_plist(ds)};
    return H5Pget_layout(dcpl);
}


// cartesian product of [element_t X all_t]
TYPED_TEST(H5Meigen, is_linalg) {
    using cartesian_product =  h5::test::eigen::all_t<TypeParam>;
    h5::meta::static_for<cartesian_product>( [&]( auto i ){
        using T = typename std::tuple_element<i, cartesian_product>::type;
        ASSERT_TRUE(h5::impl::is_linalg<T>::value);
    });
}

// cartesian product of [element_t X all_t]
TYPED_TEST(H5Meigen, has_data) {
    using cartesian_product =   h5::test::eigen::all_t<TypeParam>;
    h5::meta::static_for<cartesian_product>( [&]( auto i ){
        using T = typename std::tuple_element<i, cartesian_product>::type;
        ASSERT_TRUE(h5::meta::has_data<T>::value);
    });
}

// cartesian product of [element_t X all_t]
TYPED_TEST(H5Meigen, has_iterator) {
    using cartesian_product = h5::test::eigen::all_t<TypeParam>;
    h5::meta::static_for<cartesian_product>( [&]( auto i ){
        using T = typename std::tuple_element<i, cartesian_product>::type;
        ASSERT_FALSE(h5::meta::has_iterator<T>::value);
    });
}

// cartesian product of [element_t X all_t]
TYPED_TEST(H5Meigen, is_contiguous) {
    using dense_product  = h5::test::eigen::dense_t<TypeParam>;
    using sparse_product = h5::test::eigen::sparse_t<TypeParam>;
    h5::meta::static_for<dense_product>( [&]( auto i ){
        using T = typename std::tuple_element<i, dense_product>::type;
        ASSERT_TRUE(h5::impl::is_contiguous<T>::value);
    });
    h5::meta::static_for<sparse_product>( [&]( auto i ){
        using T = typename std::tuple_element<i, sparse_product>::type;
        //ASSERT_FALSE(h5::impl::is_contiguous<T>::value);
    });
}

// cartesian product of [element_t X all_t]
TYPED_TEST(H5Meigen, decay) {
    using dense_product  = h5::test::eigen::all_t<TypeParam>;
    h5::meta::static_for<dense_product>( [&]( auto i ){
        using T = typename std::tuple_element<i, dense_product>::type;
        using element_t = typename h5::impl::decay<T>::type;
        ::testing::StaticAssertTypeEq<element_t, TypeParam>();
    });
}
TYPED_TEST(H5Meigen_write, rowvec_data) {
    using dense_product  = h5::test::eigen::all_t<TypeParam>;
    auto data = h5::test::eigen::get_data<TypeParam, 4,10>();
    h5::meta::static_for<decltype(data)>( [&]( auto i ){
        auto obj = std::get<i>(data);
        using T = decltype(obj);
        std::string type = h5::test::name<T>::value;
        // fill data
        h5::count_t count = h5::impl::size(obj);
        TypeParam* ptr = h5::impl::data(obj);
        std::iota(ptr, ptr+static_cast<size_t>(count), 1);
        h5::write(this->fd, this->name +"/"+type, obj);
        size_t rows = 10, cols=20;

        switch(h5::impl::rank<T>::value){
            case 1: h5::write(this->fd, this->name +"-unlimited/" + type, obj,
                        h5::max_dims{H5S_UNLIMITED}); break;
            case 2:
                h5::write(this->fd, this->name + "-unlimited/" + type, obj, h5::max_dims{H5S_UNLIMITED,H5S_UNLIMITED}); break;
            default: break;
        }
    });
}
/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

