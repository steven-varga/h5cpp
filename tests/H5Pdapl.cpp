/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#define GOOGLE_STRIP_LOG 1

#include <gtest/gtest.h>
#include <armadillo>
#include <h5cpp/all>

#include "event_listener.hpp"
#include "abstract.h"

template <typename T> class IntegralTest : public AbstractTest<T>{};
typedef ::testing::Types<H5CPP_TEST_PRIMITIVE_TYPES> PrimitiveTypes;
TYPED_TEST_CASE(IntegralTest, PrimitiveTypes);


TYPED_TEST(IntegralTest,create_current_dims) { // checks out!!!
}


/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

