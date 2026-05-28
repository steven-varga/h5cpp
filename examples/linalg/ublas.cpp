// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Boost.uBLAS round-trip with shape + element verification.

#include <iostream>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <h5cpp/all>

template <class T> using Matrix = boost::numeric::ublas::matrix<T>;
template <class T> using Vector = boost::numeric::ublas::vector<T>;

int main() {
    auto fd = h5::create("ublas.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // vector ─────────────────────────────────────────────────────────────────
    {
        Vector<double> v(8);
        for (std::size_t i = 0; i < v.size(); ++i) v(i) = i + 1.0;
        h5::write(fd, "/ublas/vec", v);
        auto back = h5::read<Vector<double>>(fd, "/ublas/vec");

        bool shape  = (back.size() == v.size());
        bool values = shape;
        for (std::size_t i = 0; values && i < v.size(); ++i)
            values = values && (back(i) == v(i));
        check("ublas::vector<double>(8)    shape + values", shape && values);
    }

    // matrix ────────────────────────────────────────────────────────────────
    {
        Matrix<short> M(3, 4);
        for (std::size_t r = 0; r < M.size1(); ++r)
            for (std::size_t c = 0; c < M.size2(); ++c)
                M(r, c) = static_cast<short>(r * M.size2() + c);

        h5::write(fd, "/ublas/mat", M);
        auto back = h5::read<Matrix<short>>(fd, "/ublas/mat");

        bool shape  = (back.size1() == M.size1()) && (back.size2() == M.size2());
        bool values = shape;
        for (std::size_t r = 0; values && r < M.size1(); ++r)
            for (std::size_t c = 0; values && c < M.size2(); ++c)
                values = values && (back(r, c) == M(r, c));
        check("ublas::matrix<short>(3x4)   shape + values", shape && values);
    }
    return 0;
}
