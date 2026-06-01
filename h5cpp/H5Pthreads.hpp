/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

// FAPL-scoped worker pool — opt-in via h5::threads{N}.
//
// User-facing API:
//
//     h5::fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC, h5::threads{8});
//     h5::fd_t fd = h5::create("data.h5", H5F_ACC_TRUNC, h5::threads{});   // hw_concurrency
//
// Per-dataset opt-in is the orthogonal h5::high_throughput DAPL flag
// (existing).  This FAPL property controls "is there a pool, how many
// workers"; whether any given dataset uses it is the DAPL's call.
//
// Storage mechanism mirrors h5::high_throughput (H5Pdapl.hpp):
//
//     - H5Pinsert2 stores a pointer to a worker_pool_slot_t in the FAPL
//       skip list.
//     - The slot owns a std::shared_ptr<worker_pool_t>.
//     - HDF5 internally copies the FAPL during H5Fopen/H5Fcreate; the
//       copy callback allocates a fresh slot aliasing the same pool
//       (shared_ptr refcount++).  Every FAPL copy shares the pool.
//     - The close callback drops the slot.  Pool is destroyed when the
//       last live FAPL copy releases its slot, at which point worker
//       std::jthreads receive request_stop() and join cleanly.
//
// This is the shared-ownership variant of the H5Pinsert2 + slot pattern.
// Compare with H5Pdapl.hpp's fresh-allocation-per-copy semantics used by
// the high_throughput pipeline property: that pattern allocates a fresh
// pipeline scratch buffer per copy because pipelines are per-write
// scratch state; this pattern shares one live resource across all
// copies because workers ARE the resource we want shared.  See
// tasks/h5cpp-fapl-multithreading-workplan.md §2-§3.
//
// PHASE 1.1 STATUS: lifecycle scaffolding only.  worker_pool_t owns N
// std::jthreads whose only job today is to honor std::stop_token on
// shutdown.  Phase 1.2 extends with bounded MPMC queues, compress_sync /
// compress_async API, and integration with the filter pipeline.  Phase
// 1.3 wires pt_t / h5::write / h5::read consumer sites.

#include "H5Pall.hpp"
#include "H5Zall.hpp"   // filter::warm_dispatch — resolve vendored CPU-dispatch single-threaded
#include "detail/doorbell.hpp"
#include "detail/stoppable_thread.hpp"
#include <atomic>
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

#define H5CPP_FAPL_WORKER_POOL  "h5cpp_fapl_worker_pool"
#define H5CPP_FAPL_BACKPRESSURE "h5cpp_fapl_backpressure"

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
// logic in a closure and submit() it.  This keeps the pool reusable for
// Phase II's executor and any future async work.
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

// ─── FAPL slot + lifecycle callbacks ─────────────────────────────────────────

// The heap-allocated holder whose pointer lives in the FAPL skip-list
// value slot.  Indirection is necessary because H5Pinsert2 stores raw
// bytes — it cannot run shared_ptr's constructor/destructor for us.
struct worker_pool_slot_t {
    std::shared_ptr<worker_pool_t> pool;
};

// Copy callback: HDF5 cloned the property bytes (the slot pointer was
// memcpy'd into the destination's value slot).  Allocate a NEW slot whose
// shared_ptr aliases the same pool — refcount++ via shared_ptr copy.
//
// This is the contract that makes "every FAPL copy shares one pool" work.
inline herr_t fapl_pool_copy_cb(const char* /*name*/, size_t /*size*/, void* value) {
    auto** slot_loc = static_cast<worker_pool_slot_t**>(value);
    *slot_loc = new worker_pool_slot_t{(*slot_loc)->pool};
    return 0;
}

// Close callback: drop one slot.  shared_ptr inside the slot releases its
// reference; worker_pool_t destructor runs when the last slot is freed
// (refcount reaches 0), which stops and joins the jthreads.
inline herr_t fapl_pool_close_cb(const char* /*name*/, size_t /*size*/, void* ptr) {
    delete *static_cast<worker_pool_slot_t**>(ptr);
    return 0;
}

// Setter invoked when the user applies h5::threads{N} to an FAPL.
// Idempotent — if a pool property is already installed, leaves it untouched.
//
// n == 0 maps to std::thread::hardware_concurrency() inside worker_pool_t.
inline herr_t fapl_threads_set(::hid_t fapl, unsigned n) {
    if (H5Pexist(fapl, H5CPP_FAPL_WORKER_POOL)) return 0;
    auto* slot = new worker_pool_slot_t{
        std::make_shared<worker_pool_t>(n)
    };
    return H5Pinsert2(fapl, H5CPP_FAPL_WORKER_POOL,
        sizeof(worker_pool_slot_t*), &slot,
        nullptr,             // set
        nullptr,             // get
        nullptr,             // prp_del
        fapl_pool_copy_cb,
        nullptr,             // compare
        fapl_pool_close_cb);
}

// Consumer-site helper: given a FAPL id, retrieve the worker pool shared_ptr
// if one is installed.  Returns nullptr (= no pool, fall back to synchronous
// pipeline) if the property is absent.  Used by pt_t, h5::write, h5::read
// in Phase 1.3.
inline std::shared_ptr<worker_pool_t> resolve_worker_pool(::hid_t fapl_id) noexcept {
    if (fapl_id < 0 || H5Iis_valid(fapl_id) <= 0) return nullptr;
    if (!H5Pexist(fapl_id, H5CPP_FAPL_WORKER_POOL)) return nullptr;
    worker_pool_slot_t* slot = nullptr;
    H5Pget(fapl_id, H5CPP_FAPL_WORKER_POOL, &slot);
    return slot ? slot->pool : nullptr;
}

// ─── Back-pressure cap (separate FAPL property) ──────────────────────────────

// Setter invoked when the user applies h5::backpressure{N} to an FAPL.
// Stores a plain unsigned by memcpy semantics — no lifecycle callbacks
// needed since the value is trivially copyable and owns no heap.
//
// The cap is consumed by pt_t (and Phase 1.3's h5::write/read) when
// queueing work to the pool: write_chunk blocks on drain_completed
// once the in-flight deque reaches the cap.
inline herr_t fapl_backpressure_set(::hid_t fapl, unsigned cap) {
    if (H5Pexist(fapl, H5CPP_FAPL_BACKPRESSURE)) return 0;
    return H5Pinsert2(fapl, H5CPP_FAPL_BACKPRESSURE,
        sizeof(unsigned), &cap,
        nullptr, nullptr, nullptr,
        nullptr,            // copy: memcpy is correct for POD
        nullptr,
        nullptr);           // close: nothing to release
}

// Consumer-site helper: returns the user-set back-pressure cap, or computes
// the default (H5CPP_FAPL_BACKPRESSURE_DEFAULT_FACTOR × worker_count) when
// no h5::backpressure{N} was applied.  Returns 0 only when no pool is
// installed either — caller should already have bailed in that case.
inline unsigned resolve_backpressure(::hid_t fapl_id,
                                     unsigned worker_count) noexcept {
    if (fapl_id < 0 || H5Iis_valid(fapl_id) <= 0) return 0;
    if (H5Pexist(fapl_id, H5CPP_FAPL_BACKPRESSURE)) {
        unsigned cap = 0;
        H5Pget(fapl_id, H5CPP_FAPL_BACKPRESSURE, &cap);
        if (cap > 0) return cap;
    }
    return H5CPP_FAPL_BACKPRESSURE_DEFAULT_FACTOR * worker_count;
}

} // namespace h5::impl

namespace h5 {
// User-facing tags.  Applied to a fapl_t via the property-chain mechanism.
//
//     h5::create("data.h5", H5F_ACC_TRUNC, h5::threads{8})
//     h5::create("data.h5", H5F_ACC_TRUNC, h5::threads{})              // hw_concurrency
//     h5::create("data.h5", H5F_ACC_TRUNC, h5::threads{8}
//                                          | h5::backpressure{32})    // 8 workers, 32-chunk cap
//
// h5::backpressure{N} without h5::threads{N} is silently a no-op:
// without a pool, there is no queue to bound.  Document at user-facing
// level; do not warn at runtime.
using threads      = impl::fapl_call<impl::fapl_args<hid_t, unsigned>,
                                     impl::fapl_threads_set>;
using backpressure = impl::fapl_call<impl::fapl_args<hid_t, unsigned>,
                                     impl::fapl_backpressure_set>;
}
