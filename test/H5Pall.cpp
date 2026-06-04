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

TEST_CASE("[#250] h5::threads tag installs the DAPL property via property chain") {
    // h5::threads{N} is now a per-dataset DAPL property storing the fan-out N
    // (it survives H5Dget_access_plist, unlike the old FAPL pool).
    h5::dapl_t dapl = h5::threads{4};
    CHECK(h5::impl::resolve_dataset_threads(static_cast<hid_t>(dapl)) == 4);
}

TEST_CASE("[#250] h5::threads{} (no count) fans out across hardware_concurrency") {
    h5::dapl_t dapl = h5::threads{};
    const unsigned expected = std::max(1u, std::thread::hardware_concurrency());
    CHECK(h5::impl::resolve_dataset_threads(static_cast<hid_t>(dapl)) == expected);
    CHECK(h5::impl::global_pool().worker_count() == expected);
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

