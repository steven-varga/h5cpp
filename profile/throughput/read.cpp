
#include <armadillo>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <gperftools/profiler.h>
#include <mutex>
#include <h5cpp/all>
#include <random>
#include <cmath>

// 8*3 * 10^5 * 10^6 = 24*10^11 = 2.4 * 10^12 = 2.4TB 
int main(int argc, char **argv) {
	std::random_device rd;
	std::default_random_engine gen(rd());
    std::uniform_int_distribution<unsigned> dist(1, 200);

	unsigned slices = 500, nrows=720, ncols=1280;

	std::array<unsigned,3> chunk{1,720,ncols};
	std::array<unsigned,3>  dims{slices,nrows,ncols};
	h5::count_t count( dims );
	h5::offset_t offset{0,0,0};
	size_t size = 1; for( auto n : dims ) size*=n;
	size *= sizeof(unsigned);

	unsigned * ptr_w = static_cast<unsigned*>( malloc( size ) );
	unsigned * ptr_r = static_cast<unsigned*>( malloc( size ) );
	arma::Cube<unsigned> cube(ptr_w,ncols,nrows,slices, false, true);

	h5::fd_t fd = h5::open("example.h5",H5F_ACC_RDWR);
	h5::current_dims current_dims = static_cast<h5::current_dims>( dims );
	h5::max_dims max_dims = static_cast<h5::max_dims>( dims );
	max_dims[0] = H5S_UNLIMITED;

	{
		h5::ds_t ds = h5::open(fd,"movie");
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			h5::read<unsigned>(ds, ptr_r, h5::count{slices,nrows,ncols});
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();
		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
		std::cout << (size / 1e6) / running_time <<"\t";
	}

	{
		h5::ds_t ds = h5::open(fd,"movie", h5::high_throughput);
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			h5::read<unsigned>(ds, ptr_r, h5::count{slices,nrows,ncols});
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();
		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << (size / 1e6) / running_time <<"\t";
	}

	{
		h5::ds_t ds = h5::open(fd,"append scalar", h5::high_throughput);
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			h5::read<unsigned>(ds, ptr_r, h5::count{slices,nrows,ncols});
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();
		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << (size / 1e6) / running_time  <<"\t";
	}
	{
		h5::ds_t ds = h5::open(fd,"append matrix",h5::high_throughput);
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			h5::read<unsigned>(ds, ptr_r, h5::count{slices,nrows,ncols});
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();
		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << (size / 1e6) / running_time <<std::endl;
	}

	free(ptr_w); free(ptr_r);
	return 0;
}

