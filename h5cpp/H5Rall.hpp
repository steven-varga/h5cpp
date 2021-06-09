/*
 * Copyright (c) 2018-2021 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <initializer_list>


#ifndef  H5CPP_RALL_HPP 
#define  H5CPP_RALL_HPP

namespace h5 {
    template <class... args_t>
    reference_t reference(const h5::fd_t& fd, const std::string& dataset_path, args_t&&... args) {
        using tcount = typename arg::tpos<const h5::count_t&,const args_t&...>;
        using toffset = typename arg::tpos<const h5::offset_t&, const args_t&...>;
        using tstride = typename arg::tpos<const h5::stride_t&, const args_t&...>;
        using tblock = typename arg::tpos<const h5::block_t&, const args_t&...>;
        auto tuple = std::forward_as_tuple(args...);

        if constexpr (toffset::present) { // region reference

            h5::ds_t ds = h5::open(fd, dataset_path);
            h5::sp_t sp{H5Dget_space(ds)};
            int rank = h5::get_simple_extent_ndims(sp);
            herr_t err = 0;

            h5::count_t count = std::get<tcount::value>( tuple );  //we make a copy of it
            const h5::block_t& block = arg::get( h5::default_block, args...);
            const h5::offset_t& offset = arg::get( h5::default_offset, args...);
            const h5::stride_t& stride = arg::get( h5::default_stride, args...);
            if constexpr( tblock::present ){ // we have to normalise `count` such that `size[i] = count[i] * block[i]` holds
                for(hsize_t i=0; i < rank; i++) count[i] /= block[i];
                err = H5Sselect_hyperslab(sp, H5S_SELECT_SET, *offset, *stride, *count, *block);
            } else { // we have to convert h5::count_t{..} to h5::block{..} and initiate a single block transfer
                h5::block_t block_ = static_cast<h5::block_t>(count);
                block_.rank = rank;
                err = H5Sselect_hyperslab(sp, H5S_SELECT_SET, *offset, *stride, *h5::default_count, *block_);
            }
            H5CPP_CHECK_NZ(err, h5::error::io::dataset::misc, h5::error::msg::select_hyperslab);
            reference_t ref;
            err = H5Rcreate(&ref.value, fd, dataset_path.data(), H5R_DATASET_REGION, sp);
            H5CPP_CHECK_NZ(err, h5::error::io::dataset::misc, "couldn't create reference to dataset...");
            return ref;
        } else { // TODO: object reference
            static_assert( !tcount::present && !tstride::present && !tblock::present
            ,"count | stride | block are not allowed without h5::offset present.." );
            static_assert( !toffset::present, "object reference is not yet impleneted" );
        }
    }
}
/* TODO: needs be matched with function below
    template<class T, class... args_t>
	inline void read(const h5::ds_t& ds, h5::reference_t& reference, T& object,  args_t&&... args){
        using element_t = typename impl::decay<T>::type;
        element_t* ptr = impl::data(object);
        h5::count_t size = impl::size(object);

        h5::ds_t ds_ = H5Rdereference2(ds, H5P_DEFAULT, H5R_DATASET_REGION, &reference);
        h5::sp_t file_space = H5Rget_region(ds_, H5R_DATASET_REGION, &reference);
        
        h5::sp_t mem_space = h5::create_simple( size );
        h5::dt_t<element_t> mem_type;
        h5::select_all( mem_space );
        h5::select_all( file_space );

        H5CPP_CHECK_NZ( 
        H5Dread(ds_, mem_type, mem_space, file_space, dxpl, ptr ),
            h5::error::io::dataset::read, h5::error::msg::read_dataset);
    }
*/
namespace h5::exp {
    template<class T, class... args_t>
	inline T read(const h5::ds_t& ds, h5::reference_t& reference, args_t&&... args){
        using element_t = typename impl::decay<T>::type;

        h5::ds_t ds_ = H5Rdereference2(ds, H5P_DEFAULT, H5R_DATASET_REGION, &reference);
        h5::sp_t file_space = H5Rget_region(ds_, H5R_DATASET_REGION, &reference);
        
        h5::dt_t<element_t> mem_type;
        h5::count_t start, stop, block;
        start.rank = stop.rank = block.rank = H5Sget_simple_extent_ndims(file_space);
        H5Sget_select_bounds(file_space, *start, *stop);
        for (auto i=0; i < block.rank; i++)
            block[i] = stop[i] - start[i]+1;
        T object = impl::get<T>::ctor(block);
        H5Sselect_hyperslab(file_space, H5S_SELECT_SET, 
            *start, nullptr, *h5::default_count, *block);
		element_t *ptr = impl::data(object);
        h5::sp_t mem_space = h5::create_simple(block);
        h5::select_all(mem_space);

        H5CPP_CHECK_NZ( 
        H5Dread(ds_, mem_type, mem_space, file_space, dxpl, ptr ),
            h5::error::io::dataset::read, h5::error::msg::read_dataset);
        return object; 
        
    }

    template <class T, class... args_t>
	inline h5::ds_t write( const h5::ds_t& ds, h5::reference_t& reference, 
                const T* ptr,  args_t&&... args  ) try {
                    
        h5::dt_t<T> mem_type;

		const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...); 
        h5::ds_t ds_ = H5Rdereference2(ds, H5P_DEFAULT, H5R_DATASET_REGION, &reference);
        h5::sp_t file_space = H5Rget_region(ds_, H5R_DATASET_REGION, &reference);
        h5::count_t start, stop, block;

        start.rank = stop.rank = block.rank = H5Sget_simple_extent_ndims(file_space);
        H5Sget_select_bounds(file_space, *start, *stop);
        for (auto i=0; i < block.rank; i++)
            block[i] = stop[i] - start[i]+1;
        H5Sselect_hyperslab(file_space, H5S_SELECT_SET, 
            *start, nullptr, *h5::default_count, *block);

        h5::sp_t mem_space = h5::create_simple(block);
        h5::select_all(mem_space);

		H5CPP_CHECK_NZ(
			H5Dwrite(ds_, mem_type, mem_space, file_space, dxpl, ptr),
				h5::error::io::dataset::write, h5::error::msg::write_dataset);
		return ds;
	} catch ( const std::exception& err ){
		throw h5::error::io::dataset::write( err.what() );
	}
	
    template <class T, class... args_t>
	inline h5::ds_t write( const h5::fd_t& fd, const std::string& dataset_path,  h5::reference_t& reference, 
                const T* ptr,  args_t&&... args  ) try {
        
        using tcount  = typename arg::tpos<const h5::count_t&, const args_t&...>;
		//static_assert( tcount::present, "h5::count_t{ ... } must be provided to describe T* memory region" );
		h5::ds_t ds; // initialized to H5I_UNINIT
		
		h5::mute(); // find out if we have to create the dataset 
			bool is_dataset_present = H5Lexists(fd, dataset_path.c_str(), H5P_DEFAULT) > 0;
		h5::unmute(); // <- make sure not to mute error handling longer than needed
		if (is_dataset_present) {
			const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
			ds = h5::open(fd, dataset_path, dapl);
		} else {
			// dataset doesn't exist, or some error happened, since h5::create doesn't know of the 
			// memory space size as `T& ref` never passed along we have to compute the `h5::current_dims_t{}` upfront
			using tcurrent_dims = typename arg::tpos<const h5::current_dims_t&, const args_t&...>;
			if constexpr (tcurrent_dims::present) // user knows what he is doing, specified h5::current_dims{} explicitly
				ds = h5::create<h5::reference_t>(fd, dataset_path, args...);
			else {
				h5::current_dims_t current_dims{1};  // count must be present, we checked
				ds = h5::create<h5::reference_t>(fd, dataset_path, current_dims, args...);          // and use it to create dataset
			}
		}
        h5::exp::write(ds, reference, ptr, args...);

		return ds;
	} catch ( const std::exception& err ){
		throw h5::error::io::dataset::write( err.what() );
	}
	

}
#endif 