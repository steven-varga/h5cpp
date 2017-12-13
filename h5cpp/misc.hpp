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
namespace h5{
	/**  \ingroup file-io 
	 * @brief create an HDF5 file
	 * simple mapping from the original H5Fcreate with default values, for refined access call the H5F_ relevant 
	 * routines  
	 * @param path the location where the file is created
	 * @return an open hid_t  HDF5 file descriptor 
	 * @see <a href="https://support.hdfgroup.org/HDF5/Tutor/compress.html">GZIP</a> @see close 
	 * @see <a href="https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Create">H5Fcreate</a>
	 * \code 
	 * 		hid_t fd = h5::create("example.h5");  	// create file and save descripor
	 * 		h5::close(fd); 							// close file descriptor
	 * \endcode
	 */
    inline hid_t create( const std::string& path ){

        hid_t plist = H5Pcreate(H5P_FILE_ACCESS);
        hid_t fd = H5Fcreate(path.data(), H5F_ACC_TRUNC, H5P_DEFAULT, plist);
        H5Pclose(plist);
        return fd;
    };
	/** \ingroup file-io  
	 * open an existing HDF5 file.
	 * @param path the location of the file
	 * @param flags (H5F_ACC_RDWR[|H5F_ACC_SWMR_WRITE])| H5F_ACC_RDONLY 
	 * @see <a href="https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Open">H5Fopen</a>
	 */ 
    inline hid_t open(const std::string& path,  unsigned flags ){

        hid_t plist = H5Pcreate(H5P_FILE_ACCESS);
        	hid_t fd =  H5Fopen(path.data(), flags,  plist);

        H5Pclose(plist);
        return fd;
    };
	/** \ingroup file-io 
	 * open an existing HDF5 file
	 * @param fd valid HDF5 file descripotor
	 * @param path the location of the file
	 * @see <a href="https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Open">H5Fopen</a>
	 */ 
    inline hid_t open(hid_t fd, const std::string& path ){
     	return  H5Dopen(fd, path.data(), H5P_DEFAULT);
    };

	/** \ingroup file-io  
	 * closes opened file descriptor
	 * @param fd valid and opened file descriptor to an HDF5 file
	 * @see <a href="https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Close">H5Fclose</a>
	 */
	inline void close(hid_t fd) {
		H5Fclose(fd);
	}
}
#endif

