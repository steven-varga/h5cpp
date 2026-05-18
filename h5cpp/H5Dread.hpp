/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#pragma once
#include "H5capi.hpp"
#include "H5misc.hpp"
#include "H5Dopen.hpp" // be sure this precedes error handling macro-s !!!
#include "H5Rreference.hpp"
#include <string>
#include <stdexcept>
#include <type_traits>
#include <tuple>

namespace h5 {
	/***************************  REFERENCE *****************************/
 	/** \func_read_hdr
 	*  Updates the content of passed **ptr** pointer, which must have enough memory space to receive data.
	*  Optional arguments **args:= h5::offset | h5::stride | h5::count | h5::block** may be specified for partial IO, 
	*  to describe the retrieved hyperslab from hdf5 file space. Default case is to select and retrieve all elements from dataset. 
	*  **h5::dxpl_t** provides control to datatransfer. 
	* \code
	* h5::fd_t fd = h5::open("myfile.h5", H5F_ACC_RDWR);
	* h5::ds_t ds = h5::open(fd,"path/to/dataset");
	* std::vector<float> myvec(10*10);
	* auto err = h5::read( fd, "path/to/dataset", myvec.data(), h5::count{10,10}, h5::offset{5,0} );	
	* \endcode  
	* \par_file_path \par_dataset_path \par_ptr \par_offset \par_stride \par_count \par_block \par_dxpl  \tpar_T \returns_err
 	*/ 

	template<class T, class... args_t>
	inline std::enable_if_t<!std::is_same_v<T,char**>,
	void> read( const h5::ds_t& ds, T* ptr, args_t&&... args ) try {
		using tcount   = typename arg::tpos<const h5::count_t&,const args_t&...>;
		static_assert( tcount::present, "h5::count_t{ ... } must be specified" );
		static_assert( utils::is_supported<T>, "error: " H5CPP_supported_elementary_types );

		auto tuple = std::forward_as_tuple(args...);
		const h5::count_t& count = std::get<tcount::value>( tuple );

		h5::offset_t  default_offset{0,0,0,0,0,0,0}; // must match H5CPP_MAX_RANK=7
		const h5::offset_t& offset = arg::get( default_offset, args...);

		h5::stride_t  default_stride{1,1,1,1,1,1,1};
		const h5::stride_t& stride = arg::get( default_stride, args...);

		h5::block_t  default_block{1,1,1,1,1,1,1};
		const h5::block_t& block = arg::get( default_block, args...);

		h5::count_t size; // compute actual memory space
		for(int i=0;i<count.rank;i++) size[i] = count[i] * block[i];
		size.rank = count.rank;

		const h5::dxpl_t& dxpl = arg::get( h5::default_dxpl, args...);
		H5CPP_CHECK_PROP( dxpl, h5::error::property_list::misc, "invalid data transfer property" );

		h5::sp_t file_space = h5::get_space(ds);
	   	int rank = h5::get_simple_extent_ndims( file_space );

		if( rank != count.rank ) throw h5::error::io::dataset::read( H5CPP_ERROR_MSG( h5::error::msg::rank_mismatch ));
		using element_t = typename impl::decay<T>::type;
		h5::meta::resolved_type_t<element_t> mem_type;
		hid_t dapl = h5::get_access_plist( ds );
		// See H5Dwrite.hpp for the rationale: pipeline path uses H5Dread_chunk,
		// which only works on chunked datasets. Guard on H5D_CHUNKED so DAPLs
		// with the flag applied to contiguous datasets fall through to H5Dread.
		const bool use_pipeline = [&]() {
			if (!H5Pexist(dapl, H5CPP_DAPL_HIGH_THROUGHPUT)) return false;
			hid_t dcpl_id = H5Dget_create_plist(static_cast<hid_t>(ds));
			if (dcpl_id < 0) return false;
			H5D_layout_t layout = H5Pget_layout(dcpl_id);
			H5Pclose(dcpl_id);
			return layout == H5D_CHUNKED;
		}();
		if( use_pipeline ){
			// Phase 1.3.3 — if the file's FAPL has h5::threads{N}, route
			// reads through a local pool_pipeline_t.  Currently pool_pipeline_t::
			// read_chunk_impl is synchronous (parallel decompress is Phase 1.5+),
			// so the FAPL-pool branch is semantically equivalent to the DAPL
			// path today; the structure is in place for the read-ahead
			// optimization to land later without changing call sites.
			hid_t fid  = H5Iget_file_id(static_cast<hid_t>(ds));
			hid_t fapl = H5Fget_access_plist(fid);
			auto pool  = h5::impl::resolve_worker_pool(fapl);
			if (pool) {
				const unsigned cap = h5::impl::resolve_backpressure(
					fapl, pool->worker_count());
				h5::impl::pool_pipeline_t pipe(std::move(pool), cap);
				h5::dcpl_t dcpl{H5Dget_create_plist(static_cast<hid_t>(ds))};
				hid_t type_id  = H5Dget_type(static_cast<hid_t>(ds));
				size_t elem_sz = H5Tget_size(type_id);
				H5Tclose(type_id);
				pipe.set_cache(dcpl, elem_sz);
				pipe.read(ds, offset, stride, block, count, dxpl, ptr);
			} else {
				h5::impl::pipeline_t<impl::basic_pipeline_t>* filters;
				H5Pget(dapl, H5CPP_DAPL_HIGH_THROUGHPUT, &filters);
				filters->read(ds, offset, stride, block, count, dxpl, ptr);
			}
			H5Pclose(fapl);
			H5Fclose(fid);
		}else{
			h5::sp_t mem_space = h5::create_simple( size );
			h5::select_all( mem_space );
			h5::select_hyperslab( file_space, offset, stride, count, block);

			H5CPP_CHECK_NZ( H5Dread(
					static_cast<hid_t>( ds ), static_cast<hid_t>(mem_type), static_cast<hid_t>(mem_space),
					static_cast<hid_t>(file_space),	static_cast<hid_t>(dxpl), ptr ), h5::error::io::dataset::read, h5::error::msg::read_dataset);
		}
	} catch ( const std::runtime_error& err ){
		throw h5::error::io::dataset::read( err.what() );
	}

 	/** \func_read_hdr
 	*  Updates the content of passed **ptr** pointer, which must have enough memory space to receive data.
	*  Optional arguments **args:= h5::offset | h5::stride | h5::count | h5::block** may be specified for partial IO, 
	*  to describe the retrieved hyperslab from hdf5 file space. Default case is to select and retrieve all elements from dataset. 
	*  **h5::dxpl_t** provides control to datatransfer.
	* \code
	* h5::fd_t fd = h5::open("myfile.h5", H5F_ACC_RDWR);
	* h5::ds_t ds = h5::open(fd,"path/to/dataset");
	* std::vector<float> myvec(10*10);
	* auto err = h5::read( fd, "path/to/dataset", myvec.data(), h5::count{10,10}, h5::offset{5,0} );	
	* \endcode  
	* \par_file_path \par_dataset_path \par_ptr \par_offset \par_stride \par_count \par_block \par_dxpl \tpar_T \returns_err
 	*/ 
	template<class T, class... args_t>
	inline void read( const h5::fd_t& fd, const std::string& dataset_path, T* ptr, args_t&&... args ){
		const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
		h5::ds_t ds = h5::open(fd, dataset_path, dapl ); // will throw its exception
		::h5::read<T>(ds, ptr, args...);
	}

 	/** \func_read_hdr
 	*  Updates the content of passed **ptr** pointer, which must have enough memory space to receive data.
	*  Optional arguments **args:= h5::offset | h5::stride | h5::count | h5::block** may be specified for partial IO,
	*  to describe the retrieved hyperslab from hdf5 file space. Default case is to select and retrieve all elements from dataset. 
	* \code
	* h5::fd_t fd = h5::open("myfile.h5", H5F_ACC_RDWR);
	* h5::ds_t ds = h5::open(fd,"path/to/dataset");
	* std::vector<float> myvec(10*10);
	* auto err = h5::read( fd, "path/to/dataset", myvec.data(), h5::count{10,10}, h5::offset{5,0} );	
	* \endcode  
	* \par_file_path \par_dataset_path \par_ptr \par_offset \par_stride \par_count \par_block \tpar_T \returns_err
 	*/ 
	template<class T, class... args_t>
	inline void read( const std::string& file_path, const std::string& dataset_path,T* ptr, args_t&&... args ){
		h5::fd_t fd = h5::open( file_path, H5F_ACC_RDWR );
		::h5::read( fd, dataset_path, ptr, args...);
	}


	/***************************  REFERENCE *****************************/
 	/** \func_read_hdr
 	*  Updates the content of passed **ref** reference, which must have enough memory space to receive data.
	*  Optional arguments **args:= h5::offset | h5::stride | h5::count | h5::block** may be specified for partial IO,
	*  to describe the retrieved hyperslab from hdf5 file space. Default case is to select and retrieve all elements from dataset. 
	* \code
	* h5::fd_t fd = h5::open("myfile.h5", H5F_ACC_RDWR);
	* h5::ds_t ds = h5::open(fd,"path/to/dataset");
	* std::vector<float> myvec(10*10);
	* auto err = h5::read( fd, "path/to/dataset", myvec, h5::offset{5,0} );	
	* \endcode  
	* \par_ds \par_ref \par_offset \par_stride  \par_block \tpar_T \returns_err
 	*/ 
	template<class T,  class... args_t>
		inline void read( const h5::ds_t& ds, T& ref, args_t&&... args ){
	// passed 'ref' contains memory size and element type, let's extract them
	// and delegate forward  
		using tcount  = typename arg::tpos<const h5::count_t&,const args_t&...>;
		using element_type = typename impl::decay<T>::type;

		static_assert( !tcount::present,
				"h5::count_t{ ... } is already present when passing arg by reference, did you mean to pass by pointer?" );
		// get 'count' and underlying type 
		h5::count_t count = impl::size(ref);
		element_type* ptr = impl::data(ref);
		::h5::read<element_type>(ds, ptr, count, args...);
	}
 	/** \func_read_hdr
 	*  Updates the content of passed **ref** reference, which must have enough memory space to receive data.
	*  Optional arguments **args:= h5::offset | h5::stride | h5::count | h5::block** may be specified for partial IO, 
	*  to describe the retrieved hyperslab from hdf5 file space. Default case is to select and retrieve all elements from dataset. 
	* \code
	* std::vector<float> myvec(10*10);
	* h5::fd_t fd = h5::open("myfile.h5", H5F_ACC_RDWR);
	* auto err = h5::read( fd, "path/to/dataset", myvec, h5::offset{5,0} );	
	* \endcode  
	* \par_fd \par_dataset_path \par_ref \par_offset \par_stride  \par_block \tpar_T \returns_err
 	*/ 
	template<class T,  class... args_t> // dispatch to above
		void read( const h5::fd_t& fd,  const std::string& dataset_path, T& ref, args_t&&... args ){

		const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
		h5::ds_t ds = h5::open(fd, dataset_path, dapl );
		::h5::read<T>(ds, ref, args...);
	}

 	/** \func_read_hdr
 	*  Updates the content of passed **ref** reference, which must have enough memory space to receive data.
	*  Optional arguments **args:= h5::offset | h5::stride | h5::count | h5::block** may be specified for partial IO,
	*  to describe the retrieved hyperslab from  hdf5 file space. Default case is to select and retrieve all elements from dataset. 
	* \code
	* std::vector<float> myvec(10*10);
	* auto err = h5::read( "path/to/file.h5", "path/to/dataset", myvec, h5::offset{5,0} );	
	* \endcode  
	* \par_file_path \par_dataset_path \par_ref \par_offset \par_stride \par_block \tpar_T \returns_err
 	*/ 
	template<class T, class... args_t> // dispatch to above
	void read( const std::string& file_path, const std::string& dataset_path, T& ref, args_t&&... args ){

		h5::fd_t fd = h5::open( file_path, H5F_ACC_RDWR );
		::h5::read( fd, dataset_path, ref, args...);
	}

	/***************************  OBJECT *****************************/
 	/** \func_read_hdr
 	*  Direct read from an opened dataset descriptor that returns the entire data space wrapped into the object specified. 
	*  Optional arguments **args:= h5::offset | h5::stride | h5::count | h5::block** may be specified for partial IO, 
	*  to describe the retrieved hyperslab from hdf5 file space. Default case is to select and retrieve all elements from dataset. 
	* \code
	* h5::fd_t fd = h5::open("myfile.h5", H5F_ACC_RDWR);
	* auto vec = h5::read<std::vector<float>>( fd, "path/to/dataset",	h5::count{10,10}, h5::offset{5,0} );	
	* \endcode  
	* \par_ds \par_offset \par_stride \par_count \par_block \tpar_T \returns_object 
 	*/
	template<class T, class D=typename impl::decay<T>::type, class... args_t>
	inline std::enable_if_t<!std::is_same_v<D,std::string>,
	T> read( const h5::ds_t& ds, args_t&&... args ){
	// if 'count' isn't specified use the one inside the hdf5 file, once it is obtained
	// collapse dimensions to the rank of the object returned and create this T object
	// update the content by we're good to go, since stride and offset can be processed in the 
	// update step
		using tcount  = typename arg::tpos<const h5::count_t&,const args_t&...>;
		using element_type    = typename impl::decay<T>::type;

		h5::count_t size;
		const h5::count_t& count = arg::get(size, args...);
		h5::block_t  default_block{1,1,1,1,1,1,1};
		const h5::block_t& block = arg::get( default_block, args...);

		if constexpr( !tcount::present ){ // read count ::= current_dim of file space 
			h5::sp_t file_space = h5::get_space( ds );
			h5::get_simple_extent_dims(file_space, size);
		} else {
			for(int i=0;i<count.rank;i++) size[i] = count[i] * block[i];
			size.rank = count.rank;
		}

		using traits = h5::meta::access_traits_t<T>;
		constexpr auto kind = traits::kind;
		if constexpr (kind == h5::meta::access_t::iterators) {
			size_t n = 1;
			for (int i = 0; i < size.rank; ++i) n *= size[i];
			std::vector<element_type> flat(n);
			::h5::read<element_type>(ds, flat.data(), count, args...);
			if constexpr (std::is_same_v<T, std::forward_list<element_type>>) {
				T result;
				result.assign(flat.begin(), flat.end());
				return result;
			} else {
				return T(flat.begin(), flat.end());
			}
		} else {
			T ref = impl::get<T>::ctor( count );
			element_type *ptr = impl::data( ref );
			::h5::read<element_type>(ds, ptr, count, args...);
			return ref;
		}
	}
	/***************************  STRING *****************************/
 	/** \func_read_hdr
 	*  Direct read from an opened dataset descriptor that returns the entire data space wrapped into the object specified. 
	*  Optional arguments **args:= h5::offset | h5::stride | h5::count | h5::block** may be specified for partial IO, 
	*  to describe the retrieved hyperslab from hdf5 file space. Default case is to select and retrieve all elements from dataset. 
	* \code
	* h5::fd_t fd = h5::open("myfile.h5", H5F_ACC_RDWR);
	* auto vec = h5::read<std::vector<std::string>>( fd, "path/to/variable_length_string",	h5::count{10,10}, h5::offset{5,0} );	
	* \endcode  
	* \par_ds \par_offset \par_stride \par_count \par_block \tpar_T \returns_object 
 	*/

	template<class T, class D=typename impl::decay<T>::type, class... args_t>
	inline std::enable_if_t<std::is_same_v<D,std::string>,
	T> read( const h5::ds_t& ds, args_t&&... args ){
	// if 'count' isn't specified use the one inside the hdf5 file, once it is obtained
	// collapse dimensions to the rank of the object returned and create this T object
	// update the content by we're good to go, since stride and offset can be processed in the 
	// update step
		using tcount  = typename arg::tpos<const h5::count_t&,const args_t&...>;

		h5::count_t size;
		const h5::count_t& count = arg::get(size, args...);
		h5::block_t  default_block{1,1,1,1,1,1,1};
		const h5::block_t& block = arg::get( default_block, args...);

		if constexpr( !tcount::present ){ // read count ::= current_dim of file space 
			h5::sp_t file_space = h5::get_space( ds );
			h5::get_simple_extent_dims(file_space, size);
		} else {
			for(int i=0;i<count.rank;i++) size[i] = count[i] * block[i];
			size.rank = count.rank;
		}

		h5::offset_t  default_offset{0,0,0,0,0,0,0}; // must match H5CPP_MAX_RANK=7
		const h5::offset_t& offset = arg::get( default_offset, args...);

		h5::stride_t  default_stride{1,1,1,1,1,1,1};
		const h5::stride_t& stride = arg::get( default_stride, args...);

		const h5::dxpl_t& dxpl = arg::get( h5::default_dxpl, args...);
		H5CPP_CHECK_PROP( dxpl, h5::error::property_list::misc, "invalid data transfer property" );

		h5::sp_t file_space = h5::get_space(ds);
	   	int rank = h5::get_simple_extent_ndims( file_space );

		if( rank != count.rank ) throw h5::error::io::dataset::read( H5CPP_ERROR_MSG( h5::error::msg::rank_mismatch ));
		h5::dt_t<char*> mem_type;
		hid_t dapl = h5::get_access_plist( ds );

		T ref = impl::get<T>::ctor( count );
		size_t nelem = impl::nelements(size);
		char ** ptr = static_cast<char **>(
										malloc( nelem * sizeof(char *)));
		h5::sp_t mem_space = h5::create_simple( size );
		h5::select_all( mem_space );
		h5::select_hyperslab( file_space, offset, stride, count, block);
		H5CPP_CHECK_NZ( H5Dread(
					static_cast<hid_t>( ds ), static_cast<hid_t>(mem_type), static_cast<hid_t>(mem_space),
					static_cast<hid_t>(file_space),	static_cast<hid_t>(dxpl), ptr ), h5::error::io::dataset::read, h5::error::msg::read_dataset);
		for(int i=0; i<nelem; i++)
				if( ptr[i] != nullptr )
						ref[i] = std::string( ptr[i] );
		h5::impl::reference::reclaim(mem_type, mem_space, H5P_DEFAULT, ptr);
		free(ptr);
		return ref;
	}


 	/** \func_read_hdr
 	*  Direct read from an opened file descriptor and dataset path that returns the entire data space wrapped into the object specified. 
	*  Optional arguments **args:= h5::offset | h5::stride | h5::count | h5::block** may be specified in any order for partial IO, 
	*  to describe the retrieved hyperslab from hdf5 file space. Default case is to select and retrieve all elements from dataset. 
	* \code
	* h5::fd_t fd = h5::open("myfile.h5", H5F_ACC_RDWR);
	* auto vec = h5::read<std::vector<float>>( fd, "path/to/dataset",	h5::count{10,10}, h5::offset{5,0} );	
	* \endcode  
	* \par_fd \par_dataset_path \par_offset \par_stride \par_count \par_block \tpar_T \returns_object 
 	*/
	template<class T, class... args_t> // dispatch to above
	inline T read( hid_t fd, const std::string& dataset_path, args_t&&... args ){

		const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
		h5::ds_t ds = h5::open(fd, dataset_path, dapl );
		return ::h5::read<T>(ds, args...);
	}
 	/** \func_read_hdr
 	*  Direct read from file and dataset path that returns the entire data space wrapped into the object specified.
	*  Optional arguments **args:= h5::offset | h5:stride | h5::count | h5::block** may be specified for partial IO, to describe
	 *  the retrieved hyperslab from  hdf5 file space. Default case is to select and retrieve all elements from dataset.
	* \code
	* auto vec = h5::read<std::vector<float>>( "myfile.h5","path/to/dataset", h5::count{10,10}, h5::offset{5,0} );	
	* \endcode  
	* \par_file_path \par_dataset_path \par_offset \par_stride  \par_count \par_block \tpar_T \returns_object 
 	*/
	template<class T, class... args_t> // dispatch to above
	inline T read(const std::string& file_path, const std::string& dataset_path, args_t&&... args ){
		h5::fd_t fd = h5::open( file_path, H5F_ACC_RDWR );
		return ::h5::read<T>( fd, dataset_path, args...);
	}
}
