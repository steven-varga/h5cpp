// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Translation unit #2.  Independently includes generated.h — proving that the
// inline H5CPP_REGISTER_STRUCT bodies survive multiple TU inclusions without
// ODR violations.

#include <h5cpp/all>
#include "generated.h"

#include <iostream>

void tu_02_pod_vector_round_trip(const h5::fd_t& fd) {
    // Build a small vector of POD records.  The h5::pod<T> | h5::take(n)
    // adaptor default-constructs records; we fill in the discriminator field
    // so the readback is recognisable.
    auto records = h5::pod<sn::example::record_t>{} | h5::take(8);
    for (size_t i = 0; i < records.size(); ++i)
        records[i].idx = i;

    // One-shot write into a fresh path.
    h5::write(fd, "/orm/partial/one_shot", records);

    // Same data, custom chunking + gzip.
    h5::write(fd, "/orm/partial/custom_dims", records,
        h5::max_dims{H5S_UNLIMITED}, h5::gzip{9} | h5::chunk{20});

    // Read it back through the type system defined in *this* TU.
    auto back = h5::read<std::vector<sn::example::record_t>>(
        fd, "/orm/partial/one_shot");

    std::vector<my_uint_t> idx;
    idx.reserve(back.size());
    for (const auto& r : back) idx.push_back(r.idx);
    std::cout << "tu-02: read back " << back.size()
              << " records, idx = " << idx << "\n";
}
