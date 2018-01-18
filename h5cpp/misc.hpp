/*
 * Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this  software  and associated documentation files (the "Software"), to deal in
 * the Software  without   restriction, including without limitation the rights to
 * use, copy, modify, merge,  publish,  distribute, sublicense, and/or sell copies
 * of the Software, and to  permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE  SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT  SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY  CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER
 * IN  AN  ACTION  OF  CONTRACT, TORT OR  OTHERWISE, ARISING  FROM,  OUT  OF  OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef  H5CPP_MISC_H
#define H5CPP_MISC_H
/**  
 * @namespace h5
 * @brief public namespace
 */
/*
 */
namespace h5{


	/**  @ingroup file-io
	 * @brief create an HDF5 file with default properties, in most cases this is all you need
	 * to have a container for datasets.
	 * @param path the location where the file is created
	 * @return an open hid_t  HDF5 file descriptor 
	 * @sa open close [gzip][10] [H5Fcreate][1] [H5Fopen][2] [H5Fclose][3] [H5Dopen][4] [H5Dclose][5] 
	 * @code 
	 * 		hid_t fd = h5::create("example.h5");  	// create file and save descriptor
	 * 		... 									// do some work
	 * 		h5::close(fd); 							// close file descriptor
	 * @endcode
	 * [1]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Create
	 * [2]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Open
	 * [3]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Close
	 * [4]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Open
	 * [5]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Close
	 * [10]: https://support.hdfgroup.org/HDF5/Tutor/compress.html
	 */
    inline hid_t create( const std::string& path ){

        hid_t plist = H5Pcreate(H5P_FILE_ACCESS);
        hid_t fd = H5Fcreate(path.data(), H5F_ACC_TRUNC, H5P_DEFAULT, plist);
        H5Pclose(plist);
        return fd;
    };
	
	/** @ingroup file-io 
	 * open an existing HDF5 file.
	 * @param path the location of the file
	 * @param flags (H5F_ACC_RDWR[|H5F_ACC_SWMR_WRITE])| H5F_ACC_RDONLY 
	 * @sa open close create  [H5Fcreate][1] [H5Fopen][2] [H5Fclose][3] [H5Dopen][4] [H5Dclose][5] 
	 * @code 
	 * 		hid_t fd = h5::open("example.h5", H5F_ACC_RDWR);  	// open an hdf5 file 
	 * 		... 												// do some work
	 * 		h5::close(fd); 										// close file descriptor
	 * @endcode
	 * [1]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Create
	 * [2]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Open
	 * [3]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Close
	 * [4]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Open
	 * [5]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Close
	 * [10]: https://support.hdfgroup.org/HDF5/Tutor/compress.html
	 */ 

    inline hid_t open(const std::string& path,  unsigned flags ){
        return  H5Fopen(path.data(), flags,  H5P_DEFAULT);
    };

	/** \ingroup file-io 

	 * open an existing HDF5 file
	 * @param fd valid HDF5 file descriptor
	 * @param path the location of the dataset within HDF5 file
	 * @sa open close create [gzip][10] [H5Fcreate][1] [H5Fopen][2] [H5Fclose][3] [H5Dopen][4] [H5Dclose][5] 
	 * @code 
	 * 		hid_t fd = h5::open("example.h5", H5F_ACC_RDWR);  	// open an hdf5 file in read-write mode
	 * 		hid_t ds = h5::open(fd,"dataset.txt") 				// obtain ds descriptor
	 * 		... 												// do some work
	 * 		H5Dclose(ds); 										// close ds descriptor
	 * 		H5Fclose(fd); 										// close file descriptor
	 * @endcode
	 * [1]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Create
	 * [2]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Open
	 * [3]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Close
	 * [4]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Open
	 * [5]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Close
	 * [10]: https://support.hdfgroup.org/HDF5/Tutor/compress.html
	 */ 
    inline hid_t open(hid_t fd, const std::string& path ){
     	return  H5Dopen(fd, path.data(), H5P_DEFAULT);
    };

	/** \ingroup file-io  
	 * closes opened file descriptor
	 * @param fd valid and opened file descriptor to an HDF5 file
	 * @sa open close create [H5Fcreate][1] [H5Fopen][2] [H5Fclose][3] [H5Dopen][4] [H5Dclose][5] 
	 * \code 
	 * 		hid_t fd = h5::open("example.h5", H5F_ACC_RDWR);  	// open an hdf5 file in read-write mode
	 * 		... 												// do some work
	 * 		h5::close(fd); 										// close file descriptor
	 * \endcode
	 * [1]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Create
	 * [2]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Open
	 * [3]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Close
	 * [4]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Open
	 * [5]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Close
	 * [10]: https://support.hdfgroup.org/HDF5/Tutor/compress.html
	 */
	inline void close(hid_t fd) {
		H5Fclose(fd);
	}

	/**  @ingroup file-io
	 * @brief removes default error handler	preventing diagnostic error messages printed
	 * @sa [H5Eset_auto2][11] [H5Error][12]  open close [H5Fcreate][1] [H5Fopen][2] [H5Fclose][3] 
	 * @code
	 * 		hid_t fd; 
	 * 		if( fd = h5::open("example.h5") < 0 )  	// try to open files
	 * 			fd = h5::create("example.h5"); 		// and create on failure
	 * 		... 									// do some work
	 * 		h5::close(fd); 							// close file descriptor
	 * @endcode
	 * [1]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Create
	 * [2]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Open
	 * [3]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Close
	 * [4]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Open
	 * [5]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5D.html#Dataset-Close
	 * [10]: https://support.hdfgroup.org/HDF5/Tutor/compress.html
	 * [11]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5E.html#Error-SetAuto2
	 * [12]: https://support.hdfgroup.org/HDF5/doc/H5.user/Errors.html
	 */
    inline void mute( ){
		H5Eset_auto (H5E_DEFAULT, NULL, nullptr);
    };


	/*
	 */ 
}
#endif

