// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// xtensor-blas round-trip: store the result of a BLAS-backed linalg primitive
// (xt::linalg::dot here) just like any other xtensor value. h5cpp doesn't know
// or care that the values came from a BLAS computation.

#include <xtensor/xarray.hpp>
#include <xtensor/xtensor.hpp>
#include <xtensor-blas/xlinalg.hpp>
#include <h5cpp/all>
#include <iostream>

int main() {
    auto fd = h5::create("xtensor-blas.h5", H5F_ACC_TRUNC);

    xt::xtensor<double, 2> A = {{1, 2}, {3, 4}};
    xt::xtensor<double, 2> B = {{5, 6}, {7, 8}};

    auto C = xt::linalg::dot(A, B);          // 2x2 result via BLAS
    xt::xtensor<double, 2> Cmat = C;         // materialise the lazy expression

    h5::write(fd, "/xblas/dot", Cmat);
    auto back = h5::read<xt::xtensor<double, 2>>(fd, "/xblas/dot");
    std::cout << "xt::linalg::dot(A, B) -> 2x2 read shape = "
              << back.shape()[0] << "x" << back.shape()[1] << "\n";
    std::cout << "  values: [[" << back(0,0) << "," << back(0,1) << "],"
              <<        " ["    << back(1,0) << "," << back(1,1) << "]]\n";
    return 0;
}
