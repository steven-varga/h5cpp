// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Blaze round-trip with shape + element verification.

#include <iostream>
#include <blaze/Math.h>
#include <h5cpp/all>

template <class T> using Matrix = blaze::DynamicMatrix<T, blaze::rowMajor>;
template <class T> using Vector = blaze::DynamicVector<T, blaze::columnVector>;

int main() {
    auto fd = h5::create("blaze.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // vector ─────────────────────────────────────────────────────────────────
    {
        Vector<double> v(8);
        for (std::size_t i = 0; i < v.size(); ++i) v[i] = i + 1.0;
        h5::write(fd, "/blaze/vec", v);
        auto back = h5::read<Vector<double>>(fd, "/blaze/vec");
        bool shape  = (back.size() == v.size());
        bool values = shape && (v == back);
        check("blaze::DynamicVector<double>(8)    shape + values", shape && values);
    }

    // matrix ────────────────────────────────────────────────────────────────
    {
        Matrix<short> M(3, 4);
        for (std::size_t r = 0; r < M.rows(); ++r)
            for (std::size_t c = 0; c < M.columns(); ++c)
                M(r, c) = static_cast<short>(r * M.columns() + c);
        h5::write(fd, "/blaze/mat", M);
        auto back = h5::read<Matrix<short>>(fd, "/blaze/mat");
        bool shape  = (back.rows() == M.rows()) && (back.columns() == M.columns());
        bool values = shape && (M == back);
        check("blaze::DynamicMatrix<short>(3x4)   shape + values", shape && values);
    }
    return 0;
}
