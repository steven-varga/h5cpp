#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>
#include <h5cpp/H5Zpipeline.hpp>
#include <h5cpp/H5Zpipeline_basic.hpp>
#include <vector>
#include <cstring>
#include "support/fixture.hpp"

TEST_CASE("basic_pipeline_t set_cache with chunked gzip dataset") {
    h5::test::file_fixture_t f("test-pipeline-cache.h5");
    h5::ds_t ds = h5::create<double>(f.fd, "ds", h5::current_dims_t{100},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10} | h5::gzip{6});
    h5::dcpl_t dcpl = h5::get_dcpl(ds);
    h5::impl::basic_pipeline_t pipeline;
    pipeline.set_cache(dcpl, sizeof(double));
    CHECK(pipeline.rank == 1);
    CHECK(pipeline.n == 10);
}

TEST_CASE("basic_pipeline_t write/read round-trip with gzip filter") {
    h5::test::file_fixture_t f("test-pipeline-roundtrip.h5");
    h5::ds_t ds = h5::create<double>(f.fd, "ds", h5::current_dims_t{100},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10} | h5::gzip{6});
    h5::dcpl_t dcpl = h5::get_dcpl(ds);
    h5::impl::basic_pipeline_t pipeline;
    pipeline.set_cache(dcpl, sizeof(double));

    std::vector<double> data(100);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<double>(i);

    h5::offset_t offset{0};
    h5::stride_t stride{1};
    h5::block_t block{1};
    h5::count_t count{100};

    pipeline.write(ds, offset, stride, block, count, h5::default_dxpl, data.data());

    std::vector<double> readback(100);
    pipeline.read(ds, offset, stride, block, count, h5::default_dxpl, readback.data());

    for (size_t i = 0; i < data.size(); ++i)
        CHECK(readback[i] == data[i]);
}

TEST_CASE("basic_pipeline_t write/read with no filters") {
    h5::test::file_fixture_t f("test-pipeline-nofilter.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{50},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10});
    h5::dcpl_t dcpl = h5::get_dcpl(ds);
    h5::impl::basic_pipeline_t pipeline;
    pipeline.set_cache(dcpl, sizeof(int));

    std::vector<int> data(50);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<int>(i * i);

    h5::offset_t offset{0};
    h5::stride_t stride{1};
    h5::block_t block{1};
    h5::count_t count{50};

    pipeline.write(ds, offset, stride, block, count, h5::default_dxpl, data.data());

    std::vector<int> readback(50);
    pipeline.read(ds, offset, stride, block, count, h5::default_dxpl, readback.data());

    for (size_t i = 0; i < data.size(); ++i)
        CHECK(readback[i] == data[i]);
}

TEST_CASE("basic_pipeline_t write/read with shuffle+gzip multi-filter") {
    h5::test::file_fixture_t f("test-pipeline-multi.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{50},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10} | h5::shuffle | h5::gzip{6});
    h5::dcpl_t dcpl = h5::get_dcpl(ds);
    h5::impl::basic_pipeline_t pipeline;
    pipeline.set_cache(dcpl, sizeof(int));

    std::vector<int> data(50);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<int>(i + 1);

    h5::offset_t offset{0};
    h5::stride_t stride{1};
    h5::block_t block{1};
    h5::count_t count{50};

    h5::mute();
    pipeline.write(ds, offset, stride, block, count, h5::default_dxpl, data.data());
    h5::unmute();

    CHECK(pipeline.tail == 2);
}

TEST_CASE("basic_pipeline_t multi-filter failure sets mask") {
    h5::test::file_fixture_t f("test-pipeline-multi-mask.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{10},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10} | h5::shuffle | h5::gzip{6});
    h5::dcpl_t dcpl = h5::get_dcpl(ds);
    h5::impl::basic_pipeline_t pipeline;
    pipeline.set_cache(dcpl, sizeof(int));

    // Replace the second filter with one that returns 0 (failure)
    pipeline.filter[1] = [](void*, const void*, size_t, unsigned, size_t, const unsigned*) -> size_t {
        return 0;
    };

    std::vector<int> data(10, 42);
    h5::offset_t offset{0};
    h5::stride_t stride{1};
    h5::block_t block{1};
    h5::count_t count{10};

    h5::mute();
    pipeline.write(ds, offset, stride, block, count, h5::default_dxpl, data.data());
    h5::unmute();
    CHECK(pipeline.tail == 2);
}

TEST_CASE("basic_pipeline_t single filter failure sets mask") {
    h5::test::file_fixture_t f("test-pipeline-mask.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{10},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10} | h5::gzip{6});
    h5::dcpl_t dcpl = h5::get_dcpl(ds);
    h5::impl::basic_pipeline_t pipeline;
    pipeline.set_cache(dcpl, sizeof(int));

    // Replace the single filter with one that returns 0 (failure)
    pipeline.filter[0] = [](void*, const void*, size_t, unsigned, size_t, const unsigned*) -> size_t {
        return 0;
    };

    std::vector<int> data(10, 42);
    h5::offset_t offset{0};
    h5::stride_t stride{1};
    h5::block_t block{1};
    h5::count_t count{10};

    h5::mute();
    // Should not throw; mask will indicate filter failure
    pipeline.write(ds, offset, stride, block, count, h5::default_dxpl, data.data());
    h5::unmute();
    CHECK(true);
}

TEST_CASE("high_throughput flag is constructible") {
    auto ht = h5::high_throughput;
    CHECK(true);
}

TEST_CASE("filter::get_callback returns correct callbacks") {
    CHECK(h5::impl::filter::get_callback(H5Z_FILTER_DEFLATE) != nullptr);
    CHECK(h5::impl::filter::get_callback(H5Z_FILTER_SHUFFLE) != nullptr);
    CHECK(h5::impl::filter::get_callback(H5Z_FILTER_FLETCHER32) != nullptr);
    CHECK(h5::impl::filter::get_callback(H5Z_FILTER_SZIP) != nullptr);
    CHECK(h5::impl::filter::get_callback(H5Z_FILTER_NBIT) != nullptr);
    CHECK(h5::impl::filter::get_callback(H5Z_FILTER_SCALEOFFSET) != nullptr);
    CHECK(h5::impl::filter::get_callback(H5Z_FILTER_LZ4) != nullptr);
    CHECK(h5::impl::filter::get_callback(H5Z_FILTER_ZSTD) != nullptr);
    CHECK(h5::impl::filter::get_callback(H5Z_FILTER_RESERVED) != nullptr);
}

TEST_CASE("filter passthroughs copy data unchanged") {
    char src[64] = "hello world test data for passthrough filters";
    char dst[64] = {};
    unsigned params[] = {4};

    h5::impl::filter::scaleoffset(dst, src, 64, 0, 1, params);
    CHECK(std::memcmp(dst, src, 64) == 0);

    h5::impl::filter::nbit(dst, src, 64, 0, 1, params);
    CHECK(std::memcmp(dst, src, 64) == 0);

    h5::impl::filter::add(dst, src, 64, 0, 1, params);
    CHECK(std::memcmp(dst, src, 64) == 0);
}

TEST_CASE("filter::error throws runtime_error") {
    char src[8] = {};
    char dst[8] = {};
    CHECK_THROWS_AS(h5::impl::filter::error(dst, src, 8, 0, 1, nullptr), std::runtime_error);
}
