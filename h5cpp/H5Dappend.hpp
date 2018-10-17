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
		pt_t( const h5::ds_t& handle ); // conversion ctor
		// deep copy with own cache memory
		pt_t( const h5::pt_t& pt ) : h5::pt_t(pt.ds) {
		};
		~pt_t();
		pt_t& operator=( h5::pt_t& pt ) = default;
		constexpr pt_t& operator=( h5::pt_t&& pt ) = delete;

		template<typename T> friend void append( h5::pt_t& ds, const T& ref);

		void flush();
		private:
		template <class T> void append( const T& ref );
		void save2file();

		impl::unique_ptr<void> ptr; // will call std::free on dtor
		h5::sp_t mem_space,file_space;
		h5::dt_t mem_type, file_type;
		h5::ds_t ds;
		h5::dcpl_t dcpl;
		h5::current_dims_t current_dims;
		h5::max_dims_t max_dims;
		h5::chunk_t chunk_dims;
		h5::offset_t offset;
		h5::count_t count;
		hsize_t chunk_size;
		size_t rank, type_size;
	};
}

/* initialized to invalid state
 * */
inline h5::pt_t::pt_t() :
	rank(0),type_size{0},
	dcpl{H5I_UNINIT},ds{H5I_UNINIT},
	mem_space{H5I_UNINIT}, file_space{H5I_UNINIT},
	mem_type{H5I_UNINIT}, file_type{H5I_UNINIT} {
		memset(*count, 1, H5CPP_MAX_RANK);
		memset(*offset, 0, H5CPP_MAX_RANK);
	}

// conversion ctor
inline
h5::pt_t::pt_t( const h5::ds_t& handle ) : pt_t() {
	hid_t ds_ = static_cast<::hid_t>( handle );
	/*default ctor has an invalid state -- skip initialization */
	if( !is_valid(handle) ) return;
	try {
		ds = handle; // copy handle inc ref, behaves as unique_ptr

		file_space = h5::get_space( handle );
		rank = h5::get_simple_extent_dims( file_space, current_dims, max_dims );
		dcpl = h5::create_dcpl( ds );
		chunk_dims = h5::get_chunk_dims( dcpl );

		// chunk_dims and rank are initialized 
		chunk_size=1; for(int i=0;i<rank;i++) chunk_size*=chunk_dims[i];
		file_type = h5::get_type( ds );
		type_size = h5::get_size( file_type );
		mem_space = h5::create_simple( chunk_size );
		h5::select_all( mem_space );
	} catch ( ... ){
		throw h5::error::io::packet_table::misc( H5CPP_ERROR_MSG("CTOR: unable to create handle from dataset..."));
	}

	ptr = std::move( impl::unique_ptr<void>{calloc( chunk_size, type_size )} );
	if( ptr.get() == NULL)
	   	throw h5::error::io::dataset::open( H5CPP_ERROR_MSG("CTOR: couldn't allocate memory for caching chunks, invalid/check size?"));
}

inline
h5::pt_t::~pt_t(){
	/*default ctor has an invalid state -- skip flushing cache */
	if( !h5::is_valid( ds ) )
		return;
	flush();
}

template <class T>
void h5::pt_t::append( const T& ref ){
	try {
		size_t k = (++ current_dims[0] -1) % chunk_size;
		static_cast<T*>(ptr.get())[k] = ref;
		if( k == chunk_size - 1  )
			count[0] = chunk_size, save2file();
	} catch( const std::runtime_error& err ){
		throw h5::error::io::dataset::append( err.what() );
	}
}
inline
void h5::pt_t::flush(){
	count[0] = current_dims[0] % chunk_size;
	if( count[0] ){ // there is left over then flush it
		// select the remainder of the memory
		h5::select_hyperslab( mem_space, offset, count );
		save2file();
	}
}

inline
void h5::pt_t::save2file( ){

	h5::set_extent(ds, current_dims);
	current_dims[0] -= count[0];
	h5::sp_t file_space = h5::get_space( ds );
	h5::select_hyperslab(file_space, current_dims, count);
	h5::writeds(ds, file_type, mem_space, file_space, ptr.get() );
	current_dims[0] += count[0];
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
	inline void flush( h5::pt_t& pt){
		try {
			pt.flush( );
		} catch ( const std::runtime_error& e){
			throw h5::error::io::dataset::close( e.what() );
		}
	}

}
#endif
