/*
 * Copyright (c) 2018 - 2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_FCREATE_HPP
#define H5CPP_FCREATE_HPP

#include <hdf5.h>
#include "H5config.hpp"
#include "H5Eall.hpp"
#include "H5Iall.hpp"
#include "H5Pall.hpp"
#include <string>
/**  
 * @namespace h5
 * @brief public namespace
 */
/*
 */
namespace h5{
    /**  @ingroup file-io
     * @brief creates an HDF5 file  with given set of properties and returns a managed h5::fd_t resource handle. Depending on 
     * active [conversion policy](@ref link_conversion_policy) h5::fd_t may be passed
     * [implicitly,explicitly](http://en.cppreference.com/w/cpp/language/explicit) or not at all to
     * HDF5 CAPI calls.  
     *   
     *
     * \par_file_path \par_fcrt_flags \par_fcpl \par_fapl  \returns_fd
     * \sa_h5cpp \sa_hdf5 \sa_stl
     * @code
     * { 
     *    auto fd = h5::create("example.h5", H5F_ACC_TRUNC );  // create file and save descriptor
     *    H5Dcreate(fd, ...  );                                // CAPI call with implicit conversion
     *    H5Dcreate(static_cast<hid_t>(fd), ...);              // or explicit cast
     *
     *    h5::fd_t fd_2 = h5::create("other.h5",               // thin smart pointer like RAII support  
     *                  H5F_ACC_EXCL | H5F_ACC_DEBUG,          // CAPI arguments are implicitly converted   
     *                  H5P_DEFAULT, H5P_DEFAULT   );          // if enabled
     * }
     * { // DON'Ts: assign returned smart pointer to hid_t directly, becuase 
     *   // underlying handle gets assigned to, then the smart pointer closes backing 
     *   // resource as it goes out of scope:  
     *   // hid_t fd = h5::create("example.h5", H5_ACC_TRUNC); // fd is now invalid
     *   //
     *   // DOs: capture smart pointer only when you use it, then pass it to CAPI
     *   // if it were an hid_t, or use static_cast<> to signal intent
     *   auto fd  = h5::create("example.h5", H5_ACC_TRUNC);    // instead, do this
     *   H5Dopen(fd, "mydataset", ...  );                      // if implicit cast enabled, or 
     *   H5Dcreate(static_cast<hid_t>(fd), ...);               // when explicit cast enforced
     *   hid_t capi_fd = static_cast<hid_t>( fd );             // also fine, intent is clear 
     * }                                                       
     * @endcode
     */
    inline h5::fd_t create( const std::string& path, unsigned flags,
            const h5::fcpl_t& fcpl=h5::default_fcpl, const h5::fapl_t& fapl=h5::default_fapl ) {
        H5CPP_CHECK_PROP( fcpl,  h5::error::io::file::create, "invalid file control property list" );
        H5CPP_CHECK_PROP( fapl,  h5::error::io::file::create, "invalid file access property list" );

        hid_t fd;
        H5CPP_CHECK_NZ(
                    (fd = H5Fcreate(path.data(), flags, static_cast<hid_t>( fcpl ), static_cast<hid_t>( fapl ) )),
                    h5::error::io::file::create,    h5::error::msg::create_file);
        return fd_t{fd};
    }
}
#endif

