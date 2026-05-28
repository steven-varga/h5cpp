// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Armadillo round-trip with shape + element verification.
//
// Each block writes a freshly populated container, reads it back, and asserts
// (1) shape equality and (2) per-element equality. Status is printed using the
// project's ✔/✘ symbol convention.

#include <armadillo>
#include <h5cpp/all>
#include <iostream>

int main() {
    auto fd = h5::create("arma.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // vector ─────────────────────────────────────────────────────────────────
    {
        arma::vec v = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
        h5::write(fd, "/arma/vec", v);
        auto back = h5::read<arma::vec>(fd, "/arma/vec");
        bool ok = (back.n_elem == v.n_elem) && arma::approx_equal(v, back, "absdiff", 0.0);
        check("arma::vec(8)            shape + values", ok);
    }

    // matrix (chunked + gzip) ───────────────────────────────────────────────
    {
        arma::mat M = arma::linspace<arma::mat>(1, 12, 12).reshape(3, 4); 
        h5::write(fd, "/arma/mat", M, h5::chunk{3, 4} | h5::gzip{6});
        auto back = h5::read<arma::mat>(fd, "/arma/mat");
        bool shape  = (back.n_rows == M.n_rows) && (back.n_cols == M.n_cols);
        bool values = shape && arma::approx_equal(M, back, "absdiff", 1e-12);
        check("arma::mat(3x4)          shape + values", shape && values);
    }

    // cube ──────────────────────────────────────────────────────────────────
    {
        arma::cube C(2, 3, 4);
        for (arma::uword s = 0; s < C.n_slices; ++s)
            for (arma::uword r = 0; r < C.n_rows; ++r)
                for (arma::uword c = 0; c < C.n_cols; ++c)
                    C(r, c, s) = double(s * 100 + r * 10 + c);

        h5::write(fd, "/arma/cube", C);
        auto back = h5::read<arma::cube>(fd, "/arma/cube");
        bool shape  = (back.n_rows == C.n_rows)
                   && (back.n_cols == C.n_cols)
                   && (back.n_slices == C.n_slices);
        bool values = shape && arma::approx_equal(C, back, "absdiff", 0.0);
        check("arma::cube(2x3x4)       shape + values", shape && values);
    }
    return 0;
}
