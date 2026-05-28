// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Blitz++ round-trip with shape + element verification.

#include <iostream>
#include <blitz/array.h>
#include <h5cpp/all>

template <class T> using Vec1D = blitz::Array<T, 1>;
template <class T> using Mat2D = blitz::Array<T, 2>;
template <class T> using Cube3D = blitz::Array<T, 3>;

int main() {
    auto fd = h5::create("blitz.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // 1D vector ─────────────────────────────────────────────────────────────
    {
        Vec1D<double> v(8);
        for (int i = 0; i < v.size(); ++i) v(i) = i + 1.0;
        h5::write(fd, "/blitz/vec", v);
        auto back = h5::read<Vec1D<double>>(fd, "/blitz/vec");

        bool shape = (back.extent(0) == v.extent(0));
        bool values = shape;
        for (int i = 0; values && i < v.size(); ++i) values = values && (back(i) == v(i));
        check("blitz::Array<double,1>(8)    shape + values", shape && values);
    }

    // 2D matrix (chunked + gzip) ────────────────────────────────────────────
    {
        Mat2D<short> M(3, 4);
        for (int r = 0; r < M.rows(); ++r)
            for (int c = 0; c < M.cols(); ++c)
                M(r, c) = static_cast<short>(r * M.cols() + c);
        h5::write(fd, "/blitz/mat", M, h5::chunk{3, 4} | h5::gzip{6});
        auto back = h5::read<Mat2D<short>>(fd, "/blitz/mat");

        bool shape  = (back.rows() == M.rows()) && (back.cols() == M.cols());
        bool values = shape;
        for (int r = 0; values && r < M.rows(); ++r)
            for (int c = 0; values && c < M.cols(); ++c)
                values = values && (back(r, c) == M(r, c));
        check("blitz::Array<short,2>(3x4)   shape + values", shape && values);
    }

    // 3D cube ───────────────────────────────────────────────────────────────
    {
        Cube3D<float> C(2, 3, 4);
        for (int i = 0; i < C.extent(0); ++i)
            for (int j = 0; j < C.extent(1); ++j)
                for (int k = 0; k < C.extent(2); ++k)
                    C(i, j, k) = float(i * 100 + j * 10 + k);
        h5::write(fd, "/blitz/cube", C);
        auto back = h5::read<Cube3D<float>>(fd, "/blitz/cube");

        bool shape  = (back.extent(0) == C.extent(0))
                   && (back.extent(1) == C.extent(1))
                   && (back.extent(2) == C.extent(2));
        bool values = shape;
        for (int i = 0; values && i < C.extent(0); ++i)
            for (int j = 0; values && j < C.extent(1); ++j)
                for (int k = 0; values && k < C.extent(2); ++k)
                    values = values && (back(i, j, k) == C(i, j, k));
        check("blitz::Array<float,3>(2x3x4) shape + values", shape && values);
    }
    return 0;
}
