/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_AWRITE_HPP
#define  H5CPP_AWRITE_HPP
namespace h5 {

	template <class T>
	inline void awrite( const h5::at_t& attr, const T* ptr ){
 		// in case of const array of const strings: const * char * ptr 
		// remove const -ness
		using element_t = typename impl::decay<T>::type;
		h5::dt_t<element_t> type;
		H5CPP_CHECK_NZ( H5Awrite( static_cast<hid_t>(attr), static_cast<hid_t>( type ), ptr ),
				h5::error::io::attribute::write, "couldn't write attribute.");
	}
	inline void awrite( const h5::at_t& attr, const char* ptr ){
		h5::dt_t<char*> type;
		const char** data = &ptr;
		H5CPP_CHECK_NZ( H5Awrite( static_cast<hid_t>(attr), static_cast<hid_t>( type ), data ),
				h5::error::io::attribute::write, "couldn't var length string attribute.");
	}
	// const char[]
	template <class T, class P, class... args_t> inline typename std::enable_if<
		std::is_array<T>::value && h5::impl::is_valid_attr<P>::value,
	h5::at_t>::type awrite( const P& parent, const std::string& name, const T& ref, const h5::acpl_t& acpl = h5::default_acpl ){
		h5::current_dims_t current_dims = impl::size( ref );
		using element_t = typename impl::decay<T>::type;
		h5::at_t attr = ( H5Aexists(static_cast<hid_t>(parent), name.c_str() ) > 0 ) ?
			h5::open(parent, name, h5::default_acpl) : h5::create<element_t>(parent, name, current_dims);
		h5::awrite(attr, &ref[0] );
		return attr;
	}

	// general case but not: {std::initializer_list<T>} and const char[] 
	template <class T, class P, class... args_t>
	inline typename std::enable_if<!std::is_array<T>::value && h5::impl::is_valid_attr<P>::value,
	h5::at_t>::type awrite( const P& parent, const std::string& name, const T& ref, const h5::acpl_t& acpl = h5::default_acpl ){
		h5::current_dims_t current_dims = impl::size( ref );
		using element_t = typename impl::decay<T>::type;
		h5::at_t attr = ( H5Aexists(static_cast<hid_t>(parent), name.c_str() ) > 0 ) ?
			h5::open(parent, name, h5::default_acpl) : h5::create<element_t>(parent, name, current_dims);
		h5::awrite(attr, impl::data(ref) );
		return attr;
	}

	// std::initializer_list<T> because T:= std:initializer_list<element_t> doesn't work
	template<class T, class P>
	inline typename std::enable_if<h5::impl::is_valid_attr<P>::value,
    h5::at_t>::type awrite( const P& parent, const std::string& name, const std::initializer_list<T> ref,
																		const h5::acpl_t& acpl = h5::default_acpl ) try {

		h5::current_dims_t current_dims = impl::size( ref );
		using element_t = typename impl::decay<std::initializer_list<T>>::type;

		h5::at_t attr = ( H5Aexists(static_cast<hid_t>(parent), name.c_str() ) > 0 ) ?
			h5::open(parent, name, h5::default_acpl) : h5::create<element_t>(parent, name, current_dims);
		h5::awrite<element_t>(attr, impl::data( ref ) );
		return attr;
	} catch( const std::runtime_error& err ){
		throw h5::error::io::attribute::write( err.what() );
	}
}

template<> inline
h5::at_t h5::ds_t::operator[]( const char name[] ){
	//we don't have the object parameters yet available the only thing to do is 
	//mark it H5I_UNINIT and in the second phase create the attribute 
	h5::at_t attr = ( H5Aexists(static_cast<hid_t>(*this), name ) > 0 ) ?
			h5::open(static_cast<hid_t>( *this ), name, h5::default_acpl) : h5::at_t{H5I_UNINIT};
	attr.ds   = static_cast<hid_t>(*this);
	attr.name = std::string(name);
	return attr;
}
template<> template< class V> inline
h5::at_t h5::at_t::operator=( V arg ){
	if( !H5Iis_valid(this->ds) )
		throw h5::error::io::attribute::create("unable to create attribute: underlying dataset id not provided...");

	h5::awrite(ds, name, arg);
	return *this;
}
template<> template< class V> inline
h5::at_t h5::at_t::operator=( const std::initializer_list<V> args ){
	if( !H5Iis_valid(this->ds) )
		throw h5::error::io::attribute::create("unable to create attribute: underlying dataset id not provided...");

	h5::awrite(ds, name, args);
	return *this;
}

#endif

