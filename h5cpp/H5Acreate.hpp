/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */


#ifndef  H5CPP_ACREATE_HPP
#define  H5CPP_ACREATE_HPP
namespace h5 {

	template<class T, class... args_t>
	h5::at_t create( const h5::ds_t& ds, const std::string& path, args_t&&... args ) try {
		// compile time check of property lists: 
		using tcurrent_dims = typename arg::tpos<const h5::current_dims_t&,const args_t&...>;
		using tacpl 		= typename arg::tpos<const h5::acpl_t&,const args_t&...>;

		h5::acpl_t default_acpl{ H5Pcreate(H5P_ATTRIBUTE_CREATE) };
		const h5::acpl_t& acpl = arg::get(default_acpl, args...);

		H5CPP_CHECK_PROP( acpl, h5::error::property_list::misc, "invalid attribute create property" );

		// and dimensions
		h5::current_dims_t current_dims_default{0}; // if no current dims_present 
		// this mutable value will be referenced
		const h5::current_dims_t& current_dims = arg::get(current_dims_default, args...);
		// no partial IO or chunks
		h5::sp_t space = h5::create_simple( current_dims );
		using element_t = impl::decay_t<T>;
		h5::dt_t<element_t> type;
		hid_t id;
	   	H5CPP_CHECK_NZ( (id = H5Acreate2( static_cast<hid_t>( ds ), path.c_str(),
				static_cast<hid_t>(type), static_cast<hid_t>( space ), static_cast<hid_t>( acpl ), H5P_DEFAULT )),
				h5::error::io::attribute::create, "couldn't create attribute");
		return h5::at_t{id};
	} catch( const std::runtime_error& err ) {
			throw h5::error::io::attribute::create( err.what() );
	}

}
#endif

