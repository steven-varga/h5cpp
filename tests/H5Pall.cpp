/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include "utils/abstract.hpp"

TEST(H5P, dcpl){
    h5::dcpl_t dcpl = h5::chunk{2,3} | h5::gzip(3) | h5::fill_value<int>(3);
}

H5CPP_BASIC_RUNNER( int argc, char**  argv );
