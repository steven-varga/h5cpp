/*
 * Copyright (c) 2018-2021 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include "H5capi.hpp"
#include "H5Fopen.hpp"
#include <string>
#include <stdexcept>
#include <tuple>
#include <utility>

//TODO: if constexpr(..){} >= c++17
namespace h5 {
  /** \func_create_hdr
 	*  @brief creates a dataset within  an already opened HDF5 container
	* By default the HDF5 dataset size, the file space, is derived from the passed object properties, or may be explicitly specified
	* with optional properties such as h5::count, h5::current_dims h5::max_dims, h5::stride, h5::block 
	* \par_fd
	* \par_dataset_path
    * \par_args
	* \returns_ds
	* 
	* \tpar_T
	*
	* <br/>The following arguments are context sensitive, may be passed in arbitrary order and with the exception
	* of `const ref&` or `const T*` object being saved/memory region pointed to, the arguments are optional. By default the arguments are set to sensible values,
	* and in most cases the function call will deliver good performance. With that in mind, the options below provide an easy to use high level fine
	* tuning mechanism to get the best experience without calling HDF5 CAPI functions directly. 
    *
	* \par_current_dims
	* When omitted, the system computes the default value as follows  `h5::block{..}` and `h5::stride{..}` given as:
	* 		`current_dims[i] = count[i] (stride[i] - block[i] + 1) + offset[i]` and when only `h5::count` is available 
	*       `current_dims[i] = count[i] + offset[i]`
	* \par_max_dims
	* or `H5S_UNLIMITED` along the dimension intended to be extendable
	* \par_dcpl
	* \par_lcpl
	* <br/><b>example:</b>
	* @code
	* auto ds = h5::create<short>("file.h5","path/to/dataset", 
	*	h5::current_dims{10,20}, h5::max_dims{10,H5S_UNLIMITED},
	*	h5::create_path | h5::utf8, // optional lcpl with this default settings**
	*	h5::chunk{2,3} | h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9})
	* @endcode 
 	*/
	template<class T, class... args_t> inline
	h5::ds_t create( const h5::fd_t& fd, const std::string& dataset_path, args_t&&... args ) try {
		// compile time check of property lists: 
		using tcurrent_dims = typename arg::tpos<const h5::current_dims_t&,const args_t&...>;
		using tmax_dims 	= typename arg::tpos<const h5::max_dims_t&,const args_t&...>;
		using tdcpl 		= typename arg::tpos<const h5::dcpl_t&,const args_t&...>;
		using tdt_t 		= typename arg::tpos<const h5::dt_t<T>&,const args_t&...>;

		//TODO: make copy of default dcpl
		h5::dcpl_t default_dcpl{ H5Pcreate(H5P_DATASET_CREATE) };
		// get references to property lists or system default values 
		const h5::lcpl_t& lcpl = arg::get(h5::default_lcpl, args...);
		const h5::dcpl_t& dcpl = arg::get(default_dcpl, args...);
		const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);

		H5CPP_CHECK_PROP( lcpl, h5::error::property_list::misc, "invalid list control property" );
		H5CPP_CHECK_PROP( dcpl, h5::error::property_list::misc, "invalid data control property" );
		H5CPP_CHECK_PROP( dapl, h5::error::property_list::misc, "invalid data access property" );
		// and dimensions
		h5::current_dims_t current_dims_default{0}; // if no current dims_present 
		// this mutable value will be referenced
		const h5::current_dims_t& current_dims = arg::get(current_dims_default, args...);
		h5::max_dims_t max_dims_default{0};
		const h5::max_dims_t& max_dims = arg::get(max_dims_default, args... );
		bool has_unlimited_dimension = false;
		size_t rank = 0;
		h5::sp_t space_id{H5I_UNINIT}; // set to invalid state 
		h5::ds_t ds{H5I_UNINIT};

		if constexpr( tmax_dims::present ){
			rank = max_dims.size();
			if constexpr( !tcurrent_dims::present ){
				// set current dimensions to given one or zero if H5S_UNLIMITED mimics matlab(tm) behavior while allowing extendable matrices
				for(hsize_t i=0; i<rank; i++)
					current_dims_default[i] = max_dims[i] != H5S_UNLIMITED
						? max_dims[i] : (has_unlimited_dimension=true, static_cast<hsize_t>(0));
				current_dims_default.rank = static_cast<int>(rank);
			}
		} else static_assert( tcurrent_dims::present,"current or max dimensions must be present in order to create a dataset!" );

		if( !tdcpl::present && ( has_unlimited_dimension || (tcurrent_dims::present && tmax_dims::present)) ){
			chunk_t chunk{0}; chunk.rank = current_dims.rank;
			for(hsize_t i=0; i<rank; i++)
				chunk[i] = current_dims[i] ? current_dims[i] : 1;
			h5::set_chunk(default_dcpl, chunk );
		}
		// use move semantics to set space
		space_id =  tmax_dims::present ?
			std::move( h5::create_simple( current_dims, max_dims ) ) :  std::move( h5::create_simple( current_dims ) );
		using element_t = typename meta::decay<T>::type;
		h5::meta::resolved_type_t<element_t> type;
		if constexpr (tdt_t::present)
			type = static_cast<::hid_t>(
				std::get<tdt_t::value>( std::forward_as_tuple( args...)));
		return h5::createds(fd, dataset_path, static_cast<::hid_t>(type), space_id, lcpl, dcpl, dapl);
	} catch( const std::runtime_error& err ) {
			throw h5::error::io::dataset::create( err.what() );
	}

  /** \func_create_hdr
 	*  @brief creates a dataset within  an HDF5 container opened with flag `H5F_ACC_RDWR`
	* By default the HDF5 dataset size, the file space, is derived from the passed object properties, or may be explicitly specified
	* with optional properties such as h5::count, h5::current_dims h5::max_dims, h5::stride, h5::block 
	* \par_file_path
	* \par_dataset_path
    * \par_args
	* \returns_ds
	* 
	* \tpar_T
	*
	* <br/>The following arguments are context sensitive, may be passed in arbitrary order and with the exception
	* of `const ref&` or `const T*` object being saved/memory region pointed to, the arguments are optional. By default the arguments are set to sensible values,
	* and in most cases the function call will deliver good performance. With that in mind, the options below provide an easy to use high level fine
	* tuning mechanism to get the best experience without calling HDF5 CAPI functions directly. 
    *
	* \par_current_dims
	* When omitted, the system computes the default value as follows  `h5::block{..}` and `h5::stride{..}` given as:
	* 		`current_dims[i] = count[i] (stride[i] - block[i] + 1) + offset[i]` and when only `h5::count` is available 
	*       `current_dims[i] = count[i] + offset[i]`
	* \par_max_dims
	* or `H5S_UNLIMITED` along the dimension intended to be extendable
	* \par_dcpl
	* \par_lcpl
	* <br/><b>example:</b>
	* @code
	* auto ds = h5::create<short>("file.h5","path/to/dataset", 
	*	h5::current_dims{10,20}, h5::max_dims{10,H5S_UNLIMITED},
	*	h5::create_path | h5::utf8, // optional lcpl with this default settings**
	*	h5::chunk{2,3} | h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9})
	* @endcode 
 	*/
	template<class T, class... args_t>
	inline h5::ds_t create( const std::string& file_path, const std::string& dataset_path, args_t&&... args ){
		h5::fd_t fd = h5::open(file_path, H5F_ACC_RDWR, h5::default_fapl);
		return ::h5::create<T>(fd, dataset_path, args...);
	}
}
