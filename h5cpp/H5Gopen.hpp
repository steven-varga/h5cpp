/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include "H5Gcreate.hpp"
#include <string>

namespace h5 {
	/** @ingroup group-io
	 * @brief Opens an existing HDF5 group and returns a managed h5::gr_t handle.
	 *
	 * @code
	 * h5::gr_t gr = h5::gopen(fd, "/sensors/imu");
	 * h5::awrite(gr, "sample_rate", 200.0);
	 * @endcode
	 */
	template<class HID_T>
	inline std::enable_if_t<h5::impl::is_valid_group_parent<HID_T>::value, h5::gr_t>
	gopen( const HID_T& parent, const std::string& path ) {
		hid_t id;
		H5CPP_CHECK_NZ(
			(id = H5Gopen2( static_cast<hid_t>(parent), path.c_str(), H5P_DEFAULT )),
			h5::error::io::group::open, h5::error::msg::open_group );
		return h5::gr_t{id};
	}
}
