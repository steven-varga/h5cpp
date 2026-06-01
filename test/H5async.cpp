// #287 — multithread build-mode tests.
//
// "Async / concurrency-safe writes" is no longer a separate type set; it is the
// compile-time -DH5CPP_MULTITHREAD build mode.  Under that macro h5::fd_t is a
// conversion-off handle (no implicit ::hid_t decay — explicit static_cast only)
// and every h5::write + handle close routes through the process-global lock.
// Without the macro h5::fd_t is the classic, implicitly-convertible handle.
// This file pins both the handle-conversion invariant (macro-gated) and the
// create/open lifecycle (runs in every build mode).

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
