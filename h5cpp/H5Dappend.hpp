/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 
 */

#ifndef  H5CPP_DAPPEND_HPP
#define H5CPP_DAPPEND_HPP
#include <zlib.h>

namespace h5 {
	struct pt_t;
}
std::ostream& operator<<(std::ostream& os, const h5::pt_t& pt);

// packet table template specialization with inheritance
namespace h5 {
	struct pt_t {
		pt_t();
		pt_t( const h5::ds_t& handle ); // conversion ctor
		// deep copy with own cache memory
		pt_t( const h5::pt_t& pt ) : h5::pt_t(pt.ds) {
		};
		~pt_t();

		pt_t& operator=( h5::pt_t&& pt ){
			init(pt.ds);
			return pt;
		}
		friend std::ostream& ::operator<<(std::ostream &os, const h5::pt_t& pt);
		template<class T>
		friend void append( h5::pt_t& ds, const T& ref);
		void flush();

		private:
		void init(const h5::ds_t& ds_);

		template<class T> inline typename std::enable_if<h5::impl::is_scalar<T>::value,
		void>::type append( const T* ptr );
		template<class T> inline typename std::enable_if< h5::impl::is_scalar<T>::value && !std::is_pointer<T>::value,
		void>::type append( const T& ref );
		template<class T> inline typename std::enable_if< !h5::impl::is_scalar<T>::value,
		void>::type append( const T& ref );

		impl::pipeline_t<impl::basic_pipeline_t> pipeline;
		h5::ds_t ds;
		h5::dxpl_t dxpl;
		hsize_t offset[H5CPP_MAX_RANK],
			current_dims[H5CPP_MAX_RANK],
			chunk_dims[H5CPP_MAX_RANK],
			count[H5CPP_MAX_RANK];
		size_t block_size,element_size,N,n,rank;
		void *ptr, *fill_value;
	};
}


/* initialized to invalid state
 * */
inline h5::pt_t::pt_t() :
	dxpl{H5Pcreate(H5P_DATASET_XFER)},ds{H5I_UNINIT},n{0},fill_value{NULL}{
		for( int i=0; i<H5CPP_MAX_RANK; i++ )
			count[i] = 1, offset[i] = 0;
	}

// conversion ctor
inline
h5::pt_t::pt_t( const h5::ds_t& handle ) : pt_t() {
	/*default ctor has an invalid state -- skip initialization */
	if( !is_valid(handle) ) return;
	init(handle);
}

inline
h5::pt_t::~pt_t(){
	/*default ctor has an invalid state -- skip flushing cache */
	if( !h5::is_valid( ds ) )
		return;
	flush();
	free(this->fill_value);
}

inline
void h5::pt_t::init( const h5::ds_t& handle ){
	try {
		ds = handle; // copy handle inc ref, behaves as unique_ptr

		h5::sp_t file_space = h5::get_space( handle );
		rank = h5::get_simple_extent_dims( file_space, current_dims, nullptr );

		h5::dcpl_t dcpl = h5::get_dcpl( ds );
		h5::dt_t<void*> type = h5::get_type<void*>( ds );
		hsize_t size = h5::get_size( type );
		this->fill_value = h5::get_fill_value(dcpl, type, size);
		pipeline.set_cache(dcpl, size);
		this->ptr = pipeline.chunk0;
		this->block_size = pipeline.block_size;
		this->element_size = pipeline.element_size;
		this->N = pipeline.n;
		pipeline.ds = ds; pipeline.dxpl = dxpl;
		h5::get_chunk_dims( dcpl, chunk_dims );
		for(int i=1; i<rank; i++)
			current_dims[i] = chunk_dims[i];
	} catch ( ... ){
		throw h5::error::io::packet_table::misc( H5CPP_ERROR_MSG("CTOR: unable to create handle from dataset..."));
	}
}
template<class T> inline typename std::enable_if< h5::impl::is_scalar<T>::value,
void>::type h5::pt_t::append( const T* ptr ) try {
	//PTR: write directly chunk size from provided buffer/ptr
	*offset = *current_dims;
	*current_dims += *chunk_dims;
	h5::set_extent(ds, current_dims);
	pipeline.write_chunk(offset,block_size,ptr);
} catch( const std::runtime_error& err ){
	throw h5::error::io::dataset::append( err.what() );
}

template<class T> inline typename std::enable_if< h5::impl::is_scalar<T>::value && !std::is_pointer<T>::value,
void>::type h5::pt_t::append( const T& ref ) try {
//SCALAR: store inbound data directly in pipeline cache
	static_cast<T*>( ptr )[n++] = ref;

	if( n != N ) return;

	n = 0;
	*offset = *current_dims;
	*current_dims += *chunk_dims;
	h5::set_extent(ds, current_dims);
	pipeline.write_chunk(offset,block_size,ptr);
} catch( const std::runtime_error& err ){
	throw h5::error::io::dataset::append( err.what() );
}

template<class T> inline typename std::enable_if< !h5::impl::is_scalar<T>::value,
void>::type h5::pt_t::append( const T& ref ) try {
	auto dims = impl::size( ref );

	*offset = *current_dims;
	*current_dims += 1;
	h5::set_extent(ds, current_dims);
	auto ptr_ = impl::data( ref );
	auto dims_ = impl::size( ref );

	switch( dims_.size() ){
		case 1: // vector
			if( dims[0] * element_size == block_size )
				pipeline.write_chunk(offset, block_size, (void*) ptr_ );
			else throw h5::error::io::packet_table::write(
					H5CPP_ERROR_MSG("dimension mismatch: "
						+ std::to_string( dims[0] * element_size) + " != " + std::to_string(block_size) ));
			break;
		case 2: //matrix
			if( dims[0] * dims[1] * element_size == block_size )
				pipeline.write_chunk(offset, block_size, (void*) ptr_ );
			else throw h5::error::io::packet_table::write(
					H5CPP_ERROR_MSG("dimension mismatch: "
						+ std::to_string( dims[0] * dims[1] * element_size) + " != " + std::to_string(block_size) ));
			break;
		case 3: // cube
			if( dims[0] * dims[1] * dims[2] * element_size == block_size )
				pipeline.write_chunk(offset, block_size, (void*) ptr_ );
			else throw h5::error::io::packet_table::write(
					H5CPP_ERROR_MSG("dimension mismatch: "
						+ std::to_string( dims[0] * dims[1] * dims[2] * element_size) + " != " + std::to_string(block_size) ));
			;break;
		default:
			throw h5::error::io::packet_table::misc( H5CPP_ERROR_MSG("objects with rank > 2 are not supported... "));
	}
	//pipeline.write_chunk(offset,block_size, (void*) ptr_ );

} catch( const std::runtime_error& err ){
	throw h5::error::io::dataset::append( err.what() );
}

inline
void h5::pt_t::flush(){
	if( n == 0 ) return;
	// the remainder of last chunk must be set to fill_value; arbitrary type size supported
	for(hsize_t i=0; i<(N-n); i++)
		for(size_t j=0; j < element_size; j++)
			static_cast<char*>( ptr )[(n + i) * element_size + j] = static_cast<char*>( fill_value )[ j ];
	*offset = *current_dims;
	*current_dims += *current_dims % *chunk_dims;
	size_t r=1; for(int i=1; i<rank; i++) r*=chunk_dims[i];
	*current_dims += (n % r) ? n / r + 1 : n / r;
	h5::set_extent(ds, current_dims);
	pipeline.write_chunk( offset, block_size, ptr );
}

namespace h5 {
	/** @ingroup io-append
	 * @brief extends HDF5 dataset along the first/slowest growing dimension, then writes passed object to the newly created space
	 * @param pt packet_table descriptor
	 * @param ref T type const reference to object appended
	 * @tparam T dimensions must match the dimension of HDF5 space upto rank-1 
	 */

	template<class T> inline
	void append( h5::pt_t& pt, const T& ref){
		pt.append( ref );
	}

	inline void flush( h5::pt_t& pt) try {
		pt.flush( );
	} catch ( const std::runtime_error& e){
		throw h5::error::io::dataset::close( e.what() );
	}
}

inline std::ostream& operator<<(std::ostream &os, const h5::pt_t& pt) {

	std::vector<size_t> current_dims(pt.current_dims, pt.current_dims + pt.rank);
	std::vector<size_t> chunk_dims(pt.chunk_dims, pt.chunk_dims + pt.rank);
	std::vector<size_t> count(pt.count, pt.count + pt.rank);
	std::vector<size_t> offset(pt.offset, pt.offset + pt.rank);

	os <<"packet table:\n"
		 "------------------------------------------\n";
	os << "rank: " << pt.rank << " N:" << pt.N <<" n:" << pt.n << "\n";
	os << "element size: " << pt.element_size << " block size: " << pt.block_size << "\n";
	os << "current dims: " << current_dims << std::endl;
	os << "chunk dims: " << current_dims << std::endl;
	os << "offset : " <<  offset <<  " count : " << count << std::endl;
	os <<"ds: "<< static_cast<hid_t>( pt.ds ) <<" dxpl: "<< static_cast<hid_t>( pt.dxpl ) << std::endl;
	os <<"fill value: " << std::hex << pt.fill_value << " buffer: " << pt.ptr;
	os << "\n\n";

	return os;
}
#endif
