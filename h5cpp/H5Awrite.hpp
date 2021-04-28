/*
 * Copyright (c) 2018 - 2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef  H5CPP_AWRITE_HPP
#define  H5CPP_AWRITE_HPP

#include <hdf5.h>
#include "H5config.hpp"
#include "H5Eall.hpp"
#include "H5Iall.hpp"
#include "H5Sall.hpp"
#include "H5Tall.hpp"
#include "H5Tmeta.hpp"
#include "H5Pall.hpp"
#include "H5Aopen.hpp"
#include "H5Acreate.hpp"
#include <type_traits>
#include <string>
#include <stdexcept>

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
    // const char* ( strings are converted to 
    inline void awrite( const h5::at_t& attr, const char* ptr ){
        h5::dt_t<char*> type;
        const char** data = &ptr;
        H5CPP_CHECK_NZ( H5Awrite( static_cast<hid_t>(attr), static_cast<hid_t>( type ), data ),
                h5::error::io::attribute::write, "couldn't var length string attribute.");
    }

    // const char[]
    template <class T, class HID_T, class... args_t> inline typename std::enable_if<
        std::is_array<T>::value && impl::attr::is_location<HID_T>::value,
    h5::at_t>::type awrite( const HID_T& parent, const std::string& name, const T& ref, const h5::acpl_t& acpl = h5::default_acpl ){
        std::cout <<" <const ref&:" << ref <<"  >\n";
        h5::current_dims_t current_dims = impl::size( ref );
        using element_t = typename impl::decay<T>::type;
        h5::at_t attr = ( H5Aexists(static_cast<hid_t>(parent), name.c_str() ) > 0 ) ?
            h5::aopen(parent, name, h5::default_acpl) : h5::acreate<element_t>(parent, name, current_dims);
        h5::awrite(attr, ref);
        return attr;
    }

    // general case but not: {std::initializer_list<T>} and const char[] 
    template <class T, class HID_T, class... args_t>
    inline typename std::enable_if<!std::is_array<T>::value && impl::attr::is_location<HID_T>::value,
    h5::at_t>::type awrite( const HID_T& parent, const std::string& name, const T& ref, const h5::acpl_t& acpl = h5::default_acpl ){
        h5::current_dims_t current_dims = impl::size( ref );
        using element_t = typename impl::decay<T>::type;
        h5::at_t attr = ( H5Aexists(static_cast<hid_t>(parent), name.c_str() ) > 0 ) ?
            h5::aopen(parent, name, h5::default_acpl) : h5::acreate<element_t>(parent, name, current_dims);
        h5::awrite(attr, impl::data(ref) );
        return attr;
    }

    template<class T, class HID_T>
    inline typename std::enable_if<!std::is_same<T,std::string>::value && impl::attr::is_location<HID_T>::value,
    h5::at_t>::type awrite( const HID_T& parent, const std::string& name,
        const std::initializer_list<T> ref, const h5::acpl_t& acpl = h5::default_acpl ) try {

        h5::current_dims_t current_dims = impl::size( ref );
        using element_t = typename impl::decay<std::initializer_list<T>>::type;

        h5::at_t attr = ( H5Aexists(static_cast<hid_t>(parent), name.c_str() ) > 0 ) ?
            h5::aopen(parent, name, h5::default_acpl) : h5::acreate<element_t>(parent, name, current_dims);
        h5::awrite<element_t>(attr, impl::data( ref ) );
        return attr;
    } catch( const std::runtime_error& err ){
        throw h5::error::io::attribute::write( err.what() );
    }

    template<class T, class HID_T>
    inline typename std::enable_if<std::is_same<T,std::string>::value && impl::attr::is_location<HID_T>::value,
    h5::at_t>::type awrite( const HID_T& parent, const std::string& name,
        const std::initializer_list<T> ref, const h5::acpl_t& acpl = h5::default_acpl ) try {

        h5::current_dims_t current_dims = impl::size( ref );
        using element_t = typename impl::decay<std::initializer_list<T>>::type;

        h5::at_t attr = ( H5Aexists(static_cast<hid_t>(parent), name.c_str() ) > 0 ) ?
            h5::aopen(parent, name, h5::default_acpl) : h5::acreate<element_t>(parent, name, current_dims);
        //h5::awrite<element_t>(attr, impl::data( ref ) );
        return attr;
    } catch( const std::runtime_error& err ){
        throw h5::error::io::attribute::write( err.what() );
    }
    namespace impl::tuple {
        // tail case
        template <std::size_t N, class... Fields>
        typename std::enable_if< N == 0,
        void>::type awrite(const h5::gr_t& parent, const std::tuple<Fields...>& fields, const h5::acpl_t& acpl ){
            h5::awrite(parent, std::get<N>(fields), std::get<N+1>(fields), acpl);
        }
        // 
        template <std::size_t N, class... Fields, class... Names, class... Args>
        typename std::enable_if< std::isgreater(N,0),
        void>::type awrite(const h5::gr_t& parent, const std::tuple<Fields...>& fields, const h5::acpl_t& acpl){

            impl::tuple::awrite<N-2>(parent, fields, acpl);
                h5::awrite(parent, std::get<N>(fields), std::get<N+1>(fields), acpl);
        }
    }

    // breaks up std::tuple into set of calls, and delegates them to respective awrite calls
    // std::tuple<L0,V0, L1,V1, ... ,  Ln,Vn> is a pair wise of labels and values
    template<class P, class... A>
    inline typename std::enable_if<impl::attr::is_location<P>::value,
    void>::type awrite( const P& parent, const std::tuple<A...>& attr, const h5::acpl_t& acpl = h5::default_acpl) try {
        constexpr std::size_t N = std::tuple_size<std::tuple<A...>>::value - 2;
        impl::tuple::awrite<N>(parent, attr, acpl);
    } catch( const std::runtime_error& err ){
        throw h5::error::io::attribute::write( err.what() );
    }


}

template<> inline
h5::at_t h5::ds_t::operator[]( const char name[] ){
    //we don't have the object parameters yet available the only thing to do is 
    //mark it H5I_UNINIT and in the second phase create the attribute 
    h5::at_t attr = ( H5Aexists(static_cast<hid_t>(*this), name ) > 0 ) ?
            h5::aopen(static_cast<hid_t>( *this ), name, h5::default_acpl) : h5::at_t{H5I_UNINIT};
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

