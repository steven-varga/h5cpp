/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 */
#include <newmat/newmat.h>
#include <h5cpp/all>

int main() {
    auto fd = h5::create("example_newmat.h5", H5F_ACC_TRUNC);

    NEWMAT::Matrix M(3, 4);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            M(i, j) = i * 4 + j;
    h5::write(fd, "newmat/matrix", M);

    auto M_read = h5::read<NEWMAT::Matrix>(fd, "newmat/matrix");

    NEWMAT::ColumnVector V(5);
    for (int i = 0; i < 5; ++i) V(i) = i;
    h5::write(fd, "newmat/colvec", V);

    auto V_read = h5::read<NEWMAT::ColumnVector>(fd, "newmat/colvec");

    return 0;
}
