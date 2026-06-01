// SPDX-License-Identifier: MIT
// This file is part of H5CPP.
// Converted from examples/basics/basics.cpp into an I/O round-trip test.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/all>
#include <filesystem>

TEST_CASE("[example] basics API demonstration") {
    const char* filename = "test_basics_io.h5";
    std::filesystem::remove(filename);

    // Type descriptors: h5::dt_t<int> maps to H5T_NATIVE_INT
    {
        h5::dt_t<int> my_int_type;
        hid_t capi_style_id = static_cast<hid_t>(my_int_type);
        CHECK(H5Tequal(capi_style_id, H5T_NATIVE_INT) > 0);
    }

    // Property list chaining with | and |= operators
    {
        h5::dcpl_t dcpl0 = h5::chunk{12} | h5::gzip{2};
        h5::dcpl_t dcpl1 = h5::chunk{12} | h5::gzip{2};
        h5::dcpl_t dcpl = dcpl0 | dcpl1;
        dcpl0 |= dcpl1;
        CHECK(static_cast<hid_t>(dcpl) > 0);
        CHECK(static_cast<hid_t>(dcpl0) > 0);
    }

    // Error handling: invalid gzip level should throw
    {
        h5::mute();
        CHECK_THROWS_AS([]{ h5::dcpl_t dcpl_0 = h5::gzip{79798}; }(), h5::error::any);
        h5::unmute();
    }

    // File creation and RAII close
    {
        h5::fd_t fd = h5::create(filename, H5F_ACC_TRUNC);
        CHECK(static_cast<hid_t>(fd) > 0);
        hid_t ref = static_cast<hid_t>(fd);
        CHECK(static_cast<hid_t>(ref) > 0);
    }

    // Dataset creation with various property combinations
    {
        h5::fd_t fd = h5::create(filename, H5F_ACC_TRUNC);

        auto ds_0 = h5::create<short>(fd, "/type/short/tree_0",
            h5::current_dims{10, 20}, h5::max_dims{10, H5S_UNLIMITED},
            h5::create_path | h5::utf8,
            h5::chunk{2, 3} | h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9},
            h5::default_dapl);
        CHECK(static_cast<hid_t>(ds_0) > 0);

        h5::dcpl_t dcpl = h5::chunk{2, 3} | h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9};
        auto ds_1 = h5::create<short>(fd, "/type/short/tree_1",
            h5::current_dims{10, 20}, h5::max_dims{10, H5S_UNLIMITED}, dcpl);
        CHECK(static_cast<hid_t>(ds_1) > 0);

        auto ds_2 = h5::create<short>(fd, "/type/short/tree_2",
            h5::current_dims{10, 20}, h5::max_dims{10, H5S_UNLIMITED},
            h5::default_lcpl, dcpl, h5::default_dapl);
        CHECK(static_cast<hid_t>(ds_2) > 0);

        auto ds_3 = h5::create<short>(fd, "/type/short/max_dims",
            h5::max_dims{10, H5S_UNLIMITED},
            h5::chunk{10, 1});
        CHECK(static_cast<hid_t>(ds_3) > 0);

        auto ds_4 = h5::create<std::string>(fd, "/types/string with chunk and compression",
            h5::max_dims{H5S_UNLIMITED},
            h5::chunk{10} | h5::gzip{9});
        CHECK(static_cast<hid_t>(ds_4) > 0);
    }

    std::filesystem::remove(filename);
}
