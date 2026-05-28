// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// STL set-shaped containers (iterators kind, linear_value_dataset):
//   - std::set<T>                  ordered, unique
//   - std::multiset<T>             ordered, duplicates allowed
//   - std::unordered_set<T>        hashed, unique
//   - std::unordered_multiset<T>   hashed, duplicates allowed
//
// Ordered sets compare element-by-element; unordered sets compare as
// multisets (any insertion order is valid on read-back).

#include <h5cpp/all>

#include <iostream>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>
#include <algorithm>

template <class C>
static bool same_multiset(const C& a, const C& b) {
    if (a.size() != b.size()) return false;
    std::vector<typename C::value_type> as(a.begin(), a.end());
    std::vector<typename C::value_type> bs(b.begin(), b.end());
    std::sort(as.begin(), as.end());
    std::sort(bs.begin(), bs.end());
    return as == bs;
}

int main() {
    h5::fd_t fd = h5::create("stl_sets.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // ── std::set<T> ─────────────────────────────────────────────────────────
    {
        std::set<int> s = {5, 1, 4, 2, 3};
        h5::write(fd, "/sets/set_int", s);
        auto back = h5::read<std::set<int>>(fd, "/sets/set_int");
        std::cout << "      set<int>(5)               = " << back << "\n";
        check("std::set<int>                  round-trip", back == s);
    }

    // ── std::multiset<T> ────────────────────────────────────────────────────
    {
        std::multiset<int> m = {3, 5, 5, 12, 21, 23, 28, 30, 30};
        h5::write(fd, "/sets/multiset_int", m);
        auto back = h5::read<std::multiset<int>>(fd, "/sets/multiset_int");
        std::cout << "      multiset<int>(9)          = " << back << "\n";
        check("std::multiset<int>             round-trip", back == m);
    }

    // ── std::unordered_set<T> ───────────────────────────────────────────────
    //   Element order on read-back is implementation-defined; compare via
    //   sorted-multiset equality.
    {
        std::unordered_set<int> u = {2, 4, 6, 8, 10, 12};
        h5::write(fd, "/sets/unordered_set_int", u);
        auto back = h5::read<std::unordered_set<int>>(fd, "/sets/unordered_set_int");
        std::cout << "      unordered_set<int>(6)     = " << back << "\n";
        check("std::unordered_set<int>        round-trip (multiset cmp)", same_multiset(u, back));
    }

    // ── std::unordered_multiset<T> ──────────────────────────────────────────
    {
        std::unordered_multiset<int> um = {1, 1, 2, 3, 5, 8, 13, 21};
        h5::write(fd, "/sets/unordered_multiset_int", um);
        auto back = h5::read<std::unordered_multiset<int>>(fd, "/sets/unordered_multiset_int");
        std::cout << "      u_multiset<int>(8)        = " << back << "\n";
        check("std::unordered_multiset<int>   round-trip (multiset cmp)", same_multiset(um, back));
    }

    return 0;
}
