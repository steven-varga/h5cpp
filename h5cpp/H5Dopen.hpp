/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#pragma once
#include "H5Pdapl.hpp"
#include <string>

namespace h5{
	/**
	 * \ingroup file-io
	 * @brief Open an existing HDF5 dataset by path.
	 *
	 * Returns an RAII-managed `h5::ds_t` handle; the underlying CAPI id
	 * is closed via `H5Dclose` on scope exit. Use `h5::read<T>(ds, ...)`
	 * to fetch values or `h5::write(ds, ...)` to update them.
	 *
	 * If the dataset's creation property list carries the h5cpp
	 * high-throughput pipeline tag (`H5CPP_DAPL_HIGH_THROUGHPUT`), the
	 * pipeline's per-chunk cache is initialised here from the dataset's
	 * element size, so subsequent reads/writes pick up the pre-warmed
	 * filter chain transparently.
	 *
	 * \par_fd
	 * \par_path
	 * @param dapl dataset access property list (`h5::dapl_t`)
	 * \returns_ds
	 *
	 * @throws h5::error::io::dataset::open  on `H5Dopen2` failure (dataset
	 *         does not exist, file not opened for read, invalid dapl).
	 *
	 * <br/><b>example:</b>
	 * @code
	 * h5::fd_t fd = h5::open("example.h5", H5F_ACC_RDWR);
	 * h5::ds_t ds = h5::open(fd, "/grid/data");
	 * auto v = h5::read<std::vector<float>>(ds);
	 * // ds, fd close automatically on scope exit
	 * @endcode
	 *
	 * \sa_h5cpp
	 * \sa_hdf5
	 * @sa h5::create h5::read h5::write @ref link_handle_reference
	 *     "Handles, Descriptors, and Property Lists"
	 */
    inline h5::ds_t open(const  h5::fd_t& fd, const std::string& path, const h5::dapl_t& dapl = h5::default_dapl ){

		H5CPP_CHECK_PROP( dapl, h5::error::io::dataset::open, "invalid data access property" );

		hid_t ds;
	   	H5CPP_CHECK_NZ((
			ds = H5Dopen2( static_cast<hid_t>(fd), path.data(), static_cast<hid_t>(dapl) )),
				   							h5::error::io::dataset::open, h5::error::msg::open_dataset );
		// custom h5cpp specific C++ high throughpout filter
		// is abstracted out and hidden under property list
		// see: H5Pdapl.hpp and H5Pchunk.hpp for details
		hid_t dcpl = H5Dget_create_plist( ds );

		switch( H5Pget_layout(dcpl) ){
			case H5D_LAYOUT_ERROR: break;
			case H5D_NLAYOUTS: break;  
			case H5D_COMPACT: break;
			case H5D_CONTIGUOUS: break;
			case H5D_CHUNKED:
				if( H5Pexist(dapl, H5CPP_DAPL_HIGH_THROUGHPUT) ){
					// grab pointer to uninitialized pipeline
					h5::impl::pipeline_t<impl::basic_pipeline_t>* ptr;
					H5Pget(dapl, H5CPP_DAPL_HIGH_THROUGHPUT, &ptr);
					hid_t type_id = H5Dget_type( static_cast<::hid_t>(ds) );
					size_t element_size = H5Tget_size( type_id );
					ptr->set_cache(dcpl, element_size);
				}
				break;
			case H5D_VIRTUAL: break;
		}

		h5::ds_t ds_{ds};
		//FIXME: temporary carry dapl around
		ds_.dapl = static_cast<hid_t>( dapl );
		return ds_;
    }
}

