// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Packet-table throughput: three ways to land an 8 × 128k matrix of doubles
// onto disk as eight 1-D chunks of length 128k, timed against each other,
// then a real-world movie-clip stream for the applied case.
//
//   1. element-wise append   — h5::append(pt, scalar) per element. Honest
//                              measure of the per-call dispatch cost.
//   2. row-wise append       — one h5::append(pt, row) per row of the matrix.
//                              Hot path for streaming time-series.
//   3. raw-pointer append    — h5::append(pt, ptr) feeds a chunk-sized buffer
//                              straight through with no internal copy.
//   4. movie clip            — applied case: 24 grayscale frames of 64x64
//                              uint8_t into a 3-D unlimited dataset, one
//                              chunk = one frame.
//
// Variants 1–3 produce the same on-disk layout (8 rows × 128k columns);
// only time-to-write differs.

#include <armadillo>
#include <h5cpp/all>
#include <chrono>
#include <functional>
#include <iostream>

static double time_us(std::function<void()> body) {
    using clock = std::chrono::high_resolution_clock;
    auto t0 = clock::now();
    body();
    auto t1 = clock::now();
    return std::chrono::duration<double, std::micro>(t1 - t0).count();
}

int main() {
    constexpr std::size_t nrows = 8;
    constexpr std::size_t ncols = 128 * 1024;   // 128k doubles per row

    auto fd = h5::create("packet_batches.h5", H5F_ACC_TRUNC);
    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // ── Variant 1: element-wise append ──────────────────────────────────────
    double t1_us;
    {
        h5::pt_t pt = h5::create<double>(fd, "/elementwise",
            h5::max_dims{H5S_UNLIMITED, ncols},  h5::chunk{1, ncols});

        arma::mat M(nrows, ncols, arma::fill::zeros);   // deterministic
        for (std::size_t i = 0; i < nrows; ++i)
            for (std::size_t j = 0; j < ncols; ++j)
                M(i, j) = static_cast<double>(j + i * 0.001);

        t1_us = time_us([&]() {
            for (std::size_t i = 0; i < nrows; ++i)
                for (std::size_t j = 0; j < ncols; ++j)
                    h5::append(pt, M(i, j));
        });

        pt = h5::pt_t{};   // flush
        auto back = h5::read<std::vector<double>>(fd, "/elementwise");
        check("variant 1: element-wise append      shape (8 x 128k)",
              back.size() == nrows * ncols);
    }

    // ── Variant 2: row-wise append ──────────────────────────────────────────
    double t2_us;
    {
        h5::pt_t pt = h5::create<double>(fd, "/rowwise",
            h5::max_dims{H5S_UNLIMITED, ncols}, h5::chunk{1, ncols});

        arma::mat M(nrows, ncols);
        for (std::size_t i = 0; i < nrows; ++i)
            M.row(i) = arma::randu<arma::rowvec>(ncols) + static_cast<double>(i);

        t2_us = time_us([&]() {
            for (std::size_t i = 0; i < nrows; ++i) {
                arma::rowvec r = M.row(i);   // materialize the subview into contig memory
                const double* p = r.memptr();
                h5::append<double>(pt, p);   // explicit T disambiguates to the pointer overload
            }
        });

        pt = h5::pt_t{};
        auto back = h5::read<std::vector<double>>(fd, "/rowwise");
        check("variant 2: row-wise append           shape (8 x 128k)",
              back.size() == nrows * ncols);
    }

    // ── Variant 3: raw-pointer append ───────────────────────────────────────
    //    arma::mat is column-major; transpose so each contiguous run of `ncols`
    //    doubles is one row's worth of data.
    double t3_us;
    {
        h5::pt_t pt = h5::create<double>(fd, "/rawptr",
            h5::max_dims{H5S_UNLIMITED, ncols}, h5::chunk{1, ncols});

        arma::mat M(nrows, ncols);
        for (std::size_t i = 0; i < nrows; ++i)
            M.row(i) = arma::randu<arma::rowvec>(ncols) + static_cast<double>(i);
        arma::mat T = M.t();   // now T is (ncols × nrows) col-major; each col == one row of M

        t3_us = time_us([&]() {
            for (std::size_t i = 0; i < nrows; ++i)
                h5::append<double>(pt, T.colptr(i));  // explicit T → pointer overload
        });

        pt = h5::pt_t{};
        auto back = h5::read<std::vector<double>>(fd, "/rawptr");
        check("variant 3: raw-pointer append        shape (8 x 128k)",
              back.size() == nrows * ncols);
    }

    std::cout << "\ntimings (microseconds, lower is better):\n"
              << "  variant 1 element-wise : " << t1_us << "\n"
              << "  variant 2 row-wise     : " << t2_us << "\n"
              << "  variant 3 raw-pointer  : " << t3_us << "\n\n";

    // ── Variant 4: movie clip ───────────────────────────────────────────────
    //    Real-world streaming pattern: 24 grayscale frames of 64x64 uint8_t,
    //    one chunk per frame. Each frame encodes its index so we can verify
    //    on readback. Uses the raw-pointer append path.
    constexpr std::size_t nframes = 24;
    constexpr std::size_t height  = 64;
    constexpr std::size_t width   = 64;
    constexpr std::size_t frame_bytes = height * width;

    double t4_us;
    {
        h5::pt_t pt = h5::create<std::uint8_t>(fd, "/movie",
            h5::max_dims{H5S_UNLIMITED, height, width}, h5::chunk{1, height, width} | h5::gzip{6});

        std::vector<std::uint8_t> frame(frame_bytes);
        t4_us = time_us([&]() {
            for (std::size_t f = 0; f < nframes; ++f) {
                // synthetic content: a moving diagonal that depends on `f`
                for (std::size_t y = 0; y < height; ++y)
                    for (std::size_t x = 0; x < width; ++x)
                        frame[y * width + x] = static_cast<std::uint8_t>((x + y + f) & 0xff);
                h5::append<std::uint8_t>(pt, frame.data());
            }
        });
    }   // pt destructor flushes

    auto movie = h5::read<std::vector<std::uint8_t>>(fd, "/movie");
    bool size_ok = movie.size() == nframes * frame_bytes;
    bool first_ok = size_ok;
    bool last_ok  = size_ok;
    for (std::size_t y = 0; first_ok && y < height; ++y)
        for (std::size_t x = 0; first_ok && x < width; ++x)
            first_ok = (movie[y * width + x] == static_cast<std::uint8_t>((x + y + 0) & 0xff));
    const std::size_t last_off = (nframes - 1) * frame_bytes;
    for (std::size_t y = 0; last_ok && y < height; ++y)
        for (std::size_t x = 0; last_ok && x < width; ++x)
            last_ok = (movie[last_off + y * width + x]
                       == static_cast<std::uint8_t>((x + y + (nframes - 1)) & 0xff));
    check("variant 4: movie clip               24 x 64 x 64 frames", size_ok && first_ok && last_ok);
    std::cout << "  variant 4 movie clip (write only): " << t4_us << " us\n";
    return 0;
}
