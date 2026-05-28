// SPDX-License-Identifier: MIT
// This file is part of H5CPP.
// Converted from examples/compound/struct.cpp into an I/O round-trip test.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/all>
#include <filesystem>
#include <vector>

// Header path updated to pod.h — struct.h was the legacy filename, replaced
// when the compound example split POD vs non-POD into separate files.  The
// type was also renamed from `Record` to `record_t` in that refactor.  No
// utils.hpp shim is needed: the test fabricates its own data inline.
#include "examples/compound/pod.h"
#include "examples/compound/generated.h"

TEST_CASE("[example] compound struct round-trip") {
    const char* filename = "test_compound_struct_io.h5";
    std::filesystem::remove(filename);

    // BUILD test data inline — record_t fields beyond idx are left
    // default-constructed; the round-trip check only inspects idx.
    std::vector<sn::example::record_t> original(20);
    for (std::size_t i = 0; i < original.size(); ++i)
        original[i].idx = static_cast<decltype(original[i].idx)>(i);

    // WRITE
    {
        h5::fd_t fd = h5::create(filename, H5F_ACC_TRUNC);
        h5::write(fd, "orm/partial/vector one_shot", original);

        // Also test dataset creation with compound type and custom properties
        h5::create<sn::example::record_t>(fd, "/orm/chunked_2D",
            h5::current_dims{4, 5}, h5::chunk{1, 5} | h5::gzip{8});
    }

    // READ BACK
    {
        h5::fd_t fd = h5::open(filename, H5F_ACC_RDONLY);
        auto readback = h5::read<std::vector<sn::example::record_t>>(fd, "orm/partial/vector one_shot");

        CHECK(readback.size() == original.size());
        for (size_t i = 0; i < original.size(); ++i) {
            CHECK(readback[i].idx == original[i].idx);
        }
    }

    std::filesystem::remove(filename);
}
