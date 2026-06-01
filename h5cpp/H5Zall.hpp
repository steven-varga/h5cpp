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
#include <mutex>
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

#if defined(_MSC_VER)
#include <intrin.h>
#endif

// Community HDF5 filter IDs
#ifndef H5Z_FILTER_LZ4
#define H5Z_FILTER_LZ4  32004
#endif
#ifndef H5Z_FILTER_ZSTD
#define H5Z_FILTER_ZSTD 32015
#endif
#ifndef H5Z_FILTER_GORILLA
#define H5Z_FILTER_GORILLA 32016
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

	// Resolve the vendored compressors' lazy CPU-feature dispatch on the CALLING
	// thread, exactly once.  libdeflate (and zstd) pick their SIMD implementation
	// on first use by writing a resolved function pointer into a process-global
	// (e.g. libdeflate's `adler32_impl`).  When the worker pool first runs several
	// compress/decompress jobs at once, those first-call writes race — TSan reports
	// a data race on global 'adler32_impl' in libdeflate_adler32.  Value-benign
	// (every thread resolves the same pointer) but a data race / UB nonetheless.
	// Warming the dispatch single-threaded here makes the globals read-only for the
	// workers.  Invoked once from worker_pool_t's constructor (H5Pthreads.hpp).
	inline void warm_dispatch() noexcept {
		static std::once_flag once;
		std::call_once(once, []() noexcept {
			unsigned char in[64]  = {0};
			unsigned char comp[256];
			unsigned char back[64];
#if defined(H5CPP_HAS_LIBDEFLATE)
			(void) libdeflate_adler32(1, in, sizeof in);
			(void) libdeflate_crc32(0, in, sizeof in);
			if (libdeflate_compressor* c = libdeflate_alloc_compressor(6)) {
				const size_t n = libdeflate_zlib_compress(c, in, sizeof in, comp, sizeof comp);
				libdeflate_free_compressor(c);
				if (n) if (libdeflate_decompressor* d = libdeflate_alloc_decompressor()) {
					size_t got = 0;
					(void) libdeflate_zlib_decompress(d, comp, n, back, sizeof back, &got);
					libdeflate_free_decompressor(d);
				}
			}
#endif
#if defined(H5CPP_HAS_LZ4)
			(void) LZ4_compress_default(reinterpret_cast<const char*>(in),
			                            reinterpret_cast<char*>(comp),
			                            static_cast<int>(sizeof in),
			                            static_cast<int>(sizeof comp));
#endif
#if defined(H5CPP_HAS_ZSTD)
			(void) ZSTD_compress(comp, sizeof comp, in, sizeof in, 1);
#endif
			(void) in; (void) comp; (void) back;  // unused when all compressors disabled
		});
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

		// Gorilla delta-of-delta XOR compression for time-series floating-point data.
		// Based on Pelkonen et al., VLDB 2015.  Compresses IEEE-754 float32/float64
		// by XOR-ing consecutive values and suppressing leading/trailing zero bits.
		// Params: params[0] = element size in bytes (4 or 8).
		namespace {
			template<typename T>
			inline int gorilla_clz(T value) {
				if (value == 0) return sizeof(T) * 8;
#if defined(__GNUC__) || defined(__clang__)
				if constexpr (sizeof(T) == 8) return __builtin_clzll(value);
				else return __builtin_clz(static_cast<unsigned>(value));
#elif defined(_MSC_VER)
				unsigned long index;
				if constexpr (sizeof(T) == 8) {
					if (_BitScanReverse64(&index, value)) return 63 - static_cast<int>(index);
				} else {
					if (_BitScanReverse(&index, static_cast<unsigned long>(value))) return 31 - static_cast<int>(index);
				}
				return sizeof(T) * 8;
#else
				int count = 0;
				for (int i = sizeof(T) * 8 - 1; i >= 0; --i) {
					if ((value >> i) & 1) break;
					++count;
				}
				return count;
#endif
			}

			template<typename T>
			inline int gorilla_ctz(T value) {
				if (value == 0) return sizeof(T) * 8;
#if defined(__GNUC__) || defined(__clang__)
				if constexpr (sizeof(T) == 8) return __builtin_ctzll(value);
				else return __builtin_ctz(static_cast<unsigned>(value));
#elif defined(_MSC_VER)
				unsigned long index;
				if constexpr (sizeof(T) == 8) {
					_BitScanForward64(&index, value);
				} else {
					_BitScanForward(&index, static_cast<unsigned long>(value));
				}
				return static_cast<int>(index);
#else
				int count = 0;
				for (int i = 0; i < sizeof(T) * 8; ++i) {
					if ((value >> i) & 1) break;
					++count;
				}
				return count;
#endif
			}

			struct bit_writer_t {
				uint8_t* buf;
				size_t byte_pos = 0;
				unsigned bit_pos = 0; // 0-7, MSB first

				void write_bit(bool bit) {
					if (bit_pos == 0) buf[byte_pos] = 0;
					if (bit) buf[byte_pos] |= (1u << (7 - bit_pos));
					if (++bit_pos == 8) { bit_pos = 0; ++byte_pos; }
				}
				void write_bits(uint64_t value, unsigned nbits) {
					for (int i = nbits - 1; i >= 0; --i)
						write_bit((value >> i) & 1);
				}
			};

			struct bit_reader_t {
				const uint8_t* buf;
				size_t byte_pos = 0;
				unsigned bit_pos = 0;

				bool read_bit() {
					bool bit = (buf[byte_pos] >> (7 - bit_pos)) & 1;
					if (++bit_pos == 8) { bit_pos = 0; ++byte_pos; }
					return bit;
				}
				uint64_t read_bits(unsigned nbits) {
					uint64_t result = 0;
					for (unsigned i = 0; i < nbits; ++i)
						result = (result << 1) | (read_bit() ? 1u : 0u);
					return result;
				}
			};

			template<typename T>
			inline size_t gorilla_encode_impl(void* dst, const void* src, size_t size) {
				const size_t element_size = sizeof(T);
				const size_t count = size / element_size;
				if (count == 0) return 0;
				const T* values = static_cast<const T*>(src);
				uint8_t* out = static_cast<uint8_t*>(dst);

				// Header: element_size (1 byte) + count (big-endian uint32_t)
				out[0] = static_cast<uint8_t>(element_size);
				out[1] = static_cast<uint8_t>(count >> 24);
				out[2] = static_cast<uint8_t>(count >> 16);
				out[3] = static_cast<uint8_t>(count >>  8);
				out[4] = static_cast<uint8_t>(count);

				// First value raw
				std::memcpy(out + 5, &values[0], element_size);
				if (count == 1) return 5 + element_size;

				bit_writer_t writer{out + 5 + element_size};
				T prev = values[0];
				int prev_leading = -1;
				int prev_meaningful = -1;

				for (size_t i = 1; i < count; ++i) {
					T curr = values[i];
					T xored = prev ^ curr;
					if (xored == 0) {
						writer.write_bit(false); // same value
					} else {
						writer.write_bit(true);  // different value
						int leading = gorilla_clz(xored);
						int trailing = gorilla_ctz(xored);
						int meaningful = sizeof(T) * 8 - leading - trailing;
						// Original Gorilla condition: current meaningful bits fit inside previous block.
						// Chimp heuristic: only reuse if it's cheaper than paying the 14-bit new-block overhead.
						bool fits_in_prev = prev_leading >= 0 && leading >= prev_leading &&
						    (leading + meaningful) <= (prev_leading + prev_meaningful);
						bool cheaper_to_reuse = fits_in_prev && (2 + prev_meaningful) <= (14 + meaningful);
						if (cheaper_to_reuse) {
							writer.write_bit(false); // same block
							int block_trailing = sizeof(T) * 8 - prev_leading - prev_meaningful;
							writer.write_bits(static_cast<uint64_t>(xored >> block_trailing), prev_meaningful);
						} else {
							writer.write_bit(true);  // new block
							writer.write_bits(static_cast<uint64_t>(leading), 6);
							// meaningful can be 64, which doesn't fit in 6 bits.
							// Encode 64 as 0 (same convention as go-tsz); decoder remaps 0 -> 64.
							writer.write_bits(static_cast<uint64_t>(meaningful == 64 ? 0 : meaningful), 6);
							prev_leading = leading;
							prev_meaningful = meaningful;
							writer.write_bits(static_cast<uint64_t>(xored >> trailing), meaningful);
						}
					}
					prev = curr;
				}
				return 5 + element_size + writer.byte_pos + (writer.bit_pos > 0 ? 1 : 0);
			}

			template<typename T>
			inline size_t gorilla_decode_impl(void* dst, const void* src, size_t size) {
				if (size < 5) return 0;
				const uint8_t* in = static_cast<const uint8_t*>(src);
				// byte 0 = element_size, already verified by caller
				uint32_t count =
					(static_cast<uint32_t>(in[1]) << 24) |
					(static_cast<uint32_t>(in[2]) << 16) |
					(static_cast<uint32_t>(in[3]) <<  8) |
					 static_cast<uint32_t>(in[4]);
				if (size < 5 + sizeof(T) || count == 0) return 0;

				T* values = static_cast<T*>(dst);
				std::memcpy(&values[0], in + 5, sizeof(T));
				if (count == 1) return sizeof(T);

				bit_reader_t reader{in + 5 + sizeof(T)};
				T prev = values[0];
				int prev_leading = -1;
				int prev_meaningful = -1;

				for (uint32_t i = 1; i < count; ++i) {
					if (!reader.read_bit()) {
						values[i] = prev;
					} else {
						int leading, meaningful;
						if (!reader.read_bit()) {
							leading = prev_leading;
							meaningful = prev_meaningful;
						} else {
							leading = static_cast<int>(reader.read_bits(6));
							meaningful = static_cast<int>(reader.read_bits(6));
							// meaningful=64 is encoded as 0 in 6 bits (see encoder).
							if (meaningful == 0) meaningful = 64;
							prev_leading = leading;
							prev_meaningful = meaningful;
						}
						int trailing = sizeof(T) * 8 - leading - meaningful;
						T xored = static_cast<T>(reader.read_bits(meaningful));
						xored <<= trailing;
						values[i] = prev ^ xored;
					}
					prev = values[i];
				}
				return static_cast<size_t>(count) * sizeof(T);
			}
		} // anonymous namespace

		inline size_t gorilla(void* dst, const void* src, size_t size, unsigned flags, size_t n, const unsigned params[]) {
			if (size == 0) return 0;

			if (flags & H5Z_FLAG_REVERSE) {
				// Decode: element_size is self-described in header
				if (size < 1) return 0;
				const uint8_t* in = static_cast<const uint8_t*>(src);
				size_t element_size = in[0];
				if (element_size != 4 && element_size != 8) return 0;
				return (element_size == 4)
					? gorilla_decode_impl<uint32_t>(dst, src, size)
					: gorilla_decode_impl<uint64_t>(dst, src, size);
			} else {
				// Encode: element_size must be provided (n >= 1, params[0] > 0).
				// Auto-detect is intentionally not supported because chunk sizes
				// divisible by both 4 and 8 are ambiguous (float32 vs float64).
				// HDF5 set_local callback or explicit h5::gorilla{N} is the correct
				// way to communicate element size.
				if (n == 0 || params[0] == 0) {
					std::memcpy(dst, src, size);
					return size;
				}
				size_t element_size = params[0];
				if (element_size != 4 && element_size != 8) {
					std::memcpy(dst, src, size);
					return size;
				}
				return (element_size == 4)
					? gorilla_encode_impl<uint32_t>(dst, src, size)
					: gorilla_encode_impl<uint64_t>(dst, src, size);
			}
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
			case H5Z_FILTER_GORILLA:    return filter::gorilla;
			default:
					return filter::error;
		}
	}
}

namespace h5::impl {
	// HDF5 filter callback wrapper for Gorilla XOR compression.
	// Bridges HDF5's filter interface to h5cpp's filter::gorilla.
	inline size_t gorilla_hdf5_filter(unsigned int flags, size_t cd_nelmts,
								  const unsigned int cd_values[],
								  size_t nbytes, size_t* buf_size, void** buf) {
		// Worst-case expansion: ~2x for 64-bit random data + header overhead.
		size_t max_out = nbytes * 2 + 256;
		void* out = std::malloc(max_out);
		if (!out) return 0;

		size_t out_size = filter::gorilla(out, *buf, nbytes, flags, cd_nelmts, cd_values);
		if (out_size == 0) {
			std::free(out);
			return 0;
		}

		if (out_size <= *buf_size) {
			std::memcpy(*buf, out, out_size);
			std::free(out);
			return out_size;
		} else {
			// Hand ownership of the larger buffer to HDF5.
			*buf = out;
			*buf_size = out_size;
			return out_size;
		}
	}

	// Register the Gorilla filter with HDF5. Thread-safe; idempotent.
	inline herr_t gorilla_register_filter() {
		static const H5Z_class2_t gorilla_class = {
			H5Z_CLASS_T_VERS,
			H5Z_FILTER_GORILLA,
			1, 1,
			"gorilla",
			nullptr,
			nullptr,
			gorilla_hdf5_filter
		};
		static std::once_flag flag;
		std::call_once(flag, []() {
			H5Zregister(&gorilla_class);
		});
		return 0;
	}
}
