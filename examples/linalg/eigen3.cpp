// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Eigen3 round-trip with shape + element verification.
//
// Only dynamic Eigen types round-trip; fixed-size matrices live on the stack
// and bypass the data() access path. Column vectors are written as Matrix<T>{n,1}.

#include <iostream>
#include <Eigen/Dense>
#include <h5cpp/all>

template <class T> using Matrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>;
template <class T> using Array  = Eigen::Array <T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>;

int main() {
    auto fd = h5::create("eigen3.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // matrix ────────────────────────────────────────────────────────────────
    {
        Matrix<double> M = Matrix<double>::Random(3, 4);
        h5::write(fd, "/eigen/mat", M);
        auto back = h5::read<Matrix<double>>(fd, "/eigen/mat");
        bool shape  = (back.rows() == M.rows()) && (back.cols() == M.cols());
        bool values = shape && M.isApprox(back, 0.0);
        check("Eigen::Matrix<double>(3x4)   shape + values", shape && values);
    }

    // array ─────────────────────────────────────────────────────────────────
    {
        Array<float> A(2, 5);
        for (Eigen::Index r = 0; r < A.rows(); ++r)
            for (Eigen::Index c = 0; c < A.cols(); ++c)
                A(r, c) = float(r * A.cols() + c);
        h5::write(fd, "/eigen/array", A);
        auto back = h5::read<Array<float>>(fd, "/eigen/array");
        bool shape  = (back.rows() == A.rows()) && (back.cols() == A.cols());
        bool values = shape && ((A == back).all());
        check("Eigen::Array<float>(2x5)     shape + values", shape && values);
    }

    // column-vector via Matrix(n, 1) ─────────────────────────────────────────
    {
        Matrix<double> v(8, 1);
        for (Eigen::Index i = 0; i < v.rows(); ++i) v(i, 0) = i + 1.0;
        h5::write(fd, "/eigen/colvec", v);
        auto back = h5::read<Matrix<double>>(fd, "/eigen/colvec");
        bool shape  = (back.rows() == v.rows()) && (back.cols() == v.cols());
        bool values = shape && v.isApprox(back, 0.0);
        check("Eigen column-vec(8x1)        shape + values", shape && values);
    }
    return 0;
}
