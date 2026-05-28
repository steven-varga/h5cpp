// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Translation unit #1.  Includes generated.h so this TU knows how to map the
// POD structs to HDF5 compound types; uses Armadillo to show that linalg
// containers and registered compound types coexist in the same TU.

#include <armadillo>
#include <h5cpp/all>
#include "generated.h"

#include <iostream>

void tu_01_linalg_and_create(const h5::fd_t& fd) {
    constexpr int CHUNK = 5;
    constexpr int NROWS = 4 * CHUNK;
    constexpr int NCOLS = 1 * CHUNK;

    // 1. A plain linalg write — no compound, no generated.h needed for this call.
    arma::imat M(NROWS, NCOLS, arma::fill::eye);
    h5::write(fd, "/linalg/armadillo", M);

    // 2. Create a 2-D compound dataset of `sn::example::record_t`, chunked + gzipped,
    //    ready for partial I/O later.  Possible because this TU saw generated.h.
    h5::create<sn::example::record_t>(fd, "/orm/chunked_2D",
        h5::current_dims{NROWS, NCOLS},
        h5::chunk{1, CHUNK} | h5::gzip{8});

    // 3. Unbounded dataset of the type-check record.
    h5::create<sn::typecheck::record_t>(fd, "/orm/typecheck",
        h5::max_dims{H5S_UNLIMITED});

    std::cout << "tu-01: wrote arma::imat(" << NROWS << "x" << NCOLS
              << ") and created two compound datasets\n";
}
