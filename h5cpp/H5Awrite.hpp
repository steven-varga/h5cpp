/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_AWRITE_HPP
#define  H5CPP_AWRITE_HPP
namespace h5{
	template <class T>
	inline void awrite( const h5::at_t& ds, const T* ptr ){
 		// in case of const array of const strings: const * char * ptr 
		// remove const -ness
		using element_t = typename impl::decay<T>::type;
		h5::dt_t<element_t> type;
		H5CPP_CHECK_NZ( H5Awrite( static_cast<hid_t>(ds), static_cast<hid_t>( type ), ptr ),
				h5::error::io::attribute::write, "couldn't write attribute.");
	}
	inline void awrite( const h5::at_t& ds, const char* ptr ){
		h5::dt_t<char*> type;
		const char** data = &ptr;
		H5CPP_CHECK_NZ( H5Awrite( static_cast<hid_t>(ds), static_cast<hid_t>( type ), data ),
				h5::error::io::attribute::write, "couldn't var length string attribute.");
	}

	// general case but not: {std::initializer_list<T>} 
	template <class T, class... args_t> inline
	h5::at_t awrite( const h5::ds_t& ds, const std::string& name, const T& ref, const h5::acpl_t& acpl = h5::default_acpl ){
		h5::current_dims_t current_dims = impl::size( ref );
		using element_t = typename impl::decay<T>::type;
		h5::at_t attr = ( H5Aexists(ds, name.c_str() ) > 0 ) ?
			h5::open(ds, name, h5::default_acpl) : h5::create<element_t>(ds, name, current_dims);
 		h5::awrite(attr, impl::data(ref) );
		return attr;
	}

	// std::initializer_list<T> because T:= std:initializer_list<element_t> doesn't work
	template <class T> inline
	h5::at_t awrite( const h5::ds_t& ds, const std::string& name, const std::initializer_list<T> ref,
																		const h5::acpl_t& acpl = h5::default_acpl ) try {

		h5::current_dims_t current_dims = impl::size( ref );
		using element_t = typename impl::decay<std::initializer_list<T>>::type;

		h5::at_t attr = ( H5Aexists(ds, name.c_str() ) > 0 ) ?
			h5::open(ds, name, h5::default_acpl) : h5::create<element_t>(ds, name, current_dims);
		h5::awrite<element_t>(attr, impl::data( ref ) );
		return attr;
	} catch( const std::runtime_error& err ){
		throw h5::error::io::attribute::write( err.what() );
	}
}
#endif

