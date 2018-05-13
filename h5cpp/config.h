/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>

 */

#ifndef H5CPP_MAX_RANK
	#define H5CPP_MAX_RANK 7 //< maximum dimensions of stored arrays
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

#ifndef H5CPP_SMART_POINTERS_ONLY
#endif

// implicit conversion enabled by default `-DH5CPP_CONVERSION_EXPLICIT` to disable 
#ifndef H5CPP_CONVERSION_EXPLICIT
	#define H5CPP_CONVERSION_IMPLICIT
#endif
// conversion from CAPI enabled by default `-DH5CPP_CONVERSION_FROM_CAPI_DISABLED` to disable 
#ifndef H5CPP_CONVERSION_FROM_CAPI_DISABLED
	#define H5CPP_CONVERSION_FROM_CAPI
#endif
// conversion to CAPI enabled by default `-DH5CPP_CONVERSION_TO_CAPI_DISABLED` to disable 
#ifndef H5CPP_CONVERSION_TO_CAPI_DISABLED
	#define H5CPP_CONVERSION_TO_CAPI
#endif


#include <complex>
#include <hdf5.h>

namespace h5{
	using cx_double =  std::complex<double>; /**< scientific type */
	using cx_float = std::complex<float>;    /**< scientific type */
	using chunk_t = std::initializer_list<hsize_t>; /**< */
	using dims_t = std::initializer_list<hsize_t>;
	using count_t = std::initializer_list<hsize_t>;
	using offset_t = std::initializer_list<hsize_t>;
	using offset_t = std::initializer_list<hsize_t>;
}




/**
 
@example file.cpp
@example properties.cpp

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
@example ublas.cpp
@example itpp.cpp
@example blitz.cpp
@example blaze.cpp
@example dlib.cpp
*/

/** @defgroup io-wrap h5::fd_t,  h5::ds_t,  h5::at_t, h5::gr_t, h5::ob_t, h5::cpl_t, h5::apl_t, h5::tpl_t  
 * \brief thin [std::unique_ptr] like type safe wraps for CAPI **hid_t**,**herr_t** types which upon destruction call
 * [H5Dclose]|[H5Dclose]  facilitating [RAII]. Automatic/implicit conversion between CAPI and H5CPP equivalent 
 * allow transparent integration with existing CAPI code. 
 * `H5CPP_CAPI_CONVERSION_EXPLICIT` preprocessor directive provided to deny implicit conversion requiring explicit 
 * type cast: static_cast<hid_t>( fd_t ) or compile time error. 
 * \hdf5_links
 */




/** @defgroup file-io h5::open | h5::create | h5::mute
 *  \brief The  **open** | **close**, **create**  operations  are to create/manipulate a place holder, an hdf5 file, of datasets. 
 *  In POSIX sense the HDF5 container is an entire **image** of a file system and the **dataset** is a document within. The datasets can be manipulated
 *  with **Create|Read|Write|Append** operations. 
 *  File IO operations are straight maps from already existing CAPI HDF5 calls, with the addition of type safety, and 
 *  [RAII idiom](@ref link_raii_idiom) to aid productivity. 
 *  How the  returned managed handles may be passed to CAPI calls is governed by H5CPP conversion policy 
 *  [and you can read on the topic further on here](@ref link_conversion_policy) 
 *  \hdf5_links
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






