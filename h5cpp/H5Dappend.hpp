/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#pragma once
#include "H5capi.hpp"
#include "H5Tmeta.hpp"
#include "H5cout.hpp"
#include "H5io_registry.hpp"
#include "H5collector.hpp"   // h5::impl::on_collector — global HDF5 lock under MT, no-op classic
#include <memory>
#include <string>
#include <variant>
#include <array>
#include <cstring>
#include <deque>
#include <future>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <ostream>

namespace h5 {
	struct pt_t;
}
std::ostream& operator<<(std::ostream& os, const h5::pt_t& pt);

namespace h5::impl {
    // pt_t::pipeline selects between two pipeline implementations:
    //
    //   basic_pipeline_t — synchronous filter chain on the calling
    //                      thread; default when the file's FAPL has
    //                      no h5::threads{N} pool installed.
    //   pool_pipeline_t  — FAPL-scoped shared worker pool, async-
    //                      pipelined dispatch with back-pressure.
    //                      Selected when init() resolves a pool
    //                      from the file's FAPL.
    //
    // Both are indirect-owned through unique_ptr so the variant
    // remains move-assignable regardless of the underlying pipeline's
    // move semantics.
    using pt_pipeline_t = std::variant<
        std::unique_ptr<impl::basic_pipeline_t>,
        std::unique_ptr<impl::pool_pipeline_t>
    >;
}

// packet table template specialization with inheritance
namespace h5 {
	struct pt_t {
		pt_t();
		pt_t( const h5::ds_t& handle ); // FAPL-aware ctor: pool when h5::threads{N} is set, basic otherwise
		// deep copy with own cache memory — re-runs init(), so the copy
		// resolves its own pipeline from the dataset's file FAPL.
		pt_t( const h5::pt_t& pt ) : h5::pt_t(pt.ds) {
		};
		~pt_t();

		pt_t& operator=( h5::pt_t&& pt ){
            // prevent self assign
            if (this == &pt) return *this;
            if(H5Iis_valid(static_cast<hid_t>(this->ds))){ // flush and close dataset
                this->flush();
                free(this->fill_value);
            }

            this->ds = std::move(pt.ds);
            this->dxpl = std::move(pt.dxpl);
            this->pipeline = std::move(pt.pipeline);

            this->block_size = pt.block_size;
            this->element_size = pt.element_size;
            this->N = pt.N; this->n = pt.n; this->rank = pt.rank;
            this->ptr = pt.ptr;  this->fill_value = pt.fill_value;

            pt.ptr = nullptr; pt.fill_value = nullptr;
            pt.N=0; pt.n=0; pt.rank=0;
		    for(hsize_t i=0; i<rank; i++){
			    this->offset[i] = pt.offset[i];
                this->current_dims[i] = pt.current_dims[i];
                this->chunk_dims[i] = pt.chunk_dims[i];
                this->count[i] = pt.count[i];
            }
			return *this;
		}
		friend std::ostream& ::operator<<(std::ostream &os, const h5::pt_t& pt);
		template<class T>
		friend void append( h5::pt_t& ds, const T& ref);
		template<class T>
		friend void append( h5::pt_t& ds, const T* ptr);
		friend void flush(h5::pt_t&);
		// resets the packet-table dimension tracker so the same pt_t can be reused
		// for a fresh logical session (e.g. start-of-day re-init in streaming sinks).
		void reset();
		private:
		void init(const h5::ds_t& ds_);
		void flush();

		template<class T> inline std::enable_if_t<h5::meta::is_scalar<T>::value,
		void> append( const T* ptr );
		template<class T> inline std::enable_if_t< h5::meta::is_scalar<T>::value && !std::is_pointer_v<T>,
		void> append( const T& ref );
		template<class T> inline std::enable_if_t< !h5::meta::is_scalar<T>::value,
		void> append( const T& ref );
		void append( const std::string& ref );

		// Resolve the variant to a reference to the live pipeline_t<Derived> CRTP
		// instance, regardless of which alternative is active. Both alternatives
		// expose the same set of fields (chunk0, block_size, ds, dxpl, ...) inherited
		// from pipeline_t<Derived>, so the visiting lambda can use the result
		// duck-typed across alternatives.
		template <typename F>
		decltype(auto) visit_pipeline(F&& f) {
			return std::visit([&](auto& p) -> decltype(auto) {
				return std::forward<F>(f)(*p);   // both alternatives are unique_ptr
			}, pipeline);
		}

		impl::pt_pipeline_t pipeline;
		h5::dxpl_t dxpl;
		h5::ds_t ds;
		h5::dt_t<void> dt;

		hsize_t offset[H5CPP_MAX_RANK], current_dims[H5CPP_MAX_RANK],
			chunk_dims[H5CPP_MAX_RANK], count[H5CPP_MAX_RANK];
		size_t block_size,element_size,N,n,rank;
		void *ptr, *fill_value;

		// Phase 1.3.3 — chunk dispatch is uniform across all variant
		// alternatives via visit_pipeline + write_chunk.  pool_pipeline_t
		// holds the pool reference, in-flight deque, and back-pressure
		// logic internally; pt_t no longer needs per-instance pool fields.
	};
}


/* initialized to invalid state
 * */
inline h5::pt_t::pt_t() :
	pipeline{std::make_unique<impl::basic_pipeline_t>()},
	dxpl{H5Pcreate(H5P_DATASET_XFER)},ds{H5I_UNINIT},n{0},fill_value{nullptr}{
		for(hsize_t i=0; i<H5CPP_MAX_RANK; i++ )
			count[i] = 1, offset[i] = 0;
	}

// FAPL-aware conversion ctor — init() resolves pool from the dataset's
// FAPL and swaps the variant to pool_pipeline_t when h5::threads{N} is
// installed.  Otherwise the default basic_pipeline_t stays active.
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
	this->flush();
	free(this->fill_value);
}

inline
void h5::pt_t::init( const h5::ds_t& handle ){
	try {
		h5::impl::on_collector([&]{   // open + registry resolve + space/type probing under the global lock (MT)
		// Re-open with zero HDF5 chunk cache: the default DAPL allocates ~1MB per H5Dopen2
		// call which accumulates in malloc arenas when pt_t is used in loops, even though
		// the memory is logically freed on close.
		hid_t raw = static_cast<hid_t>(handle);
		hid_t fid = H5Iget_file_id(raw);
		ssize_t nlen = H5Iget_name(raw, nullptr, 0);
		std::vector<char> dname(static_cast<size_t>(nlen) + 1);
		H5Iget_name(raw, dname.data(), dname.size());
		hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
		H5Pset_chunk_cache(dapl, 0, 0, H5D_CHUNK_CACHE_W0_DEFAULT);
		ds = h5::ds_t{H5Dopen2(fid, dname.data(), dapl)};
		H5Pclose(dapl);

		// Phase 1.3.3 / slice C (#286) — H5Fget_access_plist strips user
		// properties, so resolve_worker_pool on a reconstructed FAPL
		// always returns nullptr.  Look up the pool in the per-file
		// registry instead, keyed by H5Fget_fileno.
		{
			const unsigned long fileno = impl::file_key_of_file(fid);
			if (auto pool = impl::registry().resolve_pool(fileno)) {
				const unsigned cap = impl::registry().resolve_cap(fileno);
				pipeline.emplace<std::unique_ptr<impl::pool_pipeline_t>>(
					std::make_unique<impl::pool_pipeline_t>(std::move(pool), cap));
			}
		}

		H5Fclose(fid);
		dt = h5::dt_t<void>{H5Dget_type(static_cast<hid_t>(ds))};
		h5::sp_t file_space = h5::get_space( handle );
		rank = h5::get_simple_extent_dims( file_space, current_dims, nullptr );

		h5::dcpl_t dcpl = h5::get_dcpl( ds );
		h5::dt_t<void*> type = h5::get_type<void*>( ds );
		hsize_t size = h5::get_size( type );
		this->fill_value = h5::get_fill_value(dcpl, type, size);
		visit_pipeline([&](auto& p) {
			p.set_cache(dcpl, size);
			this->ptr = p.chunk0;
			this->block_size = p.block_size;
			this->element_size = p.element_size;
			this->N = p.n;
			p.ds = ds; p.dxpl = dxpl;
		});
		h5::get_chunk_dims( dcpl, chunk_dims );
		for(hsize_t i=1; i<rank; i++)
			current_dims[i] = chunk_dims[i];
		}); // on_collector
	} catch ( ... ){
		throw h5::error::io::packet_table::misc( H5CPP_ERROR_MSG("CTOR: unable to create handle from dataset..."));
	}
}

template<class T> inline std::enable_if_t< h5::meta::is_scalar<T>::value,
void> h5::pt_t::append( const T* ptr ) try {
	//PTR: write directly chunk size from provided buffer/ptr
	*offset = *current_dims;
	*current_dims += *chunk_dims;
	h5::impl::on_collector([&]{   // serialize the HDF5 chunk flush under the global lock (MT)
		h5::set_extent(ds, current_dims);
		visit_pipeline([&](auto& p){ p.write_chunk(offset, block_size, ptr); });
	});
} catch( const std::runtime_error& err ){
	throw h5::error::io::dataset::append( err.what() );
}
inline void h5::pt_t::append( const std::string& ref ) {
	static_cast<const char**>( ptr )[n++] = ref.data();
	if( n != N ) return;

	*offset = *current_dims;
	*current_dims += *chunk_dims;
	h5::impl::on_collector([&]{   // serialize the variable-length flush under the global lock (MT)
		h5::set_extent(ds, current_dims);

		hsize_t block = 1, count = n;
		h5::sp_t mem_space{H5Screate_simple(static_cast<int>(rank), &count, nullptr )};
		h5::sp_t file_space{H5Dget_space( static_cast<::hid_t>(ds) )};
		h5::select_all( mem_space );
		H5Sselect_hyperslab( static_cast<hid_t>(file_space), H5S_SELECT_SET, offset, nullptr, &block, &count);

		H5Dwrite( static_cast<hid_t>( ds ),
			dt, static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space), static_cast<hid_t>(dxpl), ptr);
	});
	n = 0;
}
template <>
inline void h5::pt_t::append( const char* ref ) {
	static_cast<const char**>( ptr )[n++] = ref;
	if( n != N ) return;

	*offset = *current_dims;
	*current_dims += *chunk_dims;
	h5::impl::on_collector([&]{   // serialize the variable-length flush under the global lock (MT)
		h5::set_extent(ds, current_dims);

		hsize_t block = 1, count = n;
		h5::sp_t mem_space{H5Screate_simple(static_cast<int>(rank), &count, nullptr )};
		h5::sp_t file_space{H5Dget_space( static_cast<::hid_t>(ds) )};
		h5::select_all( mem_space );
		H5Sselect_hyperslab( static_cast<hid_t>(file_space), H5S_SELECT_SET, offset, NULL, &block, &count);

		H5Dwrite( static_cast<hid_t>( ds ),
			dt, static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space), static_cast<hid_t>(dxpl), ptr);
	});
	n = 0;
}


template<class T> inline std::enable_if_t< h5::meta::is_scalar<T>::value && !std::is_pointer_v<T>,
void> h5::pt_t::append( const T& ref ) try {
//SCALAR: store inbound data directly in pipeline cache
	static_cast<T*>( ptr )[n++] = ref;
	if( n != N ) return;

	n = 0;
	*offset = *current_dims;
	*current_dims += *chunk_dims;
	h5::impl::on_collector([&]{   // serialize the HDF5 chunk flush under the global lock (MT)
		h5::set_extent(ds, current_dims);
		visit_pipeline([&](auto& p){ p.write_chunk(offset, block_size, ptr); });
	});
} catch( const std::runtime_error& err ){
	throw h5::error::io::dataset::append( err.what() );
}

template<class T> inline std::enable_if_t< !h5::meta::is_scalar<T>::value,
void> h5::pt_t::append( const T& ref ) try {
	// Iterator-only containers (list, forward_list, deque, set, …) have no .data().
	// Delegate element-by-element to the scalar append overload.
	using access = h5::meta::access_traits_t<T>;
	if constexpr (access::kind == h5::meta::access_t::iterators) {
		for (const auto& elem : ref)
			this->append(elem);
	} else {

	auto dims = meta::size( ref );

	*offset = *current_dims;
	*current_dims += 1;
	auto ptr_ = meta::data( ref );
	auto dims_ = meta::size( ref );

	h5::impl::on_collector([&]{   // serialize the HDF5 chunk flush under the global lock (MT)
	h5::set_extent(ds, current_dims);
	switch( dims_.size() ){
		case 1: // vector
			if( dims[0] * element_size == block_size )
				visit_pipeline([&](auto& p){ p.write_chunk(offset, block_size, (void*) ptr_ ); });
			else throw h5::error::io::packet_table::write(
					H5CPP_ERROR_MSG("dimension mismatch: "
						+ std::to_string( dims[0] * element_size) + " != " + std::to_string(block_size) ));
			break;
		case 2: //matrix
			if( dims[0] * dims[1] * element_size == block_size )
				visit_pipeline([&](auto& p){ p.write_chunk(offset, block_size, (void*) ptr_ ); });
			else throw h5::error::io::packet_table::write(
					H5CPP_ERROR_MSG("dimension mismatch: "
						+ std::to_string( dims[0] * dims[1] * element_size) + " != " + std::to_string(block_size) ));
			break;
		case 3: // cube
			if( dims[0] * dims[1] * dims[2] * element_size == block_size )
				visit_pipeline([&](auto& p){ p.write_chunk(offset, block_size, (void*) ptr_ ); });
			else throw h5::error::io::packet_table::write(
					H5CPP_ERROR_MSG("dimension mismatch: "
						+ std::to_string( dims[0] * dims[1] * dims[2] * element_size) + " != " + std::to_string(block_size) ));
			;break;
		default:
			throw h5::error::io::packet_table::misc( H5CPP_ERROR_MSG("objects with rank > 2 are not supported... "));
	}
	}); // on_collector
	} // end else (non-iterator path)
} catch( const std::runtime_error& err ){
	throw h5::error::io::dataset::append( err.what() );
}

inline
void h5::pt_t::flush(){
	// Whole flush — the trailing partial-chunk write AND the pool drain (which
	// issues H5Dwrite_chunk for in-flight compressed chunks) — runs under the
	// global HDF5 lock when built MT; no-op pass-through in a classic build.
	h5::impl::on_collector([&]{
	if( n != 0 ) {
		*offset = *current_dims;
		*current_dims += *chunk_dims;
		h5::set_extent(ds, current_dims);

		if( H5Tis_variable_str(this->dt)) {
			hsize_t block = 1, count = n;
			h5::sp_t mem_space{H5Screate_simple(static_cast<int>(rank), &count, nullptr )};
			h5::sp_t file_space{H5Dget_space( static_cast<::hid_t>(ds) )};
			h5::select_all( mem_space );
			H5Sselect_hyperslab( static_cast<hid_t>(file_space), H5S_SELECT_SET, offset, nullptr, &block, &count);

			H5Dwrite( static_cast<hid_t>( ds ),
				dt, static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space), static_cast<hid_t>(dxpl), ptr);
		} else {
			// the remainder of last chunk must be set to fill_value; arbitrary type size supported
			for(hsize_t i=0; i<(N-n); i++)
				for(size_t j=0; j < element_size; j++)
					static_cast<char*>( ptr )[(n + i) * element_size + j] = static_cast<char*>( fill_value )[ j ];
			visit_pipeline([&](auto& p){ p.write_chunk(offset, block_size, ptr); });
		}
		n = 0;
	}
	// Pool path: drain in-flight chunks so flush() honors the "data
	// on disk after this returns" contract.  basic_pipeline_t writes
	// inline; the visit is a no-op for that alternative.
	std::visit([](auto& p) {
		using T = std::decay_t<decltype(*p)>;
		if constexpr (std::is_same_v<T, impl::pool_pipeline_t>)
			p->drain();
	}, pipeline);
	});
}

inline void h5::pt_t::reset() {
	std::memset(current_dims, 0, H5CPP_MAX_RANK * sizeof(hsize_t));
}

namespace h5 {
	/**
	 * \func_append_hdr
	 * @brief Append a value to the streaming end of a packet table.
	 *
	 * Buffers `ref` into the packet table's in-memory chunk; once the
	 * chunk fills it is flushed to the underlying dataset along the
	 * first (slowest-growing) dimension. Multi-rank packet tables
	 * write a hyperplane at a time — `ref`'s shape must match
	 * `chunk_dims[1..rank-1]`.
	 *
	 * @param pt   open `h5::pt_t` packet-table descriptor.
	 * @param ref  value to append; must match the packet table's
	 *             element type and per-record shape.
	 * \tpar_T
	 * \returns_err
	 *
	 * @throws h5::error::io::dataset::write   on `H5Dwrite` failure
	 *         during a chunk flush.
	 *
	 * <br/><b>example:</b>
	 * @code
	 * h5::fd_t fd = h5::create("stream.h5", H5F_ACC_TRUNC);
	 * h5::pt_t pt = h5::create<float>(fd, "/stream",
	 *                  h5::max_dims{H5S_UNLIMITED}, h5::chunk{1024});
	 *
	 * for (float sample : stream)
	 *     h5::append(pt, sample);
	 *
	 * h5::flush(pt); // explicit flush of the trailing partial chunk
	 * @endcode
	 *
	 * \sa_h5cpp
	 * \sa_hdf5
	 * @sa h5::create h5::flush h5::reset @ref link_handle_reference
	 *     "Handles, Descriptors, and Property Lists"
	 */
	template<class T> inline
	void append( h5::pt_t& pt, const T& ref){
		pt.append( ref );
	}

	/**
	 * \func_append_hdr
	 * @brief Raw-pointer append — writes one full chunk straight from `ptr`.
	 *
	 * Bypasses the per-element buffer used by the by-reference overload
	 * — the caller supplies a contiguous block of exactly
	 * `chunk_dims[0] * ... * chunk_dims[rank-1]` elements and the
	 * packet table flushes it as a single chunk. Use this when you
	 * already have a chunk-sized buffer to deposit.
	 *
	 * Without this overload, raw pointers would bind to the by-ref
	 * template above (with `T` deduced as `<scalar>*`) and route to
	 * the non-scalar member path, which expects a container with
	 * `meta::data` / `meta::size` — silently the wrong dispatch.
	 *
	 * @param pt   open `h5::pt_t` packet-table descriptor.
	 * @param ptr  pointer to a contiguous chunk-sized buffer of `T`.
	 * \tpar_T
	 * \returns_err
	 *
	 * @throws h5::error::io::dataset::write   on `H5Dwrite` failure.
	 *
	 * \sa_h5cpp
	 * @sa h5::append(h5::pt_t&, const T&)
	 */
	template<class T> inline
	void append( h5::pt_t& pt, const T* ptr){
		pt.append( ptr );
	}

	/**
	 * \func_append_hdr
	 * @brief Flush the packet table's pending in-memory chunk to disk.
	 *
	 * The destructor of `h5::pt_t` also flushes; call this explicitly
	 * when you need to make the streamed data visible before the
	 * descriptor goes out of scope (e.g. for an external reader, or
	 * before mid-program `h5::read` on the same dataset).
	 *
	 * @param pt  open `h5::pt_t` packet-table descriptor.
	 * \returns_err
	 *
	 * @throws h5::error::io::dataset::close   on `H5Dwrite` failure
	 *         during the flush.
	 *
	 * \sa_h5cpp
	 * @sa h5::append h5::reset
	 */
	inline void flush(h5::pt_t& pt) try {
		pt.flush();
        //TODO: find better mechanism for deprecating code: #pragma message("not implemented: do not call pt_t::flush() ...")
		// for now
	} catch ( const std::runtime_error& e){
		throw h5::error::io::dataset::close( e.what() );
	}

	/**
	 * \func_append_hdr
	 * @brief Reset the packet table's dimension tracker for reuse.
	 *
	 * Zeros the in-memory position so the same `pt_t` can stream a
	 * fresh logical session into the same underlying dataset. Does
	 * **not** shrink the dataset on disk — the caller is responsible
	 * for any HDF5-level cleanup (truncate via `H5Dset_extent`, etc.).
	 *
	 * @param pt  open `h5::pt_t` packet-table descriptor.
	 * \returns_err
	 *
	 * @throws h5::error::io::dataset::write   on internal flush failure.
	 *
	 * \sa_h5cpp
	 * @sa h5::append h5::flush
	 */
	inline void reset(h5::pt_t& pt) try {
		pt.reset();
	} catch ( const std::runtime_error& e){
		throw h5::error::io::dataset::write( e.what() );
	}
}

inline std::ostream& operator<<(std::ostream &os, const h5::pt_t& pt) {
    os << std::dec;
	os <<"packet table:\n"
		 "------------------------------------------\n";
    if( !H5Iis_valid(static_cast<hid_t>(pt.ds))) {
        os << "ds: H5I_UNINIT" <<std::endl;
        return os;
    }
	std::vector<size_t> current_dims(pt.current_dims, pt.current_dims + pt.rank);
	std::vector<size_t> chunk_dims(pt.chunk_dims, pt.chunk_dims + pt.rank);
	std::vector<size_t> count(pt.count, pt.count + pt.rank);
	std::vector<size_t> offset(pt.offset, pt.offset + pt.rank);

	os << "rank: " << pt.rank << " N:" << pt.N <<" n:" << pt.n << "\n";
	os << "element size: " << pt.element_size << " block size: " << pt.block_size << "\n";
	os << "current dims: " << current_dims << std::endl;
	os << "chunk dims: " << chunk_dims << std::endl;
	os << "offset : " <<  offset <<  " count : " << count << std::endl;
	os << "ds: "<< static_cast<hid_t>( pt.ds ) <<" dxpl: "<< static_cast<hid_t>( pt.dxpl ) << std::endl;
	os << "fill value: " << std::hex << pt.fill_value << " buffer: " << pt.ptr;
	os << "\n\n";
    os << std::dec;
	return os;
}
