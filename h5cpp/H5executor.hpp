/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

// Phase II executor — one thread per async fd.  Every HDF5 C-API call on an
// h5::async::* descriptor is serialized through this thread via
// submit_and_wait, restoring thread-safety for HDF5 builds compiled without
// --enable-threadsafe.  Compression continues to parallelize through the
// Phase I worker_pool_t held by reference.
//
// Construction model mirrors Phase I's worker_pool_t (H5Pthreads.hpp): single
// mutex-guarded queue + doorbell + stoppable_thread_t, std::packaged_task in
// shared_ptr wrapped in std::function<void()> for type erasure.  The only
// substantive difference is the single worker thread and the sync wait
// facade.

#include "H5Pthreads.hpp"   // worker_pool_t

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>

#include "detail/doorbell.hpp"
#include "detail/stoppable_thread.hpp"

namespace h5::impl {

struct executor_t {
    // pool is optional today; Phase II operation overloads (PR-B) will
    // require it.  Default-construct keeps the scaffold trivially usable
    // in lifecycle tests that don't exercise compression.
    explicit executor_t(std::shared_ptr<worker_pool_t> pool = nullptr)
        : pool_(std::move(pool)),
          worker_([this](h5::detail::stop_token_t st) { worker_loop(st); })
    {}

    ~executor_t() {
        wait_idle();
        stopping_.store(true, std::memory_order_release);
        bell_.ring();
        // stoppable_thread_t dtor requests stop + joins
    }

    executor_t(const executor_t&)            = delete;
    executor_t& operator=(const executor_t&) = delete;
    executor_t(executor_t&&)                 = delete;
    executor_t& operator=(executor_t&&)      = delete;

    // Submit a callable and block until it completes on the executor thread.
    // Exception in the callable propagates back to the submitting thread via
    // future::get.
    //
    // Reentrant safety: if the caller is already running on the executor
    // thread (e.g. one h5cpp operation calling another inside the executor),
    // run the callable inline rather than queueing — queueing would
    // deadlock the executor on itself.
    template <typename Fn>
    auto submit_and_wait(Fn&& fn) -> std::invoke_result_t<Fn> {
        using R = std::invoke_result_t<Fn>;

        if (std::this_thread::get_id() ==
                worker_thread_id_.load(std::memory_order_acquire)) {
            // Already on the executor thread — run inline, no queue trip.
            // Exception thrown in fn propagates back to caller naturally
            // (no packaged_task wrap needed in this branch).
            return std::forward<Fn>(fn)();
        }

        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<Fn>(fn));
        auto fut  = task->get_future();
        in_flight_.fetch_add(1, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lk(m_);
            tasks_.emplace([task] { (*task)(); });
        }
        bell_.ring();
        // Decrement counter on the submitter's path, AFTER fut.get()
        // unblocks (causal: the worker has completed task() because the
        // future is ready).  RAII guard ensures the decrement also fires
        // when fut.get() rethrows.  This ties in_flight_ to "submit_and_wait
        // is in flight on this thread" rather than "task is still queued"
        // — tests that check in_flight() right after the call now see 0
        // deterministically, no race with the worker thread.
        struct decrement_on_exit {
            std::atomic<int>& counter;
            ~decrement_on_exit() { counter.fetch_sub(1, std::memory_order_release); }
        } guard{in_flight_};
        return fut.get();   // blocks on executor; rethrows exception
    }

    // Block until the queue drains.  Called by ~executor_t before stop.
    void wait_idle() {
        while (in_flight_.load(std::memory_order_acquire) > 0)
            std::this_thread::yield();
    }

    [[nodiscard]] std::shared_ptr<worker_pool_t> pool() const { return pool_; }
    [[nodiscard]] int  in_flight()        const noexcept { return in_flight_.load(std::memory_order_acquire); }
    [[nodiscard]] std::thread::id worker_thread_id() const noexcept {
        return worker_thread_id_.load(std::memory_order_acquire);
    }

private:
    void worker_loop(h5::detail::stop_token_t st) {
        worker_thread_id_.store(std::this_thread::get_id(),
                                std::memory_order_release);
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
                    last_seq = bell_.load();
                }
            }

            if (got_task) {
                try { task(); } catch (...) { /* packaged_task captures it */ }
                // in_flight_ is decremented by submit_and_wait on the
                // submitter's path (see RAII guard there), not here.
                // Worker just executes the task; the counter belongs to
                // the submission's lifetime.
                continue;
            }

            if (st.stop_requested() || stopping_.load(std::memory_order_acquire)) return;
            bell_.wait(last_seq);
        }
    }

    std::shared_ptr<worker_pool_t>      pool_;
    std::mutex                          m_;
    std::queue<std::function<void()>>   tasks_;
    h5::detail::doorbell_t              bell_;
    std::atomic<int>                    in_flight_{0};
    std::atomic<bool>                   stopping_{false};
    std::atomic<std::thread::id>        worker_thread_id_{};
    h5::detail::stoppable_thread_t      worker_;     // declared LAST so all
                                                     // members are live when
                                                     // worker_loop starts
};

} // namespace h5::impl
