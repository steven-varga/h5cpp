/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_DWRITE_HPP
#define H5CPP_DWRITE_HPP

namespace h5 {
 	/** \func_write_hdr
 	*  TODO: write doxy for here  
	*  \par_file_path \par_dataset_path \par_ref \par_offset \par_count \par_dxpl \tpar_T \returns_herr 
 	*/ 
	template <class T>
	void write( const h5::ds_t& ds, const h5::sp_t& mem_space, const h5::sp_t& file_space, const h5::dxpl_t& dxpl, const T* ptr  ){
		H5CPP_CHECK_PROP( dxpl, h5::error::io::dataset::write, "invalid data transfer property" );
		using element_t = typename h5::impl::decay<T>::type;
		h5::dt_t<element_t> type;
		H5CPP_CHECK_NZ(
				H5Dwrite( static_cast<hid_t>( ds ), type, mem_space, file_space, static_cast<hid_t>(dxpl), ptr),
							h5::error::io::dataset::write, h5::error::msg::write_dataset);
	}
 	/** \func_write_hdr
 	*  TODO: write doxy for here  
	*  \par_file_path \par_dataset_path \par_ref \par_offset \par_count \par_dxpl \tpar_T \returns_herr 
 	*/ 
	template <class T, class... args_t>
	h5::ds_t write( const h5::ds_t& ds, const T* ptr,  args_t&&... args  ) try {

		// element types: pod | [signed|unsigned](int8 | int16 | int32 | int64) | float | double
		using tcount = typename arg::tpos<const h5::count_t&,const args_t&...>;
		static_assert( tcount::present,"h5::count_t{ ... } must be provided to describe T* memory region" );
		//static_assert( utils::is_supported<T>, "error: " H5CPP_supported_elementary_types );

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
		return ds;
	} catch ( const std::exception& err ){
		throw h5::error::io::dataset::write( err.what() );
	}

 	/** \func_write_hdr
 	*  TODO: write doxy for here  
	*  \par_file_path \par_dataset_path \par_ref \par_offset \par_count \par_dxpl \tpar_T \returns_herr 
 	*/ 
	template <class T, class... args_t>
	typename std::enable_if<!std::is_same<T,char**>::value,
	h5::ds_t>::type write( const h5::ds_t& ds, const T& ref,   args_t&&... args  ) try {
		// element types: pod | [signed|unsigned](int8 | int16 | int32 | int64) | float | double | std::string
		using element_t = typename impl::decay<T>::type;
		using tcount = typename arg::tpos<const h5::count_t&,const args_t&...>;
		h5::count_t default_count;

		default_count = impl::size( ref );
		const h5::count_t& count = arg::get(default_count, args...);

		// std::string is variable length
		if constexpr( std::is_same<std::string,element_t>::value ){
			std::vector<char*> ptr;
			try {
				for( const auto& reference:ref)
            		ptr.push_back( strdup( reference.data()) );
			} catch( ... ){
				throw h5::error::io::dataset::write( h5::error::msg::mem_alloc );
			}
			// will throw it's own
 			h5::write(ds, ptr.data(), count, args...);
        	for( auto p:ptr ) free(p);
			return ds;
		}  else // ditto, throws error
			return h5::write<element_t>(ds, impl::data(ref), count, args...  );
	} catch ( const std::runtime_error& err ){
		throw h5::error::io::dataset::write( err.what() );
	}
	/** \func_write_hdr
 	*  TODO: write doxy for here  
	*  \par_file_path \par_dataset_path \par_ref \par_offset \par_count \par_dxpl \tpar_T \returns_herr 
 	*/ 
	template <class T, class... args_t>
	h5::ds_t write( const h5::fd_t& fd, const std::string& dataset_path, const T* ptr,  args_t&&... args  ){

		using tcount  = typename arg::tpos<const h5::count_t&,const args_t&...>;
		using toffset = typename arg::tpos<const h5::offset_t&,const args_t&...>;
		using tstride = typename arg::tpos<const h5::stride_t&,const args_t&...>;
		using tblock  = typename arg::tpos<const h5::block_t&,const args_t&...>;
		using tdapl   = typename arg::tpos<const h5::dapl_t&,const args_t&...>;
		using tcurrent_dims = typename arg::tpos<const h5::current_dims_t&,const args_t&...>;

		static_assert( tcount::present,"h5::count_t{ ... } must be provided to describe T* memory region" );

		h5::current_dims_t def_current_dims;

		auto tuple = std::forward_as_tuple(args...);
		const h5::count_t& count = std::get<tcount::value>( tuple );
		int rank = def_current_dims.rank =  count.rank;

		if constexpr( !tcurrent_dims::present ){
			for(int i =0; i < rank; i++ )
				def_current_dims[i] = count[i];
			if constexpr( tstride::present ){ //STRIDE
				const h5::stride_t& stride =  std::get<tstride::value>( std::forward_as_tuple( args...) );
				for(int i=0; i < rank; i++)
					def_current_dims[i] *= stride[i];
			}
			if constexpr( toffset::present ){ //OFFSET
				const h5::offset_t& offset =  std::get<toffset::value>( std::forward_as_tuple( args...) );
				for(int i=0; i < rank; i++)
					def_current_dims[i]+=offset[i];
			}
		}
		const h5::current_dims_t& current_dims  = arg::get(def_current_dims, args... );
		const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);

		h5::mute();
		//NOTE: this call is unchecked on purpose, return value -1 means the path doesn't exist along to 
		//queried leaf node. The missing path will be created by h5::create 
		h5::ds_t ds = (H5Lexists(fd, dataset_path.c_str(), H5P_DEFAULT ) > 0) ? // will throw error
			h5::open( fd, dataset_path, dapl) : h5::create<T>(fd, dataset_path, args..., current_dims );
		h5::unmute();
 		h5::write<T>(ds, ptr,  args...);
		return ds;
	}

 	/** \func_write_hdr
 	*  TODO: write doxy for here  
	*  \par_file_path \par_dataset_path \par_ref \par_offset \par_count \par_dxpl \tpar_T \returns_herr 
 	*/ 
	template <class T, class... args_t>
	h5::ds_t write( const h5::fd_t& fd, const std::string& dataset_path, const T& ref,  args_t&&... args  ){
		using tcount  = typename arg::tpos<const h5::count_t&,const args_t&...>;
		using toffset = typename arg::tpos<const h5::offset_t&,const args_t&...>;
		using tstride = typename arg::tpos<const h5::stride_t&,const args_t&...>;
		using tblock = typename arg::tpos<const h5::block_t&,const args_t&...>;
		using tcurrent_dims = typename arg::tpos<const h5::current_dims_t&,const args_t&...>;
		using tdapl   = typename arg::tpos<const h5::dapl_t&,const args_t&...>;

		int rank = impl::rank<T>::value;

		using element_t = typename impl::decay<T>::type;
		h5::current_dims_t def_current_dims;
		h5::count_t count = impl::size( ref );

		rank = def_current_dims.rank = count.rank;

		if constexpr( !tcurrent_dims::present ){
			for(int i =0; i < rank; i++ )
				def_current_dims[i] = count[i];
			if constexpr( tstride::present ){
				const h5::stride_t& stride =  std::get<tstride::value>( std::forward_as_tuple( args...) );
				if constexpr( tblock::present ){ // tricky, we have block as well
					const h5::block_t& block =  std::get<tblock::value>( std::forward_as_tuple( args...) );
					for(int i=0; i < rank; i++)
						def_current_dims[i] *= (stride[i] - block[i] + 1);
				} else
					for(int i=0; i < rank; i++)
						def_current_dims[i] *= stride[i];
			}
			if constexpr( toffset::present ){
				const h5::offset_t& offset =  std::get<toffset::value>( std::forward_as_tuple( args...) );
				for(int i=0; i < rank; i++)
					def_current_dims[i]+=offset[i];
			}
		}
		const h5::current_dims_t& current_dims  = arg::get(def_current_dims, args... );
		const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);

		h5::mute();

		//NOTE: this call is unchecked on purpose, return value -1 means the path doesn't exist along to 
		//queried leaf node. The missing path will be created by h5::create 
		h5::ds_t ds = ( H5Lexists(fd, dataset_path.c_str(), H5P_DEFAULT ) > 0 ) ?
			h5::open( fd, dataset_path, dapl) : h5::create<element_t>(fd, dataset_path, args..., current_dims );
		h5::unmute();
 		return h5::write<T>(ds, ref,  args...);
	}

   /** \func_write_hdr
 	*  \func_write_desc \par_file_path \par_dataset_path 
	*  \par_offset \par_stride \par_count \par_block  \par_dxpl \tpar_T \returns_herr 
 	*/ 
	template <class... args_t>
	h5::ds_t write( const std::string& file_path, const std::string& dataset_path, args_t&&... args  ){
		//TODO: check if exists create if doesn't
		h5::fd_t fd = h5::open( file_path, H5F_ACC_RDWR, h5::default_fapl );
		return h5::write( fd, dataset_path, args...);
	}
}
#endif
