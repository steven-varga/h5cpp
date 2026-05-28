/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <armadillo>
#include <h5cpp/all>

#include <array>
#include <iostream>
#include <string>
#include <vector>

// =========================================================================
// h5cpp UTF-8 identifier demo.
//
// HDF5 1.8+ supports UTF-8 throughout the object-naming surface — file
// names, group names, dataset names, attribute names, and string-typed
// data. h5cpp opts into UTF-8 by default for every link / attribute it
// creates (see h5::default_lcpl, h5::default_acpl in H5Pall.hpp).
//
// This example verifies the full UTF-8 round-trip across:
//   1. File name (the .h5 path itself)
//   2. Dataset names (top-level, 14 scripts)
//   3. Nested group paths (mixed-script segments under "/")
//   4. Attribute names AND attribute string content
//   5. Re-open by UTF-8 path with HDF5's own libver_bounds(V18, V18)
//      so the resulting file is readable by HDF5 1.8-era tools.
//
// String content round-trip is exercised more thoroughly in examples/string;
// this example focuses on UTF-8 in the *naming* surface.
// =========================================================================

namespace {
    int errors = 0;

    template <class A, class B>
    void check(const char* tag, const A& expected, const B& got) {
        const bool ok = (expected == got);
        if (!ok) ++errors;
        std::cout << (ok ? "✔ ok      " : "✘ failed  ") << tag << "\n";
    }

    // Fourteen scripts, one short phrase each — every byte in the dataset
    // paths and attribute names below is UTF-8 multi-byte (except the
    // first, ASCII baseline).
    const std::array<const char*, 14> phrases = {
        "hello world",            // ASCII baseline
        "مرحبا بالعالم",        // Arabic
        "Բարեւ աշխարհ",         // Armenian
        "Здравей свят",         // Bulgarian (Cyrillic)
        "Прывітанне Сусвет",    // Belarusian (Cyrillic)
        "မင်္ဂလာပါကမ္ဘာလောက",   // Burmese
        "你好，世界",            // Chinese
        "Γειά σου Κόσμε",       // Greek
        "હેલ્લો વિશ્વ",          // Gujarati
        "Helló Világ",          // Hungarian
        "こんにちは世界",         // Japanese
        "안녕 세상",             // Korean
        "سلام دنیا",            // Persian
        "העלא וועלט",           // Yiddish
    };
}

int main() {
    const std::string filename = "こんにちは世界.h5";   // UTF-8 filename

    // ---------------------------------------------------------------------
    // 1. Create file with a UTF-8 name; pin file format to HDF5 1.8 so the
    //    resulting file is portable to 1.8-era readers (h5py, h5dump 1.8+,
    //    Julia HDF5.jl). Newer HDF5 features (H5T_STD_REF, etc.) are
    //    rejected at write-time in that mode; for plain UTF-8 strings the
    //    constraint is inert.
    // ---------------------------------------------------------------------
    {
        h5::fd_t fd = h5::create(filename, H5F_ACC_TRUNC, h5::default_fcpl,
            h5::libver_bounds({H5F_LIBVER_V18, H5F_LIBVER_V18}));

        // 2. Each script gets a top-level dataset and a UTF-8 attribute
        //    holding the same phrase as content. Dataset name = attribute
        //    name = attribute value — three places the same UTF-8 bytes
        //    need to survive without truncation or re-encoding.
        for (const auto& phrase : phrases) {
            arma::mat M = arma::ones(3, 4);
            h5::ds_t ds = h5::write(fd, phrase, M);
            ds[phrase] = std::string(phrase);   // attribute: name + value both UTF-8
        }

        // 3. Nested groups with mixed-script segments. h5::write creates
        //    intermediate groups via the default LCPL (UTF-8 cset, create
        //    intermediates ON).
        arma::Col<int> v = {1, 2, 3, 4, 5};
        h5::write(fd, "温度/مجموعة/données", v);
    }

    // ---------------------------------------------------------------------
    // 4. Reopen the file by UTF-8 filename; round-trip every UTF-8
    //    identifier through h5::open / h5::aread / h5::read.
    // ---------------------------------------------------------------------
    {
        h5::fd_t fd = h5::open(filename, H5F_ACC_RDONLY);

        check("file opens by UTF-8 filename", true, H5Iis_valid(static_cast<hid_t>(fd)) > 0);

        for (const auto& phrase : phrases) {
            h5::ds_t ds = h5::open(fd, phrase);
            auto attr_value = h5::aread<std::string>(ds, phrase);
            check(phrase, std::string(phrase), attr_value);
        }

        // Verify the nested mixed-script group path round-trips its content.
        auto v_back = h5::read<arma::Col<int>>(fd, "温度/مجموعة/données");
        check("UTF-8 nested group path: 温度/مجموعة/données",
              std::size_t(5), v_back.n_elem);
    }

    std::cout << "\n"
              << (errors == 0 ? "✔ all checks passed"
                              : "✘ some checks failed")
              << ", errors=" << errors << "\n";
    return errors;
}
