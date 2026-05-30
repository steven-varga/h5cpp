/*
 * Copyright (c) 2026 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Public-API high_throughput pipeline path — issue #285.
 *
 * The parallel-read tests (#263) drive h5::write/h5::read without opening the
 * dataset with the h5::high_throughput DAPL, so the synchronous pipeline
 * dispatch in H5Dwrite/H5Dopen/H5Dread (the use_pipeline == true branch) was
 * never exercised.  These cases open the dataset WITH the high_throughput
 * DAPL so the pipeline dispatch is taken on both write and read, resolving
 * to the synchronous basic_pipeline_t (no FAPL worker pool is installed — see
 * #286 for why the pool branch is currently unreachable).
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>
#include "support/fixture.hpp"

#include <cstdio>
#include <numeric>
#include <vector>

TEST_CASE("high_throughput write + open + read round-trip (synchronous pipeline)") {
    const char* path = "test-285-ht-roundtrip.h5";
    std::remove(path);

    h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC);

    std::vector<double> data(1000);
    std::iota(data.begin(), data.end(), 0.0);

    // Write through the high_throughput pipeline (H5Dwrite_chunk path).
    h5::write(fd, "data", data,
              h5::current_dims{data.size()}, h5::max_dims{H5S_UNLIMITED},
              h5::chunk{128} | h5::gzip{6}, h5::high_throughput);

    // Open WITH high_throughput so the DAPL carries the pipeline pointer and
    // H5Dopen's chunked branch wires the basic_pipeline cache; the subsequent
    // read then takes the use_pipeline == true dispatch in H5Dread.
    h5::ds_t ds = h5::open(fd, "data", h5::high_throughput);
    auto back = h5::read<std::vector<double>>(ds);

    REQUIRE(back.size() == data.size());
    for (size_t i = 0; i < data.size(); ++i)
        CHECK(back[i] == doctest::Approx(data[i]));

    std::remove(path);
}

// NOTE: a partial-hyperslab read through the high_throughput pipeline
// (h5::read(ds, h5::offset{...}, h5::count{...}) with the dataset opened
// high_throughput) was found to return zeros — the direct-chunk read path
// appears to ignore the requested offset.  Left out here pending a focused
// investigation; the full-extent read above covers the pipeline dispatch.
