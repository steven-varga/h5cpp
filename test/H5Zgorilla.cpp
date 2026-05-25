#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "support/common.hpp"
#include "support/fixture.hpp"
#include <h5cpp/io>
#include <h5cpp/H5Zall.hpp>
#include <h5cpp/H5Pall.hpp>

#include <vector>
#include <cmath>
#include <numeric>

namespace {

std::vector<float> smooth_floats(size_t n) {
    std::vector<float> data(n);
    for (size_t i = 0; i < n; ++i)
        data[i] = static_cast<float>(std::sin(i * 0.01) * 100.0);
    return data;
}

std::vector<double> smooth_doubles(size_t n) {
    std::vector<double> data(n);
    for (size_t i = 0; i < n; ++i)
        data[i] = std::sin(i * 0.01) * 100.0;
    return data;
}

std::vector<float> identical_floats(size_t n) {
    std::vector<float> data(n, 3.14159265f);
    return data;
}

std::vector<double> random_doubles(size_t n) {
    std::vector<double> data(n);
    for (size_t i = 0; i < n; ++i)
        data[i] = static_cast<double>(i) * 1.61803398875;
    return data;
}

} // anonymous namespace

TEST_CASE("H5Z gorilla callback round-trip float32") {
    const auto input = smooth_floats(1024);
    const size_t in_bytes = input.size() * sizeof(float);
    const unsigned params[] = {4};

    std::vector<unsigned char> encoded(in_bytes * 2 + 256);
    const size_t enc_size = h5::impl::filter::gorilla(
        encoded.data(), input.data(), in_bytes, 0, 1, params);

    REQUIRE(enc_size > 0);
    std::vector<float> decoded(input.size());
    const size_t dec_size = h5::impl::filter::gorilla(
        decoded.data(), encoded.data(), enc_size, H5Z_FLAG_REVERSE, 0, nullptr);

    REQUIRE(dec_size == in_bytes);
    CHECK(std::memcmp(decoded.data(), input.data(), in_bytes) == 0);
}

TEST_CASE("H5Z gorilla callback round-trip float64") {
    const auto input = smooth_doubles(1024);
    const size_t in_bytes = input.size() * sizeof(double);
    const unsigned params[] = {8};

    std::vector<unsigned char> encoded(in_bytes * 2 + 256);
    const size_t enc_size = h5::impl::filter::gorilla(
        encoded.data(), input.data(), in_bytes, 0, 1, params);

    REQUIRE(enc_size > 0);
    std::vector<double> decoded(input.size());
    const size_t dec_size = h5::impl::filter::gorilla(
        decoded.data(), encoded.data(), enc_size, H5Z_FLAG_REVERSE, 0, nullptr);

    REQUIRE(dec_size == in_bytes);
    CHECK(std::memcmp(decoded.data(), input.data(), in_bytes) == 0);
}

TEST_CASE("H5Z gorilla callback compresses identical values") {
    const auto input = identical_floats(1024);
    const size_t in_bytes = input.size() * sizeof(float);
    const unsigned params[] = {4};

    std::vector<unsigned char> encoded(in_bytes * 2 + 256);
    const size_t enc_size = h5::impl::filter::gorilla(
        encoded.data(), input.data(), in_bytes, 0, 1, params);

    REQUIRE(enc_size > 0);
    CHECK(enc_size < in_bytes);
}

TEST_CASE("H5Z gorilla callback compresses smooth data") {
    const auto input = smooth_doubles(4096);
    const size_t in_bytes = input.size() * sizeof(double);
    const unsigned params[] = {8};

    std::vector<unsigned char> encoded(in_bytes * 2 + 256);
    const size_t enc_size = h5::impl::filter::gorilla(
        encoded.data(), input.data(), in_bytes, 0, 1, params);

    REQUIRE(enc_size > 0);
    // Smooth data should compress significantly (typically < 50%)
    CHECK(enc_size < in_bytes);
}

TEST_CASE("H5Z gorilla callback falls back when no element size given") {
    // Without element_size params, filter falls back to memcpy
    std::vector<unsigned char> input(64, 0xAB);
    std::vector<unsigned char> encoded(input.size() * 2);

    const size_t enc_size = h5::impl::filter::gorilla(
        encoded.data(), input.data(), input.size(), 0, 0, nullptr);

    REQUIRE(enc_size == input.size());
    CHECK(std::memcmp(encoded.data(), input.data(), input.size()) == 0);
}

TEST_CASE("h5::gorilla property instantiates") {
    h5::gorilla g4{4};
    h5::gorilla g8{8};
    h5::gorilla g0; // auto-detect element size
    CHECK(true);
}

TEST_CASE("h5::gorilla write/read round-trip via native HDF5") {
    h5::test::file_fixture_t f("test-gorilla-native.h5");

    const auto input = smooth_floats(256);

    h5::ds_t ds = h5::create<float>(f.fd, "smooth_floats", h5::current_dims_t{256},
        h5::chunk{64} | h5::gorilla{4});

    h5::write(ds, input.data(), h5::count{input.size()});

    std::vector<float> readback(input.size());
    h5::read(ds, readback.data(), h5::count{input.size()});

    CHECK(std::memcmp(readback.data(), input.data(), input.size() * sizeof(float)) == 0);
}

TEST_CASE("h5::gorilla write/read round-trip double via native HDF5") {
    h5::test::file_fixture_t f("test-gorilla-native-dbl.h5");

    const auto input = smooth_doubles(256);

    h5::ds_t ds = h5::create<double>(f.fd, "smooth_doubles", h5::current_dims_t{256},
        h5::chunk{64} | h5::gorilla{8});

    h5::write(ds, input.data(), h5::count{input.size()});

    std::vector<double> readback(input.size());
    h5::read(ds, readback.data(), h5::count{input.size()});

    CHECK(std::memcmp(readback.data(), input.data(), input.size() * sizeof(double)) == 0);
}
