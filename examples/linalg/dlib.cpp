// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Dlib round-trip with shape + element verification. Dlib uses {nr, nc}
// matrix shapes; column vectors are written as nr×1 matrices.

#include <iostream>
#include <dlib/matrix.h>
#include <h5cpp/all>

template <class T> using Matrix = dlib::matrix<T>;

int main() {
    auto fd = h5::create("dlib.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // matrix ────────────────────────────────────────────────────────────────
    {
        Matrix<double> M(3, 4);
        for (long r = 0; r < M.nr(); ++r)
            for (long c = 0; c < M.nc(); ++c)
                M(r, c) = r * M.nc() + c;

        h5::write(fd, "/dlib/mat", M);
        auto back = h5::read<Matrix<double>>(fd, "/dlib/mat");

        bool shape  = (back.nr() == M.nr()) && (back.nc() == M.nc());
        bool values = shape;
        for (long r = 0; values && r < M.nr(); ++r)
            for (long c = 0; values && c < M.nc(); ++c)
                values = values && (back(r, c) == M(r, c));
        check("dlib::matrix<double>(3x4)    shape + values", shape && values);
    }

    // column vector (n x 1 matrix) ──────────────────────────────────────────
    {
        Matrix<short> v(5, 1);
        v = 0, 1, 2, 3, 4;
        h5::write(fd, "/dlib/colvec", v);
        auto back = h5::read<Matrix<short>>(fd, "/dlib/colvec");

        bool shape  = (back.nr() == v.nr()) && (back.nc() == v.nc());
        bool values = shape;
        for (long r = 0; values && r < v.nr(); ++r)
            values = values && (back(r, 0) == v(r, 0));
        check("dlib::matrix<short>(5x1)     shape + values", shape && values);
    }
    return 0;
}
