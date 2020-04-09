/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <armadillo>
#include <chrono>
#include <vector>
#include <functional>
#include <h5cpp/all>



// factored out timer, note that this is not quite right as you're 
// measuring 'lamda' mechanism of the compiler, OTOH is an example
// how flexible H5CPP underlying descriptors are
void timer( std::function<void()> callback ){

	using tictoc = std::chrono::high_resolution_clock;
	using us = std::chrono::microseconds;
	auto duration = std::chrono::duration_values<us>::zero();
	auto start = tictoc::now();
		callback();
	auto stop = tictoc::now();
	duration += std::chrono::duration_cast<us>(stop - start);
	std::cout << duration.count() << " [us]" << std::endl;
}


int main() {

	h5::fd_t fd = h5::create("pt.h5", H5F_ACC_TRUNC);
	h5::pt_t pt = h5::create<double>(fd, "vectors",
									h5::max_dims{H5S_UNLIMITED, 128*1024},
									h5::chunk{1, 128*1024});
	{   // VARIANT 1: vector<objects<elements>> save element wise, no penalty as you write directly to buffer

		// I see what you're doing here, while semantically correct
		// please observe that std::vector<arma::colvec> is NOT RUGGED hence is a 
		// matrix with colvector bases, a common construct to 'pack' a set
		// of BLAS level 1 into a single level 2 or 3 operation
		// in any event lets have this 'your way' then below 
		std::vector<arma::colvec> v(8);  // a batch of 8 chunks
		for (size_t i = 0; i < v.size(); ++i)
			v[i] = arma::randu<arma::colvec>(128*1024) + i; // let's add 'i' to color rows

		// c++ is not a bore :) let's capture 'h5::pt_t pt' by reference
		timer( [&]() -> void {
			for( size_t i = 0; i < v.size(); i++)
				for (size_t j = 0; j < v[i].n_elem; ++j) h5::append(pt, v[i][j]);
		});
	}
	{   // VARIANT 2: set of non-rugged vector is a matrix with column vec bases
		arma::mat M(8,128*1024);  // a batch of 8 chunks
		for (size_t i = 0; i < M.n_rows; ++i)
			M(i,arma::span::all) = arma::randu<arma::rowvec>(128*1024)+i;

		timer( [&]() -> void {
			arma::rowvec v(128*1024); // reserve buffer
			for( size_t i = 0; i < M.n_rows; i++){
				h5::append(pt,
				// arma::subview_row<T> should be cast to arma::rowvec: zero copy 
						static_cast<arma::rowvec>( M(i,arma::span::all) ));
			}
		});
	}
	{   // VARIANT 3: append/write 'chunk size' data block directly from raw memory buffer
		arma::mat M(8,128*1024);  // a batch of 8 chunks
		for (size_t i = 0; i < M.n_rows; ++i)
			M(i,arma::span::all) = arma::randu<arma::rowvec>(128*1024) + i;
		// this is an expensive operation, will not work with huge matrices
		arma::mat T = M.t();
		// data directly written to file from passed pointer, best performance
		timer( [&]() -> void {
			for (size_t i = 0; i < T.n_cols; ++i)
				h5::append(pt, &T(0,i) );
		});
	}
	/*  DOESN't WORK FOR NOW: in the next update will add functionality to 
	 *  chunk/block input data directly from object's buffer. The code is there
	 *  just not 'enabled' 
	 *
	{   // VARIANT 3: set of non-rugged vector is a matrix with column vec bases
		// the buffer is 'v' variable, and h5::append knows that no copy is needed
		// instead it uses the object memory address directly instead of internal buffer

		// CAVEAT: the row/col order is not correctly handled as of 2019 spring, work in progress
		// to make 'transpose' efficient and seamless. Please check this with Julia/R/Matlab 
		// if reads it in the right order
		arma::mat v(8,128*1024);  // a batch of 8 chunks
		for (size_t i = 0; i < v.n_rows; ++i)
			v(i,arma::span::all) = arma::randu<arma::rowvec>(128*1024) + 1;

		// since we buffered data acquisition h5::append is reduced to a single op 
		timer( [&]() -> void {
			//h5::append(pt, v);
		});
	}
	*/
	return 0;
}
