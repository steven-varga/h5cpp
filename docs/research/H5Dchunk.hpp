/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_DCHUNK_HPP
#define  H5CPP_DCHUNK_HPP

namespace h5 {
	using owrite_chunk_t = herr_t(*)( hid_t dset_id, hid_t dxpl_id, uint32_t filter_mask, hsize_t *offset, size_t data_size, const void *buf );
	using oread_chunk_t = herr_t (*)( hid_t dset_id, hid_t dxpl_id, const hsize_t *offset, uint32_t *filter_mask, void *buf );
}

namespace h5{ namespace impl {
	struct pipeline {
		pipeline( const h5::ds_t& ds );
		pipeline(){};
		~pipeline();
		void apply( const hsize_t* offset, const hsize_t* dims, const void* ptr );
		void push( filter::call_t filter );
		void pop();
		void forward( const hsize_t* offset, size_t nbytes, const void* ptr );
		void reverse();

		char* chunk0, *chunk1;

	private:
		impl::unique_ptr<char> ptr0, ptr1; // will call std::free on dtor
		filter::call_t filter[H5CPP_MAX_FILTER];
		hsize_t C[H5CPP_MAX_RANK],D[H5CPP_MAX_RANK],
				n,tail,rank,
				N[H5CPP_MAX_RANK],B[H5CPP_MAX_RANK],R[H5CPP_MAX_RANK];
		unsigned cd_values[H5CPP_MAX_FILTER][H5CPP_MAX_FILTER_PARAM],
				flags[H5CPP_MAX_FILTER];
		size_t 	block_size, element_size, cd_size[H5CPP_MAX_FILTER];
		h5::dcpl_t dcpl;
		h5::ds_t ds;
	};
}}

#define h5cpp_outer( idx ) for( j##idx =0; j##idx < n##idx; j##idx += b##idx)
#define h5cpp_inner( idx ) for( i##idx = j##idx; i##idx < std::min(j##idx+b##idx,n##idx); i##idx++)
#define h5cpp_def( idx ) hsize_t i##idx=0, j##idx = 0, s##idx=0, n##idx=N[idx], b##idx=B[idx], r##idx=R[idx];
inline void h5::impl::pipeline::apply( const hsize_t* chunk_offset, const hsize_t* dims, const void* ptr_ ){

	const char* ptr = static_cast<const char*>( ptr_ );
	// compute edges if any - for the data size
	// computes R residuals, and sets N dimension in reverse order
	// when actual data dimensions - current_dims are greater then a chunk, it must be broken into chunk size
	// if there are residuals on the edges, then padding is needed.
	unsigned i=0;
	for(; i<rank; i++ )
		N[i] = dims[rank-i-1], R[i] = N[i] % B[i];
	for(; i<H5CPP_MAX_RANK; i++) N[i] = B[i] = 1, R[i] = 0;

	// b - block size, j - block index pos, r - remainder at the edges, n - actual dimension of data
	//
	h5cpp_def(0) h5cpp_def(1) h5cpp_def(2) h5cpp_def(3) h5cpp_def(4) h5cpp_def(5) h5cpp_def(6)

	h5cpp_outer( 6 ){ h5cpp_outer( 5 ){ h5cpp_outer( 4 ){ h5cpp_outer( 3 ){
	h5cpp_outer( 2 ){ h5cpp_outer( 1 ){ h5cpp_outer( 0 ){
		char* p = chunk0;
		// reset memory page only on the edges
		if( j0 + b0 > n0 || j1 + b1 > n1 || j2 + b2 > n2 ||
			j3 + b3 > n3 || j4 + b4 > n4 || j5 + b5 > n5 || j6 + b6 > n6 ) memset(p,0, block_size);

		h5cpp_inner(6){	h5cpp_inner(5){ h5cpp_inner(4){
		h5cpp_inner(3){ h5cpp_inner(2){ h5cpp_inner(1){
			hsize_t offset = i6*n5*n4*n3*n2*n1*n0 + i5*n4*n3*n2*n1*n0 +
					i4*n3*n2*n1*n0 + i3*n2*n1*n0 + i2*n1*n0 + i1*n0 + j0;
			if( j0 != n0 - r0 ) // block copy
				memcpy(p, ptr + offset * element_size, b0 * element_size );
			else // edge handling
				memcpy(p, ptr + offset * element_size, r0 * element_size );
			p += b0 * element_size;
		}}}}}}
		// at this point we have a single 'chunk' in this->chunk0 buffer ready to go
		// the coordinates are in j indices
		D[0] = j0, D[1]=j1, D[2]=j2, D[3]=j3, D[4]=j4, D[5]=j5, D[6]=j6;
		//coordinates are reversed in D, invert them:
		for(int k=0;k<rank; k++ ) C[k] = D[rank-k-1] + chunk_offset[k];
		// execute filters in forward direction
		forward( this->C, this->block_size, this->chunk0 );
	}}}}}}}
}
#undef h5cpp_def
#undef h5cpp_inner
#undef h5cpp_outer
h5::impl::pipeline::pipeline( const h5::ds_t& ds )
	: n(1), tail(0), ds(ds) {
	// grab chunk dimensions, which is a block we're breaking data into
/*	h5::chunk_t block;
	h5::dcpl_t dcpl = h5::create_dcpl( ds );
	if( (rank = h5::get_chunk_dims( dcpl, block )) == 0  )
			throw std::runtime_error("data-space is rank 0, is data space a scalar? ");

	// data element type information is available from the dataset
	::hid_t type_id = H5Dget_type( static_cast<::hid_t>(ds) );
	element_size = H5Tget_size( type_id );

	//fix B block/chunk size for the lifespan of pipeline
	for(int i=0; i<rank; i++ )
	   	n *= block[i], B[i] = block[rank-i-1];

	block_size = n*element_size;

	unsigned filter_config;
	unsigned N = H5Pget_nfilters( dcpl );
	H5Z_filter_t filter_id;
	for(unsigned i; i<N; i++){
		cd_size[i] = H5CPP_MAX_FILTER_PARAM;
		push(
			filter::get_callback( H5Pget_filter2( dcpl, i, &flags[i], &cd_size[i], cd_values[i], 0, nullptr, &filter_config )));
	}
	ptr0 = std::move( impl::unique_ptr<char>{ (char*)aligned_alloc( H5CPP_MEM_ALIGNMENT, block_size )} );
	ptr1 = std::move( impl::unique_ptr<char>{ (char*)aligned_alloc( H5CPP_MEM_ALIGNMENT, block_size )} );
	// get an alias to smart ptr
	if( (chunk0 = ptr0.get()) == NULL || (chunk1 = ptr1.get()) == NULL )
	   	throw h5::error::io::dataset::open( H5CPP_ERROR_MSG("CTOR: couldn't allocate memory for caching chunks, invalid/check size?"));
		*/
}

h5::impl::pipeline::~pipeline(){
}

inline void h5::impl::pipeline::push(filter::call_t filter_){
	filter[tail++] = filter_;
}
inline void h5::impl::pipeline::pop(){
	tail--;
}

inline void h5::impl::pipeline::forward( const hsize_t* offset, size_t nbytes,  const void* data){

	size_t length = nbytes; // filter may changed this, think of compression
	void *in = chunk0, *out=chunk1, *tmp = chunk0; // invariant: out must point to data block written
	switch( tail ){ // tail = index pointing to queue holding filters
		case 0: // no filters, ( if blocking ) -> data == chunk0 otherwise directly from container 
			H5Dwrite_chunk(ds, 0, 0, offset, nbytes, data);
			break;
		case 1: // single filter
			length = filter[0](out, data, nbytes, flags[0], cd_size[0], cd_values[0] );
		default: // more than one filter
			for(int j=1; j<tail; j++){ // invariant: out == buffer holding final result
				tmp = in, in = out, out = tmp;
				length = filter[j](out,in,length, flags[j], cd_size[j], cd_values[j]);
			}
			// direct write available from > 1.10.4
			H5Dwrite_chunk(ds, 0, 0, offset, length, out);
	}
}

inline void h5::impl::pipeline::reverse(){
	throw std::runtime_error("not implemented yet...");
}

namespace h5 { namespace impl {
	void write_chunk( const h5::ds_t& ds, const h5::count_t& count,  const h5::offset_t& offset, const void* ptr ){

	//	h5::impl::pipeline pipeline( ds );
	//	pipeline.apply( offset.begin(), count.begin(), ptr );
	}
}}
#endif


