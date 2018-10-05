/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>

 */

#ifndef H5CPP_CONFIG_H
#define H5CPP_CONFIG_H

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
// redefine to your liking
#ifndef H5CPP_ERROR_MSG
	#define H5CPP_ERROR_MSG( msg ) std::string( __FILE__) + " line# " + std::to_string( __LINE__) + msg
#endif



/**
 
@example basics.cpp
@example compound.cpp
@example compound.c
@example compound.h
@example struct.cpp
@example struct.h
@example arma.cpp
@example blaze.cpp
@example blitz.cpp
@example dlib.cpp
@example eigen3.cpp
@example itpp.cpp
@example ublas.cpp
@example packettable.cpp
@example raw.cpp
@example transform.cpp

*/


/** @defgroup io-create template <T> ds_t create( file, path, space [,lcpl] [,dcpl] [,acpl] );
 * \brief **fd** - open file descriptor or path to hdf5 file,   **path** - how you reach dataset within file, 
 * **space** -- describes the current and maximum dimensions of dataset, 
 * [h5::lcpl | h5::dcpl h5::dapl](@ref link_property_lists) are to fine tune link,dataset properties.
 */

/** @defgroup io-read h5::read<T>( ds | path [,offset] [,stride] [,count] [,dxpl] ); 
 * \brief Templated full or partial IO READ operations that help you to have access to [dataset]s by either returning 
 * [supported linear algebra](@ref link_linalg_template_types) and [STL containers](@ref link_stl_template_types) 
 * or updating the content of already existing objects by passing reference or pointer 
 * to them.  The provided implementations rely on compile time constexpr evaluations, SFINEA pattern matching as 
 * well as static_assert compile time error handling where ever was permitted. Optional [runtime error mechanism](@ref link_error_handler)
 * added with HDF5 error stack unwinding otherwise. 
 * Starting from the  most convenient implementation, where you only have to point at a dataset and the right size object is returned, you find 
 * calls which operate on pre-allocated objects. In case the objects are unsupported there is efficient implementation for raw pointers.
 * When objects are passed then the number of elements are computed from the size of the object, therefore specifying **h5::count** is 
 * compile time error. On the other hand when working with classes and  raw pointers, **h5::count** is the only way to tell how much data you're to retrieve,
 * hence it is required.   
 * The first group of function arguments are mandatory whereas the optional arguments may be specified in any order, 
 * or omitted entirely.
 *
 * [dataset]: https://support.hdfgroup.org/HDF5/doc/H5.intro.html#Intro-PMRdWrPortion 
 */

/** @defgroup io-write herr_t h5::write<T>( ds | path, object<T> [,offset] [ ,stride ] [,count] [,dxpl] );
 *  \brief Templated WRITE operations object:= std::vector<S> | arma::Row<T> | arma::Col<T> | arma::Mat<T> | arma::Cube<T> | raw_ptr 
 */

/** @defgroup io-append h5::append<T>( pt , T object);
 *  \brief dataset APPEND operations for streamed data access with examples
 */

/** @defgroup io-wrap [  handle | type_id ] -s with RAII
 * \brief thin std::unique_ptr like type safe thin wrap-s for CAPI **hid_t**,**herr_t** types which upon destruction, or being passed to 
 * HDF5 CAPI functions will do the right thing. 
 * \hdf5_links
 */

/** @defgroup file-io h5::open | h5::create | h5::mute | h5::unmute
 *  \brief These  h5::open | h5::close | h5::create  operations  are to create/manipulate an hdf5 container. 
 *  In POSIX sense HDF5 container is an entire **image** of a file system and the **dataset** is a document within. Datasets may be manipulated
 *  with h5::create | h5::read | h5::write | h5::append operations. While File IO operations are straight maps from already existing CAPI HDF5 calls, 
 *  they are furnished with [RAII](@ref link_raii_idiom), and type safety to aid productivity. 
 *  How the  returned managed handles may be passed to CAPI calls is [governed by H5CPP conversion policy](@ref link_conversion_policy) 
 *  h5::mute | h5::unmute are miscellaneous thread safe calls for those rare occasions when you need to turn HDF5 CAPI error handler output
 *  **off** and **on**.  Typically used when failure is information: checking existence of [dataset|path] by call-fail pattern, etc...  
 *  \hdf5_links
 */

#endif





