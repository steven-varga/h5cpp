/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */


#ifndef  H5CPP_ACREATE_HPP
#define  H5CPP_ACREATE_HPP
namespace h5 {
	namespace impl {
		/*this template defines what HDF5 object types may have attributes */
		template <class H, class T=void> using is_valid_attr =
			std::integral_constant<bool, std::is_same<H, ::hid_t>::value ||
				std::is_same<H, h5::gr_t>::value || std::is_same<H, h5::ds_t>::value ||
				std::is_same<H, h5::ob_t>::value || std::is_same<H, h5::dt_t<T>>::value>;
		/*template <class H, class T=void> using is_valid_attr =
			std::integral_constant<bool,
				std::is_same<H, h5::ds_t>::value>;*/
	}

	template<class T, class HID_T, class... args_t> 
	inline typename std::enable_if<h5::impl::is_valid_attr<HID_T>::value,
	h5::at_t>::type create( const HID_T& parent, const std::string& path, args_t&&... args ){
		try {
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
			using element_t = typename impl::decay<T>::type;
			h5::dt_t<element_t> type;
			hid_t id = H5I_UNINIT;
			H5CPP_CHECK_NZ( (id = H5Acreate2( static_cast<hid_t>( parent ), path.c_str(),
					static_cast<hid_t>(type), static_cast<hid_t>( space ), static_cast<hid_t>( acpl ), H5P_DEFAULT )),
					h5::error::io::attribute::create, "couldn't create attribute");
			return h5::at_t{id};
		} catch( const std::runtime_error& err ) {
				throw h5::error::io::attribute::create( err.what() );
		}
	}

}
#endif

