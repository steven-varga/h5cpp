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

#ifndef H5CPP_MAX_RANK
	#define H5CPP_MAX_RANK 5 //< maximum dimensions of stored arrays
#endif
#ifndef H5CPP_NO_COMPRESSION 
	#define H5CPP_NO_COMPRESSION 0 //< maximum dimensions of stored arrays
#endif
#ifndef H5CPP_DEFAULT_COMPRESSION 
	#define H5CPP_DEFAULT_COMPRESSION 9 //< maximum dimensions of stored arrays
#endif
#ifndef H5CPP_RANK_VEC 
	#define H5CPP_RANK_VEC 1
#endif
#ifndef H5CPP_RANK_MAT 
	#define H5CPP_RANK_MAT 2
#endif
#ifndef H5CPP_RANK_CUBE 
	#define H5CPP_RANK_CUBE 3
#endif


namespace h5{
	typedef std::complex<double> cx_double;
	typedef std::complex<float>  cx_float;
}
/**
@example arma.cpp
@example arma-partial.cpp
@example raw.cpp
@example stl.cpp
@example string.cpp
@example struct.cpp
*/

/** @defgroup io-create HDF5 dataset Create
 * \brief Detailed description when and how to **create** an HDF5 dataset within an HDF5 file whether to use **chunk**-ed dataset,
 *  what compression is supported, and missing features which are planned, and ones which for now not implemented.  
 */

/** @defgroup io-read HDF5 dataset Read
 *  \brief  Templated READ  operations for supported objects and use cases  
 */

/** @defgroup io-write HDF5 dataset Write
 *  \brief Templated WRITE operations 
 */

/** @defgroup io-append HDF5 dataset Append
 *  \brief dataset APPEND operations for streamed data access with examples
  */

/** @defgroup file-io HDF5 file IO
 *  \brief The  **open** | **close**, **create**  operations listed here are to create a place holder, an hdf5 file, for your datasets. In POSIX sense this is an entire **image** of a file system and the **dataset** is a file within that you manipulate with  the above Create|Read|Write|Append operation. File IO operations are straight maps from already existing HDF5 calls, hence they are freely interchangeable.
 */






