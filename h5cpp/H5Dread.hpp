/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#pragma once
#include "H5capi.hpp"
#include "H5misc.hpp"
#include "H5Tsparse.hpp"
#include "H5Dopen.hpp" // be sure this precedes error handling macro-s !!!
#include "H5Rreference.hpp"
#include "H5Dscatter.hpp"
#include <string>
#include <stdexcept>
#include <type_traits>
#include <tuple>

namespace h5 {
 	/**
 	 * \func_read_hdr
 	 * @brief Read elements from an open HDF5 dataset into caller-allocated memory.
 	 *
 	 * Low-level raw-pointer overload — caller owns the memory and must
 	 * supply `h5::count{...}` so the dispatch knows how many elements
 	 * to materialise. Optional `h5::offset` / `h5::stride` / `h5::block`
 	 * select a hyperslab; without them the read covers the whole extent
 	 * (provided `count` matches it). For container / value targets use
 	 * the `h5::read(ds, T& ref, ...)` overload below, which derives the
 	 * element count from the destination object.
 	 *
 	 * \par_ds
 	 * \par_ptr
 	 * \par_args
 	 * \tpar_T
 	 * \returns_err
 	 *
 	 * @throws h5::error::io::dataset::read   on `H5Dread` failure
 	 *         (type-conversion error, rank mismatch, invalid hyperslab).
 	 *
 	 * <br/><b>example:</b>
 	 * @code
 	 * h5::ds_t ds = h5::open(fd, "/grid/data");           // 10x10 float dataset
 	 * std::vector<float> buf(10*10);
 	 * h5::read(ds, buf.data(), h5::count{10,10});         // whole extent
 	 * h5::read(ds, buf.data(), h5::count{4,4}, h5::offset{5,0});  // hyperslab
 	 * @endcode
 	 *
 	 * \sa_h5cpp
 	 * \sa_hdf5
 	 * @sa h5::open h5::write h5::create @ref link_base_template_types
 	 *     "Supported Types"
 	 */
	template<class T, class... args_t>
	inline std::enable_if_t<!std::is_same_v<T,char**>,
	void> read( const h5::ds_t& ds, T* ptr, args_t&&... args ) try {
		using tcount   = typename arg::tpos<const h5::count_t&,const args_t&...>;
		static_assert( tcount::present, "h5::count_t{ ... } must be specified" );
		static_assert( utils::is_supported<T>, "error: " H5CPP_supported_elementary_types );

		auto tuple = std::forward_as_tuple(args...);
		const h5::count_t& count = std::get<tcount::value>( tuple );

		h5::offset_t  default_offset{0,0,0,0,0,0,0}; // must match H5CPP_MAX_RANK=7
		const h5::offset_t& offset = arg::get( default_offset, args...);

		h5::stride_t  default_stride{1,1,1,1,1,1,1};
		const h5::stride_t& stride = arg::get( default_stride, args...);

		h5::block_t  default_block{1,1,1,1,1,1,1};
		const h5::block_t& block = arg::get( default_block, args...);

		h5::count_t size; // compute actual memory space
		for(int i=0;i<count.rank;i++) size[i] = count[i] * block[i];
		size.rank = count.rank;

		const h5::dxpl_t& dxpl = arg::get( h5::default_dxpl, args...);
		H5CPP_CHECK_PROP( dxpl, h5::error::property_list::misc, "invalid data transfer property" );

		h5::sp_t file_space = h5::get_space(ds);
	   	int rank = h5::get_simple_extent_ndims( file_space );

		if( rank != count.rank ) throw h5::error::io::dataset::read( H5CPP_ERROR_MSG( h5::error::msg::rank_mismatch ));
		using element_t = typename impl::decay<T>::type;
		h5::meta::resolved_type_t<element_t> mem_type;
		hid_t dapl = h5::get_access_plist( ds );
		// See H5Dwrite.hpp for the rationale: pipeline path uses H5Dread_chunk,
		// which only works on chunked datasets. Guard on H5D_CHUNKED so DAPLs
		// with the flag applied to contiguous datasets fall through to H5Dread.
		const bool use_pipeline = [&]() {
			if (!H5Pexist(dapl, H5CPP_DAPL_HIGH_THROUGHPUT)) return false;
			hid_t dcpl_id = H5Dget_create_plist(static_cast<hid_t>(ds));
			if (dcpl_id < 0) return false;
			H5D_layout_t layout = H5Pget_layout(dcpl_id);
			H5Pclose(dcpl_id);
			return layout == H5D_CHUNKED;
		}();
		if( use_pipeline ){
			// Phase 1.3.3 — if the file's FAPL has h5::threads{N}, route
			// reads through a local pool_pipeline_t.  Currently pool_pipeline_t::
			// read_chunk_impl is synchronous (parallel decompress is Phase 1.5+),
			// so the FAPL-pool branch is semantically equivalent to the DAPL
			// path today; the structure is in place for the read-ahead
			// optimization to land later without changing call sites.
			hid_t fid  = H5Iget_file_id(static_cast<hid_t>(ds));
			hid_t fapl = H5Fget_access_plist(fid);
			auto pool  = h5::impl::resolve_worker_pool(fapl);
			if (pool) {
				const unsigned cap = h5::impl::resolve_backpressure(
					fapl, pool->worker_count());
				h5::impl::pool_pipeline_t pipe(std::move(pool), cap);
				h5::dcpl_t dcpl{H5Dget_create_plist(static_cast<hid_t>(ds))};
				hid_t type_id  = H5Dget_type(static_cast<hid_t>(ds));
				size_t elem_sz = H5Tget_size(type_id);
				H5Tclose(type_id);
				pipe.set_cache(dcpl, elem_sz);
				pipe.read(ds, offset, stride, block, count, dxpl, ptr);
			} else {
				h5::impl::pipeline_t<impl::basic_pipeline_t>* filters;
				H5Pget(dapl, H5CPP_DAPL_HIGH_THROUGHPUT, &filters);
				filters->read(ds, offset, stride, block, count, dxpl, ptr);
			}
			H5Pclose(fapl);
			H5Fclose(fid);
		}else{
			// Scalar dataspaces don't support hyperslab selection; H5Sselect_all
			// on both sides is the equivalent path for rank=0 / H5S_SCALAR.
			// Detect by file_space class rather than count.rank so we behave
			// correctly when the caller passed an empty count.
			H5S_class_t file_cls = H5Sget_simple_extent_type(static_cast<hid_t>(file_space));
			h5::sp_t mem_space = (file_cls == H5S_SCALAR)
				? h5::sp_t{H5Screate(H5S_SCALAR)}
				: h5::create_simple( size );
			h5::select_all( mem_space );
			if (file_cls == H5S_SCALAR) {
				H5Sselect_all(static_cast<hid_t>(file_space));
			} else {
				h5::select_hyperslab( file_space, offset, stride, count, block);
			}

			H5CPP_CHECK_NZ( H5Dread(
					static_cast<hid_t>( ds ), static_cast<hid_t>(mem_type), static_cast<hid_t>(mem_space),
					static_cast<hid_t>(file_space),	static_cast<hid_t>(dxpl), ptr ), h5::error::io::dataset::read, h5::error::msg::read_dataset);
		}
	} catch ( const std::runtime_error& err ){
		throw h5::error::io::dataset::read( err.what() );
	}

 	/**
 	 * \func_read_hdr
 	 * @brief Open a dataset by path and read elements into caller-allocated memory.
 	 *
 	 * Convenience overload — opens the dataset internally then forwards
 	 * to `h5::read(ds, ptr, ...)`. The dataset handle is closed via RAII
 	 * before this function returns. Requires an explicit `h5::count{...}`
 	 * (SFINAE-gated): a bare `read(fd, path, buf)` without `count` does
 	 * NOT match here and is dispatched to the by-reference overload
 	 * instead — that path lets `char[N]` reach the fixed-length-string
 	 * branch.
 	 *
 	 * \par_fd
 	 * \par_dataset_path
 	 * \par_ptr
 	 * \par_args
 	 * \tpar_T
 	 * \returns_err
 	 *
 	 * @throws h5::error::io::dataset::open   if the dataset is not present.
 	 * @throws h5::error::io::dataset::read   on `H5Dread` failure.
 	 *
 	 * <br/><b>example:</b>
 	 * @code
 	 * h5::fd_t fd = h5::open("myfile.h5", H5F_ACC_RDWR);
 	 * std::vector<float> buf(10*10);
 	 * h5::read(fd, "/path/to/dataset", buf.data(),
 	 *          h5::count{10,10}, h5::offset{5,0});
 	 * @endcode
 	 *
 	 * \sa_h5cpp
 	 * \sa_hdf5
 	 * @sa h5::open h5::write h5::create
 	 */
	// SFINAE-gate the pointer overload on h5::count_t presence so a bare
	// `read(fd, path, buf)` with no count is NOT matched here (where it would
	// hit a hard static_assert) but instead routes to the by-reference path,
	// letting char[N] reach the fixed-length-string dispatch.
	template<class T, class... args_t,
		class = std::enable_if_t<arg::tpos<const h5::count_t&, const args_t&...>::present>>
	inline void read( const h5::fd_t& fd, const std::string& dataset_path, T* ptr, args_t&&... args ){
		const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
		h5::ds_t ds = h5::open(fd, dataset_path, dapl ); // will throw its exception
		::h5::read<T>(ds, ptr, args...);
	}


 	/**
 	 * \func_read_hdr
 	 * @brief Open a file and dataset by path then read into caller-allocated memory.
 	 *
 	 * Convenience overload — opens the file in `H5F_ACC_RDWR` mode and
 	 * forwards to `h5::read(fd, dataset_path, ptr, ...)`. Both the file
 	 * and dataset handles close via RAII before this function returns.
 	 *
 	 * \par_file_path
 	 * \par_dataset_path
 	 * \par_ptr
 	 * \par_args
 	 * \tpar_T
 	 * \returns_err
 	 *
 	 * @throws h5::error::io::file::open      if the file cannot be opened.
 	 * @throws h5::error::io::dataset::open   if the dataset is not present.
 	 * @throws h5::error::io::dataset::read   on `H5Dread` failure.
 	 *
 	 * \sa_h5cpp
 	 * \sa_hdf5
 	 * @sa h5::open h5::write
 	 */
	template<class T, class... args_t>
	inline void read( const std::string& file_path, const std::string& dataset_path,T* ptr, args_t&&... args ){
		h5::fd_t fd = h5::open( file_path, H5F_ACC_RDWR );
		::h5::read( fd, dataset_path, ptr, args...);
	}


	/***************************  REFERENCE *****************************/
 	/**
 	 * \func_read_hdr
 	 * @brief Read into a caller-allocated container or value of type `T`.
 	 *
 	 * Primary by-reference overload — `T` is anything the dispatch
 	 * accepts (see @ref link_base_template_types "Supported Types"):
 	 * elementary scalar, registered compound POD, fixed or
 	 * variable-length string, STL container, linear-algebra container,
 	 * `std::tuple` / `std::pair` / `std::complex`. Element count is
 	 * derived from `ref` via `access_traits_t<T>::size`; passing
 	 * `h5::count{...}` here is a compile-time error.
 	 *
 	 * Optional `h5::offset` / `h5::stride` / `h5::block` arguments select
 	 * a hyperslab from the file space; omitting them reads the whole
 	 * extent. The on-disk type must be compatible with `T` — HDF5 has
 	 * no fixed-length / VLEN string conversion, so a fixed-length
 	 * string dataset reads into `char[N]` / `std::array<char,N>` but
 	 * not into `std::string` (and vice versa).
 	 *
 	 * \par_ds
 	 * \par_ref
 	 * \par_args
 	 * \tpar_T
 	 * \returns_err
 	 *
 	 * @throws h5::error::io::dataset::read   on `H5Dread` failure
 	 *         (type-conversion error, rank mismatch, etc.).
 	 *
 	 * <br/><b>example:</b>
 	 * @code
 	 * h5::ds_t ds = h5::open(fd, "/grid/data");
 	 *
 	 * std::vector<float> v(100);
 	 * h5::read(ds, v);                                   // whole extent
 	 *
 	 * arma::Mat<double> mat(10, 10);
 	 * h5::read(ds, mat, h5::offset{5,0});                // hyperslab
 	 *
 	 * std::string label;
 	 * h5::read(ds, label);                               // VLEN string
 	 * @endcode
 	 *
 	 * \sa_h5cpp
 	 * \sa_hdf5
 	 * @sa h5::open h5::write h5::create @ref link_base_template_types
 	 *     "Supported Types"
 	 */
	template<class T, class... args_t>
	inline void read(const h5::ds_t& ds, T& ref, args_t&&... args) try {
		using tcount  = typename arg::tpos<const h5::count_t&, const args_t&...>;
		using element_t = typename impl::decay<T>::type;
		using traits  = h5::meta::access_traits_t<T>;
		using sr_t    = h5::meta::storage_representation_t;
		constexpr auto kind    = traits::kind;
		constexpr auto storage = h5::meta::storage_representation_v<T>;

		static_assert(!tcount::present,
			"h5::count_t{ ... } is already present when passing arg by reference, did you mean to pass by pointer?");

		// Stopper: scatter types must use the fd-gateway overload so the compiler-generated
		// h5::gather<T> specialization can dispatch. Calling h5::read(ds, ref) on a scatter
		// type would silently bypass gather and fall through to the aggregate path with
		// register_struct<T>() == H5I_UNINIT.
		static_assert(!h5::has_scatter<std::decay_t<T>>::value,
			"h5::read(ds, ref): scatter types must use h5::read(fd, path, ref) so the "
			"compiler-generated h5::gather<T> specialization can dispatch.");

		// Stopper: unsupported storage usually means an unregistered POD aggregate, a
		// deeply-nested container, or std::vector<bool>. Bootstrap-aware: skipped when
		// h5cpp-compiler defines H5CPP_BUILDING_TYPE_INFO (generated.h not yet emitted).
#ifndef H5CPP_BUILDING_TYPE_INFO
		static_assert(storage != sr_t::unsupported,
			"h5::read: storage_representation_v<T> resolved to 'unsupported'. "
			"Check: unregistered POD aggregate (use H5CPP_REGISTER_STRUCT), "
			"std::vector<bool>, or container nesting beyond vector<vector<T>>/vector<string>.");
#endif

		// Stopper (borrowed from #274): containers of containers must route via VLEN
		// storage. element_t comes from impl::decay<T>, SFINAE-safe for non-container T.
		static_assert(
			!h5::meta::is_stl_like<element_t>::value ||
			storage == sr_t::ragged_vlen_dataset ||
			storage == sr_t::vlen_text_dataset ||
			storage == sr_t::array_dataset ||
			storage == sr_t::array_element ||
			storage == sr_t::fls_dataset ||
			storage == sr_t::fixed_inner_extent_dataset,
			"h5::read: containers of containers are only supported for vector<string> "
			"(vlen_text_dataset) or vector<vector<T>> (ragged_vlen_dataset).");

		if constexpr (storage == sr_t::array_element) {
			// Scalar dataspace, single H5T_ARRAY element. Use the file's
			// stored datatype directly via H5Dget_type so the in-memory and
			// on-disk type IDs are guaranteed equal — and pass H5S_ALL on
			// both sides to avoid scalar-space selection mismatches.
			// const_cast strips any const that survives traits::data(ref)
			// when ref is a nested std::array (the const overload picks up
			// the implicit `this`-const propagation).
			hid_t mem_type = H5Dget_type(static_cast<hid_t>(ds));
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			H5CPP_CHECK_NZ(
				H5Dread(static_cast<hid_t>(ds), mem_type,
					H5S_ALL, H5S_ALL,
					static_cast<hid_t>(dxpl), const_cast<void*>(static_cast<const void*>(traits::data(ref)))),
				h5::error::io::dataset::read, h5::error::msg::read_dataset);
			H5Tclose(mem_type);
		} else if constexpr (storage == sr_t::array_dataset) {
			using inner_t = std::remove_cv_t<typename std::remove_reference_t<T>::value_type>;
			using elem_scalar = typename inner_t::value_type;
			constexpr std::size_t N_inner = std::tuple_size<inner_t>::value;
			hsize_t array_dims[1] = { static_cast<hsize_t>(N_inner) };
			h5::meta::resolved_type_t<elem_scalar> base_type;
			hid_t array_type = H5Tarray_create(static_cast<hid_t>(base_type), 1, array_dims);
			h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
			hsize_t outer = 0;
			H5Sget_simple_extent_dims(static_cast<hid_t>(file_space), &outer, nullptr);
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			h5::sp_t mem_space = h5::create_simple(outer);
			h5::select_all(mem_space);
			H5Sselect_all(file_space);
			// Vector: direct buffer. List/set/etc.: read into scratch, then assign.
			if constexpr (h5::meta::has_data_pointer<std::remove_cv_t<std::remove_reference_t<T>>>::value) {
				ref.resize(static_cast<std::size_t>(outer));
				H5CPP_CHECK_NZ(
					H5Dread(static_cast<hid_t>(ds), array_type,
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
						static_cast<hid_t>(dxpl), ref.empty() ? nullptr : ref.front().data()),
					h5::error::io::dataset::read, h5::error::msg::read_dataset);
			} else {
				std::vector<inner_t> scratch(static_cast<std::size_t>(outer));
				H5CPP_CHECK_NZ(
					H5Dread(static_cast<hid_t>(ds), array_type,
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
						static_cast<hid_t>(dxpl), scratch.empty() ? nullptr : scratch.front().data()),
					h5::error::io::dataset::read, h5::error::msg::read_dataset);
				if constexpr (h5::meta::is_set_like<std::remove_cv_t<std::remove_reference_t<T>>>::value) {
					ref = std::remove_cv_t<std::remove_reference_t<T>>(scratch.begin(), scratch.end());
				} else {
					ref.assign(scratch.begin(), scratch.end());
				}
			}
			H5Tclose(array_type);
		} else if constexpr (storage == sr_t::fls_dataset) {
			using inner_t = std::remove_cv_t<typename std::remove_reference_t<T>::value_type>;
			constexpr std::size_t N_inner = std::tuple_size<inner_t>::value;
			hid_t str_type = H5Tcopy(H5T_C_S1);
			H5Tset_size(str_type, N_inner);
			h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
			hsize_t outer = 0;
			H5Sget_simple_extent_dims(static_cast<hid_t>(file_space), &outer, nullptr);
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			h5::sp_t mem_space = h5::create_simple(outer);
			h5::select_all(mem_space);
			H5Sselect_all(file_space);
			if constexpr (h5::meta::has_data_pointer<std::remove_cv_t<std::remove_reference_t<T>>>::value) {
				ref.resize(static_cast<std::size_t>(outer));
				H5CPP_CHECK_NZ(
					H5Dread(static_cast<hid_t>(ds), str_type,
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
						static_cast<hid_t>(dxpl), ref.empty() ? nullptr : ref.front().data()),
					h5::error::io::dataset::read, h5::error::msg::read_dataset);
			} else {
				std::vector<inner_t> scratch(static_cast<std::size_t>(outer));
				H5CPP_CHECK_NZ(
					H5Dread(static_cast<hid_t>(ds), str_type,
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
						static_cast<hid_t>(dxpl), scratch.empty() ? nullptr : scratch.front().data()),
					h5::error::io::dataset::read, h5::error::msg::read_dataset);
				if constexpr (h5::meta::is_set_like<std::remove_cv_t<std::remove_reference_t<T>>>::value) {
					ref = std::remove_cv_t<std::remove_reference_t<T>>(scratch.begin(), scratch.end());
				} else {
					ref.assign(scratch.begin(), scratch.end());
				}
			}
			H5Tclose(str_type);
		} else if constexpr (kind == h5::meta::access_t::composite) {
			// scalar composite (std::tuple<Ts...>): H5Dread into pack buffer, unpack into ref.
			std::vector<char> buf(traits::bytes());
			h5::meta::resolved_type_t<T> mem_type;
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			h5::sp_t mem_space{H5Screate(H5S_SCALAR)};
			h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
			H5Sselect_all(static_cast<hid_t>(file_space));
			H5CPP_CHECK_NZ(
				H5Dread(static_cast<hid_t>(ds), static_cast<hid_t>(mem_type),
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
					static_cast<hid_t>(dxpl), buf.data()),
				h5::error::io::dataset::read, h5::error::msg::read_dataset);
			traits::unpack(ref, buf.data());
		} else if constexpr (kind == h5::meta::access_t::text) {
			// Scalar text read — variable-length (std::string) reads via
			// char** relay + reclaim; fixed-length (char[N]) reads N raw
			// bytes straight into ref.
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
			if constexpr (storage == sr_t::fixed_length_string) {
				H5CPP_CHECK_NZ(
					H5Dread(static_cast<hid_t>(ds), str_type,
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
						static_cast<hid_t>(dxpl), traits::data(ref)),
					h5::error::io::dataset::read, h5::error::msg::read_dataset);
			} else {
				char* relay = nullptr;
				H5CPP_CHECK_NZ(
					H5Dread(static_cast<hid_t>(ds), str_type,
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
						static_cast<hid_t>(dxpl), &relay),
					h5::error::io::dataset::read, h5::error::msg::read_dataset);
				ref = relay ? std::decay_t<T>(relay) : std::decay_t<T>{};
				h5::impl::reference::reclaim(str_type, static_cast<hid_t>(mem_space), H5P_DEFAULT, &relay);
			}
			H5Tclose(str_type);
		} else if constexpr (kind == h5::meta::access_t::contiguous || kind == h5::meta::access_t::object) {
			auto ptr = traits::data(ref);
			h5::count_t count = traits::size(ref);
			::h5::read<typename traits::element_t>(ds, ptr, count, args...);
		} else if constexpr (kind == h5::meta::access_t::pointers) {
			if constexpr (storage == sr_t::vlen_text_dataset) {
				h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
				hsize_t n = 0;
				H5Sget_simple_extent_dims(static_cast<hid_t>(file_space), &n, nullptr);
				ref.resize(static_cast<std::size_t>(n));
				std::vector<char*> relay(static_cast<std::size_t>(n), nullptr);
				hid_t vlen_str = H5Tcopy(H5T_C_S1);
				H5Tset_size(vlen_str, H5T_VARIABLE);
				const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
				h5::sp_t mem_space = h5::create_simple(n);
				h5::select_all(mem_space);
				H5Sselect_all(static_cast<hid_t>(file_space));
				H5CPP_CHECK_NZ(
					H5Dread(static_cast<hid_t>(ds), vlen_str,
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
						static_cast<hid_t>(dxpl), relay.data()),
					h5::error::io::dataset::read, h5::error::msg::read_dataset);
				for (std::size_t i = 0; i < static_cast<std::size_t>(n); ++i)
					ref[i] = relay[i] ? std::string(relay[i]) : std::string{};
				h5::impl::reference::reclaim(vlen_str, static_cast<hid_t>(mem_space), H5P_DEFAULT, relay.data());
				H5Tclose(vlen_str);
			} else if constexpr (storage == sr_t::ragged_vlen_dataset) {
				using inner_t = typename traits::element_t;
				using elem_t  = typename inner_t::value_type;
				h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
				hsize_t n = 0;
				H5Sget_simple_extent_dims(static_cast<hid_t>(file_space), &n, nullptr);
				ref.resize(static_cast<std::size_t>(n));
				std::vector<hvl_t> relay(static_cast<std::size_t>(n));
				h5::meta::resolved_type_t<elem_t> base_type;
				hid_t vlen_type = H5Tvlen_create(static_cast<hid_t>(base_type));
				const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
				h5::sp_t mem_space = h5::create_simple(n);
				h5::select_all(mem_space);
				H5Sselect_all(static_cast<hid_t>(file_space));
				H5CPP_CHECK_NZ(
					H5Dread(static_cast<hid_t>(ds), vlen_type,
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
						static_cast<hid_t>(dxpl), relay.data()),
					h5::error::io::dataset::read, h5::error::msg::read_dataset);
				for (std::size_t i = 0; i < static_cast<std::size_t>(n); ++i) {
					const elem_t* src = static_cast<const elem_t*>(relay[i].p);
					// Sequence containers expose .assign(iter, iter); set-like
					// containers don't, but accept range construction.
					if constexpr (h5::meta::is_set_like<inner_t>::value ||
								  h5::meta::is_associative_like<inner_t>::value) {
						ref[i] = inner_t(src, src + relay[i].len);
					} else {
						ref[i].assign(src, src + relay[i].len);
					}
				}
				h5::impl::reference::reclaim(vlen_type, static_cast<hid_t>(mem_space), H5P_DEFAULT, relay.data());
				H5Tclose(vlen_type);
			} else if constexpr (h5::meta::access_kind_v<typename traits::element_t> == h5::meta::access_t::composite) {
				// vector<tuple<Ts...>>: H5Dread into pack buffer, unpack each element.
				using elem_traits = h5::meta::access_traits_t<typename traits::element_t>;
				h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
				hsize_t n = 0;
				H5Sget_simple_extent_dims(static_cast<hid_t>(file_space), &n, nullptr);
				ref.resize(static_cast<std::size_t>(n));
				std::vector<char> buf(static_cast<std::size_t>(n) * elem_traits::bytes());
				h5::meta::resolved_type_t<typename traits::element_t> mem_type;
				const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
				h5::sp_t mem_space = h5::create_simple(n);
				h5::select_all(mem_space);
				H5Sselect_all(static_cast<hid_t>(file_space));
				H5CPP_CHECK_NZ(
					H5Dread(static_cast<hid_t>(ds), static_cast<hid_t>(mem_type),
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
						static_cast<hid_t>(dxpl), buf.data()),
					h5::error::io::dataset::read, h5::error::msg::read_dataset);
				for (std::size_t i = 0; i < static_cast<std::size_t>(n); ++i)
					elem_traits::unpack(ref[i], buf.data() + i * elem_traits::bytes());
			}
		} else if constexpr (kind == h5::meta::access_t::iterators) {
			if constexpr (storage == sr_t::key_value_dataset) {
				using element_t = typename traits::element_t;
				using key_t   = std::remove_const_t<typename element_t::first_type>;
				using value_t = typename element_t::second_type;
				struct kv_t { key_t key; value_t value; };
				h5::meta::resolved_type_t<key_t>   kt;
				h5::meta::resolved_type_t<value_t> vt;
				hid_t compound = H5Tcreate(H5T_COMPOUND, sizeof(kv_t));
				H5Tinsert(compound, "key",   offsetof(kv_t, key),   static_cast<hid_t>(kt));
				H5Tinsert(compound, "value", offsetof(kv_t, value), static_cast<hid_t>(vt));
				h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
				hsize_t n = 0;
				H5Sget_simple_extent_dims(static_cast<hid_t>(file_space), &n, nullptr);
				std::vector<kv_t> buffer(static_cast<std::size_t>(n));
				h5::sp_t mem_space = h5::create_simple(n);
				h5::select_all(mem_space);
				H5Sselect_all(static_cast<hid_t>(file_space));
				const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
				H5CPP_CHECK_NZ(
					H5Dread(static_cast<hid_t>(ds), compound,
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
						static_cast<hid_t>(dxpl), buffer.data()),
					h5::error::io::dataset::read, h5::error::msg::read_dataset);
				ref.clear();
				for (const auto& kv : buffer)
					ref.insert({kv.key, kv.value});
				H5Tclose(compound);
			} else if constexpr (h5::meta::access_kind_v<typename traits::element_t> == h5::meta::access_t::composite) {
				// list<tuple>, set<tuple>, deque<tuple>: read pack buffer, unpack into container.
				using elem_t = typename traits::element_t;
				using elem_traits = h5::meta::access_traits_t<elem_t>;
				h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
				hsize_t n = 0;
				H5Sget_simple_extent_dims(static_cast<hid_t>(file_space), &n, nullptr);
				std::vector<char> buf(static_cast<std::size_t>(n) * elem_traits::bytes());
				h5::meta::resolved_type_t<elem_t> mem_type;
				h5::sp_t mem_space = h5::create_simple(n);
				h5::select_all(mem_space);
				H5Sselect_all(static_cast<hid_t>(file_space));
				const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
				H5CPP_CHECK_NZ(
					H5Dread(static_cast<hid_t>(ds), static_cast<hid_t>(mem_type),
						static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
						static_cast<hid_t>(dxpl), buf.data()),
					h5::error::io::dataset::read, h5::error::msg::read_dataset);
				ref.clear();
				std::vector<elem_t> staging(static_cast<std::size_t>(n));
				for (std::size_t i = 0; i < static_cast<std::size_t>(n); ++i)
					elem_traits::unpack(staging[i], buf.data() + i * elem_traits::bytes());
				if constexpr (std::is_same_v<T, std::forward_list<elem_t>>) {
					ref.assign(staging.begin(), staging.end());
				} else {
					std::copy(staging.begin(), staging.end(), std::inserter(ref, ref.end()));
				}
			} else {
				using element_t = typename impl::decay<typename traits::element_t>::type;
				// Guard (review item A2): non-std-layout element_t (e.g., escaped composite)
				// would silently produce wrong on-disk layout. Composite element types route
				// through the branch above; this catches any future escape.
				static_assert(std::is_standard_layout_v<element_t>,
					"h5::read: iterator-staging path requires standard-layout element_t. "
					"Wrap non-std-layout types in std::tuple (composite kind) or convert "
					"to a flat representation before reading.");
				h5::sp_t file_space{H5Dget_space(static_cast<hid_t>(ds))};
				hsize_t n = 0;
				H5Sget_simple_extent_dims(static_cast<hid_t>(file_space), &n, nullptr);
				std::vector<element_t> buffer(static_cast<std::size_t>(n));
				h5::count_t count{std::array<std::size_t,1>{static_cast<std::size_t>(n)}};
				::h5::read<element_t>(ds, buffer.data(), count, args...);
				ref.clear();
				if constexpr (std::is_same_v<T, std::forward_list<element_t>>) {
					ref.assign(buffer.begin(), buffer.end());
				} else {
					std::copy(buffer.begin(), buffer.end(), std::inserter(ref, ref.end()));
				}
			}
		} else {
			static_assert(kind != h5::meta::access_t::unsupported, "unsupported type for h5::read");
		}
	} catch (const std::exception& err) {
		throw h5::error::io::dataset::read(err.what());
	}
 	/**
 	 * \func_read_hdr
 	 * @brief Open a dataset by path and read into a caller-allocated container or value.
 	 *
 	 * Convenience overload — opens the dataset then forwards to
 	 * `h5::read(ds, ref, ...)`. For scatter-decomposed types (compiler-
 	 * emitted via `H5CPP_REGISTER_STRUCT` with deep nesting) routes
 	 * through the generated `h5::gather<T>` specialization instead.
 	 *
 	 * \par_fd
 	 * \par_dataset_path
 	 * \par_ref
 	 * \par_args
 	 * \tpar_T
 	 * \returns_err
 	 *
 	 * @throws h5::error::io::dataset::open   if the dataset is not present.
 	 * @throws h5::error::io::dataset::read   on `H5Dread` failure.
 	 *
 	 * \sa_h5cpp
 	 * \sa_hdf5
 	 * @sa h5::open h5::write
 	 */
	template<class T,  class... args_t,
		class = std::enable_if_t<!h5::meta::is_sparse_v<std::decay_t<T>>>> // dispatch to above
		void read( const h5::fd_t& fd,  const std::string& dataset_path, T& ref, args_t&&... args ){
		if constexpr (h5::has_scatter<std::decay_t<T>>::value) {
			// Gather path: compiler-generated gather<T> handles open + row read.
			h5::gather<std::decay_t<T>>(fd, dataset_path, ref);
		} else {
			// Stopper: mirror the ds-dispatch overload's unsupported-storage guard so
			// the gateway path fails at compile time too. (Scatter types are handled
			// by the if-branch above, before this assert.) Bootstrap-aware.
#ifndef H5CPP_BUILDING_TYPE_INFO
			static_assert(h5::meta::storage_representation_v<T> != h5::meta::storage_representation_t::unsupported,
				"h5::read: storage_representation_v<T> resolved to 'unsupported'. "
				"Check: unregistered POD aggregate (use H5CPP_REGISTER_STRUCT), "
				"std::vector<bool>, or container nesting beyond vector<vector<T>>/vector<string>.");
#endif
			const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
			h5::ds_t ds = h5::open(fd, dataset_path, dapl );
			::h5::read<T>(ds, ref, args...);
		}
	}

 	/**
 	 * \func_read_hdr
 	 * @brief Open a file and dataset by path then read into a caller-allocated container.
 	 *
 	 * Convenience overload — opens the file in `H5F_ACC_RDWR` mode and
 	 * forwards to `h5::read(fd, dataset_path, ref, ...)`.
 	 *
 	 * \par_file_path
 	 * \par_dataset_path
 	 * \par_ref
 	 * \par_args
 	 * \tpar_T
 	 * \returns_err
 	 *
 	 * @throws h5::error::io::file::open      if the file cannot be opened.
 	 * @throws h5::error::io::dataset::open   if the dataset is not present.
 	 * @throws h5::error::io::dataset::read   on `H5Dread` failure.
 	 *
 	 * \sa_h5cpp
 	 * \sa_hdf5
 	 * @sa h5::open h5::write
 	 */
	template<class T, class... args_t,
		class = std::enable_if_t<!h5::meta::is_sparse_v<std::decay_t<T>>>> // dispatch to above
	void read( const std::string& file_path, const std::string& dataset_path, T& ref, args_t&&... args ){

		h5::fd_t fd = h5::open( file_path, H5F_ACC_RDWR );
		::h5::read( fd, dataset_path, ref, args...);
	}

	/***************************  OBJECT *****************************/
 	/**
 	 * \func_read_hdr
 	 * @brief Return-by-value read — materialise the dataset as a fresh `T`.
 	 *
 	 * The most convenient form — `T` is constructed in-place from the
 	 * dataset's shape and populated in one call. Use this when you don't
 	 * already have a target container; use the by-reference overload
 	 * (`h5::read(ds, T& ref, ...)`) when you do.
 	 *
 	 * Optional `h5::offset` / `h5::stride` / `h5::count` / `h5::block`
 	 * select a hyperslab; omitting them returns the whole extent.
 	 *
 	 * \par_ds
 	 * \par_args
 	 * \tpar_T
 	 * \returns_object
 	 *
 	 * @throws h5::error::io::dataset::read   on `H5Dread` failure.
 	 *
 	 * <br/><b>example:</b>
 	 * @code
 	 * h5::ds_t ds = h5::open(fd, "/grid/data");
 	 *
 	 * auto v   = h5::read<std::vector<float>>(ds);                          // full extent
 	 * auto mat = h5::read<arma::Mat<double>>(fd, "/grid/data",              // hyperslab
 	 *                  h5::count{10,10}, h5::offset{5,0});
 	 * auto lbl = h5::read<std::string>(ds);                                  // VLEN string
 	 * @endcode
 	 *
 	 * \sa_h5cpp
 	 * \sa_hdf5
 	 * @sa h5::open h5::write h5::create @ref link_base_template_types
 	 *     "Supported Types"
 	 */
	template<class T, class... args_t>
	inline T read(const h5::ds_t& ds, args_t&&... args) {
		using tcount  = typename arg::tpos<const h5::count_t&, const args_t&...>;
		using element_t = typename impl::decay<T>::type;
		using traits  = h5::meta::access_traits_t<T>;
		using sr_t    = h5::meta::storage_representation_t;
		constexpr auto kind    = traits::kind;
		constexpr auto storage = h5::meta::storage_representation_v<T>;

		// Stopper: scatter types must use the fd-gateway overload.
		static_assert(!h5::has_scatter<std::decay_t<T>>::value,
			"h5::read<T>(ds): scatter types must use h5::read<T>(fd, path) so the "
			"compiler-generated h5::gather<T> specialization can dispatch.");
		// Stopper: unsupported storage. Bootstrap-aware (see ds-overload above).
#ifndef H5CPP_BUILDING_TYPE_INFO
		static_assert(storage != sr_t::unsupported,
			"h5::read<T>: storage_representation_v<T> resolved to 'unsupported'. "
			"Check: unregistered POD aggregate (use H5CPP_REGISTER_STRUCT), "
			"std::vector<bool>, or container nesting beyond vector<vector<T>>/vector<string>.");
#endif
		// Stopper: containers of containers must route via VLEN storage.
		static_assert(
			!h5::meta::is_stl_like<element_t>::value ||
			storage == sr_t::ragged_vlen_dataset ||
			storage == sr_t::vlen_text_dataset ||
			storage == sr_t::array_dataset ||
			storage == sr_t::array_element ||
			storage == sr_t::fls_dataset ||
			storage == sr_t::fixed_inner_extent_dataset,
			"h5::read<T>: containers of containers are only supported for vector<string> "
			"(vlen_text_dataset) or vector<vector<T>> (ragged_vlen_dataset).");

		h5::count_t size;
		const h5::count_t& count = arg::get(size, args...);
		h5::block_t default_block{1,1,1,1,1,1,1};
		const h5::block_t& block = arg::get(default_block, args...);

		if constexpr (!tcount::present) {
			h5::sp_t file_space = h5::get_space(ds);
			h5::get_simple_extent_dims(file_space, size);
		} else {
			for (int i = 0; i < count.rank; ++i) size[i] = count[i] * block[i];
			size.rank = count.rank;
		}

		if constexpr (storage == sr_t::array_element) {
			// Return-style read of std::array<T,N> / T[N] — construct T (which
			// holds N consecutive elements) then H5Dread into traits::data(ref).
			// Use the file's stored datatype directly via H5Dget_type so the
			// in-memory and on-disk types are guaranteed to match (avoids
			// HDF5's "no conversion path" error when two H5Tarray_create calls
			// produce structurally identical but distinct type IDs).
			T ref{};
			hid_t mem_type = H5Dget_type(static_cast<hid_t>(ds));
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			// Use H5S_ALL for both spaces — equivalent to the dataset's own
			// scalar dataspace with all elements selected.  Avoids the
			// "different number of elements selected" mismatch the explicit
			// H5Sselect_all path produces on scalar dataspaces.
			H5CPP_CHECK_NZ(
				H5Dread(static_cast<hid_t>(ds), mem_type,
					H5S_ALL, H5S_ALL,
					static_cast<hid_t>(dxpl), traits::data(ref)),
				h5::error::io::dataset::read, h5::error::msg::read_dataset);
			H5Tclose(mem_type);
			return ref;
		} else if constexpr (storage == sr_t::array_dataset) {
			using inner_t = std::remove_cv_t<typename T::value_type>;
			using elem_scalar = typename inner_t::value_type;
			constexpr std::size_t N_inner = std::tuple_size<inner_t>::value;
			hsize_t array_dims[1] = { static_cast<hsize_t>(N_inner) };
			h5::meta::resolved_type_t<elem_scalar> base_type;
			hid_t array_type = H5Tarray_create(static_cast<hid_t>(base_type), 1, array_dims);
			h5::sp_t file_space = h5::get_space(ds);
			hsize_t outer = 0;
			H5Sget_simple_extent_dims(static_cast<hid_t>(file_space), &outer, nullptr);
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			h5::sp_t mem_space = h5::create_simple(outer);
			h5::select_all(mem_space);
			H5Sselect_all(static_cast<hid_t>(file_space));
			// Read into scratch (always — return-style can't assume direct
			// .data() access since list / set / forward_list / etc. lack it).
			std::vector<inner_t> scratch(static_cast<std::size_t>(outer));
			H5CPP_CHECK_NZ(
				H5Dread(static_cast<hid_t>(ds), array_type,
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
					static_cast<hid_t>(dxpl), scratch.empty() ? nullptr : scratch.front().data()),
				h5::error::io::dataset::read, h5::error::msg::read_dataset);
			H5Tclose(array_type);
			if constexpr (h5::meta::is_set_like<T>::value) {
				return T(scratch.begin(), scratch.end());
			} else if constexpr (std::is_same_v<T, std::vector<inner_t>>) {
				return T(scratch.begin(), scratch.end());
			} else {
				T ref;
				if constexpr (std::is_same_v<T, std::forward_list<inner_t>>) {
					ref.assign(scratch.begin(), scratch.end());
				} else {
					ref.assign(scratch.begin(), scratch.end());
				}
				return ref;
			}
		} else if constexpr (storage == sr_t::fls_dataset) {
			using inner_t = std::remove_cv_t<typename T::value_type>;
			constexpr std::size_t N_inner = std::tuple_size<inner_t>::value;
			hid_t str_type = H5Tcopy(H5T_C_S1);
			H5Tset_size(str_type, N_inner);
			h5::sp_t file_space = h5::get_space(ds);
			hsize_t outer = 0;
			H5Sget_simple_extent_dims(static_cast<hid_t>(file_space), &outer, nullptr);
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			h5::sp_t mem_space = h5::create_simple(outer);
			h5::select_all(mem_space);
			H5Sselect_all(static_cast<hid_t>(file_space));
			std::vector<inner_t> scratch(static_cast<std::size_t>(outer));
			H5CPP_CHECK_NZ(
				H5Dread(static_cast<hid_t>(ds), str_type,
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
					static_cast<hid_t>(dxpl), scratch.empty() ? nullptr : scratch.front().data()),
				h5::error::io::dataset::read, h5::error::msg::read_dataset);
			H5Tclose(str_type);
			if constexpr (h5::meta::is_set_like<T>::value) {
				return T(scratch.begin(), scratch.end());
			} else {
				return T(scratch.begin(), scratch.end());
			}
		} else if constexpr (kind == h5::meta::access_t::composite) {
			// scalar composite (std::tuple<Ts...>): H5Dread into pack buffer, unpack into ref.
			std::vector<char> buf(traits::bytes());
			h5::meta::resolved_type_t<T> mem_type;
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			h5::sp_t mem_space{H5Screate(H5S_SCALAR)};
			h5::sp_t file_space = h5::get_space(ds);
			H5CPP_CHECK_NZ(
				H5Dread(static_cast<hid_t>(ds), static_cast<hid_t>(mem_type),
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
					static_cast<hid_t>(dxpl), buf.data()),
				h5::error::io::dataset::read, h5::error::msg::read_dataset);
			T ref{};
			traits::unpack(ref, buf.data());
			return ref;
		} else if constexpr (h5::meta::access_kind_v<element_t> == h5::meta::access_t::composite) {
			// container-of-composite (vector<tuple>, list<tuple>, set<tuple>, deque<tuple>):
			// read pack buffer, unpack each element, range-construct T (or assign for forward_list).
			using elem_traits = h5::meta::access_traits_t<element_t>;
			std::size_t n = 1;
			for (int i = 0; i < size.rank; ++i) n *= size[i];
			std::vector<char> buf(n * elem_traits::bytes());
			h5::meta::resolved_type_t<element_t> mem_type;
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			h5::sp_t mem_space = h5::create_simple(static_cast<hsize_t>(n));
			h5::select_all(mem_space);
			h5::sp_t file_space = h5::get_space(ds);
			H5Sselect_all(static_cast<hid_t>(file_space));
			H5CPP_CHECK_NZ(
				H5Dread(static_cast<hid_t>(ds), static_cast<hid_t>(mem_type),
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
					static_cast<hid_t>(dxpl), buf.data()),
				h5::error::io::dataset::read, h5::error::msg::read_dataset);
			std::vector<element_t> flat;
			flat.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				element_t elem{};
				elem_traits::unpack(elem, buf.data() + i * elem_traits::bytes());
				flat.push_back(std::move(elem));
			}
			if constexpr (std::is_same_v<T, std::forward_list<element_t>>) {
				T result; result.assign(flat.begin(), flat.end()); return result;
			} else { return T(flat.begin(), flat.end()); }
		} else if constexpr (kind == h5::meta::access_t::text) {
			// Scalar text return-style read — variable-length lands a
			// freshly-owned T (std::string) via char** relay; fixed-length
			// (char[N]) isn't returnable by value (C-arrays can't be), so
			// this branch only handles vlen_text. char[N] return-style is
			// rejected with a static_assert at the call site.
			static_assert(storage != sr_t::fixed_length_string,
				"h5::read<char[N]>(fd, path) is not supported — C-arrays "
				"cannot be returned by value. Use the by-reference form: "
				"`char buf[N]; h5::read(fd, path, buf);`");
			hid_t vlen_str = H5Tcopy(H5T_C_S1);
			H5Tset_size(vlen_str, H5T_VARIABLE);
			char* relay = nullptr;
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			h5::sp_t mem_space{H5Screate(H5S_SCALAR)};
			h5::sp_t file_space = h5::get_space(ds);
			H5Sselect_all(static_cast<hid_t>(file_space));
			H5CPP_CHECK_NZ(
				H5Dread(static_cast<hid_t>(ds), vlen_str,
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
					static_cast<hid_t>(dxpl), &relay),
				h5::error::io::dataset::read, h5::error::msg::read_dataset);
			T ref = relay ? T(relay) : T{};
			h5::impl::reference::reclaim(vlen_str, static_cast<hid_t>(mem_space), H5P_DEFAULT, &relay);
			H5Tclose(vlen_str);
			return ref;
		} else if constexpr (storage == sr_t::vlen_text_dataset) {
			// T = vector<string>: char** relay + reclaim.
			// Partial IO: when tcount::present the user passed h5::count{};
			// `size` already encodes count*block (see the cascade prologue),
			// so size[0] is the number of strings to read. We size the relay
			// to that, then narrow the file_space selection to the matching
			// hyperslab. Without tcount::present the prior `size` was set
			// from the file extent — select_all on file_space matches.
			// Mirrors the numeric pointer-read path at line ~118 and the
			// 2018 std::vector<std::string> specialization that the
			// access_traits_t × storage_representation_v rewire (commits
			// 30ca72af / 536408fa) consolidated but lost the hyperslab call.
			std::size_t n = static_cast<std::size_t>(size[0]);
			T ref; ref.resize(n);
			std::vector<char*> relay(n, nullptr);
			hid_t vlen_str = H5Tcopy(H5T_C_S1);
			H5Tset_size(vlen_str, H5T_VARIABLE);
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			h5::sp_t mem_space = h5::create_simple(static_cast<hsize_t>(n));
			h5::select_all(mem_space);
			h5::sp_t file_space = h5::get_space(ds);
			if constexpr (tcount::present) {
				h5::offset_t default_offset{0,0,0,0,0,0,0};
				h5::stride_t default_stride{1,1,1,1,1,1,1};
				h5::block_t  default_block_loc{1,1,1,1,1,1,1};
				const h5::offset_t& offset = arg::get(default_offset, args...);
				const h5::stride_t& stride = arg::get(default_stride, args...);
				const h5::block_t&  hyper_block = arg::get(default_block_loc, args...);
				h5::select_hyperslab(file_space, offset, stride, count, hyper_block);
			} else {
				H5Sselect_all(static_cast<hid_t>(file_space));
			}
			H5CPP_CHECK_NZ(
				H5Dread(static_cast<hid_t>(ds), vlen_str,
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
					static_cast<hid_t>(dxpl), relay.data()),
				h5::error::io::dataset::read, h5::error::msg::read_dataset);
			for (std::size_t i = 0; i < n; ++i)
				ref[i] = relay[i] ? std::string(relay[i]) : std::string{};
			h5::impl::reference::reclaim(vlen_str, static_cast<hid_t>(mem_space), H5P_DEFAULT, relay.data());
			H5Tclose(vlen_str);
			return ref;
		} else if constexpr (storage == sr_t::ragged_vlen_dataset) {
			// T = vector<vector<E>>: hvl_t relay + reclaim
			using inner_t = typename traits::element_t;
			using elem_t  = typename inner_t::value_type;
			std::size_t n = static_cast<std::size_t>(size[0]);
			T ref; ref.resize(n);
			std::vector<hvl_t> relay(n);
			h5::meta::resolved_type_t<elem_t> base_type;
			hid_t vlen_type = H5Tvlen_create(static_cast<hid_t>(base_type));
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			h5::sp_t mem_space = h5::create_simple(static_cast<hsize_t>(n));
			h5::select_all(mem_space);
			h5::sp_t file_space = h5::get_space(ds);
			H5Sselect_all(static_cast<hid_t>(file_space));
			H5CPP_CHECK_NZ(
				H5Dread(static_cast<hid_t>(ds), vlen_type,
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
					static_cast<hid_t>(dxpl), relay.data()),
				h5::error::io::dataset::read, h5::error::msg::read_dataset);
			for (std::size_t i = 0; i < n; ++i) {
				const elem_t* src = static_cast<const elem_t*>(relay[i].p);
				if constexpr (h5::meta::is_set_like<inner_t>::value ||
							  h5::meta::is_associative_like<inner_t>::value) {
					ref[i] = inner_t(src, src + relay[i].len);
				} else {
					ref[i].assign(src, src + relay[i].len);
				}
			}
			h5::impl::reference::reclaim(vlen_type, static_cast<hid_t>(mem_space), H5P_DEFAULT, relay.data());
			H5Tclose(vlen_type);
			return ref;
		} else if constexpr (storage == sr_t::key_value_dataset) {
			// T = map<K,V>: flat kv_t compound buffer, insert into map
			using element_t = typename traits::element_t;
			using key_t   = std::remove_const_t<typename element_t::first_type>;
			using value_t = typename element_t::second_type;
			struct kv_t { key_t key; value_t value; };
			h5::meta::resolved_type_t<key_t>   kt;
			h5::meta::resolved_type_t<value_t> vt;
			hid_t compound = H5Tcreate(H5T_COMPOUND, sizeof(kv_t));
			H5Tinsert(compound, "key",   offsetof(kv_t, key),   static_cast<hid_t>(kt));
			H5Tinsert(compound, "value", offsetof(kv_t, value), static_cast<hid_t>(vt));
			std::size_t n = static_cast<std::size_t>(size[0]);
			std::vector<kv_t> buffer(n);
			h5::sp_t mem_space = h5::create_simple(static_cast<hsize_t>(n));
			h5::select_all(mem_space);
			h5::sp_t file_space = h5::get_space(ds);
			H5Sselect_all(static_cast<hid_t>(file_space));
			const h5::dxpl_t& dxpl = arg::get(h5::default_dxpl, args...);
			H5CPP_CHECK_NZ(
				H5Dread(static_cast<hid_t>(ds), compound,
					static_cast<hid_t>(mem_space), static_cast<hid_t>(file_space),
					static_cast<hid_t>(dxpl), buffer.data()),
				h5::error::io::dataset::read, h5::error::msg::read_dataset);
			T ref;
			for (const auto& kv : buffer)
				ref.insert({kv.key, kv.value});
			H5Tclose(compound);
			return ref;
		} else if constexpr (kind == h5::meta::access_t::iterators) {
			// T = list<E>, set<E>, deque<E> with non-composite E — staging buffer + range construct.
			// Composite-element containers (list<tuple>, set<tuple>) are handled by the
			// access_kind_v<element_t> == composite branch above; this is the catch-all.
			using iter_elem_t = typename impl::decay<typename traits::element_t>::type;
			static_assert(std::is_standard_layout_v<iter_elem_t>,
				"h5::read<T>: iterator-staging path requires standard-layout element_t. "
				"Wrap non-std-layout types in std::tuple (composite kind) or convert "
				"to a flat representation before reading.");
			std::size_t n = 1;
			for (int i = 0; i < size.rank; ++i) n *= size[i];
			std::vector<iter_elem_t> flat(n);
			::h5::read<iter_elem_t>(ds, flat.data(), count, args...);
			if constexpr (std::is_same_v<T, std::forward_list<iter_elem_t>>) {
				T result;
				result.assign(flat.begin(), flat.end());
				return result;
			} else return T(flat.begin(), flat.end());
		} else {
			// contiguous / object / text: allocate, get pointer, raw read.
			// Two paths, branched on whether T has a named impl::rank<T> spec:
			//   (a) rank<T> > 0  → by-name registrations (std::vector, std::array,
			//                      arma::*, eigen, xtensor, …): impl::get<T>::ctor +
			//                      impl::data(ref).
			//   (b) rank<T> == 0 → detection-driven: any T with .data() + .size()
			//                      and a T(size_t) ctor lands here. Lets custom
			//                      vector-shaped containers round-trip without
			//                      registering impl::rank / impl::get / impl::data.
			using element_type = typename impl::decay<T>::type;
			if constexpr (impl::rank<T>::value == 0 &&
			              !std::is_arithmetic_v<T> &&
			              h5::meta::has_data<T>::value &&
			              h5::meta::has_size<T>::value &&
			              std::is_constructible_v<T, std::size_t>) {
				std::size_t n = 1;
				for (int i = 0; i < size.rank; ++i)
					n *= static_cast<std::size_t>(size[i]);
				T ref(n);
				auto* ptr = impl::structural_data(ref);
				::h5::read<element_type>(ds, ptr, count, args...);
				return ref;
			} else {
				T ref = impl::get<T>::ctor(count);
				element_type* ptr = impl::data(ref);
				::h5::read<element_type>(ds, ptr, count, args...);
				return ref;
			}
		}
	}


 	/**
 	 * \func_read_hdr
 	 * @brief Open a dataset by path and return its contents as a fresh `T`.
 	 *
 	 * Convenience overload — opens the dataset then forwards to
 	 * `h5::read<T>(ds, ...)`. Most concise form when both the file and
 	 * a one-shot read are needed in a single line.
 	 *
 	 * \par_fd
 	 * \par_dataset_path
 	 * \par_args
 	 * \tpar_T
 	 * \returns_object
 	 *
 	 * @throws h5::error::io::dataset::open   if the dataset is not present.
 	 * @throws h5::error::io::dataset::read   on `H5Dread` failure.
 	 *
 	 * \sa_h5cpp
 	 * \sa_hdf5
 	 * @sa h5::open h5::write
 	 */
	template<class T, class... args_t,
		class = std::enable_if_t<!h5::meta::is_sparse_v<std::decay_t<T>>>>
	inline T read( hid_t fd, const std::string& dataset_path, args_t&&... args ){

		const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
		h5::ds_t ds = h5::open(fd, dataset_path, dapl );
		return ::h5::read<T>(ds, args...);
	}
 	/**
 	 * \func_read_hdr
 	 * @brief Open a file + dataset by path and return contents as a fresh `T`.
 	 *
 	 * Most concise convenience form — opens the file in `H5F_ACC_RDWR`
 	 * mode, opens the dataset, performs the read, and returns the value
 	 * all in one line. All intermediate handles close via RAII.
 	 *
 	 * \par_file_path
 	 * \par_dataset_path
 	 * \par_args
 	 * \tpar_T
 	 * \returns_object
 	 *
 	 * @throws h5::error::io::file::open      if the file cannot be opened.
 	 * @throws h5::error::io::dataset::open   if the dataset is not present.
 	 * @throws h5::error::io::dataset::read   on `H5Dread` failure.
 	 *
 	 * <br/><b>example:</b>
 	 * @code
 	 * auto v = h5::read<std::vector<float>>("myfile.h5", "/path/to/dataset");
 	 * auto m = h5::read<arma::Mat<double>>("myfile.h5", "/path/to/dataset",
 	 *              h5::count{10,10}, h5::offset{5,0});
 	 * @endcode
 	 *
 	 * \sa_h5cpp
 	 * \sa_hdf5
 	 * @sa h5::open h5::write
 	 */
	template<class T, class... args_t,
		class = std::enable_if_t<!h5::meta::is_sparse_v<std::decay_t<T>>>> // dispatch to above
	inline T read(const std::string& file_path, const std::string& dataset_path, args_t&&... args ){
		h5::fd_t fd = h5::open( file_path, H5F_ACC_RDWR );
		return ::h5::read<T>( fd, dataset_path, args...);
	}
}
