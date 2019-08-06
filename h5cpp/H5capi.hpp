/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_CAPI_HPP
#define  H5CPP_CAPI_HPP



/* rules:
 * h5::id_t{ hid_t } or direct initialization  doesn't increment reference count
 */ 
namespace h5 { namespace impl {
	struct free {
		template <typename T>
		void operator()(T *p) const {
			using T_ = typename std::remove_const<T>::type;
			std::free( const_cast<T_*>(p) );
		}
	};

	template <typename T>
		using unique_ptr = std::unique_ptr<T,h5::impl::free>;
}}


namespace h5 {
	inline ::hid_t get_access_plist( const ds_t& ds ){
		return ds.dapl;
	}
	template<class T> 
	inline bool is_valid( const T& v ){
		int val;
		H5CPP_CHECK_NZ( (val = H5Iis_valid( static_cast<hid_t>( v ))), std::runtime_error, h5::error::msg::inc_ref );
		return val;
	}

	inline h5::sp_t get_space( const ds_t& ds ){
		hid_t ds_ = static_cast<hid_t>(ds );
		hid_t file_space;
		H5CPP_CHECK_NZ( (file_space = H5Dget_space( ds_ )), std::runtime_error,	 h5::error::msg::get_filespace );
		return h5::sp_t{file_space}; // direct initialization doesn't make copy
	}

	inline unsigned get_simple_extent_dims( const h5::sp_t&file_space, h5::current_dims& current_dims, h5::max_dims_t& max_dims ){
		int rank;
		H5CPP_CHECK_NZ( (rank = H5Sget_simple_extent_dims(static_cast<hid_t>( file_space ), *current_dims, *max_dims)),
				std::runtime_error, h5::error::msg::get_simple_extent_dims );
		// don't forget to set rank
		current_dims.rank = max_dims.rank = rank;
		return static_cast<unsigned>( rank );
	}
	inline unsigned get_simple_extent_dims( const h5::sp_t&file_space, hsize_t* current_dims, hsize_t* max_dims ){
		int rank;
		H5CPP_CHECK_NZ( (rank = H5Sget_simple_extent_dims(static_cast<hid_t>( file_space ), current_dims, max_dims)),
				std::runtime_error, h5::error::msg::get_simple_extent_dims );
		return static_cast<unsigned>( rank );
	}


	template <class T>
	inline unsigned get_simple_extent_dims( const h5::sp_t& file_space, T& current_dims ){
		int rank;
		H5CPP_CHECK_NZ( (rank = H5Sget_simple_extent_dims(static_cast<hid_t>( file_space ), *current_dims, NULL )),
				std::runtime_error, h5::error::msg::get_simple_extent_dims );
		// don't forget to set rank
		current_dims.rank = rank;
		return rank;
	}
	inline unsigned get_simple_extent_ndims( const h5::sp_t&file_space ){
		int rank;
		H5CPP_CHECK_NZ( (rank = H5Sget_simple_extent_ndims(static_cast<hid_t>( file_space ) )),
				std::runtime_error, h5::error::msg::get_simple_extent_dims );
		return static_cast<unsigned>( rank );
	}


	inline h5::dcpl_t get_dcpl( const h5::ds_t& ds ){
		hid_t dcpl;
		H5CPP_CHECK_NZ(
				(dcpl =  H5Dget_create_plist( static_cast<hid_t>(ds))),std::runtime_error, h5::error::msg::create_dcpl );
		return h5::dcpl_t{ dcpl };
	}
	inline h5::dapl_t get_dapl( const h5::ds_t& ds ){
		hid_t dapl;
		H5CPP_CHECK_NZ(
				(dapl =  H5Dget_access_plist( static_cast<hid_t>(ds))),std::runtime_error, h5::error::msg::create_dcpl );
		return h5::dapl_t{ dapl };
	}


	inline int get_chunk_dims( const h5::dcpl_t& dcpl,  h5::chunk_t& chunk_dims ){
		int rank;
		H5CPP_CHECK_NZ( (rank = H5Pget_chunk( static_cast<hid_t>(dcpl), H5CPP_MAX_RANK, *chunk_dims )), std::runtime_error,
					h5::error::msg::get_chunk_dims );
		return rank;
	}
	inline void get_chunk_dims( const h5::dcpl_t& dcpl,  hsize_t* chunk_dims ){
		H5CPP_CHECK_NZ( H5Pget_chunk( static_cast<hid_t>(dcpl), H5CPP_MAX_RANK, chunk_dims ), std::runtime_error,
					h5::error::msg::get_chunk_dims );
	}
	inline h5::chunk_t get_chunk_dims( const h5::dcpl_t& dcpl ){
		h5::chunk_t chunk_dims;
		get_chunk_dims( dcpl, chunk_dims );
		return chunk_dims;
	}
	template<class T>
	inline h5::dt_t<T> get_type(const h5::ds_t& ds ){
		hid_t dataset_type;
		H5CPP_CHECK_NZ( (dataset_type = H5Dget_type( static_cast<hid_t>(ds))), std::runtime_error, h5::error::msg::get_dataset_type );
		return h5::dt_t<T>{ dataset_type };
	}
	template<class T>
	inline size_t get_size( const h5::dt_t<T>& type ){
		size_t size;
		H5CPP_CHECK_NZ( (size =  H5Tget_size( static_cast<hid_t>(type) )), std::runtime_error, h5::error::msg::get_filetype_size );
		return size;
	}

	inline h5::sp_t create_simple( const hsize_t dim ){
		hid_t mem_space;
		H5CPP_CHECK_NZ( (mem_space = H5Screate_simple(1, &dim, nullptr )), std::runtime_error, h5::error::msg::create_memspace);
		return h5::sp_t{mem_space};
	}
	template <class T>
	inline h5::sp_t create_simple( const T& dim ){
		hid_t space;

		if( dim.rank > 0 ){
			H5CPP_CHECK_NZ( (space = H5Screate_simple(dim.rank, *dim, nullptr )), std::runtime_error, h5::error::msg::create_memspace);
		}else{ // scalar space
			H5CPP_CHECK_NZ( (space = H5Screate( H5S_SCALAR )), std::runtime_error, h5::error::msg::create_memspace);
		}
		return h5::sp_t{space};
	}
	inline h5::sp_t create_simple( const h5::current_dims& current_dims, const h5::max_dims& max_dims  ){
		return h5::sp_t{H5Screate_simple( current_dims.size(), current_dims.begin(), max_dims.begin() )};
	}


	inline void select_all(const h5::sp_t& sp){
		H5CPP_CHECK_NZ(
				H5Sselect_all(static_cast<hid_t>(sp)), std::runtime_error,	 h5::error::msg::select_memspace );
	}
	template<class T>
	inline void select_hyperslab(const h5::sp_t& sp, const T& offset, const h5::count_t& count ){
		hsize_t cnt[] =  {1,1,1,1,1,1,1,1};
		H5CPP_CHECK_NZ(
				H5Sselect_hyperslab( static_cast<hid_t>(sp), H5S_SELECT_SET, *offset, NULL, cnt, *count),
			   std::runtime_error,	h5::error::msg::select_hyperslab);
	}
	inline void select_hyperslab(const h5::sp_t& sp, const h5::offset_t& offset, const h5::stride_t& stride,
		   const h5::count_t& count, const h5::block_t& block ){
		H5CPP_CHECK_NZ(
				H5Sselect_hyperslab( static_cast<hid_t>(sp), H5S_SELECT_SET, *offset, *stride, *count, *block),
			   std::runtime_error,	h5::error::msg::select_hyperslab);
	}
	inline void set_extent(const h5::ds_t& ds, const h5::current_dims_t& dims ){
		H5CPP_CHECK_NZ(
				H5Dset_extent( static_cast<hid_t>(ds), *dims ),std::runtime_error,	 h5::error::msg::set_extent);
	}
	inline void set_extent(const h5::ds_t& ds, const hsize_t* dims ){
		H5CPP_CHECK_NZ(
				H5Dset_extent( static_cast<hid_t>(ds), dims ),std::runtime_error,	 h5::error::msg::set_extent);
	}
	template <class T>
	inline void writeds(const h5::ds_t& ds,
			const h5::dt_t<T>& file_type, const h5::sp_t& mem_space, const h5::sp_t& file_space,
			const void* ptr ){
		H5CPP_CHECK_NZ(
				H5Dwrite(static_cast<hid_t>(ds), static_cast<hid_t>(file_type),
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),  H5P_DEFAULT, ptr ), 
				std::runtime_error,	h5::error::msg::write_dataset );
	}

	inline void set_chunk( h5::dcpl_t& dcpl, const h5::chunk_t& chunk ){
		H5CPP_CHECK_NZ(
				H5Pset_chunk(static_cast<::hid_t>(dcpl), chunk.rank, *chunk ), std::runtime_error,	 h5::error::msg::set_chunk );
	}
	template<class T>
	inline h5::ds_t createds(const h5::fd_t& fd, const std::string& path, const h5::dt_t<T>& type,
		  const h5::sp_t& sp, const h5::lcpl_t& lcpl, const h5::dcpl_t& dcpl, const h5::dapl_t& dapl ){
		hid_t ds;
		H5CPP_CHECK_NZ(( ds = H5Dcreate2( static_cast<hid_t>(fd), path.data(), type, static_cast<hid_t>(sp),
								static_cast<hid_t>(lcpl), static_cast<hid_t>(dcpl), static_cast<hid_t>( dapl )  )),
			   std::runtime_error,	h5::error::msg::create_dataset );
		//FIXME: hack to carry prop over
		h5::ds_t ds_{ds};

		switch( H5Pget_layout(static_cast<::hid_t>( dcpl)) ){
			case H5D_COMPACT: break;
			case H5D_CONTIGUOUS: break;
			case H5D_CHUNKED:
				if( H5Pexist(dapl, H5CPP_DAPL_HIGH_THROUGPUT) ){
					// grab pointer to uninitialized pipeline
					h5::impl::pipeline_t<impl::basic_pipeline_t>* ptr;
					H5Pget(dapl, H5CPP_DAPL_HIGH_THROUGPUT, &ptr);
					hid_t type_id = H5Dget_type( static_cast<::hid_t>(ds) );
					size_t element_size = H5Tget_size( type_id );
					ptr->set_cache(dcpl, element_size);
				}
				break;
			case H5D_VIRTUAL: break;
		}
		ds_.dapl = static_cast<::hid_t>( dapl );
		return ds_;
	}

	inline void * get_fill_value(const h5::dcpl_t& dcpl, const h5::dt_t<void*>& type, size_t size){

		H5D_fill_value_t fill_value_status;
		H5Pfill_value_defined(static_cast<::hid_t>(dcpl), &fill_value_status);

		if( fill_value_status != H5D_FILL_VALUE_UNDEFINED ){
			void * buffer = malloc(size);
			H5Pget_fill_value(
					static_cast<::hid_t>(dcpl),
					static_cast<::hid_t>(type), buffer );
			return buffer;
		} else
			return NULL;
	}

	inline void * get_fill_value(const h5::ds_t& ds){
		h5::dt_t<void*> type = h5::get_type<void*>( ds );
		hsize_t size = h5::get_size( type );
		h5::dcpl_t dcpl = h5::get_dcpl( ds );

		return h5::get_fill_value(dcpl, type, size);
	}
}
#endif

