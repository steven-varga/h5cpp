/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <Eigen/SparseCore>
#include <h5cpp/all>

#include <iostream>
#include <random>
#include <vector>

// Demonstrates h5cpp sparse round-trip with Eigen::SparseMatrix and
// Eigen::SparseVector. Eigen sparse storage is ColMajor by default; that
// matches the canonical CSC on-disk layout exactly.

int main() {
    using SpMat = Eigen::SparseMatrix<double>;          // ColMajor, int index
    using SpVec = Eigen::SparseVector<double>;

    h5::fd_t fd = h5::create("eigen.h5", H5F_ACC_TRUNC);

    // ---- matrix ----
    std::mt19937 gen(7);
    std::uniform_real_distribution<double> uni(-1.0, 1.0);
    std::bernoulli_distribution coin(0.20);
    const int rows = 10, cols = 15;
    std::vector<Eigen::Triplet<double>> trips;
    for (int j = 0; j < cols; ++j)
        for (int i = 0; i < rows; ++i)
            if (coin(gen)) trips.emplace_back(i, j, uni(gen));
    SpMat A(rows, cols);
    A.setFromTriplets(trips.begin(), trips.end());
    A.makeCompressed();   // required precondition for h5::write
    std::cout << "wrote  SparseMatrix: " << A.rows() << "x" << A.cols()
              << " nnz=" << A.nonZeros() << "\n";
    h5::write(fd, "matrix/A", A);

    SpMat A_back = h5::read<SpMat>(fd, "matrix/A");
    std::cout << "read   SparseMatrix: " << A_back.rows() << "x" << A_back.cols()
              << " nnz=" << A_back.nonZeros() << "\n";

    Eigen::MatrixXd diff = Eigen::MatrixXd(A) - Eigen::MatrixXd(A_back);
    double err = diff.cwiseAbs().maxCoeff();
    std::cout << "round-trip residual (max|A - A'|): " << err << "\n";

    // ---- vector ----
    SpVec v(40);
    v.insert(4) = 0.5; v.insert(11) = -3.14; v.insert(33) = 8.0;
    h5::write(fd, "matrix/v", v);
    SpVec v_back = h5::read<SpVec>(fd, "matrix/v");
    std::cout << "vec round-trip: nnz=" << v_back.nonZeros()
              << ", v.coeff(11)=" << v_back.coeff(11) << " (expected -3.14)\n";

    return (err == 0.0) ? 0 : 1;
}
