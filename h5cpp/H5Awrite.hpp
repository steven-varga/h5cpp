/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#pragma once
#include "H5Aopen.hpp"
#include <string>
#include <stdexcept>
#include <type_traits>
#include <initializer_list>
namespace h5 {
	// --- low-level attribute writers (leaf operations used by simple branches) ---
	// Generic template — handles all element pointer types. Used by both the
	// initializer_list overload and the iter-staging / object / contiguous
	// branches of the high-level awrite below.
	template <class T>
	inline void awrite( const h5::at_t& attr, const T* ptr ){
		using element_t = typename meta::decay<T>::type;
		h5::dt_t<element_t> type;
		H5CPP_CHECK_NZ( H5Awrite( static_cast<hid_t>(attr), static_cast<hid_t>( type ), ptr ),
				h5::error::io::attribute::write, "couldn't write attribute.");
	}
	// Note: a previously-present non-template `awrite(at_t, const char*)` overload
	// was removed because it caused overload-resolution conflicts with the template
	// above when called via explicit template syntax `awrite<char>(attr, const char*)`
	// (e.g., from the initializer_list overload writing `{'a','b','c'}`). Per C++
	// overload rules, the non-template wins over an equally-matching template, so
	// the vlen-string special path was hijacking ordinary `NATIVE_CHAR` writes and
	// causing HDF5 type-conversion failures. Writing a scalar vlen string to an
	// attribute is now done by the high-level `kind == text` branch directly.

	namespace detail {
		// Helper: open existing attribute or create new one with the given type+space.
		// Used by the special branches (vlen_text / ragged_vlen / key_value / composite)
		// where the standard h5::create<T> path (which uses dt_t<T>) cannot supply the
		// non-trivial HDF5 type — the caller builds the type explicitly.
		inline h5::at_t open_or_create_attr(
			hid_t parent, const std::string& name, hid_t type, hid_t space, hid_t acpl)
		{
			if (H5Aexists(parent, name.c_str()) > 0)
				return h5::open(parent, name, h5::default_acpl);
			hid_t id = H5I_UNINIT;
			H5CPP_CHECK_NZ((id = H5Acreate2(parent, name.c_str(), type, space, acpl, H5P_DEFAULT)),
				h5::error::io::attribute::create, "couldn't create attribute.");
			return h5::at_t{id};
		}
	}

	// --- high-level dispatched awrite: kind × storage matrix, mirrors H5Dwrite ---
	template <class T, class P, class... args_t>
	inline std::enable_if_t<h5::impl::is_valid_attr<P>::value,
	h5::at_t> awrite( const P& parent, const std::string& name, const T& ref, const h5::acpl_t& acpl = h5::default_acpl ) try {
		using traits    = h5::meta::access_traits_t<T>;
		using sr_t      = h5::meta::storage_representation_t;
		using element_t = typename impl::decay<T>::type;
		constexpr auto kind    = traits::kind;
		constexpr auto storage = h5::meta::storage_representation_v<T>;

		// Stopper: unsupported storage usually means an unregistered POD aggregate, a
		// deeply-nested container, or std::vector<bool>. Bootstrap-aware (see H5Dwrite).
#ifndef H5CPP_BUILDING_TYPE_INFO
		static_assert(storage != sr_t::unsupported,
			"h5::awrite: storage_representation_v<T> resolved to 'unsupported'. "
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
			"h5::awrite: containers of containers are only supported for vector<string> "
			"(vlen_text_dataset) or vector<vector<T>> (ragged_vlen_dataset).");

		if constexpr (kind == h5::meta::access_t::composite) {
			// scalar composite (std::tuple<Ts...>): pack into buffer matching
			// the HDF5 compound type's field offsets, scalar dataspace.
			std::vector<char> buf(traits::bytes());
			traits::pack(ref, buf.data());
			h5::meta::resolved_type_t<T> mem_type;
			h5::sp_t space{H5Screate(H5S_SCALAR)};
			h5::at_t attr = detail::open_or_create_attr(
				static_cast<hid_t>(parent), name,
				static_cast<hid_t>(mem_type), static_cast<hid_t>(space),
				static_cast<hid_t>(acpl));
			H5CPP_CHECK_NZ(
				H5Awrite(static_cast<hid_t>(attr), static_cast<hid_t>(mem_type), buf.data()),
				h5::error::io::attribute::write, "couldn't write composite attribute.");
			return attr;
		} else if constexpr (kind == h5::meta::access_t::text
			&& storage == sr_t::fixed_length_string) {
			// char[N] / std::array<char,N> — H5T_C_S1 + H5Tset_size(N), scalar.
			hid_t fls_type = H5Tcopy(H5T_C_S1);
			H5Tset_size(fls_type, traits::fixed_length);
			h5::sp_t space{H5Screate(H5S_SCALAR)};
			h5::at_t attr = detail::open_or_create_attr(
				static_cast<hid_t>(parent), name,
				fls_type, static_cast<hid_t>(space), static_cast<hid_t>(acpl));
			H5CPP_CHECK_NZ(
				H5Awrite(static_cast<hid_t>(attr), fls_type, traits::data(ref)),
				h5::error::io::attribute::write, "couldn't write fixed-length string attribute.");
			H5Tclose(fls_type);
			return attr;
		} else if constexpr (kind == h5::meta::access_t::text) {
			// Scalar VLEN text (std::string, std::string_view, const char*, char*).
			h5::dt_t<char*> vlen_str;
			h5::sp_t space{H5Screate(H5S_SCALAR)};
			h5::at_t attr = detail::open_or_create_attr(
				static_cast<hid_t>(parent), name,
				static_cast<hid_t>(vlen_str), static_cast<hid_t>(space),
				static_cast<hid_t>(acpl));
			const char* ptr = traits::data(ref);
			H5CPP_CHECK_NZ(
				H5Awrite(static_cast<hid_t>(attr), static_cast<hid_t>(vlen_str), &ptr),
				h5::error::io::attribute::write, "couldn't write text attribute.");
			return attr;
		} else if constexpr (storage == sr_t::array_element) {
			// std::array<T,N> / T[N] (non-char) — scalar dataspace, H5T_ARRAY[N] element.
			using element_t_loc = typename traits::element_t;
			auto dims = traits::size(ref);
			hsize_t array_dims[H5CPP_MAX_RANK];
			for (std::size_t i = 0; i < dims.size(); ++i) array_dims[i] = dims[i];
			h5::meta::resolved_type_t<element_t_loc> base_type;
			hid_t array_type = H5Tarray_create(static_cast<hid_t>(base_type),
				static_cast<unsigned>(dims.size()), array_dims);
			h5::sp_t space{H5Screate(H5S_SCALAR)};
			h5::at_t attr = detail::open_or_create_attr(
				static_cast<hid_t>(parent), name,
				array_type, static_cast<hid_t>(space), static_cast<hid_t>(acpl));
			H5CPP_CHECK_NZ(
				H5Awrite(static_cast<hid_t>(attr), array_type, traits::data(ref)),
				h5::error::io::attribute::write, "couldn't write array_element attribute.");
			H5Tclose(array_type);
			return attr;
		} else if constexpr (storage == sr_t::array_dataset) {
			// std::vector<std::array<T,N>> (non-char) — rank-1 of H5T_ARRAY[N].
			using inner_t = std::remove_cv_t<typename std::remove_reference_t<T>::value_type>;
			using elem_scalar = typename inner_t::value_type;
			constexpr std::size_t N_inner = std::tuple_size<inner_t>::value;
			hsize_t array_dims[1] = { static_cast<hsize_t>(N_inner) };
			h5::meta::resolved_type_t<elem_scalar> base_type;
			hid_t array_type = H5Tarray_create(static_cast<hid_t>(base_type), 1, array_dims);
			hsize_t outer = static_cast<hsize_t>(ref.size());
			h5::sp_t space{H5Screate_simple(1, &outer, nullptr)};
			h5::at_t attr = detail::open_or_create_attr(
				static_cast<hid_t>(parent), name,
				array_type, static_cast<hid_t>(space), static_cast<hid_t>(acpl));
			H5CPP_CHECK_NZ(
				H5Awrite(static_cast<hid_t>(attr), array_type,
					ref.empty() ? nullptr : ref.front().data()),
				h5::error::io::attribute::write, "couldn't write array_dataset attribute.");
			H5Tclose(array_type);
			return attr;
		} else if constexpr (storage == sr_t::fls_dataset) {
			// std::vector<std::array<char,N>> — rank-1 of H5T_C_S1+set_size(N).
			using inner_t = std::remove_cv_t<typename std::remove_reference_t<T>::value_type>;
			constexpr std::size_t N_inner = std::tuple_size<inner_t>::value;
			hid_t fls_type = H5Tcopy(H5T_C_S1);
			H5Tset_size(fls_type, N_inner);
			hsize_t outer = static_cast<hsize_t>(ref.size());
			h5::sp_t space{H5Screate_simple(1, &outer, nullptr)};
			h5::at_t attr = detail::open_or_create_attr(
				static_cast<hid_t>(parent), name,
				fls_type, static_cast<hid_t>(space), static_cast<hid_t>(acpl));
			H5CPP_CHECK_NZ(
				H5Awrite(static_cast<hid_t>(attr), fls_type,
					ref.empty() ? nullptr : ref.front().data()),
				h5::error::io::attribute::write, "couldn't write fls_dataset attribute.");
			H5Tclose(fls_type);
			return attr;
		} else if constexpr (kind == h5::meta::access_t::object ||
		                     kind == h5::meta::access_t::contiguous) {
			// arithmetic / pair / complex / std::vector<T> / std::array<T,N> / registered aggregate
			h5::current_dims_t current_dims = traits::size(ref);
			using attr_element_t = typename traits::element_t;
			h5::at_t attr = ( H5Aexists(static_cast<hid_t>(parent), name.c_str() ) > 0 ) ?
				h5::open(parent, name, h5::default_acpl) :
				h5::create<attr_element_t>(parent, name, current_dims);
			h5::awrite(attr, traits::data(ref));
			return attr;
		} else if constexpr (kind == h5::meta::access_t::pointers) {
			// Nested: traits::element_t only referenced inside this block, where
			// the pointers access_traits_t spec guarantees it exists.
			if constexpr (storage == sr_t::vlen_text_dataset) {
				// vector<string>: char* relay + H5T_VARIABLE string type, rank-1 attribute.
				std::vector<const char*> relay;
				relay.reserve(ref.size());
				for (const auto& s : ref) relay.push_back(s.c_str());
				h5::dt_t<char*> vlen_str;
				hsize_t n = ref.size();
				h5::sp_t space{H5Screate_simple(1, &n, nullptr)};
				h5::at_t attr = detail::open_or_create_attr(
					static_cast<hid_t>(parent), name,
					static_cast<hid_t>(vlen_str), static_cast<hid_t>(space),
					static_cast<hid_t>(acpl));
				H5CPP_CHECK_NZ(
					H5Awrite(static_cast<hid_t>(attr), static_cast<hid_t>(vlen_str), relay.data()),
					h5::error::io::attribute::write, "couldn't write vlen_text attribute.");
				return attr;
			} else if constexpr (storage == sr_t::ragged_vlen_dataset) {
				// vector<vector<T>>: hvl_t relay + H5Tvlen_create(base_type).
				using inner_t = typename traits::element_t;
				using elem_t  = typename inner_t::value_type;
				std::vector<hvl_t> relay(ref.size());
				for (std::size_t i = 0; i < ref.size(); ++i) {
					relay[i].len = ref[i].size();
					relay[i].p   = const_cast<void*>(static_cast<const void*>(ref[i].data()));
				}
				h5::meta::resolved_type_t<elem_t> base_type;
				hid_t vlen_type = H5Tvlen_create(static_cast<hid_t>(base_type));
				hsize_t n = ref.size();
				h5::sp_t space{H5Screate_simple(1, &n, nullptr)};
				h5::at_t attr = detail::open_or_create_attr(
					static_cast<hid_t>(parent), name, vlen_type,
					static_cast<hid_t>(space), static_cast<hid_t>(acpl));
				H5CPP_CHECK_NZ(
					H5Awrite(static_cast<hid_t>(attr), vlen_type, relay.data()),
					h5::error::io::attribute::write, "couldn't write ragged_vlen attribute.");
				H5Tclose(vlen_type);
				return attr;
			} else if constexpr (h5::meta::access_kind_v<typename traits::element_t> == h5::meta::access_t::composite) {
				// vector<tuple<Ts...>>: pack each element via element traits.
				using elem_t = typename traits::element_t;
				using elem_traits = h5::meta::access_traits_t<elem_t>;
				std::size_t n = ref.size();
				std::vector<char> buf(n * elem_traits::bytes());
				for (std::size_t i = 0; i < n; ++i)
					elem_traits::pack(ref[i], buf.data() + i * elem_traits::bytes());
				h5::meta::resolved_type_t<elem_t> mem_type;
				hsize_t hn = n;
				h5::sp_t space{H5Screate_simple(1, &hn, nullptr)};
				h5::at_t attr = detail::open_or_create_attr(
					static_cast<hid_t>(parent), name,
					static_cast<hid_t>(mem_type), static_cast<hid_t>(space),
					static_cast<hid_t>(acpl));
				H5CPP_CHECK_NZ(
					H5Awrite(static_cast<hid_t>(attr), static_cast<hid_t>(mem_type), buf.data()),
					h5::error::io::attribute::write, "couldn't write composite-element attribute.");
				return attr;
			} else {
				static_assert(storage == sr_t::vlen_text_dataset ||
			storage == sr_t::array_dataset ||
			storage == sr_t::array_element ||
			storage == sr_t::fls_dataset ||
			storage == sr_t::fixed_inner_extent_dataset, "h5::awrite: unsupported pointers storage class.");
				return h5::at_t{H5I_UNINIT};
			}
		} else if constexpr (kind == h5::meta::access_t::iterators) {
			// Nested for the same reason as the pointers block above.
			if constexpr (storage == sr_t::key_value_dataset) {
				// map<K,V> and variants: compound HDF5 type with "key" and "value" fields.
				using element_t = typename traits::element_t;
				using key_t   = std::remove_const_t<typename element_t::first_type>;
				using value_t = typename element_t::second_type;
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
				hsize_t n = ref.size();
				h5::sp_t space{H5Screate_simple(1, &n, nullptr)};
				h5::at_t attr = detail::open_or_create_attr(
					static_cast<hid_t>(parent), name, compound,
					static_cast<hid_t>(space), static_cast<hid_t>(acpl));
				H5CPP_CHECK_NZ(
					H5Awrite(static_cast<hid_t>(attr), compound, buffer.data()),
					h5::error::io::attribute::write, "couldn't write key_value attribute.");
				H5Tclose(compound);
				return attr;
			} else if constexpr (h5::meta::access_kind_v<typename traits::element_t> == h5::meta::access_t::composite) {
				// list<tuple>, set<tuple>, deque<tuple>: pack via element traits.
				using elem_t = typename traits::element_t;
				using elem_traits = h5::meta::access_traits_t<elem_t>;
				std::size_t n = traits::size(ref)[0];
				std::vector<char> buf(n * elem_traits::bytes());
				std::size_t i = 0;
				for (const auto& elem : ref) {
					elem_traits::pack(elem, buf.data() + i * elem_traits::bytes());
					++i;
				}
				h5::meta::resolved_type_t<elem_t> mem_type;
				hsize_t hn = n;
				h5::sp_t space{H5Screate_simple(1, &hn, nullptr)};
				h5::at_t attr = detail::open_or_create_attr(
					static_cast<hid_t>(parent), name,
					static_cast<hid_t>(mem_type), static_cast<hid_t>(space),
					static_cast<hid_t>(acpl));
				H5CPP_CHECK_NZ(
					H5Awrite(static_cast<hid_t>(attr), static_cast<hid_t>(mem_type), buf.data()),
					h5::error::io::attribute::write, "couldn't write iter-composite attribute.");
				return attr;
			} else {
				// list<T>, set<T>, deque<T> with non-composite, std-layout element_t.
				using iter_elem_t = typename impl::decay<typename traits::element_t>::type;
				static_assert(std::is_standard_layout_v<iter_elem_t>,
					"h5::awrite: iterator-staging path requires standard-layout element_t. "
					"Wrap non-std-layout types in std::tuple (composite kind) or convert "
					"to a flat representation before writing.");
				std::vector<iter_elem_t> buffer;
				buffer.reserve(traits::size(ref)[0]);
				for (const auto& elem : ref) buffer.push_back(elem);
				h5::current_dims_t current_dims = traits::size(ref);
				h5::at_t attr = ( H5Aexists(static_cast<hid_t>(parent), name.c_str() ) > 0 ) ?
					h5::open(parent, name, h5::default_acpl) :
					h5::create<iter_elem_t>(parent, name, current_dims);
				h5::awrite(attr, buffer.data());
				return attr;
			}
		} else {
			static_assert(kind != h5::meta::access_t::unsupported, "unsupported type for h5::awrite");
			return h5::at_t{H5I_UNINIT}; // unreachable; silences "no return" diagnostic
		}
	} catch( const std::runtime_error& err ){
		throw h5::error::io::attribute::write( err.what() );
	}

	// std::initializer_list<T> overload preserved verbatim (specialized form)
	template<class T, class P>
	inline std::enable_if_t<h5::impl::is_valid_attr<P>::value,
    h5::at_t> awrite( const P& parent, const std::string& name, const std::initializer_list<T> ref, const h5::acpl_t& acpl = h5::default_acpl ) try {
		h5::current_dims_t current_dims = impl::size( ref );
		using element_t = typename impl::decay<std::initializer_list<T>>::type;

		h5::at_t attr = ( H5Aexists(static_cast<hid_t>(parent), name.c_str() ) > 0 ) ?
			h5::open(parent, name, h5::default_acpl) : h5::create<element_t>(parent, name, current_dims);
		h5::awrite<element_t>(attr, impl::data( ref ) );
		return attr;
	} catch( const std::runtime_error& err ){
		throw h5::error::io::attribute::write( err.what() );
	}
}

template<> inline
h5::at_t h5::ds_t::operator[]( const char name[] ){
	//we don't have the object parameters yet available the only thing to do is
	//mark it H5I_UNINIT and in the second phase create the attribute
	h5::at_t attr = ( H5Aexists(static_cast<hid_t>(*this), name ) > 0 ) ?
			h5::open(static_cast<hid_t>( *this ), name, h5::default_acpl) : h5::at_t{H5I_UNINIT};
	attr.ds   = static_cast<hid_t>(*this);
	attr.name = std::string(name);
	return attr;
}
template<> template< class V> inline
h5::at_t h5::at_t::operator=( V arg ){
	if( !H5Iis_valid(this->ds) )
		throw h5::error::io::attribute::create("unable to create attribute: underlying dataset id not provided...");

	h5::awrite(ds, name, arg);
	return *this;
}
template<> template< class V> inline
h5::at_t h5::at_t::operator=( const std::initializer_list<V> args ){
	if( !H5Iis_valid(this->ds) )
		throw h5::error::io::attribute::create("unable to create attribute: underlying dataset id not provided...");

	h5::awrite(ds, name, args);
	return *this;
}
