// Minimal Newmat stub for h5cpp integration
#ifndef NEWMAT_H
#define NEWMAT_H
#include <cstddef>

namespace NEWMAT {

typedef double Real;

class Matrix {
    Real* data_;
    int nrows_, ncols_;
public:
    Matrix(int r, int c) : nrows_(r), ncols_(c) { data_ = new Real[r*c]; }
    ~Matrix() { delete[] data_; }
    Real* data() { return data_; }
    const Real* data() const { return data_; }
    int nrows() const { return nrows_; }
    int ncols() const { return ncols_; }
    Real& operator()(int i, int j) { return data_[i*ncols_ + j]; }
    const Real& operator()(int i, int j) const { return data_[i*ncols_ + j]; }
};

class ColumnVector {
    Real* data_;
    int size_;
public:
    ColumnVector(int n) : size_(n) { data_ = new Real[n]; }
    ~ColumnVector() { delete[] data_; }
    Real* data() { return data_; }
    const Real* data() const { return data_; }
    int size() const { return size_; }
    Real& operator()(int i) { return data_[i]; }
    const Real& operator()(int i) const { return data_[i]; }
};

class RowVector {
    Real* data_;
    int size_;
public:
    RowVector(int n) : size_(n) { data_ = new Real[n]; }
    ~RowVector() { delete[] data_; }
    Real* data() { return data_; }
    const Real* data() const { return data_; }
    int size() const { return size_; }
    Real& operator()(int i) { return data_[i]; }
    const Real& operator()(int i) const { return data_[i]; }
};

} // namespace NEWMAT
#endif
