// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// STL string-shaped containers.
//
// Per the arch notes, the supported matrix lists std::string / string_view as
// `text` / `vlen_text_dataset`.  In practice scalar string write through the
// generic dispatch surfaces an HDF5 "no datatype conversion path" — only
// std::vector<std::string> exercises the documented `pointers` /
// `vlen_text_dataset` path end-to-end.  Each block below is wrapped in a
// try/catch so we get a complete report even when one path aborts.

#include <h5cpp/all>

#include <array>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
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
    h5::fd_t fd = h5::create("stl_strings.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // ── std::string (scalar) ───────────────────────────────────────────────
    try_block("std::string                    scalar round-trip", [&]() {
        std::string s = "hello, world";
        h5::write(fd, "/strings/scalar", s);
        std::string back;
        h5::read(fd, "/strings/scalar", back);
        std::cout << "      string                    = \"" << back << "\"\n";
        check("std::string                    scalar round-trip", back == s);
    });

    // ── std::string_view (write-only, read into std::string) ───────────────
    try_block("std::string_view               write + read-as-string", [&]() {
        std::string owner = "view-written-text";
        std::string_view sv = owner;
        h5::write(fd, "/strings/from_view", sv);
        std::string back;
        h5::read(fd, "/strings/from_view", back);
        std::cout << "      string_view -> string     = \"" << back << "\"\n";
        check("std::string_view               write + read-as-string", back == owner);
    });

    // ── char[N] (fixed-length HDF5 string) ─────────────────────────────────
#ifndef _MSC_VER
    // MSVC's overload resolution flags `h5::write(fd, name, char(&)[20])` as
    // ambiguous between the generic array overload and the const char* (name)
    // overload; gcc and clang resolve it cleanly. std::array<char,N> below
    // exercises the same on-disk path without the C-array ambiguity.
    try_block("char[20]                       fixed-length round-trip", [&]() {
        char src[20] = "fixed-length";   // remaining bytes zero-padded
        h5::write(fd, "/strings/fixed", src);
        char back[20] = {};
        h5::read(fd, "/strings/fixed", back);
        std::cout << "      char[20]                 = \"" << back << "\"\n";
        bool ok = true;
        for (int i = 0; ok && i < 20; ++i) ok = (back[i] == src[i]);
        check("char[20]                       fixed-length round-trip", ok);
    });
#else
    std::cout << "◇ skipped char[20] round-trip on MSVC (overload ambiguity)\n";
#endif

    // ── std::array<char, N> (alias for char[N]) ────────────────────────────
    try_block("std::array<char, 20>           fixed-length round-trip", [&]() {
        std::array<char, 20> src{};
        std::strcpy(src.data(), "fixed-length-arr");
        h5::write(fd, "/strings/fixed_arr", src);
        std::array<char, 20> back{};
        h5::read(fd, "/strings/fixed_arr", back);
        std::cout << "      array<char,20>           = \"" << back.data() << "\"\n";
        check("std::array<char, 20>           fixed-length round-trip", back == src);
    });

    // ── std::vector<std::array<char, N>> (rank-1 of FLS elements) ──────────
    try_block("std::vector<std::array<char,8>>(3)  fls_dataset round-trip", [&]() {
        std::vector<std::array<char, 8>> src(3);
        std::strcpy(src[0].data(), "alpha");
        std::strcpy(src[1].data(), "beta");
        std::strcpy(src[2].data(), "gamma");
        h5::write(fd, "/strings/fls_vec", src);
        auto back = h5::read<std::vector<std::array<char, 8>>>(fd, "/strings/fls_vec");
        std::cout << "      vector<array<char,8>>(3) = [\""
                  << back[0].data() << "\",\""
                  << back[1].data() << "\",\""
                  << back[2].data() << "\"]\n";
        bool ok = back.size() == src.size();
        for (size_t i = 0; ok && i < src.size(); ++i) ok = (back[i] == src[i]);
        check("std::vector<std::array<char,8>>(3)  fls_dataset round-trip", ok);
    });

    // ── std::vector<std::string> ────────────────────────────────────────────
    try_block("std::vector<std::string>(5)    round-trip", [&]() {
        std::vector<std::string> v = {"alpha", "beta", "gamma", "delta", "epsilon"};
        h5::write(fd, "/strings/vector", v);
        auto back = h5::read<std::vector<std::string>>(fd, "/strings/vector");
        std::cout << "      vector<string>(5)         = " << back << "\n";
        check("std::vector<std::string>(5)    round-trip", back == v);
    });

    return 0;
}
