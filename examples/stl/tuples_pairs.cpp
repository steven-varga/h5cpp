// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// STL tuples and pairs (composite + object kinds, scalar + container forms).
//
// Per the arch notes, the supported matrix lists:
//   - std::tuple<Ts...>          scalar:   composite kind / scalar storage
//   - std::vector<std::tuple>    rank-1:   pointers kind / linear_value
//   - std::list<std::tuple>      rank-1:   iterators kind / linear_value
//   - std::pair<K,V>             scalar:   object kind / scalar storage
//   - std::vector<std::pair>     rank-1:   contiguous kind / linear_value
//
// The scalar tuple/pair write paths currently surface HDF5 type-creation
// failures; each block is wrapped in try/catch so the run reports every
// path's status without aborting.

#include <h5cpp/all>

#include <iostream>
#include <list>
#include <string>
#include <tuple>
#include <utility>
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
    h5::fd_t fd = h5::create("stl_tuples_pairs.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    try_block("std::tuple<int,double>         scalar round-trip", [&]() {
        std::tuple<int, double> t = {42, 3.14};
        h5::write(fd, "/tup/scalar", t);
        std::tuple<int, double> back;
        h5::read(fd, "/tup/scalar", back);
        std::cout << "      tuple<int,double>         = " << back << "\n";
        check("std::tuple<int,double>         scalar round-trip", back == t);
    });

    // NOTE: std::tuple with std::string fields requires custom pack/unpack to
    // bridge the 24-byte std::string in-memory representation to HDF5's 8-byte
    // char* vlen string layout. Tuple_layout currently memcpys the std::string
    // object as-is, which yields garbage on disk for the string field. Not
    // attempted in this pass; document and move on.
    std::cout << "◇ na     std::tuple<..., std::string>   scalar (layout mismatch: std::string in compound)\n";

    try_block("std::vector<std::tuple<int,double>>(4) round-trip", [&]() {
        std::vector<std::tuple<int, double>> v = {{1, 1.1}, {2, 2.2}, {3, 3.3}, {4, 4.4}};
        h5::write(fd, "/tup/vector", v);
        auto back = h5::read<std::vector<std::tuple<int, double>>>(fd, "/tup/vector");
        std::cout << "      vector<tuple<int,double>> = " << back << "\n";
        check("std::vector<std::tuple<int,double>>(4) round-trip", back == v);
    });

    // NOTE: vector<tuple<..., std::string, ...>> hits the same layout-mismatch
    // problem as the scalar form — the in-memory 24-byte std::string doesn't
    // match the 8-byte vlen char* the HDF5 compound type expects. Excluded
    // here; same future-work item as the scalar string-in-compound case.

    try_block("std::list<std::tuple<int,double>>(3) round-trip", [&]() {
        std::list<std::tuple<int, double>> l = {{10, 0.1}, {20, 0.2}, {30, 0.3}};
        h5::write(fd, "/tup/list", l);
        auto back = h5::read<std::list<std::tuple<int, double>>>(fd, "/tup/list");
        std::cout << "      list<tuple<int,double>>   = " << back << "\n";
        check("std::list<std::tuple<int,double>>(3) round-trip", back == l);
    });

    try_block("std::pair<int, double>         scalar round-trip", [&]() {
        std::pair<int, double> p = {7, 2.718};
        h5::write(fd, "/pair/scalar", p);
        std::pair<int, double> back;
        h5::read(fd, "/pair/scalar", back);
        std::cout << "      pair<int,double>          = " << back << "\n";
        check("std::pair<int, double>         scalar round-trip", back == p);
    });

    try_block("std::vector<std::pair<int,double>>(5) round-trip", [&]() {
        std::vector<std::pair<int, double>> v = {{1, 0.5}, {2, 1.5}, {3, 2.5}, {4, 3.5}, {5, 4.5}};
        h5::write(fd, "/pair/vector", v);
        auto back = h5::read<std::vector<std::pair<int, double>>>(fd, "/pair/vector");
        std::cout << "      vector<pair<int,double>>  = " << back << "\n";
        check("std::vector<std::pair<int,double>>(5) round-trip", back == v);
    });

    return 0;
}
