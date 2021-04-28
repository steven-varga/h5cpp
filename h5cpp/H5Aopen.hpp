/*
 * Copyright (c) 2018 -2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_AOPEN_HPP
#define  H5CPP_AOPEN_HPP

#include <hdf5.h>
#include "H5config.hpp"
#include "H5Eall.hpp"
#include "H5Iall.hpp"
#include "H5Pall.hpp"
#include "H5Acreate.hpp"
#include <type_traits>
#include <string>

namespace h5 {
    template<class HID_T>
    inline typename std::enable_if<h5::impl::is_valid_attr<HID_T>::value,
    h5::at_t>::type aopen(const  HID_T& parent, const std::string& path, const h5::acpl_t& acpl = h5::default_acpl ){

        H5CPP_CHECK_PROP( acpl, h5::error::io::attribute::open, "invalid attribute creation property" );
        hid_t attr = H5I_UNINIT;
        H5CPP_CHECK_NZ((
            attr = H5Aopen( static_cast<hid_t>(parent), path.c_str(), static_cast<hid_t>(acpl))),
                                            h5::error::io::attribute::open, "can't open attribute..." );
        return  h5::at_t{attr};
    }
}
#endif

