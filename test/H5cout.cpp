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
