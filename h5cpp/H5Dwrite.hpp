/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_DWRITE_HPP
#define H5CPP_DWRITE_HPP

// defines an runtime_exception thrown to track template matching
// for various types;  see tests/H5Dwrite 
#ifndef H5CPP_WRITE_DISPATCH_TEST_
    #define H5CPP_WRITE_DISPATCH_TEST_(type, param, args)
#endif
#include <string.h>

namespace h5::impl {
    template <class T, class... args_t>
    h5::current_dims_t get_current_dims( const T& ref, args_t&&... args ) {
        using tcount  = typename arg::tpos<const h5::count_t&,const args_t&...>;
        using toffset = typename arg::tpos<const h5::offset_t&,const args_t&...>;
        using tstride = typename arg::tpos<const h5::stride_t&,const args_t&...>;
        using tblock = typename arg::tpos<const h5::block_t&,const args_t&...>;

		h5::current_dims_t current_dims;
        // retrieve the dimensions from the object reference
        h5::count_t count = impl::size( ref );
		int rank = impl::rank<T>::value;

        for(int i =0; i < rank; i++ ) current_dims[i] = count[i];
        if constexpr( tstride::present ){
            const h5::stride_t& stride =  std::get<tstride::value>( std::forward_as_tuple( args...) );
            if constexpr( tblock::present ){ // tricky, we have block as well
                const h5::block_t& block =  std::get<tblock::value>( std::forward_as_tuple( args...) );
                for(int i=0; i < rank; i++)
                    current_dims[i] *= (stride[i] - block[i] + 1);
            } else
                for(int i=0; i < rank; i++)
                    current_dims[i] *= stride[i];
        }
        if constexpr( toffset::present ) {
            const h5::offset_t& offset =  std::get<toffset::value>( std::forward_as_tuple( args...) );
            for(int i=0; i < rank; i++)
                current_dims[i]+=offset[i];
        }
        return current_dims;
    }
}



/*  - open existing dataset: 
 *      1. write to opened dataset descriptor: h5::ds_t
 *      2. write to location: 
 *          - h5::fd_t + path/to/dataset
 *          - h5::gr_t
 *  - create dataset
 *      1. write to opened dataset h5::ds_t 
 * */
namespace h5 {
	template <class T>
	void write( const h5::ds_t& ds, const h5::sp_t& mem_space, const h5::sp_t& file_space, const h5::dxpl_t& dxpl, const T* ptr  ){
		H5CPP_CHECK_PROP( dxpl, h5::error::io::dataset::write, "invalid data transfer property" );
		using element_t = typename h5::impl::decay<T>::type;
		h5::dt_t<element_t> type;
		H5CPP_CHECK_NZ(
				H5Dwrite( static_cast<hid_t>( ds ), type, mem_space, file_space, static_cast<hid_t>(dxpl), ptr),
							h5::error::io::dataset::write, h5::error::msg::write_dataset);
	}

    // 
	template <class T, class... args_t>
	ds_t write( const ds_t& ds, const T& ref,   args_t&&... args  ) try {
        H5CPP_WRITE_DISPATCH_TEST_(T, "T& object reference")
    
		using tcount = typename arg::tpos<const h5::count_t&,const args_t&...>;
		static_assert( tcount::present,"h5::count_t{ ... } must be provided to describe T* memory region" );

        auto tuple = std::forward_as_tuple(args...);
		const h5::count_t& count = std::get<tcount::value>( tuple );
		const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
		h5::sp_t file_space{H5Dget_space( static_cast<::hid_t>(ds) )};

		int rank = h5::get_simple_extent_ndims( file_space );
		h5::offset_t  default_offset{0,0,0,0,0,0,0};
		const h5::offset_t& offset = arg::get( default_offset, args...);
		h5::stride_t  default_stride{1,1,1,1,1,1,1};
		const h5::stride_t& stride = arg::get( default_stride, args...);
		h5::block_t  default_block{1,1,1,1,1,1,1};
		const h5::block_t& block = arg::get( default_block, args...);

		hsize_t size = 1;for(int i=0;i<rank;i++) size *= count[i] * block[i];
		hid_t dapl = h5::get_access_plist( ds );

/*
		if( H5Pexist(dapl, H5CPP_DAPL_HIGH_THROUGPUT) ){
			h5::impl::pipeline_t<impl::basic_pipeline_t>* filters;
			H5Pget(dapl, H5CPP_DAPL_HIGH_THROUGPUT, &filters);
			filters->write(ds, offset, stride, block, count, dxpl, ptr);
		}else{
			h5::sp_t mem_space = h5::create_simple( size );
			h5::select_all( mem_space );
			h5::select_hyperslab( file_space, offset, stride, count, block);
			// this can throw exception
			h5::write<T>(ds, mem_space, file_space, dxpl, ptr);
		}
*/
        return ds;
	} catch ( const std::runtime_error& err ){
		throw error::io::dataset::write( err.what() );
	}

    // character literal: "lklkjl"
/*	template <class T, class... args_t>
	ds_t write( const ds_t& ds, const T* ptr,  args_t&&... args  ) try {
        H5CPP_WRITE_DISPATCH_TEST_(T, "character literal")
        h5::dt_t<char*> dt{H5Dget_type(ds)};
		const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
        H5CPP_CHECK_NZ(
				H5Dwrite( ds, dt, H5S_ALL, H5S_ALL, static_cast<hid_t>(dxpl), ptr),
							h5::error::io::dataset::write, h5::error::msg::write_dataset);
		return ds;
	} catch ( const std::exception& err ){
		throw error::io::dataset::write( err.what() );
	}
*/
	template <class T, class... args_t>
	ds_t write( const ds_t& ds, const  std::initializer_list<T>& list,   args_t&&... args  ) try {
        H5CPP_WRITE_DISPATCH_TEST_(T, "std::initializer_list<T>")
        return ds;
	} catch ( const std::runtime_error& err ){
		throw error::io::dataset::write( err.what() );
	}

    // create + write
	template <class T, class... args_t>
	h5::ds_t write( const h5::fd_t& fd, const std::string& dataset_path, const T& ref,  args_t&&... args  ){
        H5CPP_WRITE_DISPATCH_TEST_(T, "write(fd, path, T& ref, args...)")
		using element_t = typename impl::decay<T>::type;
		using tcurrent_dims = typename arg::tpos<const h5::current_dims_t&,const args_t&...>;

        mute();
        ds_t ds;
        if( H5Lexists(fd, dataset_path.c_str(), H5P_DEFAULT ) > 0 ) {
            const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
            h5::open( fd, dataset_path, dapl);
        } else {
            using element_t = typename impl::decay<T>::type;
            ds = h5::create<T>(fd, dataset_path, args...);
		    const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
            h5::dt_t<T> dt = create<T>();
            //if ( impl::is_scalar<T>::value ){ // strings/character literals and scalars
            const element_t * ptr = h5::impl::data( ref );
            H5CPP_CHECK_NZ(
                    H5Dwrite( ds, dt, H5S_ALL, H5S_ALL, static_cast<hid_t>(dxpl), ptr),
                        h5::error::io::dataset::write, h5::error::msg::write_dataset);
        }
		h5::unmute();
 		return ds; //return h5::write<T>(ds, ref,  args...);
	}



    template <class... args_t>
	h5::ds_t write( const std::string& file_path, const std::string& dataset_path, args_t&&... args  ){
        H5CPP_WRITE_DISPATCH_TEST_(T, "file...")
		h5::fd_t fd = h5::open( file_path, H5F_ACC_RDWR, h5::default_fapl );
		return h5::write( fd, dataset_path, args...);
	}
}
#endif
