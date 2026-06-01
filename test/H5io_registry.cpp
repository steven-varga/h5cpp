/*
 * Copyright (c) 2026 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Tests for the per-file worker-pool registry (h5::impl::io_registry_t).
 * Issue #286 — slice D.
 *
 * Four test groups:
 *   1. multi-open → one pool   (#286-reg-1)
 *   2. call_once first-open race   (#286-reg-2)
 *   3. leaked-fd / teardown ordering   (#286-reg-3)
 *   4. regression: pool actually ENGAGES at write/read time   (#286-reg-4)
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>
#include <h5cpp/H5io_registry.hpp>

#include <atomic>
#include <cstdio>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// [#286-reg-1] multi-open → one pool
//
// Create a chunked+gzip file with h5::threads{N}.  Open the SAME file a
// second time with h5::threads{N}.  Both handles share the same physical
// inode, so H5Fget_fileno returns an identical key, and the registry must
// resolve to the SAME pool shared_ptr (pointer equality).  The ref count
// inside file_io_t must be exactly 2 (one per h5::create / h5::open call).
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("[#286-reg-1] two opens of same file share one pool entry") {
    const char* path = "test-286-multi-open.h5";
    std::remove(path);

    // Ensure the singleton is alive (lazy init via registry()).
    using reg_singleton = h5::impl::singleton_t<h5::impl::io_registry_t>;
    h5::impl::registry();  // force lazy init via std::call_once
    REQUIRE(reg_singleton::is_initialized());

    h5::fapl_t fapl = h5::threads{4};
    h5::fd_t fd1 = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);

    const unsigned long fileno1 = h5::impl::file_key_of_file(
            static_cast<::hid_t>(fd1));

    // First open must have registered a pool.
    auto pool1 = h5::impl::registry().resolve_pool(fileno1);
    REQUIRE(pool1 != nullptr);

    // Write a chunked+gzip dataset to make the file non-trivial.
    std::vector<double> data(1024);
    std::iota(data.begin(), data.end(), 0.0);
    h5::write(fd1, "ds", data,
              h5::current_dims{data.size()}, h5::max_dims{H5S_UNLIMITED},
              h5::chunk{128} | h5::gzip{4}, h5::high_throughput);

    // Second open of the same path — must share the same fileno and pool.
    h5::fd_t fd2 = h5::open(path, H5F_ACC_RDONLY, fapl);
    const unsigned long fileno2 = h5::impl::file_key_of_file(
            static_cast<::hid_t>(fd2));

    CHECK(fileno1 == fileno2);

    auto pool2 = h5::impl::registry().resolve_pool(fileno2);
    REQUIRE(pool2 != nullptr);

    // Same underlying pool object — shared_ptr aliasing.
    CHECK(pool1.get() == pool2.get());

    // Close fd2 — ref count drops to 1, pool must still be reachable.
    {
        h5::fd_t fd2_local = std::move(fd2);
    } // fd2_local destroyed → registry().detach() decrements refs to 1

    auto pool_after_second_close = h5::impl::registry().resolve_pool(fileno1);
    CHECK(pool_after_second_close != nullptr);

    // Close fd1 — refs drop to 0, entry erased.
    {
        h5::fd_t fd1_local = std::move(fd1);
    } // fd1_local destroyed → registry().detach() erases entry

    auto pool_after_all_closed = h5::impl::registry().resolve_pool(fileno1);
    CHECK(pool_after_all_closed == nullptr);

    std::remove(path);
}

// ─────────────────────────────────────────────────────────────────────────────
// [#286-reg-2] call_once first-open race
//
// The goal is to exercise the std::call_once path inside h5::impl::registry().
// HDF5 (threadsafety OFF) is not re-entrant, so H5Fcreate calls are serialized
// via a mutex.  Each thread then calls h5::impl::registry().resolve_pool(...)
// concurrently, which is safe because io_registry_t::mu_ guards the map.
//
// After joining, assert:
//   - The singleton initialized exactly once (is_initialized() == true).
//   - No exception was thrown (visible via the exceptions counter).
//   - All created files have a live pool entry in the registry.
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("[#286-reg-2] call_once singleton init + concurrent registry reads") {
    // HDF5 is built Threadsafety OFF: NO HDF5 C-API call is safe from multiple
    // threads — not even H5Pcreate (the FAPL ctor) or H5Pclose (its dtor). So all
    // file/property work stays on the main thread; the worker threads exercise ONLY
    // the registry — std::call_once lazy init + the mutex-guarded map — which is
    // exactly the thread-safety #286 must provide (concurrent dispatch-site reads).
    constexpr int N = 6;
    std::vector<std::string> paths(N);
    for (int i = 0; i < N; ++i)
        paths[i] = "test-286-race-" + std::to_string(i) + ".h5";
    for (auto& p : paths) std::remove(p.c_str());

    std::vector<unsigned long> filenos(N, 0UL);

    {
        // Main thread only: create N threaded files, record their filenos.
        std::vector<h5::fd_t> fds;
        fds.reserve(N);
        for (int i = 0; i < N; ++i) {
            h5::fapl_t fapl = h5::threads{2};
            fds.emplace_back(h5::create(paths[i], H5F_ACC_TRUNC, h5::default_fcpl, fapl));
            filenos[i] = h5::impl::file_key_of_file(static_cast<::hid_t>(fds.back()));
            CHECK(filenos[i] != 0UL);
        }

        // Concurrent registry hammering — NO HDF5 calls in the threads.
        constexpr int T = 16, ITERS = 2000;
        std::atomic<int> exceptions{0};
        std::atomic<int> misses{0};
        std::vector<std::thread> ths;
        ths.reserve(T);
        for (int t = 0; t < T; ++t) {
            ths.emplace_back([&] {
                try {
                    for (int k = 0; k < ITERS; ++k)
                        for (int i = 0; i < N; ++i)
                            if (!h5::impl::registry().resolve_pool(filenos[i]))
                                misses.fetch_add(1);  // every read must find the live pool
                } catch (...) { exceptions.fetch_add(1); }
            });
        }
        for (auto& t : ths) t.join();

        CHECK(h5::impl::singleton_t<h5::impl::io_registry_t>::is_initialized());
        CHECK(exceptions.load() == 0);
        CHECK(misses.load() == 0);
    } // fds destroyed on the main thread → all detached

    for (int i = 0; i < N; ++i)
        CHECK(h5::impl::registry().resolve_pool(filenos[i]) == nullptr);

    for (auto& p : paths) std::remove(p.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// [#286-reg-3] leaked-fd / teardown ordering
//
// Verify RAII teardown:
//   (a) Open a threaded file, write, close via RAII (scope exit).
//       After close, resolve_pool(fileno) must return nullptr.
//   (b) Move-assign an fd_t (old handle is closed in the move target dtor),
//       same postcondition.
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("[#286-reg-3a] RAII close removes registry entry") {
    const char* path = "test-286-raii-close.h5";
    std::remove(path);

    unsigned long fileno = 0;
    {
        h5::fapl_t fapl = h5::threads{3};
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);
        fileno = h5::impl::file_key_of_file(static_cast<::hid_t>(fd));

        std::vector<int> data(200);
        std::iota(data.begin(), data.end(), 0);
        h5::write(fd, "ds", data,
                  h5::current_dims{data.size()}, h5::max_dims{H5S_UNLIMITED},
                  h5::chunk{64} | h5::gzip{3}, h5::high_throughput);

        // Pool must be reachable while fd is live.
        CHECK(h5::impl::registry().resolve_pool(fileno) != nullptr);
    } // fd destroyed here — registry must be cleaned up before we reach here

    CHECK(h5::impl::registry().resolve_pool(fileno) == nullptr);
    std::remove(path);
}

TEST_CASE("[#286-reg-3b] move-assign fd_t closes old handle and removes registry entry") {
    const char* path_a = "test-286-move-a.h5";
    const char* path_b = "test-286-move-b.h5";
    std::remove(path_a);
    std::remove(path_b);

    unsigned long fileno_a = 0;
    {
        h5::fapl_t fapl = h5::threads{2};
        h5::fd_t fd_a = h5::create(path_a, H5F_ACC_TRUNC, h5::default_fcpl, fapl);
        h5::fd_t fd_b = h5::create(path_b, H5F_ACC_TRUNC, h5::default_fcpl, fapl);

        fileno_a = h5::impl::file_key_of_file(static_cast<::hid_t>(fd_a));
        unsigned long fileno_b = h5::impl::file_key_of_file(
                static_cast<::hid_t>(fd_b));

        CHECK(h5::impl::registry().resolve_pool(fileno_a) != nullptr);
        CHECK(h5::impl::registry().resolve_pool(fileno_b) != nullptr);

        // Move-assign: fd_b's dtor closes old fd_b handle → detach(fileno_b).
        // fd_b now owns fd_a's handle.
        fd_b = std::move(fd_a);

        // fileno_b entry must be gone after the move-assign closes old fd_b.
        CHECK(h5::impl::registry().resolve_pool(fileno_b) == nullptr);

        // fileno_a entry must still be alive (now owned by fd_b).
        CHECK(h5::impl::registry().resolve_pool(fileno_a) != nullptr);
    } // fd_b destroyed → detach(fileno_a)

    CHECK(h5::impl::registry().resolve_pool(fileno_a) == nullptr);

    std::remove(path_a);
    std::remove(path_b);
}

// ─────────────────────────────────────────────────────────────────────────────
// [#286-reg-4] regression: pool actually ENGAGES at write time
//
// This is the core #286 assertion.  Before the fix, H5Dwrite.hpp looked up
// the pool via H5Fget_access_plist, which silently strips user-inserted FAPL
// properties (documented by the H5async.cpp regression test).  The pool was
// always nullptr → pool_pipeline_t never constructed → ~94% coverage cap.
//
// After the fix (slice C): H5Dwrite.hpp calls
//   h5::impl::registry().resolve_pool(fileno)
// which is populated at file-open time while the original FAPL is still live.
//
// The probe: before calling h5::write, assert the registry entry exists.
// After h5::write, read back and assert data integrity.  If the pool branch
// runs, pool_pipeline_t is constructed and filter compression goes parallel.
//
// This test WOULD FAIL on the pre-fix code (resolve_pool would return nullptr
// at the write site, because resolve_worker_pool(H5Fget_access_plist(fd))
// always returned nullptr — but the registry was not populated either, so
// the pool branch was simply never taken).
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("[#286-reg-4] pool ENGAGES: write+read round-trip with registry-resolved pool") {
    const char* path = "test-286-pool-engage.h5";
    std::remove(path);

    h5::fapl_t fapl = h5::threads{4};
    h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);

    const unsigned long fileno = h5::impl::file_key_of_file(
            static_cast<::hid_t>(fd));

    // Pre-write assertion: registry must have a pool for this file.
    auto pre_write_pool = h5::impl::registry().resolve_pool(fileno);
    REQUIRE_MESSAGE(pre_write_pool != nullptr,
        "registry has no pool for this file — slice A (H5io_registry.hpp) "
        "or slice B (H5Fcreate.hpp/H5Fopen.hpp attach) is broken");

    std::vector<double> data(10'000);
    std::iota(data.begin(), data.end(), 0.0);

    // Write with high_throughput: triggers the use_pipeline branch in
    // H5Dwrite.hpp, which now resolves the pool from the registry.
    // If the pool is non-null there, pool_pipeline_t is constructed and
    // compression is dispatched to worker threads.
    h5::write(fd, "ds", data,
              h5::current_dims{data.size()}, h5::max_dims{H5S_UNLIMITED},
              h5::chunk{512} | h5::gzip{4}, h5::high_throughput);

    // Pool still reachable after write (fd still open).
    auto post_write_pool = h5::impl::registry().resolve_pool(fileno);
    CHECK(post_write_pool != nullptr);
    CHECK(post_write_pool.get() == pre_write_pool.get());

    // Read back through the pool pipeline as well.
    h5::ds_t ds = h5::open(fd, "ds", h5::high_throughput);
    auto back = h5::read<std::vector<double>>(ds);

    REQUIRE(back.size() == data.size());
    bool data_ok = true;
    for (size_t i = 0; i < data.size(); ++i) {
        if (back[i] != doctest::Approx(data[i])) { data_ok = false; break; }
    }
    CHECK(data_ok);

    std::remove(path);
}

// Additional probe: verify that a file opened WITHOUT h5::threads{N} has
// no pool in the registry (the no-pool synchronous fallback path).
TEST_CASE("[#286-reg-4b] no-pool file: registry entry absent, synchronous fallback works") {
    const char* path = "test-286-no-pool.h5";
    std::remove(path);

    // Classic create — no h5::threads, no pool installed.
    h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC);
    const unsigned long fileno = h5::impl::file_key_of_file(
            static_cast<::hid_t>(fd));

    // Registry should have no entry for this file.
    CHECK(h5::impl::registry().resolve_pool(fileno) == nullptr);

    // h5::write still works via the synchronous basic_pipeline fallback.
    std::vector<double> data(1000);
    std::iota(data.begin(), data.end(), 0.0);
    h5::write(fd, "ds", data,
              h5::current_dims{data.size()}, h5::max_dims{H5S_UNLIMITED},
              h5::chunk{256} | h5::gzip{4}, h5::high_throughput);

    h5::ds_t ds = h5::open(fd, "ds", h5::high_throughput);
    auto back = h5::read<std::vector<double>>(ds);
    REQUIRE(back.size() == data.size());
    for (size_t i = 0; i < data.size(); ++i)
        CHECK(back[i] == doctest::Approx(data[i]));

    std::remove(path);
}
