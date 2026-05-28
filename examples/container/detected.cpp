// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Demonstrates that h5cpp's container dispatch is *structural* — driven by
// Walter Brown's detection idiom (N4502) over the type's surface, not by name.
//
// Four custom containers from tiny_containers.hpp are exercised:
//   tiny::vec<T>      — contiguous sequence (data + size + value_type)
//   tiny::flist<T>    — iterator-only sequence (begin/end + value_type)
//   tiny::set<T>      — set-like (key_type + value_type, no mapped_type)
//   tiny::dict<K,V>   — map-like (key_type + mapped_type + value_type)
//
// For each we:
//   1. print which Layer-1 detection traits fire (the "trait card"),
//   2. print which Layer-2 storage representation h5cpp picks,
//   3. write the container, then read it back as the std:: counterpart to
//      confirm the on-disk layout matches what the dispatcher promised.
//
// Note: the *read* path is not yet generic — see notes at the bottom — so the
// round-trip is asymmetric (custom write → std:: read).

#include "tiny_containers.hpp"

#include <h5cpp/all>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

// ── pretty helpers ───────────────────────────────────────────────────────────
static constexpr const char* glyph(bool b) { return b ? "✔" : "✘"; } // ✔ / ✘

static const char* storage_name(h5::meta::storage_representation_t s) {
    using sr = h5::meta::storage_representation_t;
    switch (s) {
        case sr::unsupported:                return "unsupported";
        case sr::scalar:                     return "scalar";
        case sr::c_array:                    return "c_array";
        case sr::linear_value_dataset:       return "linear_value_dataset";
        case sr::key_value_dataset:          return "key_value_dataset";
        case sr::ragged_vlen_dataset:        return "ragged_vlen_dataset";
        case sr::fixed_inner_extent_dataset: return "fixed_inner_extent_dataset";
        case sr::vlen_text_dataset:          return "vlen_text_dataset";
    }
    return "?";
}

template <class T>
static void trait_card(const char* label) {
    using U = std::remove_cv_t<std::remove_reference_t<T>>;
    namespace m = h5::meta;

    std::cout << "\n  " << label << "\n";
    std::cout << "    " << glyph(m::has_iterator<U>::value)         << " has_iterator\n";
    std::cout << "    " << glyph(m::has_value_type<U>::value)       << " has_value_type\n";
    std::cout << "    " << glyph(m::has_data<U>::value)             << " has_data\n";
    std::cout << "    " << glyph(m::has_size<U>::value)             << " has_size\n";
    std::cout << "    " << glyph(m::is_sequential_like<U>::value)   << " is_sequential_like\n";
    std::cout << "    " << glyph(m::is_set_like<U>::value)          << " is_set_like        (has key_type, no mapped_type)\n";
    std::cout << "    " << glyph(m::is_map_like<U>::value)          << " is_map_like        (has key_type + mapped_type)\n";
    std::cout << "    storage_representation_v = "
              << storage_name(m::storage_representation_v<U>) << "\n";
}

int main() {
    std::cout << "h5cpp detection-idiom dispatch — custom container demo\n";
    std::cout << "======================================================\n";

    // ── Layer-1 trait inspection ────────────────────────────────────────────
    std::cout << "\n[Layer-1: capability detection (Walter Brown N4502)]\n";
    trait_card<tiny::vec<int>>           ("tiny::vec<int>");
    trait_card<tiny::flist<int>>         ("tiny::flist<int>");
    trait_card<tiny::set<int>>           ("tiny::set<int>");
    trait_card<tiny::dict<int, double>>  ("tiny::dict<int,double>");

    // ── Layer-2 round-trip via h5cpp ────────────────────────────────────────
    std::cout << "\n[Layer-2: write via custom container, read as std::]\n";
    auto fd = h5::create("detected.h5", H5F_ACC_TRUNC);

    {
        tiny::vec<int> v{1, 2, 3, 4, 5};
        h5::write(fd, "tiny_vec", v);
        // Read back into tiny::vec<int> itself — the detection-driven ctor
        // path in H5Dread.hpp picks T(size_t) without an impl::get spec.
        auto back = h5::read<tiny::vec<int>>(fd, "tiny_vec");
        std::vector<int> view(back.begin(), back.end());      // adapt for pretty-print
        std::cout << "  tiny::vec<int>     wrote " << v.size()
                  << "  back as tiny::vec:   " << view << "\n";
    }

    {
        tiny::flist<int> l{10, 20, 30, 40};
        h5::write(fd, "tiny_flist", l);
        auto back = h5::read<std::vector<int>>(fd, "tiny_flist");
        std::cout << "  tiny::flist<int>   wrote " << l.size()
                  << "  back as std::vector: " << back << "\n";
    }

    {
        tiny::set<int> s{3, 1, 4, 1, 5, 9, 2, 6};   // duplicates dropped
        h5::write(fd, "tiny_set", s);
        auto back = h5::read<std::set<int>>(fd, "tiny_set");
        std::cout << "  tiny::set<int>     wrote " << s.size()
                  << "  back as std::set:    " << back << "\n";
    }

    {
        tiny::dict<int, double> d{{1, 1.5}, {2, 2.5}, {3, 3.5}};
        h5::write(fd, "tiny_dict", d);
        auto back = h5::read<std::map<int, double>>(fd, "tiny_dict");
        std::cout << "  tiny::dict<int,d>  wrote " << d.size()
                  << "  back as std::map:    " << back << "\n";
    }

    // ── notes ───────────────────────────────────────────────────────────────
    std::cout << "\n[Read side]\n";
    std::cout <<
        "  Read for contiguous vector-shape is now structural: any T with .data() +\n"
        "  .size() + T(size_t) ctor round-trips into T itself (see tiny::vec above).\n"
        "  Set-like / map-like / iterator-only customs still read back via the std::\n"
        "  counterpart — the iterator-staging and kv_t-compound paths construct\n"
        "  through range-ctor / insert, which doesn't have a structural equivalent.\n";

    return 0;
}
