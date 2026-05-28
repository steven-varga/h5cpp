/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include "H5Aopen.hpp"
#include <string>
#include <array>
#include <type_traits>
namespace h5 {
	namespace detail {
		// Cross-version helper: H5Treclaim (HDF5 >= 1.12) vs H5Dvlen_reclaim (older).
		inline herr_t attr_vlen_reclaim(hid_t type, hid_t space, void* buf) {
#if H5_VERSION_GE(1,12,0)
			return H5Treclaim(type, space, H5P_DEFAULT, buf);
#else
			return H5Dvlen_reclaim(type, space, H5P_DEFAULT, buf);
#endif
		}
	}

	template <class T, class P>
	std::enable_if_t<h5::impl::is_valid_attr<P>::value, T>
	aread( const P& ds, const std::string& name, const h5::acpl_t& acpl = h5::default_acpl ){
		using traits    = h5::meta::access_traits_t<T>;
		using sr_t      = h5::meta::storage_representation_t;
		using element_t = typename impl::decay<T>::type;
		constexpr auto kind    = traits::kind;
		constexpr auto storage = h5::meta::storage_representation_v<T>;

		// Stopper: unsupported storage usually means an unregistered POD aggregate, a
		// deeply-nested container, or std::vector<bool>. Bootstrap-aware (see H5Dwrite).
#ifndef H5CPP_BUILDING_TYPE_INFO
		static_assert(storage != sr_t::unsupported,
			"h5::aread: storage_representation_v<T> resolved to 'unsupported'. "
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
			"h5::aread: containers of containers are only supported for vector<string> "
			"(vlen_text_dataset) or vector<vector<T>> (ragged_vlen_dataset).");

		h5::at_t attr = h5::open(ds, name, h5::default_acpl);
		hid_t sid;
		H5CPP_CHECK_NZ( (sid = H5Aget_space( static_cast<hid_t>(attr) )),
			h5::error::io::attribute::read, "couldn't get space...");
		h5::sp_t file_space{sid};
		h5::current_dims_t current_dims;
		get_simple_extent_dims(file_space, current_dims);

		if constexpr (kind == h5::meta::access_t::composite) {
			// scalar composite (std::tuple<Ts...>): H5Aread into pack buffer, unpack into ref.
			std::vector<char> buf(traits::bytes());
			h5::meta::resolved_type_t<T> mem_type;
			H5CPP_CHECK_NZ( H5Aread( static_cast<hid_t>(attr), static_cast<hid_t>(mem_type), buf.data() ),
				h5::error::io::attribute::read, "couldn't read composite attribute.");
			T object{};
			traits::unpack(object, buf.data());
			return object;
		} else if constexpr (storage == sr_t::array_element) {
			// std::array<T,N> / T[N] — scalar dataspace, H5T_ARRAY element.
			// Use the file's stored type via H5Aget_type for an exact match.
			T object{};
			hid_t mem_type = H5Aget_type(static_cast<hid_t>(attr));
			H5CPP_CHECK_NZ(H5Aread(static_cast<hid_t>(attr), mem_type, traits::data(object)),
				h5::error::io::attribute::read, "couldn't read array_element attribute.");
			H5Tclose(mem_type);
			return object;
		} else if constexpr (storage == sr_t::array_dataset) {
			using inner_t = std::remove_cv_t<typename T::value_type>;
			using elem_scalar = typename inner_t::value_type;
			constexpr std::size_t N_inner = std::tuple_size<inner_t>::value;
			hsize_t array_dims[1] = { static_cast<hsize_t>(N_inner) };
			h5::meta::resolved_type_t<elem_scalar> base_type;
			hid_t array_type = H5Tarray_create(static_cast<hid_t>(base_type), 1, array_dims);
			std::size_t outer = impl::nelements(current_dims);
			T object; object.resize(outer);
			H5CPP_CHECK_NZ(H5Aread(static_cast<hid_t>(attr), array_type,
				object.empty() ? nullptr : object.front().data()),
				h5::error::io::attribute::read, "couldn't read array_dataset attribute.");
			H5Tclose(array_type);
			return object;
		} else if constexpr (storage == sr_t::fls_dataset) {
			using inner_t = std::remove_cv_t<typename T::value_type>;
			constexpr std::size_t N_inner = std::tuple_size<inner_t>::value;
			hid_t fls_type = H5Tcopy(H5T_C_S1);
			H5Tset_size(fls_type, N_inner);
			std::size_t outer = impl::nelements(current_dims);
			T object; object.resize(outer);
			H5CPP_CHECK_NZ(H5Aread(static_cast<hid_t>(attr), fls_type,
				object.empty() ? nullptr : object.front().data()),
				h5::error::io::attribute::read, "couldn't read fls_dataset attribute.");
			H5Tclose(fls_type);
			return object;
		} else if constexpr (kind == h5::meta::access_t::text
			&& storage == sr_t::fixed_length_string) {
			// char[N] / std::array<char,N> — H5T_C_S1 + H5Tset_size(N).
			T object{};
			hid_t fls_type = H5Tcopy(H5T_C_S1);
			H5Tset_size(fls_type, traits::fixed_length);
			H5CPP_CHECK_NZ(H5Aread(static_cast<hid_t>(attr), fls_type, traits::data(object)),
				h5::error::io::attribute::read, "couldn't read fixed-length string attribute.");
			H5Tclose(fls_type);
			return object;
		} else if constexpr (kind == h5::meta::access_t::text ||
		                     (kind == h5::meta::access_t::pointers &&
		                      h5::meta::is_text_like<element_t>::value)) {
			// Scalar std::string / std::vector<std::string>. The on-disk
			// attribute might be either VLEN (h5::awrite(ds, "x", std::string{...}))
			// or fixed-length (h5::awrite(ds, "x", "literal") — the literal
			// has type `char[N]` and routes to H5Awrite.hpp's fixed_length
			// branch). HDF5 has no fixed↔VLEN conversion path, so we must
			// match the source type on read; pick the mem_type based on the
			// attribute's stored datatype rather than always using VLEN.
			::hid_t atype = H5Aget_type(static_cast<hid_t>(attr));
			htri_t is_vlen = (H5Tget_class(atype) == H5T_STRING)
				? H5Tis_variable_str(atype) : htri_t{-1};
			if (is_vlen == 0 && std::is_same_v<std::string, T>) {
				// Fixed-length string on disk: read N raw bytes into a buffer
				// and construct std::string trimming NULLTERM padding.
				size_t asize = H5Tget_size(atype);
				std::vector<char> buf(asize, '\0');
				::hid_t fls_type = H5Tcopy(H5T_C_S1);
				H5Tset_size(fls_type, asize);
				H5CPP_CHECK_NZ( H5Aread(static_cast<hid_t>(attr), fls_type, buf.data()),
					h5::error::io::attribute::read, "couldn't read fixed-length string attribute.");
				H5Tclose(fls_type);
				H5Tclose(atype);
				size_t len = 0; while (len < asize && buf[len] != '\0') ++len;
				if constexpr (std::is_same_v<std::string, T>) {
					return T(buf.data(), len);
				} else {
					return T{};  // unreachable (guarded by is_same_v above)
				}
			}
			H5Tclose(atype);
			// VLEN path
			h5::dt_t<char*> type;
			T object = impl::get<T>::ctor(current_dims);
			size_t nelem = impl::nelements(current_dims);
			char** ptr = static_cast<char**>(malloc(nelem * sizeof(char*)));
			H5CPP_CHECK_NZ( H5Aread( static_cast<hid_t>(attr), static_cast<hid_t>(type), ptr ),
				h5::error::io::attribute::read, "couldn't read attribute...");
			if constexpr (std::is_same_v<std::string, T>) {
				object = std::string(*ptr);
			} else {
				for (size_t i = 0; i < nelem; i++)
					if (ptr[i] != nullptr) object[i] = std::string(ptr[i]);
			}
			detail::attr_vlen_reclaim(static_cast<hid_t>(type), static_cast<hid_t>(file_space), ptr);
			free(ptr);
			return object;
		} else if constexpr (kind == h5::meta::access_t::object) {
			// Scalar path: arithmetic, POD aggregates, complex<T>, pair<K,V>
			h5::dt_t<element_t> type;
			T object{};
			H5CPP_CHECK_NZ( H5Aread( static_cast<hid_t>(attr), static_cast<hid_t>(type), &object ),
				h5::error::io::attribute::read, "couldn't read attribute...");
			return object;
		} else if constexpr (kind == h5::meta::access_t::pointers) {
			// Nested: traits::element_t only referenced inside this block, where
			// the pointers access_traits_t spec guarantees it exists.
			if constexpr (storage == sr_t::ragged_vlen_dataset) {
				// vector<vector<T>>: hvl_t relay + reclaim.
				using inner_t = typename traits::element_t;
				using elem_t  = typename inner_t::value_type;
				std::size_t n = impl::nelements(current_dims);
				T object; object.resize(n);
				std::vector<hvl_t> relay(n);
				h5::meta::resolved_type_t<elem_t> base_type;
				hid_t vlen_type = H5Tvlen_create(static_cast<hid_t>(base_type));
				H5CPP_CHECK_NZ( H5Aread(static_cast<hid_t>(attr), vlen_type, relay.data()),
					h5::error::io::attribute::read, "couldn't read ragged_vlen attribute.");
				for (std::size_t i = 0; i < n; ++i) {
					const elem_t* src = static_cast<const elem_t*>(relay[i].p);
					object[i].assign(src, src + relay[i].len);
				}
				detail::attr_vlen_reclaim(vlen_type, static_cast<hid_t>(file_space), relay.data());
				H5Tclose(vlen_type);
				return object;
			} else if constexpr (h5::meta::access_kind_v<typename traits::element_t> == h5::meta::access_t::composite) {
				// vector<tuple<Ts...>>: H5Aread into pack buffer, unpack each.
				using elem_t = typename traits::element_t;
				using elem_traits = h5::meta::access_traits_t<elem_t>;
				std::size_t n = impl::nelements(current_dims);
				T object; object.resize(n);
				std::vector<char> buf(n * elem_traits::bytes());
				h5::meta::resolved_type_t<elem_t> mem_type;
				H5CPP_CHECK_NZ( H5Aread(static_cast<hid_t>(attr), static_cast<hid_t>(mem_type), buf.data()),
					h5::error::io::attribute::read, "couldn't read composite-element attribute.");
				for (std::size_t i = 0; i < n; ++i)
					elem_traits::unpack(object[i], buf.data() + i * elem_traits::bytes());
				return object;
			} else {
				static_assert(storage == sr_t::ragged_vlen_dataset, "h5::aread: unsupported pointers storage class.");
				return T{};
			}
		} else if constexpr (kind == h5::meta::access_t::iterators) {
			// Nested for the same reason as the pointers block above.
			if constexpr (storage == sr_t::key_value_dataset) {
				// map<K,V>: flat kv_t compound buffer, insert into map.
				using map_elem_t = typename traits::element_t;
				using key_t   = std::remove_const_t<typename map_elem_t::first_type>;
				using value_t = typename map_elem_t::second_type;
				struct kv_t { key_t key; value_t value; };
				h5::meta::resolved_type_t<key_t>   kt;
				h5::meta::resolved_type_t<value_t> vt;
				hid_t compound = H5Tcreate(H5T_COMPOUND, sizeof(kv_t));
				H5Tinsert(compound, "key",   offsetof(kv_t, key),   static_cast<hid_t>(kt));
				H5Tinsert(compound, "value", offsetof(kv_t, value), static_cast<hid_t>(vt));
				std::size_t n = impl::nelements(current_dims);
				std::vector<kv_t> buffer(n);
				H5CPP_CHECK_NZ( H5Aread(static_cast<hid_t>(attr), compound, buffer.data()),
					h5::error::io::attribute::read, "couldn't read key_value attribute.");
				T object;
				for (const auto& kv : buffer)
					object.insert({kv.key, kv.value});
				H5Tclose(compound);
				return object;
			} else if constexpr (h5::meta::access_kind_v<typename traits::element_t> == h5::meta::access_t::composite) {
				// list<tuple>, set<tuple>, deque<tuple>: read pack buffer, unpack into container.
				using elem_t = typename traits::element_t;
				using elem_traits = h5::meta::access_traits_t<elem_t>;
				std::size_t n = impl::nelements(current_dims);
				std::vector<char> buf(n * elem_traits::bytes());
				h5::meta::resolved_type_t<elem_t> mem_type;
				H5CPP_CHECK_NZ( H5Aread(static_cast<hid_t>(attr), static_cast<hid_t>(mem_type), buf.data()),
					h5::error::io::attribute::read, "couldn't read iter-composite attribute.");
				std::vector<elem_t> staging(n);
				for (std::size_t i = 0; i < n; ++i)
					elem_traits::unpack(staging[i], buf.data() + i * elem_traits::bytes());
				T object;
				if constexpr (std::is_same_v<T, std::forward_list<elem_t>>) {
					object.assign(staging.begin(), staging.end());
				} else {
					std::copy(staging.begin(), staging.end(), std::inserter(object, object.end()));
				}
				return object;
			} else {
				// list<T>, set<T>, deque<T> with non-composite, std-layout element_t.
				using iter_elem_t = typename impl::decay<typename traits::element_t>::type;
				static_assert(std::is_standard_layout_v<iter_elem_t>,
					"h5::aread: iterator-staging path requires standard-layout element_t.");
				std::size_t n = impl::nelements(current_dims);
				std::vector<iter_elem_t> staging(n);
				h5::dt_t<iter_elem_t> type;
				H5CPP_CHECK_NZ( H5Aread(static_cast<hid_t>(attr), static_cast<hid_t>(type), staging.data()),
					h5::error::io::attribute::read, "couldn't read iter attribute.");
				T object;
				if constexpr (std::is_same_v<T, std::forward_list<iter_elem_t>>) {
					object.assign(staging.begin(), staging.end());
				} else {
					std::copy(staging.begin(), staging.end(), std::inserter(object, object.end()));
				}
				return object;
			}
		} else {
			// Array/container path: std::array<T,N>, std::vector<T>, std::vector<std::complex<T>>,
			// armadillo / Eigen / Blaze / Blitz / xtensor linalg mapper types.
			//
			// Use impl::data (not traits::data) to get a writable pointer — the linalg
			// mapper access_traits_t specs only provide a const-qualified data() overload,
			// and H5Aread needs void* (no implicit conversion from const T*).
			//
			// Direct-initialise via impl::get<T>::ctor for rank > 0 (mirrors H5Dread's
			// array fallback at ~line 580). Default-construct + assign doesn't work for
			// every container — blitz::Array, in particular, ends up unallocated after
			// `T{}; obj = ctor(...)` because its operator= treats LHS shape as authoritative.
			h5::dt_t<element_t> type;
			if constexpr (impl::rank<T>::value > 0) {
				T object = impl::get<T>::ctor(current_dims);
				auto* ptr = impl::data(object);
				H5CPP_CHECK_NZ( H5Aread( static_cast<hid_t>(attr), static_cast<hid_t>(type), ptr ),
					h5::error::io::attribute::read, "couldn't read attribute...");
				return object;
			} else {
				T object{};
				auto* ptr = impl::data(object);
				H5CPP_CHECK_NZ( H5Aread( static_cast<hid_t>(attr), static_cast<hid_t>(type), ptr ),
					h5::error::io::attribute::read, "couldn't read attribute...");
				return object;
			}
		}
	}
}
