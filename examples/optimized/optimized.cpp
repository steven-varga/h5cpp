/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <armadillo>
#include <h5cpp/all>


/* EXAMPLE:
 * to demonstrate how to factor out arguments for in-loop or lean + mean in-sub-routine data operations
 * using c+17 most operations are compiler time, and eliminated if not used. Arguments are stored on 
 * stack and have minimal size often none.
 * By not specifying optional arguments you give the compiler to eliminate entire branches compile time
 * resulting in highly optimized final binary: the way an expert wrote the code using HDF5 CAPI.  
 */

int main(){
	// have a file descriptor read
	h5::fd_t fd = h5::create("optimized.h5",H5F_ACC_TRUNC);

	// create or use existing linalg objects, the memory is reserved on heap and 
	// usually is a big chunk
	arma::mat M(10,1); M.zeros();
	// create or open the dataset withing HDF5 container, with the right properties set  
	h5::ds_t ds = h5::create<short>(fd,"huge dataset"
				,h5::current_dims{10,100} 			// the actual size of the dataspace created inside HDF5
				,h5::max_dims{10,H5S_UNLIMITED} 	// if it can grow: use `append` or H5CAPI calls 
				,h5::chunk{10,10} 					// compression and partial IO requires chunk-ing, which is 
													// reading data by small blocks at a time with internal caching mechanism
													// for handling edges and frequent read | write to same region
													//
				| h5::gzip{9} 						// compression comes at cost
				| h5::fill_value<short>{0} );


	// SUGGESTED: Notice that `count` is not specified, but created on 
	// that stack along `offset`. These operations have minimal impact if any. 
	for( hsize_t i=0; i < 1; i ++){
		h5::read( ds,  M, h5::offset{0,0} );
		M[0,0] = i; 	// your science thing, using 80% of in-core available memory
		// this is where you swap out 
		h5::write( ds,  M, h5::offset{0,i} );
	}


	/* EXTREME:
	 * h5::offset, h5::count, ... are space optimized (H5CPP_MAX_RANK + 1) * sizeof(hsize_t) small objects 
	 * placed on the local stack (not on the heap) suitable for pass by value, or reference. In the following
	 * section offset and count are factored out from the high performance loop to demonstrate how it can 
	 * be done, but in most cases this is an **overkill** and not the suggested way. 
	 * Use profiler to identify hot spots. See `profile` directory for examples.
	 * */
	h5::offset offset{0,0};  // must have the proper rank: this is rank 2
	h5::count  count{10,1};  // describe the memory region of `M` matrix

	// ready for optimized loop:
	for( hsize_t i=0; i < 1; i ++){
		offset[1] = i; 	// set new coordinates
		M[1,0] = i; 	// do your science thing
		h5::write( ds,  M, offset, count );
	}

}


