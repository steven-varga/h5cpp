// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// STL sequences round-trip showdown:
//   - std::vector<T>            (contiguous, linear_value_dataset)
//   - std::array<T, N>          (contiguous, linear_value_dataset)
//   - std::list<T>              (iterators,  linear_value_dataset; staging vector)
//   - std::deque<T>             (iterators,  linear_value_dataset; staging vector)
//   - std::forward_list<T>      (iterators,  linear_value_dataset; staging vector)
//   - T[N]                      (contiguous, c_array)
//
// Every round-trip prints the read-back via h5cpp's STL inserter (operator<<
// from H5Uall.hpp) and verifies element equality with the ✔/✘ convention.

#include <h5cpp/all>

#include <array>
#include <deque>
#include <forward_list>
#include <iostream>
#include <list>
#include <vector>

int main() {
    static_assert(h5::meta::storage_representation_v<std::vector<int>> ==
                  h5::meta::storage_representation_t::linear_value_dataset,
                  "vector<int> must be linear_value_dataset");
    static_assert(h5::meta::storage_representation_v<std::array<double, 5>> ==
                  h5::meta::storage_representation_t::array_element,
                  "std::array<double, 5> must be array_element");
    h5::fd_t fd = h5::create("stl_sequences.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // ── std::vector<T> ──────────────────────────────────────────────────────
    {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};
        h5::write(fd, "/seq/vector_int", v);
        auto back = h5::read<std::vector<int>>(fd, "/seq/vector_int");
        std::cout << "      vector<int>(8)            = " << back << "\n";
        check("std::vector<int>(8)            round-trip", back == v);
    }

    // ── std::array<T, N> ────────────────────────────────────────────────────
    {
        std::array<double, 5> a = {1.5, 2.5, 3.5, 4.5, 5.5};
        h5::write(fd, "/seq/array_double", a);
        auto back = h5::read<std::array<double, 5>>(fd, "/seq/array_double");
        std::cout << "      array<double,5>           = " << back << "\n";
        check("std::array<double, 5>          round-trip", back == a);
    }

    // ── std::list<T> ────────────────────────────────────────────────────────
    {
        std::list<int> l = {10, 20, 30, 40, 50};
        h5::write(fd, "/seq/list_int", l);
        auto back = h5::read<std::list<int>>(fd, "/seq/list_int");
        std::cout << "      list<int>(5)              = " << back << "\n";
        check("std::list<int>(5)              round-trip", back == l);
    }

    // ── std::deque<T> ───────────────────────────────────────────────────────
    {
        std::deque<short> d = {-3, -2, -1, 0, 1, 2, 3};
        h5::write(fd, "/seq/deque_short", d);
        auto back = h5::read<std::deque<short>>(fd, "/seq/deque_short");
        std::cout << "      deque<short>(7)           = " << back << "\n";
        check("std::deque<short>(7)           round-trip", back == d);
    }

    // ── std::forward_list<T> ────────────────────────────────────────────────
    {
        std::forward_list<float> fl = {0.1f, 0.2f, 0.3f, 0.4f};
        h5::write(fd, "/seq/forward_list_float", fl);
        auto back = h5::read<std::forward_list<float>>(fd, "/seq/forward_list_float");
        std::cout << "      forward_list<float>(4)    = " << back << "\n";
        check("std::forward_list<float>(4)    round-trip", back == fl);
    }

    // ── C-array T[N] ────────────────────────────────────────────────────────
    //   Writes a plain C-array; reads back into a std::array<T, N> for the
    //   verify comparison since C-arrays aren't directly returnable.
    {
        std::int64_t arr[6] = {100, 200, 300, 400, 500, 600};
        h5::write<std::int64_t>(fd, "/seq/c_array", arr, h5::count{6});
        auto back = h5::read<std::array<std::int64_t, 6>>(fd, "/seq/c_array");
        std::cout << "      int64_t[6]                = " << back << "\n";
        bool ok = true;
        for (std::size_t i = 0; ok && i < 6; ++i) ok = (back[i] == arr[i]);
        check("int64_t[6]                     round-trip (read into array)", ok);
    }

    return 0;
}
