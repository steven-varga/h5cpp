// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// STL × compiler-assisted reflection: a vector of POD-struct records.
//
// The POD `sn::example::Record` is registered as an HDF5 compound type via
// the H5CPP_REGISTER_STRUCT macro in `generated.h` (which the h5cpp compiler
// emits from `struct.h` — committed here so the example builds without the
// compiler installed). Once registered, std::vector<Record> rides the
// existing `pointers` / `linear_value_dataset` path used by any other
// vector<T>.

#include <h5cpp/all>
#include "generated.h"

#include <iostream>
#include <vector>

int main() {
    h5::fd_t fd = h5::create("stl_compound.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // Populate a vector of POD records with index-encoded `idx` field.
    constexpr std::size_t N = 8;
    std::vector<sn::example::Record> src = h5::pod<sn::example::Record>{} | h5::take(N);
    for (std::size_t i = 0; i < src.size(); ++i) src[i].idx = i;

    h5::write(fd, "/orm/records", src);

    auto back = h5::read<std::vector<sn::example::Record>>(fd, "/orm/records");

    bool size_ok = back.size() == src.size();
    bool idx_ok  = size_ok;
    for (std::size_t i = 0; idx_ok && i < src.size(); ++i)
        idx_ok = (back[i].idx == src[i].idx);

    // POD `Record` has no operator<<; pretty-print the `idx` projection.
    std::vector<int> idx; idx.reserve(back.size());
    for (const auto& r : back) idx.push_back(int(r.idx));
    std::cout << "      vector<Record>(8) idx     = " << idx << "\n";

    check("std::vector<sn::example::Record>(8) round-trip (idx field)",
          size_ok && idx_ok);

    // Also write into a chunk+gzip dataset with custom dims to show that
    // the compound path composes with the same property-list flags.
    h5::write(fd, "/orm/records_chunked", src,
            h5::max_dims{H5S_UNLIMITED}, h5::chunk{4} | h5::gzip{6});
    auto back2 = h5::read<std::vector<sn::example::Record>>(fd, "/orm/records_chunked");
    bool ok2 = back2.size() == src.size();
    for (std::size_t i = 0; ok2 && i < src.size(); ++i)
        ok2 = (back2[i].idx == src[i].idx);
    check("                                    chunked+gzip round-trip", ok2);

    return 0;
}
