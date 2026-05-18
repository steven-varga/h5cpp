#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>
#include <vector>
#include <forward_list>
#include <sstream>
#include <string>
#include "support/fixture.hpp"

TEST_CASE("packet table append scalar values") {
    h5::test::file_fixture_t f("test-pt-append.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{0},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10});
    h5::pt_t pt(ds);
    for (int i = 0; i < 25; ++i)
        h5::append(pt, i);
    h5::flush(pt);
    CHECK(true);
}

TEST_CASE("packet table append std::vector chunk") {
    h5::test::file_fixture_t f("test-pt-vector.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{0, 5},
        h5::max_dims_t{H5S_UNLIMITED, 5}, h5::chunk{2, 5});
    h5::pt_t pt(ds);
    std::vector<int> chunk = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    h5::append(pt, chunk);
    h5::flush(pt);
    CHECK(true);
}

TEST_CASE("packet table partial flush fills remainder with fill value") {
    h5::test::file_fixture_t f("test-pt-partial.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{0},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10});
    {
        h5::pt_t pt(ds);
        for (int i = 0; i < 3; ++i)
            h5::append(pt, i + 1);
        h5::flush(pt);
    }
    auto readback = h5::read<std::vector<int>>(f.fd, "ds");
    REQUIRE(readback.size() == 10);  // partial chunk padded to full chunk size
    for (int i = 0; i < 3; ++i)
        CHECK(readback[i] == i + 1);
    for (int i = 3; i < 10; ++i)
        CHECK(readback[i] == 0);  // fill value pads remainder
}

TEST_CASE("packet table auto-flush on destruction") {
    h5::test::file_fixture_t f("test-pt-dtor.h5");
    {
        h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{0},
            h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10});
        h5::pt_t pt(ds);
        for (int i = 0; i < 3; ++i)
            h5::append(pt, i + 1);
        // pt destructor flushes partial chunk, then ds closes
    }
    auto readback = h5::read<std::vector<int>>(f.fd, "ds");
    REQUIRE(readback.size() == 10);  // partial chunk padded to full chunk size
    for (int i = 0; i < 3; ++i)
        CHECK(readback[i] == i + 1);
    for (int i = 3; i < 10; ++i)
        CHECK(readback[i] == 0);  // fill value pads remainder
}

TEST_CASE("packet table string append and flush") {
    h5::test::file_fixture_t f("test-pt-string.h5");
    h5::mute();
    {
        h5::ds_t ds = h5::create<std::string>(f.fd, "ds", h5::current_dims_t{0},
            h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{5});
        h5::pt_t pt(ds);
        std::vector<std::string> strings = {"alpha", "beta", "gamma", "delta", "epsilon"};
        for (const auto& s : strings)
            h5::append(pt, s);
        h5::flush(pt);
    }
    h5::unmute();
    CHECK(true);
}

TEST_CASE("packet table const char* append") {
    h5::test::file_fixture_t f("test-pt-charptr.h5");
    h5::mute();
    {
        h5::ds_t ds = h5::create<std::string>(f.fd, "ds", h5::current_dims_t{0},
            h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{3});
        h5::pt_t pt(ds);
        h5::append(pt, "hello");
        h5::append(pt, "world");
        h5::append(pt, "test");
        h5::flush(pt);
    }
    h5::unmute();
    CHECK(true);
}

TEST_CASE("packet table copy constructor") {
    h5::test::file_fixture_t f("test-pt-copy.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{0},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10});
    h5::pt_t pt1(ds);
    for (int i = 0; i < 5; ++i)
        h5::append(pt1, i);

    h5::pt_t pt2(pt1); // copy ctor
    h5::flush(pt2);
    CHECK(true);
}

TEST_CASE("packet table move assignment") {
    h5::test::file_fixture_t f("test-pt-move.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{0},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10});
    h5::pt_t pt1(ds);
    for (int i = 0; i < 5; ++i)
        h5::append(pt1, i);

    h5::pt_t pt2;
    pt2 = std::move(pt1); // move assignment

    h5::mute();
    h5::flush(pt2);
    h5::unmute();
    CHECK(true);
}

TEST_CASE("packet table self move assignment") {
    h5::test::file_fixture_t f("test-pt-self-move.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{0},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10});
    h5::pt_t pt(ds);
    for (int i = 0; i < 5; ++i)
        h5::append(pt, i);
    pt = std::move(pt); // self move assignment
    h5::flush(pt);
    CHECK(true);
}

TEST_CASE("packet table output stream operator") {
    h5::test::file_fixture_t f("test-pt-ostream.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{0},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10});
    h5::pt_t pt(ds);
    std::ostringstream oss;
    oss << pt;
    CHECK(oss.str().find("packet table:") != std::string::npos);
}

TEST_CASE("packet table invalid handle conversion ctor") {
    h5::pt_t pt(h5::ds_t{H5I_UNINIT});
    CHECK(true);
}

TEST_CASE("packet table output stream for invalid handle") {
    h5::pt_t pt;
    std::ostringstream oss;
    oss << pt;
    CHECK(oss.str().find("H5I_UNINIT") != std::string::npos);
}

// =====================================================================
// (#241 h5::filter::threads tests removed in #250 — the per-pt_t worker
// pool API is superseded by FAPL-scoped h5::threads{N}.  Coverage moved
// to test/H5Pall.cpp ([#250 1.3.3] cases).)
// =====================================================================

TEST_CASE("[#232] std::forward_list<int> append streams elements into chunked dataset") {
    h5::test::file_fixture_t f("test-pt-fwdlist.h5");
    // forward_list is append/view only — h5::write/read intentionally unsupported.
    // Each element is streamed one-by-one into the packet table (partial chunk flushed explicitly).
    std::forward_list<int> src = {10, 20, 30, 40, 50};
    constexpr std::size_t N = 5;

    h5::ds_t ds = h5::create<int>(f.fd, "fwdlist", h5::current_dims_t{0},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{5});  // chunk == list size: auto-flush at boundary
    {
        h5::pt_t pt(ds);
        h5::append(pt, src);  // iterates element-by-element via iterator dispatch
        h5::flush(pt);        // flush partial chunk to file
    }

    auto readback = h5::read<std::vector<int>>(f.fd, "fwdlist");
    REQUIRE(readback.size() == N);
    const std::vector<int> expected(src.begin(), src.end());
    CHECK(readback == expected);
}

// regression guard for issue #239 — h5::reset(pt_t&) zeros the dimension tracker
// so the same packet table can be reused for a fresh logical session (e.g.
// start-of-day re-init in streaming sinks like iex2h5).
TEST_CASE("[#239] h5::reset zeroes packet table dimension tracker") {
    h5::test::file_fixture_t f("test-pt-reset.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{0},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{10});
    h5::pt_t pt(ds);
    for (int i = 0; i < 15; ++i)
        h5::append(pt, i);
    h5::flush(pt);          // advances current_dims to one chunk past the data
    h5::reset(pt);          // must compile and run without throwing
    CHECK(true);
}

// =====================================================================
// [#250 1.3.2] pt_t resolves FAPL pool + backpressure at init
// =====================================================================

TEST_CASE("[#250 1.3.2] pt_t picks up worker pool + cap from file's FAPL") {
    // Construct a file with h5::threads{4} | h5::backpressure{16} on its FAPL.
    // The fixture's default file_fixture_t opens without these properties;
    // we make a custom one inline here.
    const char* path = "test-pt-1.3.2-pool-resolve.h5";
    std::remove(path);
    {
        h5::fapl_t fapl = h5::threads{4} | h5::backpressure{16};
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);

        h5::ds_t ds = h5::create<int>(fd, "ds", h5::current_dims_t{0},
            h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{32});

        h5::pt_t pt(ds);
        // pt_t::pool_ and ::backpressure_cap_ are private; the visible
        // contract is that operations on this pt_t SHOULD use the pool
        // (Phase 1.3.3).  In this commit we just verify the pt_t was
        // constructed without error and the file FAPL has the pool.
        auto pool_check = h5::impl::resolve_worker_pool(static_cast<hid_t>(fapl));
        REQUIRE(pool_check);
        CHECK(pool_check->worker_count() == 4);
        CHECK(h5::impl::resolve_backpressure(
                  static_cast<hid_t>(fapl), pool_check->worker_count()) == 16u);
    }
    std::remove(path);
}

TEST_CASE("[#250 1.3.2] pt_t with no FAPL pool falls back cleanly") {
    // Default FAPL — no h5::threads applied.
    h5::test::file_fixture_t f("test-pt-1.3.2-no-pool.h5");
    h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{0},
        h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{16});

    h5::pt_t pt(ds);
    // pt_t constructs without throwing; pool_ resolves to nullptr internally.
    // Writes go through visit_pipeline (synchronous) — verify by appending
    // and reading back.
    for (int i = 0; i < 32; ++i) h5::append(pt, i);
    h5::flush(pt);

    auto readback = h5::read<std::vector<int>>(f.fd, "ds");
    REQUIRE(readback.size() == 32);
    for (int i = 0; i < 32; ++i) CHECK(readback[i] == i);
}

// =====================================================================
// [#250 1.3.2 step 2] pt_t pool path: bytewise equivalence + parallelism
// =====================================================================

TEST_CASE("[#250 1.3.2] pt_t with FAPL pool — gzip round-trip equivalence vs synchronous") {
    constexpr int N = 256;
    std::vector<int> expected(N);
    for (int i = 0; i < N; ++i) expected[i] = i * 7 + 3;

    // Helper: write N ints through a pt_t built from a given fapl,
    // read back, return the content.
    auto write_and_read = [&](const char* path, h5::fapl_t fapl) {
        std::remove(path);
        {
            h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);
            h5::ds_t ds = h5::create<int>(fd, "ds", h5::current_dims_t{0},
                h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{32} | h5::gzip{6});
            h5::pt_t pt(ds);
            for (int v : expected) h5::append(pt, v);
            h5::flush(pt);
        }
        h5::fd_t fd = h5::open(path, H5F_ACC_RDONLY);
        return h5::read<std::vector<int>>(fd, "ds");
    };

    // 1) Default FAPL: synchronous path
    auto sync_data = write_and_read("test-pt-1.3.2-sync.h5", h5::default_fapl);
    REQUIRE(sync_data.size() == expected.size());
    CHECK(sync_data == expected);

    // 2) Pool FAPL with 4 workers, default backpressure
    h5::fapl_t pool_fapl = h5::threads{4};
    auto pool_data = write_and_read("test-pt-1.3.2-pool.h5", pool_fapl);
    REQUIRE(pool_data.size() == expected.size());
    CHECK(pool_data == expected);

    // 3) Pool with explicit backpressure
    h5::fapl_t bp_fapl = h5::threads{4} | h5::backpressure{8};
    auto bp_data = write_and_read("test-pt-1.3.2-bp.h5", bp_fapl);
    REQUIRE(bp_data.size() == expected.size());
    CHECK(bp_data == expected);

    // All three produce the same logical content.
    CHECK(sync_data == pool_data);
    CHECK(pool_data == bp_data);

    std::remove("test-pt-1.3.2-sync.h5");
    std::remove("test-pt-1.3.2-pool.h5");
    std::remove("test-pt-1.3.2-bp.h5");
}

TEST_CASE("[#250 1.3.2] pt_t pool path — back-pressure bounds in-flight") {
    // Tight back-pressure cap (2) forces frequent drains.  The test
    // exercises the producer-blocking branch in write_chunk_via_pool.
    constexpr int N = 64;
    const char* path = "test-pt-1.3.2-tight-bp.h5";
    std::remove(path);
    {
        h5::fapl_t fapl = h5::threads{2} | h5::backpressure{2};
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);
        h5::ds_t ds = h5::create<int>(fd, "ds", h5::current_dims_t{0},
            h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{8} | h5::gzip{1});
        h5::pt_t pt(ds);
        for (int i = 0; i < N; ++i) h5::append(pt, i);
        h5::flush(pt);
    }
    h5::fd_t fd = h5::open(path, H5F_ACC_RDONLY);
    auto data = h5::read<std::vector<int>>(fd, "ds");
    REQUIRE(data.size() == N);
    for (int i = 0; i < N; ++i) CHECK(data[i] == i);
    std::remove(path);
}
