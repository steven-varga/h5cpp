// SPDX-License-Identifier: MIT
// This file is part of H5CPP.
// Converted from examples/packet-table/packettable.cpp into an I/O round-trip test.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <armadillo>
#include <h5cpp/all>
#include <filesystem>
#include <vector>
#include <numeric>

// No utils.hpp shim — test data is fabricated inline below.
#include "examples/packet-table/struct.h"
#include "examples/packet-table/generated.h"

TEST_CASE("[example] packet table int stream round-trip") {
    const char* filename = "test_packet_table_io.h5";
    std::filesystem::remove(filename);

    std::vector<int> original(10);
    std::iota(original.begin(), original.end(), 1);

    // WRITE via packet table append
    {
        h5::fd_t fd = h5::create(filename, H5F_ACC_TRUNC);
        h5::pt_t pt = h5::create<int>(fd, "stream of integral",
            h5::current_dims{0}, h5::max_dims{H5S_UNLIMITED}, h5::chunk{5} | h5::fill_value<int>(3));
        for (auto record : original)
            h5::append(pt, record);
    }

    // READ BACK and verify
    {
        h5::fd_t fd = h5::open(filename, H5F_ACC_RDONLY);
        auto readback = h5::read<std::vector<int>>(fd, "stream of integral");

        CHECK(readback.size() == original.size());
        for (size_t i = 0; i < original.size(); ++i) {
            CHECK(readback[i] == original[i]);
        }
    }

    std::filesystem::remove(filename);
}

TEST_CASE("[example] packet table compound struct stream round-trip") {
    const char* filename = "test_packet_table_struct_io.h5";
    std::filesystem::remove(filename);

    std::vector<sn::example::Record> original(10);
    for (std::size_t i = 0; i < original.size(); ++i)
        original[i].idx = static_cast<decltype(original[i].idx)>(i);

    // WRITE via packet table append
    {
        h5::fd_t fd = h5::create(filename, H5F_ACC_TRUNC);
        h5::pt_t pt = h5::create<sn::example::Record>(fd, "stream of struct",
            h5::max_dims{H5S_UNLIMITED}, h5::chunk{5} | h5::gzip{9});
        for (const auto& record : original)
            h5::append(pt, record);
    }

    // READ BACK and verify
    {
        h5::fd_t fd = h5::open(filename, H5F_ACC_RDONLY);
        auto readback = h5::read<std::vector<sn::example::Record>>(fd, "stream of struct");

        CHECK(readback.size() == original.size());
        for (size_t i = 0; i < original.size(); ++i) {
            CHECK(readback[i].idx == original[i].idx);
        }
    }

    std::filesystem::remove(filename);
}
