/*
 * Copyright (c) 2018 - 2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef  H5CPP_PIPELINE_BASIC_HPP
#define  H5CPP_PIPELINE_BASIC_HPP

#include <hdf5.h>
#include "H5Zpipeline.hpp"
#include <stdexcept>

inline void h5::impl::basic_pipeline_t::write_chunk_impl( const hsize_t* offset, size_t nbytes, const void* data ){

    size_t length = nbytes;                        // filter may change this, think of compression
    void *in = chunk0, *out=chunk1, *tmp = chunk0; // invariant: out must point to data block written
    uint32_t mask = 0x0;                           // filter mask = 0x0 all filters applied
    switch( tail ){ // tail = index pointing to queue holding filters
        case 0: // no filters, ( if blocking ) -> data == chunk0 otherwise directly from container 
            H5Dwrite_chunk( ds, dxpl, 0x0, offset, nbytes, data);
            break;
        case 1: // single filter
            length = filter[0](out, data, nbytes, flags[0], cd_size[0], cd_values[0] ) ;
            if( !length )
                mask = 1 << 0;
        default: // more than one filter
            for(int j=1; j<tail; j++){ // invariant: out == buffer holding final result
                tmp = in, in = out, out = tmp;
                length = filter[j](out,in,length, flags[j], cd_size[j], cd_values[j]);
                if( !length )
                    mask |= 1 << j;
            }
            // direct write available from > 1.10.4
            H5Dwrite_chunk(ds, dxpl, mask, offset, length, out);
    }
}


inline void h5::impl::basic_pipeline_t::read_chunk_impl( const hsize_t* offset, size_t nbytes, void* data){
    size_t length = nbytes; // filter may changed this, think of compression
  //  void *in = chunk0, *out=chunk1, *tmp = chunk1; // invariant: out must point to data block written
    uint32_t filter_mask;
//  for(int i=0; i<9; i++)
//      std::cout << ((short*) in)[i] << " ";
//  std::cout<<"\n";
    //FIXME:
    length +=0; // shut up pgi compilers
    switch( tail ){ // tail = index pointing to queue holding filters
        case 0: // no filters, ( if blocking ) -> data == chunk0 otherwise directly from container
            H5Dread_chunk(ds, dxpl, offset, &filter_mask, chunk0);
            break;
        case 1: // single filter
            H5Dread_chunk(ds, dxpl, offset, &filter_mask, chunk1);
            length = filter[0](chunk0, chunk1, nbytes, flags[0], cd_size[0], cd_values[0] );
            break;
        default: // more than one filter
            throw std::runtime_error("filters not implemented yet...");
            /* unreachable to pgc++
            if( tail % 2 ){
                H5Dread_chunk(ds, dxpl, offset, &filter_mask, chunk0);
                for(int j=tail; j>0; j--){ // invariant: out == buffer holding final result
                    tmp = in, in = out, out = tmp;
                    length = filter[j](out,in,length, flags[j], cd_size[j], cd_values[j]);
                }
            }*/
            // direct write available from > 1.10.4
    }
}

#endif
