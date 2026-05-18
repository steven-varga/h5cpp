// Phase II scaffold tests — h5::async::* types, executor lifecycle,
// submit_and_wait correctness, exception propagation.  Operation
// overloads (h5::write / h5::read on async fds) are tested in PR-B.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "support/fixture.hpp"

// ===========================================================================
// [#252 2.1] async descriptor types — type traits and constructibility
// ===========================================================================

TEST_CASE("[#252] h5::async types satisfy is_async_v") {
    static_assert( h5::is_async_v<h5::async::fd_t>, "async::fd_t");
    static_assert( h5::is_async_v<h5::async::ds_t>, "async::ds_t");
    static_assert( h5::is_async_v<h5::async::at_t>, "async::at_t");
    static_assert( h5::is_async_v<h5::async::gr_t>, "async::gr_t");
    static_assert( h5::is_async_v<h5::async::ob_t>, "async::ob_t");
    static_assert(!h5::is_async_v<h5::fd_t>,        "fd_t classic");
    static_assert(!h5::is_async_v<h5::ds_t>,        "ds_t classic");
    static_assert(!h5::is_async_v<h5::at_t>,        "at_t classic");
    CHECK(true);
}

TEST_CASE("[#252] async descriptors compile-block raw ::hid_t conversion") {
    // The `= delete` on operator ::hid_t() means is_constructible_v<::hid_t, T>
    // is false for every async wrapper.  Classic wrappers retain the explicit
    // conversion so they remain constructible via static_cast.
    static_assert(!std::is_constructible_v<::hid_t, h5::async::fd_t>, "fd_t");
    static_assert(!std::is_constructible_v<::hid_t, h5::async::ds_t>, "ds_t");
    static_assert(!std::is_constructible_v<::hid_t, h5::async::at_t>, "at_t");
    static_assert(!std::is_constructible_v<::hid_t, h5::async::gr_t>, "gr_t");
    static_assert(!std::is_constructible_v<::hid_t, h5::async::ob_t>, "ob_t");
    // Control assertions on the classic surface — these MUST stay constructible.
    static_assert( std::is_constructible_v<::hid_t, h5::fd_t>, "classic fd_t");
    static_assert( std::is_constructible_v<::hid_t, h5::ds_t>, "classic ds_t");
    CHECK(true);
}

TEST_CASE("[#252] h5::async::* default-construct to H5I_UNINIT") {
    h5::async::fd_t fd;
    h5::async::ds_t ds;
    h5::async::gr_t gr;
    h5::async::at_t at;
    CHECK(fd.handle == H5I_UNINIT);
    CHECK(ds.handle == H5I_UNINIT);
    CHECK(gr.handle == H5I_UNINIT);
    CHECK(at.handle == H5I_UNINIT);
}

// ===========================================================================
// [#252 2.2] FAPL executor slot — install + resolve + H5Pcopy preserves slot
// ===========================================================================

TEST_CASE("[#252 2.2] fapl_async_set installs an executor on the FAPL") {
    h5::fapl_t fapl{H5Pcreate(H5P_FILE_ACCESS)};
    REQUIRE(h5::impl::fapl_async_set(static_cast<::hid_t>(fapl)) == 0);
    auto exec = h5::impl::resolve_executor(static_cast<::hid_t>(fapl));
    REQUIRE(exec);
    CHECK(exec->pool());                   // default pool auto-installed
    CHECK(exec->in_flight() == 0);
}

TEST_CASE("[#252 2.2] H5Pcopy preserves shared executor ownership") {
    h5::fapl_t src{H5Pcreate(H5P_FILE_ACCESS)};
    h5::impl::fapl_async_set(static_cast<::hid_t>(src));
    auto exec_src = h5::impl::resolve_executor(static_cast<::hid_t>(src));
    REQUIRE(exec_src);

    h5::fapl_t dst{H5Pcopy(static_cast<::hid_t>(src))};
    auto exec_dst = h5::impl::resolve_executor(static_cast<::hid_t>(dst));
    REQUIRE(exec_dst);

    // Same executor instance through both FAPLs — slot copy_cb aliases.
    CHECK(exec_src.get() == exec_dst.get());
}

TEST_CASE("[#252 2.2] fapl_async_set is idempotent") {
    h5::fapl_t fapl{H5Pcreate(H5P_FILE_ACCESS)};
    REQUIRE(h5::impl::fapl_async_set(static_cast<::hid_t>(fapl)) == 0);
    auto exec1 = h5::impl::resolve_executor(static_cast<::hid_t>(fapl));
    REQUIRE(h5::impl::fapl_async_set(static_cast<::hid_t>(fapl)) == 0);
    auto exec2 = h5::impl::resolve_executor(static_cast<::hid_t>(fapl));
    CHECK(exec1.get() == exec2.get());
}

TEST_CASE("[#252 2.2] async fapl chains h5::threads{N} when explicit") {
    h5::fapl_t fapl = h5::threads{6};
    h5::impl::fapl_async_set(static_cast<::hid_t>(fapl));
    auto exec = h5::impl::resolve_executor(static_cast<::hid_t>(fapl));
    REQUIRE(exec);
    REQUIRE(exec->pool());
    CHECK(exec->pool()->worker_count() == 6);
}

// ===========================================================================
// [#252 2.3] executor_t — submit_and_wait + exception propagation + reentry
// ===========================================================================

TEST_CASE("[#252 2.3] executor_t::submit_and_wait runs callable and returns result") {
    h5::impl::executor_t exec;
    int result = exec.submit_and_wait([]{ return 42; });
    CHECK(result == 42);
    CHECK(exec.in_flight() == 0);
}

TEST_CASE("[#252 2.3] executor_t::submit_and_wait runs on the executor thread") {
    h5::impl::executor_t exec;
    std::thread::id caller = std::this_thread::get_id();
    std::thread::id callee = exec.submit_and_wait([]{ return std::this_thread::get_id(); });
    CHECK(caller != callee);
    CHECK(callee == exec.worker_thread_id());
}

TEST_CASE("[#252 2.3] exception inside submit_and_wait propagates to caller") {
    h5::impl::executor_t exec;
    CHECK_THROWS_AS(
        exec.submit_and_wait([]{ throw std::runtime_error("boom"); }),
        std::runtime_error);
    CHECK(exec.in_flight() == 0);
}

TEST_CASE("[#252 2.3] executor_t::submit_and_wait re-entry runs inline (no deadlock)") {
    h5::impl::executor_t exec;
    int result = exec.submit_and_wait([&]{
        // Nested call from inside the executor thread — must not enqueue
        // (would deadlock) — runs inline by the same-thread check.
        return exec.submit_and_wait([]{ return 7; }) * 6;
    });
    CHECK(result == 42);
}

TEST_CASE("[#252 2.3] executor_t void-returning callable") {
    h5::impl::executor_t exec;
    std::atomic<int> counter{0};
    exec.submit_and_wait([&]{ counter.fetch_add(1); });
    CHECK(counter.load() == 1);
}

// ===========================================================================
// [#252 2.4] Multi-thread submission stress — TSAN sees nothing
// ===========================================================================

TEST_CASE("[#252 2.4] executor_t serializes 8 concurrent submitters correctly") {
    h5::impl::executor_t exec;
    constexpr int per_thread = 32;
    constexpr int n_threads  = 8;
    std::atomic<int> sum{0};

    std::vector<std::thread> ths;
    ths.reserve(n_threads);
    for (int t = 0; t < n_threads; ++t) {
        ths.emplace_back([&, t]{
            for (int i = 0; i < per_thread; ++i) {
                int got = exec.submit_and_wait([&, t, i]{
                    return t * 100 + i;
                });
                sum.fetch_add(got, std::memory_order_relaxed);
            }
        });
    }
    for (auto& th : ths) th.join();

    int expected = 0;
    for (int t = 0; t < n_threads; ++t)
        for (int i = 0; i < per_thread; ++i)
            expected += t * 100 + i;
    CHECK(sum.load() == expected);
    CHECK(exec.in_flight() == 0);
}

// ===========================================================================
// [#252 2.5] h5::async::create / open — file round-trip lifecycle
// ===========================================================================
//
// The executor is reached via `fd.exec` directly (a shared_ptr field on the
// async wrapper) rather than via H5Fget_access_plist on the file id.
// Phase I's choice to use the FAPL slot pattern + H5Fget_access_plist runs
// into HDF5 1.10.9's behavior: H5Fget_access_plist returns a synthetic
// FAPL reconstructed from standard properties only, so user-inserted
// properties (H5Pinsert2) silently disappear.  See the regression test
// below for the documented HDF5 behavior.

TEST_CASE("[#252 2.5] h5::async::create + close round-trip") {
    const char* path = "test-252-async-create.h5";
    std::remove(path);
    {
        h5::async::fd_t fd = h5::async::create(path, H5F_ACC_TRUNC);
        REQUIRE(H5Iis_valid(fd.handle));
        REQUIRE(fd.exec);
        CHECK(fd.exec->pool());
    }
    // fd dtor closed the file — verify by re-opening with the classic API.
    {
        h5::fd_t fd = h5::open(path, H5F_ACC_RDONLY);
        CHECK(H5Iis_valid(static_cast<::hid_t>(fd)));
    }
    std::remove(path);
}

TEST_CASE("[#252 2.5] h5::async::open round-trip on existing file") {
    const char* path = "test-252-async-open.h5";
    std::remove(path);
    {
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC);  // classic create
    }
    {
        h5::async::fd_t fd = h5::async::open(path, H5F_ACC_RDWR);
        REQUIRE(H5Iis_valid(fd.handle));
        REQUIRE(fd.exec);
    }
    std::remove(path);
}

TEST_CASE("[#252 2.5] async file with h5::threads{N} keeps explicit pool size") {
    const char* path = "test-252-async-threads.h5";
    std::remove(path);
    {
        h5::async::fd_t fd = h5::async::create(path, H5F_ACC_TRUNC,
                                               h5::default_fcpl,
                                               h5::fapl_t{h5::threads{4}});
        REQUIRE(fd.exec);
        REQUIRE(fd.exec->pool());
        CHECK(fd.exec->pool()->worker_count() == 4);
    }
    std::remove(path);
}

TEST_CASE("[#252 2.5] copy of h5::async::fd_t shares the executor") {
    const char* path = "test-252-async-share.h5";
    std::remove(path);
    {
        h5::async::fd_t fd_a = h5::async::create(path, H5F_ACC_TRUNC);
        h5::async::fd_t fd_b = fd_a;          // shared_ptr aliases
        REQUIRE(fd_a.exec);
        REQUIRE(fd_b.exec);
        CHECK(fd_a.exec.get() == fd_b.exec.get());
    }
    std::remove(path);
}

// ===========================================================================
// [#252 2.5] HDF5 1.10.9 regression doc — user FAPL properties don't survive
// H5Fget_access_plist.  This documents *why* fd.exec is stored on the
// wrapper rather than retrieved from the file's FAPL.
// ===========================================================================

TEST_CASE("[#252] HDF5 strips user-inserted FAPL properties on H5Fget_access_plist") {
    // Build a FAPL with a user-inserted property (Phase I's worker pool
    // property uses the same mechanism via H5Pinsert2).
    h5::fapl_t fapl_in = h5::threads{4};
    REQUIRE(H5Pexist(static_cast<::hid_t>(fapl_in), "h5cpp_fapl_worker_pool") > 0);

    const char* path = "test-252-fapl-strip.h5";
    std::remove(path);
    {
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl_in);
        ::hid_t fapl_out = H5Fget_access_plist(static_cast<::hid_t>(fd));
        REQUIRE(fapl_out >= 0);

        // Documents the HDF5 behavior we work around in h5::async by
        // storing the executor on the wrapper directly:
        CHECK(H5Pexist(fapl_out, "h5cpp_fapl_worker_pool") == 0);
        H5Pclose(fapl_out);
    }
    std::remove(path);
}
