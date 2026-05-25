/*
 * Copyright (c) 2026 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Attribute type coverage tests — issue #11.
 * Covers arithmetic scalars, std::vector<T>, std::array<T,N>, std::string,
 * and POD struct round-trips via h5::awrite / h5::aread.
 */
#include <hdf5.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/H5Fcreate.hpp>
#include <h5cpp/H5Fopen.hpp>
#include <h5cpp/H5Dcreate.hpp>
#include <h5cpp/H5Dopen.hpp>
#include <h5cpp/H5Acreate.hpp>
#include <h5cpp/H5Aopen.hpp>
#include <h5cpp/H5Awrite.hpp>
#include <h5cpp/H5Aread.hpp>
#include "support/fixture.hpp"
#include <array>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Arithmetic scalar round-trips
// ---------------------------------------------------------------------------
TEST_CASE("awrite/aread arithmetic scalars") {
    h5::test::file_fixture_t f("test-h5aall-scalar.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{1});

    SUBCASE("int") {
        h5::awrite(ds, "v_int", 42);
        CHECK(h5::aread<int>(ds, "v_int") == 42);
    }
    SUBCASE("unsigned int") {
        h5::awrite(ds, "v_uint", 99u);
        CHECK(h5::aread<unsigned int>(ds, "v_uint") == 99u);
    }
    SUBCASE("long long") {
        h5::awrite(ds, "v_ll", 1234567890LL);
        CHECK(h5::aread<long long>(ds, "v_ll") == 1234567890LL);
    }
    SUBCASE("float") {
        h5::awrite(ds, "v_float", 1.5f);
        CHECK(h5::aread<float>(ds, "v_float") == doctest::Approx(1.5f));
    }
    SUBCASE("double") {
        h5::awrite(ds, "v_double", 3.14159);
        CHECK(h5::aread<double>(ds, "v_double") == doctest::Approx(3.14159));
    }
}

// ---------------------------------------------------------------------------
// std::string round-trip
// ---------------------------------------------------------------------------
TEST_CASE("awrite/aread std::string") {
    h5::test::file_fixture_t f("test-h5aall-string.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{1});

    h5::awrite(ds, "s", std::string("hello h5cpp"));
    std::string result = h5::aread<std::string>(ds, "s");
    CHECK(result == "hello h5cpp");
}

// ---------------------------------------------------------------------------
// std::vector<T> of arithmetic elements round-trip
// ---------------------------------------------------------------------------
TEST_CASE("awrite/aread std::vector of arithmetic") {
    h5::test::file_fixture_t f("test-h5aall-vector.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{1});

    SUBCASE("vector<int>") {
        std::vector<int> in = {10, 20, 30, 40};
        h5::awrite(ds, "v", in);
        auto out = h5::aread<std::vector<int>>(ds, "v");
        REQUIRE(out.size() == in.size());
        for (std::size_t i = 0; i < in.size(); ++i)
            CHECK(out[i] == in[i]);
    }
    SUBCASE("vector<double>") {
        std::vector<double> in = {1.1, 2.2, 3.3};
        h5::awrite(ds, "vd", in);
        auto out = h5::aread<std::vector<double>>(ds, "vd");
        REQUIRE(out.size() == in.size());
        for (std::size_t i = 0; i < in.size(); ++i)
            CHECK(out[i] == doctest::Approx(in[i]));
    }
    SUBCASE("vector<float>") {
        std::vector<float> in = {0.5f, 1.0f, 1.5f, 2.0f, 2.5f};
        h5::awrite(ds, "vf", in);
        auto out = h5::aread<std::vector<float>>(ds, "vf");
        REQUIRE(out.size() == in.size());
        for (std::size_t i = 0; i < in.size(); ++i)
            CHECK(out[i] == doctest::Approx(in[i]));
    }
}

// ---------------------------------------------------------------------------
// std::array<T,N> of arithmetic elements round-trip
// ---------------------------------------------------------------------------
TEST_CASE("awrite/aread std::array of arithmetic") {
    h5::test::file_fixture_t f("test-h5aall-array.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{1});

    SUBCASE("array<int,4>") {
        std::array<int,4> in = {1, 2, 3, 4};
        h5::awrite(ds, "ai", in);
        auto out = h5::aread<std::array<int,4>>(ds, "ai");
        REQUIRE(out.size() == in.size());
        for (std::size_t i = 0; i < in.size(); ++i)
            CHECK(out[i] == in[i]);
    }
    SUBCASE("array<float,3>") {
        std::array<float,3> in = {1.0f, 2.0f, 3.0f};
        h5::awrite(ds, "af", in);
        auto out = h5::aread<std::array<float,3>>(ds, "af");
        REQUIRE(out.size() == in.size());
        for (std::size_t i = 0; i < in.size(); ++i)
            CHECK(out[i] == doctest::Approx(in[i]));
    }
    SUBCASE("array<double,2>") {
        std::array<double,2> in = {2.71828, 3.14159};
        h5::awrite(ds, "ad", in);
        auto out = h5::aread<std::array<double,2>>(ds, "ad");
        REQUIRE(out.size() == in.size());
        for (std::size_t i = 0; i < in.size(); ++i)
            CHECK(out[i] == doctest::Approx(in[i]));
    }
}

// ---------------------------------------------------------------------------
// std::vector<std::string> — KNOWN LIMITATION (not tested here)
// awrite for vector<string> passes std::string* directly to H5Awrite, which
// expects const char** for variable-length strings.  Fixing this requires
// building a temporary const char*[] buffer from the vector before calling
// H5Awrite.  The same limitation exists for h5::write (dataset) — tracked
// separately.  Do not add a test here until the underlying write path is fixed.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// POD struct (compound type) — KNOWN LIMITATION (not tested here)
// h5::awrite/aread for compound structs requires a manual h5::create<T>()
// specialization (dt_t path) or C++26 auto-reflection.  Without either,
// H5CPP_REGISTER_STRUCT returns H5I_UNINIT and H5Acreate2 fails.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// initializer_list<T> convenience (write-only — no typed read needed)
// ---------------------------------------------------------------------------
TEST_CASE("awrite initializer_list<int>") {
    h5::test::file_fixture_t f("test-h5aall-initlist.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{1});

    h5::awrite(ds, "il", {10, 20, 30});
    auto out = h5::aread<std::vector<int>>(ds, "il");
    REQUIRE(out.size() == 3);
    CHECK(out[0] == 10);
    CHECK(out[1] == 20);
    CHECK(out[2] == 30);
}

// ---------------------------------------------------------------------------
// operator[] convenience syntax
// ---------------------------------------------------------------------------
TEST_CASE("ds[name] = value convenience write") {
    h5::test::file_fixture_t f("test-h5aall-opbracket.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{1});

    ds["answer"] = 42;
    CHECK(h5::aread<int>(ds, "answer") == 42);

    ds["greeting"] = std::string("h5cpp");
    CHECK(h5::aread<std::string>(ds, "greeting") == "h5cpp");
}
