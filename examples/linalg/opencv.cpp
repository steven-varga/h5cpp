/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 */
#include <opencv2/core.hpp>
#include <h5cpp/all>

int main() {
    auto fd = h5::create("example_opencv.h5", H5F_ACC_TRUNC);

    cv::Mat_<double> M(3, 4);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            M(i, j) = i * 4 + j;
    h5::write(fd, "opencv/matrix_3x4", M);

    auto M_read = h5::read<cv::Mat_<double>>(fd, "opencv/matrix_3x4");

    return 0;
}
