/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

// Worker pool for the parallel filter stage + the per-dataset DAPL parallelism
// tags (h5::threads{N} / h5::backpressure{M}).
//
//     h5::write(fd, "ds", data, h5::chunk{C} | h5::gzip{6}, h5::threads{8});
//     h5::create<float>(fd, "ds", h5::chunk{C}, h5::threads{});   // hw_concurrency
//
// Parallelism is a per-DATASET concern, so it lives on the DAPL — which, unlike a
// FAPL property, survives the H5Dget_access_plist round-trip and is read back
// directly at the write/read site (no #286 fileno registry).  One process-global
// worker_pool_t (global_pool(), below) backs every dataset's filter fan-out:
// HDF5 (threadsafety-OFF / under the global recursive mutex) funnels all library
// calls through one thread, so datasets are written sequentially and never
// contend for the pool — a per-file pool would only oversubscribe.

#include "H5Pall.hpp"
#include "H5Zall.hpp"   // filter::warm_dispatch — resolve vendored CPU-dispatch single-threaded
#include "detail/doorbell.hpp"
#include "detail/stoppable_thread.hpp"
#include <atomic>
#include <cstdlib>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>


// Default in-flight chunk cap when h5::threads{N} is set without an
// accompanying h5::backpressure{N}.  Resolves to 8 × worker_count, which
// is loose enough to keep all workers fed during steady-state streaming
// and tight enough to bound memory growth (chunk_size × cap bytes per pt_t).
// Override at compile time via -DH5CPP_FAPL_BACKPRESSURE_DEFAULT_FACTOR=K.
#ifndef H5CPP_FAPL_BACKPRESSURE_DEFAULT_FACTOR
#define H5CPP_FAPL_BACKPRESSURE_DEFAULT_FACTOR 8u
#endif

namespace h5::impl {

// ─── Pool ────────────────────────────────────────────────────────────────────
//
// Generic compute pool: workers pull type-erased tasks off a single MPMC-ish
// queue (std::mutex + std::queue + doorbell signal) and execute them.  The
// submit<F>(callable) API returns a std::future of the callable's result type,
// using std::packaged_task wrapped in a std::function<void()> for storage.
//
// The pool is deliberately HDF5-agnostic at this layer.  Consumer sites
// (Phase 1.3 — pt_t, h5::write, h5::read) wrap their HDF5-specific compress
// logic in a closure and submit() it.  This keeps the pool reusable for any
// future parallel-compute work.
struct worker_pool_t {
    // Pool size is fixed at construction; cannot resize at runtime.
    // n == 0 means "use std::thread::hardware_concurrency()".
    explicit worker_pool_t(unsigned n) {
        // Resolve vendored compressors' lazy CPU-feature dispatch single-threaded
        // before any worker can run; otherwise the first parallel compress/decompress
        // self-patches a process-global function pointer from several threads at once
        // (TSan: data race on libdeflate's 'adler32_impl').  See filter::warm_dispatch.
        h5::impl::filter::warm_dispatch();
        const unsigned count = n ? n : std::max(1u, std::thread::hardware_concurrency());
        workers_.reserve(count);
        for (unsigned i = 0; i < count; ++i)
            workers_.emplace_back([this](h5::detail::stop_token_t st) { worker_loop(st); });
    }

    // Destruction sequence:
    //   1. wait_idle() — let all submitted tasks complete (caller can also
    //      have called this earlier).
    //   2. Set stopping_ flag and ring the doorbell so all waiters wake.
    //   3. stoppable_thread_t destructors automatically request stop + join.
    ~worker_pool_t() {
        wait_idle();
        stopping_.store(true, std::memory_order_release);
        bell_.ring_all();
    }

    worker_pool_t(const worker_pool_t&) = delete;
    worker_pool_t& operator=(const worker_pool_t&) = delete;
    worker_pool_t(worker_pool_t&&) = delete;
    worker_pool_t& operator=(worker_pool_t&&) = delete;

    // Introspection for tests and scheduling decisions.
    [[nodiscard]] unsigned worker_count() const noexcept {
        return static_cast<unsigned>(workers_.size());
    }

    // Submit a callable for execution by any worker.  Returns a future of
    // the callable's result type.  Callable must be invocable with no
    // arguments; capture state via closure if needed.
    //
    // Internally uses std::packaged_task to bridge the move-only callable
    // into a copyable std::function (wrapped in shared_ptr) so it can sit
    // in the queue.
    template <typename Fn>
    auto submit(Fn&& fn) -> std::future<std::invoke_result_t<Fn>> {
        using R = std::invoke_result_t<Fn>;
        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<Fn>(fn));
        auto fut = task->get_future();
        in_flight_.fetch_add(1, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lk(m_);
            tasks_.emplace([task] { (*task)(); });
        }
        bell_.ring();
        return fut;
    }

    // Block until all submitted tasks have completed.  Intended for clean
    // shutdown or as a synchronization barrier between submission phases.
    void wait_idle() {
        while (in_flight_.load(std::memory_order_acquire) > 0)
            std::this_thread::yield();
    }

    // Coarse approximation of pending+running work.  Useful for tests and
    // monitoring; not a strict ordering guarantee.
    [[nodiscard]] int in_flight() const noexcept {
        return in_flight_.load(std::memory_order_acquire);
    }

private:
    void worker_loop(h5::detail::stop_token_t st) {
        while (!st.stop_requested()) {
            std::function<void()> task;
            std::uint32_t last_seq = 0;
            bool got_task = false;
            {
                std::lock_guard<std::mutex> lk(m_);
                if (!tasks_.empty()) {
                    task = std::move(tasks_.front());
                    tasks_.pop();
                    got_task = true;
                } else {
                    // Snapshot the doorbell sequence WHILE HOLDING the lock.
                    // submit() will ring the bell only AFTER it has released
                    // the same lock, so any subsequent ring advances past
                    // last_seq — bell_.wait(last_seq) below cannot miss it.
                    last_seq = bell_.load();
                }
            }

            if (got_task) {
                try { task(); } catch (...) { /* packaged_task captures it */ }
                in_flight_.fetch_sub(1, std::memory_order_release);
                continue;
            }

            // Queue was empty — wait for a ring or for shutdown.
            if (st.stop_requested() ||
                stopping_.load(std::memory_order_acquire)) return;
            bell_.wait(last_seq);
        }
    }

    std::mutex                                  m_;
    std::queue<std::function<void()>>           tasks_;
    h5::detail::doorbell_t                      bell_;
    std::atomic<int>                            in_flight_{0};
    std::atomic<bool>                           stopping_{false};
    std::vector<h5::detail::stoppable_thread_t> workers_;
};

// ─── Global worker pool ──────────────────────────────────────────────────────
//
// One process-wide pool backs every dataset's parallel filter stage. Justified
// by the single-producer model: HDF5 (threadsafety-OFF, or the global recursive
// mutex) funnels all library calls through one thread, so datasets are written
// sequentially and never contend for the pool — a per-file pool would only
// oversubscribe. Lazily constructed, hardware-sized; size is overridable BEFORE
// first use via set_pool_size() or the H5CPP_THREADS env var. Lives until program
// exit (the function-local static's jthreads join at static teardown; the pool
// owns no HDF5 handles, so teardown order is irrelevant).
inline unsigned& global_pool_size_override() noexcept { static unsigned n = 0; return n; }

// Set the global pool size; no-op once the pool has been constructed.
inline void set_pool_size(unsigned n) noexcept { global_pool_size_override() = n; }

inline worker_pool_t& global_pool() {
    static worker_pool_t pool([]() -> unsigned {
        unsigned n = global_pool_size_override();
        if (!n) if (const char* e = std::getenv("H5CPP_THREADS"))
            n = static_cast<unsigned>(std::strtoul(e, nullptr, 10));
        return n;   // 0 → worker_pool_t maps to hardware_concurrency
    }());
    return pool;
}

// Non-owning shared_ptr alias to the global pool, for consumers (pool_pipeline_t,
// pt_t) whose interface takes a shared_ptr.  The global pool outlives every
// pipeline, so the deleter is a no-op — nothing here owns the pool.
inline std::shared_ptr<worker_pool_t> global_pool_ptr() {
    return std::shared_ptr<worker_pool_t>(&global_pool(), [](worker_pool_t*){});
}

// ─── DAPL parallelism properties (the cleanup target) ────────────────────────
//
// threads{N} / backpressure{M} become per-DATASET DAPL properties: parallelism
// is a dataset-IO concern, and — unlike a FAPL property — a DAPL user property
// SURVIVES the H5Dget_access_plist round-trip, so it is read back directly at
// the write site with no #286 registry. Both are plain trivially-copyable
// unsigned values (HDF5's default memcpy copy is correct; no callbacks).
#define H5CPP_DAPL_THREADS      "h5cpp_dapl_threads"
#define H5CPP_DAPL_BACKPRESSURE "h5cpp_dapl_backpressure"

inline herr_t dapl_threads_set(::hid_t dapl, unsigned n) {
    if (H5Pexist(dapl, H5CPP_DAPL_THREADS)) return 0;
    // threads{} (n==0) means "fan out across the whole pool" — store the resolved
    // worker count so a stored value of 0 unambiguously means "property absent /
    // no pool" at the read site (resolve_dataset_threads).
    if (n == 0) n = std::max(1u, std::thread::hardware_concurrency());
    return H5Pinsert2(dapl, H5CPP_DAPL_THREADS, sizeof(unsigned), &n,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}
inline herr_t dapl_backpressure_set(::hid_t dapl, unsigned cap) {
    if (H5Pexist(dapl, H5CPP_DAPL_BACKPRESSURE)) return 0;
    return H5Pinsert2(dapl, H5CPP_DAPL_BACKPRESSURE, sizeof(unsigned), &cap,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

// N for this dataset (0 = not requested → no pool / synchronous filters).
inline unsigned resolve_dataset_threads(::hid_t dapl_id) noexcept {
    if (dapl_id < 0 || H5Iis_valid(dapl_id) <= 0) return 0;
    if (!H5Pexist(dapl_id, H5CPP_DAPL_THREADS)) return 0;
    unsigned n = 0;
    H5Pget(dapl_id, H5CPP_DAPL_THREADS, &n);
    return n;
}

// In-flight chunk cap for this dataset: explicit backpressure{M}, else the
// default factor × the dataset's requested concurrency N.
inline unsigned resolve_dataset_backpressure(::hid_t dapl_id, unsigned n) noexcept {
    if (dapl_id >= 0 && H5Iis_valid(dapl_id) > 0
            && H5Pexist(dapl_id, H5CPP_DAPL_BACKPRESSURE)) {
        unsigned cap = 0;
        H5Pget(dapl_id, H5CPP_DAPL_BACKPRESSURE, &cap);
        if (cap > 0) return cap;
    }
    return H5CPP_FAPL_BACKPRESSURE_DEFAULT_FACTOR * (n ? n : global_pool().worker_count());
}

} // namespace h5::impl

namespace h5 {
// User-facing tags.  Parallelism is a per-DATASET concern, so these are DAPL
// properties applied alongside the dataset's other access/create properties:
//
//     h5::write (fd, "ds", data, h5::chunk{C} | h5::gzip{6}, h5::threads{8});
//     h5::create<float>(fd, "ds", h5::chunk{C}, h5::threads{8} | h5::backpressure{32});
//     h5::pt_t pt = h5::create<float>(fd, "ds", h5::chunk{C}, h5::threads{8});
//
// h5::threads{N} marks this dataset to fan its filter stage out across N workers
// of the process-global pool; h5::threads{} uses hardware_concurrency.
// h5::backpressure{M} bounds in-flight chunks; without h5::threads{N} it is
// silently a no-op (no pool → no queue to bound).  Unlike the old FAPL pool, a
// DAPL property survives the H5Dget_access_plist round-trip, so it is read back
// directly at the write site — no #286 registry.
using threads      = impl::dapl_call<impl::dapl_args<hid_t, unsigned>,
                                     impl::dapl_threads_set>;
using backpressure = impl::dapl_call<impl::dapl_args<hid_t, unsigned>,
                                     impl::dapl_backpressure_set>;
}
