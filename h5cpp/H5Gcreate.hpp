/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include "H5Pall.hpp"
#include <string>

namespace h5 {
	namespace impl {
		template<class H> using is_valid_group_parent =
			std::bool_constant<std::is_same_v<H, h5::fd_t> ||
			                   std::is_same_v<H, h5::gr_t> ||
			                   std::is_same_v<H, ::hid_t>>;
	}

	/** @ingroup group-io
	 * @brief Creates an HDF5 group and returns a managed h5::gr_t handle.
	 *
	 * Accepts a file or group as parent. The optional @p lcpl controls link
	 * creation semantics (character encoding, intermediate path creation).
	 * h5::default_lcpl enables UTF-8 encoding and intermediate group creation,
	 * which is almost always the right default.
	 *
	 * @code
	 * h5::gr_t gr = h5::gcreate(fd, "sensors/imu");          // intermediate path created
	 * h5::gr_t sub = h5::gcreate(gr, "channel_0");            // nested under existing group
	 * @endcode
	 */
	template<class HID_T>
	inline std::enable_if_t<h5::impl::is_valid_group_parent<HID_T>::value, h5::gr_t>
	gcreate( const HID_T& parent, const std::string& path,
	         const h5::lcpl_t& lcpl = h5::default_lcpl ) {
		H5CPP_CHECK_PROP( lcpl, h5::error::io::group::create, "invalid link creation property list" );
		hid_t id;
		H5CPP_CHECK_NZ(
			(id = H5Gcreate2( static_cast<hid_t>(parent), path.c_str(),
			                  static_cast<hid_t>(lcpl), H5P_DEFAULT, H5P_DEFAULT )),
			h5::error::io::group::create, h5::error::msg::create_group );
		return h5::gr_t{id};
	}
}
