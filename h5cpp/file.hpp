/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <hdf5.h>
#include <string>

#ifndef  H5CPP_FILE_H
#define H5CPP_FILE_H
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
			const h5::fcpl_t& fcpl=h5::default_fcpl, const h5::fapl_t& fapl=h5::default_fapl ) noexcept {

        hid_t fd = H5Fcreate(path.data(), flags, static_cast<hid_t>( fcpl ), static_cast<hid_t>( fapl ) );
        return fd_t{fd};
    }

	/** @ingroup file-io 
	 * opens an existing HDF5 file, the returned h5::fd_t descriptor automatically closes backed resource when leaving code block
	 * The h5::fd_t is a thin hid_t size object with std::unique_ptr like properties.
	 * \par_file_path \par_fopn_flags \par_fapl  \returns_fd
	 * \sa_h5cpp \sa_hdf5 \sa_stl
	 * @code 
	 * {
	 *    h5::fd_t fd = h5::open("example.h5", H5F_ACC_RDWR); // open an hdf5 file 
	 *    ...                                                 // do some work
	 * }                                                      // underlying hid_t is closed when leaving code block 
	 * @endcode
	 */ 
    inline h5::fd_t open(const std::string& path,  unsigned flags, const h5::fapl_t& fapl=h5::default_fapl ){
        return  h5::fd_t{
				H5Fopen(path.data(), flags,  static_cast<hid_t>(fapl))};
    }
	/** \ingroup file-io 

	 * open an existing dataset/document within an valid HDF5 file, the returned handle automatically closed when leaving code block
	 * \par_fd \par_dataset_path \par_dapl  \returns_fd 
	 * \sa_h5cpp \sa_hdf5  \sa_stl 
	 * @code
	 * { 
	 * 	  h5::fd_t fd = h5::open("example.h5", H5F_ACC_RDWR); // open an hdf5 file in read-write mode
	 * 	  h5::ds_t ds = h5::open(fd,"dataset.txt") 			  // obtain ds descriptor
	 * 	  ... 												  // do some work
	 * 	}	                                                  // ds and fd closes automatically 
	 * @endcode 
	 */
    inline h5::ds_t open(const  fd_t& fd, const std::string& path, const h5::dapl_t& dapl=h5::default_dapl ){
		hid_t ds = H5Dopen( static_cast<hid_t>(fd), path.data(), static_cast<hid_t>(dapl) );
     	return  h5::ds_t{ds};
    }

	/** \ingroup file-io  
	 * A dummy call, to facilitate open-do some work - close pattern
	 * \par_fd \sa_h5cpp \sa_hdf5 \sa_stl 
	 * \code 
	 *   h5::fd_t fd = h5::open("example.h5", H5F_ACC_RDWR);  	// returned descriptor automatically 
	 *      ... 												// do some work
	 *   h5::close(fd); 										// dummy call for symmetry
	 * \endcode
	 */
	inline void close(h5::fd_t& fd) {
	}

	/**  @ingroup file-io
	 * @brief removes default error handler	preventing diagnostic error messages printed
	 * for direct CAPI calls. This is a thread safe implementation. [Read on Error Handling/Exceptions](@ref link_error_handler)
	 * \sa_h5cpp \sa_hdf5 \sa_stl 
	 * @code
	 * h5::mute();             // mute  error handling
	 *    ... H5?xxx calls ... // do your capi calls which output annoying error messages
	 * h5::unmute();           // restore previously saved handler
	 * @endcode
	 */
    inline void mute( ){
		H5Eget_auto2(H5E_DEFAULT, &error_stack_callback, &error_stack_client_data);
		H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
    }
	/**  @ingroup file-io
	 * @brief restores previously saved error handler with h5::mute [Read on Error Handling/Exceptions](@ref link_error_handler)
	 * @code
	 * @code
	 * h5::mute();             // mute  error handling
	 *    ... H5?xxx calls ... // do your capi calls which output annoying error messages
	 * h5::unmute();           // restore previously saved handler
	 * @endcode
	 */
    inline void unmute( ){
		H5Eset_auto2(H5E_DEFAULT, error_stack_callback, error_stack_client_data);
    }
	/*
	 */ 
}
#endif

