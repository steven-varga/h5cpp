// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// xtensor round-trip with h5cpp. Two shapes supported:
//
//   xt::xtensor<T, N>  — static rank, fastest dispatch, preferred default
//   xt::xarray<T>      — dynamic rank, runtime extents

#include <xtensor.hpp>
#include <h5cpp/all>
#include <iostream>

int main() {
    auto fd = h5::create("xtensor.h5", H5F_ACC_TRUNC);

    // xt::xtensor (static rank) ─────────────────────────────────────────────
    {
        xt::xtensor<double, 2> M = {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
        h5::write(fd, "/xtensor/static", M);
        auto back = h5::read<xt::xtensor<double, 2>>(fd, "/xtensor/static");
        std::cout << "xt::xtensor<double,2>(2x3) read shape = "
                  << back.shape()[0] << "x" << back.shape()[1] << "\n";
    }

    // xt::xarray (dynamic rank): not supported, get in touch if you need it ─
    /*{
        xt::xarray<double> V = xt::xarray<double>::from_shape({8});
        for (std::size_t i = 0; i < 8; ++i) V(i) = i + 1.0;
        h5::write(fd, "/xtensor/dynamic", V);
        auto back = h5::read<xt::xarray<double>>(fd, "/xtensor/dynamic");
        std::cout << "xt::xarray<double>(8) read = ";
        for (std::size_t i = 0; i < back.size(); ++i) std::cout << back(i) << " ";
        std::cout << "\n";
    }*/

    // 3-tensor ──────────────────────────────────────────────────────────────
    {
        xt::xtensor<float, 3> T = xt::xtensor<float, 3>::from_shape({2, 3, 4});
        std::fill(T.begin(), T.end(), 1.0f);
        h5::write(fd, "/xtensor/cube", T);
        auto back = h5::read<xt::xtensor<float, 3>>(fd, "/xtensor/cube");
        std::cout << "xt::xtensor<float,3>(2x3x4) read shape = "
                  << back.shape()[0] << "x" << back.shape()[1] << "x" << back.shape()[2] << "\n";
    }
    return 0;
}
