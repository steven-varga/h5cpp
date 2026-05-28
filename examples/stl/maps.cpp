// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// STL associative-map containers (iterators kind, key_value_dataset):
//   - std::map<K, V>                 ordered by key, unique keys
//   - std::multimap<K, V>            ordered by key, duplicate keys allowed
//   - std::unordered_map<K, V>       hashed, unique keys
//   - std::unordered_multimap<K, V>  hashed, duplicate keys allowed
//
// On disk these become a rank-1 dataset of a 2-field {key, value} compound;
// hash-bucket ordering for unordered_* is not preserved across the round-trip.

#include <h5cpp/all>

#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

template <class C>
static bool same_map(const C& a, const C& b) {
    if (a.size() != b.size()) return false;
    // C::value_type for the unordered_* variants is pair<const K, V> which
    // isn't sortable; copy into pair<K, V> first.
    using key_t = typename C::key_type;
    using mapped_t = typename C::mapped_type;
    std::vector<std::pair<key_t, mapped_t>> as(a.begin(), a.end());
    std::vector<std::pair<key_t, mapped_t>> bs(b.begin(), b.end());
    auto cmp = [](const auto& x, const auto& y) {
        if (x.first != y.first) return x.first < y.first;
        return x.second < y.second;
    };
    std::sort(as.begin(), as.end(), cmp);
    std::sort(bs.begin(), bs.end(), cmp);
    return as == bs;
}

int main() {
    h5::fd_t fd = h5::create("stl_maps.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // ── std::map<K, V> ──────────────────────────────────────────────────────
    {
        std::map<int, double> m = {{1, 1.1}, {2, 2.2}, {3, 3.3}, {4, 4.4}};
        h5::write(fd, "/maps/map_int_double", m);
        auto back = h5::read<std::map<int, double>>(fd, "/maps/map_int_double");
        std::cout << "      map<int,double>(4)        = " << back << "\n";
        check("std::map<int, double>          round-trip", back == m);
    }

    // ── std::multimap<K, V> ─────────────────────────────────────────────────
    {
        std::multimap<short, int> mm = {{1, 10}, {1, 11}, {2, 20}, {3, 30}, {3, 31}};
        h5::write(fd, "/maps/multimap_short_int", mm);
        auto back = h5::read<std::multimap<short, int>>(fd, "/maps/multimap_short_int");
        std::cout << "      multimap<short,int>(5)    = " << back << "\n";
        check("std::multimap<short, int>      round-trip", back == mm);
    }

    // ── std::unordered_map<K, V> ────────────────────────────────────────────
    {
        std::unordered_map<int, float> um = {{10, 1.0f}, {20, 2.0f}, {30, 3.0f}};
        h5::write(fd, "/maps/unordered_map_int_float", um);
        auto back = h5::read<std::unordered_map<int, float>>(fd, "/maps/unordered_map_int_float");
        std::cout << "      u_map<int,float>(3)       = " << back << "\n";
        check("std::unordered_map<int, float> round-trip (key-cmp)", same_map(um, back));
    }

    // ── std::unordered_multimap<K, V> ───────────────────────────────────────
    {
        std::unordered_multimap<int, int> umm = {{1, 1}, {1, 2}, {2, 3}, {2, 4}, {2, 5}};
        h5::write(fd, "/maps/unordered_multimap", umm);
        auto back = h5::read<std::unordered_multimap<int, int>>(fd, "/maps/unordered_multimap");
        std::cout << "      u_multimap<int,int>(5)    = " << back << "\n";
        check("std::unordered_multimap<int,int> round-trip (key-cmp)", same_map(umm, back));
    }

    return 0;
}
