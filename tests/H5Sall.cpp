/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include "utils/abstract.hpp"
#include <cstdlib>
#include <random>
#include "../h5cpp/H5Sall.hpp"


TEST(ctor, count) {
    h5::count count;
    ASSERT_EQ(0, 0);
}


/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

