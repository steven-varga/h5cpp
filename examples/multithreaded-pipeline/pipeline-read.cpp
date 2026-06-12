/* pipeline-read.cpp — h5cpp multithreaded gzip READ pipeline (#287).
 *
 * Reads back the dataset that pipeline-write produced (multithreaded-pipeline.h5)
 * and reports throughput.  Opening the dataset with h5::threads{N} routes the read
 * through pool_pipeline_t: the H5Dread_chunk I/O stays on the caller thread while
 * gzip inflate fans out across the process-global worker pool.  Without the tag the
 * read is the synchronous direct-chunk path.
 *
 * Run multithreaded-pipeline-write first to produce the file.
 *
 * Env knobs (all optional):
 *   H5CPP_BENCH_THREADS  read fan-out (default hardware_concurrency)
 */

#include <h5cpp/all>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include <string>

static std::size_t env(const char* key, std::size_t fallback) {
    const char* v = std::getenv(key);
    return (v && *v) ? std::strtoull(v, nullptr, 10) : fallback;
}

int main() {
    const unsigned N = static_cast<unsigned>(
        env("H5CPP_BENCH_THREADS", std::max(1u, std::thread::hardware_concurrency())));
    std::string path = "multithreaded-pipeline.h5";
    // Size the global worker pool to `N` so the parallel read fans inflate across
    // exactly `N` workers (must run before first pool use).
    h5::impl::set_pool_size(N);

    try {
        auto fd = h5::open(path, H5F_ACC_RDONLY);
        // h5::threads{N} on the open DAPL engages pool_pipeline_t parallel decompression.
        h5::ds_t ds = h5::open(fd, "data", h5::threads{N});

        h5::current_dims_t dims;
        h5::get_simple_extent_dims(h5::get_space(ds), dims);
        const std::size_t n = dims[0];

        std::vector<float> data(n);
        const double mib = static_cast<double>(n * sizeof(float)) / (1024.0 * 1024.0);
        std::cout << "reading " << n << " floats (" << mib << " MiB), threads=" << N << '\n';

        const auto t0 = std::chrono::steady_clock::now();
        h5::read(ds, data.data(), h5::count{n});
        const auto t1 = std::chrono::steady_clock::now();

        const double sec = std::chrono::duration<double>(t1 - t0).count();
        std::cout << "done: " << sec << " s   " << mib / sec << " MiB/s   " << (static_cast<double>(n) / sec) / 1e6 << " Mfloat/s\n";
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "  (run multithreaded-pipeline-write first to create " << path << ")\n";
        return 1;
    }
    return 0;
}
