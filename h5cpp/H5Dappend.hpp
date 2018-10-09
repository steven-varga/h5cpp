/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 
 */

#ifndef  H5CPP_DAPPEND_HPP
#define H5CPP_DAPPEND_HPP

#define H5CPP_CHECK_NZ( call, msg ) if( call < 0 ) throw rollback_error(__FILE__,__LINE__, msg );
#define H5CPP_CHECK_NULL( call, msg ) if( call == NULL  ) throw rollback_error(__FILE__,__LINE__, msg );

// packet table template specialization with inheritance
namespace h5 {
	struct pt_t {
		pt_t();
		pt_t( const h5::ds_t& handle ); // conversion ctor
		// deep copy with own cache memory
		pt_t( const h5::pt_t& pt ) : h5::pt_t(pt.ds) {
		};
		~pt_t();
		pt_t& operator=( const h5::pt_t& pt ) = default;
		constexpr pt_t& operator=( h5::pt_t&& pt ) = delete;

		template<typename T>
		friend void append( h5::pt_t& ds, const T& ref);

		private:
		template <class T> void append( const T& ref );
		void flush();
		void save2file();
		void cleanup( const std::string& fname, unsigned line );
		h5::error::io::packet_table::misc rollback_error( const std::string& fname, unsigned line, const std::string& msg  );
		void* ptr;
		hid_t ds, plist, mem_type, file_type, mem_space, file_space;
		hsize_t current_dims[H5CPP_MAX_RANK],
				max_dims[H5CPP_MAX_RANK], chunk_dims[H5CPP_MAX_RANK],
				offset[H5CPP_MAX_RANK], count[H5CPP_MAX_RANK],
				chunk_size;
		size_t rank, type_size;
	};
}

/* initialized to invalid state
 * */
inline h5::pt_t::pt_t() :
	ds(H5I_UNINIT), rank(0),type_size{0}, ptr{NULL},
	mem_space{H5I_UNINIT}, file_space{H5I_UNINIT}, plist{H5I_UNINIT},
	mem_type{H5I_UNINIT}, file_type{H5I_UNINIT} {
		memset(count,1,H5CPP_MAX_RANK); 
		memset(offset, 0,H5CPP_MAX_RANK);
	}

// conversion ctor
inline
h5::pt_t::pt_t( const h5::ds_t& handle ) : pt_t() {
	ds = static_cast<hid_t>( handle);

	/*default ctor has an invalid state -- skip initialization */
	if( !H5Iis_valid(ds) ) return;

	H5CPP_CHECK_NZ( H5Iinc_ref(ds), h5::error::msg::inc_ref );
	H5CPP_CHECK_NZ( (file_space = H5Dget_space(ds)),  h5::error::msg::get_filespace );
	H5CPP_CHECK_NZ( (rank = H5Sget_simple_extent_dims(file_space, current_dims, max_dims)), h5::error::msg::get_rank );
	H5CPP_CHECK_NZ( H5Sclose(file_space), h5::error::msg::close_filespace );
	H5CPP_CHECK_NZ( (plist =  H5Dget_create_plist(ds)), h5::error::msg::create_property_list );
	H5CPP_CHECK_NZ( H5Pget_chunk(plist, rank, chunk_dims ), h5::error::msg::get_chunk_dims );
	H5CPP_CHECK_NZ( H5Pclose(plist), h5::error::msg::close_property_list);

	// chunk_dims and rank are initialized 
	chunk_size=1; for(int i=0;i<rank;i++) chunk_size*=chunk_dims[i];

	H5CPP_CHECK_NZ( (file_type = H5Dget_type(ds)), h5::error::msg::get_filetype );
	H5CPP_CHECK_NZ( (type_size =  H5Tget_size( file_type )), h5::error::msg::get_filetype_size );
	H5CPP_CHECK_NULL( (ptr = calloc( chunk_size, type_size )), h5::error::msg::mem_alloc );
	H5CPP_CHECK_NZ( (mem_space = H5Screate_simple(1, &chunk_size, NULL )), h5::error::msg::create_memspace);
	H5CPP_CHECK_NZ( H5Sselect_all(mem_space), h5::error::msg::select_memspace );
}

inline
h5::pt_t::~pt_t(){
	/*default ctor has an invalid state -- skip cleaning up */
	if( !H5Iis_valid( ds ))
		return;
	flush();
	cleanup(__FILE__,__LINE__);
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
		H5CPP_CHECK_NZ(
				H5Sselect_hyperslab(mem_space, H5S_SELECT_SET,offset, NULL, count, NULL), h5::error::msg::select_hyperslab);
		save2file();
	}
}

inline
void h5::pt_t::save2file( ){

	H5CPP_CHECK_NZ( H5Dset_extent(ds, current_dims ), h5::error::msg::set_extent);
	*current_dims -= *count; 				// 
	// select target
	H5CPP_CHECK_NZ( (file_space = H5Dget_space(ds)), h5::error::msg::get_filespace);
		H5CPP_CHECK_NZ( H5Sselect_hyperslab(file_space, H5S_SELECT_SET, current_dims, NULL, count, NULL), h5::error::msg::select_hyperslab );
		H5CPP_CHECK_NZ( H5Dwrite(ds, file_type, mem_space, file_space,  H5P_DEFAULT, ptr), h5::error::msg::write_dataset );
	H5CPP_CHECK_NZ( H5Sclose(file_space), h5::error::msg::close_filespace );
	*current_dims += *count;
}

inline 
h5::error::io::packet_table::misc h5::pt_t::rollback_error(  const std::string& fname, unsigned line, const std::string& msg ){
	std::string prefix = fname + " line#  " +  std::to_string( line ) + " : ";
	cleanup(fname, line);
	return h5::error::io::packet_table::misc( prefix + msg );
}

inline void h5::pt_t::cleanup( const std::string& fname, unsigned line ){
//		hid_t ds, plist, mem_type, file_type, mem_space, file_space;

	free( ptr ); // we can call this on NULL
	std::string prefix = fname + " line#  " +  std::to_string( line ) + " : ";

	if( H5Iis_valid(ds) && H5Idec_ref(ds) < 0 ) 
		throw h5::error::io::packet_table::rollback( prefix +  h5::error::msg::dec_ref );
	if( H5Iis_valid(plist) && H5Pclose(plist) < 0 )
		throw h5::error::io::packet_table::rollback( prefix + h5::error::msg::close_property_list );
	if( H5Iis_valid(mem_type) && H5Tclose( mem_type) < 0 )
		throw h5::error::io::packet_table::rollback( prefix + h5::error::msg::close_memtype );
	if( H5Iis_valid(file_type) && H5Tclose( file_type) < 0 )
		throw h5::error::io::packet_table::rollback( prefix + h5::error::msg::close_filetype );
	if( H5Iis_valid(mem_space) && H5Sclose(mem_space) < 0 )
		throw h5::error::io::packet_table::rollback( prefix + h5::error::msg::close_memspace );
	if( H5Iis_valid(file_space) && H5Sclose(file_space) < 0 )
		throw h5::error::io::packet_table::rollback( prefix + h5::error::msg::close_filespace );
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
#undef H5CPP_CHECK_NZ
#undef H5CPP_CHECK_NULL

#endif
