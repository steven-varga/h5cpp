/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <mpi.h>
#include <h5cpp/all>
#include <chrono>
#include <vector>
#include <algorithm>

#pragma GCC diagnostic ignored "-Wnarrowing"
// armadillo
//./configure -DCMAKE_INSTALL_PREFIX=/usr/local -DDETECT_HDF5=OFF
int main(int argc, char** argv) {

	// usual boiler place
    int size, rank, name_len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Init(NULL, NULL);
	MPI_Info info  = MPI_INFO_NULL;
	MPI_Comm comm  = MPI_COMM_WORLD;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(processor_name, &name_len);

	int nrows = 10;
	{ // CREATE - WRITE
		std::vector<double> v(nrows);
		std::fill(std::begin(v), std::end(v), rank + 2 );
		// open file with MPIIO
		auto fd = h5::create("collective.h5", H5F_ACC_TRUNC,
				h5::fcpl, 
				h5::mpiio({MPI_COMM_WORLD, info}) // pass the communicator and hints as usual
		);
		// single write request is expanded to chunked write at compile time, setting up 
		// required arguments. 
		// Passed property lists, chunk and size descriptors may be interchanged, and or omitted
		h5::write( fd, "dataset", v,
				h5::chunk{nrows,1}, h5::current_dims{nrows,size}, h5::offset{0,rank}, h5::count{nrows,1},
				h5::collective ); // this makes `collective IO` magic happen
		// RAII will close all descriptors when leaving code block
	}

	{ // READ
		// open container with MPIIO enabled
		auto fd = h5::open("collective.h5", H5F_ACC_RDWR,  h5::mpiio({MPI_COMM_WORLD, info}));
		// this is a single shot read, all memory reservations are inside the `read` operator, convenient
		// but suboptimal for loops. 
		auto data = h5::read<std::vector<double>>(fd, "dataset", h5::offset{0,rank}, h5::count{nrows,1}, h5::collective);
		std::cout << "rank: " << rank <<" data: ";
		for( auto v : data) std::cout << v << " "; std::cout <<" ";

		// for high performance loops constructs please use:
		std::vector<double> buffer(nrows); // pre-allocate buffer, see documentation for variety of
		// linear algebra, the STL or raw memory objects

		// make sure to open dataset outside of the loop
		auto ds = h5::open(fd, "dataset");
		// this is as efficient as it gets
		h5::read(ds, buffer.data(),  h5::offset{0,rank}, h5::count{nrows,1}, h5::collective);
	}

	MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}
