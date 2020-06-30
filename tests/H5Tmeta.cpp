/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include "utils/abstract.hpp"
#include <cstdlib>
#include <random>
#include "../h5cpp/H5meta.hpp"

namespace ns = h5::impl;

TEST(rank, string) {
    ASSERT_EQ(ns::rank<std::string>::value, 0);
}
TEST(rank, character_literal) {
    ASSERT_EQ(ns::rank<decltype("character literal")>::value, 0);
}
TEST(rank, character_array) {
    ASSERT_EQ(ns::rank<char[10][2]>::value, 1);
    ASSERT_EQ(ns::rank<char[10][2][4]>::value, 2);
    ASSERT_EQ(ns::rank<char[10][2][4][5]>::value, 3);
    ASSERT_EQ(ns::rank<char[10][2][4][5][3]>::value, 4);
}

template <typename T> struct H5M : public TypedTest<T> {};
typedef ::testing::Types<
        unsigned char, unsigned short, unsigned int, unsigned long long int,
		std::byte, signed char, signed short, signed int, signed long long int, float, double, h5::test::pod_t
> element_t;
typedef ::testing::Types<std::string, char> string_t;
TYPED_TEST_SUITE(H5M, element_t, h5::test::element_names_t);
TYPED_TEST_SUITE(H5Mstring, string_t, h5::test::element_names_t);

TYPED_TEST(H5M, rank){
    ASSERT_EQ(ns::rank<TypeParam>::value, 0);
    ASSERT_EQ(ns::rank<TypeParam[]>::value, 1);
    ASSERT_EQ(ns::rank<TypeParam[][10]>::value, 2);
    ASSERT_EQ(ns::rank<TypeParam[][10][11]>::value, 3);

    ASSERT_EQ(ns::rank<std::initializer_list<TypeParam>>::value, 1);
}

TEST(rank, character_literal_initializer_list) {
    ASSERT_EQ(ns::rank<std::initializer_list<char[10]>>::value, 1);
}
/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

