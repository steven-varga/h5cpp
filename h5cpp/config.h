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

#include <complex>

namespace h5{
	typedef std::complex<double> cx_double;
	typedef std::complex<float>  cx_float;
}
/**
@example arma-partial.cpp
@example raw.cpp
@example stl.cpp
@example string.cpp
@example struct.cpp
@example arma-perf.cpp
@example stl-perf.cpp
@example struct-perf.cpp

@example arma.cpp
@example eigen3.cpp
*/

/** @defgroup io-create h5::create(fd, path, max_dims, chunk_dims, deflate );
 * \brief **fd** - file descriptor, **path** - how you reach dataset within file, **max_dims** -- tells the size of dataset, 
 * the value **H5S_UNLIMITED** marks given dimension **extendable**, **chunk_dims** tells the size of elementary IO operations 
 * if specified the rank of **max_dims** and **chunk_dims** must match.  
 * Empty set:{} indicates no-chunked operations.  And finally **deflate** controls the level of GZIP compression. 
 */

/** @defgroup io-read h5::read<T>( [ds | path], {offset}, {size} ); 
 * \brief Templated [full|partial] READ operations for T:= std::vector<S> | arma::Row<T> | arma::Col<T> | arma::Mat<T> | arma::Cube<T> objects 
 * from opened **ds** [dataset] descriptor or **path**.  Returned objects are [RVO] optimized and for calls within loops
 * **raw pointer** templates provide means to load  [dataset]s from **offset** and given **size**.  
 * [RVO]: http://en.cppreference.com/w/cpp/language/copy_elision
 * [dataset]: https://support.hdfgroup.org/HDF5/doc/H5.intro.html#Intro-PMRdWrPortion 
 */

/** @defgroup io-write h5::write<T>( [ds | path], Object<T>, {offset},{size} );
 *  \brief Templated WRITE operations Object:= std::vector<S> | arma::Row<T> | arma::Col<T> | arma::Mat<T> | arma::Cube<T> 
 */

/** @defgroup io-append h5::append(context, record);
 *  \brief dataset APPEND operations for streamed data access with examples
  */

/** @defgroup file-io h5::open | h5::close | h5::create | h5::mute
 *  \brief The  **open** | **close**, **create**  operations listed here are to create a place holder, an [hdf5 file][3], for your [datasets][2]. 
 *  In POSIX sense this is an entire **image** of a file system and the **dataset** is a file within. These datasets can be manipulated
 *  with **Create|Read|Write|Append** operation. File IO operations are straight maps from already [existing HDF5 calls][1], hence they are 
 *  freely interchangeable.
 *  [1]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html
 *  [2]: https://support.hdfgroup.org/HDF5/doc/H5.intro.html#Intro-ODatasets
 *  [3]: https://support.hdfgroup.org/HDF5/doc/H5.intro.html#Intro-FileOrg
 */






