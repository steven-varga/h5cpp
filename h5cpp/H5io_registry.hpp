/*
 * Copyright (c) 2018-2026 Steven Varga / Varga Labs — MIT
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * MT close path.
 *
 * The #286 per-file worker-pool registry was retired in #287: parallelism moved
 * to a per-dataset DAPL property (h5::threads{N}, which survives the
 * H5Dget_access_plist round-trip) backed by one process-global pool, so there is
 * no per-file state to register, resolve, or detach.  Only the global-lock close
 * helper remains here.
 */
#pragma once

#include "H5Pthreads.hpp"   // worker_pool_t / global_pool / DAPL parallelism helpers

#include <hdf5.h>

namespace h5::impl {

#ifdef H5CPP_MULTITHREAD
// close_global — close a conversion-off descriptor under the global HDF5 lock.
//
// Every HDF5 close on a conversion-off handle runs while holding the one global
// HDF5 lock (capi_lock), so it never races another thread's C-API call.  The
// H5Iis_valid check is done INSIDE the lock — never an unlocked C-API call.
// Defined here because capi_lock / capi_close_t come from H5Iall.hpp, which
// includes this header after defining them.
inline void close_global(::hid_t handle, capi_close_t capi_close) {
    if (handle <= 0) return;
    capi_lock _lk;                 // serialize with every other HDF5 C-API call
    if (!H5Iis_valid(handle)) return;
    capi_close(handle);            // the actual close — under the lock
}
#endif

} // namespace h5::impl
