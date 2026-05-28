


// ── verification + comparison helpers ──────────────────────────────────────
static void check(const char* label, bool ok) {
    std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
}

static bool same(const arma::mat& a, const arma::mat& b) {
    return a.n_rows == b.n_rows && a.n_cols == b.n_cols
        && arma::approx_equal(a, b, "absdiff", 0.0);
}
static bool same(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b) {
    return a.rows() == b.rows() && a.cols() == b.cols() && a == b;
}
static bool same(const blitz::Array<double, 2>& a, const blitz::Array<double, 2>& b) {
    if (a.rows() != b.rows() || a.cols() != b.cols()) return false;
    for (int r = 0; r < a.rows(); ++r)
        for (int c = 0; c < a.cols(); ++c) if (a(r, c) != b(r, c)) return false;
    return true;
}
static bool same(const dlib::matrix<double>& a, const dlib::matrix<double>& b) {
    if (a.nr() != b.nr() || a.nc() != b.nc()) return false;
    for (long r = 0; r < a.nr(); ++r)
        for (long c = 0; c < a.nc(); ++c) if (a(r, c) != b(r, c)) return false;
    return true;
}
static bool same(const boost::numeric::ublas::matrix<double>& a,
                 const boost::numeric::ublas::matrix<double>& b) {
    if (a.size1() != b.size1() || a.size2() != b.size2()) return false;
    for (std::size_t r = 0; r < a.size1(); ++r)
        for (std::size_t c = 0; c < a.size2(); ++c) if (a(r, c) != b(r, c)) return false;
    return true;
}
static bool same(const xt::xarray<double>& a, const xt::xarray<double>& b) {
    return a == b;
}
static bool same(const std::valarray<double>& a, const std::valarray<double>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) if (a[i] != b[i]) return false;
    return true;
}