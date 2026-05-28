/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <armadillo>
#include <h5cpp/all>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <random>

// Demonstrates h5cpp sparse round-trip with Armadillo SpMat and SpCol.
// On-disk layout is canonical CSC (data / indices / indptr / shape) and is
// directly readable by scipy.sparse.csc_matrix / 10x Genomics / Loompy.

static arma::SpMat<double> make_random_spmat(std::size_t rows, std::size_t cols, double density) {
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> uni(-1.0, 1.0);
    std::bernoulli_distribution coin(density);
    arma::SpMat<double> M(rows, cols);
    for (std::size_t j = 0; j < cols; ++j)
        for (std::size_t i = 0; i < rows; ++i)
            if (coin(gen)) M(i, j) = uni(gen);
    M.sync();    // flush insert cache before direct CSC array access
    return M;
}

int main() {
    h5::fd_t fd = h5::create("arma.h5", H5F_ACC_TRUNC);

    // ---- matrix ----
    arma::SpMat<double> A = make_random_spmat(8, 12, 0.15);
    std::cout << "wrote  SpMat: " << A.n_rows << "x" << A.n_cols
              << " nnz=" << A.n_nonzero << "\n";
    h5::write(fd, "matrix/A", A);

    auto A_back = h5::read<arma::SpMat<double>>(fd, "matrix/A");
    std::cout << "read   SpMat: " << A_back.n_rows << "x" << A_back.n_cols
              << " nnz=" << A_back.n_nonzero << "\n";

    arma::SpMat<double> diff = A - A_back;
    // Hand-roll max-abs over the iterator to avoid pulling in BLAS via arma::norm.
    double err = 0.0;
    for (auto it = diff.begin(); it != diff.end(); ++it)
        err = std::max(err, std::abs(*it));
    std::cout << "round-trip residual (max|A - A'| on nonzeros): " << err << "\n";

    // ---- vector ----
    arma::SpCol<double> v(50);
    v(3) = 1.5; v(17) = -2.25; v(42) = 7.0;
    v.sync();    // flush insert cache; same precondition as for SpMat
    h5::write(fd, "matrix/v", v);
    auto v_back = h5::read<arma::SpCol<double>>(fd, "matrix/v");
    std::cout << "vec round-trip: nnz=" << v_back.n_nonzero
              << ", v(17)=" << v_back(17) << " (expected -2.25)\n";

    return (err == 0.0) ? 0 : 1;
}
