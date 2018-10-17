/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_DOPEN_HPP 
#define  H5CPP_DOPEN_HPP

namespace h5{
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
    inline h5::ds_t open(const  h5::fd_t& fd, const std::string& path, const h5::dapl_t& dapl = h5::default_dapl ){

		hid_t ds;
	   	H5CPP_CHECK_NZ((
			ds = H5Dopen( static_cast<hid_t>(fd), path.data(), static_cast<hid_t>(dapl))),
				   							h5::error::io::dataset::open, h5::error::msg::open_dataset );
     	return  h5::ds_t{ds};
    }
}

#endif

