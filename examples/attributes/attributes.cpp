// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// HDF5 attribute showcase — one TU exercises every kind of attribute the
// dispatch supports.  Each value is written through both surfaces in turn:
//
//   * operator[]  sugar:    ds["name"] = value;
//   * explicit API:         h5::awrite(ds, "name", value);
//
// Read-backs use h5::aread<T>(parent, name) and are compared element-by-
// element with the project's ✔ / ✘ symbol convention.  Container readbacks
// are printed via h5cpp's STL pretty-printer (operator<< from H5Uall.hpp).
//
// Coverage groups:
//
//   1. Linear-algebra backends (7): Armadillo, Eigen3, Blitz++, dlib, uBLAS,
//      std::valarray, xtensor.  Blaze is intentionally excluded — its LAPACK
//      Fortran prototypes clash with Armadillo's in the same TU; the Blaze
//      surface is exercised in `examples/linalg/blaze.cpp` instead.
//   2. Canonical fixed-extent (Winston model): char[N], std::array<char,N>,
//      std::array<T,N>, std::vector<std::array<T,N>>, std::vector<std::array<char,N>>.
//   3. Scalar object kinds: std::complex<T>, std::pair<K,V>, std::tuple<...>.
//   4. Compound POD: sn::example::record_t — single + vector.
//   5. STL strings / containers: std::string, vector<string>, ragged
//      vector<vector>, list, set, map, vector<tuple>.
//   6. Rewrite (same type/shape OK) + deliberate type-mismatch (must throw).

#include <armadillo>
#include <Eigen/Dense>
#include <blitz/array.h>
#include <dlib/matrix.h>
#include <boost/numeric/ublas/matrix.hpp>
#include <xtensor/xarray.hpp>

#include <array>
#include <complex>
#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <valarray>
#include <vector>

#include "struct.h"
#include <h5cpp/all>
#include "generated.h" 
#include "utils.hpp"


int main() {
    // H5F_LIBVER_V18 enables the v2 object header, lifting the default 64 KB
    // attribute ceiling — required because this demo packs ~25 attributes
    // (matrices, compounds, containers) onto a single dataset.  Without it,
    // the cumulative attribute size overflows the v1 header.
    h5::fd_t fd = h5::create("attributes.h5", H5F_ACC_TRUNC, h5::default_fcpl,
        h5::libver_bounds({H5F_LIBVER_V18, H5F_LIBVER_V18}));

    // Host dataset for ds["..."] sugar; group for the explicit h5::awrite path.
    constexpr std::size_t rows = 3, cols = 4;
    arma::mat host(rows, cols);
    for (std::size_t r = 0; r < rows; ++r)
        for (std::size_t c = 0; c < cols; ++c) host(r, c) = double(r * cols + c);
    h5::ds_t ds = h5::write(fd, "/dataset", host);
    h5::gr_t gr = h5::gcreate(fd, "/group");

    // ── 1. Linear-algebra backends ─────────────────────────────────────────
    std::cout << "\n=== linear-algebra attributes ===\n";
    Eigen::MatrixXd eigen_m(rows, cols); eigen_m << 0,1,2,3, 4,5,6,7, 8,9,10,11;
    blitz::Array<double, 2> blitz_m(rows, cols); blitz_m = blitz::tensor::i * static_cast<int>(cols) + blitz::tensor::j;
    dlib::matrix<double> dlib_m(rows, cols);
        for (long r = 0; r < (long)rows; ++r)
            for (long c = 0; c < (long)cols; ++c) dlib_m(r, c) = double(r * cols + c);
    boost::numeric::ublas::matrix<double> ublas_m(rows, cols);
        for (std::size_t r = 0; r < rows; ++r)
            for (std::size_t c = 0; c < cols; ++c) ublas_m(r, c) = double(r * cols + c);
    xt::xarray<double> xt_m = {{0., 1., 2., 3.}, {4., 5., 6., 7.}, {8., 9., 10., 11.}};
    std::valarray<double> va = {0.5, 1.5, 2.5, 3.5, 4.5};

    ds["arma"] = host;
    ds["eigen"] = eigen_m;
    ds["blitz"] = blitz_m;
    ds["dlib"] = dlib_m;
    ds["ublas"] = ublas_m;
    ds["xtensor"] = xt_m;
    ds["valarray"] = va;

    check("arma::mat(3x4)               attribute", same(host, h5::aread<arma::mat>(ds, "arma")));
    check("Eigen::MatrixXd(3x4)         attribute", same(eigen_m, h5::aread<Eigen::MatrixXd>(ds, "eigen")));
    check("blitz::Array<double,2>(3x4)  attribute", same(blitz_m, h5::aread<blitz::Array<double, 2>>(ds, "blitz")));
    check("dlib::matrix<double>(3x4)    attribute", same(dlib_m, h5::aread<dlib::matrix<double>>(ds, "dlib")));
    check("ublas::matrix<double>(3x4)   attribute", same(ublas_m, h5::aread<boost::numeric::ublas::matrix<double>>(ds, "ublas")));
    check("xt::xarray<double>(3x4)      attribute", same(xt_m, h5::aread<xt::xarray<double>>(ds, "xtensor")));
    check("std::valarray<double>(5)     attribute", same(va, h5::aread<std::valarray<double>>(ds, "valarray")));

    // char[N] / std::array<char,N>  → H5T_C_S1 + H5Tset_size(N), scalar
    // std::array<T,N> (non-char)    → H5T_ARRAY[N] dt_t<T>,     scalar
    // vector<std::array<T,N>>       → H5T_ARRAY[N] dt_t<T>,     rank-1
    // vector<std::array<char,N>>    → H5T_C_S1 + size(N),       rank-1
    std::cout << "\n=== canonical fixed-extent attributes ===\n";

    char c_str[16] = "char[N]";
    std::array<char, 16> ac{"array<char,N>"};
    std::array<int, 5> ai = {10, 20, 30, 40, 50};
    std::vector<std::array<int, 3>> v_ai = {{{1, 2, 3}}, {{4, 5, 6}}, {{7, 8, 9}}};
    std::vector<std::array<char, 8>> v_ac{{"alpha"},{"beta"}};

    h5::awrite(ds, "char_arr", c_str);
    h5::awrite(ds, "array_char", ac);
    ds["array_int"] = ai;
    ds["vec_arr_int"] = v_ai;
    ds["vec_arr_char"] = v_ac;

    {
        auto back = h5::aread<std::array<char, 16>>(ds, "char_arr");
        check("char[16]                     attribute (FLS)",
              std::strncmp(back.data(), c_str, 16) == 0);
    }
    check("std::array<char,16>          attribute (FLS)",        h5::aread<std::array<char, 16>>(ds, "array_char") == ac);
    check("std::array<int,5>            attribute (H5T_ARRAY)",  h5::aread<std::array<int, 5>>(ds, "array_int") == ai);

    auto v_ai_back = h5::aread<std::vector<std::array<int, 3>>>(ds, "vec_arr_int");
    check("std::vector<std::array<int,3>>(3) attribute (rank-1 of H5T_ARRAY)",  v_ai_back == v_ai);
    auto v_ac_back = h5::aread<std::vector<std::array<char, 8>>>(ds, "vec_arr_char");
    check("std::vector<std::array<char,8>>(2) attribute (rank-1 of FLS)", v_ac_back == v_ac);

    // ── 3. Scalar object kinds ─────────────────────────────────────────────
    std::cout << "\n=== scalar object kinds ===\n";

    std::complex<double> cx{1.5, -2.25};
    std::pair<int, double> pr{42, 3.14};
    std::tuple<int, double, char> tup{42, 3.14, 'x'};

    ds["complex"] = cx;
    ds["pair"] = pr;
    ds["tuple"] = tup;

    check("std::complex<double>         scalar attribute",             h5::aread<std::complex<double>>(ds, "complex") == cx);
    check("std::pair<int,double>        scalar attribute",             h5::aread<std::pair<int, double>>(ds, "pair") == pr);
    check("std::tuple<int,double,char>  scalar attribute (composite)", h5::aread<std::tuple<int, double, char>>(ds, "tuple") == tup);

    // ── 4. Compound POD (compiler-assisted reflection) ─────────────────────
    std::cout << "\n=== compound POD ===\n";

    sn::example::record_t rec = *h5::pod<sn::example::record_t>{};
    rec.idx = 7;
    std::vector<sn::example::record_t> records = h5::pod<sn::example::record_t>{} | h5::take(8);
    for (std::size_t i = 0; i < records.size(); ++i) records[i].idx = i + 100;

    ds["pod"]  = rec;
    ds["pods"] = records;
    check("sn::example::record_t        scalar attribute (idx field)", h5::aread<sn::example::record_t>(ds, "pod").idx == rec.idx);

    auto pods_back = h5::aread<std::vector<sn::example::record_t>>(ds, "pods");
    bool pods_ok = pods_back.size() == records.size();
    for (std::size_t i = 0; pods_ok && i < records.size(); ++i)
        pods_ok = (pods_back[i].idx == records[i].idx);
    check("std::vector<record_t>(8)     attribute (idx field)", pods_ok);

    // ── 5. STL strings / containers ────────────────────────────────────────
    std::cout << "\n=== STL containers ===\n";

    std::string str = "std::string attribute";
    std::vector<std::string> vec_s = {"alpha", "beta", "gamma", "delta"};
    std::vector<std::vector<double>> ragged = {{1.0}, {2.0, 3.0}, {4.0, 5.0, 6.0}};
    std::list<int> list_i = {10, 20, 30, 40};
    std::set<int> set_i = {5, 1, 4, 2, 3};
    std::map<int, double> map_id = {{1, 1.1}, {2, 2.2}, {3, 3.3}};
    std::vector<std::tuple<int, float>> v_tup = {{1, 1.1f}, {2, 2.2f}, {3, 3.3f}};

    ds["str"]    = str;
    ds["vec_s"]  = vec_s;
    ds["ragged"] = ragged;
    ds["list"]   = list_i;
    ds["set"]    = set_i;
    ds["map"]    = map_id;
    ds["vtups"]  = v_tup;

    check("std::string                  scalar attribute (vlen)",          h5::aread<std::string>(ds, "str") == str);
    check("std::vector<std::string>(4)  attribute (vlen-text dataset)",    h5::aread<std::vector<std::string>>(ds, "vec_s") == vec_s);
    check("std::vector<std::vector<d>>  attribute (ragged_vlen)",          h5::aread<std::vector<std::vector<double>>>(ds, "ragged") == ragged);
    check("std::list<int>(4)            attribute (iterators)",            h5::aread<std::list<int>>(ds, "list") == list_i);
    check("std::set<int>(5)             attribute (iterators)",            h5::aread<std::set<int>>(ds, "set") == set_i);
    check("std::map<int,double>(3)      attribute (key_value)",            h5::aread<std::map<int, double>>(ds, "map") == map_id);
    check("std::vector<std::tuple<i,f>> attribute (composite element)",    h5::aread<std::vector<std::tuple<int, float>>>(ds, "vtups") == v_tup);

    // ── 6. Explicit h5::awrite surface (group as target) ───────────────────
    std::cout << "\n=== explicit h5::awrite (target: /group) ===\n";

    h5::awrite(gr, "int",    42);
    h5::awrite(gr, "double", 3.14);
    h5::awrite(gr, "str",    std::string("via h5::awrite"));
    h5::awrite(gr, "arma",   host);

    check("int / double / std::string / arma::mat via h5::awrite on group",
          h5::aread<int>(gr, "int") == 42
       && h5::aread<double>(gr, "double") == 3.14
       && h5::aread<std::string>(gr, "str") == "via h5::awrite"
       && same(host, h5::aread<arma::mat>(gr, "arma")));

    // ── 7. Pretty-print readback (h5cpp STL operator<<) ────────────────────
    std::cout << "\n=== pretty-print readback ===\n";
    std::cout << "  vec_s    = " << h5::aread<std::vector<std::string>>(ds, "vec_s") << "\n";
    std::cout << "  ragged   = " << h5::aread<std::vector<std::vector<double>>>(ds, "ragged") << "\n";
    std::cout << "  list     = " << h5::aread<std::list<int>>(ds, "list") << "\n";
    std::cout << "  set      = " << h5::aread<std::set<int>>(ds, "set") << "\n";
    std::cout << "  map      = " << h5::aread<std::map<int, double>>(ds, "map") << "\n";
    std::cout << "  vtups    = " << h5::aread<std::vector<std::tuple<int, float>>>(ds, "vtups") << "\n";
    std::cout << "  tuple    = " << h5::aread<std::tuple<int, double, char>>(ds, "tuple") << "\n";
    std::cout << "  pair     = " << h5::aread<std::pair<int, double>>(ds, "pair") << "\n";
    std::cout << "  ai       = " << h5::aread<std::array<int, 5>>(ds, "array_int") << "\n";
    std::cout << "  v_ai     = " << v_ai_back << "\n";

    // ── 8. Rewrite + deliberate type-mismatch ──────────────────────────────
    std::cout << "\n=== rewrite + type-mismatch error ===\n";

    ds["scalar_int"] = 42;       // create
    ds["scalar_int"] = 99;       // rewrite, same type — OK
    check("rewrite ds[\"scalar_int\"] same type", h5::aread<int>(ds, "scalar_int") == 99);

    bool threw = false;
    h5::mute();
    try {
        h5::awrite(ds, "scalar_int", std::string("not an int"));
    } catch (const h5::error::io::attribute::any&) {
        threw = true;
    }
    h5::unmute();
    check("rewrite type mismatch → throws h5::error::io::attribute::any", threw);

    return 0;
}
