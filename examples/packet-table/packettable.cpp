// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Packet-table writers — append-only streams that buffer up to a chunk's worth
// of records in memory and flush when full. The point is the call shape:
//
//     auto pt = h5::create<T>(fd, "...", h5::max_dims{H5S_UNLIMITED, ...}, h5::chunk{...});
//     for (auto record : stream) h5::append(pt, record);
//
// `h5::pt_t` is just `h5::ds_t` with a thin write-cache layered on top, so
// every dataset creation flag — `h5::chunk`, `h5::gzip`, `h5::fill_value`, ...
// — applies unchanged.

#include <Eigen/Dense>          // include before h5cpp/all to enable the eigen mapper
#include <armadillo>
#include <h5cpp/all>
#include "generated.h"          // registers sn::example::Record via H5CPP_REGISTER_STRUCT

#include <iostream>
#include <numeric>
#include <vector>

template <class T> using RowMajor = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

int main() {
    auto fd = h5::create("packettable.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // Packet-table aligns the on-disk extent to chunk boundaries.  Streams
    // below are sized to whole-chunk multiples so the read-back has no
    // fill-value tail.  See the README for what happens with partial chunks.

    // 1. stream of int into a 1-D unlimited dataset ──────────────────────────
    {
        std::vector<int> stream(80);     // 5 chunks of 16
        std::iota(stream.begin(), stream.end(), 1);
        {
            h5::pt_t pt = h5::create<int>(fd, "/stream/int_1d",
                    h5::max_dims{H5S_UNLIMITED},
                    h5::chunk{16} | h5::gzip{6} | h5::fill_value<int>(0));
            for (int x : stream) h5::append(pt, x);
        }   // pt destructor flushes the trailing partial chunk
        auto back = h5::read<std::vector<int>>(fd, "/stream/int_1d");
        check("stream<int> -> 1D unlimited dataset      (80 elements)",
              back.size() == stream.size() && back == stream);
    }

    // 2. stream of int into a 3-D unlimited dataset (frame stream) ──────────
    //    Each appended int fills one cell of a 3×5 frame; once 15 ints land
    //    a frame is complete and the leading dim grows by one.  Chunk holds
    //    two frames, so we use 4 frames = 60 ints to land cleanly.
    {
        std::vector<int> stream(60);     // 4 frames; 2-frame chunks; clean
        std::iota(stream.begin(), stream.end(), 1);
        {
            h5::pt_t pt = h5::create<int>(fd, "/stream/int_3d",
                    h5::max_dims{H5S_UNLIMITED, 3, 5},
                    h5::chunk{2, 3, 5} | h5::gzip{6} | h5::fill_value<int>(0));
            for (int x : stream) h5::append(pt, x);
        }
        auto back = h5::read<std::vector<int>>(fd, "/stream/int_3d");
        check("stream<int> -> 3D unlimited dataset      (4 x 3 x 5)",
              back.size() == stream.size() && back == stream);
    }

    // 3. stream of POD struct into a 1-D unlimited dataset ──────────────────
    {
        auto stream = h5::pod<sn::example::Record>{} | h5::take(128);   // 4 chunks of 32
        for (size_t i = 0; i < stream.size(); ++i) stream[i].idx = i;

        {
            h5::pt_t pt = h5::create<sn::example::Record>(fd, "/stream/record",
                    h5::max_dims{H5S_UNLIMITED},
                    h5::chunk{32} | h5::gzip{6});
            for (const auto& r : stream) h5::append(pt, r);
        }
        auto back = h5::read<std::vector<sn::example::Record>>(fd, "/stream/record");
        bool size_ok = back.size() == stream.size();
        bool idx_ok = size_ok;
        for (size_t i = 0; idx_ok && i < back.size(); ++i)
            idx_ok = (back[i].idx == stream[i].idx);
        check("stream<POD struct> -> 1D unlimited        (128 records, idx)",
              size_ok && idx_ok);
    }

    // 4. stream of fixed-size matrices into a 3-D unlimited dataset ─────────
    //    A "frame" is a 2 x 256 row-major float matrix; we stream `nframes`
    //    of them and the leading dim grows.
    {
        constexpr size_t nrows = 2, ncols = 256, nframes = 20;
        RowMajor<float> frame(nrows, ncols);
        for (size_t i = 0; i < nrows; ++i)
            for (size_t j = 0; j < ncols; ++j)
                frame(i, j) = static_cast<float>(i * ncols + j);
        {
            h5::pt_t pt = h5::create<float>(fd, "/stream/frames",
                    h5::max_dims{H5S_UNLIMITED, nrows, ncols},
                    h5::chunk{1, nrows, ncols});
            for (size_t f = 0; f < nframes; ++f) h5::append(pt, frame);
        }

        // Read back; size should be nframes * nrows * ncols.
        auto back = h5::read<std::vector<float>>(fd, "/stream/frames");
        bool size_ok = back.size() == nframes * nrows * ncols;
        bool values_ok = size_ok;
        // Compare the first frame's worth of data.
        for (size_t k = 0; values_ok && k < nrows * ncols; ++k)
            values_ok = (back[k] == static_cast<float>(k));
        check("stream<Eigen frame> -> 3D unlimited   (20 x 2 x 256)",
              size_ok && values_ok);
    }
    return 0;
}
