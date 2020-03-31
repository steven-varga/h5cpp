
/*
 * Copyright (c) 2018-2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include "utils/abstract.hpp"



#define check( name ) TEST(linalg, name) {  \
    EXPECT_TRUE(1);             			\
}                                          	\

// see CMakeList.txt 
check(EIGEN3)
check(ARMADILLO)
check(UBLAS)
check(DLIB)
check(BLAZE)
check(ITPP)
check(BLITZ)

/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

