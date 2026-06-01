// #287 — multithread build-mode tests.
//
// "Async / concurrency-safe writes" is no longer a separate type set; it is the
// compile-time -DH5CPP_MULTITHREAD build mode.  Under that macro h5::fd_t is a
// conversion-off handle (no implicit ::hid_t decay — explicit static_cast only)
// and every h5::write + handle close routes through the process-global
// collector.  Without the macro h5::fd_t is the classic, implicitly-convertible
// handle.  This file pins both the handle-conversion invariant (macro-gated) and
// the create/open lifecycle (runs in every build mode), plus the still-present
// impl-level executor/FAPL scaffolding.

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
// handle conversion invariant — depends on the build mode
// ===========================================================================

TEST_CASE("[#287] h5::fd_t ::hid_t implicit-decay policy follows the build mode") {
    // -DH5CPP_MULTITHREAD force-defines the conversion-off macros (see
    // H5config.hpp), so the conversion-off boundary — and this invariant — also
    // holds when either conversion macro is set explicitly (as build-conv-off
    // does without H5CPP_MULTITHREAD).  Key on the actual conversion gate, not on
    // H5CPP_MULTITHREAD.
#if defined(H5CPP_CONVERSION_TO_CAPI_DISABLED) || defined(H5CPP_CONVERSION_FROM_CAPI_DISABLED)
    // Conversion-off boundary makes operator ::hid_t() *explicit*: it blocks the
    // SILENT/implicit decay (H5Dwrite(fd) won't compile) that would let a thread
    // bypass the collector, while a DELIBERATE static_cast<hid_t>(fd) — the
    // visible, on-collector escape h5cpp internals use — still works.  So the
    // invariant is "not implicitly convertible", not "not constructible at all".
    static_assert(!std::is_convertible_v<h5::fd_t, ::hid_t>, "fd_t");
    static_assert(!std::is_convertible_v<h5::ds_t, ::hid_t>, "ds_t");
    static_assert(!std::is_convertible_v<h5::at_t, ::hid_t>, "at_t");
    static_assert(!std::is_convertible_v<h5::gr_t, ::hid_t>, "gr_t");
#else
    // Classic single-threaded build: handles decay implicitly, exactly as before.
    static_assert(std::is_convertible_v<h5::fd_t, ::hid_t>, "fd_t classic");
    static_assert(std::is_convertible_v<h5::ds_t, ::hid_t>, "ds_t classic");
    static_assert(std::is_convertible_v<h5::at_t, ::hid_t>, "at_t classic");
    static_assert(std::is_convertible_v<h5::gr_t, ::hid_t>, "gr_t classic");
#endif
    // Explicit construction MUST work in BOTH build modes (the static_cast idiom).
    static_assert(std::is_constructible_v<::hid_t, h5::fd_t>, "fd_t explicit");
    static_assert(std::is_constructible_v<::hid_t, h5::ds_t>, "ds_t explicit");
    CHECK(true);
}

TEST_CASE("[#287] h5::* handles default-construct to H5I_UNINIT") {
    h5::fd_t fd;
    h5::ds_t ds;
    h5::gr_t gr;
    h5::at_t at;
    CHECK(static_cast<::hid_t>(fd) == H5I_UNINIT);
    CHECK(static_cast<::hid_t>(ds) == H5I_UNINIT);
    CHECK(static_cast<::hid_t>(gr) == H5I_UNINIT);
    CHECK(static_cast<::hid_t>(at) == H5I_UNINIT);
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
// [#287] h5::create / open — file round-trip lifecycle (every build mode)
// ===========================================================================

TEST_CASE("[#287] h5::create + close round-trip") {
    const char* path = "test-287-create.h5";
    std::remove(path);
    {
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC);
        REQUIRE(H5Iis_valid(static_cast<::hid_t>(fd)));
    }
    // fd dtor closed the file — verify by re-opening.
    {
        h5::fd_t fd = h5::open(path, H5F_ACC_RDONLY);
        CHECK(H5Iis_valid(static_cast<::hid_t>(fd)));
    }
    std::remove(path);
}

TEST_CASE("[#287] h5::open round-trip on existing file") {
    const char* path = "test-287-open.h5";
    std::remove(path);
    {
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC);
        (void)fd;
    }
    {
        h5::fd_t fd = h5::open(path, H5F_ACC_RDWR);
        REQUIRE(H5Iis_valid(static_cast<::hid_t>(fd)));
    }
    std::remove(path);
}

TEST_CASE("[#287] file with h5::threads{N} FAPL round-trips") {
    const char* path = "test-287-threads.h5";
    std::remove(path);
    {
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC,
                                 h5::default_fcpl,
                                 h5::fapl_t{h5::threads{4}});
        CHECK(H5Iis_valid(static_cast<::hid_t>(fd)));
    }
    std::remove(path);
}

// ===========================================================================
// [#252 2.5] HDF5 1.10.9 regression doc — user FAPL properties don't survive
// H5Fget_access_plist.  This documents *why* the worker pool is resolved from
// the fileno registry rather than retrieved from the file's FAPL.
// ===========================================================================

TEST_CASE("[#252] HDF5 strips user-inserted FAPL properties on H5Fget_access_plist") {
    // Build a FAPL with a user-inserted property (the worker pool property uses
    // the same H5Pinsert2 mechanism).
    h5::fapl_t fapl_in = h5::threads{4};
    REQUIRE(H5Pexist(static_cast<::hid_t>(fapl_in), "h5cpp_fapl_worker_pool") > 0);

    const char* path = "test-252-fapl-strip.h5";
    std::remove(path);
    {
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl_in);
        ::hid_t fapl_out = H5Fget_access_plist(static_cast<::hid_t>(fd));
        REQUIRE(fapl_out >= 0);

        // Documents the HDF5 behavior worked around by resolving the pool from
        // the fileno registry instead of the file's FAPL:
        CHECK(H5Pexist(fapl_out, "h5cpp_fapl_worker_pool") == 0);
        H5Pclose(fapl_out);
    }
    std::remove(path);
}
