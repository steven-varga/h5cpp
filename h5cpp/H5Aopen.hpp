/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */


#ifndef  H5CPP_AOPEN_HPP
#define  H5CPP_AOPEN_HPP
namespace h5{
    inline h5::at_t open(const  h5::ds_t& ds, const std::string& path, const h5::acpl_t& acpl = h5::default_acpl ){

		H5CPP_CHECK_PROP( acpl, h5::error::io::attribute::open, "invalid attribute creation property" );
		hid_t attr;
	   	H5CPP_CHECK_NZ((
			attr = H5Aopen( static_cast<hid_t>(ds), path.c_str(), static_cast<hid_t>(acpl))),
				   							h5::error::io::attribute::open, "can't open attribute..." );
     	return  h5::at_t{attr};
    }
}
#endif

