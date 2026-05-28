// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// =============================================================================
// h5cpp pipeline demo
// =============================================================================
//
// Three pipeline opt-in surfaces, one per section:
//
//   1. default                           — basic_pipeline_t, no flag needed.
//                                          Single-threaded chunk tiling +
//                                          filter chain (gzip, shuffle, ...).
//
//   2. h5::high_throughput  (DAPL)        — per-dataset opt-in. Allocates a
//                                          fresh pipeline scratch buffer on
//                                          DAPL copy (fix for issue #242 — the
//                                          historical 1.12.x double-free).
//                                          Still basic_pipeline_t under the
//                                          hood today; the flag is the
//                                          per-dataset switch the future
//                                          threaded_pipeline_t will hang off.
//
//   3. h5::threads{N} (+ h5::backpressure{M})  (FAPL)
//                                       — pool_pipeline_t. A worker_pool_t is
//                                          allocated at h5::create time and
//                                          shared across every dataset in the
//                                          file. Filter work parallelises
//                                          across N workers; backpressure
//                                          caps in-flight chunks at M.
//
// Pipelines listed but NOT demonstrated (architecture-notes line 133-135):
//   threaded_pipeline_t, romio_pipeline_t, hadoop_pipeline_t — STUBs.
//
// The example sizes the data so gzip has actual work to do (otherwise the
// pipeline machinery overhead dominates timing). All three modes write the
// same logical dataset, then read it back and verify byte equality.

#include <h5cpp/all>

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

// 16 MiB of test data. Sourced from h5cpp's H5Uall.hpp generators — the same
// `h5::normal<T>{mean, stddev} | h5::take(n)` pipe used elsewhere in the
// examples. Gaussian noise compresses poorly (~1:1 with gzip), so the
// comparison here is primarily about *pipeline overhead*, not filter
// throughput. Switch the generator (e.g. `h5::uniform<int>{0,15}` cast to
// double) if you want gzip to do real work.
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

    // ── 1. default — basic_pipeline_t, no flag ─────────────────────────────
    section("1. default (basic_pipeline_t, single-threaded)");
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

    // ── 2. h5::high_throughput — DAPL per-dataset opt-in ───────────────────
    // The flag is consumed by building an explicit dapl_t that h5::create<>()
    // can attach to the dataset, then re-applying the same dapl on the read
    // side so H5Pexist(dapl, H5CPP_DAPL_HIGH_THROUGHPUT) sees the property.
    section("2. h5::high_throughput (DAPL, basic_pipeline_t with per-DAPL scratch)");
    {
        double t = time_ms([&]{
            auto fd = h5::create("pipeline_high_throughput.h5", H5F_ACC_TRUNC);
            // Pre-create the dataset with the DAPL so the high_throughput
            // property is attached at construction time, then write into it.
            auto ds = h5::create<double>(fd, "dataset",
                h5::current_dims{k_rows, k_cols},  h5::chunk{k_chunk, k_cols} | h5::gzip{4}, h5::high_throughput);
            h5::write(ds, data.data(), h5::count{k_rows, k_cols});
        });
        auto back = h5::read<std::vector<double>>(
            "pipeline_high_throughput.h5", "dataset", h5::high_throughput);
        std::cout << "  write+read: " << std::setw(7) << t << " ms"
                  << "   roundtrip ok: " << (back == data ? "yes" : "NO")  << "\n";
    }

    // ── 3. h5::threads{N} (+ h5::backpressure{M}) — FAPL pool_pipeline_t ───
    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    section("3. h5::threads + h5::backpressure (FAPL, pool_pipeline_t)");
    std::cout << "  hardware_concurrency() = " << hw << "\n";
    {
        h5::fapl_t fapl = h5::threads{hw} | h5::backpressure{32};
        double t = time_ms([&]{
            auto fd = h5::create("pipeline_threads.h5", H5F_ACC_TRUNC,h5::default_fcpl, fapl);
            h5::write(fd, "dataset", data,
                h5::current_dims{k_rows, k_cols}, h5::chunk{k_chunk, k_cols} | h5::gzip{4});
        });
        auto fd_r = h5::open("pipeline_threads.h5", H5F_ACC_RDONLY, fapl);
        auto back = h5::read<std::vector<double>>(fd_r, "dataset");
        std::cout << "  write+read: " << std::setw(7) << t << " ms"
                  << "   roundtrip ok: " << (back == data ? "yes" : "NO") << "\n";
    }
    return 0;
}
