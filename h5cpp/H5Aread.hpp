/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_AREAD_HPP
#define  H5CPP_AREAD_HPP
namespace h5 {

	//ARITHMETIC ELEMENT TYPES 
	template <class T,  class HID_T, class... args_t> inline
	typename std::enable_if< impl::is_arithmetic<T>::value && impl::attr::is_location<HID_T>::value,
		T>::type aread( const HID_T& ds, const std::string& name, const h5::acpl_t& acpl = h5::default_acpl ){

		h5::at_t attr = h5::aopen(ds, name, h5::default_acpl);
		hid_t id;
		H5CPP_CHECK_NZ( (id = H5Aget_space( static_cast<hid_t>(attr) )),
			   h5::error::io::attribute::read, "couldn't get space...");
		h5::sp_t file_space{id};
		h5::current_dims_t current_dims;

		int rank = get_simple_extent_dims(file_space, current_dims );
		using element_t = typename impl::decay<T>::type;
		h5::dt_t<element_t> type;
		T object = impl::get<T>::ctor( current_dims );
		element_t *ptr = const_cast<element_t*>( impl::data( object ));
		H5CPP_CHECK_NZ( H5Aread( static_cast<hid_t>(attr), static_cast<hid_t>(type), ptr ),
			   h5::error::io::attribute::read, "couldn't read attribute ...");
		return object;
	}

	//POD ELEMENT TYPES: Rank 0 and rank > 0 btw: strings are not pods 
	template <class T, class D=typename impl::decay<T>::type, class HID_T, class... args_t> inline
	typename std::enable_if< impl::is_pod<T>::value && impl::attr::is_location<HID_T>::value,
	T>::type aread( const HID_T& ds, const std::string& name, const h5::acpl_t& acpl = h5::default_acpl ){
		h5::at_t attr = h5::aopen(ds, name, h5::default_acpl);
		hid_t id;
		H5CPP_CHECK_NZ( (id = H5Aget_space( static_cast<hid_t>(attr) )),
			   h5::error::io::attribute::read, "couldn't get space...");
		h5::sp_t file_space{id};
		h5::current_dims_t current_dims;
		int rank = get_simple_extent_dims(file_space, current_dims );
		using element_t = typename impl::decay<T>::type;
		h5::dt_t<element_t> type;
		if( !rank ){ // FIXME: write impl::get<pod_type>(...)
			T object;
			H5CPP_CHECK_NZ( H5Aread( static_cast<hid_t>(attr), static_cast<hid_t>(type), &object ),
				   h5::error::io::attribute::read, "couldn't read attribute ...");
			return object;
		}else{ // we are dealing with rank 0 or single object
			T object = impl::get<T>::ctor( current_dims );
			element_t *ptr = const_cast<element_t*>( impl::data( object ));
			H5CPP_CHECK_NZ( H5Aread( static_cast<hid_t>(attr), static_cast<hid_t>(type), ptr ),
				   h5::error::io::attribute::read, "couldn't read attribute ...");
			return object;
		}
	}

	// STD::STRING: SCALAR
	template <class T, class HID_T, class... args_t> inline
	typename std::enable_if<impl::is_string<T>::value && impl::is_scalar<T>::value && h5::impl::is_valid_attr<HID_T>::value, //TODO: add char**
	T>::type aread( const HID_T& ds, const std::string& name, const h5::acpl_t& acpl = h5::default_acpl ){
		h5::at_t attr = h5::aopen(ds, name, h5::default_acpl);
		h5::dt_t<char*> type; // gets variable length string
		char* ptr;
		H5CPP_CHECK_NZ( H5Aread( static_cast<hid_t>(attr), static_cast<hid_t>(type), &ptr ),
			   h5::error::io::attribute::read, "couldn't read attribute ...");
		return std::string(ptr);
	}

	// STD::STRING: NOT SCALAR
	template <class T, class HID_T, class... args_t> inline
	typename std::enable_if<impl::is_string<T>::value && !impl::is_scalar<T>::value && h5::impl::is_valid_attr<HID_T>::value, //TODO: add char**
	T>::type aread( const HID_T& ds, const std::string& name, const h5::acpl_t& acpl = h5::default_acpl ){
		h5::at_t attr = h5::aopen(ds, name, h5::default_acpl);
		hid_t id;
		H5CPP_CHECK_NZ( (id = H5Aget_space( static_cast<hid_t>(attr) )),
			   h5::error::io::attribute::read, "couldn't get space...");
		h5::sp_t file_space{id};
		h5::current_dims_t current_dims;
		int rank = get_simple_extent_dims(file_space, current_dims );
		using element_t = typename impl::decay<T>::type;
		h5::dt_t<char*> type;
		T object = impl::get<T>::ctor( current_dims );
		size_t nelem = impl::nelements(current_dims);
		char** ptr = static_cast<char**>( malloc ( nelem * sizeof (char *)));
		H5CPP_CHECK_NZ( H5Aread( static_cast<hid_t>(attr), static_cast<hid_t>(type), ptr ),
			   h5::error::io::attribute::read, "couldn't read attribute ...");
		for( size_t i=0; i<nelem; i++)
			if( ptr[i] != NULL ) object[i] = std::string( ptr[i] );
        free(ptr);
		return object;
	}

}
#endif

