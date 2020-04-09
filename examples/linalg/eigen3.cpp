/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <iostream>
#include <Eigen/Dense> // must include Eigen before <h5cpp/core>
#include <h5cpp/all>

using namespace std;
// EIGEN3 templates are unusually complex, let's use our template definitions
template<class T> using Matrix   = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>;
template<class T> using Colvec 	 = Eigen::Matrix<T, Eigen::Dynamic, 1, Eigen::ColMajor>;

template <class T> using ArrayX1D = Eigen::Array<T, Eigen::Dynamic, 1, Eigen::ColMajor>;
template <class T> using ArrayX3D = Eigen::Array<T, Eigen::Dynamic, 3, Eigen::RowMajor>;



// only EIGEN::DYNAMIC [ARRAY|MATRIX|VECTOR] are supported
// in other words Eigen::Matrix<T,S,S>  where S \in unsigned will not work, rather cast static allocation  into Dynamic (heap memory) structure

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
		Colvec<float> V(8); 			                  // create a vector
		// simple one shot write that computes current dimensions and saves matrix
		h5::write( "linalg.h5", "one shot create write",  V);
		// what if you want to position a matrix inside a higher dimension with some added complexity?	
		h5::write( "linalg.h5", "arma vec inside matrix",  V // object contains 'count' and rank being written
			,h5::current_dims{40,50}  // control file_space directly where you want to place vector
			,h5::offset{5,0}            // when no explicit current dimension given current dimension := offset .+ object_dim .* stride (hadamard product)  
 			,h5::count{1,1}, h5::stride{4,4}, h5::block{3,3}
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
		std::cout << M << std::endl;
	}
	
	{ // fixed/compile time  length arrays/matrices upto size 4 
		auto fd = h5::open("linalg.h5", H5F_ACC_RDWR);
		ArrayX1D<double> x1d = ArrayX1D<double>::Zero(10);
    	h5::write(fd, "/x1d ",  x1d);
    	ArrayX3D<float> x3d = ArrayX3D<float>::Zero(10,3); // first dimension is fixed size
    	h5::write(fd, "/x3d",  x3d);
	}
}

