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


namespace h5{
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


	inline h5::dcpl_t create_dcpl( const h5::ds_t& ds ){
		hid_t dcpl;
		H5CPP_CHECK_NZ(
				(dcpl =  H5Dget_create_plist( static_cast<hid_t>(ds))),std::runtime_error, h5::error::msg::create_dcpl );
		return h5::dcpl_t{ dcpl };
	}

	inline void get_chunk_dims( const h5::dcpl_t& dcpl,  h5::chunk_t& chunk_dims ){
		H5CPP_CHECK_NZ( H5Pget_chunk( static_cast<hid_t>(dcpl), chunk_dims.rank, *chunk_dims ), std::runtime_error,
					h5::error::msg::get_chunk_dims );
	}
	inline h5::chunk_t get_chunk_dims( const h5::dcpl_t& dcpl ){
		h5::chunk_t chunk_dims;
		get_chunk_dims( dcpl, chunk_dims );
		return chunk_dims;
	}
	inline h5::dt_t get_type(const h5::ds_t& ds ){
		hid_t dataset_type;
		H5CPP_CHECK_NZ( (dataset_type = H5Dget_type( static_cast<hid_t>(ds))), std::runtime_error, h5::error::msg::get_dataset_type );
		return h5::dt_t{ dataset_type };
	}
	inline size_t get_size( const h5::dt_t& type ){
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
		hid_t mem_space;
		H5CPP_CHECK_NZ( (mem_space = H5Screate_simple(dim.rank, *dim, nullptr )), std::runtime_error, h5::error::msg::create_memspace);
		return h5::sp_t{mem_space};
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
		H5CPP_CHECK_NZ(
				H5Sselect_hyperslab( static_cast<hid_t>(sp), H5S_SELECT_SET, *offset, NULL, *count, NULL),
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

	inline void writeds(const h5::ds_t& ds,
			const h5::dt_t& file_type, const h5::sp_t& mem_space, const h5::sp_t& file_space,
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

	inline h5::ds_t createds(const h5::fd_t& fd, const std::string& path, const h5::dt_t& type,
		  const h5::sp_t& sp, const h5::lcpl_t& lcpl, const h5::dcpl_t& dcpl, const h5::dapl_t& dapl ){
		hid_t ds;
		H5CPP_CHECK_NZ(( ds = H5Dcreate2( static_cast<hid_t>(fd), path.data(), type, static_cast<hid_t>(sp),
								static_cast<hid_t>(lcpl), static_cast<hid_t>(dcpl), static_cast<hid_t>(dapl) )),
			   std::runtime_error,	h5::error::msg::create_dataset );
		return h5::ds_t{ds};
	}

}

template <class T>
inline std::ostream& operator<<(std::ostream& os, const h5::impl::array<T>& arr){
	os << "{";
		for(int i=0;i<arr.rank; i++){
			char sep = i != arr.rank - 1  ? ',' : '}';
			if( arr[i] < std::numeric_limits<hsize_t>::max() )
				os << arr[i] << sep;
			else
				os << "max" << sep;
		}
	return os;
}

inline
std::ostream& operator<<(std::ostream &os, const h5::sp_t& sp) {
	//htri_t H5Sis_regular_hyperslab( hid_t space_id ) 1.10.0
	//herr_t H5Sget_select_bounds(hid_t space_id, hsize_t *start, hsize_t *end )
	// hssize_t H5Sget_select_npoints( hid_t space_id )
	hid_t id = static_cast<hid_t>( sp );
	#if H5_VERSION_GE(1,10,0)

	#endif

	h5::offset_t start,end;
	h5::current_dims_t current_dims;
	h5::max_dims_t max_dims;
	unsigned rank = h5::get_simple_extent_dims( sp, current_dims, max_dims);
	hsize_t total_elements = H5Sget_simple_extent_npoints( id );
 	herr_t err = H5Sget_select_bounds(id, *start, *end);
	start.rank = end.rank = rank;
	hsize_t nblocks =  H5Sget_select_hyper_nblocks( id );
	hsize_t ncoordinates = 2*rank*nblocks;

    os << "[rank]\t" << rank << "\t[total elements]\t" << total_elements << std::endl;
   	os << "[dimensions]\tcurrent: " << current_dims << "\tmaximum: " << max_dims << std::endl;
	os << "[selection]\tstart: " << start << "\tend:" << end << std::endl;

	h5::impl::unique_ptr<hsize_t> buffer{
			static_cast<hsize_t*>( std::calloc( ncoordinates, sizeof(hsize_t))) };
	H5CPP_CHECK_NZ( H5Sget_select_hyper_blocklist(id, 0, nblocks, buffer.get() ), std::runtime_error,	
			"couldn't retrieve selected block");
	os << "[selected block count]\t" << nblocks <<std::endl;
	os << "[selected blocks]\t";
	for( int i=0; i<nblocks; i++){
		os << "[{";
		for( int j=0; j<rank; j++) os << *( buffer.get() + i*2*rank+j ) << (j < rank-1 ? "," : "}{");
		for( int j=rank; j<2*rank; j++) os << *( buffer.get() + i*2*rank+j ) << ( j < 2*rank-1 ? "," : "}");
		os << "] ";
	}
	return os;
}
#endif

