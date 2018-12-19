/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_PIPELINE_BASIC_HPP
#define  H5CPP_PIPELINE_BASIC_HPP


inline void h5::impl::basic_pipeline_t::write_chunk_impl( const hsize_t* offset, size_t nbytes, const void* data ){

	size_t length = nbytes; // filter may changed this, think of compression
	void *in = chunk0, *out=chunk1, *tmp = chunk0; // invariant: out must point to data block written
	switch( tail ){ // tail = index pointing to queue holding filters
		case 0: // no filters, ( if blocking ) -> data == chunk0 otherwise directly from container 
			H5Dwrite_chunk( ds, dxpl, 0x0, offset, nbytes, data);
			break;
		case 1: // single filter
			length = filter[0](out, data, nbytes, flags[0], cd_size[0], cd_values[0] );
		default: // more than one filter
			for(int j=1; j<tail; j++){ // invariant: out == buffer holding final result
				tmp = in, in = out, out = tmp;
				length = filter[j](out,in,length, flags[j], cd_size[j], cd_values[j]);
			}
			// direct write available from > 1.10.4
			H5Dwrite_chunk(ds, dxpl, 0, offset, length, out);
	}
}


inline void h5::impl::basic_pipeline_t::read_chunk_impl( const hsize_t* offset, size_t nbytes, void* data){
	size_t length = nbytes; // filter may changed this, think of compression
	void *in = chunk0, *out=chunk1, *tmp = chunk1; // invariant: out must point to data block written
	uint32_t filter_mask;
	H5Dread_chunk(ds, dxpl, offset, &filter_mask, in);
//	for(int i=0; i<9; i++)
//		std::cout << ((short*) in)[i] << " ";
//	std::cout<<"\n";
/*
	switch( tail ){ // tail = index pointing to queue holding filters
		case 0: // no filters, ( if blocking ) -> data == chunk0 otherwise directly from container 
			break;
		case 1: // single filter
			length = filter[0](out, in, nbytes, flags[0], cd_size[0], cd_values[0] );
			break;
		default: // more than one filter
			for(int j=tail; j>0; j--){ // invariant: out == buffer holding final result
				tmp = in, in = out, out = tmp;
				length = filter[j](out,in,length, flags[j], cd_size[j], cd_values[j]);
			}
			// direct write available from > 1.10.4
	}*/
}

#endif
