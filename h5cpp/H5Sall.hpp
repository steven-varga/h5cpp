/*
 * Copyright (c) 2018-2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef  H5CPP_SALL_HPP
#define  H5CPP_SALL_HPP

#include <hdf5.h>
#include "H5config.hpp"
#include "H5Iall.hpp"
#include "H5meta.hpp"
#include <initializer_list>
#include <array>


namespace h5::impl {
    struct max_dims_t{}; struct current_dims_t{};
    struct dims_t{}; struct chunk_t{};  struct offset_t{}; struct stride_t{};  struct count_t{}; struct block_t{};

    template <typename T, int N=H5CPP_MAX_RANK>
    struct array  {

        array( size_t rank, hsize_t value  ) : rank( rank ) {
            for(int i=0; i<rank; i++) data[i] = value;
        }
        array( const std::initializer_list<size_t> list  ) : rank( list.size() ) {
            for(int i=0; i<rank; i++) data[i] = *(list.begin() + i);
        }
        // support linalg objects upto 3 dimensions or cubes
        template<class A> array( const std::array<A,0> l ) : rank(0), data{} {}
        template<class A> array( const std::array<A,1> l ) : rank(1), data{l[0]} {}
        template<class A> array( const std::array<A,2> l ) : rank(2), data{l[0],l[1]} {}
        template<class A> array( const std::array<A,3> l ) : rank(3), data{l[0],l[1],l[2]} {}
        // automatic conversion to std::array means to collapse tail dimensions
        template<class A>
        operator const std::array<A,0> () const {
            return {};
        }
        operator hsize_t () const {
            hsize_t a = data[0];
            for(int i=1;i<rank;i++) a*=data[i];
            return a;
        }
        template<class A>
        operator const std::array<A,1> () const {
            size_t a = data[0];
            for(int i=1;i<rank;i++) a*=data[i];
            return {a};
        }
        template<class A>
        operator const std::array<A,2> () const {
            size_t a,b; a = data[0]; b = data[1];
            for(int i=2;i<rank;i++) b*=data[i];
            return {a,b};
        }
        template<class A>
        operator const std::array<A,3> () const {
            size_t a,b,c;   a = data[0]; b = data[1]; c = data[2];
            for(int i=3;i<rank;i++) c*=data[i];
            return {a,b,c};
        }

        array() : rank(0){};
        array( array&& arg ) = default;
        array( array& arg ) = default;
        array& operator=( array&& arg ) = default;
        array& operator=( array& arg ) = default;
        template<class C>
        explicit operator array<C>(){
            array<C> arr;
            arr.rank = rank;
            for( int i=0; i<rank; i++) arr[i] = data[i];
            return arr;
        }

        hsize_t& operator[](size_t i){ return *(data + i); }
        const hsize_t& operator[](size_t i) const { return *(data + i); }
        hsize_t* operator*() { return data; }
        const hsize_t* operator*() const { return data; }

        using type = T;
        size_t size() const { return rank; }
        const hsize_t* begin()const { return data; }
        hsize_t* begin() { return data; }
        int rank;
        hsize_t data[N];
    };
    template <class T> inline
    size_t nelements( const h5::impl::array<T>& arr ){
        size_t size = 1;
        for( int i=0; i<arr.rank; i++) size *= arr[i];
        return size;
    }
}

/*PUBLIC CALLS*/
namespace h5 {
    using max_dims_t       = impl::array<impl::max_dims_t>;
    using current_dims_t   = impl::array<impl::current_dims_t>;
    using dims_t   = impl::array<impl::dims_t>;
    using chunk_t  = impl::array<impl::chunk_t>;
    using offset_t = impl::array<impl::offset_t>;
    using stride_t = impl::array<impl::stride_t>;
    using count_t  = impl::array<impl::count_t>;
    using block_t  = impl::array<impl::block_t>;

    using offset = offset_t;
    using stride = stride_t;
    using count  = count_t;
    using block  = block_t;
    using dims   = dims_t;
    using current_dims   = current_dims_t;
    using max_dims       = max_dims_t;

    h5::sp_t create_space( ::hid_t id ){
        return sp_t{id};
    }
    template<class T, class... args_t>
    h5::sp_t create_space(const T& ds, args_t&&... args) {
		using toffset = arg::tpos<h5::offset_t,args_t...>;
        using tstride = arg::tpos<h5::stride_t,args_t...>;
        using tblock = arg::tpos<h5::block_t,args_t...>;
        using tcount = arg::tpos<h5::count_t,args_t...>;

        if constexpr (!toffset::present && !tcount::present) // memspace, filespace expected to match
            return h5::sp_t{H5S_ALL};
        else {
            auto tuple = std::forward_as_tuple(args...);
            h5::offset_t  default_offset{0,0,0,0,0,0,0}; //TODO: make it upto 32 dims? 
		    const h5::offset_t& offset = arg::get( default_offset, args...);
		    h5::stride_t  default_stride{1,1,1,1,1,1,1};
		    const h5::stride_t& stride = arg::get( default_stride, args...);
		    h5::block_t  default_block{1,1,1,1,1,1,1};
		    const h5::block_t& block = arg::get( default_block, args...);
            const h5::count_t& count = std::get<tcount::value>( tuple );
            hsize_t size = 1; //for(int i=0;i<count.rank;i++) size *= count[i] * block[i];

            h5::sp_t sp{H5Screate_simple(1, &size, nullptr )};;
            H5Sselect_all(sp);
            return sp;
        }
    }
/*
    template<class... args_t>
    std::pair<h5::sp_t,h5::sp_t> create_space(const h5::ds_t& ds, args_t&&... args){
        using tcount = arg::tpos<h5::count_t,args_t...>;
        
        h5::sp_t file_space{H5S_ALL}, mem_space{H5S_ALL}; // default CTOR is H5S_ALL which is `0`

        if constexpr (tcount::present) {
            file_space = H5Dget_space(ds);
            h5::dcpl_t dcpl{H5Dget_create_plist(ds)};
            // hyperslab selection is enabled only for chucked layout
            auto tuple = std::forward_as_tuple(args...);
            const h5::count_t& count = std::get<tcount::value>( tuple );
            file_space = H5Dget_space(ds);
            int rank = h5::get_simple_extent_ndims( file_space );
            // capture arguments if specified or assign a default value
            const h5::offset_t& offset = arg::get( h5::offset(rank,0), args...);
            const h5::stride_t& stride = arg::get( h5::stride(rank,1), args...);
            const h5::block_t& block = arg::get( h5::block(rank,1), args...);

            hid_t dapl = h5::get_access_plist( ds );
            // custom pipeline requested
            if( H5Pexist(dapl, H5CPP_DAPL_HIGH_THROUGPUT) && H5Pget_layout(dcpl) == H5D_CHUNKED ){
                h5::impl::pipeline_t<impl::basic_pipeline_t>* filters;
                H5Pget(dapl, H5CPP_DAPL_HIGH_THROUGPUT, &filters);
                filters->write(ds, offset, stride, block, count, dxpl, ptr);
                return ds;
            } else {
                hsize_t size = 1;for(int i=0;i<rank;i++) size *= count[i] * block[i];
                mem_space = h5::create_simple( size );
                h5::select_all( mem_space );
                H5CPP_CHECK_NZ( H5Sselect_hyperslab( file_space, H5S_SELECT_SET, *offset, *stride, *count, *block),
                    std::runtime_error,	h5::error::msg::select_hyperslab);
            }
        }

    }*/
}


#endif
