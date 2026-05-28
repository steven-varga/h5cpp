// SPDX-License-Identifier: MIT
// This file is part of H5CPP.
// Converted from examples/datatypes/two-bit.cpp into an I/O round-trip test.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>
#include <filesystem>
#include <vector>

// custom_types.hpp is the current path — the legacy two-bit.hpp filename was
// folded into the broader custom_types.hpp header when the datatypes example
// was consolidated.
#include "examples/datatypes/custom_types.hpp"

TEST_CASE("[example] custom two-bit type round-trip") {
    const char* filename = "test_two_bit_io.h5";
    std::filesystem::remove(filename);

    namespace nm = bitstring;

    std::vector<nm::two_bit> original = {0xff, 0x0f, 0xf0, 0x00, 0b0001'1011};

    // WRITE
    {
        h5::fd_t fd = h5::create(filename, H5F_ACC_TRUNC);
        h5::write(fd, "data", original);
    }

    // READ BACK
    {
        h5::fd_t fd = h5::open(filename, H5F_ACC_RDONLY);
        auto readback = h5::read<std::vector<nm::two_bit>>(fd, "data");

        CHECK(readback.size() == original.size());
        for (size_t i = 0; i < original.size(); ++i) {
            CHECK(readback[i].value == original[i].value);
        }
    }

    std::filesystem::remove(filename);
}
