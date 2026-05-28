// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Lean inner-loop I/O.
//
// The h5cpp call shape `h5::write(ds, ref, h5::offset{...}, h5::count{...}, ...)`
// looks heavy but compiles to almost nothing: every optional argument is a
// small stack-allocated POD (`h5::offset_t` etc. are `H5CPP_MAX_RANK * hsize_t`
// arrays), and the unused branches are eliminated at compile time via
// `if constexpr`. Two equivalent patterns are shown:
//
//   1. SUGGESTED  — construct the argument structs inline on every call.
//                   Readable; the compiler hoists the constants for you.
//   2. EXTREME    — hoist offset/count out of the loop and mutate in place.
//                   Worth it only when a profiler points at this loop.
//
// Both patterns produce the same on-disk layout.

#include <armadillo>
#include <h5cpp/all>
#include <iostream>

int main() {
    h5::fd_t fd = h5::create("optimized.h5", H5F_ACC_TRUNC);

    // A 10-row column vector — the unit we'll move around the dataset.
    arma::Col<short> M(10, arma::fill::zeros);

    // 10 × unlimited dataset of short, chunked + gzipped, zero-filled.
    h5::ds_t ds = h5::create<short>(fd, "huge dataset",
            h5::current_dims{10, 100},
            h5::max_dims{10, H5S_UNLIMITED},
            h5::chunk{10, 10} | h5::gzip{9} | h5::fill_value<short>{0});

    // ── 1. SUGGESTED: inline argument structs ────────────────────────────────
    // For each i in 0..3, fill the column vector with i+1 and write it into
    // column i of the dataset.  `count` is implied by M's shape ({10, 1}).
    for (short i = 0; i < 4; ++i) {
        M.fill(static_cast<short>(i + 1));
        h5::write(ds, M, h5::offset{0, static_cast<hsize_t>(i)});
    }

    // ── 2. EXTREME: hoist offset/count out of the loop ───────────────────────
    // Same behaviour, but offset[1] is mutated in place — no fresh POD on
    // each iteration.  Profile first; this is rarely the bottleneck.
    h5::offset offset{0, 0};
    h5::count  count {10, 1};
    for (short i = 10; i < 15; ++i) {
        offset[1] = static_cast<hsize_t>(i);
        M.fill(static_cast<short>(i + 1));
        h5::write(ds, M, offset, count);
    }

    // ── Verify: read each column individually and print its head value ───────
    // Per-column reads are layout-free — a 10×1 slab is just 10 contiguous
    // shorts regardless of row/column-major interpretation.
    std::vector<int> head_of_each_col(15);
    for (int c = 0; c < 15; ++c) {
        auto col = h5::read<arma::Col<short>>(ds,
                h5::offset{0, static_cast<hsize_t>(c)}, h5::count{10, 1});
        head_of_each_col[c] = col(0);
    }
    std::cout << "col[0..14](row 0): " << head_of_each_col << "\n";

    // Column 11 was filled with 12s — read the whole column and prove it.
    auto col11 = h5::read<arma::Col<short>>(ds,
            h5::offset{0, 11}, h5::count{10, 1});
    std::vector<int> col11_vals(col11.n_elem);
    for (arma::uword r = 0; r < col11.n_elem; ++r) col11_vals[r] = col11(r);
    std::cout << "column 11        : " << col11_vals << "\n";

    return 0;
}
