// SPDX-License-Identifier: MIT
// This file is part of H5CPP.
// Compile-time verification of compiler-generated multi-TU headers.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/all>
#include <filesystem>

// The multi-tu example splits compound registration across translation units
// (tu-01.cpp, tu-02.cpp, main.cpp). This test consolidates the registration
// path into a single TU by including generated.h directly — it's a
// type-system smoke test, not a cross-TU linkage test. The type name was
// also renamed from Record to record_t when the example was modernised.
#include "examples/multi-tu/struct.h"
#include "examples/multi-tu/generated.h"

TEST_CASE("[example] multi-TU compiler-generated header compiles and runs") {
    const char* filename = "test_multi_tu.h5";
    std::filesystem::remove(filename);

    {
        h5::fd_t fd = h5::create(filename, H5F_ACC_TRUNC);

        // sn::example::record_t — registered via generated.h
        auto ds = h5::create<sn::example::record_t>(fd, "/orm/record",
            h5::current_dims{5, 3}, h5::chunk{1, 3} | h5::gzip{8});
        CHECK(static_cast<hid_t>(ds) > 0);

        // sn::typecheck::record_t — same path, different shape
        auto ds_tc = h5::create<sn::typecheck::record_t>(fd, "/orm/typecheck",
            h5::current_dims{1}, h5::max_dims{H5S_UNLIMITED}, h5::chunk{1});
        CHECK(static_cast<hid_t>(ds_tc) > 0);

        // Write and round-trip sn::example::record_t (inline test data).
        std::vector<sn::example::record_t> original(10);
        for (std::size_t i = 0; i < original.size(); ++i)
            original[i].idx = static_cast<decltype(original[i].idx)>(i);
        h5::write(fd, "/data/vector", original);

        auto readback = h5::read<std::vector<sn::example::record_t>>(fd, "/data/vector");
        CHECK(readback.size() == original.size());
        for (size_t i = 0; i < original.size(); ++i) {
            CHECK(readback[i].idx == original[i].idx);
        }
    }

    std::filesystem::remove(filename);
}
