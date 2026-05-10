/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 */
#include <gmtl/Matrix.h>
#include <gmtl/Vec.h>
#include <h5cpp/all>

int main() {
    auto fd = h5::create("example_gmtl.h5", H5F_ACC_TRUNC);

    gmtl::Matrix<double, 3, 3> M;
    M.set(1, 2, 3, 4, 5, 6, 7, 8, 9);
    h5::write(fd, "gmtl/matrix_3x3", M);

    auto M_read = h5::read<gmtl::Matrix<double, 3, 3>>(fd, "gmtl/matrix_3x3");

    gmtl::Vec<double, 3> V(1.0, 2.0, 3.0);
    h5::write(fd, "gmtl/vec_3", V);

    auto V_read = h5::read<gmtl::Vec<double, 3>>(fd, "gmtl/vec_3");

    return 0;
}
