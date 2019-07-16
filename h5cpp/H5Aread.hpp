/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_AREAD_HPP
#define  H5CPP_AREAD_HPP
namespace h5 {

	//ARITHMETIC ELEMENT TYPES 
	template <class T, class D=typename impl::decay<T>::type, class... args_t> inline
	typename std::enable_if< std::is_integral<D>::value || std::is_floating_point<D>::value,
	T>::type aread( const h5::ds_t& ds, const std::string& name, const h5::acpl_t& acpl = h5::default_acpl ){

		h5::at_t attr = h5::open(ds, name, h5::default_acpl);
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
			   h5::error::io::attribute::read, "couldn't read dataset ...");
		return object;
	}

	//POD ELEMENT TYPES: Rank 0 and rank > 0
	template <class T, class D=typename impl::decay<T>::type, class... args_t> inline
	typename std::enable_if< !std::is_arithmetic<D>::value && std::is_pod<D>::value,
	T>::type aread( const h5::ds_t& ds, const std::string& name, const h5::acpl_t& acpl = h5::default_acpl ){
		h5::at_t attr = h5::open(ds, name, h5::default_acpl);
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
				   h5::error::io::attribute::read, "couldn't read dataset ...");
			return object;
		}else{
			T object = impl::get<T>::ctor( current_dims );
			element_t *ptr = const_cast<element_t*>( impl::data( object ));
			H5CPP_CHECK_NZ( H5Aread( static_cast<hid_t>(attr), static_cast<hid_t>(type), ptr ),
				   h5::error::io::attribute::read, "couldn't read dataset ...");
			return object;
		}
	}

	// STD::STRING
	template <class T, class D=typename impl::decay<T>::type, class... args_t> inline
	typename std::enable_if<std::is_same<D,std::string>::value, //TODO: add char**
	T>::type aread( const h5::ds_t& ds, const std::string& name, const h5::acpl_t& acpl = h5::default_acpl ){
		h5::at_t attr = h5::open(ds, name, h5::default_acpl);
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
			   h5::error::io::attribute::read, "couldn't read dataset ...");
		for( size_t i=0; i<nelem; i++)
			if( ptr[i] != NULL ) object[i] = std::string( ptr[i] );
        free(ptr);
		return object;
	}

}
#endif

