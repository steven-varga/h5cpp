/*
 * Copyright (c) 2018-2026 Steven Varga / Varga Labs — MIT
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Per-file I/O collector — the SINGLE HDF5 thread for an async file.
 *
 * HDF5 (threadsafety OFF) permits only one thread to call the HDF5 C API for a
 * given file.  io_collector_t is that thread: it owns BOTH the chunk-data writes
 * (H5Dwrite_chunk) AND the metadata calls (H5Dcreate / H5Dclose / dataspace /
 * H5Fclose) for one file.  It therefore subsumes the role the inert executor_t
 * scaffold was meant to play; executor_t is retired in favour of this type.
 *
 * Two intake paths, serviced FIFO on the one worker thread:
 *   - submit_and_wait(fn)  — request/response.  Runs `fn` ON the collector
 *     thread and blocks the caller until it returns (with a reentrancy-inline
 *     fast path ported from executor_t).  Used for metadata.
 *   - io_enqueue(ds,dxpl,req) — fire-and-forget streaming chunk write with
 *     windowed back-pressure (blocks the producer only when the in-flight
 *     window is full, never per chunk).  Used for chunk data.  Many producer
 *     threads (caller + filter workers) enqueue; the one collector consumes,
 *     so the handoff is MPSC.  Requests are self-describing (carry their chunk
 *     offset) so out-of-order arrival from N filter workers is fine.
 *
 * Lifetime: created per async file at registry attach; drained + joined at the
 * last H5Fclose (registry detach, refs->0), BEFORE the file id is closed.
 *
 * NOTE (read path): the streaming queue and worker loop are written so a
 * chunk-READ request variant can be added later without touching producers.
 */
#pragma once

#include "H5config.hpp"      // H5CPP_MAX_RANK
#include "H5Pthreads.hpp"    // worker_pool_t
#include "detail/stoppable_thread.hpp"

#include <hdf5.h>

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>

namespace h5::impl {

// Who owns the bytes a chunk_write_request_t points at.
enum class ownership_t : std::uint8_t {
    pinned_source,   // `data` borrows the caller's buffer; collector frees nothing.
                     //   The producer guarantees the buffer outlives the drain of
                     //   this request (async write drains before it returns).
    arena_owned,     // `data` lives inside `hold`; dropping `hold` frees it.
};

// One chunk's worth of work for the collector.  Self-describing via `offset`,
// so the collector can write requests in any arrival order.
struct chunk_write_request_t {
    const void*                          data{nullptr};   // bytes to write (filtered or raw)
    std::size_t                          nbytes{0};       // length handed to H5Dwrite_chunk
    std::array<hsize_t, H5CPP_MAX_RANK>  offset{};        // chunk offset in the dataset
    std::uint32_t                        mask{0};         // filter mask
    ownership_t                          owner{ownership_t::pinned_source};
    std::shared_ptr<void>                hold{};          // owns the buffer when arena_owned
};

struct io_collector_t {
    // `cap` is the in-flight chunk window (back-pressure).  Clamped to >= 1.
    // `fileno` is the H5Fget_fileno key of the file this collector serves; it is
    // computed once (single-threaded) at open and cached here so the close path
    // can detach the registry without an off-thread HDF5 call.
    io_collector_t(std::shared_ptr<worker_pool_t> pool, unsigned cap, unsigned long fileno)
        : pool_(std::move(pool)),
          io_cap_(cap ? cap : 1u),
          fileno_(fileno),
          worker_([this](h5::detail::stop_token_t st) { loop(st); })
    {}

    ~io_collector_t() {
        drain();
        { std::lock_guard<std::mutex> lk(m_); stopping_ = true; }
        cv_.notify_all();
        // stoppable_thread_t dtor requests stop + joins the worker thread.
    }

    io_collector_t(const io_collector_t&)            = delete;
    io_collector_t& operator=(const io_collector_t&) = delete;
    io_collector_t(io_collector_t&&)                 = delete;
    io_collector_t& operator=(io_collector_t&&)      = delete;

    // ── metadata side ────────────────────────────────────────────────────────
    // Run `fn` on the collector thread and block until it returns.  An exception
    // in `fn` propagates back via the future.  Reentrancy: if the caller already
    // IS the collector thread, run inline (queueing would self-deadlock).
    template <typename Fn>
    auto submit_and_wait(Fn&& fn) -> std::invoke_result_t<Fn> {
        using R = std::invoke_result_t<Fn>;
        if (std::this_thread::get_id() ==
                worker_thread_id_.load(std::memory_order_acquire)) {
            return std::forward<Fn>(fn)();
        }
        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<Fn>(fn));
        auto fut  = task->get_future();
        {
            std::lock_guard<std::mutex> lk(m_);
            tasks_.emplace([task] { (*task)(); });
        }
        cv_.notify_all();
        return fut.get();   // blocks on the collector thread; rethrows
    }

    // ── streaming chunk-write side ───────────────────────────────────────────
    // Enqueue a chunk for H5Dwrite_chunk on the collector thread.  Fire and
    // forget; blocks the producer only while the in-flight window is full.
    void io_enqueue(::hid_t ds, ::hid_t dxpl, chunk_write_request_t req) {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [&] { return io_in_flight_ < io_cap_ || stopping_; });
        if (stopping_) return;   // collector shutting down — drop (shouldn't happen pre-drain)
        ++io_in_flight_;
        tasks_.emplace([this, ds, dxpl, req = std::move(req)]() mutable {
            const herr_t rc = H5Dwrite_chunk(ds, dxpl, req.mask,
                                             req.offset.data(), req.nbytes, req.data);
            req.hold.reset();   // free arena_owned buffer (no-op for pinned_source)
            {
                std::lock_guard<std::mutex> lk2(m_);
                if (rc < 0 && err_ == 0) err_ = rc;
                --io_in_flight_;
            }
            cv_.notify_all();   // wake producers waiting on the window, and drain()
        });
        lk.unlock();
        cv_.notify_all();       // wake the collector thread
    }

    // Block until every queued task has run and every streaming chunk has been
    // written + released.  Metadata submit_and_wait calls are already complete by
    // the time they return, so this just has to flush the chunk stream.
    void drain() {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [&] { return tasks_.empty() && io_in_flight_ == 0; });
    }

    // First HDF5 error captured by the collector thread (0 = none).
    [[nodiscard]] herr_t last_error() const noexcept {
        std::lock_guard<std::mutex> lk(m_);
        return err_;
    }

    [[nodiscard]] std::shared_ptr<worker_pool_t> pool() const { return pool_; }
    [[nodiscard]] unsigned long fileno() const noexcept { return fileno_; }
    [[nodiscard]] std::thread::id worker_thread_id() const noexcept {
        return worker_thread_id_.load(std::memory_order_acquire);
    }

private:
    void loop(h5::detail::stop_token_t st) {
        worker_thread_id_.store(std::this_thread::get_id(), std::memory_order_release);
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lk(m_);
                cv_.wait(lk, [&] {
                    return !tasks_.empty() || stopping_ || st.stop_requested();
                });
                if (tasks_.empty()) {
                    if (stopping_ || st.stop_requested()) return;
                    continue;
                }
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();             // metadata closure or chunk-write closure
            cv_.notify_all();   // let drain() / blocked producers re-check
        }
    }

    std::shared_ptr<worker_pool_t>    pool_;       // shared filter/compute pool
    mutable std::mutex                m_;
    std::condition_variable           cv_;
    std::queue<std::function<void()>> tasks_;      // FIFO: metadata + chunk writes
    unsigned                          io_in_flight_{0};   // chunks queued-but-not-yet-written (guarded by m_)
    unsigned                          io_cap_{1};         // back-pressure window
    unsigned long                     fileno_{0};         // H5Fget_fileno of the served file (cached at open)
    herr_t                            err_{0};            // first capture (guarded by m_)
    bool                              stopping_{false};   // (guarded by m_)
    std::atomic<std::thread::id>      worker_thread_id_{};
    h5::detail::stoppable_thread_t    worker_;     // declared LAST: all members live when loop() starts
};

// Run `op` as a single mutually-exclusive HDF5 operation.  Under H5CPP_MULTITHREAD
// this takes the process-global HDF5 lock (h5::impl::capi_lock, defined in
// H5Iall.hpp) for the whole op — HDF5 Threadsafety-OFF has lock-free global state
// (H5FL/H5CX), so the C-API must be serialized exactly as HDF5's own threadsafe
// build does.  Any thread may run the op; only one is inside HDF5 at a time, and
// nested h5cpp calls re-enter the lock for free (thread-local depth).  In a classic
// build capi_lock is a no-op, so this is a zero-overhead pass-through.
//
// (The earlier collector-THREAD model is retired: a dedicated thread can't own ALL
// HDF5 — H5_term_library runs atexit on the main thread and property-list/refcount
// calls happen on the caller — so the free-lists corrupted across threads.  io_
// collector_t above is kept dormant for a future genuine async-IO slice.)
template <class Op>
inline auto on_collector(Op&& op) -> std::invoke_result_t<Op> {
    capi_lock _lk;                       // global HDF5 lock under MT; no-op in classic builds
    return std::forward<Op>(op)();
}

} // namespace h5::impl
