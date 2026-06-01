// SPDX-License-Identifier: MIT
// This file is part of H5CPP.
// Converted from examples/attributes/attributes.cpp into an I/O round-trip test.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/all>
#include <filesystem>
#include <vector>
#include <string>

TEST_CASE("[example] attributes round-trip") {
    const char* filename = "test_attributes_io.h5";
    std::filesystem::remove(filename);

    // Inner scope so all h5cpp RAII handles release the file before the final
    // remove().  On Windows, std::filesystem::remove on a still-open file
    // throws filesystem_error; Linux silently allows it.
    {
    // CREATE dataset
    h5::fd_t fd = h5::create(filename, H5F_ACC_TRUNC);
    h5::ds_t ds = h5::create<double>(fd, "directory/dataset", h5::current_dims{5, 6});

    // WRITE attributes with operator[]
    ds["att_01"] = 42;
    ds["att_02"] = std::vector<double>{1., 2., 3., 4.};
    ds["att_03"] = std::vector<int>{'1', '2', '3', '4'};
    ds["att_05"] = "const char[N]";
    ds["att_07"] = std::string("std::string");

    // WRITE attributes with h5::awrite
    h5::awrite(ds, "att_21", 42);
    h5::awrite(ds, "att_22", std::vector<double>{1., 3., 4., 5.});
    h5::awrite(ds, "att_23", std::vector<int>{'1', '3', '4', '5'});
    h5::awrite(ds, "att_25", "const char[N]");
    h5::awrite(ds, "att_27", std::string("std::string"));

    // WRITE attributes to group
    h5::gr_t gr{H5Gopen(static_cast<hid_t>(fd), "/directory", H5P_DEFAULT)};
    h5::awrite(gr, "att_21", 42);
    h5::awrite(gr, "att_22", std::vector<double>{1., 3., 4., 5.});
    h5::awrite(gr, "att_25", "const char[N]");
    h5::awrite(gr, "att_27", std::string("std::string"));

    // READ BACK and verify operator[] attributes
    {
        int att_01 = h5::aread<int>(ds, "att_01");
        CHECK(att_01 == 42);

        auto att_02 = h5::aread<std::vector<double>>(ds, "att_02");
        CHECK(att_02.size() == 4);
        CHECK(att_02[0] == doctest::Approx(1.));
        CHECK(att_02[1] == doctest::Approx(2.));
        CHECK(att_02[2] == doctest::Approx(3.));
        CHECK(att_02[3] == doctest::Approx(4.));

        auto att_03 = h5::aread<std::vector<int>>(ds, "att_03");
        CHECK(att_03.size() == 4);
        CHECK(att_03[0] == '1');
        CHECK(att_03[1] == '2');
        CHECK(att_03[2] == '3');
        CHECK(att_03[3] == '4');

        std::string att_05 = h5::aread<std::string>(ds, "att_05");
        CHECK(att_05 == "const char[N]");

        std::string att_07 = h5::aread<std::string>(ds, "att_07");
        CHECK(att_07 == "std::string");
    }

    // READ BACK and verify h5::awrite attributes
    {
        int att_21 = h5::aread<int>(ds, "att_21");
        CHECK(att_21 == 42);

        auto att_22 = h5::aread<std::vector<double>>(ds, "att_22");
        CHECK(att_22.size() == 4);
        CHECK(att_22[0] == doctest::Approx(1.));
        CHECK(att_22[1] == doctest::Approx(3.));
        CHECK(att_22[2] == doctest::Approx(4.));
        CHECK(att_22[3] == doctest::Approx(5.));

        auto att_23 = h5::aread<std::vector<int>>(ds, "att_23");
        CHECK(att_23.size() == 4);
        CHECK(att_23[0] == '1');
        CHECK(att_23[1] == '3');
        CHECK(att_23[2] == '4');
        CHECK(att_23[3] == '5');

        std::string att_25 = h5::aread<std::string>(ds, "att_25");
        CHECK(att_25 == "const char[N]");

        std::string att_27 = h5::aread<std::string>(ds, "att_27");
        CHECK(att_27 == "std::string");
    }

    // Verify group attributes exist (h5::aread is only available for ds_t)
    CHECK(H5Aexists(static_cast<hid_t>(gr), "att_21") > 0);
    CHECK(H5Aexists(static_cast<hid_t>(gr), "att_22") > 0);
    CHECK(H5Aexists(static_cast<hid_t>(gr), "att_25") > 0);
    CHECK(H5Aexists(static_cast<hid_t>(gr), "att_27") > 0);
    }

    std::filesystem::remove(filename);
}
