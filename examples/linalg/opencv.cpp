/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <opencv2/core.hpp>
#include <h5cpp/all>


int main(){
	{ // CREATE - WRITE
		cv::Mat_<double> M(2,3);
		M(0,0) = 1.0; M(0,1) = 2.0; M(0,2) = 3.0;
		M(1,0) = 4.0; M(1,1) = 5.0; M(1,2) = 6.0;
		h5::fd_t fd = h5::create("opencv.h5",H5F_ACC_TRUNC);	// and a file
		h5::ds_t ds = h5::create<double>(fd,"create then write"
				,h5::current_dims{10,20}
				,h5::max_dims{10,H5S_UNLIMITED}
				,h5::chunk{2,3} | h5::fill_value<double>{3} |  h5::gzip{9}
		);
		h5::write( ds,  M, h5::offset{2,2}, h5::stride{1,3}  );
	}
	{
		cv::Vec<double,8> V;
		for(int i=0; i<8; i++) V[i] = i + 1.0;
		// simple one shot write that computes current dimensions and saves vector
		h5::write( "opencv.h5", "one shot create write",  V);
	}
	{ // READ:
		cv::Mat_<double> M = h5::read<cv::Mat_<double>>("opencv.h5","create then write");
	}
}
