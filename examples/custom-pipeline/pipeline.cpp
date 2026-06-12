// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// =============================================================================
// h5cpp pipeline demo — the three write paths (#287)
// =============================================================================
//
// How a chunk flows from h5::write to disk is decided at the write site:
//
//   1. direct chunk (DEFAULT)  — a plain, no-hyperslab chunked write goes through
//                                h5cpp's own pipeline: basic_pipeline_t →
//                                H5Dwrite_chunk, filter chain on the calling
//                                thread.  No opt-in.
//
//   2. CAPI hyperslab          — a hyperslab selection (h5::offset/stride/block),
//                                a contiguous dataset, or an HDF5-applied filter
//                                (NBIT/SCALEOFFSET) routes through HDF5's own
//                                chunk processor + filter pipeline (H5Dwrite).
//                                The most flexible path; usually the slowest.
//
//   3. parallel  (h5::threads{N}) — a per-dataset DAPL property fans the filter
//                                stage out across one process-global worker pool:
//                                pool_pipeline_t.  h5::backpressure{M} bounds
//                                in-flight chunks.  H5Dwrite_chunk stays on the
//                                caller; only compression is parallel.
//
// All three write the same logical dataset, then read it back and verify byte
// equality.  Gaussian noise compresses poorly, so the timings here are mostly a
// comparison of pipeline overhead, not filter throughput.

#include <h5cpp/all>

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr std::size_t k_rows   = 1024;
constexpr std::size_t k_cols   = 2048;   // 1024 × 2048 × 8B ≈ 16 MiB
constexpr std::size_t k_chunk  = 64;     // 64 × 2048 doubles per chunk

std::vector<double> make_data() {
    return h5::normal<double>{0.0, 1.0} | h5::take(k_rows * k_cols);
}

template <class Fn> double time_ms(Fn&& fn) {
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();
    fn();
    auto t1 = clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

void section(const char* title) {
    std::cout << "\n" << title << "\n"
              << std::string(std::strlen(title), '-') << "\n";
}

} // namespace

int main() {
    const auto data = make_data();

    // ── 1. direct chunk — the DEFAULT for a no-hyperslab chunked write ──────
    section("1. direct chunk (basic_pipeline_t, default)");
    {
        double t = time_ms([&]{
            auto fd = h5::create("pipeline_default.h5", H5F_ACC_TRUNC);
            h5::write(fd, "dataset", data,  h5::current_dims{k_rows, k_cols},
                h5::chunk{k_chunk, k_cols} | h5::gzip{4});
        });
        auto back = h5::read<std::vector<double>>("pipeline_default.h5", "dataset");
        std::cout << std::fixed << std::setprecision(1)
                  << "  write+read: " << std::setw(7) << t << " ms"
                  << "   roundtrip ok: " << (back == data ? "yes" : "NO")  << "\n";
    }

    // ── 2. CAPI hyperslab — HDF5's own chunk processor + filter pipeline ────
    // A hyperslab selection (h5::offset present) is a COMPILE-TIME signal that
    // routes the write through stock H5Dwrite instead of direct-chunk.
    section("2. CAPI hyperslab (HDF5's own chunk processor + filters)");
    {
        double t = time_ms([&]{
            auto fd = h5::create("pipeline_capi.h5", H5F_ACC_TRUNC);
            auto ds = h5::create<double>(fd, "dataset",
                h5::current_dims{k_rows, k_cols}, h5::chunk{k_chunk, k_cols} | h5::gzip{4});
            h5::write(ds, data.data(), h5::count{k_rows, k_cols}, h5::offset{0, 0});
        });
        auto back = h5::read<std::vector<double>>("pipeline_capi.h5", "dataset");
        std::cout << "  write+read: " << std::setw(7) << t << " ms"
                  << "   roundtrip ok: " << (back == data ? "yes" : "NO")  << "\n";
    }

    // ── 3. h5::threads{N} (+ h5::backpressure{M}) — DAPL pool_pipeline_t ────
    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    section("3. h5::threads + h5::backpressure (DAPL, pool_pipeline_t)");
    std::cout << "  hardware_concurrency() = " << hw << "\n";
    {
        h5::dapl_t dapl = h5::threads{hw} | h5::backpressure{32};
        double t = time_ms([&]{
            auto fd = h5::create("pipeline_threads.h5", H5F_ACC_TRUNC);
            h5::write(fd, "dataset", data,
                h5::current_dims{k_rows, k_cols}, h5::chunk{k_chunk, k_cols} | h5::gzip{4}, dapl);
        });
        auto fd_r = h5::open("pipeline_threads.h5", H5F_ACC_RDONLY);
        auto back = h5::read<std::vector<double>>(fd_r, "dataset");
        std::cout << "  write+read: " << std::setw(7) << t << " ms"
                  << "   roundtrip ok: " << (back == data ? "yes" : "NO") << "\n";
    }
    return 0;
}
