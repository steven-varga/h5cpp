/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

// Phase II FAPL executor property — same shared_ptr-in-slot pattern proven
// by Phase I's worker pool (H5Pthreads.hpp).  An executor_t lives behind
// a shared_ptr stored in a heap-allocated holder whose address occupies
// the H5Pinsert2 value slot; copy_cb clones the slot but aliases the same
// executor (refcount +1), close_cb drops one slot (refcount -1).  When
// the last fd dies, the executor destructor joins the worker thread.
//
// h5::async::create / h5::async::open install this property in addition
// to (and consuming) any h5::threads{N} pool the user chained in.  If no
// h5::threads{N} is present, fapl_async_set auto-installs a default-sized
// pool so the executor always has a pool reference.

#include "H5Pthreads.hpp"     // worker_pool_t + fapl_threads_set + resolve_worker_pool
#include "H5executor.hpp"     // executor_t

#include <hdf5.h>

#include <memory>

namespace h5::impl {

#define H5CPP_FAPL_EXECUTOR "h5cpp_fapl_executor"

// Heap-allocated holder, parallel to worker_pool_slot_t in H5Pthreads.hpp.
struct executor_slot_t {
    std::shared_ptr<executor_t> exec;
};

// Copy: HDF5 memcpy'd the slot pointer into the destination.  Allocate a
// fresh holder whose shared_ptr aliases the same executor (++refcount).
inline herr_t fapl_exec_copy_cb(const char* /*name*/, size_t /*size*/, void* value) {
    auto** slot_loc = static_cast<executor_slot_t**>(value);
    *slot_loc = new executor_slot_t{(*slot_loc)->exec};
    return 0;
}

// Close: delete one holder; shared_ptr drops one reference.  The
// executor_t destructor (which joins the worker thread) runs when the
// last reference is released.
inline herr_t fapl_exec_close_cb(const char* /*name*/, size_t /*size*/, void* ptr) {
    delete *static_cast<executor_slot_t**>(ptr);
    return 0;
}

// Setter invoked by h5::async::create / h5::async::open.  Idempotent —
// if the FAPL already has the property installed, leaves it alone.
// Auto-installs a default-sized worker_pool_t (n=0) when no h5::threads{N}
// was chained into the FAPL, so the executor always has a pool to hand
// to compression callbacks (Phase II PR-B wires that connection).
inline herr_t fapl_async_set(::hid_t fapl) {
    if (H5Pexist(fapl, H5CPP_FAPL_EXECUTOR)) return 0;

    auto pool = resolve_worker_pool(fapl);
    if (!pool) {
        // No h5::threads{N} in the chain — install a default-sized pool
        // so the executor has a compression backend available.
        fapl_threads_set(fapl, 0);   // 0 → hardware_concurrency()
        pool = resolve_worker_pool(fapl);
    }

    auto* slot = new executor_slot_t{
        std::make_shared<executor_t>(std::move(pool))
    };
    return H5Pinsert2(fapl, H5CPP_FAPL_EXECUTOR,
        sizeof(executor_slot_t*), &slot,
        nullptr,             // set
        nullptr,             // get
        nullptr,             // prp_del
        fapl_exec_copy_cb,
        nullptr,             // compare
        fapl_exec_close_cb);
}

// Consumer-site: given a FAPL id, retrieve the executor shared_ptr if one
// is installed.  Returns nullptr when the property is absent (classic-mode
// FAPL — caller should not have been routed here).
inline std::shared_ptr<executor_t> resolve_executor(::hid_t fapl_id) noexcept {
    if (fapl_id < 0 || H5Iis_valid(fapl_id) <= 0) return nullptr;
    if (!H5Pexist(fapl_id, H5CPP_FAPL_EXECUTOR)) return nullptr;
    executor_slot_t* slot = nullptr;
    H5Pget(fapl_id, H5CPP_FAPL_EXECUTOR, &slot);
    return slot ? slot->exec : nullptr;
}

} // namespace h5::impl
