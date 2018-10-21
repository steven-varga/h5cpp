/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_AREAD_HPP
#define  H5CPP_AREAD_HPP
namespace h5 {

	// general case but not: {std::string, std::initializer_list<T>} 
	template <class T, class... args_t> inline
	T aread( const h5::ds_t& ds, const std::string& name, const h5::acpl_t& acpl = h5::default_acpl ){

		h5::at_t attr = h5::open(ds, name, h5::default_acpl);
		hid_t id;
		H5CPP_CHECK_NZ( (id = H5Aget_space( static_cast<hid_t>(attr) )),
			   h5::error::io::attribute::read, "couldn't get space...");
		h5::sp_t file_space{id};
		h5::current_dims_t current_dims;
		int rank = get_simple_extent_dims(file_space, current_dims );
		using element_t = typename impl::decay<T>::type;
		h5::dt_t type{ utils::h5type<element_t>() };
		T object = impl::get<T>::ctor( current_dims );
		H5CPP_CHECK_NZ( H5Aread( static_cast<hid_t>(attr), static_cast<hid_t>(type), impl::data( object ) ),
			   h5::error::io::attribute::read, "couldn't read dataset ...");

		return object;
	}
}
#endif

