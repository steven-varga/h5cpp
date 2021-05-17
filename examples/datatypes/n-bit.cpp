/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <algorithm>
#include <armadillo>
#include <Eigen/Dense> 
#include <h5cpp/core> // include this before custom type definition
	#include "n-bit.hpp"
#include <h5cpp/io> // IO operators become aware of your custom type

namespace ei {
	template <class T>
	using Matrix   = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
}

// in all cases when filtering used chunk must be set as well, no contiguous 
int main(){
	namespace bs = bitstring;

	h5::fd_t fd = h5::create("example.h5",H5F_ACC_TRUNC);
	// prints out type info, eases on debugging
	std::cout << h5::dt_t<bs::n_bit>() << std::endl;

	std::random_device rd;
    std::mt19937 random_int(rd());
    std::uniform_int_distribution<> sample(0, 3);

	{
	// method 1: use C++ conversion CTOR to convert from fundamental type to your custom one
	// notice the armadillo can only hold arithmetic types, you have to do some forcing 
	// also that colmajor, coordinates are swapped
		arma::Mat<unsigned char> M(12,8);
		std::generate(M.begin(), M.end(), [&](){ return sample(random_int);} );

		h5::ds_t ds = h5::create<bs::n_bit>(fd, "arma",
			   h5::current_dims{8,12,1}, h5::max_dims{8,12,H5S_UNLIMITED}, h5::chunk{4,3,1} | h5::nbit);
		// force conversion for zero copy: 
		h5::write<bs::n_bit>(fd,"arma", (bs::n_bit*)M.memptr(), h5::offset{0,0,0}, h5::count{8,12,1});

		arma::Mat<unsigned char> data(12,8);
	   	h5::read<bs::n_bit>(fd, "arma", (bs::n_bit*)data.memptr(), h5::offset{0,0,0}, h5::count{8,12,1});
		data.print();
		std::cout <<"\n\n";
		M.print();
	}
	{
	// method 2: eigen allows native type handling
		ei::Matrix<bs::n_bit> M(12,8);
		for(int i=0; i<12; i++) for( int j=0; j<8; j++)
	   		M(i,j) = static_cast<bs::n_bit>( sample(random_int));

		h5::ds_t ds = h5::create<bs::n_bit>(fd, "eigen", // chunk must be used with nbit
			   h5::current_dims{12,8}, h5::max_dims{12,H5S_UNLIMITED}, h5::chunk{3,4} | h5::nbit);
		h5::write<bs::n_bit>(ds, h5::impl::data(M), h5::count{12,8});

		ei::Matrix<bs::n_bit> data(12,8);
	  	h5::read(fd, "eigen", data, h5::offset{0,0});
		std::cout << data << std::endl <<std::endl << M <<std::endl;
	}
	{ //method 3, use STL 
		std::vector<bs::n_bit> V(12*8);
		std::generate(V.begin(), V.end(), [&](){
			return static_cast<bs::n_bit>(sample(random_int));
		});

		h5::ds_t ds = h5::create<bs::n_bit>(fd, "stl", // chunk must be used with nbit
			   h5::current_dims{12,8}, h5::max_dims{12,H5S_UNLIMITED}, h5::chunk{3,4} | h5::nbit);
		// from typed memory pointer to different shape
		h5::write<bs::n_bit>(ds, V.data(), h5::count{12,8}); // single shot write

		auto data = h5::read<std::vector<bs::n_bit>>(fd, "stl");
		for( int i=0; i<V.size(); i++ )
			std::cout << static_cast<unsigned int>( data[i] ) << " ";
		std::cout << "\n\ncomputing difference ||saved - read|| expecting norm to be zero:\n";
		for( int i=0; i<V.size(); i++ )
			std::cout << abs(V[i].value - data[i].value) <<" ";
	}
}

