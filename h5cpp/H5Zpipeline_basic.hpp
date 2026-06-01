/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include "H5Zpipeline.hpp"

inline void h5::impl::basic_pipeline_t::write_chunk_impl( const hsize_t* offset, size_t nbytes, const void* data ){
	size_t length = nbytes;                        // filter may change this, think of compression
	uint32_t mask = 0x0;                           // filter mask = 0x0 all filters applied

	// Determine if all filters in the chain are in-place safe.
	// Shuffle, fletcher32, scaleoffset, nbit, and add are safe.
	// Compression filters (gzip, szip, lz4, zstd) require separate buffers.
	bool all_inplace = true;
	for (hsize_t j = 0; j < tail; ++j) {
		if (filter_id[j] != H5Z_FILTER_SHUFFLE &&
		    filter_id[j] != H5Z_FILTER_FLETCHER32 &&
		    filter_id[j] != H5Z_FILTER_SCALEOFFSET &&
		    filter_id[j] != H5Z_FILTER_NBIT) {
			all_inplace = false;
			break;
		}
	}

	if (tail == 0) {
		// no filters, ( if blocking ) -> data == chunk0 otherwise directly from container
		H5Dwrite_chunk(static_cast<::hid_t>(ds), dxpl, 0x0, offset, nbytes, data);
		return;
	}

	if (all_inplace) {
		// In-place path: copy data once, then apply all filters on chunk0.
		std::memcpy(chunk0, data, nbytes);
		void* buf = chunk0;
		for (hsize_t j = 0; j < tail; ++j) {
			length = filter[j](buf, buf, length, flags[j], cd_size[j], cd_values[j]);
			if (!length)
				mask |= 1u << j;
		}
		H5Dwrite_chunk(static_cast<::hid_t>(ds), dxpl, mask, offset, length, buf);
		return;
	}

	// Ping-pong path for filters that require separate buffers.
	void *in = chunk0, *out = chunk1, *tmp = chunk0;
	length = filter[0](out, data, nbytes, flags[0], cd_size[0], cd_values[0]);
	if( !length )
		mask = 1 << 0;
	for(hsize_t j=1; j<tail; j++){ // invariant: out == buffer holding final result
		tmp = in, in = out, out = tmp;
		length = filter[j](out, in, length, flags[j], cd_size[j], cd_values[j]);
		if( !length )
			mask |= 1 << j;
	}
	// direct write available from > 1.10.4
	H5Dwrite_chunk(static_cast<::hid_t>(ds), dxpl, mask, offset, length, out);
}


inline void h5::impl::basic_pipeline_t::read_chunk_impl( const hsize_t* offset, size_t nbytes, void* data){
	(void)data;
	size_t length = nbytes;
	uint32_t filter_mask;

	if (tail == 0) {
		// No filters: read decompressed chunk directly into chunk0
#if H5_VERSION_GE(2,0,0)
		size_t buf_size = nbytes;
		H5Dread_chunk2(static_cast<::hid_t>(ds), dxpl, offset, &filter_mask, chunk0, &buf_size);
#else
		H5Dread_chunk(static_cast<::hid_t>(ds), dxpl, offset, &filter_mask, chunk0);
#endif
		return;
	}

	// Parity-based buffer selection: N ping-pong swaps must leave the result in chunk0.
	// If tail is odd we start reading into chunk1; if even, into chunk0.
	// After the loop 'src' always points to chunk0 (the decompressed result).
	void* read_target = (tail % 2 == 1) ? chunk1 : chunk0;
#if H5_VERSION_GE(2,0,0)
	size_t buf_size = nbytes;
	H5Dread_chunk2(static_cast<::hid_t>(ds), dxpl, offset, &filter_mask, read_target, &buf_size);
#else
	H5Dread_chunk(static_cast<::hid_t>(ds), dxpl, offset, &filter_mask, read_target);
#endif

	void* src = read_target;
	void* dst = (read_target == chunk0) ? static_cast<void*>(chunk1) : static_cast<void*>(chunk0);

	// Apply filters in reverse order (highest index first) with H5Z_FLAG_REVERSE
	for (hsize_t j = tail; j > 0; --j) {
		const hsize_t fi = j - 1;
		length = filter[fi](dst, src, length,
			flags[fi] | H5Z_FLAG_REVERSE, cd_size[fi], cd_values[fi]);
		void* tmp = src; src = dst; dst = tmp;
	}
	// src now points to chunk0, which holds the decompressed chunk data
}
