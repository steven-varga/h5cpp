/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include "H5Pall.hpp"
#include "H5io_registry.hpp"   // h5::impl::close_global (MT close path; #286 registry retired)
#include <string>

/**
 * @namespace h5
 * @brief public namespace
 */
namespace h5{
	/** @ingroup file-io 
	 * opens an existing HDF5 file, the returned h5::fd_t descriptor automatically closes backed resource when leaving code block
	 * The h5::fd_t is a thin hid_t size object with std::unique_ptr like properties.
	 * \par_path \par_fopn_flags \par_fapl  \returns_fd
	 * \sa_h5cpp \sa_hdf5 \sa_stl
	 * @code 
	 * {
	 *    h5::fd_t fd = h5::open("example.h5", H5F_ACC_RDWR); // open an hdf5 file 
	 *    ...                                                 // do some work
	 * }                                                      // underlying hid_t is closed when leaving code block 
	 * @endcode
	 */ 
    inline h5::fd_t open(const std::string& path,  unsigned flags, const h5::fapl_t& fapl = h5::default_fapl ){
		H5CPP_CHECK_PROP( fapl,  h5::error::io::file::create, "invalid file access property list" );

        // MT: file open under the process-global HDF5 lock (see H5Fcreate.hpp).
        // No-op pass-through in a classic build.
        return h5::impl::on_collector([&]() -> h5::fd_t {
            hid_t fd;
            H5CPP_CHECK_NZ( (fd = H5Fopen(path.data(), flags,  static_cast<hid_t>(fapl))),
                   h5::error::io::file::open, h5::error::msg::open_file );
            return  h5::fd_t{fd};
        });
    }
}
