// #287 — process-global HDF5 lock + multithread write path.
//
// The headline property: under -DH5CPP_MULTITHREAD several producer threads may
// h5::write distinct datasets into ONE file concurrently, and every HDF5 C-API
// call is serialized by the process-global HDF5 lock (on_collector) — so a
// Threadsafety-OFF HDF5 never sees two threads at once.  These tests must pass
// under clang-20 -fsanitize=thread (HDF5 1.12.3): no data race, correct
// round-trip.  Without -DH5CPP_MULTITHREAD the concurrent-writer body is a
// compile-time no-op; the single-threaded round-trips run in every build mode.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>

#include <atomic>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

namespace {
std::vector<double> make_data(std::size_t n, int seed) {
    std::vector<double> v(n);
    for (std::size_t i = 0; i < n; ++i) v[i] = static_cast<double>((seed * 7 + i) & 0xF);
    return v;
}
} // namespace

// ─── 1. single write round-trip (filtered) ──────────────────────────────────
TEST_CASE("[#287] write + read round-trip (gzip)") {
    const char* path = "test-287-rt.h5";
    std::remove(path);
    const auto data = make_data(4096, 1);
    {
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC,
            h5::default_fcpl, h5::fapl_t{h5::threads{4}});
        h5::write(fd, "data", data,
                  h5::current_dims{data.size()}, h5::chunk{512} | h5::gzip{6});
    } // fd closes (under the global HDF5 lock under H5CPP_MULTITHREAD)
    auto back = h5::read<std::vector<double>>(path, "data");
    CHECK(back == data);
    std::remove(path);
}

// ─── 2. no-filter write round-trip ───────────────────────────────────────────
TEST_CASE("[#287] write + read round-trip (no filter)") {
    const char* path = "test-287-raw.h5";
    std::remove(path);
    const auto data = make_data(4096, 2);
    {
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC,
            h5::default_fcpl, h5::fapl_t{h5::threads{4}});
        h5::write(fd, "raw", data,
                  h5::current_dims{data.size()}, h5::chunk{512});
    }
    auto back = h5::read<std::vector<double>>(path, "raw");
    CHECK(back == data);
    std::remove(path);
}

// ─── 3a. SEQUENTIAL multiple datasets into one file ──────────────────────────
TEST_CASE("[#287] many datasets into one file (sequential)") {
    const char* path = "test-287-seq.h5";
    std::remove(path);
    constexpr int M = 8;
    std::vector<std::vector<double>> sets(M);
    for (int k = 0; k < M; ++k) sets[k] = make_data(2048, k + 3);
    {
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC,
            h5::default_fcpl, h5::fapl_t{h5::threads{4}});
        for (int k = 0; k < M; ++k)
            h5::write(fd, "ds" + std::to_string(k), sets[k],
                      h5::current_dims{sets[k].size()}, h5::chunk{256} | h5::gzip{4});
    }
    for (int k = 0; k < M; ++k) {
        auto back = h5::read<std::vector<double>>(path, "ds" + std::to_string(k));
        CHECK(back == sets[k]);
    }
    std::remove(path);
}

// ─── 3b. CONCURRENT writers — the headline safety claim ──────────────────────
//
// Several producer threads write distinct datasets into one file at once.  Under
// -DH5CPP_MULTITHREAD every h5::write + every handle close is serialized by the
// process-global HDF5 lock, so the M create+write+close operations run one at a
// time inside HDF5 (race-free, passes under TSan).
//
// CONSTRAINT (real, documented): producers must not construct/destruct HDF5
// property lists per call.  `h5::chunk{..} | h5::gzip{..}` builds a DCPL via
// H5Pcreate/H5Pset and frees it via H5Pclose IN THE CALLER'S EXPRESSION, on the
// producer thread — which races.  So for concurrent writers, build the property
// list ONCE (single-threaded) and pass it by reference.  With that, producers
// touch zero HDF5 directly and the path is race-free.
//
// This thread-spawning body is only safe/meaningful under H5CPP_MULTITHREAD; in
// the classic single-threaded build it is a compile-time no-op.
TEST_CASE("[#287] concurrent writers — one file, one collector") {
#ifdef H5CPP_MULTITHREAD
    const char* path = "test-287-concurrent.h5";
    std::remove(path);
    constexpr int M = 8;
    std::vector<std::vector<double>> sets(M);
    for (int k = 0; k < M; ++k) sets[k] = make_data(2048, k + 3);

    std::atomic<int> errors{0};
    {
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC,
            h5::default_fcpl, h5::fapl_t{h5::threads{4}});
        // Build the chunk/gzip DCPL ONCE, on this thread — producers share it by
        // reference and so make no HDF5 property-list calls of their own.
        h5::dcpl_t dcpl = h5::chunk{256} | h5::gzip{4};

        std::vector<std::thread> ths;
        ths.reserve(M);
        for (int k = 0; k < M; ++k) {
            ths.emplace_back([&, k] {
                try {
                    h5::write(fd, "ds" + std::to_string(k), sets[k],
                              h5::current_dims{sets[k].size()}, dcpl);
                } catch (...) { errors.fetch_add(1); }
            });
        }
        for (auto& t : ths) t.join();
    } // fd closes on the collector

    CHECK(errors.load() == 0);
    for (int k = 0; k < M; ++k) {
        auto back = h5::read<std::vector<double>>(path, "ds" + std::to_string(k));
        CHECK(back == sets[k]);
    }
    std::remove(path);
#else
    CHECK(true); // MT-only test, no-op in classic build
#endif
}

// ─── 4. CONCURRENT appenders — h5::append through the global lock ─────────────
//
// Each producer thread streams into its OWN packet table (h5::pt_t) in one file,
// all sharing the file's h5::threads{4} worker pool (gzip fans across the pool).
// Under -DH5CPP_MULTITHREAD every pt_t HDF5 touch — set_extent, the per-chunk
// H5Dwrite_chunk, and the flush drain — funnels through on_collector, so the
// Threadsafety-OFF HDF5 never sees two appenders at once.  Passes under TSan.
//
// Same discipline as the concurrent-writers case: the pt_t handles are built
// up-front (single-threaded) — construction opens datasets / resolves the pool —
// and producers thereafter make no direct HDF5 calls of their own.
TEST_CASE("[#287] concurrent appenders — one file, one pool") {
#ifdef H5CPP_MULTITHREAD
    const char* path = "test-287-append.h5";
    std::remove(path);
    constexpr int M    = 6;       // producer threads / datasets
    constexpr int ROWS = 5000;    // appends per dataset (~20 gzip chunks each)

    std::vector<std::vector<double>> expect(M);
    for (int k = 0; k < M; ++k) expect[k] = make_data(ROWS, k + 11);

    std::atomic<int> errors{0};
    {
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC,
            h5::default_fcpl, h5::fapl_t{h5::threads{4}});

        // Build the M packet-table handles up front, single-threaded.  pt_t
        // construction opens the dataset and resolves the file's pool, so it must
        // not race a concurrent appender — exactly as the DCPL is built once above.
        std::vector<h5::pt_t> pts;
        pts.reserve(M);
        for (int k = 0; k < M; ++k) {
            h5::ds_t ds = h5::create<double>(fd, "pt" + std::to_string(k),
                h5::current_dims_t{0}, h5::max_dims_t{H5S_UNLIMITED},
                h5::chunk{256} | h5::gzip{4});
            pts.emplace_back(ds);   // swaps to pool_pipeline_t (file has h5::threads{4})
        }

        std::vector<std::thread> ths;
        ths.reserve(M);
        for (int k = 0; k < M; ++k) {
            ths.emplace_back([&, k] {
                try {
                    for (int i = 0; i < ROWS; ++i)
                        h5::append(pts[k], expect[k][i]);
                    h5::flush(pts[k]);   // drain this table's in-flight chunks
                } catch (...) { errors.fetch_add(1); }
            });
        }
        for (auto& t : ths) t.join();
    } // pts + fd destroyed single-threaded; each pt_t dtor flush is a no-op now

    CHECK(errors.load() == 0);
    for (int k = 0; k < M; ++k) {
        auto back = h5::read<std::vector<double>>(path, "pt" + std::to_string(k));
        // pt_t rounds the trailing partial chunk up to a full chunk and pads the
        // tail with fill-value, so the dataset is ≥ ROWS; the ROWS appended values
        // must match exactly.
        CHECK(back.size() >= static_cast<std::size_t>(ROWS));
        bool ok = back.size() >= static_cast<std::size_t>(ROWS);
        for (int i = 0; ok && i < ROWS; ++i)
            if (back[i] != expect[k][i]) ok = false;
        CHECK(ok);
    }
    std::remove(path);
#else
    CHECK(true); // MT-only test, no-op in classic build
#endif
}
