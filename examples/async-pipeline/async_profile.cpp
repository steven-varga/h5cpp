// Copyright (c) 2018-2026 Steven Varga / Varga Labs — MIT
//
// #287 async write profiling — measure the multithread collector write path.
//
// Under -DH5CPP_MULTITHREAD every h5::write + handle close funnels through the
// process-global collector (the single HDF5 thread) while gzip/zstd fans out
// across the h5::threads{N} worker pool.  This harness writes a low-entropy
// (compressible) dataset and reports logical MB/s, so the path can be profiled
// and its I/O properties varied.  In the classic build it is the same code with
// no collector routing.
//
// Tunables (env):
//   H5CPP_BENCH_N            element count (doubles)   [default 8Mi = 64 MiB]
//   H5CPP_BENCH_CHUNK        elements per chunk        [default 64Ki = 512 KiB]
//   H5CPP_BENCH_GZIP_LEVEL   zlib level                [default 6]
//   H5CPP_BENCH_THREADS      worker pool size          [default min(16,hw)]
//   H5CPP_BENCH_FILTERS      1=gzip, 0=raw chunks      [default 1]
//   H5CPP_BENCH_DIR          output directory          [default /tmp]
//
// Profile with: /usr/bin/perf stat -e task-clock,context-switches,cpu-migrations \
//                 ./examples-async-pipeline
#include <h5cpp/all>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace {
unsigned envu(const char* k, unsigned d) {
    if (const char* e = std::getenv(k)) return static_cast<unsigned>(std::strtoul(e, nullptr, 10));
    return d;
}
std::string envs(const char* k, const char* d) {
    if (const char* e = std::getenv(k)) return e;
    return d;
}
} // namespace

int main() {
    const std::size_t n        = envu("H5CPP_BENCH_N", 8u * 1024u * 1024u);  // 8Mi doubles = 64 MiB
    const std::size_t chunk    = envu("H5CPP_BENCH_CHUNK", 64u * 1024u);     // 512 KiB/chunk
    const unsigned    gzip     = envu("H5CPP_BENCH_GZIP_LEVEL", 6u);
    const unsigned    threads  = envu("H5CPP_BENCH_THREADS",
                                      std::min(16u, std::max(1u, std::thread::hardware_concurrency())));
    const bool        filters  = envu("H5CPP_BENCH_FILTERS", 1u) != 0u;
    const std::string dir      = envs("H5CPP_BENCH_DIR", "/tmp");
    const std::string path     = dir + "/h5cpp-async-profile.h5";
    const std::size_t logical  = n * sizeof(double);

    // Low-entropy doubles (16 distinct values): gzip has real, parallelizable work.
    std::vector<double> data(n);
    std::mt19937_64 rng(42);
    for (auto& x : data) x = static_cast<double>(rng() & 0xFu);

    std::printf("async write profile (#287)\n"
                "  logical  : %zu MiB (%zu doubles)\n"
                "  chunk    : %zu KiB\n"
                "  gzip     : %u\n"
                "  threads  : %u  (hw %u)\n"
                "  filters  : %s\n"
                "  out      : %s\n",
                logical >> 20, n, (chunk * sizeof(double)) >> 10, gzip,
                threads, std::thread::hardware_concurrency(),
                filters ? "gzip" : "none", path.c_str());

    auto run = [&] {
        std::remove(path.c_str());
        h5::fapl_t fapl = h5::threads{threads};
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);
        // h5::high_throughput (DAPL) engages the parallel pool_pipeline_t; without
        // it the write falls back to stock single-threaded HDF5 filters.
        if (filters)
            h5::write(fd, "data", data, h5::current_dims{n},
                      h5::chunk{chunk} | h5::gzip{gzip}, h5::high_throughput);
        else
            h5::write(fd, "data", data, h5::current_dims{n},
                      h5::chunk{chunk}, h5::high_throughput);
    };

    using clk = std::chrono::steady_clock;
    run();   // warm-up (pool spin-up, first-touch)
    double best_ms = 1e30;
    for (int i = 0; i < 3; ++i) {
        const auto t0 = clk::now();
        run();
        const auto t1 = clk::now();
        best_ms = std::min(best_ms, std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    const double mbps = (static_cast<double>(logical) / 1e6) / (best_ms / 1e3);

    const auto back = h5::read<std::vector<double>>(path, "data");
    const bool ok   = (back == data);
    std::printf("  best of 3 : %.1f ms   %.1f MB/s   roundtrip=%s\n",
                best_ms, mbps, ok ? "ok" : "FAIL");
    std::remove(path.c_str());
    return ok ? 0 : 1;
}
