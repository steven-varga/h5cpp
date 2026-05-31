/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include <type_traits>
#include <hdf5.h>
#include "H5Iall.hpp"
#include "H5Tall.hpp"
#include "H5Sall.hpp"
#include "H5capi.hpp"

// Return-type macro for h5::create<T>(parent, path, ...).
// Real compile: the SFINAE-constrained h5::impl::attr_parent_t<hid_t>
//   alias — only parents that can carry attributes (file, group,
//   dataset, opaque object, committed datatype) participate in overload
//   resolution.
// Doxygen   : collapses to plain h5::at_t so the rendered signature
//   reads cleanly without leaking the std::enable_if_t<...> machinery
//   into the headline. Gated on H5CPP_DOXYGEN from doxy/Doxyfile.
#ifdef H5CPP_DOXYGEN
#  define H5CPP_ATTR_CREATE_RET(hid_t) h5::at_t
#else
#  define H5CPP_ATTR_CREATE_RET(hid_t) h5::impl::attr_parent_t<hid_t>
#endif

namespace h5 {
	namespace impl {
		template <class H, class T=void> using is_valid_attr =
			std::bool_constant<std::is_same_v<H, ::hid_t> ||
				std::is_same_v<H, h5::gr_t> || std::is_same_v<H, h5::ds_t> ||
				std::is_same_v<H, h5::ob_t> || std::is_same_v<H, h5::dt_t<T>>>;
		/*template <class H, class T=void> using is_valid_attr =
			std::bool_constant<std::is_same_v<H, h5::ds_t>>;*/

		template <class hid_t>
		using attr_parent_t = std::enable_if_t<is_valid_attr<hid_t>::value, h5::at_t>;
	}

	/**
	 * \func_attr_hdr
	 * @brief Create a new attribute of element type `T` on a parent HDF5 object.
	 *
	 * Attributes are small named metadata items attached to a parent object (file, group, dataset, opaque object, or committed datatype).
	 * `h5::create` allocates the on-disk slot — the value itself is later  deposited with `h5::awrite(parent, path, value)` or read back with
	 * `h5::aread<T>(parent, path)`. The element type `T` follows the same dispatch as the dataset API (see @ref link_base_template_types
	 * "Supported Types"): elementary scalar, registered compound, fixed or variable-length string, etc. Attributes do not chunk and do not
	 * support partial I/O, so neither `h5::chunk{}` nor offset/stride/count  apply here.
	 *
	 * @param parent    open parent handle: raw `::hid_t`, `h5::gr_t`, `h5::ds_t`, `h5::ob_t`, or `h5::dt_t<T>` — enforced at compile
	 *                  time via `h5::impl::is_valid_attr`. Note: typed `h5::fd_t` is **not** accepted directly; pass `static_cast<::hid_t>(fd)`
	 *                  to attach an attribute to the file root. 
	 * @param path      attribute name (UTF-8); resolved relative to `parent`.
	 * \par_args_attr
	 * \tpar_T
	 * @tparam hid_t    deduced from the `parent` argument; must satisfy `h5::impl::is_valid_attr<hid_t>::value`.
	 * @return `h5::at_t` RAII handle owning the new attribute id; closes
	 *         automatically via `H5Aclose` on scope exit. Throws on failure.
	 *
	 * The optional arguments are context-sensitive and may be passed in  any order. By default the attribute is created with an empty
	 * (rank-0) shape and the default attribute creation property list:
	 *
	 * \par_current_dims
	 * Defaults to `{0}` — a scalar attribute. Pass a non-trivial extent for an array-valued attribute (e.g. `h5::current_dims{8}` for an
	 * 8-element vector attribute).
	 *
	 * @arg \c acpl —  attribute creation property list (`h5::acpl_t`); defaults to `H5P_ATTRIBUTE_CREATE`.
	 *
	 * @throws h5::error::io::attribute::create   on `H5Acreate2` failure (parent does not exist, attribute already exists, type
	 *         conversion error, etc.).
	 * @throws h5::error::property_list::misc     when an invalid acpl is supplied.
	 *
	 * <br/><b>example:</b> explicit pre-allocation — useful when the shape
	 * is non-default or you want the attribute slot to exist before any
	 * value is written. Most call sites skip `h5::create<T>` and let
	 * `h5::awrite` create-on-demand instead.
	 * @code
	 * h5::fd_t fd = h5::open("file.h5", H5F_ACC_RDWR);
	 * h5::ds_t ds = h5::open(fd, "/grid/data");
	 *
	 * // rank-1 of 3 floats; deposit the value later with h5::awrite.
	 * h5::create<float>(ds, "spacing", h5::current_dims{3});
	 *
	 * // Scalar attribute on the file root — fd_t needs an explicit cast
	 * // (h5::fd_t is not in is_valid_attr; raw ::hid_t is).
	 * h5::create<double>(static_cast<::hid_t>(fd), "schema_version");
	 *
	 * // Round-trip with h5::awrite / h5::aread:
	 * h5::awrite(ds, "spacing", std::vector<float>{0.5f, 0.5f, 1.0f});
	 * auto v = h5::aread<std::vector<float>>(ds, "spacing");
	 * @endcode
	 *
	 * \sa_h5cpp
	 * \sa_hdf5
	 * @sa h5::aread h5::awrite @ref link_handle_reference
	 *     "Handles, Descriptors, and Property Lists"
	 */
	template<class T, class hid_t, class... args_t>
	inline H5CPP_ATTR_CREATE_RET(hid_t)
	create( const hid_t& parent, const std::string& path, args_t&&... args ){
		try {
			h5::acpl_t default_acpl{ H5Pcreate(H5P_ATTRIBUTE_CREATE) };
			const h5::acpl_t& acpl = arg::get(default_acpl, args...);

			H5CPP_CHECK_PROP( acpl, h5::error::property_list::misc, "invalid attribute create property" );

			// and dimensions
			h5::current_dims_t current_dims_default{0}; // if no current dims_present
			// this mutable value will be referenced
			const h5::current_dims_t& current_dims = arg::get(current_dims_default, args...);
			// no partial IO or chunks
			h5::sp_t space = h5::create_simple( current_dims );
			using element_t = typename meta::decay<T>::type;
			h5::dt_t<element_t> type;
			::hid_t id = H5I_UNINIT;
			H5CPP_CHECK_NZ( (id = H5Acreate2( static_cast<::hid_t>( parent ), path.c_str(),
					static_cast<::hid_t>(type), static_cast<::hid_t>( space ), static_cast<::hid_t>( acpl ), H5P_DEFAULT )),
					h5::error::io::attribute::create, "couldn't create attribute");
			return h5::at_t{id};
		} catch( const std::runtime_error& err ) {
				throw h5::error::io::attribute::create( err.what() );
		}
	}
}
