/*
 * Copyright (c) 2026 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Reachable edge paths consolidated for coverage — issue #285.
 *   - H5Awrite : re-write existing attribute, initializer_list overload,
 *                ds_t::operator[] / at_t::operator= sugar.
 *   - H5Dcreate: implicit auto-chunk when max_dims is unlimited but no
 *                explicit chunk/dcpl was supplied.
 *   - H5capi   : createds() chunked + high_throughput cache-wiring branch.
 *   - H5Zall   : fletcher32 SIMD batch loop (input >= 360 16-bit words).
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>
#include <h5cpp/H5Zpipeline.hpp>
#include "support/fixture.hpp"

#include <numeric>
#include <vector>

TEST_CASE("awrite re-writes an existing attribute (open path)") {
    h5::test::file_fixture_t f("test-285-attr-rewrite.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{1});
    h5::awrite(ds, "v", 1);
    h5::awrite(ds, "v", 7);                 // H5Aexists > 0 -> open existing
    CHECK(h5::aread<int>(ds, "v") == 7);
}

TEST_CASE("awrite initializer_list overload") {
    h5::test::file_fixture_t f("test-285-attr-initlist.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{1});
    h5::awrite<int>(ds, "il", {1, 2, 3, 4});
    auto back = h5::aread<std::vector<int>>(ds, "il");
    REQUIRE(back.size() == 4);
    CHECK(back[3] == 4);
}

TEST_CASE("ds_t::operator[] attribute assignment sugar") {
    h5::test::file_fixture_t f("test-285-attr-subscript.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{1});
    ds["answer"] = 42;
    CHECK(h5::aread<int>(ds, "answer") == 42);
}

TEST_CASE("create with unlimited max_dims auto-chunks without explicit chunk") {
    h5::test::file_fixture_t f("test-285-autochunk.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "auto",
        h5::current_dims_t{10}, h5::max_dims_t{H5S_UNLIMITED});
    h5::dcpl_t dcpl = h5::get_dcpl(ds);
    CHECK(H5Pget_layout(static_cast<hid_t>(dcpl)) == H5D_CHUNKED);
}

TEST_CASE("createds wires the basic_pipeline cache for chunked high_throughput") {
    h5::test::file_fixture_t f("test-285-createds-ht.h5");
    // high_throughput supplied at create time so createds() takes the
    // H5D_CHUNKED + H5CPP_DAPL_HIGH_THROUGHPUT branch.
    h5::ds_t ds = h5::create<double>(f.fd, "ht",
        h5::current_dims_t{100}, h5::chunk{10} | h5::gzip{6}, h5::high_throughput);
    CHECK(H5Iis_valid(static_cast<hid_t>(ds)) > 0);
}

TEST_CASE("fletcher32 checksum SIMD batch loop (large chunk)") {
    // 2000 ints = 8000 bytes = 4000 16-bit words, well past the 360-word
    // batch threshold, so the unrolled accumulation loop runs.
    h5::test::file_fixture_t f("test-285-fletcher32-batch.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "f32",
        h5::current_dims_t{2000}, h5::max_dims_t{H5S_UNLIMITED},
        h5::chunk{2000} | h5::fletcher32);
    h5::dcpl_t dcpl = h5::get_dcpl(ds);
    h5::impl::basic_pipeline_t pipeline;
    pipeline.set_cache(dcpl, sizeof(int));

    std::vector<int> data(2000);
    std::iota(data.begin(), data.end(), 0);

    h5::offset_t offset{0};
    h5::stride_t stride{1};
    h5::block_t  block{1};
    h5::count_t  count{2000};

    pipeline.write(ds, offset, stride, block, count, h5::default_dxpl, data.data());

    std::vector<int> rb(2000);
    pipeline.read(ds, offset, stride, block, count, h5::default_dxpl, rb.data());
    for (size_t i = 0; i < data.size(); ++i)
        CHECK(rb[i] == data[i]);
}
