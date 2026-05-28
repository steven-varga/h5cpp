/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include "H5Acreate.hpp"
#include <string>
#include <type_traits>

// Return-type macro for h5::adelete(parent, name). See H5Acreate.hpp.
#ifdef H5CPP_DOXYGEN
#  define H5CPP_ATTR_DELETE_RET(hid_t) void
#else
#  define H5CPP_ATTR_DELETE_RET(hid_t) std::enable_if_t<h5::impl::is_valid_attr<hid_t>::value, void>
#endif

namespace h5 {
    /**
     * \func_attr_hdr
     * @brief Delete an attribute by name from a parent HDF5 object.
     *
     * Removes the named attribute from the on-disk representation of
     * `parent`. The operation is final — there is no rollback once
     * `H5Adelete` has succeeded.
     *
     * @param parent  open parent handle: raw `::hid_t`, `h5::gr_t`, `h5::ds_t`, `h5::ob_t`, or `h5::dt_t<T>` — enforced at compile
     *                time via `h5::impl::is_valid_attr`. Typed `h5::fd_t` is **not** accepted directly; pass `static_cast<::hid_t>(fd)`.
     * @param name    attribute name (UTF-8); resolved relative to `parent`.
     *
     * @tparam hid_t  deduced from the `parent` argument; must satisfy `h5::impl::is_valid_attr<hid_t>::value`.
     * @return void — failures are reported by throwing.
     *
     * @throws h5::error::io::attribute::delete_  on `H5Adelete` failure (attribute does not exist, parent invalid, write access denied).
     *
     * <br/><b>example:</b>
     * @code
     * h5::ds_t ds = h5::open(fd, "/grid/data");
     * h5::adelete(ds, "stale_marker");
     * @endcode
     *
     * \sa_h5cpp
     * \sa_hdf5
     * @sa h5::create h5::open @ref link_handle_reference
     *     "Handles, Descriptors, and Property Lists"
     */
    template<class hid_t>
    inline H5CPP_ATTR_DELETE_RET(hid_t)
    adelete(const hid_t& parent, const std::string& name) {
        H5CPP_CHECK_NZ(
            H5Adelete(static_cast<::hid_t>(parent), name.c_str()),
            h5::error::io::attribute::delete_,
            "couldn't delete attribute");
    }
}

