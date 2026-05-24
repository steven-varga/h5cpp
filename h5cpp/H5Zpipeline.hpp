/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <ostream>
#include <cstring>
#include <utility>
#include <cstdlib>
#include <new>

#include "H5Iall.hpp"
#include "H5Sall.hpp"
#include "H5Zall.hpp"

#ifdef _WIN32
#include <malloc.h>
#endif

namespace h5 {
	int get_chunk_dims( const h5::dcpl_t& dcpl,  h5::chunk_t& chunk_dims );
}

namespace h5{ namespace impl {

	struct aligned_deleter {
		void operator()(char* ptr) const {
#ifdef _WIN32
			_aligned_free(ptr);
#else
			std::free(ptr);
#endif
		}
	};
	using aligned_ptr = std::unique_ptr<char, aligned_deleter>;

	inline size_t round_up_to_alignment(size_t size, size_t alignment) {
		return alignment ? ((size + alignment - 1) / alignment) * alignment : size;
	}

	inline aligned_ptr make_aligned(size_t alignment, size_t size) {
		const size_t allocation_size = round_up_to_alignment(size, alignment);
		void* ptr = nullptr;
#ifdef _WIN32
		ptr = _aligned_malloc(allocation_size, alignment);
#else
		if (posix_memalign(&ptr, alignment, allocation_size) != 0)
			ptr = nullptr;
#endif
		if (!ptr)
			throw std::bad_alloc();
		return aligned_ptr(static_cast<char*>(ptr));
	}

	// ------------------------------------------------------------------
	// Bump-pointer arena: eliminates per-allocation system calls.
	// Default 256 MiB holds ~32K 8 KB chunks; tunable at construction.
	// ------------------------------------------------------------------
	struct chunk_arena_t {
		static constexpr size_t alignment = H5CPP_MEM_ALIGNMENT;
		static constexpr size_t default_capacity = 256 * 1024 * 1024;

		struct aligned_deleter {
			void operator()(char* ptr) const { std::free(ptr); }
		};
		std::unique_ptr<char, aligned_deleter> base;
		char* bump = nullptr;
		char* end = nullptr;

		explicit chunk_arena_t(size_t capacity = default_capacity) {
			void* ptr = nullptr;
			if (posix_memalign(&ptr, alignment, capacity) != 0)
				throw std::bad_alloc();
			base.reset(static_cast<char*>(ptr));
			bump = base.get();
			end = bump + capacity;
		}

		chunk_arena_t(chunk_arena_t&&) = default;
		chunk_arena_t& operator=(chunk_arena_t&&) = default;
		chunk_arena_t(const chunk_arena_t&) = delete;
		chunk_arena_t& operator=(const chunk_arena_t&) = delete;

		[[nodiscard]] char* allocate(size_t size) {
			size = round_up_to_alignment(size, alignment);
			if (bump + size > end) [[unlikely]]
				return allocate_fallback(size);
			char* ptr = bump;
			bump += size;
			return ptr;
		}

		void reset() noexcept { bump = base.get(); }

		bool owns(const void* ptr) const noexcept {
			const char* p = static_cast<const char*>(ptr);
			return p >= base.get() && p < end;
		}

	private:
		[[nodiscard]] char* allocate_fallback(size_t size) {
			void* ptr = nullptr;
			if (posix_memalign(&ptr, alignment, size) != 0)
				throw std::bad_alloc();
			return static_cast<char*>(ptr);
		}
	};

	// ------------------------------------------------------------------
	// Processor-matched memory operations
	// ------------------------------------------------------------------
#if defined(__AVX__)
	#include <immintrin.h>
#endif

	inline void simd_memset_zero(char* dst, size_t n) {
#if defined(__AVX2__)
		size_t m = n;
		const __m256i zero = _mm256_setzero_si256();
		for (; m >= 32; m -= 32, dst += 32)
			_mm256_store_si256(reinterpret_cast<__m256i*>(dst), zero);
		std::memset(dst, 0, m);
#else
		std::memset(dst, 0, n);
#endif
	}

	inline void nontemporal_memcpy(char* __restrict dst, const char* __restrict src, size_t n) {
#if defined(__AVX__)
		size_t m = n;
		for (; m >= 32; m -= 32, dst += 32, src += 32) {
			_mm256_stream_si256(reinterpret_cast<__m256i*>(dst),
				_mm256_loadu_si256(reinterpret_cast<const __m256i*>(src)));
		}
		_mm_sfence();
		std::memcpy(dst, src, m);
#else
		std::memcpy(dst, src, n);
#endif
	}

	enum struct filter_direction_t {
		forward = 0, reverse = 1
	};

	template <class Derived>
	struct pipeline_t {
		pipeline_t(){};
		~pipeline_t(){};
		pipeline_t& operator=( pipeline_t&& rhs ) {
            if (this == &rhs) return *this;

            this->chunk0 = rhs.chunk0; rhs.chunk0 = nullptr;
            this->chunk1 = rhs.chunk1; rhs.chunk1 = nullptr;
            this->tail = rhs.tail; rhs.tail = 0;
            this->rank = rhs.rank; rhs.rank = 0;

            this->arena = std::move(rhs.arena);
            memcpy(filter, rhs.filter,  sizeof(filter));
            memcpy(filter_id, rhs.filter_id, sizeof(filter_id));

            memcpy(cd_values, rhs.cd_values,  sizeof(cd_values));
            memcpy(cd_size, rhs.cd_size,  sizeof(cd_size));
            memcpy(flags, rhs.flags, sizeof(flags));
            n = rhs.n; block_size = rhs.block_size; element_size = rhs.element_size;
            memcpy(C, rhs.C, sizeof(C));
            memcpy(B, rhs.B, sizeof(B));
            memcpy(D, rhs.D, sizeof(D));
            memcpy(N, rhs.N, sizeof(N));
            memcpy(Rx, rhs.Rx, sizeof(Rx));
            memcpy(Ry, rhs.Ry, sizeof(Ry));

            this->dcpl = std::move(rhs.dcpl);
            this->dxpl = std::move(rhs.dxpl);
            this->ds = std::move(rhs.ds);

            return *this;
        }

		void set_cache( const h5::dcpl_t& dcpl, size_t element_size );
		void write(const h5::ds_t& ds, const h5::offset_t& start, const h5::stride_t& stride, const h5::block_t& block, const h5::count_t& count,
				const h5::dxpl_t& dxpl, const void* ptr);

		void read(const h5::ds_t& ds, const h5::offset_t& start, const h5::stride_t& stride, const h5::block_t& block, const h5::count_t& count,
				const h5::dxpl_t& dxpl, void* ptr);

		void write_chunk(  const hsize_t* offset, size_t nbytes, const void* ptr ){
			static_cast<Derived*>(this)->write_chunk_impl(offset, nbytes, ptr);
		}
		void read_chunk( const hsize_t* offset, size_t nbytes, void* ptr ){
			static_cast<Derived*>(this)->read_chunk_impl(offset, nbytes, ptr);
		}
		void split_to_chunk_write(filter_direction_t direction, const hsize_t* offset, const hsize_t* dims, const void* ptr );
		void split_to_chunk_read(filter_direction_t direction, const hsize_t* offset, const hsize_t* dims, void* ptr );

		char *chunk0, *chunk1;
		hsize_t tail,rank;

	public:
		void push( filter::call_t filter );
		void pop();

		chunk_arena_t arena;
		filter::call_t filter[H5CPP_MAX_FILTER];
		H5Z_filter_t filter_id[H5CPP_MAX_FILTER];
		hsize_t n,
				C[H5CPP_MAX_RANK], D[H5CPP_MAX_RANK],
				N[H5CPP_MAX_RANK], B[H5CPP_MAX_RANK], Rx[H5CPP_MAX_RANK],Ry[H5CPP_MAX_RANK];
		unsigned cd_values[H5CPP_MAX_FILTER][H5CPP_MAX_FILTER_PARAM],
				flags[H5CPP_MAX_FILTER];
		size_t 	block_size, element_size, cd_size[H5CPP_MAX_FILTER];
		h5::dcpl_t dcpl;
		h5::dxpl_t dxpl;
		h5::ds_t ds;
	};


	struct basic_pipeline_t : public pipeline_t<basic_pipeline_t>{
		void write_chunk_impl( const hsize_t* offset, size_t nbytes, const void* ptr );
		void read_chunk_impl( const hsize_t* offset, size_t nbytes, void* ptr );
	};
		struct romio_pipeline_t : public pipeline_t<romio_pipeline_t>{
			void write_chunk_impl( const hsize_t* offset, size_t nbytes, const void* ptr ){
				(void)offset; (void)nbytes; (void)ptr;
			}
			void read_chunk_impl( const hsize_t* offset, size_t nbytes, void* ptr ){
				(void)offset; (void)nbytes; (void)ptr;
			}
		};
		struct hadoop_pipeline_t : public pipeline_t<hadoop_pipeline_t>{
			void write_chunk_impl( const hsize_t* offset, size_t nbytes, const void* ptr ){
				(void)offset; (void)nbytes; (void)ptr;
			}
			void read_chunk_impl( const hsize_t* offset, size_t nbytes, void* ptr ){
				(void)offset; (void)nbytes; (void)ptr;
			}
		};
}}

template< class Derived>
	inline void h5::impl::pipeline_t<Derived>::write(
			const h5::ds_t& ds, const h5::offset_t& offset, const h5::stride_t& stride, const h5::block_t& block, const h5::count_t& count,
					const h5::dxpl_t& dxpl, const void* ptr){

		(void)stride;
		h5::offset_t offset_; h5::count_t count_;
		for(hsize_t i=0; i<rank; i++)
			offset_[i] = offset[i], count_[i] = count[i] * block[i];
	this->dxpl = dxpl; this->ds = ds;
	split_to_chunk_write(filter_direction_t::forward, offset_.begin(), count_.begin(), ptr );
}

template< class Derived>
	inline void h5::impl::pipeline_t<Derived>::read(
			const h5::ds_t& ds, const h5::offset_t& offset, const h5::stride_t& stride, const h5::block_t& block, const h5::count_t& count,
					const h5::dxpl_t& dxpl, void* ptr){

		(void)stride;
		h5::offset_t offset_; h5::count_t count_;
		for(hsize_t i=0; i<rank; i++)
			offset_[i] = offset[i], count_[i] = count[i] * block[i];
	this->dxpl = dxpl; this->ds = ds;
	split_to_chunk_read(filter_direction_t::reverse, offset_.begin(), count_.begin(), ptr );
}

template< class Derived>
inline void h5::impl::pipeline_t<Derived>::set_cache( const h5::dcpl_t& dcpl, size_t element_size ) {
	n = 1, tail = 0; this->element_size = element_size;

	// grab chunk dimensions, which is a block we're breaking data into
	h5::chunk_t block;
	if( (rank = h5::get_chunk_dims( dcpl, block )) == 0  )
			throw std::runtime_error("data-space is rank 0, is data space a scalar? ");

	//fix B block/chunk size for the lifespan of pipeline
	for(hsize_t i=0; i<rank; i++ )
	   	n *= block[i], B[i] = block[rank-i-1];

	block_size = n*element_size;
	unsigned filter_config;
	unsigned N = H5Pget_nfilters( dcpl );
	for(unsigned i=0; i<N; i++){
		cd_size[i] = H5CPP_MAX_FILTER_PARAM;
		H5Z_filter_t id = H5Pget_filter2( dcpl, i, &flags[i], &cd_size[i], cd_values[i], 0, nullptr, &filter_config );
		push( filter::get_callback( id ) );
		filter_id[i] = id;
		// Guarantee that params[1] always holds the uncompressed chunk byte count as a
		// reliable decompression output-size hint.  External HDF5 files written by
		// community plugins (LZ4 ID 32004, Zstd ID 32015, …) may store only
		// compression level in params[0] and leave params[1] absent; libdeflate also
		// requires the exact output size.  Overwriting here is safe: encoders only read
		// params[0] (compression level) and ignore params[1].
		if (cd_size[i] < 2) cd_size[i] = 2;
		cd_values[i][1] = static_cast<unsigned>(block_size & 0xFFFFFFFFu);
	}

	const size_t scratch_size = filter::filter_scratch_bound(block_size);
	chunk0 = arena.allocate(scratch_size);
	chunk1 = arena.allocate(scratch_size);

	if( chunk0 == nullptr || chunk1 == nullptr )
	   	throw h5::error::io::dataset::open( H5CPP_ERROR_MSG("CTOR: couldn't allocate memory for caching chunks, invalid/check size?"));
}

#define h5cpp_outer( idx ) for( j##idx =0; j##idx < n##idx; j##idx += b##idx)
#define h5cpp_inner( idx ) for( i##idx = j##idx; i##idx < std::min(j##idx+b##idx,n##idx); i##idx++)
	#define h5cpp_def( idx ) [[maybe_unused]] hsize_t i##idx=0, j##idx = 0, s##idx=0, n##idx=N[idx], b##idx=B[idx], rx##idx=Rx[idx],  ry##idx=Ry[idx];

template< class Derived>
	inline void h5::impl::pipeline_t<Derived>::split_to_chunk_read(
		filter_direction_t direction, const hsize_t* chunk_offset, const hsize_t* dims, void* ptr_ ){
		(void)direction;
		char* ptr = static_cast<char*>( ptr_ );
	// compute edges if any - for the data size
	// computes R residuals, and sets N dimension in reverse order
	// when actual data dimensions - current_dims are greater then a chunk, it must be broken into chunk size
	// if there are residuals on the edges, then padding is needed.
	unsigned i=0;
	for(; i<rank; i++ )
		N[i] = dims[rank-i-1], Ry[i] = N[i] % B[i], Rx[i] = B[i] - chunk_offset[i] % B[i];
	// set the rest to default
	for(; i<H5CPP_MAX_RANK; i++) N[i] = B[i] = 1, Rx[i] = Ry[i] = 0;

	// b - block size, j - block index pos, n - actual dimension of data
	// rx - remainder at the leading edges, ry - remainder at trailing edges 
	h5cpp_def(0) h5cpp_def(1) h5cpp_def(2) h5cpp_def(3) h5cpp_def(4) h5cpp_def(5) h5cpp_def(6)

	h5cpp_outer( 6 ){ h5cpp_outer( 5 ){ h5cpp_outer( 4 ){ h5cpp_outer( 3 ){
	h5cpp_outer( 2 ){ h5cpp_outer( 1 ){ h5cpp_outer( 0 ){
		char* p = chunk0;
		// at this point we have a single 'chunk' in this->chunk0 buffer ready to go
		// the coordinates are in j indices
		D[0] = j0, D[1]=j1, D[2]=j2, D[3]=j3, D[4]=j4, D[5]=j5, D[6]=j6;
		//coordinates are reversed in D, invert them:
			for(hsize_t k=0;k<rank; k++ ) C[k] = D[rank-k-1] + chunk_offset[k];
		// execute filters in reverse direction
		read_chunk( this->C, this->block_size, this->chunk0 );

		h5cpp_inner(6){	h5cpp_inner(5){ h5cpp_inner(4){
		h5cpp_inner(3){ h5cpp_inner(2){ h5cpp_inner(1){
			hsize_t offset = i6*n5*n4*n3*n2*n1*n0 + i5*n4*n3*n2*n1*n0 +
					i4*n3*n2*n1*n0 + i3*n2*n1*n0 + i2*n1*n0 + i1*n0 + j0;
			if( j0 != n0 - ry0 ) // block copy
				memcpy(ptr + offset * element_size, p, b0 * element_size );
			else // edge handling
				memcpy(ptr + offset * element_size,p, ry0 * element_size );
			p += b0 * element_size;
		}}}}}}
	}}}}}}}
}

template< class Derived>
	inline void h5::impl::pipeline_t<Derived>::split_to_chunk_write(
		filter_direction_t direction, const hsize_t* chunk_offset, const hsize_t* dims, const void* ptr_ ){
		(void)direction;
		const char* ptr = static_cast<const char*>( ptr_ ); const hsize_t* O = chunk_offset;
	// compute edges if any - for the data size
	// computes R residuals, and sets N dimension in reverse order
	// when actual data dimensions - current_dims are greater then a chunk, it must be broken into chunk size
	// if there are residuals on the edges, then padding is needed.
	unsigned i=0;
	for(; i<rank; i++ )
		N[i] = dims[rank-i-1], Rx[i] = B[i] - O[i] % B[i], Ry[i] = (N[i] - Rx[i]) % B[i];
	for(; i<H5CPP_MAX_RANK; i++) N[i] = B[i] = 1, Rx[i] = Ry[i] = 0;

	// b - block size, j - block index pos, n - actual dimension of data
	// rx - remainder at the leading edges, ry - remainder at trailing edges 
	h5cpp_def(0) h5cpp_def(1) h5cpp_def(2) h5cpp_def(3) h5cpp_def(4) h5cpp_def(5) h5cpp_def(6)

	// rank-1 fast path: single large memcpy per chunk, no nested loops
	if (rank == 1 && (O[0] % B[0]) == 0) [[likely]] {
		constexpr hsize_t prefetch_distance = 4;
		for (hsize_t j = 0; j < N[0]; j += B[0]) {
			if (j + (prefetch_distance + 1) * B[0] < N[0])
				__builtin_prefetch(ptr + (j + prefetch_distance * B[0]) * element_size, 0, 3);
			hsize_t bytes_in_chunk = (j + B[0] <= N[0]) ? B[0] : (N[0] - j);
			hsize_t bytes_to_copy = bytes_in_chunk * element_size;
			if (bytes_to_copy < block_size) [[unlikely]]
				simd_memset_zero(chunk0, block_size);
			if (tail == 0)
				std::memcpy(chunk0, ptr + j * element_size, bytes_to_copy);
			else
				memcpy(chunk0, ptr + j * element_size, bytes_to_copy);
			C[0] = j + O[0];
			write_chunk(C, block_size, chunk0);
		}
		return;
	}

	h5cpp_outer( 6 ){ h5cpp_outer( 5 ){ h5cpp_outer( 4 ){ h5cpp_outer( 3 ){
	h5cpp_outer( 2 ){ h5cpp_outer( 1 ){ h5cpp_outer( 0 ){
		char* p = chunk0;
		// reset memory page only on the edges
		if( j0 + b0 > n0 || j1 + b1 > n1 || j2 + b2 > n2 ||
			j3 + b3 > n3 || j4 + b4 > n4 || j5 + b5 > n5 || j6 + b6 > n6 ) memset(p,0x00, block_size);

		h5cpp_inner(6){	h5cpp_inner(5){ h5cpp_inner(4){
		h5cpp_inner(3){ h5cpp_inner(2){ h5cpp_inner(1){
			hsize_t offset = i6*n5*n4*n3*n2*n1*n0 + i5*n4*n3*n2*n1*n0 +
					i4*n3*n2*n1*n0 + i3*n2*n1*n0 + i2*n1*n0 + i1*n0 + j0;
			if( j0 != n0 - ry0 ) // block copy
				memcpy(p, ptr + offset * element_size, b0 * element_size );
			else // trailing edge handling
				memcpy(p, ptr + offset * element_size, ry0 * element_size );
			p += b0 * element_size;
		}}}}}}
		// at this point we have a single 'chunk' in this->chunk0 buffer ready to go
		// the coordinates are in j indices
		D[0] = j0, D[1]=j1, D[2]=j2, D[3]=j3, D[4]=j4, D[5]=j5, D[6]=j6;
		//coordinates are reversed in D, invert them:
			for(hsize_t k=0;k<rank; k++ ) C[k] = D[rank-k-1] + chunk_offset[k];

		// execute filters in direction
		write_chunk( this->C, this->block_size, this->chunk0 );
	}}}}}}}
}
#undef h5cpp_def
#undef h5cpp_inner
#undef h5cpp_outer

template< class Derived>
inline void h5::impl::pipeline_t<Derived>::push(filter::call_t filter_){
	filter[tail++] = filter_;
}

template< class Derived>
inline void h5::impl::pipeline_t<Derived>::pop(){
	tail--;
}
template<class T>
inline std::ostream& operator<<(std::ostream &os, const h5::impl::pipeline_t<T>& p) {
    os << std::dec;
	os <<"pipeline:\n"
		 "------------------------------------------\n";
    os << "n: " << p.n;
	return os;
}
