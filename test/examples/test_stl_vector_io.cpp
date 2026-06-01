// SPDX-License-Identifier: MIT
// This file is part of H5CPP.
// Converted from examples/stl/vector.cpp into an I/O round-trip test.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/all>
#include <vector>
#include <filesystem>

TEST_CASE("[example] stl vector<double> round-trip") {
    const char* filename = "test_stl_vector_io.h5";
    std::filesystem::remove(filename);

    std::vector<double> original(10, 1.0);
    for (size_t i = 0; i < original.size(); ++i)
        original[i] = static_cast<double>(i);

    // WRITE
    {
        h5::fd_t fd = h5::create(filename, H5F_ACC_TRUNC);
        h5::write(fd, "stl/vector/full.dat", original);

        // Create a chunked/gzipped dataset (no write, just verify creation)
        h5::ds_t ds = h5::create<double>(fd, "stl/vector/chunked.dat",
            h5::current_dims{21, 10},
            h5::max_dims{H5S_UNLIMITED, 10},
            h5::chunk{1, 10} | h5::gzip{9}
        );
        CHECK(static_cast<hid_t>(ds) > 0);
    }

    // READ BACK
    {
        h5::fd_t fd = h5::open(filename, H5F_ACC_RDONLY);
        auto readback = h5::read<std::vector<double>>(fd, "stl/vector/full.dat");

        CHECK(readback.size() == original.size());
        for (size_t i = 0; i < original.size(); ++i) {
            CHECK(readback[i] == doctest::Approx(original[i]));
        }
    }

    std::filesystem::remove(filename);
}
