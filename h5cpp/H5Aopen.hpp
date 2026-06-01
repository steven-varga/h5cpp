/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include "H5Acreate.hpp"
#include <string>
#include <type_traits>

// Return-type macro for h5::open(parent, path, acpl) attribute overload.
// Real compile: SFINAE-constrained h5::impl::attr_parent_t<hid_t> alias.
// Doxygen     : plain h5::at_t. See H5Acreate.hpp for the rationale.
#ifdef H5CPP_DOXYGEN
#  define H5CPP_ATTR_OPEN_RET(hid_t) h5::at_t
#else
#  define H5CPP_ATTR_OPEN_RET(hid_t) h5::impl::attr_parent_t<hid_t>
#endif

namespace h5 {
	/**
	 * \func_attr_hdr
	 * @brief Open an existing attribute by name on a parent HDF5 object.
	 *
	 * Lookup is by attribute name relative to `parent`. The returned
	 * `h5::at_t` is RAII-managed; the underlying CAPI handle is closed
	 * via `H5Aclose` on scope exit. Use @ref h5::aread to retrieve the
	 * value, or pass the handle to `h5::awrite` overloads that take an
	 * already-open attribute.
	 *
	 * @param parent  open parent handle: raw `::hid_t`, `h5::gr_t`, `h5::ds_t`, `h5::ob_t`, or `h5::dt_t<T>` — enforced at compile
	 *                time via `h5::impl::is_valid_attr`. Typed `h5::fd_t` is **not** accepted directly; pass `static_cast<::hid_t>(fd)`.
	 * @param path    attribute name (UTF-8); resolved relative to `parent`.
	 * @param acpl    attribute creation property list (`h5::acpl_t`); defaults to `h5::default_acpl`.
	 *
	 * @tparam hid_t  deduced from the `parent` argument; must satisfy `h5::impl::is_valid_attr<hid_t>::value`.
	 * @return `h5::at_t` RAII handle owning the opened attribute id; closes automatically via `H5Aclose` on scope exit. Throws on failure.
	 *
	 * @throws h5::error::io::attribute::open  on `H5Aopen` failure (attribute
	 *         not present, parent does not exist, invalid acpl).
	 *
	 * <br/><b>example:</b>
	 * @code
	 * h5::fd_t fd = h5::open("file.h5", H5F_ACC_RDONLY);
	 * h5::ds_t ds = h5::open(fd, "/grid/data");
	 * h5::at_t att = h5::open(ds, "spacing");
	 * auto axes = h5::aread<std::vector<float>>(ds, "spacing");
	 * @endcode
	 *
	 * \sa_h5cpp
	 * \sa_hdf5
	 * @sa h5::aread h5::awrite @ref link_handle_reference
	 *     "Handles, Descriptors, and Property Lists"
	 */
	template<class hid_t>
	inline H5CPP_ATTR_OPEN_RET(hid_t)
	open(const hid_t& parent, const std::string& path, const h5::acpl_t& acpl = h5::default_acpl ){

		H5CPP_CHECK_PROP( acpl, h5::error::io::attribute::open, "invalid attribute creation property" );
		::hid_t attr = H5I_UNINIT;
	   	H5CPP_CHECK_NZ(( attr = H5Aopen( static_cast<::hid_t>(parent),
			path.c_str(), static_cast<::hid_t>(acpl))), h5::error::io::attribute::open, "can't open attribute..." );
     	return  h5::at_t{attr};
    }
}
