// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// STL nested + numeric specialisations:
//   - std::vector<std::vector<T>>      pointers,  ragged_vlen_dataset (hvl_t relay)
//   - std::vector<std::array<T, N>>    contiguous, fixed_inner_extent_dataset
//   - std::complex<T>                  object,    scalar (compound or H5T_COMPLEX)
//   - std::vector<std::complex<T>>    contiguous, linear_value_dataset
//
// Each block is wrapped in try/catch so partial paths reporting failure
// don't abort the run.

#include <h5cpp/all>

#include <array>
#include <complex>
#include <deque>
#include <forward_list>
#include <iostream>
#include <list>
#include <set>
#include <unordered_set>
#include <vector>

template <class Fn>
static void try_block(const char* label, Fn&& fn) {
    try { fn(); }
    catch (const std::exception& e) {
        std::cout << "✘ failed  " << label << "  (" << e.what() << ")\n";
    } catch (...) {
        std::cout << "✘ failed  " << label << "  (unknown exception)\n";
    }
}

int main() {
    h5::fd_t fd = h5::create("stl_nested.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    try_block("std::vector<std::vector<int>>  ragged_vlen round-trip", [&]() {
        std::vector<std::vector<int>> vv = {{1, 2, 3}, {4, 5, 6, 7}, {8, 9}, {10}};
        h5::write(fd, "/nested/vec_of_vec", vv);
        auto back = h5::read<std::vector<std::vector<int>>>(fd, "/nested/vec_of_vec");
        std::cout << "      vector<vector<int>>(4)    = " << back << "\n";
        check("std::vector<std::vector<int>>  ragged_vlen round-trip", back == vv);
    });

    try_block("std::vector<std::array<double,3>>(3) round-trip", [&]() {
        std::vector<std::array<double, 3>> va = {{0.0, 1.0, 2.0}, {3.0, 4.0, 5.0}, {6.0, 7.0, 8.0}};
        h5::write(fd, "/nested/vec_of_array", va);
        auto back = h5::read<std::vector<std::array<double, 3>>>(fd, "/nested/vec_of_array");
        std::cout << "      vector<array<double,3>>(3)= " << back << "\n";
        check("std::vector<std::array<double,3>>(3) round-trip", back == va);
    });

    try_block("std::complex<double>           scalar round-trip", [&]() {
        std::complex<double> c{1.5, -2.25};
        h5::write(fd, "/nested/complex_scalar", c);
        std::complex<double> back;
        h5::read(fd, "/nested/complex_scalar", back);
        std::cout << "      complex<double>           = " << back << "\n";
        check("std::complex<double>           scalar round-trip", back == c);
    });

    // ── vector<list<T>> / vector<deque<T>> / vector<set<T>> / vector<forward_list<T>>
    try_block("std::vector<std::list<int>>(3)         ragged_vlen", [&]() {
        std::vector<std::list<int>> v = {{1, 2, 3}, {4, 5}, {6, 7, 8, 9}};
        h5::write(fd, "/nested/vec_of_list", v);
        auto back = h5::read<std::vector<std::list<int>>>(fd, "/nested/vec_of_list");
        std::cout << "      vector<list<int>>(3)      = " << back << "\n";
        check("std::vector<std::list<int>>(3)        ragged_vlen round-trip", back == v);
    });

    try_block("std::vector<std::deque<int>>(2)        ragged_vlen", [&]() {
        std::vector<std::deque<int>> v = {{10, 20, 30}, {40, 50, 60, 70}};
        h5::write(fd, "/nested/vec_of_deque", v);
        auto back = h5::read<std::vector<std::deque<int>>>(fd, "/nested/vec_of_deque");
        std::cout << "      vector<deque<int>>(2)     = " << back << "\n";
        check("std::vector<std::deque<int>>(2)       ragged_vlen round-trip", back == v);
    });

    try_block("std::vector<std::set<int>>(2)          ragged_vlen", [&]() {
        std::vector<std::set<int>> v = {{1, 3, 5, 7}, {2, 4, 6}};
        h5::write(fd, "/nested/vec_of_set", v);
        auto back = h5::read<std::vector<std::set<int>>>(fd, "/nested/vec_of_set");
        std::cout << "      vector<set<int>>(2)       = " << back << "\n";
        check("std::vector<std::set<int>>(2)         ragged_vlen round-trip", back == v);
    });

    try_block("std::vector<std::forward_list<int>>(2) ragged_vlen", [&]() {
        std::vector<std::forward_list<int>> v = {{100, 200}, {300, 400, 500}};
        h5::write(fd, "/nested/vec_of_flist", v);
        auto back = h5::read<std::vector<std::forward_list<int>>>(fd, "/nested/vec_of_flist");
        std::cout << "      vector<forward_list>(2)   = " << back << "\n";
        check("std::vector<std::forward_list<int>>(2) ragged_vlen round-trip", back == v);
    });

    // ── std::array<std::array<T,N>, M> (nested H5T_ARRAY scalar) ───────────
    try_block("std::list<std::array<int,3>>(2)   list-of-fixed-array", [&]() {
        std::list<std::array<int, 3>> src = {{{1, 2, 3}}, {{4, 5, 6}}};
        h5::write(fd, "/nested/list_arr", src);
        auto back = h5::read<std::list<std::array<int, 3>>>(fd, "/nested/list_arr");
        std::cout << "      list<array<int,3>>(2)    = [...]\n";
        check("std::list<std::array<int,3>>(2)   list-of-fixed-array", back == src);
    });

    try_block("std::array<std::array<int,3>, 2>  nested array_element", [&]() {
        std::array<std::array<int, 3>, 2> src{{ {{1,2,3}}, {{4,5,6}} }};
        h5::write(fd, "/nested/arr_arr_int", src);
        std::array<std::array<int, 3>, 2> back{};
        h5::read(fd, "/nested/arr_arr_int", back);
        std::cout << "      array<array<int,3>,2>    = [[" << back[0][0] << "," << back[0][1] << "," << back[0][2]
                  << "],[" << back[1][0] << "," << back[1][1] << "," << back[1][2] << "]]\n";
        check("std::array<std::array<int,3>, 2>  nested array_element", back == src);
    });

    try_block("std::vector<std::complex<float>>(4) round-trip", [&]() {
        std::vector<std::complex<float>> vc = {{1.0f, 0.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f}, {0.0f, -1.0f}};
        h5::write(fd, "/nested/vector_complex", vc);
        auto back = h5::read<std::vector<std::complex<float>>>(fd, "/nested/vector_complex");
        std::cout << "      vector<complex<float>>(4) = " << back << "\n";
        check("std::vector<std::complex<float>>(4) round-trip", back == vc);
    });

    return 0;
}
