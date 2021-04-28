/*
 * Copyright (c) 2018 - 2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_ZALL_HPP
#define  H5CPP_ZALL_HPP

#include <hdf5.h>
#include "H5Iall.hpp"
#include <string.h>
#include <iomanip>
#include <zlib.h>
#include <math.h>


namespace h5::impl::filter {
    // TODO: figure something out to map c++ filters to C calls? 
    template<class Derived>
    struct filter_t {
        htri_t can_apply(::hid_t dcpl, ::hid_t type, ::hid_t space){
            return static_cast<Derived*>(this)->can_apply_impl(dcpl,type,space);
        }
        herr_t set_local(::hid_t dcpl, ::hid_t type, ::hid_t space){
            return static_cast<Derived*>(this)->set_local_impl(dcpl,type,space);
        }
        size_t callback(unsigned int flags, size_t cd_nelmts, const unsigned int cd_values[], size_t nbytes, size_t *buf_size, void **buf){
            return 0;
        }
        size_t apply( void* dst, const void* src, size_t size){
            return static_cast<Derived*>(this)->apply(dst,src,size);
        }
        int version;
        unsigned id;
        unsigned encoder_present;
        unsigned decoder_present;
        std::string name;
    };

    using call_t = size_t (*)(void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] );
    inline size_t mock( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
        memcpy(dst,src,size);
        return size;
    }
    inline size_t deflate( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[]){
        memcpy(dst,src,size);
        return size;
    }
    inline size_t scaleoffset( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[]){
        memcpy(dst,src,size);
        return size;
    }
    inline size_t gzip( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[]){
        size_t nbytes = size;
        int ret = compress2( (unsigned char*)dst, &nbytes, (const unsigned char*)src, size, params[0]);
        return nbytes;
    }
    inline size_t szip( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[]){
        memcpy(dst,src,size);
        return size;
    }
    inline size_t nbit( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[]){
        memcpy(dst,src,size);
        return size;
    }
    inline size_t fletcher32( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[]){
        memcpy(dst,src,size);
        return size;
    }

    inline size_t add( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
        memcpy(dst,src,size);
        return size;
    }
    inline size_t shuffle( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
        memcpy(dst,src,size);
        return size;
    }
    inline size_t jpeg( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
        memcpy(dst,src,size);
        return size;
    }
    inline size_t disperse( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
        memcpy(dst,src,size);
        return size;
    }
    inline size_t error( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
        throw std::runtime_error("invalid filter");
    }
    inline call_t get_callback( H5Z_filter_t filter_id ){

        switch( filter_id ){
            case H5Z_FILTER_DEFLATE: return filter::gzip;
            case H5Z_FILTER_SHUFFLE: return filter::shuffle;
            case H5Z_FILTER_FLETCHER32: return filter::fletcher32;
            case H5Z_FILTER_SZIP: return filter::szip;
            case H5Z_FILTER_NBIT: return filter::nbit;
            case H5Z_FILTER_SCALEOFFSET: return filter::scaleoffset;
            default:
                    return filter::error;
        }
    }


}
#endif
