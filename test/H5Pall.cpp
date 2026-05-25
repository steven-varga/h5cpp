#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/all>
#include <h5cpp/core>
#include <h5cpp/io>
#include <h5cpp/H5Pall.hpp>
#include <atomic>
#include <thread>
#include <unordered_set>
#include <vector>
#include "support/fixture.hpp"

TEST_CASE("instantiate commonly used property types") {
    h5::chunk c{10};
    h5::gzip g{6};
    h5::deflate d{6};
    h5::flag::fletcher32 f;
    h5::flag::shuffle s;
    h5::flag::nbit n;
    h5::fill_time ft{H5D_FILL_TIME_ALLOC};
    h5::alloc_time at{H5D_ALLOC_TIME_EARLY};
    h5::layout l{H5D_CHUNKED};
    h5::libver_bounds lv({H5F_LIBVER_LATEST, H5F_LIBVER_LATEST});
    h5::fclose_degree fd_{H5F_CLOSE_WEAK};
    h5::char_encoding ce{H5T_CSET_UTF8};
    h5::create_intermediate_group cig{1};
    CHECK(true);
}

TEST_CASE("instantiate dead fcpl property types") {
    h5::sizes sz;
    h5::sym_k sk;
    h5::istore_k ik;
    h5::shared_mesg_nindexes smn;
    h5::shared_mesg_index smi;
    h5::shared_mesg_phase_change smpc;
    h5::userblock ub;
    CHECK(true);
}

TEST_CASE("instantiate dead fapl property types") {
    h5::cache ca;
    h5::alignment al;
    h5::meta_block_size mbs;
    h5::sieve_buf_size sbs;
    h5::elink_file_cache_size efcs;
    CHECK(true);
}

TEST_CASE("instantiate dead gcpl property types") {
    h5::local_heap_size_hint lhsh;
    h5::link_creation_order lco;
    h5::est_link_info eli;
    h5::link_phase_change lpc;
    CHECK(true);
}

TEST_CASE("property types can be daisy-chained with operator|") {
    h5::dcpl_t props = h5::chunk{10} | h5::gzip{6} | h5::flag::fletcher32{};
    CHECK(static_cast<hid_t>(props) > 0);
}

// =====================================================================
// [#242] DAPL high_throughput pipeline lifecycle — regression guard
// =====================================================================
//
// HDF5 ≥ 1.10.7 internally copies the DAPL during H5Dopen / H5Dcreate so the
// dataset owns its own.  Without a copy callback on the high_throughput
// property, both the user's DAPL and HDF5's internal copy hold the same
// pipeline pointer → close callback fires twice on the same pointer →
// double-free.  The fix in H5Pdapl.hpp adds dapl_pipeline_copy that allocates
// a fresh pipeline for the destination DAPL.
//
// This test installs a tracking close-callback (count invocations, verify each
// pointer freed exactly once) directly via H5Pinsert2, exercises an open/close
// cycle, and asserts no pointer is freed more than once.
namespace h5_242_regression {
    using pipeline = h5::impl::pipeline_t<h5::impl::basic_pipeline_t>;
    inline std::atomic<int> allocations{0};
    inline std::atomic<int> deletions{0};
    inline std::unordered_set<void*>& live_pointers() {
        static std::unordered_set<void*> s;
        return s;
    }
    inline std::atomic<bool> double_free_detected{false};

    inline herr_t tracking_close_cb(const char*, size_t, void* ptr) {
        auto* stored = *static_cast<pipeline**>(ptr);
        auto& live = live_pointers();
        auto it = live.find(stored);
        if (it == live.end()) {
            double_free_detected.store(true);
            return -1;
        }
        live.erase(it);
        deletions.fetch_add(1);
        delete stored;
        return 0;
    }
    inline herr_t tracking_copy_cb(const char*, size_t, void* value) {
        auto** ptr_loc = static_cast<pipeline**>(value);
        auto* fresh = new pipeline();
        live_pointers().insert(fresh);
        allocations.fetch_add(1);
        *ptr_loc = fresh;
        return 0;
    }

    inline h5::dapl_t make_tracked_dapl(bool with_copy_cb) {
        hid_t raw = H5Pcreate(H5P_DATASET_ACCESS);
        auto* p = new pipeline();
        live_pointers().insert(p);
        allocations.fetch_add(1);
        H5Pinsert2(raw, "h5cpp_dapl_highthroughput",
                   sizeof(pipeline*), &p,
                   nullptr, nullptr, nullptr,
                   with_copy_cb ? tracking_copy_cb : nullptr,
                   nullptr, tracking_close_cb);
        return h5::dapl_t{raw};
    }

    inline void reset_counters() {
        allocations.store(0);
        deletions.store(0);
        live_pointers().clear();
        double_free_detected.store(false);
    }
}

TEST_CASE("[#242] regression scaffold — test fails when copy-cb is omitted") {
    // Sanity check on the test itself: install the property WITHOUT a copy
    // callback (the broken pre-fix state) and confirm our tracking close-cb
    // detects the double-free. Guards against silent test breakage where the
    // assertion becomes a no-op.
    using namespace h5_242_regression;
    reset_counters();

    {
        h5::test::file_fixture_t f("test-pdapl-242-scaffold.h5");
        h5::dapl_t my_dapl = make_tracked_dapl(/*with_copy_cb=*/false);
        {
            h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{0},
                h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{16}, my_dapl);
            (void)ds;
        }
    }

    // Without copy-cb the close-cb fires twice on the same pointer.
    CHECK(double_free_detected.load());
    // Reset state so subsequent tests start clean.
    reset_counters();
}

TEST_CASE("[#242] high_throughput pipeline survives open/close without double-free") {
    using namespace h5_242_regression;
    reset_counters();

    {
        h5::test::file_fixture_t f("test-pdapl-242.h5");
        h5::dapl_t my_dapl = make_tracked_dapl(/*with_copy_cb=*/true);
        // Open/close cycle — HDF5 copies the DAPL into the dataset, then destroys
        // the copy on H5Dclose, firing the close-cb on the COPY's pipeline pointer.
        {
            h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{0},
                h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{16}, my_dapl);
            (void)ds;
        }
        // my_dapl destructs at end of scope, firing close-cb on the USER's pointer.
    }

    CHECK(!double_free_detected.load());
    // Each allocation (user + each copy) was freed exactly once.
    CHECK(allocations.load() == deletions.load());
    CHECK(live_pointers().empty());
    CHECK(allocations.load() >= 1);   // at least the user's pipeline existed
}

TEST_CASE("[#242] high_throughput round-trip end-to-end via h5::write / h5::read") {
    using namespace h5_242_regression;
    reset_counters();

    constexpr int N = 128;
    std::vector<int> expected(N);
    for (int i = 0; i < N; ++i) expected[i] = i * 7 + 3;

    {
        h5::test::file_fixture_t f("test-pdapl-242-rt.h5");
        h5::dapl_t my_dapl = make_tracked_dapl(/*with_copy_cb=*/true);
        h5::ds_t ds = h5::create<int>(f.fd, "ds", h5::current_dims_t{N},
            h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{32}, my_dapl);
        h5::write(ds, expected.data(), h5::count{N});
    }
    {
        h5::fd_t fd = h5::open("test-pdapl-242-rt.h5", H5F_ACC_RDONLY);
        auto readback = h5::read<std::vector<int>>(fd, "ds");
        REQUIRE(readback.size() == expected.size());
        CHECK(readback == expected);
    }
    CHECK(!double_free_detected.load());
    CHECK(allocations.load() == deletions.load());
    CHECK(live_pointers().empty());
}

// regression guard for issue #239 — combining a property builder with an
// existing property-list handle (v1.10-era idiom) must remain supported.
TEST_CASE("property builder can be chained against an existing dcpl handle") {
    h5::dcpl_t base = h5::gzip{6};
    REQUIRE(static_cast<hid_t>(base) > 0);

    h5::dcpl_t combined = h5::chunk{64 * 1024} | base;
    REQUIRE(static_cast<hid_t>(combined) > 0);
    CHECK(static_cast<hid_t>(combined) != static_cast<hid_t>(base)); // deep copy

    // chunk dims set by the LHS builder survive the merge
    hsize_t chunk_dims[H5S_MAX_RANK] = {0};
    int rank = H5Pget_chunk(static_cast<hid_t>(combined), H5S_MAX_RANK, chunk_dims);
    CHECK(rank == 1);
    CHECK(chunk_dims[0] == 64 * 1024);

    // deflate filter inherited from base — scan all filters since pipeline order
    // depends on H5Pcopy + lazy-apply semantics.
    int nfilters = H5Pget_nfilters(static_cast<hid_t>(combined));
    CHECK(nfilters == 1);
    bool found_deflate = false;
    for (int i = 0; i < nfilters; ++i) {
        unsigned flags = 0, filter_config = 0;
        size_t cd_nelmts = 1;
        unsigned cd_values[1] = {0};
        char name[16] = {0};
        H5Z_filter_t f = H5Pget_filter2(static_cast<hid_t>(combined), i, &flags,
            &cd_nelmts, cd_values, sizeof(name), name, &filter_config);
        if (f == H5Z_FILTER_DEFLATE && cd_values[0] == 6)
            found_deflate = true;
    }
    CHECK(found_deflate);
}

// =====================================================================
// [#250] Phase I — FAPL worker-pool slot lifecycle
// =====================================================================
//
// Same H5Pinsert2 + slot pattern as the DAPL pipeline (#242/#244), but with
// shared (refcounted) ownership instead of fresh-allocation-per-copy.
// Multiple FAPL copies alias one underlying worker_pool_t via shared_ptr;
// the pool is destroyed when the last live FAPL copy releases its slot.
//
// Workplan: tasks/h5cpp-fapl-multithreading-workplan.md §3.

namespace h5_250_fapl_regression {
    using slot_t = h5::impl::worker_pool_slot_t;
    using pool_t = h5::impl::worker_pool_t;

    inline std::atomic<int> slot_allocations{0};
    inline std::atomic<int> slot_deletions{0};
    inline std::unordered_set<void*>& live_slots() {
        static std::unordered_set<void*> s;
        return s;
    }
    inline std::atomic<bool> double_free_detected{false};

    inline herr_t tracking_close_cb(const char*, size_t, void* ptr) {
        auto* slot = *static_cast<slot_t**>(ptr);
        auto& live = live_slots();
        auto it = live.find(slot);
        if (it == live.end()) {
            double_free_detected.store(true);
            return -1;
        }
        live.erase(it);
        slot_deletions.fetch_add(1);
        delete slot;
        return 0;
    }
    inline herr_t tracking_copy_cb(const char*, size_t, void* value) {
        auto** loc = static_cast<slot_t**>(value);
        auto* fresh = new slot_t{(*loc)->pool};
        live_slots().insert(fresh);
        slot_allocations.fetch_add(1);
        *loc = fresh;
        return 0;
    }

    inline hid_t make_tracked_fapl(unsigned workers, bool with_copy_cb) {
        hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
        auto* slot = new slot_t{std::make_shared<pool_t>(workers)};
        live_slots().insert(slot);
        slot_allocations.fetch_add(1);
        H5Pinsert2(fapl, H5CPP_FAPL_WORKER_POOL, sizeof(slot_t*), &slot,
                   nullptr, nullptr, nullptr,
                   with_copy_cb ? tracking_copy_cb : nullptr,
                   nullptr, tracking_close_cb);
        return fapl;
    }

    inline void reset_counters() {
        slot_allocations.store(0);
        slot_deletions.store(0);
        live_slots().clear();
        double_free_detected.store(false);
    }
}

TEST_CASE("[#250] regression scaffold — slot lifecycle test fails when copy-cb is omitted") {
    using namespace h5_250_fapl_regression;
    reset_counters();
    {
        hid_t fapl_a = make_tracked_fapl(/*workers=*/4, /*copy_cb=*/false);
        hid_t fapl_b = H5Pcopy(fapl_a);
        H5Pclose(fapl_a);
        H5Pclose(fapl_b);
    }
    CHECK(double_free_detected.load());
    reset_counters();
}

TEST_CASE("[#250] single FAPL clean lifecycle") {
    using namespace h5_250_fapl_regression;
    reset_counters();
    {
        hid_t fapl = make_tracked_fapl(/*workers=*/4, /*copy_cb=*/true);
        H5Pclose(fapl);
    }
    CHECK(!double_free_detected.load());
    CHECK(slot_allocations.load() == slot_deletions.load());
    CHECK(live_slots().empty());
}

TEST_CASE("[#250] H5Pcopy preserves shared pool ownership across FAPL copies") {
    using namespace h5_250_fapl_regression;
    reset_counters();
    {
        hid_t fapl_a = make_tracked_fapl(/*workers=*/4, /*copy_cb=*/true);
        auto pool_a = h5::impl::resolve_worker_pool(fapl_a);
        REQUIRE(pool_a);
        CHECK(pool_a->worker_count() == 4);

        hid_t fapl_b = H5Pcopy(fapl_a);
        auto pool_b = h5::impl::resolve_worker_pool(fapl_b);
        REQUIRE(pool_b);
        CHECK(pool_a.get() == pool_b.get());     // SAME pool — refcount shared

        H5Pclose(fapl_a);
        // pool still alive via fapl_b's slot
        auto pool_after_close = h5::impl::resolve_worker_pool(fapl_b);
        CHECK(pool_after_close.get() == pool_a.get());

        H5Pclose(fapl_b);
    }
    CHECK(!double_free_detected.load());
    CHECK(slot_allocations.load() == slot_deletions.load());
    CHECK(live_slots().empty());
}

TEST_CASE("[#250] h5::threads tag installs the FAPL property via property chain") {
    using namespace h5_250_fapl_regression;
    reset_counters();
    {
        // Construct an FAPL with h5::threads{N} applied via the property
        // chain — this exercises the real fapl_threads_set callback path,
        // not the tracking shim.  Uses h5cpp's actual copy/close callbacks.
        h5::fapl_t fapl = h5::threads{4};
        auto pool = h5::impl::resolve_worker_pool(static_cast<hid_t>(fapl));
        REQUIRE(pool);
        CHECK(pool->worker_count() == 4);
    }
    // Pool is destroyed when fapl goes out of scope — no leak detection
    // possible here without separate scaffolding, but TSAN coverage will
    // catch any worker-thread shutdown issues.
}

TEST_CASE("[#250] h5::threads{} (no count) uses hardware_concurrency") {
    h5::fapl_t fapl = h5::threads{};
    auto pool = h5::impl::resolve_worker_pool(static_cast<hid_t>(fapl));
    REQUIRE(pool);
    const unsigned expected = std::max(1u, std::thread::hardware_concurrency());
    CHECK(pool->worker_count() == expected);
}

// =====================================================================
// [#250] Phase 1.2 — worker_pool_t generic submit() + wait_idle()
// =====================================================================

TEST_CASE("[#250 1.2] worker_pool_t — single submit + future return") {
    h5::impl::worker_pool_t pool{4};
    REQUIRE(pool.worker_count() == 4);

    auto fut = pool.submit([] { return 42; });
    CHECK(fut.get() == 42);
    pool.wait_idle();   // close the small post-future-ready/pre-decrement window
    CHECK(pool.in_flight() == 0);
}

TEST_CASE("[#250 1.2] worker_pool_t — many submits resolve in parallel") {
    constexpr unsigned N = 256;
    h5::impl::worker_pool_t pool{8};
    std::vector<std::future<int>> futures;
    futures.reserve(N);
    for (unsigned i = 0; i < N; ++i)
        futures.emplace_back(pool.submit([i] { return static_cast<int>(i * i); }));

    int sum = 0;
    for (auto& f : futures) sum += f.get();
    int expected = 0;
    for (unsigned i = 0; i < N; ++i) expected += static_cast<int>(i * i);
    CHECK(sum == expected);
    pool.wait_idle();   // close the small post-future-ready/pre-decrement window
    CHECK(pool.in_flight() == 0);
}

TEST_CASE("[#250 1.2] worker_pool_t — multi-thread submission is safe (MPMC)") {
    constexpr unsigned PRODUCERS = 4;
    constexpr unsigned PER_PRODUCER = 100;
    h5::impl::worker_pool_t pool{4};
    std::atomic<int> total{0};

    std::vector<std::thread> producers;
    producers.reserve(PRODUCERS);
    for (unsigned p = 0; p < PRODUCERS; ++p) {
        producers.emplace_back([&pool, &total] {
            for (unsigned i = 0; i < PER_PRODUCER; ++i)
                pool.submit([&total] { total.fetch_add(1); }).wait();
        });
    }
    for (auto& t : producers) t.join();
    CHECK(total.load() == static_cast<int>(PRODUCERS * PER_PRODUCER));
    pool.wait_idle();   // close the small post-future-ready/pre-decrement window
    CHECK(pool.in_flight() == 0);
}

TEST_CASE("[#250 1.2] worker_pool_t — wait_idle blocks until completion") {
    h5::impl::worker_pool_t pool{2};
    std::atomic<int> done{0};
    for (int i = 0; i < 50; ++i)
        pool.submit([&done] {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            done.fetch_add(1);
        });
    pool.wait_idle();
    CHECK(done.load() == 50);
    pool.wait_idle();   // close the small post-future-ready/pre-decrement window
    CHECK(pool.in_flight() == 0);
}

TEST_CASE("[#250 1.2] worker_pool_t — exception in task is captured in future") {
    h5::impl::worker_pool_t pool{2};
    auto fut = pool.submit([] () -> int { throw std::runtime_error("boom"); });
    bool caught = false;
    try { (void)fut.get(); }
    catch (const std::runtime_error& e) {
        caught = std::string(e.what()) == "boom";
    }
    CHECK(caught);
    pool.wait_idle();   // close the small post-future-ready/pre-decrement window
    CHECK(pool.in_flight() == 0);
}

TEST_CASE("[#250 1.2] worker_pool_t — dtor drains pending work cleanly") {
    std::atomic<int> done{0};
    {
        h5::impl::worker_pool_t pool{4};
        for (int i = 0; i < 100; ++i)
            pool.submit([&done] {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                done.fetch_add(1);
            });
        // No explicit wait_idle — dtor must drain before joining workers.
    }
    CHECK(done.load() == 100);
}

// =====================================================================
// [#250] Phase 1.3.1 — h5::backpressure FAPL property
// =====================================================================

TEST_CASE("[#250 1.3] h5::backpressure tag installs the FAPL property") {
    h5::fapl_t fapl = h5::threads{4} | h5::backpressure{16};
    auto pool = h5::impl::resolve_worker_pool(static_cast<hid_t>(fapl));
    REQUIRE(pool);
    CHECK(pool->worker_count() == 4);
    CHECK(h5::impl::resolve_backpressure(static_cast<hid_t>(fapl), pool->worker_count()) == 16);
}

TEST_CASE("[#250 1.3] resolve_backpressure default = 8 × worker_count when unset") {
    h5::fapl_t fapl = h5::threads{4};
    auto pool = h5::impl::resolve_worker_pool(static_cast<hid_t>(fapl));
    REQUIRE(pool);
    CHECK(h5::impl::resolve_backpressure(static_cast<hid_t>(fapl), pool->worker_count()) == 32u);
}

TEST_CASE("[#250 1.3] backpressure property survives H5Pcopy") {
    h5::fapl_t fapl = h5::threads{4} | h5::backpressure{64};
    hid_t copy = H5Pcopy(static_cast<hid_t>(fapl));
    auto pool_copy = h5::impl::resolve_worker_pool(copy);
    REQUIRE(pool_copy);
    CHECK(h5::impl::resolve_backpressure(copy, pool_copy->worker_count()) == 64u);
    H5Pclose(copy);
}

TEST_CASE("[#250 1.3] backpressure without threads — resolver returns default but no pool") {
    h5::fapl_t fapl = h5::backpressure{32};
    CHECK(h5::impl::resolve_worker_pool(static_cast<hid_t>(fapl)) == nullptr);
    // Cap is present even without a pool — resolver returns user value.
    // pt_t / h5::write are responsible for ignoring it when no pool exists.
    CHECK(h5::impl::resolve_backpressure(static_cast<hid_t>(fapl), 0) == 32u);
}

// =====================================================================
// [#250 1.3.3] h5::write / h5::read pool integration end-to-end
// =====================================================================
//
// Per workplan Approach 2: h5::write / h5::read consult the file's FAPL
// for a pool and route through a local pool_pipeline_t when one exists.
// Per-dataset opt-in is still h5::high_throughput on the DAPL; without
// it, even a FAPL-pool-equipped file falls through to standard H5Dwrite.

TEST_CASE("[#250 1.3.3] h5::write + h5::read round-trip through FAPL pool") {
    constexpr int N = 256;
    std::vector<int> expected(N);
    for (int i = 0; i < N; ++i) expected[i] = i * 11 + 7;

    const char* path = "test-h5write-1.3.3-pool.h5";
    std::remove(path);
    {
        h5::fapl_t fapl = h5::threads{4} | h5::backpressure{16};
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);
        // chunked + gzip + DAPL high_throughput to opt the dataset into
        // the pipeline path; the FAPL pool then claims the chunks.
        h5::dapl_t dapl = h5::high_throughput;
        h5::ds_t ds = h5::create<int>(fd, "ds", h5::current_dims_t{N},
            h5::max_dims_t{H5S_UNLIMITED},
            h5::chunk{32} | h5::gzip{6}, dapl);
        h5::write(ds, expected.data(), h5::count{N});
    }
    {
        h5::fapl_t fapl = h5::threads{4};
        h5::fd_t fd = h5::open(path, H5F_ACC_RDONLY, h5::default_fapl);
        // Read back without a pool — verify the data is on disk regardless
        // of FAPL choice on the reader side.
        auto data = h5::read<std::vector<int>>(fd, "ds");
        REQUIRE(data.size() == expected.size());
        CHECK(data == expected);
    }
    std::remove(path);
}

TEST_CASE("[#250 1.3.3] h5::write without high_throughput bypasses pool") {
    // Even with FAPL pool installed, h5::write to a dataset whose DAPL
    // doesn't have high_throughput should go through standard H5Dwrite.
    constexpr int N = 128;
    std::vector<int> expected(N);
    for (int i = 0; i < N; ++i) expected[i] = i;

    const char* path = "test-h5write-1.3.3-no-ht.h5";
    std::remove(path);
    {
        h5::fapl_t fapl = h5::threads{4};
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);
        // No high_throughput on the DAPL; pool is on FAPL but unused.
        h5::ds_t ds = h5::create<int>(fd, "ds", h5::current_dims_t{N},
            h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{16});
        h5::write(ds, expected.data(), h5::count{N});
    }
    {
        h5::fd_t fd = h5::open(path, H5F_ACC_RDONLY);
        auto data = h5::read<std::vector<int>>(fd, "ds");
        REQUIRE(data.size() == expected.size());
        CHECK(data == expected);
    }
    std::remove(path);
}

// =====================================================================
// [#250 1.5] TSAN-targeted coverage — multi-fd isolation, bytewise
// equivalence vs. basic pipeline, fd shutdown drains in-flight chunks
// =====================================================================
//
// These tests exist to harden the FAPL-scoped pool against data races
// and ownership bugs that show up under `-fsanitize=thread`.  They
// run as ordinary ctest cases on the local toolchain (no TSAN gate),
// and as race detectors on the CI `tsan` job introduced in #250
// Phase 1.5.

TEST_CASE("[#250 1.5] two FAPLs with independent pools — bytewise equivalence to basic") {
    // Same payload, gzip-compressed, written via three configurations:
    // basic pipeline, FAPL pool A (4 threads), FAPL pool B (2 threads).
    // All three on-disk files must read back to the same logical data.
    constexpr int N = 1024;
    std::vector<int> expected(N);
    for (int i = 0; i < N; ++i) expected[i] = (i * 31337) ^ 0xCAFE;

    auto write_with = [&](const char* path, h5::fapl_t fapl) {
        std::remove(path);
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);
        h5::dapl_t dapl = h5::high_throughput;
        h5::ds_t ds = h5::create<int>(fd, "ds", h5::current_dims_t{N},
            h5::max_dims_t{H5S_UNLIMITED},
            h5::chunk{64} | h5::gzip{6}, dapl);
        h5::write(ds, expected.data(), h5::count{N});
    };

    const char* p_basic = "test-1.5-basic.h5";
    const char* p_pool4 = "test-1.5-pool4.h5";
    const char* p_pool2 = "test-1.5-pool2.h5";

    write_with(p_basic, h5::default_fapl);
    write_with(p_pool4, h5::fapl_t{h5::threads{4} | h5::backpressure{16}});
    write_with(p_pool2, h5::fapl_t{h5::threads{2} | h5::backpressure{8}});

    auto load = [](const char* p) {
        h5::fd_t fd = h5::open(p, H5F_ACC_RDONLY);
        return h5::read<std::vector<int>>(fd, "ds");
    };

    auto a = load(p_basic), b = load(p_pool4), c = load(p_pool2);
    REQUIRE(a.size() == expected.size());
    CHECK(a == expected);
    CHECK(b == expected);
    CHECK(c == expected);
    CHECK(a == b);
    CHECK(b == c);

    std::remove(p_basic);
    std::remove(p_pool4);
    std::remove(p_pool2);
}

TEST_CASE("[#250 1.5] interleaved writes through two FAPLs do not cross-contaminate") {
    // Two simultaneously-open files, each owning its own h5::fd_t with
    // its own FAPL pool.  Writes are issued sequentially (HDF5 itself is
    // not thread-safe in default builds), but both pools are alive at
    // the same time — a shared-state bug would cross chunks between
    // them.  Compression on each pool's workers runs in parallel.
    //
    // (Concurrent H5 C-API calls from multiple threads are explicitly
    // out of scope: the FAPL pool parallelizes compression, not HDF5
    // itself.  See Phase II in the workplan for the async/thread-safe
    // story.)
    constexpr int N = 512;
    std::vector<int> payload_a(N), payload_b(N);
    for (int i = 0; i < N; ++i) {
        payload_a[i] = i * 3 + 1;
        payload_b[i] = i * 5 + 2;
    }

    const char* pa = "test-1.5-interleaved-a.h5";
    const char* pb = "test-1.5-interleaved-b.h5";
    std::remove(pa);
    std::remove(pb);

    {
        h5::fapl_t fapl_a = h5::threads{4};
        h5::fapl_t fapl_b = h5::threads{2};
        h5::fd_t fa = h5::create(pa, H5F_ACC_TRUNC, h5::default_fcpl, fapl_a);
        h5::fd_t fb = h5::create(pb, H5F_ACC_TRUNC, h5::default_fcpl, fapl_b);

        h5::dapl_t dapl = h5::high_throughput;
        h5::ds_t da = h5::create<int>(fa, "ds", h5::current_dims_t{N},
            h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{32} | h5::gzip{4}, dapl);
        h5::ds_t db = h5::create<int>(fb, "ds", h5::current_dims_t{N},
            h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{32} | h5::gzip{4}, dapl);

        // Interleave the writes so both pools have in-flight work
        // simultaneously.  h5::write on each side drains its own pool
        // before returning, so by the time we move on the other pool
        // is still alive with its own queued work.
        h5::write(da, payload_a.data(), h5::count{N});
        h5::write(db, payload_b.data(), h5::count{N});
    }

    h5::fd_t fa = h5::open(pa, H5F_ACC_RDONLY);
    h5::fd_t fb = h5::open(pb, H5F_ACC_RDONLY);
    auto ra = h5::read<std::vector<int>>(fa, "ds");
    auto rb = h5::read<std::vector<int>>(fb, "ds");
    REQUIRE(ra.size() == payload_a.size());
    REQUIRE(rb.size() == payload_b.size());
    CHECK(ra == payload_a);
    CHECK(rb == payload_b);

    std::remove(pa);
    std::remove(pb);
}

TEST_CASE("[#250 1.5] pt_t destructor drains pool before fd close — data on disk") {
    // pt_t::flush is called by ~pt_t, which on the pool path calls
    // pool_pipeline_t::drain() and blocks until every in-flight chunk
    // has completed compression + H5Dwrite_chunk.  After the pt_t
    // scope exits, the data must be readable from a fresh open even
    // though the worker pool is still alive (held by the FAPL slot).
    constexpr int N = 384;
    std::vector<int> expected(N);
    for (int i = 0; i < N; ++i) expected[i] = i * 13;

    const char* path = "test-1.5-pt-drain.h5";
    std::remove(path);
    {
        h5::fapl_t fapl = h5::threads{4} | h5::backpressure{8};
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);
        h5::dapl_t dapl = h5::high_throughput;
        h5::ds_t ds = h5::create<int>(fd, "ds", h5::current_dims_t{0},
            h5::max_dims_t{H5S_UNLIMITED},
            h5::chunk{24} | h5::gzip{6}, dapl);
        {
            h5::pt_t pt(ds);
            for (int v : expected) h5::append(pt, v);
            // No explicit h5::flush — let ~pt_t drain.
        }
        // ds and fd close at scope exit.
    }
    {
        h5::fd_t fd = h5::open(path, H5F_ACC_RDONLY);
        auto data = h5::read<std::vector<int>>(fd, "ds");
        REQUIRE(data.size() == expected.size());
        CHECK(data == expected);
    }
    std::remove(path);
}

TEST_CASE("[#250 1.5] fd close after FAPL pool flush releases workers") {
    // After the fd goes out of scope, the FAPL property's close
    // callback drops its shared_ptr to the worker_pool_t.  When the
    // last user releases, the pool destructor stops the workers
    // cleanly.  This test exercises the lifecycle directly; TSAN
    // catches use-after-free if the slot order is wrong.  Each
    // iteration uses a distinct path because Windows holds file
    // handles slightly longer than POSIX after the HDF5 close.
    constexpr int N = 256;
    std::vector<int> expected(N);
    for (int i = 0; i < N; ++i) expected[i] = i + 100;

    auto write_then_close = [&](const char* path) {
        std::remove(path);
        h5::fapl_t fapl = h5::threads{2};
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);
        h5::dapl_t dapl = h5::high_throughput;
        h5::ds_t ds = h5::create<int>(fd, "ds", h5::current_dims_t{N},
            h5::max_dims_t{H5S_UNLIMITED},
            h5::chunk{32} | h5::gzip{1}, dapl);
        h5::write(ds, expected.data(), h5::count{N});
        // ds / fd close at scope exit; FAPL slot drops the pool ref.
    };

    const char* paths[] = {
        "test-1.5-fd-close-a.h5",
        "test-1.5-fd-close-b.h5",
        "test-1.5-fd-close-c.h5",
        "test-1.5-fd-close-d.h5",
    };
    for (const char* p : paths) write_then_close(p);

    for (const char* p : paths) {
        h5::fd_t fd = h5::open(p, H5F_ACC_RDONLY);
        auto data = h5::read<std::vector<int>>(fd, "ds");
        REQUIRE(data.size() == expected.size());
        CHECK(data == expected);
        std::remove(p);
    }
}
