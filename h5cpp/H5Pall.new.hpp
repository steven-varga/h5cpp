/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_PALL_HPP
#define  H5CPP_PALL_HPP
/** impl::property_group<H5Pset_property, args...> args... ::= all argument types except the first `this` hid_t prop_ID
 *  since it is implicitly passed by template class. 
 */ 

namespace h5 {
// FILE CREATION PROPERTY LISTS
using userblock                = impl::fcpl_call< impl::fcpl_args<hid_t,hsize_t>, H5Pset_userblock>;
using file_space_page_size     = impl::fcpl_call< impl::fcpl_args<hid_t,hsize_t>,H5Pset_file_space_page_size>;

}


namespace h5 {
	const static h5::dapl_t default_dapl = static_cast<h5::dapl_t>( H5P_DEFAULT );
	const static h5::dcpl_t default_dcpl = static_cast<h5::dcpl_t>( H5P_DEFAULT );
	const static h5::dxpl_t default_dxpl = static_cast<h5::dxpl_t>( H5P_DEFAULT );
	const static h5::lcpl_t default_lcpl = static_cast<h5::lcpl_t>( H5P_DEFAULT );
	const static h5::fapl_t default_fapl = static_cast<h5::fapl_t>( H5P_DEFAULT );
	const static h5::fcpl_t default_fcpl = static_cast<h5::fcpl_t>( H5P_DEFAULT );
}

#endif

