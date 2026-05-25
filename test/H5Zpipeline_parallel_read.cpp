/*
 * Copyright (c) 2026 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Tests for parallel decompression read path (pool_pipeline_t::read).
 * Issue #263.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>

#include <numeric>
#include <vector>

TEST_CASE("[#263] parallel decompression read with gzip + h5::threads{4}") {
    const char* path = "test-263-parallel-read-gzip.h5";
    std::remove(path);

    h5::fapl_t fapl = h5::threads{4};
    h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);

    std::vector<double> data(10'000);
    std::iota(data.begin(), data.end(), 0.0);

    h5::write(fd, "data", data,
              h5::current_dims{data.size()}, h5::max_dims{H5S_UNLIMITED},
              h5::chunk{1024} | h5::gzip{6}, h5::high_throughput);

    auto back = h5::read<std::vector<double>>(fd, "data");

    REQUIRE(back.size() == data.size());
    for (size_t i = 0; i < data.size(); ++i)
        CHECK(back[i] == doctest::Approx(data[i]));

    std::remove(path);
}

TEST_CASE("[#263] parallel decompression read with partial trailing chunk") {
    const char* path = "test-263-parallel-read-partial.h5";
    std::remove(path);

    h5::fapl_t fapl = h5::threads{4};
    h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);

    // 1,005 elements -> 10 full chunks of 100 + 1 partial chunk of 5
    std::vector<double> data(1'005);
    std::iota(data.begin(), data.end(), 0.0);

    h5::write(fd, "data", data,
              h5::current_dims{data.size()}, h5::max_dims{H5S_UNLIMITED},
              h5::chunk{100} | h5::gzip{6}, h5::high_throughput);

    auto back = h5::read<std::vector<double>>(fd, "data");

    REQUIRE(back.size() == data.size());
    for (size_t i = 0; i < data.size(); ++i)
        CHECK(back[i] == doctest::Approx(data[i]));

    std::remove(path);
}

TEST_CASE("[#263] parallel decompression read with single chunk") {
    const char* path = "test-263-parallel-read-single.h5";
    std::remove(path);

    h5::fapl_t fapl = h5::threads{4};
    h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);

    std::vector<double> data(100);
    std::iota(data.begin(), data.end(), 0.0);

    h5::write(fd, "data", data,
              h5::current_dims{data.size()}, h5::max_dims{H5S_UNLIMITED},
              h5::chunk{256} | h5::gzip{6}, h5::high_throughput);

    auto back = h5::read<std::vector<double>>(fd, "data");

    REQUIRE(back.size() == data.size());
    for (size_t i = 0; i < data.size(); ++i)
        CHECK(back[i] == doctest::Approx(data[i]));

    std::remove(path);
}

TEST_CASE("[#263] synchronous fallback when no pool is present") {
    const char* path = "test-263-fallback-sync.h5";
    std::remove(path);

    // No h5::threads -- pool is absent, read should fall back to synchronous.
    h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC);

    std::vector<double> data(1'000);
    std::iota(data.begin(), data.end(), 0.0);

    h5::write(fd, "data", data,
              h5::current_dims{data.size()}, h5::max_dims{H5S_UNLIMITED},
              h5::chunk{256} | h5::gzip{6}, h5::high_throughput);

    auto back = h5::read<std::vector<double>>(fd, "data");

    REQUIRE(back.size() == data.size());
    for (size_t i = 0; i < data.size(); ++i)
        CHECK(back[i] == doctest::Approx(data[i]));

    std::remove(path);
}
