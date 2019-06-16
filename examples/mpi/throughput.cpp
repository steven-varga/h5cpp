/* Copyright (c) 2019 vargaconsulting, Toronto,ON Canada
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
    int size, rank, name_len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Init(NULL, NULL);
	MPI_Info info  = MPI_INFO_NULL;
	MPI_Comm comm  = MPI_COMM_WORLD;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(processor_name, &name_len);

	int nrows = 100000000; // 800MB per rank!!
	{ // CREATE - WRITE
		std::vector<double> v(nrows);
		std::fill(std::begin(v), std::end(v), rank + 2 );
		size_t vsize =v.size() * sizeof(double);

		auto fd = h5::create("collective.h5", H5F_ACC_TRUNC,
				h5::fcpl, 
				h5::mpiio({MPI_COMM_WORLD, info}) // pass the communicator and hints as usual
		);
		h5::ds_t ds = h5::create<double>(fd,"dataset"
				,h5::max_dims{size,nrows}, h5::chunk{2,10000} | h5::alloc_time_early );

		// ACTUAL WRITE MEASUREMENT
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			h5::write( ds, v, h5::current_dims{nrows,size},
				h5::offset{rank,0}, h5::count{1,nrows}, h5::collective );
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();
		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
		double MB_sec =  (vsize / 1e6) / running_time;

		// COLLECTING RESULTS: 
		std::vector<double> throughput(size);
		MPI_Gather(&MB_sec, 1, MPI_DOUBLE, throughput.data(), 1, MPI_DOUBLE, 0, comm);
		if( rank == 0){
			for( auto i : throughput) std::cout<< i <<" ";
			std::cout << "\nWRITE: " <<
				std::accumulate(throughput.begin(), throughput.end(), 0) <<" MB/s" <<std::endl;
		}
	}
	{ // READ
		std::vector<double> v(nrows);
		size_t vsize =v.size() * sizeof(double);
		auto fd = h5::open("collective.h5", H5F_ACC_RDWR,  h5::mpiio({MPI_COMM_WORLD, info}));
		auto ds = h5::open(fd, "/dataset");

		// ACTUAL WRITE MEASUREMENT
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			h5::read(ds, v.data(),  h5::offset{rank,0}, h5::count{1,nrows}, h5::collective);
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
		double MB_sec =  (vsize / 1e6) / running_time;

		// COLLECTING RESULTS: 
		std::vector<double> throughput(size);
		MPI_Gather(&MB_sec, 1, MPI_DOUBLE, throughput.data(), 1, MPI_DOUBLE, 0, comm);
		if( rank == 0){
			for( auto i : throughput) std::cout<< i <<" ";
			std::cout << "\nREAD: " <<
				std::accumulate(throughput.begin(), throughput.end(), 0) <<" MB/s" <<std::endl;
		}
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}
