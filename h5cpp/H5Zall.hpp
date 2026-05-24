/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include <hdf5.h>
#include <string>
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <zlib.h>
#include "H5config.hpp"

// libdeflate is opt-in. Build systems that want the fast path define
// H5CPP_HAS_LIBDEFLATE=1 (and link libdeflate). The h5cpp::h5cpp CMake target
// propagates this automatically when h5cpp was configured with libdeflate.
// Drop-in header consumers default to the zlib fallback in zlib_deflate_*,
// which only requires zlib — already a transitive dependency of HDF5 in
// every common distribution path.
#if !defined(H5CPP_DISABLE_LIBDEFLATE) && defined(H5CPP_HAS_LIBDEFLATE)
#include <libdeflate.h>
#endif

#if defined(H5CPP_HAS_LZ4)
#include <lz4.h>
#endif

#if defined(H5CPP_HAS_ZSTD)
#include <zstd.h>
#endif

#if defined(H5CPP_HAS_SZIP)
#include <szlib.h>
#endif

// Community HDF5 filter IDs
#ifndef H5Z_FILTER_LZ4
#define H5Z_FILTER_LZ4  32004
#endif
#ifndef H5Z_FILTER_ZSTD
#define H5Z_FILTER_ZSTD 32015
#endif

namespace h5::impl::filter {
	// TODO: figure something out to map c++ filters to C calls? 
	template<class Derived>
	struct filter_t {
		htri_t can_apply(::hid_t dcpl, ::hid_t type, ::hid_t space){
			return static_cast<Derived*>(this)->can_apply_impl(dcpl,type,space);
		}
			herr_t set_local(::hid_t dcpl, ::hid_t type, ::hid_t space){
				return static_cast<Derived*>(this)->set_local_impl(dcpl,type,space);
			}
			size_t callback(unsigned int flags, size_t cd_nelmts, const unsigned int cd_values[], size_t nbytes, size_t *buf_size, void **buf){
				(void)flags; (void)cd_nelmts; (void)cd_values; (void)nbytes; (void)buf_size; (void)buf;
				return 0;
			}
		size_t apply( void* dst, const void* src, size_t size){
			return static_cast<Derived*>(this)->apply(dst,src,size);
		}
		int version;
		unsigned id;
		unsigned encoder_present;
		unsigned decoder_present;
		std::string name;
	};

		using call_t = size_t (*)(void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] );
		inline size_t mock( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
			(void)flags; (void)n; (void)params;
			memcpy(dst,src,size);
			return size;
		}
	inline unsigned compression_level(size_t n, const unsigned params[]) {
		return n > 0 ? params[0] : H5CPP_DEFAULT_COMPRESSION;
	}
	inline size_t decompressed_size_hint(size_t input_size, size_t n, const unsigned params[]) {
		return n > 1 ? params[1] : input_size;
	}
	inline size_t deflate_bound(size_t size) {
		return static_cast<size_t>(compressBound(static_cast<uLong>(size)));
	}
	inline size_t lz4_bound(size_t size) {
#if defined(H5CPP_HAS_LZ4)
		return static_cast<size_t>(LZ4_compressBound(static_cast<int>(size)));
#else
		return size + (size / 4) + 16;
#endif
	}
	inline size_t zstd_bound(size_t size) {
#if defined(H5CPP_HAS_ZSTD)
		return ZSTD_compressBound(size);
#else
		return size + (size / 4) + 128;
#endif
	}
	// Largest possible output across all supported filters for a given input size.
	// Use this to size the pipeline scratch buffers.
	inline size_t filter_scratch_bound(size_t size) {
		size_t b = deflate_bound(size);
		b = std::max(b, lz4_bound(size));
		b = std::max(b, zstd_bound(size));
		b = std::max(b, size + 4);   // fletcher32 appends 4-byte checksum
		return b;
	}
	inline size_t zlib_deflate_encode(void* dst, const void* src, size_t size, unsigned level) {
#if defined(H5CPP_HAS_LIBDEFLATE)
		libdeflate_compressor* compressor = libdeflate_alloc_compressor(static_cast<int>(level));
		if (!compressor)
			return 0;
		const size_t nbytes = libdeflate_zlib_compress(
			compressor, src, size, dst, deflate_bound(size));
		libdeflate_free_compressor(compressor);
		return nbytes;
#else
		uLongf nbytes = static_cast<uLongf>(deflate_bound(size));
		const int status = compress2(
			static_cast<Bytef*>(dst), &nbytes,
			static_cast<const Bytef*>(src), static_cast<uLong>(size),
			static_cast<int>(level));
		return status == Z_OK ? static_cast<size_t>(nbytes) : 0;
#endif
	}
	inline size_t zlib_deflate_decode(void* dst, const void* src, size_t compressed_size, size_t output_size) {
#if defined(H5CPP_HAS_LIBDEFLATE)
		libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();
		if (!decompressor)
			return 0;
		size_t actual_size = 0;
		const libdeflate_result status = libdeflate_zlib_decompress(
			decompressor, src, compressed_size, dst, output_size, &actual_size);
		libdeflate_free_decompressor(decompressor);
		return status == LIBDEFLATE_SUCCESS ? actual_size : 0;
#else
		uLongf actual_size = static_cast<uLongf>(output_size);
		const int status = uncompress(
			static_cast<Bytef*>(dst), &actual_size,
			static_cast<const Bytef*>(src), static_cast<uLong>(compressed_size));
		return status == Z_OK ? static_cast<size_t>(actual_size) : 0;
#endif
	}
	inline size_t deflate( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[]){
		if (flags & H5Z_FLAG_REVERSE)
			return zlib_deflate_decode(dst, src, size, decompressed_size_hint(size, n, params));
		return zlib_deflate_encode(dst, src, size, compression_level(n, params));
	}
	// H5Z_FILTER_SCALEOFFSET (id=6): quantised float/int pre-processing.
	// Intentional passthrough: the HDF5 C library registers and applies this filter
	// natively during H5Dread/H5Dwrite.  Reimplementing it in H5CPP provides no
		// throughput benefit for the trading use-case and would introduce maintenance
		// cost with no measurable gain.  Delegate to HDF5 C.
		inline size_t scaleoffset( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[]){
			(void)flags; (void)n; (void)params;
			memcpy(dst,src,size);
			return size;
		}

	// Byte-level shuffle / unshuffle.
	// Matching HDF5 H5Z_FILTER_SHUFFLE semantics: for n elements of `type_size` bytes
	// each, reorder bytes so that byte-k of every element is contiguous.
	// params[0] = element size in bytes (set by H5Pset_shuffle / H5Pget_filter2).
#if defined(__SSSE3__)
	#include <tmmintrin.h>
	inline void shuffle_ssse3(void* dst, const void* src, size_t size, size_t type_size, bool reverse) {
		const char* s = static_cast<const char*>(src);
		char* d = static_cast<char*>(dst);
		const size_t count = size / type_size;
		const size_t block = 16 / type_size;
		const size_t nblocks = count / block;

		__m128i mask;
		switch (type_size) {
			case 2: mask = _mm_set_epi8(15,13,11,9,7,5,3,1,14,12,10,8,6,4,2,0); break;
			case 4: mask = _mm_set_epi8(15,11,7,3,14,10,6,2,13,9,5,1,12,8,4,0); break;
			case 8: mask = _mm_set_epi8(15,7,14,6,13,5,12,4,11,3,10,2,9,1,8,0); break;
			default: {
				if (reverse) {
					for (size_t byte = 0; byte < type_size; ++byte)
						for (size_t elem = 0; elem < count; ++elem)
							d[elem * type_size + byte] = s[byte * count + elem];
				} else {
					for (size_t byte = 0; byte < type_size; ++byte)
						for (size_t elem = 0; elem < count; ++elem)
							d[byte * count + elem] = s[elem * type_size + byte];
				}
				return;
			}
		}

		for (size_t b = 0; b < nblocks; ++b) {
			__m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s + b * 16));
			__m128i shuf = _mm_shuffle_epi8(v, mask);
			_mm_storeu_si128(reinterpret_cast<__m128i*>(d + b * 16), shuf);
		}

		if (reverse) {
			for (size_t byte = 0; byte < type_size; ++byte)
				for (size_t elem = nblocks * block; elem < count; ++elem)
					d[elem * type_size + byte] = s[byte * count + elem];
		} else {
			for (size_t byte = 0; byte < type_size; ++byte)
				for (size_t elem = nblocks * block; elem < count; ++elem)
					d[byte * count + elem] = s[elem * type_size + byte];
		}
	}
#endif

	inline size_t shuffle( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
		const size_t type_size = (n > 0 && params[0] > 1) ? static_cast<size_t>(params[0]) : 1;
		if (type_size == 1 || size == 0) {
			memcpy(dst, src, size);
			return size;
		}
#if defined(__SSSE3__)
		if (type_size == 2 || type_size == 4 || type_size == 8) {
			shuffle_ssse3(dst, src, size, type_size, flags & H5Z_FLAG_REVERSE);
			return size;
		}
#endif
		const size_t count = size / type_size;
		const char* s = static_cast<const char*>(src);
		char*       d = static_cast<char*>(dst);
		if (flags & H5Z_FLAG_REVERSE) {
			// Unshuffle: interleaved → original AoS layout
			for (size_t byte = 0; byte < type_size; ++byte)
				for (size_t elem = 0; elem < count; ++elem)
					d[elem * type_size + byte] = s[byte * count + elem];
		} else {
			// Shuffle: AoS → byte-plane layout
			for (size_t byte = 0; byte < type_size; ++byte)
				for (size_t elem = 0; elem < count; ++elem)
					d[byte * count + elem] = s[elem * type_size + byte];
		}
		return size;
	}

	namespace {
	inline uint32_t fletcher32_checksum(const void* data, size_t nbytes) {
		const uint8_t* p = static_cast<const uint8_t*>(data);
		uint32_t sum1 = 0, sum2 = 0;
		const size_t words = nbytes / 2;
		size_t i = 0;

		// Process in batches of 360 words to reduce modulo overhead.
		// Fletcher32 can accumulate up to 360 words in 32-bit registers
		// before modulo is required to avoid overflow.
		for (; i + 360 <= words; i += 360) {
			uint32_t batch_sum1 = 0, batch_sum2 = 0;
			for (size_t j = 0; j < 360; ++j) {
				const uint16_t w = (static_cast<uint16_t>(p[2*(i+j)]) << 8) | p[2*(i+j)+1];
				batch_sum1 += w;
				batch_sum2 += batch_sum1;
			}
			sum2 += 360 * sum1 + batch_sum2;
			sum1 += batch_sum1;
			sum1 %= 65535u;
			sum2 %= 65535u;
		}

		for (; i < words; ++i) {
			const uint16_t w = (static_cast<uint16_t>(p[2*i]) << 8) | p[2*i+1];
			sum1 = (sum1 + w) % 65535u;
			sum2 = (sum2 + sum1) % 65535u;
		}
		if (nbytes & 1u) {
			const uint16_t w = static_cast<uint16_t>(p[nbytes - 1]) << 8;
			sum1 = (sum1 + w) % 65535u;
			sum2 = (sum2 + sum1) % 65535u;
		}
		return (sum2 << 16) | sum1;
	}
	} // anonymous namespace

	// Fletcher32 checksum filter.
	// Encode: appends a 4-byte big-endian checksum → returns size + 4.
		// Decode: verifies, strips 4-byte checksum → returns size - 4, or 0 on mismatch.
		inline size_t fletcher32( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[]){
			(void)n; (void)params;
			if (flags & H5Z_FLAG_REVERSE) {
			if (size < 4) return 0;
			const size_t data_size = size - 4;
			const uint8_t* stored_bytes = static_cast<const uint8_t*>(src) + data_size;
			const uint32_t stored =
				(static_cast<uint32_t>(stored_bytes[0]) << 24) |
				(static_cast<uint32_t>(stored_bytes[1]) << 16) |
				(static_cast<uint32_t>(stored_bytes[2]) <<  8) |
				 static_cast<uint32_t>(stored_bytes[3]);
			const uint32_t computed = fletcher32_checksum(src, data_size);
			if (stored != computed) return 0;
			memcpy(dst, src, data_size);
			return data_size;
		} else {
			const uint32_t checksum = fletcher32_checksum(src, size);
			memcpy(dst, src, size);
			uint8_t* out = static_cast<uint8_t*>(dst) + size;
			out[0] = static_cast<uint8_t>(checksum >> 24);
			out[1] = static_cast<uint8_t>(checksum >> 16);
			out[2] = static_cast<uint8_t>(checksum >>  8);
			out[3] = static_cast<uint8_t>(checksum);
			return size + 4;
		}
	}
	inline size_t gzip( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[]){
		return deflate(dst, src, size, flags, n, params);
	}
	// H5Z_FILTER_SZIP (id=4): Rice/Golomb lossless compression via vendored libaec/szip.
	// HDF5 cd_values layout (from H5Zszip.c):
	//   params[0] = options_mask   (SZ_EC_OPTION_MASK | SZ_NN_OPTION_MASK | SZ_MSB/LSB | ...)
	//   params[1] = bits_per_pixel (element size in bits, 1..24 or 32/64)
		//   params[2] = pixels_per_block (2..32, must be even)
		//   params[3] = pixels_per_scanline (= total elements in chunk)
		inline size_t szip( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[]){
			(void)flags; (void)n; (void)params;
#if defined(H5CPP_HAS_SZIP)
			if (n < 4) return 0;

		SZ_com_t param;
		param.options_mask       = static_cast<int>(params[0]);
		param.bits_per_pixel     = static_cast<int>(params[1]);
		param.pixels_per_block   = static_cast<int>(params[2]);
		param.pixels_per_scanline = static_cast<int>(params[3]);

		if (flags & H5Z_FLAG_REVERSE) {
			// Decompress: output is the original uncompressed chunk.
			// SZ_BufftoBuffDecompress needs to know the number of output pixels:
			// pixels = params[3] (pixels_per_scanline stores total chunk elements for HDF5).
			size_t out_size = static_cast<size_t>(param.pixels_per_scanline)
			                * ((static_cast<size_t>(param.bits_per_pixel) + 7) / 8);
			int ret = SZ_BufftoBuffDecompress(dst, &out_size, src, size, &param);
			return (ret == SZ_OK) ? out_size : 0;
		} else {
			// Compress.
			size_t out_size = size + size / 2 + 128; // szip worst-case is ~1.5x
			if (out_size < size + 128) out_size = size + 128;
			int ret = SZ_BufftoBuffCompress(dst, &out_size, src, size, &param);
			return (ret == SZ_OK) ? out_size : 0;
		}
#else
		// Passthrough when szip not compiled in.
		memcpy(dst, src, size);
		return size;
#endif
	}
	// H5Z_FILTER_NBIT (id=5): compact integer storage via bit-precision packing.
	// Intentional passthrough: the HDF5 C library registers and applies this filter
	// natively during H5Dread/H5Dwrite.  Reimplementing it in H5CPP provides no
	// throughput benefit for the trading use-case (integer sensor data compression
		// is not on the critical path) and would require reverse-engineering HDF5's
		// internal cd_values descriptor format.  Delegate to HDF5 C.
		inline size_t nbit( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[]){
			(void)flags; (void)n; (void)params;
			memcpy(dst,src,size);
			return size;
		}
		inline size_t add( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
			(void)flags; (void)n; (void)params;
			memcpy(dst,src,size);
			return size;
		}
		inline size_t jpeg( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
			(void)flags; (void)n; (void)params;
			memcpy(dst,src,size);
			return size;
		}
		inline size_t disperse( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
			(void)flags; (void)n; (void)params;
			memcpy(dst,src,size);
			return size;
		}

		// LZ4 extreme-throughput compression (community filter ID 32004).
		// Enabled when compiled with H5CPP_HAS_LZ4; falls back to passthrough otherwise.
		inline size_t lz4( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
			(void)flags; (void)n; (void)params;
#if defined(H5CPP_HAS_LZ4)
			if (flags & H5Z_FLAG_REVERSE) {
			const int out = LZ4_decompress_safe(
				static_cast<const char*>(src), static_cast<char*>(dst),
				static_cast<int>(size),
				static_cast<int>(decompressed_size_hint(size, n, params)));
			return out > 0 ? static_cast<size_t>(out) : 0;
		} else {
			const int out = LZ4_compress_default(
				static_cast<const char*>(src), static_cast<char*>(dst),
				static_cast<int>(size), static_cast<int>(lz4_bound(size)));
			return out > 0 ? static_cast<size_t>(out) : 0;
		}
#else
		memcpy(dst, src, size);
		return size;
#endif
	}

		// Zstd compression (community filter ID 32015).
		// Enabled when compiled with H5CPP_HAS_ZSTD; falls back to passthrough otherwise.
		inline size_t zstd( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
			(void)flags; (void)n; (void)params;
#if defined(H5CPP_HAS_ZSTD)
			if (flags & H5Z_FLAG_REVERSE) {
			const size_t out = ZSTD_decompress(
				dst, decompressed_size_hint(size, n, params), src, size);
			return ZSTD_isError(out) ? 0 : out;
		} else {
			const size_t out = ZSTD_compress(
				dst, zstd_bound(size), src, size,
				static_cast<int>(compression_level(n, params)));
			return ZSTD_isError(out) ? 0 : out;
		}
#else
		memcpy(dst, src, size);
		return size;
#endif
	}

		inline size_t error( void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[] ){
			(void)dst; (void)src; (void)flags; (void)n; (void)params;
			throw std::runtime_error("invalid filter");
			return size;
	}
	inline call_t get_callback( H5Z_filter_t filter_id ){

		switch( filter_id ){
			case H5Z_FILTER_DEFLATE:    return filter::gzip;
			case H5Z_FILTER_SHUFFLE:    return filter::shuffle;
			case H5Z_FILTER_FLETCHER32: return filter::fletcher32;
			case H5Z_FILTER_SZIP:       return filter::szip;
			case H5Z_FILTER_NBIT:       return filter::nbit;
			case H5Z_FILTER_SCALEOFFSET:return filter::scaleoffset;
			case H5Z_FILTER_LZ4:        return filter::lz4;
			case H5Z_FILTER_ZSTD:       return filter::zstd;
			default:
					return filter::error;
		}
	}
}
