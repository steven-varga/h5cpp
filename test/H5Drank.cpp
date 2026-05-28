/*
 * Copyright (c) 2026 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Focused round-trip tests for rank-4 through rank-7 C-arrays.
 * Issue #115: support rank-7 arrays in type engine and I/O layer.
 *
 * Strategy: use the raw-pointer API with explicit h5::current_dims / h5::count
 * to write and read back multi-dimensional arrays.  N=2 throughout, giving
 * sizes 16 / 32 / 64 / 128 elements for ranks 4–7.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>
#include <h5cpp/H5Tmeta.hpp>
#include "support/types.hpp"
#include <filesystem>
#include <cstring>
#include <numeric>

// ---------------------------------------------------------------------------
// Compile-time rank verification (issue #115 requirement)
// ---------------------------------------------------------------------------
static_assert(h5::meta::rank<double[2][2][2][2]>::value       == 4, "rank-4 mismatch");
static_assert(h5::meta::rank<double[2][2][2][2][2]>::value    == 5, "rank-5 mismatch");
static_assert(h5::meta::rank<double[2][2][2][2][2][2]>::value == 6, "rank-6 mismatch");
static_assert(h5::meta::rank<double[2][2][2][2][2][2][2]>::value == 7, "rank-7 mismatch");

// storage_representation must classify ranks 4–7 as array_element
static_assert(h5::meta::storage_representation_v<double[2][2][2][2]>
              == h5::meta::storage_representation_t::array_element, "rank-4 not array_element");
static_assert(h5::meta::storage_representation_v<double[2][2][2][2][2]>
              == h5::meta::storage_representation_t::array_element, "rank-5 not array_element");
static_assert(h5::meta::storage_representation_v<double[2][2][2][2][2][2]>
              == h5::meta::storage_representation_t::array_element, "rank-6 not array_element");
static_assert(h5::meta::storage_representation_v<double[2][2][2][2][2][2][2]>
              == h5::meta::storage_representation_t::array_element, "rank-7 not array_element");

// ---------------------------------------------------------------------------
// Helper: temp file with RAII cleanup
// ---------------------------------------------------------------------------
namespace {
    struct temp_file {
        std::string path;
        explicit temp_file(const std::string& name) {
            path = (std::filesystem::temp_directory_path() / name).string();
            std::filesystem::remove(path);
        }
        ~temp_file() {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
    };
}

// ---------------------------------------------------------------------------
// Rank-4 round-trip: double[2][2][2][2]  — 16 elements
// ---------------------------------------------------------------------------
TEST_CASE("[#115] rank-4 double C-array round-trip") {
    temp_file tf("h5drank_r4.h5");

    constexpr int N = 2;
    double write_buf[N][N][N][N];
    double read_buf[N][N][N][N];
    // fill with sequential values
    double* wp = reinterpret_cast<double*>(write_buf);
    for (int i = 0; i < 16; ++i) wp[i] = static_cast<double>(i) + 1.0;

    {
        h5::fd_t fd = h5::create(tf.path, H5F_ACC_TRUNC);
        auto ds = h5::create<double>(fd, "rank4",
            h5::current_dims{N, N, N, N});
        h5::write(ds, wp, h5::count{N, N, N, N});
    }

    {
        h5::fd_t fd = h5::open(tf.path, H5F_ACC_RDONLY);
        double* rp = reinterpret_cast<double*>(read_buf);
        h5::read<double>(fd, "rank4", rp, h5::count{N, N, N, N});
        for (int i = 0; i < 16; ++i)
            CHECK(rp[i] == doctest::Approx(wp[i]));
    }
}

// ---------------------------------------------------------------------------
// Rank-5 round-trip: double[2][2][2][2][2]  — 32 elements
// ---------------------------------------------------------------------------
TEST_CASE("[#115] rank-5 double C-array round-trip") {
    temp_file tf("h5drank_r5.h5");

    constexpr int N = 2;
    double write_buf[N][N][N][N][N];
    double read_buf[N][N][N][N][N];
    double* wp = reinterpret_cast<double*>(write_buf);
    for (int i = 0; i < 32; ++i) wp[i] = static_cast<double>(i) + 1.0;

    {
        h5::fd_t fd = h5::create(tf.path, H5F_ACC_TRUNC);
        auto ds = h5::create<double>(fd, "rank5",
            h5::current_dims{N, N, N, N, N});
        h5::write(ds, wp, h5::count{N, N, N, N, N});
    }

    {
        h5::fd_t fd = h5::open(tf.path, H5F_ACC_RDONLY);
        double* rp = reinterpret_cast<double*>(read_buf);
        h5::read<double>(fd, "rank5", rp, h5::count{N, N, N, N, N});
        for (int i = 0; i < 32; ++i)
            CHECK(rp[i] == doctest::Approx(wp[i]));
    }
}

// ---------------------------------------------------------------------------
// Rank-6 round-trip: double[2][2][2][2][2][2]  — 64 elements
// ---------------------------------------------------------------------------
TEST_CASE("[#115] rank-6 double C-array round-trip") {
    temp_file tf("h5drank_r6.h5");

    constexpr int N = 2;
    double write_buf[N][N][N][N][N][N];
    double read_buf[N][N][N][N][N][N];
    double* wp = reinterpret_cast<double*>(write_buf);
    for (int i = 0; i < 64; ++i) wp[i] = static_cast<double>(i) + 1.0;

    {
        h5::fd_t fd = h5::create(tf.path, H5F_ACC_TRUNC);
        auto ds = h5::create<double>(fd, "rank6",
            h5::current_dims{N, N, N, N, N, N});
        h5::write(ds, wp, h5::count{N, N, N, N, N, N});
    }

    {
        h5::fd_t fd = h5::open(tf.path, H5F_ACC_RDONLY);
        double* rp = reinterpret_cast<double*>(read_buf);
        h5::read<double>(fd, "rank6", rp, h5::count{N, N, N, N, N, N});
        for (int i = 0; i < 64; ++i)
            CHECK(rp[i] == doctest::Approx(wp[i]));
    }
}

// ---------------------------------------------------------------------------
// Rank-7 round-trip: double[2][2][2][2][2][2][2]  — 128 elements
// ---------------------------------------------------------------------------
TEST_CASE("[#115] rank-7 double C-array round-trip") {
    temp_file tf("h5drank_r7.h5");

    constexpr int N = 2;
    double write_buf[N][N][N][N][N][N][N];
    double read_buf[N][N][N][N][N][N][N];
    double* wp = reinterpret_cast<double*>(write_buf);
    for (int i = 0; i < 128; ++i) wp[i] = static_cast<double>(i) + 1.0;

    {
        h5::fd_t fd = h5::create(tf.path, H5F_ACC_TRUNC);
        auto ds = h5::create<double>(fd, "rank7",
            h5::current_dims{N, N, N, N, N, N, N});
        h5::write(ds, wp, h5::count{N, N, N, N, N, N, N});
    }

    {
        h5::fd_t fd = h5::open(tf.path, H5F_ACC_RDONLY);
        double* rp = reinterpret_cast<double*>(read_buf);
        h5::read<double>(fd, "rank7", rp, h5::count{N, N, N, N, N, N, N});
        for (int i = 0; i < 128; ++i)
            CHECK(rp[i] == doctest::Approx(wp[i]));
    }
}
