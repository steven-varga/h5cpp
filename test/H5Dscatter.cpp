/*
 * Copyright (c) 2026 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Tests for scatter/gather I/O dispatch infrastructure.
 * Issue #258.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>
#include <h5cpp/H5Dscatter.hpp>
#include "support/fixture.hpp"

/* ------------------------------------------------------------------
 * 1.  Trait tests
 * ------------------------------------------------------------------ */
struct unregistered_t {
    int x;
};

struct scatter_pod_t {
    std::uint64_t timestamp;
    double        value;
};

// Register the type as scatter-eligible. The actual specializations follow below.
H5CPP_REGISTER_SCATTER(scatter_pod_t);

TEST_CASE("[#258] has_scatter defaults to false for unregistered types") {
    static_assert(!h5::has_scatter<unregistered_t>::value,
        "unregistered types must not have scatter");
    CHECK(!h5::has_scatter<unregistered_t>::value);
}

TEST_CASE("[#258] H5CPP_REGISTER_SCATTER sets has_scatter to true") {
    static_assert(h5::has_scatter<scatter_pod_t>::value,
        "registered types must have scatter");
    CHECK(h5::has_scatter<scatter_pod_t>::value);
}

/* ------------------------------------------------------------------
 * 2.  Manual scatter/gather specializations for the test struct
 *
 * In production code these are emitted by h5cpp-compiler into a
 * generated header. Here we hand-write them to test the library path.
 * ------------------------------------------------------------------ */
namespace h5::generated::scatter_pod_t_ {

struct row_t {
    std::uint64_t timestamp;
    double        value;
};

inline hid_t compound_type() {
    static const hid_t ct = [] {
        hid_t t = H5Tcreate(H5T_COMPOUND, sizeof(row_t));
        H5Tinsert(t, "timestamp", HOFFSET(row_t, timestamp), H5T_NATIVE_UINT64);
        H5Tinsert(t, "value",     HOFFSET(row_t, value),     H5T_NATIVE_DOUBLE);
        return t;
    }();
    return ct;
}

} // namespace h5::generated::scatter_pod_t_

template<>
inline h5::ds_t h5::scatter<scatter_pod_t>(hid_t fd, const std::string& path,
                                            const scatter_pod_t& obj) {
    using namespace h5::generated::scatter_pod_t_;
    h5::ds_t ds;

    h5::mute();
    bool exists = H5Lexists(fd, path.c_str(), H5P_DEFAULT) > 0;
    h5::unmute();

    if (exists) {
        ds = h5::open(h5::fd_t(fd), path, h5::default_dapl);
    } else {
        h5::dcpl_t dcpl{H5Pcreate(H5P_DATASET_CREATE)};
        hsize_t chunk = 64;
        H5Pset_chunk(dcpl, 1, &chunk);

        hsize_t cur = 0;
        hsize_t max = H5S_UNLIMITED;
        hid_t space = H5Screate_simple(1, &cur, &max);
        ds = h5::createds(h5::fd_t(fd), path, compound_type(),
                          h5::sp_t{space}, h5::default_lcpl, dcpl, h5::default_dapl);
    }

    hsize_t row = h5::detail::next_row(static_cast<hid_t>(ds));
    row_t r{obj.timestamp, obj.value};
    herr_t err = h5::detail::write_one_row(static_cast<hid_t>(ds), compound_type(), row, &r);
    REQUIRE(err >= 0);
    return ds;
}

template<>
inline void h5::gather<scatter_pod_t>(hid_t fd, const std::string& path,
                                       scatter_pod_t& obj) {
    using namespace h5::generated::scatter_pod_t_;
    h5::ds_t ds = h5::open(h5::fd_t(fd), path, h5::default_dapl);

    hsize_t nrows = h5::detail::next_row(static_cast<hid_t>(ds));
    REQUIRE(nrows > 0);

    row_t r{};
    herr_t err = h5::detail::read_one_row(static_cast<hid_t>(ds), compound_type(), nrows - 1, &r);
    REQUIRE(err >= 0);

    obj.timestamp = r.timestamp;
    obj.value     = r.value;
}

/* ------------------------------------------------------------------
 * 3.  detail helper tests (next_row, write_one_row, read_one_row)
 * ------------------------------------------------------------------ */
TEST_CASE("[#258] detail::next_row on empty 1-D dataset returns 0") {
    h5::test::file_fixture_t f("test-scatter-empty.h5");

    hsize_t chunk = 8;
    h5::dcpl_t dcpl{H5Pcreate(H5P_DATASET_CREATE)};
    H5Pset_chunk(dcpl, 1, &chunk);

    hsize_t cur = 0;
    hsize_t max = H5S_UNLIMITED;
    hid_t space = H5Screate_simple(1, &cur, &max);
    hid_t ctype = H5Tcreate(H5T_COMPOUND, sizeof(double));
    H5Tinsert(ctype, "v", 0, H5T_NATIVE_DOUBLE);

    h5::ds_t ds = h5::createds(f.fd, "ds", ctype, h5::sp_t{space},
                               h5::default_lcpl, dcpl, h5::default_dapl);
    H5Tclose(ctype);

    CHECK(h5::detail::next_row(static_cast<hid_t>(ds)) == 0);
}

TEST_CASE("[#258] detail::write_one_row extends dataset and writes row") {
    h5::test::file_fixture_t f("test-scatter-write-row.h5");

    hsize_t chunk = 8;
    h5::dcpl_t dcpl{H5Pcreate(H5P_DATASET_CREATE)};
    H5Pset_chunk(dcpl, 1, &chunk);

    hsize_t cur = 0;
    hsize_t max = H5S_UNLIMITED;
    hid_t space = H5Screate_simple(1, &cur, &max);
    hid_t ctype = H5Tcreate(H5T_COMPOUND, sizeof(double));
    H5Tinsert(ctype, "v", 0, H5T_NATIVE_DOUBLE);

    h5::ds_t ds = h5::createds(f.fd, "ds", ctype, h5::sp_t{space},
                               h5::default_lcpl, dcpl, h5::default_dapl);

    double buf = 3.14;
    herr_t err = h5::detail::write_one_row(static_cast<hid_t>(ds), ctype, 0, &buf);
    CHECK(err >= 0);
    CHECK(h5::detail::next_row(static_cast<hid_t>(ds)) == 1);

    double buf2 = 2.71;
    err = h5::detail::write_one_row(static_cast<hid_t>(ds), ctype, 1, &buf2);
    CHECK(err >= 0);
    CHECK(h5::detail::next_row(static_cast<hid_t>(ds)) == 2);

    H5Tclose(ctype);
}

TEST_CASE("[#258] detail::read_one_row reads back correct row") {
    h5::test::file_fixture_t f("test-scatter-read-row.h5");

    hsize_t chunk = 8;
    h5::dcpl_t dcpl{H5Pcreate(H5P_DATASET_CREATE)};
    H5Pset_chunk(dcpl, 1, &chunk);

    hsize_t cur = 0;
    hsize_t max = H5S_UNLIMITED;
    hid_t space = H5Screate_simple(1, &cur, &max);
    hid_t ctype = H5Tcreate(H5T_COMPOUND, sizeof(double));
    H5Tinsert(ctype, "v", 0, H5T_NATIVE_DOUBLE);

    h5::ds_t ds = h5::createds(f.fd, "ds", ctype, h5::sp_t{space},
                               h5::default_lcpl, dcpl, h5::default_dapl);

    double v0 = 1.0, v1 = 2.0, v2 = 3.0;
    h5::detail::write_one_row(static_cast<hid_t>(ds), ctype, 0, &v0);
    h5::detail::write_one_row(static_cast<hid_t>(ds), ctype, 1, &v1);
    h5::detail::write_one_row(static_cast<hid_t>(ds), ctype, 2, &v2);

    double out = 0.0;
    herr_t err = h5::detail::read_one_row(static_cast<hid_t>(ds), ctype, 1, &out);
    CHECK(err >= 0);
    CHECK(out == doctest::Approx(2.0));

    H5Tclose(ctype);
}

/* ------------------------------------------------------------------
 * 4.  h5::write / h5::read dispatch tests
 * ------------------------------------------------------------------ */
TEST_CASE("[#258] h5::write dispatches to scatter specialization") {
    h5::test::file_fixture_t f("test-scatter-dispatch-write.h5");

    scatter_pod_t obj{42ULL, 3.14};
    h5::ds_t ds = h5::write(f.fd, "session", obj);
    CHECK(H5Iis_valid(static_cast<hid_t>(ds)) > 0);
    CHECK(h5::detail::next_row(static_cast<hid_t>(ds)) == 1);
}

TEST_CASE("[#258] h5::read dispatches to gather specialization") {
    h5::test::file_fixture_t f("test-scatter-dispatch-read.h5");

    scatter_pod_t original{99ULL, 2.718};
    h5::write(f.fd, "session", original);

    scatter_pod_t retrieved{};
    h5::read(f.fd, "session", retrieved);

    CHECK(retrieved.timestamp == 99ULL);
    CHECK(retrieved.value == doctest::Approx(2.718));
}

TEST_CASE("[#258] multiple scatter writes append rows") {
    h5::test::file_fixture_t f("test-scatter-multi-row.h5");

    h5::write(f.fd, "log", scatter_pod_t{1000ULL, 1.0});
    h5::write(f.fd, "log", scatter_pod_t{2000ULL, 2.0});
    h5::write(f.fd, "log", scatter_pod_t{3000ULL, 3.0});

    h5::ds_t ds = h5::open(f.fd, "log");
    CHECK(h5::detail::next_row(static_cast<hid_t>(ds)) == 3);

    // gather reads the last row by default in our test specialization
    scatter_pod_t last{};
    h5::gather<scatter_pod_t>(static_cast<hid_t>(f.fd), "log", last);
    CHECK(last.timestamp == 3000ULL);
    CHECK(last.value == doctest::Approx(3.0));
}

TEST_CASE("[#258] unregistered type still uses normal write path") {
    h5::test::file_fixture_t f("test-scatter-normal-path.h5");

    std::vector<double> data = {1.0, 2.0, 3.0};
    h5::ds_t ds = h5::write(f.fd, "vec", data);
    CHECK(H5Iis_valid(static_cast<hid_t>(ds)) > 0);

    auto back = h5::read<std::vector<double>>(f.fd, "vec");
    CHECK(back.size() == 3);
    CHECK(back[0] == doctest::Approx(1.0));
}
