/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

// to activate must include: #include "rest_vol_public.h"
// see: https://bitbucket.hdfgroup.org/users/jhenderson/repos/rest-vol/browse
#ifdef H5CPP_WITH_KITA
	#define H5CPP_HAVE_KITA
#endif

// ROS3 VFD — set by CMake when H5_HAVE_ROS3_VFD is present in HDF5 pubconf
#ifdef H5CPP_HAVE_ROS3_VFD
namespace h5 { constexpr bool have_ros3_vfd = true; }
#else
namespace h5 { constexpr bool have_ros3_vfd = false; }
#endif

#ifndef H5CPP_MAX_RANK
	#define H5CPP_MAX_RANK 7 //< maximum dimensions of stored arrays
#endif

#ifndef H5CPP_MAX_FILTER
	#define H5CPP_MAX_FILTER 16 //< maximum number of filters in a chain
#endif
#ifndef H5CPP_MAX_FILTER_PARAM
	#define H5CPP_MAX_FILTER_PARAM 16 //< maximum number of filters in a chain
#endif
#ifndef H5CPP_MEM_ALIGNMENT
	#define H5CPP_MEM_ALIGNMENT 64 //< maximum number of filters in a chain
#endif
#ifndef H5CPP_PIPELINE_WORKERS
	#define H5CPP_PIPELINE_WORKERS 0 //< 0 = use hardware_concurrency()
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
	#define H5CPP_ERROR_MSG( msg ) std::string( __FILE__ ) + " line#  " + std::to_string( __LINE__ ) + " : " + msg
#endif

#define H5CPP_CHECK_EQ( call, exception, msg ) if( call == 0 ) throw exception( H5CPP_ERROR_MSG( msg ));
#define H5CPP_CHECK_NZ( call, exception, msg ) if( call < 0 ) throw exception( H5CPP_ERROR_MSG( msg ));
#define H5CPP_CHECK_SIZE( call, exception, msg ) if( call == 0 ) throw exception( H5CPP_ERROR_MSG( msg ));
#define H5CPP_CHECK_NULL( call, exception, msg ) if( call == nullptr ) throw exception( H5CPP_ERROR_MSG( msg ));
#define H5CPP_CHECK_PROP( id, exception, msg ) if( static_cast<::hid_t>( id ) < 0 ) throw exception( H5CPP_ERROR_MSG( msg ));
#define H5CPP_CHECK_ID( id, exception, msg ) if( !static_cast<::hid_t>( id ) ) throw exception( H5CPP_ERROR_MSG( msg ));


#ifndef H5CPP_CONSOLE_WIDTH 
	#define H5CPP_CONSOLE_WIDTH 30
#endif
/* uncomment for automatically detext OpenHDR half 
#ifdef _HALF_H // prefix openxdr half float
	#define WITH_OPENEXR_HALF
#endif
*/
#ifndef OPENEXR_NAMESPACE // prefix openxdr half float
	#define OPENEXR_NAMESPACE
#endif

/**
@example attributes.cpp
@example basics.cpp
@example compound.cpp
@example struct.h
@example container.cpp
@example detected.cpp
@example tiny_containers.hpp
@example cout.cpp
@example csv2hdf5.cpp
@example datasets.cpp
@example datatypes.cpp
@example pipeline.cpp
@example groups.cpp
@example arma.cpp
@example blaze.cpp
@example blitz.cpp
@example dlib.cpp
@example eigen3.cpp
@example itpp.cpp
@example ublas.cpp
@example mdspan.cpp
@example collective.cpp
@example independent.cpp
@example throughput.cpp
@example file_per_rank.cpp
@example main.cpp
@example optimized.cpp
@example packettable.cpp
@example packet_batches.cpp
@example pprint.cpp
@example raw.cpp
@example reference.cpp
@example reflection.cpp
@example s3.cpp
@example smart_ptr.cpp
@example eigen.cpp
@example maps.cpp
@example nested.cpp
@example sequences.cpp
@example sets.cpp
@example strings.cpp
@example tuples_pairs.cpp
@example string.cpp
@example transform.cpp
@example utf.cpp
*/


/** @defgroup datasets HDF5 datasets
 *  \brief Templated dataset I/O — `h5::create`, `h5::open`,
 *  `h5::read`, `h5::write`, packet-table streaming (`h5::append`
 *  / `h5::flush` / `h5::reset`), and the sparse-matrix CSC group
 *  form. Element type `T` follows the
 *  [Supported Types](@ref link_base_template_types) dispatch
 *  matrix; offset / stride / count / block select hyperslabs;
 *  chunking, compression, and access tuning go through the
 *  [property-list family](@ref link_property_lists).
 *  \sa_hdf5
 */

/** @defgroup attribute-io HDF5 attributes
 *  \brief Templated attribute I/O on any parent that can carry metadata —
 *  files, groups, datasets, opaque objects, committed datatypes. Element
 *  type `T` follows the same dispatch as the dataset API
 *  (see [Supported Types](@ref link_base_template_types)); attributes do
 *  not chunk, do not support partial I/O, and use the
 *  [acpl property list](@ref link_property_lists) family.
 *  \sa_hdf5
 */

/** @defgroup io-wrap RAII handles
 *  \brief Thin, `std::unique_ptr`-like type-safe wrappers for the CAPI
 *  `hid_t` / `herr_t` types. Closes the underlying CAPI handle on
 *  destruction or when passed to the corresponding HDF5 CAPI function.
 *  \sa_hdf5
 */

/** @defgroup file-io HDF5 files
 *  \brief Create, open, and close an HDF5 container.
 *  In POSIX terms an HDF5 container is the entire **image** of a file
 *  system and the **dataset** is a document within. Datasets are
 *  manipulated via `h5::create` / `h5::read` / `h5::write` /
 *  `h5::append`. File operations map directly onto HDF5 CAPI calls
 *  but are furnished with [RAII](@ref link_raii_idiom) and type safety.
 *  How the returned managed handles may be passed to CAPI calls is [governed by the H5CPP conversion policy](@ref link_conversion_policy).
 *  `h5::mute` | `h5::unmute` are miscellaneous thread-safe calls for the rare occasions when you need to turn HDF5 CAPI error-handler output
 *  **off** and **on** — typically used when failure is information (checking existence of a dataset / path by the call-fail pattern, etc.).
 *  \sa_hdf5
 */
