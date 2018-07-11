#include <iostream>
#include <blitz/array.h>
#include <h5cpp/all>

using namespace std;
// 

template<class T> using Colvec = blitz::Array<T,1>;
template<class T> using Matrix = blitz::Array<T,2>;
template<class T> using Cube   = blitz::Array<T,3>;

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

	{
		Colvec<float> V(4); 			                  // create a vector
		// simple one shot write that computes current dimensions and saves matrix
		h5::write( "linalg.h5", "one shot create write",  V);
		// what if you want to position a matrix inside a higher dimension with some added complexity?	
		h5::write( "linalg.h5", "vector inside matrix",  V // object contains 'count' and rank being written
			,h5::current_dims{40,50}  // control file_space directly where you want to place vector
			,h5::offset{5,0}            // when no explicit current dimension given current dimension := offset .+ object_dim .* stride (hadamard product)  
 			,h5::stride{4,4}, h5::block{3,3}
			,h5::max_dims{40,H5S_UNLIMITED}  // wouldn't it be nice to have unlimited dimension? if no explicit chunk is set, then the object dimension 
							 // is used as unit chunk
		);
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

