#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>
#include <h5cpp/H5cout.hpp>
#include <sstream>
#include "support/fixture.hpp"

TEST_CASE("operator<< for dxpl_t prints handle") {
    std::ostringstream oss;
    oss << h5::default_dxpl;
    CHECK(oss.str().find("handle") != std::string::npos);
}

TEST_CASE("operator<< for sp_t prints rank and dims") {
    h5::test::file_fixture_t f("test-cout-sp.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{3, 4});
    h5::sp_t sp = h5::get_space(ds);
    std::ostringstream oss;
    oss << sp;
    CHECK(oss.str().find("rank") != std::string::npos);
}

TEST_CASE("operator<< for impl::array prints dims") {
    h5::current_dims_t dims{3, 4, 5};
    std::ostringstream oss;
    oss << dims;
    CHECK(oss.str().find("3") != std::string::npos);
    CHECK(oss.str().find("4") != std::string::npos);
    CHECK(oss.str().find("5") != std::string::npos);
}

TEST_CASE("operator<< for impl::array with inf max_dims") {
    h5::max_dims_t dims{10, H5S_UNLIMITED};
    std::ostringstream oss;
    oss << dims;
    CHECK(oss.str().find("inf") != std::string::npos);
}

TEST_CASE("operator<< for std::vector prints elements") {
    std::vector<int> vec = {1, 2, 3};
    std::ostringstream oss;
    oss << vec;
    CHECK(oss.str().find("1") != std::string::npos);
}

TEST_CASE("operator<< for std::vector with many elements prints ellipsis") {
    std::vector<int> vec(200, 42);
    std::ostringstream oss;
    oss << vec;
    // H5Uall.hpp recursive STL printer truncates at H5CPP_CONSOLE_WIDTH with
    // a ", ..." marker (replaced the legacy ".. fix me .." printer that was
    // removed alongside the operator<<(std::vector<T>) overload in H5cout.hpp).
    CHECK(oss.str().find(", ...") != std::string::npos);
}

TEST_CASE("operator<< for sp_t with hyperslab selection prints blocks") {
    h5::test::file_fixture_t f("test-cout-sp-blocks.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{10});
    h5::sp_t sp = h5::get_space(ds);
    h5::offset_t offset{0};
    h5::stride_t stride{2};
    h5::count_t count{5};
    h5::block_t block{1};
    h5::select_hyperslab(sp, offset, stride, count, block);
    std::ostringstream oss;
    oss << sp;
    CHECK(oss.str().find("rank") != std::string::npos);
}

TEST_CASE("operator<< for rank-0 current_dims_t prints n/a") {
    h5::current_dims_t dims;
    std::ostringstream oss;
    oss << dims;
    CHECK(oss.str().find("n/a") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Handle pretty-printers — fd_t / ds_t / gr_t / at_t / dcpl_t / fapl_t plus
// the generic hid_t<...> printer and the invalid-handle (valid=no) branches.
// These exercise H5cout.hpp:138-444, previously uncovered.
// ---------------------------------------------------------------------------

TEST_CASE("operator<< for fd_t prints path and mode") {
    h5::test::file_fixture_t f("test-cout-fd.h5");
    std::ostringstream oss;
    oss << f.fd;
    const std::string s = oss.str();
    CHECK(s.find("fd_t") != std::string::npos);
    CHECK(s.find("path='") != std::string::npos);
    CHECK(s.find("mode=") != std::string::npos);
    CHECK(s.find("size=") != std::string::npos);
}

TEST_CASE("operator<< for ds_t prints chunk dims and filter count") {
    h5::test::file_fixture_t f("test-cout-ds.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "chunked",
        h5::current_dims_t{100}, h5::chunk{10} | h5::gzip{6});
    std::ostringstream oss;
    oss << ds;
    const std::string s = oss.str();
    CHECK(s.find("ds_t") != std::string::npos);
    CHECK(s.find("dtype=INTEGER") != std::string::npos);
    CHECK(s.find("layout=CHUNKED") != std::string::npos);
    CHECK(s.find("chunk={10}") != std::string::npos);
    CHECK(s.find("filters=") != std::string::npos);
}

TEST_CASE("operator<< for gr_t prints path and child count") {
    h5::test::file_fixture_t f("test-cout-gr.h5");
    h5::gr_t gr = h5::gcreate(f.fd, "sensors");
    h5::gr_t child = h5::gcreate(gr, "imu");
    (void) child;
    std::ostringstream oss;
    oss << gr;
    const std::string s = oss.str();
    CHECK(s.find("gr_t") != std::string::npos);
    CHECK(s.find("path='/sensors'") != std::string::npos);
    CHECK(s.find("children=1") != std::string::npos);
}

TEST_CASE("operator<< for at_t prints attribute name") {
    h5::test::file_fixture_t f("test-cout-at.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{1});
    h5::awrite(ds, "units", 42);
    h5::at_t at{H5Aopen(static_cast<hid_t>(ds), "units", H5P_DEFAULT)};
    std::ostringstream oss;
    oss << at;
    const std::string s = oss.str();
    CHECK(s.find("at_t") != std::string::npos);
    CHECK(s.find("name='units'") != std::string::npos);
    CHECK(s.find("dtype=INTEGER") != std::string::npos);
}

TEST_CASE("operator<< for dcpl_t prints layout, alloc, fill and filter pipeline") {
    h5::test::file_fixture_t f("test-cout-dcpl.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "chunked",
        h5::current_dims_t{100}, h5::chunk{10} | h5::gzip{6});
    h5::dcpl_t dcpl{H5Dget_create_plist(static_cast<hid_t>(ds))};
    std::ostringstream oss;
    oss << dcpl;
    const std::string s = oss.str();
    CHECK(s.find("dcpl_t") != std::string::npos);
    CHECK(s.find("layout=CHUNKED") != std::string::npos);
    CHECK(s.find("chunk={10}") != std::string::npos);
    CHECK(s.find("alloc=") != std::string::npos);
    CHECK(s.find("fill=") != std::string::npos);
    CHECK(s.find("filters=[") != std::string::npos);
}

TEST_CASE("operator<< for fapl_t prints libver and cache config") {
    h5::fapl_t fapl{H5Pcreate(H5P_FILE_ACCESS)};
    std::ostringstream oss;
    oss << fapl;
    const std::string s = oss.str();
    CHECK(s.find("fapl_t") != std::string::npos);
    CHECK(s.find("libver=[") != std::string::npos);
    CHECK(s.find("cache={") != std::string::npos);
}

TEST_CASE("operator<< generic handle printer covers an unspecialized handle") {
    // lcpl_t has no dedicated specialization, so it routes through the
    // generic hid_t<...> printer and the class_tag<lcpl_t> specialization.
    h5::lcpl_t lcpl{H5Pcreate(H5P_LINK_CREATE)};
    std::ostringstream oss;
    oss << lcpl;
    const std::string s = oss.str();
    CHECK(s.find("lcpl_t") != std::string::npos);
    CHECK(s.find("valid=yes") != std::string::npos);
    CHECK(s.find("refs=") != std::string::npos);
}

TEST_CASE("operator<< prints valid=no for default-constructed (uninitialized) handles") {
    h5::fd_t   fd;
    h5::ds_t   ds;
    h5::gr_t   gr;
    h5::at_t   at;
    h5::dcpl_t dcpl;
    h5::fapl_t fapl;
    h5::lcpl_t lcpl;   // generic printer, invalid branch
    for (const std::string& s : {
            [&]{ std::ostringstream o; o << fd;   return o.str(); }(),
            [&]{ std::ostringstream o; o << ds;   return o.str(); }(),
            [&]{ std::ostringstream o; o << gr;   return o.str(); }(),
            [&]{ std::ostringstream o; o << at;   return o.str(); }(),
            [&]{ std::ostringstream o; o << dcpl; return o.str(); }(),
            [&]{ std::ostringstream o; o << fapl; return o.str(); }(),
            [&]{ std::ostringstream o; o << lcpl; return o.str(); }() }) {
        CHECK(s.find("valid=no") != std::string::npos);
    }
}
