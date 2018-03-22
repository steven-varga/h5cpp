/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 
 */

#include <memory>
#ifndef  H5CPP_APPEND_H 
#define H5CPP_APPEND_H

//#if H5_VERSION_GE(1,10,0)
//	#include "context-v110.hpp"
//#else
	#include "context-v18.hpp"
//#endif
namespace h5 {
	/** @ingroup io-append
	 * @brief extends HDF5 dataset along the first/slowest growing dimension, then writes passed object to the newly created space
	 * @param ctx context for dataset @see context
	 * @param ref T type const reference to object appended
	 * @tparam T dimensions must match the dimension of HDF5 space upto rank-1 
	 */ 
	template<typename T> inline void append( h5::context<T>& ctx, const T& ref){
		ctx.append( ref );
	}
}

#endif
