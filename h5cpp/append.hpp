/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 
 */

#include <memory>
#ifndef  H5CPP_APPEND_H 
#define H5CPP_APPEND_H

// packet table template specialization with inheritence
namespace h5 {
	struct pt_t {
		pt_t( const h5::ds_t& handle );
		~pt_t();

		template<typename T>
		friend void append( h5::pt_t& ds, const T& ref);

		private:
		template <class T> void append( const T& ref );
		void flush();
		void save2file();

		void* ptr;
		hid_t ds, mem_type, file_type, mem_space, file_space;
		hsize_t current_dims[H5CPP_MAX_RANK],
				max_dims[H5CPP_MAX_RANK], chunk_dims[H5CPP_MAX_RANK],
				offset[H5CPP_MAX_RANK], count[H5CPP_MAX_RANK],
				chunk_size;
		size_t rank, type_size;
	};
}

//size_t H5Tget_size( hid_t dtype_id )

h5::pt_t::pt_t( const h5::ds_t& handle ) : ds( static_cast<hid_t>( handle) ){
	H5Iinc_ref(ds); // be sure you keep this alive
	hid_t file_space = H5Dget_space(ds);
		rank = H5Sget_simple_extent_dims(file_space, current_dims, max_dims);
	H5Sclose(file_space);


	hid_t plist =  H5Dget_create_plist(ds);
		H5Pget_chunk(plist,rank, chunk_dims );
	H5Pclose(plist);

	for(int i=0;i<rank;i++) count[i]=1,offset[i]=0;
	chunk_size=1; for(int i=0;i<rank;i++) chunk_size*=chunk_dims[i];

	file_type = H5Dget_type(ds);
	type_size =  H5Tget_size( file_type );
	ptr = calloc( chunk_size, type_size );

	mem_space = H5Screate_simple(1, &chunk_size, NULL );
	H5Sselect_all(mem_space);
}

h5::pt_t::~pt_t(){
		flush();
		H5Sclose(mem_space);
		H5Tclose(file_type);
		H5Dclose(ds);
		free(ptr);
}

template <class T>
void h5::pt_t::append( const T& ref ){

	size_t k = (++*current_dims -1)% chunk_size;
	static_cast<T*>(ptr)[k] = ref;
	if( k == chunk_size - 1  )
		*count=chunk_size, save2file();
}

void h5::pt_t::flush(){
	*count = *current_dims % chunk_size;
	if( *count ){ // there is left over then flush it
		// select the remainder of the memory
		H5Sselect_hyperslab(mem_space, H5S_SELECT_SET,offset, NULL, count, NULL);
		save2file();
	}
}

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

namespace h5 {
	/** @ingroup io-append
	 * @brief extends HDF5 dataset along the first/slowest growing dimension, then writes passed object to the newly created space
	 * @param ctx context for dataset @see context
	 * @param ref T type const reference to object appended
	 * @tparam T dimensions must match the dimension of HDF5 space upto rank-1 
	 */ 
	template<typename T> inline void append( h5::pt_t& ds, const T& ref){
		ds.append( ref );
	}
}
#endif
