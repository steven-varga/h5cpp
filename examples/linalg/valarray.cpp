// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// std::valarray round-trip with shape + element verification. The minimum
// linalg surface — STL ships it, no third-party dependency.

#include <iostream>
#include <valarray>
#include <h5cpp/all>

int main() {
    auto fd = h5::create("valarray.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    auto eq = [](const auto& a, const auto& b) {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i)
            if (a[i] != b[i]) return false;
        return true;
    };

    // valarray<double> ──────────────────────────────────────────────────────
    {
        std::valarray<double> v(10);
        for (std::size_t i = 0; i < v.size(); ++i) v[i] = i + 1.0;
        h5::write(fd, "/valarray/vec", v);
        auto back = h5::read<std::valarray<double>>(fd, "/valarray/vec");

        bool shape  = (back.size() == v.size());
        bool values = shape && eq(v, back);
        check("std::valarray<double>(10)  shape + values", shape && values);
    }

    // valarray<int> with chunked + gzip ─────────────────────────────────────
    {
        std::valarray<int> w(20);
        for (std::size_t i = 0; i < w.size(); ++i) w[i] = static_cast<int>(i * i);
        h5::write(fd, "/valarray/squares", w, h5::chunk{20} | h5::gzip{6});
        auto back = h5::read<std::valarray<int>>(fd, "/valarray/squares");

        bool shape  = (back.size() == w.size());
        bool values = shape && eq(w, back);
        check("std::valarray<int>(20) gz  shape + values", shape && values);
    }
    return 0;
}
