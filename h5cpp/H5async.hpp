/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

// Phase II user-facing factories — h5::async::create / h5::async::open.
// These are the only call sites where the word "async" appears in user
// code; every downstream operation (h5::write, h5::read, etc.) deduces
// async-ness from the FD type via TAD and dispatches through the
// executor with `if constexpr (is_async_v<FD>)`.
//
// Mechanism:
//   1. Resolve / install a worker_pool_t on a local FAPL clone so the
//      executor has a compression backend (Phase II PR-B will wire that
//      connection through h5::write / h5::read).
//   2. Construct the executor with that pool.
//   3. Call H5Fcreate / H5Fopen on the user's FAPL (unchanged).
//   4. Wrap the resulting ::hid_t plus the executor shared_ptr in the
//      async fd_t.  The executor lives as long as any descriptor
//      derived from this fd holds the shared_ptr.
//
// We deliberately don't rely on H5Pinsert2 + H5Fget_access_plist for
// the executor handoff — HDF5 1.10.9 strips user properties from the
// FAPL retrieved off a file id, which would make the executor
// unreachable on the read path.  Storing it on the wrapper sidesteps
// that limitation.

#include "H5Pall.hpp"
#include "H5Pthreads.hpp"      // worker_pool_t + h5::threads + resolve_worker_pool
#include "H5executor.hpp"      // executor_t — complete type required at make_shared

#include <hdf5.h>
#include <memory>
#include <string>

namespace h5::async {

namespace impl_detail {

// Build the executor that backs an async fd.  Reuses an h5::threads{N}
// pool from the user's FAPL chain when present, otherwise stands up a
// default-sized pool on a freshly-created FAPL (we don't try to
// H5Pcopy the user's FAPL because H5P_DEFAULT — the common case — is
// not a copyable property list).
inline std::shared_ptr<h5::impl::executor_t>
make_executor_for(const h5::fapl_t& fapl) {
    auto pool = h5::impl::resolve_worker_pool(static_cast<::hid_t>(fapl));
    if (!pool) {
        h5::fapl_t scratch{H5Pcreate(H5P_FILE_ACCESS)};
        h5::impl::fapl_threads_set(static_cast<::hid_t>(scratch), 0);
        pool = h5::impl::resolve_worker_pool(static_cast<::hid_t>(scratch));
    }
    return std::make_shared<h5::impl::executor_t>(std::move(pool));
}

} // namespace impl_detail

inline h5::async::fd_t create(const std::string& path, unsigned flags,
                              const h5::fcpl_t& fcpl = h5::default_fcpl,
                              const h5::fapl_t& fapl = h5::default_fapl) {
    H5CPP_CHECK_PROP(fcpl, h5::error::io::file::create, "invalid file control property list");
    H5CPP_CHECK_PROP(fapl, h5::error::io::file::create, "invalid file access property list");

    auto exec = impl_detail::make_executor_for(fapl);

    hid_t fd;
    H5CPP_CHECK_NZ(
        (fd = H5Fcreate(path.data(), flags,
                        static_cast<::hid_t>(fcpl),
                        static_cast<::hid_t>(fapl))),
        h5::error::io::file::create, h5::error::msg::create_file);
    return h5::async::fd_t{fd, std::move(exec)};
}

inline h5::async::fd_t open(const std::string& path, unsigned flags,
                            const h5::fapl_t& fapl = h5::default_fapl) {
    H5CPP_CHECK_PROP(fapl, h5::error::io::file::open, "invalid file access property list");

    auto exec = impl_detail::make_executor_for(fapl);

    hid_t fd;
    H5CPP_CHECK_NZ(
        (fd = H5Fopen(path.data(), flags, static_cast<::hid_t>(fapl))),
        h5::error::io::file::open, h5::error::msg::open_file);
    return h5::async::fd_t{fd, std::move(exec)};
}

} // namespace h5::async
