/*
 * Copyright (c) 2018 - 2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef  H5CPP_GOPEN_HPP
#define  H5CPP_GOPEN_HPP

#include <hdf5.h>
#include "H5config.hpp"
#include "compat.hpp"
#include "H5Eall.hpp"
#include "H5Iall.hpp"
#include "H5Pall.hpp"

#include <type_traits>
#include <string>

namespace h5 {

    template<class L>
    inline typename std::enable_if<impl::is_location<L>::value,
    h5::gr_t>::type gopen(const L& parent, const std::string& path, const h5::gapl_t& gapl = h5::default_gapl ){

        H5CPP_CHECK_PROP( gapl, h5::error::io::group::open, "invalid group access property" );
        hid_t gr = H5I_UNINIT;
        H5CPP_CHECK_NZ((
            gr = H5Gopen( static_cast<hid_t>(parent), path.c_str(), static_cast<hid_t>(gapl))),
                                            h5::error::io::group::open, "can't open group..." );
        return  h5::gr_t{gr};
    }

    template<class L, class... args_t>
    inline typename std::enable_if<impl::is_location<L>::value,
    h5::gr_t>::type gcreate(const L& parent, const std::string& path, args_t&&... args ){
        using tlcpl         = typename arg::tpos<const h5::lcpl_t&,const args_t&...>;
        using tgcpl         = typename arg::tpos<const h5::gcpl_t&,const args_t&...>;
        using tgapl         = typename arg::tpos<const h5::gapl_t&,const args_t&...>;

        const h5::lcpl_t& lcpl = arg::get(h5::default_lcpl, args...);
        const h5::gcpl_t& gcpl = arg::get(h5::default_gcpl, args...);
        const h5::gapl_t& gapl = arg::get(h5::default_gapl, args...);

        H5CPP_CHECK_PROP( lcpl, h5::error::io::group::create, "invalid list creation property" );
        H5CPP_CHECK_PROP( gcpl, h5::error::io::group::create, "invalid group creation property" );
        H5CPP_CHECK_PROP( gapl, h5::error::io::group::create, "invalid group access property" );
        hid_t gr = H5I_UNINIT;
        H5CPP_CHECK_NZ((
            gr = H5Gcreate( static_cast<hid_t>(parent), path.c_str(),
                static_cast<hid_t>(lcpl), static_cast<hid_t>(gcpl), static_cast<hid_t>(gapl)  )),
                                            h5::error::io::group::create, "can't create group..." );
        return  h5::gr_t{gr};
    }
}
#endif

