/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <iostream>
#include <dlib/matrix.h>
#include <h5cpp/all>

using namespace std;

template<class T> using Matrix = dlib::matrix<T>;
/* DLIB fails from stock install 2018 july 11
* /usr/include/dlib/matrix/matrix.h:1608:38: error: ISO C++17 does not allow dynamic exception specifications
*/
int main(){
	{ // CREATE - WRITE
		Matrix<short> M(2,3); 							            // create a matrix
		h5::fd_t fd = h5::create("linalg.h5",H5F_ACC_TRUNC); 	// and a file
		h5::ds_t ds = h5::create<short>(fd,"create then write"
				,h5::current_dims{10,20}
				,h5::max_dims{10,H5S_UNLIMITED}
				,h5::chunk{2,3} | h5::fill_value<short>{3} |  h5::gzip{9}
		);
		h5::write( ds,  M, h5::offset{2,2}, h5::stride{1,3}  );
	}

	{ // CREATE - READ: we're reading back the dataset created in the very first step
	  // note that data is only data, can be reshaped, cast to any format and content be modified through filtering 
		auto fd = h5::open("linalg.h5", H5F_ACC_RDWR,           // you can have multiple fd open with H5F_ACC_RDONLY, but single write
				h5::fclose_degree_strong | h5::sec2); 		   // and able to set various properties  
	}
	{ // READ: 
		Matrix<short> M = h5::read<Matrix<short>>("linalg.h5","create then write"); // read entire dataset back with a single read
	}
}

