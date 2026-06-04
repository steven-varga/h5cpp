/* pipeline-write.cpp — h5cpp multithreaded gzip WRITE pipeline (#287).
 *
 * Generates a block of random floats and writes it to a chunked, gzip dataset
 * through the h5cpp worker-pool pipeline. h5::threads{N} (a per-dataset DAPL
 * property) fans chunk compression across the process-global worker pool, while
 * the H5Dwrite_chunk calls stay on the caller thread. Reports throughput.
 *
 * Modeled on the IEX copy_gzip6 benchmark, but with generated data — no input
 * file required.  Read it back with pipeline-read.
 *
 * Env knobs (all optional):
 *   H5CPP_BENCH_N        element count        (default 16,777,216 = 64 MiB)
 *   H5CPP_BENCH_THREADS  worker-pool size     (default hardware_concurrency)
 *   H5CPP_BENCH_CHUNK    chunk size, floats   (default 1,048,576)
 *   H5CPP_BENCH_GZIP     deflate level 0..9   (default 6)
 */

#include <h5cpp/all>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

static std::size_t env(const char* key, std::size_t fallback) {
    const char* v = std::getenv(key);
    return (v && *v) ? std::strtoull(v, nullptr, 10) : fallback;
}

int main() {
    const std::size_t n = env("H5CPP_BENCH_N", std::size_t{16} << 20);  // 16 M floats = 64 MiB
    const unsigned N = static_cast<unsigned>(
        env("H5CPP_BENCH_THREADS", std::max(1u, std::thread::hardware_concurrency())));
    const std::size_t chunk = env("H5CPP_BENCH_CHUNK",   std::size_t{1} << 20);   // 1 M floats / chunk
    const unsigned level = static_cast<unsigned>(env("H5CPP_BENCH_GZIP", 6));
    // Size the process-global worker pool to `thr` so h5::threads{thr} fans the
    // filter stage across exactly `thr` workers (must run before first pool use).
    h5::impl::set_pool_size(N);
    try {
        // ── generate a block of random floats (h5cpp's distribution API) ────
        std::vector<float> data = h5::uniform<float>{0.0f, 1.0f} | h5::take(n);

        const double mib = static_cast<double>(n * sizeof(float)) / (1024.0 * 1024.0);
        std::cout << "writing " << n 
            << " random floats (" << mib << " MiB), gzip-" << level << ", chunk=" << chunk << ", threads=" << N << '\n';

        auto fd = h5::create("multithreaded-pipeline.h5", H5F_ACC_TRUNC);
        const auto t0 = std::chrono::steady_clock::now();
        h5::write(fd, "data", data,  // per-dataset DAPL → fans across the global pool
            h5::current_dims{n},   h5::chunk{chunk} | h5::gzip{level}, h5::threads{N});
        const auto t1 = std::chrono::steady_clock::now();
        const double sec = std::chrono::duration<double>(t1 - t0).count();
        std::cout << "done: " << sec << " s   " << mib / sec << " MiB/s   "  << (static_cast<double>(n) / sec) / 1e6 << " Mfloat/s\n";
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
