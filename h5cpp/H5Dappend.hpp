/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 
 */

#ifndef  H5CPP_DAPPEND_HPP
#define H5CPP_DAPPEND_HPP

// packet table template specialization with inheritance
namespace h5 {
	struct pt_t {
		pt_t();
		pt_t( const h5::ds_t& handle );
		pt_t( const h5::pt_t& pt ){
			init_ref( pt );
			init_space();
		};

		~pt_t();
		pt_t& operator=( const h5::pt_t& pt ){
			init_ref( pt );
			init_space();
			return *this;
		};
		pt_t& operator=( const h5::pt_t&& pt ){
			init_ref( pt );
			init_space();
			return *this;
		};

		template<typename T>
		friend void append( h5::pt_t& ds, const T& ref);

		private:
		template <class T> void append( const T& ref );
		void flush();
		void save2file();
		void init_ref( const h5::pt_t& pt );
		void init_space();
		void cleanup();
		void* ptr;
		hid_t ds, mem_type, file_type, mem_space, file_space;
		hsize_t current_dims[H5CPP_MAX_RANK],
				max_dims[H5CPP_MAX_RANK], chunk_dims[H5CPP_MAX_RANK],
				offset[H5CPP_MAX_RANK], count[H5CPP_MAX_RANK],
				chunk_size;
		size_t rank, type_size;
	};
}

inline h5::pt_t::pt_t() : ds(H5I_UNINIT){}

inline
h5::pt_t::pt_t( const h5::ds_t& handle ) : ds( static_cast<hid_t>( handle) ){
	if( !H5Iis_valid(ds) ) return;

	if( H5Iinc_ref(ds) < 0 ) throw h5::error::io::dataset::misc(H5CPP_ERROR_MSG("reference counting failure..."));
	// be sure you keep this alive
	hid_t file_space = H5Dget_space(ds);
	if( file_space < 0) throw h5::error::io::dataset::open(H5CPP_ERROR_MSG("invalid filespace..."));
	rank = H5Sget_simple_extent_dims(file_space, current_dims, max_dims);
	if( rank < 0  )
		H5Sclose(file_space), throw h5::error::io::dataset::open(H5CPP_ERROR_MSG("invalid rank..."));
	H5Sclose(file_space);

	hid_t plist =  H5Dget_create_plist(ds);
		if( H5Pget_chunk(plist,rank, chunk_dims ) < 0 ) H5Sclose(file_space), H5Pclose(plist),
			throw h5::error::io::dataset::open(H5CPP_ERROR_MSG("invalid chunk dimensions..."));
	H5Pclose(plist);

	for(int i=0;i<rank;i++) count[i]=1,offset[i]=0;
	chunk_size=1; for(int i=0;i<rank;i++) chunk_size*=chunk_dims[i];

	init_space();
}

inline
h5::pt_t::~pt_t(){
	if( !H5Iis_valid( ds )) 
		return;
	flush();
	cleanup();
}

template <class T>
void h5::pt_t::append( const T& ref ){
	size_t k = (++*current_dims -1) % chunk_size;
	static_cast<T*>(ptr)[k] = ref;
	if( k == chunk_size - 1  )
		*count=chunk_size, save2file();
}
inline
void h5::pt_t::flush(){
	*count = *current_dims % chunk_size;
	if( *count ){ // there is left over then flush it
		// select the remainder of the memory
		H5Sselect_hyperslab(mem_space, H5S_SELECT_SET,offset, NULL, count, NULL);
		save2file();
	}
}

inline
void h5::pt_t::save2file( ){

	H5Dset_extent(ds, current_dims ); 		// make space
	*current_dims -= *count; 				// 
	// select target
	file_space = H5Dget_space(ds);
		H5Sselect_hyperslab(file_space, H5S_SELECT_SET, current_dims, NULL, count, NULL);
		H5Dwrite(ds, file_type, mem_space, file_space,  H5P_DEFAULT, ptr);
	H5Sclose(file_space);
	*current_dims += *count;
}

inline void h5::pt_t::init_ref( const h5::pt_t& pt ){
	rank = pt.rank; chunk_size = pt.chunk_size;
	type_size = pt.type_size; ds = pt.ds;
	// we don't have an object yet, nothing to clean up
	if( H5Iinc_ref(ds) < 0 ) throw h5::error::io::dataset::misc(H5CPP_ERROR_MSG("reference counting failure..."));

	for(int i=0; i<rank; i++){
		current_dims[i] = pt.current_dims[i];
		max_dims[i] = pt.max_dims[i];
		chunk_dims[i] = pt.chunk_dims[i];
		offset[i] = pt.offset[i];
		count[i] = pt.count[i];
	}

}

inline void h5::pt_t::init_space( ){
	file_type = H5Dget_type(ds);
	if( file_type < 0 )
		throw h5::error::io::dataset::open(H5CPP_ERROR_MSG("couldn't obtain datatype of dataset..."));
	type_size =  H5Tget_size( file_type );
	if( type_size < 0 )
	throw h5::error::io::dataset::open(H5CPP_ERROR_MSG("couldn't obtain datatype size..."));

	ptr = calloc( chunk_size, type_size );
	if( ptr == NULL )
		throw h5::error::io::dataset::misc(H5CPP_ERROR_MSG("memory allocation failure, too large chunk?..."));

	mem_space = H5Screate_simple(1, &chunk_size, NULL );
	if ( mem_space < 0 ) free(ptr),
		throw h5::error::io::dataset::open(H5CPP_ERROR_MSG("invalid memory space..."));
	if ( H5Sselect_all(mem_space) < 0 ) free(ptr),
		throw h5::error::io::dataset::open(H5CPP_ERROR_MSG("invalid memory space..."));
}

inline void h5::pt_t::cleanup(){
	// start here, and go by priority, leaving dataset closure for last, as it is the most likely to fail
	free(ptr);

	if( H5Sclose(mem_space) < 0 )
		throw h5::error::io::dataset::close(H5CPP_ERROR_MSG("unable to close dataspace..."));
	if( H5Tclose(file_type) < 0 )
		throw h5::error::io::dataset::close(H5CPP_ERROR_MSG("unable to close dataset type..."));
	if( H5Dclose(ds) < 0 ) 
		throw h5::error::io::dataset::close(H5CPP_ERROR_MSG("unable to close dataset..."));
}


namespace h5 {
	/** @ingroup io-append
	 * @brief extends HDF5 dataset along the first/slowest growing dimension, then writes passed object to the newly created space
	 * @param pt packet_table descriptor
	 * @param ref T type const reference to object appended
	 * @tparam T dimensions must match the dimension of HDF5 space upto rank-1 
	 */ 
	template<typename T> inline void append( h5::pt_t& pt, const T& ref){
		pt.append( ref );
	}
}
#endif
