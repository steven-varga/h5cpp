/*
 * Copyright (c) 2018 - 2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef  H5CPP_PIPELINE_HPP
#define  H5CPP_PIPELINE_HPP

#include <hdf5.h>
#include "H5config.hpp"
#include "H5Eall.hpp"  /* all methods throwing exceptions must be included after */
#include "H5Iall.hpp"
#include "H5Sall.hpp"
#include "H5Zall.hpp"
#include <memory>
#include <string.h>

namespace h5 {
    int get_chunk_dims( const h5::dcpl_t& dcpl,  h5::chunk_t& chunk_dims );
}

namespace h5::impl {
    enum struct filter_direction_t {
        forward = 0, reverse = 1
    };

    template <class Derived>
    struct pipeline_t {
        pipeline_t(){};
        ~pipeline_t(){};
        void set_cache( const h5::dcpl_t& dcpl, size_t element_size );
        void write(const h5::ds_t& ds, const h5::offset_t& start, const h5::stride_t& stride, const h5::block_t& block, const h5::count_t& count,
                const h5::dxpl_t& dxpl, const void* ptr);

        void read(const h5::ds_t& ds, const h5::offset_t& start, const h5::stride_t& stride, const h5::block_t& block, const h5::count_t& count,
                const h5::dxpl_t& dxpl, void* ptr);

        void write_chunk(  const hsize_t* offset, size_t nbytes, const void* ptr ){
            static_cast<Derived*>(this)->write_chunk_impl(offset, nbytes, ptr);
        }
        void read_chunk( const hsize_t* offset, size_t nbytes, void* ptr ){
            static_cast<Derived*>(this)->read_chunk_impl(offset, nbytes, ptr);
        }
        void split_to_chunk_write(filter_direction_t direction, const hsize_t* offset, const hsize_t* dims, const void* ptr );
        void split_to_chunk_read(filter_direction_t direction, const hsize_t* offset, const hsize_t* dims, void* ptr );

        char *chunk0, *chunk1;
        hsize_t tail,rank;

    public:
        void push( filter::call_t filter );
        void pop();

        std::unique_ptr<char> ptr0, ptr1; // will call std::free on dtor
        filter::call_t filter[H5CPP_MAX_FILTER];
        hsize_t n,
                C[H5CPP_MAX_RANK], D[H5CPP_MAX_RANK],
                N[H5CPP_MAX_RANK], B[H5CPP_MAX_RANK], Rx[H5CPP_MAX_RANK],Ry[H5CPP_MAX_RANK];
        unsigned cd_values[H5CPP_MAX_FILTER][H5CPP_MAX_FILTER_PARAM],
                flags[H5CPP_MAX_FILTER];
        size_t  block_size, element_size, cd_size[H5CPP_MAX_FILTER];
        h5::dcpl_t dcpl;
        h5::dxpl_t dxpl;
        h5::ds_t ds;
    };

    struct basic_pipeline_t : public pipeline_t<basic_pipeline_t>{
        void write_chunk_impl( const hsize_t* offset, size_t nbytes, const void* ptr );
        void read_chunk_impl( const hsize_t* offset, size_t nbytes, void* ptr );
    };
    struct threaded_pipeline_t : public pipeline_t<threaded_pipeline_t>{
        void write_chunk_impl( const hsize_t* offset, size_t nbytes, const void* ptr ){
        }
        void read_chunk_impl( const hsize_t* offset, size_t nbytes, void* ptr ){
        }
    };
    struct romio_pipeline_t : public pipeline_t<romio_pipeline_t>{
        void write_chunk_impl( const hsize_t* offset, size_t nbytes, const void* ptr ){
        }
        void read_chunk_impl( const hsize_t* offset, size_t nbytes, void* ptr ){
        }
    };
    struct hadoop_pipeline_t : public pipeline_t<hadoop_pipeline_t>{
        void write_chunk_impl( const hsize_t* offset, size_t nbytes, const void* ptr ){
        }
        void read_chunk_impl( const hsize_t* offset, size_t nbytes, void* ptr ){
        }
    };
}

template< class Derived>
inline void h5::impl::pipeline_t<Derived>::write(
        const h5::ds_t& ds, const h5::offset_t& offset, const h5::stride_t& stride, const h5::block_t& block, const h5::count_t& count,
                const h5::dxpl_t& dxpl, const void* ptr){

    h5::offset_t offset_; h5::count_t count_;
    for(int i=0; i<rank; i++)
        offset_[i] = offset[i], count_[i] = count[i] * block[i];
    this->dxpl = dxpl; this->ds = ds;
    split_to_chunk_write(filter_direction_t::forward, offset_.begin(), count_.begin(), ptr );
}

template< class Derived>
inline void h5::impl::pipeline_t<Derived>::read(
        const h5::ds_t& ds, const h5::offset_t& offset, const h5::stride_t& stride, const h5::block_t& block, const h5::count_t& count,
                const h5::dxpl_t& dxpl, void* ptr){

    h5::offset_t offset_; h5::count_t count_;
    for(int i=0; i<rank; i++)
        offset_[i] = offset[i], count_[i] = count[i] * block[i];
    this->dxpl = dxpl; this->ds = ds;
    split_to_chunk_read(filter_direction_t::reverse, offset_.begin(), count_.begin(), ptr );
}

template< class Derived>
inline void h5::impl::pipeline_t<Derived>::set_cache( const h5::dcpl_t& dcpl, size_t element_size ){
    n = 1, tail = 0; this->element_size = element_size;

    // grab chunk dimensions, which is a block we're breaking data into
    h5::chunk_t block;
    if( (rank = h5::get_chunk_dims( dcpl, block )) == 0  )
            throw std::runtime_error("data-space is rank 0, is data space a scalar? ");

    //fix B block/chunk size for the lifespan of pipeline
    for(int i=0; i<rank; i++ )
        n *= block[i], B[i] = block[rank-i-1];

    block_size = n*element_size;
    unsigned filter_config;
    unsigned N = H5Pget_nfilters( dcpl );
    for(unsigned i=0; i<N; i++){
        cd_size[i] = H5CPP_MAX_FILTER_PARAM;
        push(
            filter::get_callback( H5Pget_filter2( dcpl, i, &flags[i], &cd_size[i], cd_values[i], 0, nullptr, &filter_config )));
    }

    ptr0 = std::move( std::unique_ptr<char>{ (char*)aligned_alloc( H5CPP_MEM_ALIGNMENT, block_size )} );
    ptr1 = std::move( std::unique_ptr<char>{ (char*)aligned_alloc( H5CPP_MEM_ALIGNMENT, block_size )} );
    // get an alias to smart ptr
    if( (chunk0 = ptr0.get()) == NULL || (chunk1 = ptr1.get()) == NULL )
        throw h5::error::io::dataset::open( H5CPP_ERROR_MSG("CTOR: couldn't allocate memory for caching chunks, invalid/check size?"));
}

#define h5cpp_outer( idx ) for( j##idx =0; j##idx < n##idx; j##idx += b##idx)
#define h5cpp_inner( idx ) for( i##idx = j##idx; i##idx < std::min(j##idx+b##idx,n##idx); i##idx++)
#define h5cpp_def( idx ) hsize_t i##idx=0, j##idx = 0, s##idx=0, n##idx=N[idx], b##idx=B[idx], rx##idx=Rx[idx],  ry##idx=Ry[idx];

template< class Derived>
inline void h5::impl::pipeline_t<Derived>::split_to_chunk_read(
    filter_direction_t direction, const hsize_t* chunk_offset, const hsize_t* dims, void* ptr_ ){
    char* ptr = static_cast<char*>( ptr_ );
    // compute edges if any - for the data size
    // computes R residuals, and sets N dimension in reverse order
    // when actual data dimensions - current_dims are greater then a chunk, it must be broken into chunk size
    // if there are residuals on the edges, then padding is needed.
    unsigned i=0;
    for(; i<rank; i++ )
        N[i] = dims[rank-i-1], Ry[i] = N[i] % B[i], Rx[i] = B[i] - chunk_offset[i] % B[i];
    // set the rest to default
    for(; i<H5CPP_MAX_RANK; i++) N[i] = B[i] = 1, Rx[i] = Ry[i] = 0;

    // b - block size, j - block index pos, n - actual dimension of data
    // rx - remainder at the leading edges, ry - remainder at trailing edges 
    h5cpp_def(0) h5cpp_def(1) h5cpp_def(2) h5cpp_def(3) h5cpp_def(4) h5cpp_def(5) h5cpp_def(6)

    h5cpp_outer( 6 ){ h5cpp_outer( 5 ){ h5cpp_outer( 4 ){ h5cpp_outer( 3 ){
    h5cpp_outer( 2 ){ h5cpp_outer( 1 ){ h5cpp_outer( 0 ){
        char* p = chunk0;
        // at this point we have a single 'chunk' in this->chunk0 buffer ready to go
        // the coordinates are in j indices
        D[0] = j0, D[1]=j1, D[2]=j2, D[3]=j3, D[4]=j4, D[5]=j5, D[6]=j6;
        //coordinates are reversed in D, invert them:
        for(int k=0;k<rank; k++ ) C[k] = D[rank-k-1] + chunk_offset[k];
        // execute filters in reverse direction
        read_chunk( this->C, this->block_size, this->chunk0 );

        h5cpp_inner(6){ h5cpp_inner(5){ h5cpp_inner(4){
        h5cpp_inner(3){ h5cpp_inner(2){ h5cpp_inner(1){
            hsize_t offset = i6*n5*n4*n3*n2*n1*n0 + i5*n4*n3*n2*n1*n0 +
                    i4*n3*n2*n1*n0 + i3*n2*n1*n0 + i2*n1*n0 + i1*n0 + j0;
            if( j0 != n0 - ry0 ) // block copy
                memcpy(ptr + offset * element_size, p, b0 * element_size );
            else // edge handling
                memcpy(ptr + offset * element_size,p, ry0 * element_size );
            p += b0 * element_size;
        }}}}}}
    }}}}}}}
}

template< class Derived>
inline void h5::impl::pipeline_t<Derived>::split_to_chunk_write(
    filter_direction_t direction, const hsize_t* chunk_offset, const hsize_t* dims, const void* ptr_ ){
    const char* ptr = static_cast<const char*>( ptr_ ); const hsize_t* O = chunk_offset;
    // compute edges if any - for the data size
    // computes R residuals, and sets N dimension in reverse order
    // when actual data dimensions - current_dims are greater then a chunk, it must be broken into chunk size
    // if there are residuals on the edges, then padding is needed.
    unsigned i=0;
    for(; i<rank; i++ )
        N[i] = dims[rank-i-1], Rx[i] = B[i] - O[i] % B[i], Ry[i] = (N[i] - Rx[i]) % B[i];
    for(; i<H5CPP_MAX_RANK; i++) N[i] = B[i] = 1, Rx[i] = Ry[i] = 0;

    // b - block size, j - block index pos, n - actual dimension of data
    // rx - remainder at the leading edges, ry - remainder at trailing edges 
    h5cpp_def(0) h5cpp_def(1) h5cpp_def(2) h5cpp_def(3) h5cpp_def(4) h5cpp_def(5) h5cpp_def(6)

    h5cpp_outer( 6 ){ h5cpp_outer( 5 ){ h5cpp_outer( 4 ){ h5cpp_outer( 3 ){
    h5cpp_outer( 2 ){ h5cpp_outer( 1 ){ h5cpp_outer( 0 ){
        char* p = chunk0;
        // reset memory page only on the edges
        if( j0 + b0 > n0 || j1 + b1 > n1 || j2 + b2 > n2 ||
            j3 + b3 > n3 || j4 + b4 > n4 || j5 + b5 > n5 || j6 + b6 > n6 ) memset(p,0x00, block_size);

        h5cpp_inner(6){ h5cpp_inner(5){ h5cpp_inner(4){
        h5cpp_inner(3){ h5cpp_inner(2){ h5cpp_inner(1){
            hsize_t offset = i6*n5*n4*n3*n2*n1*n0 + i5*n4*n3*n2*n1*n0 +
                    i4*n3*n2*n1*n0 + i3*n2*n1*n0 + i2*n1*n0 + i1*n0 + j0;
            if( j0 != n0 - ry0 ) // block copy
                memcpy(p, ptr + offset * element_size, b0 * element_size );
            else // trailing edge handling
                memcpy(p, ptr + offset * element_size, ry0 * element_size );
            p += b0 * element_size;
        }}}}}}
        // at this point we have a single 'chunk' in this->chunk0 buffer ready to go
        // the coordinates are in j indices
        D[0] = j0, D[1]=j1, D[2]=j2, D[3]=j3, D[4]=j4, D[5]=j5, D[6]=j6;
        //coordinates are reversed in D, invert them:
        for(int k=0;k<rank; k++ ) C[k] = D[rank-k-1] + chunk_offset[k];

        // execute filters in direction
        write_chunk( this->C, this->block_size, this->chunk0 );
    }}}}}}}
}
#undef h5cpp_def
#undef h5cpp_inner
#undef h5cpp_outer

template< class Derived>
inline void h5::impl::pipeline_t<Derived>::push(filter::call_t filter_){
    filter[tail++] = filter_;
}

template< class Derived>
inline void h5::impl::pipeline_t<Derived>::pop(){
    tail--;
}

#endif
