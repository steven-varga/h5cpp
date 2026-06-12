/*
 * Copyright (c) 2018 - 2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include "H5capi.hpp"
#include "H5Tmeta.hpp"
#include "H5Tsparse.hpp"
#include "H5Dopen.hpp"
#include "H5Dgather.hpp"
#include "H5Dscatter.hpp"
#include "H5io_registry.hpp"

namespace h5 {
  /** \func_write_hdr
	* @brief write the memory content of `const T* ptr` with given `mem_space`  into `file_space` of the dataset
	* \par_ds
	* @param mem_space the dimensions of the memory region being transferred
	* @param file_space the dimensions of the target dataset region `nelems(mem_space) == nelems(file_space)`
	* @param dxpl data transfer property list (`h5::dxpl_t`)
	* \par_ptr
	* \tpar_T
 	*/ 
	template <class T>
	inline void write( const h5::ds_t& ds, const h5::sp_t& mem_space, const h5::sp_t& file_space, const h5::dxpl_t& dxpl, const T* ptr  ){
		H5CPP_CHECK_PROP( dxpl, h5::error::io::dataset::write, "invalid data transfer property" );
		using element_t = typename h5::impl::decay<T>::type;
		h5::meta::resolved_type_t<element_t> type;
		H5CPP_CHECK_NZ(
			H5Dwrite( static_cast<hid_t>( ds ), type, static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space), static_cast<hid_t>(dxpl), ptr),
				h5::error::io::dataset::write, h5::error::msg::write_dataset);
	}

   /** \func_write_hdr
 	* @brief writes data, from contiguous memory region  into an existing, opened dataset specified by`h5::ds_t` descriptor 
	* Lower level template with generative programming paradigm constructs an optimal function respec to specified arguments
	* \par_ds
	* \par_ptr
    * \par_args
	* \returns_ds
	* 
	* \tpar_T
	*
	* <br/>The following arguments are context sensitive, may be passed in arbitrary order and with the exception
	* of `const T*` pointing to the memory region being saved, the arguments are optional. By default the arguments are set to sensible values,
	* and in most cases the function call will deliver good performance. With that in mind, the options below provide an easy high level fine
	* tuning mechanism to get the best experience without calling HDF5 CAPI functions directly. 
    *
	*
	* \par_count
	* in hyperslab selection; instead from this value the correct `slab_count` is derived considering the block size whenever applicable with a normalisation step:
    * `slab_count[i] = h5::count[i] / h5::block[i]`
	* \par_stride
	* \par_block
	* \par_offset
	* \par_dcpl
	* \par_dxpl
	* 
	* <br/><b>example:</b>
	* @code
	* std::vector<int> data = ...
	* //creates an extendable, chunked dataset, with initial size matching of the vector
	* h5::fd_t fd = h5::open("example.h5", H5F_ACC_RDWR);
	* h5::ds_t ds = h5::write(fd, "path/chunked layout dataset", data,
	* 		h5::max_dims{H5S_UNLIMITED}, h5::chunk{1024} | h5::gzip{9});
	* //creates a contiguous layout dataset, with size matching `data` 
	* h5::ds_t ds = h5::write(fd, "path/contiguous layout dataset", data);
	* //interop with HDF5 CAPI: will flush dataset `ds` by directly calling libhdf5 function
	* H5Dflush(ds); 
	* @endcode 
 	*/ 
 
	template <class T, class... args_t>
	inline h5::ds_t write( const h5::ds_t& ds, const T* ptr,  args_t&&... args  ) try {
		// element types: pod | [signed|unsigned](int8 | int16 | int32 | int64) | float | double
		using tcount = typename arg::tpos<const h5::count_t&,const args_t&...>;
		using toffset = typename arg::tpos<const h5::offset_t&, const args_t&...>;
		using tstride = typename arg::tpos<const h5::stride_t&, const args_t&...>;
		using tblock = typename arg::tpos<const h5::block_t&, const args_t&...>;
		static_assert( tcount::present,"h5::count_t{ ... } must be provided to describe T* memory region" );
		//static_assert( utils::is_supported<T>, "error: " H5CPP_supported_elementary_types );

		auto tuple = std::forward_as_tuple(args...);
		h5::count_t count = std::get<tcount::value>( tuple );  //we make a copy of it
		const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...); // gets reference to default value if property not present
		h5::sp_t file_space{H5Dget_space( static_cast<::hid_t>(ds) )};

		int rank = h5::get_simple_extent_ndims( file_space );
		hsize_t n_elements = static_cast<hsize_t>(count);
		// Resolve threads{N} from the dataset's REAL access plist, not ds.dapl — that
		// member caches the open-time hid_t, which dangles once a temporary dapl
		// (h5::open(fd, name, h5::threads{N})) is destroyed.  See H5Dread.hpp.  #287.
		hid_t dapl = H5Dget_access_plist( static_cast<hid_t>(ds) );
		const unsigned dapl_threads = h5::impl::resolve_dataset_threads(dapl);
		const unsigned dapl_cap = dapl_threads
			? h5::impl::resolve_dataset_backpressure(dapl, dapl_threads) : 0u;
		if (dapl >= 0) H5Pclose(dapl);
		herr_t err = 0;

		// ── Write-path dispatch (3 options) ──────────────────────────────────
		//   1. CAPI hyperslab — h5::offset/stride/block present (COMPILE-TIME),
		//      or a non-chunked dataset: HDF5's own chunk processor + filters.
		//   2. direct chunk (DEFAULT) — no hyperslab, chunked, no h5::threads{N}:
		//      basic_pipeline_t → H5Dwrite_chunk with h5cpp's filter chain.
		//   3. parallel — no hyperslab, chunked, h5::threads{N} on the dataset's
		//      DAPL: pool_pipeline_t fans compression across the global pool.
		// Options 2/3 need H5D_CHUNKED (H5Dwrite_chunk); contiguous/compact falls
		// to option 1 (also dodges the #242 MSVC SegFault).  Whether a hyperslab
		// is selected is a COMPILE-TIME fact (tag presence), so a hyperslab write
		// can never reach the pool — the exclusion is structural, no runtime guard.
		constexpr bool hyperslab = toffset::present || tstride::present || tblock::present;
		bool direct_chunk = false;
		if constexpr (!hyperslab) {
			hid_t dcpl_id = H5Dget_create_plist(static_cast<hid_t>(ds));
			if (dcpl_id >= 0) {
				if (H5Pget_layout(dcpl_id) == H5D_CHUNKED) {
					// Options 2/3 apply the filter chain in-process.  NBIT and
					// SCALEOFFSET are HDF5-internal transforms h5cpp passes through
					// (it relies on the C library to apply them); direct-chunk would
					// write them UNFILTERED, so fall back to the CAPI path (option 1).
					direct_chunk = true;
					int nf = H5Pget_nfilters(dcpl_id);
					for (int fi = 0; fi < nf; ++fi) {
						size_t ne = 0; unsigned fl = 0, cfg = 0;
						H5Z_filter_t id = H5Pget_filter2(dcpl_id, static_cast<unsigned>(fi),
							&fl, &ne, nullptr, 0, nullptr, &cfg);
						if (id == H5Z_FILTER_NBIT || id == H5Z_FILTER_SCALEOFFSET) {
							direct_chunk = false; break;
						}
					}
				}
				H5Pclose(dcpl_id);
			}
		}
		if( direct_chunk ){
			const h5::block_t&  block  = arg::get( h5::default_block,  args...);
			const h5::offset_t& offset = arg::get( h5::default_offset, args...);
			const h5::stride_t& stride = arg::get( h5::default_stride, args...);
			// A 1-D container written to an N-D chunked dataset arrives with a
			// rank-1 count, but the buffer is a flat image of the full dataset —
			// tile by the dataset's actual N-D dimensions so the chunk decomposition
			// is correct (the stock CAPI path reshaped via the dataspace; the
			// direct-chunk pipeline must be told the real dims).
			h5::count_t eff_count = count;
			if (count.rank < static_cast<unsigned>(rank)) {
				hsize_t fd_dims[H5CPP_MAX_RANK];
				H5Sget_simple_extent_dims(static_cast<hid_t>(file_space), fd_dims, nullptr);
				eff_count.rank = static_cast<unsigned>(rank);
				for (int i = 0; i < rank; i++) eff_count[i] = fd_dims[i];
			}
			// set_cache populates the filter chain from the dataset's DCPL — both
			// pipelines need it before write() (the old DAPL pipeline had it
			// cached at dataset-open; an inline one does not).
			h5::dcpl_t dcpl{H5Dget_create_plist(static_cast<hid_t>(ds))};
			hid_t type_id  = H5Dget_type(static_cast<hid_t>(ds));
			size_t elem_sz = H5Tget_size(type_id);
			H5Tclose(type_id);
			// threads{N} lives on the dataset's DAPL — resolved up front from the real
			// access plist (no #286 registry).
			if (dapl_threads > 0) {                        // OPTION 3 — parallel
				h5::impl::pool_pipeline_t pipe(h5::impl::global_pool_ptr(), dapl_cap);
				pipe.set_cache(dcpl, elem_sz);
				pipe.write(ds, offset, stride, block, eff_count, dxpl, ptr);
				// pipe destructor drains in_flight before returning.
			} else {                                       // OPTION 2 — direct chunk (default)
				h5::impl::pipeline_t<impl::basic_pipeline_t> pipe;
				pipe.set_cache(dcpl, elem_sz);
				pipe.write(ds, offset, stride, block, eff_count, dxpl, ptr);
			}
		} else {
			// Scalar dataspaces don't support hyperslab selection; H5Sselect_all
			// on both sides is the equivalent path for H5S_SCALAR file spaces.
			H5S_class_t file_cls = H5Sget_simple_extent_type(static_cast<hid_t>(file_space));
			h5::sp_t mem_space = (file_cls == H5S_SCALAR)
				? h5::sp_t{H5Screate(H5S_SCALAR)}
				: h5::create_simple( n_elements );
			h5::select_all( mem_space );
			if (file_cls == H5S_SCALAR) {
				err = H5Sselect_all(static_cast<hid_t>(file_space));
			} else if constexpr (toffset::present || tstride::present || tblock::present){
				// HYPERBLOCK selection: we either have the argument in `args...` or using default values
				const h5::block_t& block = arg::get( h5::default_block, args...);
				const h5::offset_t& offset = arg::get( h5::default_offset, args...);
				const h5::stride_t& stride = arg::get( h5::default_stride, args...);
				if constexpr( tblock::present ){ // we have to normalise `count` such that `size[i] = count[i] * block[i]` holds
					for(int i=0; i < rank; i++) count[i] /= block[i];
					err = H5Sselect_hyperslab(static_cast<hid_t>(file_space), H5S_SELECT_SET, *offset, *stride, *count, *block);
				} else { // we have to convert h5::count_t{..} to h5::block{..} and initiate a single block transfer
					h5::block_t block_ = static_cast<h5::block_t>(count);
					block_.rank = rank; // memory space may have different rank, be sure to use the rank of file_space
					err = H5Sselect_hyperslab(static_cast<hid_t>(file_space), H5S_SELECT_SET, *offset, *stride, *h5::default_count, *block_);
				}
			} else // SELECT_ALL this is the fastest approach, mem_space and file_space must match
				err = H5Sselect_all(static_cast<hid_t>(file_space));
			// throw an exception if eny error
			H5CPP_CHECK_NZ(err, h5::error::io::dataset::write, h5::error::msg::select_hyperslab);
			// MSVC partial-ordering bug: the unqualified `::h5::write(ds, mem, file, dxpl, ptr)`
			// is ambiguous between the inner overload at line 21 and the variadic forwarder at
			// line 181 (with T=sp_t).  Inline the H5Dwrite call here — matches the H5Dread.hpp:73
			// idiom — so there is no recursive call back into this overload set.
			using element_t = typename impl::decay<T>::type;
			h5::meta::resolved_type_t<element_t> type;
			H5CPP_CHECK_NZ(
				H5Dwrite( static_cast<hid_t>(ds), static_cast<hid_t>(type),
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
					static_cast<hid_t>(dxpl), ptr),
				h5::error::io::dataset::write, h5::error::msg::write_dataset);
		}
		return ds;
	} catch ( const std::exception& err ){
		throw h5::error::io::dataset::write( err.what() );
	}
	
   /** \func_write_hdr
 	*  @brief writes data  within an HDF5 container specified with `h5::fd_t` descriptor 
	* By default the HDF5 dataset size, the file space, is derived from the passed object properties, or may be explicitly specified
	* with optional properties such as h5::count, h5::current_dims h5::max_dims, h5::stride, h5::block <br/>
	* This template specialization acts as a switchboard between objects with <b>contiguous</b> content, such as `std::vector<int>` and
	* objects where the actual content maybe scattered in memory. In the altter case with the help of `h5::gather` operator, and O(n) complexity 
	* this template builds a vector of pointers to actual content, and delegates it to	`h5::write<element_t*>(.., ptr**)` call.
	*
	* \par_ds
	* \par_ref
    * \par_args
	* \returns_ds
	* 
	* \tpar_T
	*
	* <br/>The following arguments are context sensitive, may be passed in arbitrary order and with the exception
	* of `const ref&` object being saved, the arguments are optional. By default the arguments are set to sensible values,
	* and in most cases the function call will deliver good performance. With that in mind, the options below provide an easy high level fine
	* tuning mechanism to get the best experience without calling HDF5 CAPI functions directly. 
    *
	*
	* \par_current_dims
	* When omitted, the system computes the default value as follows  `h5::block{..}` and `h5::stride{..}` given as:
	* 		`current_dims[i] = count[i] (stride[i] - block[i] + 1) + offset[i]` and when only `h5::count` is available 
	*       `current_dims[i] = count[i] + offset[i]`
	* \par_max_dims
	* or `H5S_UNLIMITED` along the dimension intended to be extendable
	* \par_count
	* in hyperslab selection; instead from this value the correct `slab_count` is derived considering the block size whenever applicable with a normalisation step:
    * `slab_count[i] = h5::count[i] / h5::block[i]`
	* \par_stride
	* \par_block
	* \par_offset
	* \par_stride
	* \par_block
	* \par_offset
	* \par_dcpl
	* \par_dxpl
	* \par_dapl
	* \par_lcpl
	* 
	* <br/><b>example:</b>
	* @code
	* std::vector<int> data = ...
	* //creates an extendable, chunked dataset, with initial size matching of the vector
	* h5::fd_t fd = h5::open("example.h5", H5F_ACC_RDWR);
	* h5::ds_t ds = h5::write(fd, "path/chunked layout dataset", data,
	* 		h5::max_dims{H5S_UNLIMITED}, h5::chunk{1024} | h5::gzip{9});
	* //creates a contiguous layout dataset, with size matching `data` 
	* h5::ds_t ds = h5::write(fd, "path/contiguous layout dataset", data);
	* //interop with HDF5 CAPI: will flush dataset `ds` by directly calling libhdf5 function
	* H5Dflush(ds); 
	* @endcode 
 	*/
	// The SFINAE excludes raw pointers (they go through the T* overload) but
	// carves out C-arrays — `T[N]` decays to a pointer, but we still want the
	// by-reference path to handle them (e.g., char[N] fixed-length strings).
	template <class T, class... args_t,
		class = std::enable_if_t<!std::is_pointer_v<std::decay_t<T>> || std::is_array_v<T>>>
	inline h5::ds_t write(const h5::ds_t& ds, const T& ref,  args_t&&... args) try {
		using tcount = typename arg::tpos<const h5::count_t&,const args_t&...>;
		using element_t = typename impl::decay<T>::type;
		using traits = h5::meta::access_traits_t<T>; // we classify type
		using sr_t = h5::meta::storage_representation_t;

		constexpr auto kind = traits::kind;
		constexpr auto storage = h5::meta::storage_representation_v<T>;

		// Stopper: scatter types must use the fd-gateway overload so the compiler-generated
		// h5::scatter<T> specialization can dispatch. Calling h5::write(ds, ref) on a scatter
		// type would silently bypass scatter and fall through to the aggregate path with
		// register_struct<T>() == H5I_UNINIT.
		static_assert(!h5::has_scatter<std::decay_t<T>>::value,
			"h5::write(ds, ref): scatter types must use h5::write(fd, path, ref) so the "
			"compiler-generated h5::scatter<T> specialization can dispatch.");

		// Stopper: unsupported storage usually means an unregistered POD aggregate, a
		// deeply-nested container, or std::vector<bool>. Without this the dispatch can
		// fall through to H5Dwrite with H5I_UNINIT and produce a broken file or crash.
		// Guarded for the h5cpp-compiler bootstrap pass: when generated.h is the empty
		// stub, H5CPP_REGISTER_STRUCT hasn't fired yet, so has_registered_compound<T>
		// is false and the storage falls through to 'unsupported'. The compiler sets
		// -DH5CPP_BUILDING_TYPE_INFO so this check is skipped during its AST scan.
#ifndef H5CPP_BUILDING_TYPE_INFO
		static_assert(storage != sr_t::unsupported,
			"h5::write: storage_representation_v<T> resolved to 'unsupported'. "
			"Check: unregistered POD aggregate (use H5CPP_REGISTER_STRUCT), "
			"std::vector<bool>, or container nesting beyond vector<vector<T>>/vector<string>.");
#endif

		// Stopper (borrowed from #274): containers of containers must route via VLEN
		// storage. Iterator-staging and pointers-gather paths can't faithfully serialize
		// nested containers. element_t comes from impl::decay<T>, which is SFINAE-safe
		// for non-container T (returns T itself, so is_stl_like<element_t>=false).
		static_assert(
			!h5::meta::is_stl_like<element_t>::value ||
			storage == sr_t::ragged_vlen_dataset ||
			storage == sr_t::vlen_text_dataset ||
			storage == sr_t::array_dataset ||
			storage == sr_t::array_element ||
			storage == sr_t::fls_dataset ||
			storage == sr_t::fixed_inner_extent_dataset,
			"h5::write: containers of containers are only supported for vector<string> "
			"(vlen_text_dataset) or vector<vector<T>> (ragged_vlen_dataset).");

		if constexpr (storage == sr_t::array_element) {
			// Top-level T[N] / std::array<T,N> (non-char) — scalar dataspace +
			// H5T_ARRAY[N] dt_t<T> element. The traits::size dims drive the
			// array extent; traits::data points at the contiguous buffer.
			using element_t_loc = typename traits::element_t;
			auto dims = traits::size(ref);
			hsize_t array_dims[H5CPP_MAX_RANK];
			for (std::size_t i = 0; i < dims.size(); ++i) array_dims[i] = dims[i];
			h5::meta::resolved_type_t<element_t_loc> base_type;
			hid_t array_type = H5Tarray_create(static_cast<hid_t>(base_type),
				static_cast<unsigned>(dims.size()), array_dims);
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			h5::sp_t mem_space{H5Screate(H5S_SCALAR)};
			h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
			H5Sselect_all(static_cast<hid_t>(file_space));
			H5CPP_CHECK_NZ(
				H5Dwrite(static_cast<hid_t>(ds), array_type,
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
					static_cast<hid_t>(dxpl), traits::data(ref)),
				h5::error::io::dataset::write, h5::error::msg::write_dataset);
			H5Tclose(array_type);
		} else if constexpr (storage == sr_t::array_dataset) {
			// Outer<std::array<T,N>> (non-char T) — rank-1 dataspace of
			// H5T_ARRAY[N] dt_t<T> elements.  For vector (which exposes
			// .data()) we hand HDF5 the buffer directly; for list/set/etc.
			// we copy elements into a contiguous scratch vector first.
			using inner_t = std::remove_cv_t<typename std::remove_reference_t<T>::value_type>;
			using elem_scalar = typename inner_t::value_type;
			constexpr std::size_t N_inner = std::tuple_size<inner_t>::value;
			hsize_t array_dims[1] = { static_cast<hsize_t>(N_inner) };
			h5::meta::resolved_type_t<elem_scalar> base_type;
			hid_t array_type = H5Tarray_create(static_cast<hid_t>(base_type), 1, array_dims);
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			std::size_t outer_count = std::distance(std::begin(ref), std::end(ref));
			std::vector<inner_t> scratch;
			const void* ptr = nullptr;
			if constexpr (h5::meta::has_data_pointer<std::remove_cv_t<std::remove_reference_t<T>>>::value) {
				ptr = ref.empty() ? nullptr : ref.front().data();
			} else {
				scratch.assign(std::begin(ref), std::end(ref));
				ptr = scratch.empty() ? nullptr : scratch.front().data();
			}
			h5::sp_t mem_space = h5::create_simple(static_cast<hsize_t>(outer_count));
			h5::select_all(mem_space);
			h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
			H5Sselect_all(static_cast<hid_t>(file_space));
			H5CPP_CHECK_NZ(
				H5Dwrite(static_cast<hid_t>(ds), array_type,
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space), static_cast<hid_t>(dxpl), ptr),
				h5::error::io::dataset::write, h5::error::msg::write_dataset);
			H5Tclose(array_type);
		} else if constexpr (storage == sr_t::fls_dataset) {
			// Outer<std::array<char,N>> — rank-1 dataspace of fixed-length-
			// string elements.  Iterator-stages for non-vector outers.
			using inner_t = std::remove_cv_t<typename std::remove_reference_t<T>::value_type>;
			constexpr std::size_t N_inner = std::tuple_size<inner_t>::value;
			hid_t str_type = H5Tcopy(H5T_C_S1);
			H5Tset_size(str_type, N_inner);
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			std::size_t outer_count = std::distance(std::begin(ref), std::end(ref));
			std::vector<inner_t> scratch;
			const void* ptr = nullptr;
			if constexpr (h5::meta::has_data_pointer<std::remove_cv_t<std::remove_reference_t<T>>>::value) {
				ptr = ref.empty() ? nullptr : ref.front().data();
			} else {
				scratch.assign(std::begin(ref), std::end(ref));
				ptr = scratch.empty() ? nullptr : scratch.front().data();
			}
			h5::sp_t mem_space = h5::create_simple(static_cast<hsize_t>(outer_count));
			h5::select_all(mem_space);
			h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
			H5Sselect_all(static_cast<hid_t>(file_space));
			H5CPP_CHECK_NZ(
				H5Dwrite(static_cast<hid_t>(ds), str_type,
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space), static_cast<hid_t>(dxpl), ptr),
				h5::error::io::dataset::write, h5::error::msg::write_dataset);
			H5Tclose(str_type);
		} else if constexpr (kind == h5::meta::access_t::composite) {
			// scalar composite (std::tuple<Ts...>): pack into buffer matching the
			// HDF5 compound type's field offsets (dt_t<tuple> + tuple_layout agree).
			std::vector<char> buf(traits::bytes());
			traits::pack(ref, buf.data());
			h5::meta::resolved_type_t<T> mem_type;
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			h5::sp_t mem_space{H5Screate(H5S_SCALAR)};
			h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
			H5Sselect_all(static_cast<hid_t>(file_space));
			H5CPP_CHECK_NZ(
				H5Dwrite(static_cast<hid_t>(ds), static_cast<hid_t>(mem_type),
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
					static_cast<hid_t>(dxpl), buf.data()),
				h5::error::io::dataset::write, h5::error::msg::write_dataset);
		} else if constexpr (kind == h5::meta::access_t::text) {
			// Scalar text — single string element on a scalar dataspace.
			// Two storage flavours share this branch:
			//   - vlen_text_dataset (std::string, string_view): H5T_VARIABLE
			//     size, write the relay pointer (&relay) so HDF5 stores the
			//     pointed-to bytes.
			//   - fixed_length_string (char[N]): H5Tset_size(N), write the
			//     N raw bytes directly.
			const char* relay = traits::data(ref);
			hid_t str_type = H5Tcopy(H5T_C_S1);
			if constexpr (storage == sr_t::fixed_length_string) {
				H5Tset_size(str_type, traits::fixed_length);
			} else {
				H5Tset_size(str_type, H5T_VARIABLE);
			}
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			h5::sp_t mem_space{H5Screate(H5S_SCALAR)};
			h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
			H5Sselect_all(static_cast<hid_t>(file_space));
			H5CPP_CHECK_NZ(
				H5Dwrite(static_cast<hid_t>(ds), str_type,
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
					static_cast<hid_t>(dxpl),
					(storage == sr_t::fixed_length_string)
						? static_cast<const void*>(relay)
						: static_cast<const void*>(&relay)),
				h5::error::io::dataset::write, h5::error::msg::write_dataset);
			H5Tclose(str_type);
		} else if constexpr (kind == h5::meta::access_t::contiguous || kind == h5::meta::access_t::object) {
			auto ptr = traits::data(ref);
			if constexpr (!tcount::present) {
				h5::count_t count = traits::size( ref );
				::h5::write(ds, ptr, count, args...);
			} else ::h5::write(ds, ptr,  args...);
		} else if constexpr (kind == h5::meta::access_t::pointers) {
			if constexpr (storage == sr_t::vlen_text_dataset) {
				// vector<string> — relay array of char*; HDF5 owns no memory here
				std::vector<const char*> relay;
				relay.reserve(ref.size());
				for (const auto& s : ref) relay.push_back(s.c_str());
				hid_t vlen_str = H5Tcopy(H5T_C_S1);
				H5Tset_size(vlen_str, H5T_VARIABLE);
				const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
				h5::count_t count = traits::size(ref);
				h5::sp_t mem_space = h5::create_simple(static_cast<hsize_t>(count[0]));
				h5::select_all(mem_space);
				h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
				H5Sselect_all(static_cast<hid_t>(file_space));
				H5CPP_CHECK_NZ(
					H5Dwrite(static_cast<hid_t>(ds), vlen_str,
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space), static_cast<hid_t>(dxpl), relay.data()),
					h5::error::io::dataset::write, h5::error::msg::write_dataset);
				H5Tclose(vlen_str);
			} else if constexpr (storage == sr_t::ragged_vlen_dataset) {
				// vector<L> where L is an iterable container — hvl_t relay.
				// For L=std::vector we can point hvl_t.p directly at the inner
				// buffer; for L=list/set/deque/etc. we copy each inner into a
				// flat scratch buffer first.
				using inner_t = typename traits::element_t;
				using elem_t  = typename inner_t::value_type;
				std::vector<hvl_t> relay(ref.size());
				std::vector<std::vector<elem_t>> scratch;
				constexpr bool inner_has_data = h5::meta::has_data_pointer<inner_t>::value;
				if constexpr (!inner_has_data) scratch.resize(ref.size());
				for (std::size_t i = 0; i < ref.size(); ++i) {
					if constexpr (inner_has_data) {
						relay[i].len = ref[i].size();
						relay[i].p   = const_cast<void*>(static_cast<const void*>(ref[i].data()));
					} else {
						scratch[i].assign(ref[i].begin(), ref[i].end());
						relay[i].len = scratch[i].size();
						relay[i].p   = scratch[i].data();
					}
				}
				h5::meta::resolved_type_t<elem_t> base_type;
				hid_t vlen_type = H5Tvlen_create(static_cast<hid_t>(base_type));
				const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
				h5::count_t count = traits::size(ref);
				h5::sp_t mem_space = h5::create_simple(static_cast<hsize_t>(count[0]));
				h5::select_all(mem_space);
				h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
				H5Sselect_all(static_cast<hid_t>(file_space));
				H5CPP_CHECK_NZ(
					H5Dwrite(static_cast<hid_t>(ds), vlen_type,
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space), static_cast<hid_t>(dxpl), relay.data()),
					h5::error::io::dataset::write, h5::error::msg::write_dataset);
				H5Tclose(vlen_type);
			} else if constexpr (h5::meta::access_kind_v<typename traits::element_t> == h5::meta::access_t::composite) {
				// vector<tuple<Ts...>> — pack each element via element traits
				using elem_traits = h5::meta::access_traits_t<typename traits::element_t>;
				std::size_t n = ref.size();
				std::vector<char> buf(n * elem_traits::bytes());
				for (std::size_t i = 0; i < n; ++i)
					elem_traits::pack(ref[i], buf.data() + i * elem_traits::bytes());
				h5::meta::resolved_type_t<typename traits::element_t> mem_type;
				const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
				h5::sp_t mem_space = h5::create_simple(static_cast<hsize_t>(n));
				h5::select_all(mem_space);
				h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
				H5Sselect_all(static_cast<hid_t>(file_space));
				H5CPP_CHECK_NZ(
					H5Dwrite(static_cast<hid_t>(ds), static_cast<hid_t>(mem_type),
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space), static_cast<hid_t>(dxpl), buf.data()),
					h5::error::io::dataset::write, h5::error::msg::write_dataset);
			} else {
				// flat pointer gather: vector<NonTrivialPod> — element has .data() but is flat
				using element_t = typename impl::decay<typename traits::element_t>::type;
				std::vector<element_t> elements;
				const element_t* ptrs = h5::gather(ref, elements);
				if constexpr (!tcount::present) {
					h5::count_t count = traits::size(ref);
					::h5::write<element_t>(ds, ptrs, count, args...);
				} else ::h5::write<element_t>(ds, ptrs, args...);
			}
		} else if constexpr (kind == h5::meta::access_t::iterators) {
			if constexpr (storage == sr_t::key_value_dataset) {
				// map<K,V> and variants — compound HDF5 type with "key" and "value" fields
				using element_t = typename traits::element_t;
				using key_t     = std::remove_const_t<typename element_t::first_type>;
				using value_t   = typename element_t::second_type;
				struct kv_t { key_t key; value_t value; };
				h5::meta::resolved_type_t<key_t>   kt;
				h5::meta::resolved_type_t<value_t> vt;
				hid_t compound = H5Tcreate(H5T_COMPOUND, sizeof(kv_t));
				H5Tinsert(compound, "key",   offsetof(kv_t, key),   static_cast<hid_t>(kt));
				H5Tinsert(compound, "value", offsetof(kv_t, value), static_cast<hid_t>(vt));
				std::vector<kv_t> buffer;
				buffer.reserve(ref.size());
				for (const auto& [k, v] : ref)
					buffer.push_back({static_cast<key_t>(k), v});
				const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
				h5::count_t count = traits::size(ref);
				h5::sp_t mem_space = h5::create_simple(static_cast<hsize_t>(count[0]));
				h5::select_all(mem_space);
				h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
				H5Sselect_all(static_cast<hid_t>(file_space));
				H5CPP_CHECK_NZ(
					H5Dwrite(static_cast<hid_t>(ds), compound,
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space), static_cast<hid_t>(dxpl), buffer.data()),
					h5::error::io::dataset::write, h5::error::msg::write_dataset);
				H5Tclose(compound);
			} else if constexpr (h5::meta::access_kind_v<typename traits::element_t> == h5::meta::access_t::composite) {
				// list<tuple>, set<tuple>, deque<tuple>: pack via element traits.
				// Iterator traversal (no operator[]); count via traits::size.
				using elem_traits = h5::meta::access_traits_t<typename traits::element_t>;
				std::size_t n = traits::size(ref)[0];
				std::vector<char> buf(n * elem_traits::bytes());
				std::size_t i = 0;
				for (const auto& elem : ref) {
					elem_traits::pack(elem, buf.data() + i * elem_traits::bytes());
					++i;
				}
				h5::meta::resolved_type_t<typename traits::element_t> mem_type;
				const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
				h5::sp_t mem_space = h5::create_simple(static_cast<hsize_t>(n));
				h5::select_all(mem_space);
				h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
				H5Sselect_all(static_cast<hid_t>(file_space));
				H5CPP_CHECK_NZ(
					H5Dwrite(static_cast<hid_t>(ds), static_cast<hid_t>(mem_type),
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space), static_cast<hid_t>(dxpl), buf.data()),
					h5::error::io::dataset::write, h5::error::msg::write_dataset);
			} else {
				// staging buffer: list<T>, set<T>, deque<T> — linear sequences
				using element_t = typename impl::decay<typename traits::element_t>::type;
				// Guard (review item A2): the staging path memcpy's native element_t
				// layout to disk via resolved_type_t<element_t>. For non-std-layout
				// element_t the on-disk layout would silently diverge from the HDF5
				// compound type. Composite element types route through the branch
				// above; this catches any future escape (user-defined non-std-layout).
				static_assert(std::is_standard_layout_v<element_t>,
					"h5::write: iterator-staging path requires standard-layout element_t. "
					"Use composite-kind types (e.g. wrap in std::tuple) or convert to a "
					"flat representation before writing.");
				auto count = traits::size(ref);
				size_t n = 1;
				for (std::size_t i = 0; i < count.size(); ++i) n *= count[i];
				std::vector<element_t> buffer;
				buffer.reserve(n);
				for (const auto& elem : ref)
					buffer.push_back(elem);
				::h5::write(ds, buffer.data(), h5::count_t(count), args...);
			}
		} else static_assert(kind != h5::meta::access_t::unsupported, "unsupported type for h5::write");
		
		return ds;
	} catch ( const std::exception& err ){
		throw h5::error::io::dataset::write( err.what() );
	}

    /** \func_write_hdr
 	*  @brief writes data within an HDF5 container specified with `h5::fd_t` descriptor 
	* HDF5 dataset may or may not exist, in first case it is opened and in the latter created. The implemantation comes with sensible 
	* default arguments, which can be tuned with optional property list.
	* By default the HDF5 dataset size, the file space, is derived from the passed object properties, or may be explicitly specified
	* with optional properties such as h5::count, h5::current_dims h5::max_dims, h5::stride, h5::block 
	* \par_fd
	* \par_dataset_path
	* \par_ref
    * \par_args
	* \returns_ds
	* 
	* \tpar_T
	*
	* <br/>The following arguments are context sensitive, may be passed in arbitrary order and with the exception
	* of `T object` being saved, the arguments are optional. The arguments are set to sensible values, and in most cases
	* will provide good performance by default, with that in mind, it is an easy high level fine tuning mechanism to 
	* get the best experience witout trading readability. 
	*
	* \par_current_dims
	* When omitted, the system computes the default value as follows  `h5::block{..}` and `h5::stride{..}` given as:
	* 		`current_dims[i] = count[i] (stride[i] - block[i] + 1) + offset[i]` and when only `h5::count` is available 
	*       `current_dims[i] = count[i] + offset[i]`
	* \par_max_dims
	* or `H5S_UNLIMITED` along the dimension intended to be extendable
	*
	* \par_count
	* in hyperslab selection; instead from this value the correct `slab_count` is derived considering the block size whenever applicable with a normalisation step:
    * `slab_count[i] = h5::count[i] / h5::block[i]`
	* \par_stride
	* \par_block
	* \par_offset
	* \par_dcpl
	* \par_dxpl
	* \par_dapl
	* \par_lcpl
	* 
	* <br/><b>example:</b>
	* @code
	* std::vector<int> vec(10'000);
	* h5::fd_t fd = h5::open("example.h5", H5F_ACC_RDWR);
	* // dataset will be created with the dimensions specified
	* h5::ds_t ds = h5::write(fd, "path/dataset from direct memory", object.data(),
	* 	h5::current_dims{vec.length()}, h5::max_dims{H5S_UNLIMITED}, h5::chunk{1024} | h5::gzip{9});
	* @endcode 
 	*/ 
	// char[N] gets a dedicated overload because the generic const T* template
	// wins partial ordering for array arguments via array-to-pointer decay,
	// but for fixed-length-string semantics we want a fixed-length scalar
	// dataset. Inline the create+write here so we don't recurse back into
	// this overload via T*; the by-ref path can't be reached by template
	// argument deduction because T=char[N] vs T=char(*) is ambiguous in the
	// generic gateway.
	template <std::size_t N, class... args_t>
	inline h5::ds_t write( const h5::fd_t& fd, const std::string& dataset_path, const char (&ref)[N], args_t&&... args ){
	  return h5::impl::on_collector([&]() -> h5::ds_t {   // MT: run the whole gateway under the process-global HDF5 lock
		h5::ds_t ds;
		h5::mute();
			bool is_dataset_present = H5Lexists(static_cast<hid_t>(fd), dataset_path.c_str(), H5P_DEFAULT) > 0;
		h5::unmute();
		if (is_dataset_present) {
			const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
			ds = h5::open(fd, dataset_path, dapl);
		} else {
			const h5::lcpl_t& lcpl = arg::get(h5::default_lcpl, args...);
			h5::dcpl_t default_dcpl{H5Pcreate(H5P_DATASET_CREATE)};
			const h5::dcpl_t& dcpl = arg::get(default_dcpl, args...);
			const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
			hid_t str_type = H5Tcopy(H5T_C_S1);
			H5Tset_size(str_type, N);
			h5::sp_t space{H5Screate(H5S_SCALAR)};
			ds = h5::createds(fd, dataset_path, str_type, space, lcpl, dcpl, dapl);
			H5Tclose(str_type);
		}
		// Now write into the dataset. Goes through the kind=text/fixed_length
		// branch of the ds-write dispatch.
		::h5::write<char[N]>(ds, ref, std::forward<args_t>(args)...);
		return ds;
	  });
	}

	template <class T, class... args_t>
	inline h5::ds_t write( const h5::fd_t& fd, const std::string& dataset_path, const T* ptr,  args_t&&... args  ){
	  return h5::impl::on_collector([&]() -> h5::ds_t {   // MT: run the whole gateway under the process-global HDF5 lock
		using tcount  = typename arg::tpos<const h5::count_t&, const args_t&...>;
		static_assert( tcount::present, "h5::count_t{ ... } must be provided to describe T* memory region" );
		h5::ds_t ds; // initialized to H5I_UNINIT
		
		h5::mute(); // find out if we have to create the dataset 
			bool is_dataset_present = H5Lexists(static_cast<hid_t>(fd), dataset_path.c_str(), H5P_DEFAULT) > 0;
		h5::unmute(); // <- make sure not to mute error handling longer than needed
		if (is_dataset_present) {
			const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
			ds = h5::open(fd, dataset_path, dapl);
		} else {
			// dataset doesn't exist, or some error happened, since h5::create doesn't know of the 
			// memory space size as `T& ref` never passed along we have to compute the `h5::current_dims_t{}` upfront
			using tcurrent_dims = typename arg::tpos<const h5::current_dims_t&, const args_t&...>;
			if constexpr (tcurrent_dims::present) // user knows what he is doing, specified h5::current_dims{} explicitly
				ds = h5::create<T>(fd, dataset_path, args...);
			else {
				h5::current_dims_t current_dims = static_cast<h5::current_dims_t>(std::get<tcount::value>(std::forward_as_tuple(args...)));  // count must be present, we checked
				ds = h5::create<T>(fd, dataset_path, current_dims, args...);          // and use it to create dataset
			}
		}
		// we either have `ds` != H5I_UNINIT or an exception thrown, safe to delegate
		return ::h5::write(ds, ptr, args...);
	  });
	}

    /** \func_write_hdr
 	*  @brief writes data within an HDF5 container specified with `h5::fd_t` descriptor 
	* HDF5 dataset may or may not exist, in first case it is opened and in the latter created. The implemantation comes with sensible 
	* default arguments, which can be tuned with optional property list.
	* By default the HDF5 dataset size, the file space, is derived from the passed object properties, or may be explicitly specified
	* with optional properties such as h5::count, h5::current_dims h5::max_dims, h5::stride, h5::block 
	* \par_fd
	* \par_dataset_path
	* \par_ref
    * \par_args
	* \returns_ds
	* 
	* \tpar_T
	*
	* <br/>The following arguments are context sensitive, may be passed in arbitrary order and with the exception
	* of `T object` being saved, the arguments are optional. The arguments are set to sensible values, and in most cases
	* will provide good performance by default, with that in mind, it is an easy high level fine tuning mechanism to 
	* get the best experience witout trading readability. 
	*
	* \par_current_dims
	* When omitted, the system computes the default value as follows  `h5::block{..}` and `h5::stride{..}` given as:
	* 		`current_dims[i] = count[i] (stride[i] - block[i] + 1) + offset[i]` and when only `h5::count` is available 
	*       `current_dims[i] = count[i] + offset[i]`
	* \par_max_dims
	* or `H5S_UNLIMITED` along the dimension intended to be extendable
	*
	* \par_count
	* in hyperslab selection; instead from this value the correct `slab_count` is derived considering the block size whenever applicable with a normalisation step:
    * `slab_count[i] = h5::count[i] / h5::block[i]`
	* \par_stride
	* \par_block
	* \par_offset
	* \par_dcpl
	* \par_dxpl
	* \par_dapl
	* \par_lcpl
	* 
	* <br/><b>example:</b>
	* @code
	* std::vector<int> vec(10'000);
	* h5::fd_t fd = h5::open("example.h5", H5F_ACC_RDWR);
	* // dataset will be created with the dimensions specified
	* h5::ds_t ds = h5::write(fd, "path/dataset from direct memory", object.data(),
	* 	h5::current_dims{vec.length()}, h5::max_dims{H5S_UNLIMITED}, h5::chunk{1024} | h5::gzip{9});
	* @endcode 
 	*/ 
		template <class T, class... args_t,
			class = std::enable_if_t<!std::is_pointer_v<std::decay_t<T>>
			                      && !h5::meta::is_sparse_v<std::decay_t<T>>>>
		inline h5::ds_t write( const h5::fd_t& fd, const std::string& dataset_path, const T& ref,  args_t&&... args  ){
		  return h5::impl::on_collector([&]() -> h5::ds_t {   // MT: run the whole gateway under the process-global HDF5 lock
			if constexpr (h5::has_scatter<std::decay_t<T>>::value) {
				// Scatter path: compiler-generated scatter<T> handles open/create + row append.
				// Call-site properties (chunk, compress, etc.) are ignored here; the generated
				// specialization embeds them or the dataset was pre-created.
				return h5::scatter<std::decay_t<T>>(static_cast<hid_t>(fd), dataset_path, ref);
			} else {
				using traits   = h5::meta::access_traits_t<T>;
				using sr_t     = h5::meta::storage_representation_t;
				constexpr auto storage = h5::meta::storage_representation_v<T>;

				// Stopper: mirror the ds-dispatch overload guard so an unregistered or
				// deeply-nested type fails at compile time on the gateway path too.
				// (Scatter types are handled by the if-branch above, before this assert.)
#ifndef H5CPP_BUILDING_TYPE_INFO
				static_assert(storage != sr_t::unsupported,
					"h5::write: storage_representation_v<T> resolved to 'unsupported'. "
					"Check: unregistered POD aggregate (use H5CPP_REGISTER_STRUCT), "
					"std::vector<bool>, or container nesting beyond vector<vector<T>>/vector<string>.");
#endif

				h5::ds_t ds;
				h5::mute();
					bool is_dataset_present = H5Lexists(static_cast<hid_t>(fd), dataset_path.c_str(), H5P_DEFAULT) > 0;
				h5::unmute();

				if (is_dataset_present) {
					const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
					ds = h5::open(fd, dataset_path, dapl);
				} else {
					using tcurrent_dims = typename arg::tpos<const h5::current_dims_t&, const args_t&...>;
					const h5::lcpl_t& lcpl = arg::get(h5::default_lcpl, args...);
					h5::dcpl_t default_dcpl{H5Pcreate(H5P_DATASET_CREATE)};
					const h5::dcpl_t& dcpl = arg::get(default_dcpl, args...);
					const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);

					if constexpr (storage == sr_t::array_element) {
						// Scalar dataspace + H5T_ARRAY[dims...] dt_t<T>.
						using element_t_loc = typename traits::element_t;
						auto dims = traits::size(ref);
						hsize_t array_dims[H5CPP_MAX_RANK];
						for (std::size_t i = 0; i < dims.size(); ++i) array_dims[i] = dims[i];
						h5::meta::resolved_type_t<element_t_loc> base_type;
						hid_t array_type = H5Tarray_create(static_cast<hid_t>(base_type),
							static_cast<unsigned>(dims.size()), array_dims);
						h5::sp_t space{H5Screate(H5S_SCALAR)};
						ds = h5::createds(fd, dataset_path, array_type, space, lcpl, dcpl, dapl);
						H5Tclose(array_type);
					} else if constexpr (storage == sr_t::array_dataset) {
						// Rank-1 dataspace of H5T_ARRAY[N] dt_t<T> elements.
						using inner_t = typename traits::element_t;
						using elem_scalar = typename inner_t::value_type;
						constexpr std::size_t N_inner = std::tuple_size<inner_t>::value;
						hsize_t array_dims[1] = { static_cast<hsize_t>(N_inner) };
						h5::meta::resolved_type_t<elem_scalar> base_type;
						hid_t array_type = H5Tarray_create(static_cast<hid_t>(base_type), 1, array_dims);
						h5::current_dims_t current_dims;
						current_dims.rank = 1;
						current_dims[0] = static_cast<hsize_t>(std::distance(std::begin(ref), std::end(ref)));
						h5::sp_t space = h5::create_simple(current_dims);
						ds = h5::createds(fd, dataset_path, array_type, space, lcpl, dcpl, dapl);
						H5Tclose(array_type);
					} else if constexpr (storage == sr_t::fls_dataset) {
						// Rank-1 dataspace of H5T_C_S1+set_size(N) elements.
						using inner_t = typename traits::element_t;
						constexpr std::size_t N_inner = std::tuple_size<inner_t>::value;
						hid_t str_type = H5Tcopy(H5T_C_S1);
						H5Tset_size(str_type, N_inner);
						h5::current_dims_t current_dims;
						current_dims.rank = 1;
						current_dims[0] = static_cast<hsize_t>(std::distance(std::begin(ref), std::end(ref)));
						h5::sp_t space = h5::create_simple(current_dims);
						ds = h5::createds(fd, dataset_path, str_type, space, lcpl, dcpl, dapl);
						H5Tclose(str_type);
					} else if constexpr (traits::kind == h5::meta::access_t::text) {
						// Scalar text — single string element on a scalar
						// dataspace. Variable-length (std::string / view) or
						// fixed-length (char[N] / std::array<char,N>) depending on storage.
						hid_t str_type = H5Tcopy(H5T_C_S1);
						if constexpr (storage == sr_t::fixed_length_string) {
							H5Tset_size(str_type, traits::fixed_length);
						} else {
							H5Tset_size(str_type, H5T_VARIABLE);
						}
						h5::sp_t space{H5Screate(H5S_SCALAR)};
						ds = h5::createds(fd, dataset_path, str_type, space, lcpl, dcpl, dapl);
						H5Tclose(str_type);
					} else if constexpr (traits::kind == h5::meta::access_t::object && storage == sr_t::scalar) {
						// Scalar object kind (std::pair, std::complex): single
						// H5T_COMPOUND element on a scalar dataspace via dt_t<T>.
						h5::meta::resolved_type_t<T> mem_type;
						h5::sp_t space{H5Screate(H5S_SCALAR)};
						ds = h5::createds(fd, dataset_path, static_cast<hid_t>(mem_type), space, lcpl, dcpl, dapl);
					} else if constexpr (traits::kind == h5::meta::access_t::composite) {
						// scalar composite (tuple): scalar compound dataset
						h5::meta::resolved_type_t<T> mem_type;
						h5::sp_t space{H5Screate(H5S_SCALAR)};
						ds = h5::createds(fd, dataset_path, static_cast<hid_t>(mem_type), space, lcpl, dcpl, dapl);
					} else if constexpr (h5::meta::access_kind_v<typename traits::element_t> == h5::meta::access_t::composite) {
						// vector<tuple>/list<tuple>/etc.: rank-1 compound dataset
						using elem_t = typename traits::element_t;
						h5::meta::resolved_type_t<elem_t> mem_type;
						h5::count_t count = traits::size(ref);
						h5::current_dims_t current_dims = h5::impl::get_current_dims(count, args...);
						h5::sp_t space = h5::create_simple(current_dims);
						ds = h5::createds(fd, dataset_path, static_cast<hid_t>(mem_type), space, lcpl, dcpl, dapl);
					} else if constexpr (storage == sr_t::vlen_text_dataset) {
						// vector<string>: 1D dataset of variable-length strings
						hid_t vlen_str = H5Tcopy(H5T_C_S1);
						H5Tset_size(vlen_str, H5T_VARIABLE);
						h5::count_t count = traits::size(ref);
						h5::current_dims_t current_dims = h5::impl::get_current_dims(count, args...);
						h5::sp_t space = h5::create_simple(current_dims);
						ds = h5::createds(fd, dataset_path, vlen_str, space, lcpl, dcpl, dapl);
						H5Tclose(vlen_str);
					} else if constexpr (storage == sr_t::ragged_vlen_dataset) {
						// vector<vector<T>>: 1D dataset of HDF5 VLEN elements
						using inner_t = typename traits::element_t;
						using elem_t  = typename inner_t::value_type;
						h5::meta::resolved_type_t<elem_t> base_type;
						hid_t vlen_type = H5Tvlen_create(static_cast<hid_t>(base_type));
						h5::count_t count = traits::size(ref);
						h5::current_dims_t current_dims = h5::impl::get_current_dims(count, args...);
						h5::sp_t space = h5::create_simple(current_dims);
						ds = h5::createds(fd, dataset_path, vlen_type, space, lcpl, dcpl, dapl);
						H5Tclose(vlen_type);
					} else if constexpr (storage == sr_t::key_value_dataset) {
						// map<K,V> and variants: compound HDF5 type with "key" and "value" fields
						using element_t = typename traits::element_t;
						using key_t   = std::remove_const_t<typename element_t::first_type>;
						using value_t = typename element_t::second_type;
						struct kv_t { key_t key; value_t value; };
						h5::meta::resolved_type_t<key_t> kt;
						h5::meta::resolved_type_t<value_t> vt;
						hid_t compound = H5Tcreate(H5T_COMPOUND, sizeof(kv_t));
						H5Tinsert(compound, "key",   offsetof(kv_t, key),   static_cast<hid_t>(kt));
						H5Tinsert(compound, "value", offsetof(kv_t, value), static_cast<hid_t>(vt));
						h5::count_t count = traits::size(ref);
						h5::current_dims_t current_dims = h5::impl::get_current_dims(count, args...);
						h5::sp_t space = h5::create_simple(current_dims);
						ds = h5::createds(fd, dataset_path, compound, space, lcpl, dcpl, dapl);
						H5Tclose(compound);
					} else if constexpr (tcurrent_dims::present) {
						using element_t = typename h5::impl::decay<T>::type;
						ds = h5::create<element_t>(fd, dataset_path, args...);
					} else {
						using element_t = typename h5::impl::decay<T>::type;
						h5::count_t count = traits::size(ref);
						h5::current_dims_t current_dims = h5::impl::get_current_dims(count, args...);
						ds = h5::create<element_t>(fd, dataset_path, current_dims, args...);
					}
				}
				return ::h5::write(ds, ref, args...);
			}
		  });
		}


   /** \func_write_hdr
 	*  @brief writes content of an object, collection of object or memory location to a possibly not yet existing dataset 
	* within an HDF5 container opened with flag `H5F_ACC_RDWR`
	* By default the HDF5 dataset size, the file space, is derived from the passed object properties, or may be explicitly specified
	* with optional properties such as h5::count, h5::current_dims h5::max_dims, h5::stride, h5::block 
	* \par_file_path
	* \par_dataset_path
    * \par_args
	* \returns_ds
	* 
	* \tpar_T
	*
	* <br/>The following arguments are context sensitive, may be passed in arbitrary order and with the exception
	* of `const ref&` or `const T*` object being saved/memory region pointed to, the arguments are optional. By default the arguments are set to sensible values,
	* and in most cases the function call will deliver good performance. With that in mind, the options below provide an easy to use high level fine
	* tuning mechanism to get the best experience without calling HDF5 CAPI functions directly. 
    *
	* \par_current_dims
	* When omitted, the system computes the default value as follows  `h5::block{..}` and `h5::stride{..}` given as:
	* 		`current_dims[i] = count[i] (stride[i] - block[i] + 1) + offset[i]` and when only `h5::count` is available 
	*       `current_dims[i] = count[i] + offset[i]`
	* \par_max_dims
	* or `H5S_UNLIMITED` along the dimension intended to be extendable
	* \par_count
	* in hyperslab selection; instead from this value the correct `slab_count` is derived considering the block size whenever applicable with a normalisation step:
    * `slab_count[i] = h5::count[i] / h5::block[i]`
	* \par_stride
	* \par_block
	* \par_offset
	* \par_stride
	* \par_block
	* \par_offset
	* \par_dcpl
	* \par_dxpl
	* \par_dapl
	* \par_lcpl
	* <br/><b>example:</b>
	* @code
	* std::vector<int> data = ...
	* //creates an extendable, chunked dataset, with initial size matching of the vector
	* h5::ds_t ds = h5::write("example.h5", "path/chunked layout dataset", data,
	* 		h5::max_dims{H5S_UNLIMITED}, h5::chunk{1024} | h5::gzip{9});
	* //creates a contiguous layout dataset, with size matching `data` 
	* h5::ds_t ds = h5::write("example.h5", "path/contiguous layout dataset", data);
	* //interop with HDF5 CAPI: will flush dataset `ds` by directly calling libhdf5 function
	* H5Dflush(ds); 
	* @endcode 
 	*/ 
	template <class... args_t>
	inline h5::ds_t write( const std::string& file_path, const std::string& dataset_path, args_t&&... args  ){
		//TODO: refine delegation
		h5::fd_t fd = h5::open( file_path, H5F_ACC_RDWR, h5::default_fapl );
		return ::h5::write( fd, dataset_path, args...);
	}

	// (The separate async-fd write overloads are retired: under H5CPP_MULTITHREAD
	// the single write(fd_t) gateways below are wrapped in h5::impl::on_collector,
	// so the whole create+write+close runs under the process-global HDF5 lock —
	// concurrency-safe for distinct datasets, with compression still fanning out to
	// the worker pool.  In a classic build on_collector is a no-op pass-through.)
}
