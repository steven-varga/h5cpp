/*
 * Copyright (c) 2018-2026 Steven Varga / Varga Labs — MIT
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * on_collector — route a single HDF5 operation through the process-global lock.
 *
 * HDF5 (threadsafety OFF) has lock-free global state (H5FL free-lists, H5CX
 * context), so the C-API must be serialized exactly as HDF5's own threadsafe
 * build does.  Under H5CPP_MULTITHREAD on_collector takes the process-global
 * recursive HDF5 mutex (h5::impl::capi_lock, defined in H5Iall.hpp) around the
 * op; in a classic build capi_lock is a no-op, so on_collector is a zero-overhead
 * pass-through.
 */
#pragma once

#include <type_traits>

namespace h5::impl {

// Run `op` as a single mutually-exclusive HDF5 operation.  Under H5CPP_MULTITHREAD
// this takes the process-global HDF5 lock (h5::impl::capi_lock, defined in
// H5Iall.hpp) for the whole op — HDF5 Threadsafety-OFF has lock-free global state
// (H5FL/H5CX), so the C-API must be serialized exactly as HDF5's own threadsafe
// build does.  Any thread may run the op; only one is inside HDF5 at a time, and
// nested h5cpp calls re-enter the lock for free (thread-local depth).  In a classic
// build capi_lock is a no-op, so this is a zero-overhead pass-through.
//
// (An earlier design routed HDF5 through a dedicated collector THREAD; that model
// is retired.  A single thread can't own ALL HDF5 — H5_term_library runs atexit on
// the main thread and property-list/refcount calls happen on the caller — so the
// free-lists corrupted across threads.  The process-global lock here lets ANY thread
// run the op while keeping exactly one inside HDF5 at a time.)
template <class Op>
inline auto on_collector(Op&& op) -> std::invoke_result_t<Op> {
    capi_lock _lk;                       // global HDF5 lock under MT; no-op in classic builds
    return std::forward<Op>(op)();
}

} // namespace h5::impl
