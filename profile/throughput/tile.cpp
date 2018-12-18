
#include <armadillo>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <gperftools/profiler.h>
#include <mutex>
#include <h5cpp/all>
#include <random>
#include <cmath>
/**
 * bash :get prime numbers from range
 * factor {100..1000} | cut -d':' -f2 | tr ' ' \\n |sort -g|uniq
 */
// 36'864'565'632

//H5Z_filter_t H5Pget_filter2( hid_t plist_id, unsigned idx, unsigned int *flags, size_t *cd_nelmts, unsigned cd_values[], size_t namelen, char name[], unsigned *filter_config )

// 8*3 * 10^5 * 10^6 = 24*10^11 = 2.4 * 10^12 = 2.4TB 
int main(int argc, char **argv) {
	std::random_device rd;
	std::default_random_engine gen(rd());
    std::uniform_int_distribution<unsigned> dist(1, 200);

	unsigned slices = 500, nrows=720, ncols=1280;

	std::array<unsigned,3> chunk{1,72,ncols};
	std::array<unsigned,3>  dims{slices,nrows,ncols};
	h5::count_t count( dims );
	h5::offset_t offset{0,0,0};
	size_t size = 1; for( auto n : dims ) size*=n;
	size *= sizeof(unsigned);

	std::cout <<"\n------------------------ THROUGHPUT TEST ----------------------------\n";
	std::cout <<"MEMORY SIZE of DATASET:" <<  size / 1e6 << "MB\n";
	unsigned * ptr_w = static_cast<unsigned*>( malloc( size ) );
	unsigned * ptr_r = static_cast<unsigned*>( malloc( size ) );
	std::iota(ptr_w, ptr_w+slices*nrows*ncols, 0);
	//std::generate(ptr_w, ptr_w+slices*nrows*ncols, [&dist,&gen]{ return dist(gen); } );
	arma::Cube<unsigned> cube(ptr_w,ncols,nrows,slices, false, true);

	h5::fd_t fd = h5::create("example.h5",H5F_ACC_TRUNC);
	h5::current_dims current_dims = static_cast<h5::current_dims>( dims );
	h5::max_dims max_dims = static_cast<h5::max_dims>( dims );
	max_dims[0] = H5S_UNLIMITED;
	std::cout << "DATASET: " << dims[0]<<"x"<<dims[1]<<"x"<<dims[2] << " CHUNK: "<<chunk[0]<<"x"<<chunk[1]<<"x"<<chunk[2]<<"\n";

	// CREATE dataset
	{
	h5::ds_t ds = h5::create<unsigned>(fd,"movie"
				,current_dims,max_dims, h5::chunk{chunk}  | h5::fill_value<unsigned>{0} | h5::fill_time{H5D_FILL_TIME_NEVER} );
	}

	{   std::cout << "HDF5 1.10.4 CAPI PIPELINE:\n";
		h5::ds_t ds = h5::open(fd,"movie");
		{
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			//ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
			h5::write<unsigned>(ds, ptr_w, h5::count{slices,nrows,ncols});
			//ProfilerStop();
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();
		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughput: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
		}
		{
		std::cout<<"READ:\n";
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			//ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
			h5::read<unsigned>(ds, ptr_r, h5::count{slices,nrows,ncols});
			//ProfilerStop();
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();
		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughput: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
		std::cout <<"< write - read : " << (std::memcmp(ptr_w, ptr_r, size) == 0 ? " MATCH " : " !!!MISMATCH!!!") <<">\n";
		}
	}
	{   std::cout << "HDF5 1.10.4 H5CPP HIGH THROUGHPUT PIPELINE:\n";
		h5::ds_t ds = h5::open(fd,"movie", h5::high_throughput);
		{
			std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			//ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
			h5::write<unsigned>(ds, ptr_w, h5::count{slices,nrows,ncols});
			//ProfilerStop();
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughput: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
		}
		{
			std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			//ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
			h5::read<unsigned>(ds, ptr_r, h5::count{slices,nrows,ncols});
			//ProfilerStop();
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughput: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
		std::cout <<"< write - read : " << (std::memcmp(ptr_w, ptr_r, size) == 0 ? " MATCH " : " !!!MISMATCH!!!") <<">\n";
		}

	}

	{   std::cout << "HDF5 1.10.4 H5CPP APPEND: scalar values  directly into chunk buffer\n";
		{
		h5::pt_t pt = h5::create<unsigned>(fd,"append scalar"
				,h5::max_dims{H5S_UNLIMITED,nrows,ncols}, h5::chunk{1,nrows,ncols},  h5::high_throughput );
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			for( int i=0; i < slices * nrows * ncols; i++)
				h5::append(pt, ptr_w[i] );
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughput: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
		}
		{
			h5::ds_t ds = h5::open(fd,"append scalar", h5::high_throughput);
			std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			//ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
			h5::read<unsigned>(ds, ptr_r, h5::count{slices,nrows,ncols});
			//ProfilerStop();
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughput: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
		std::cout <<"< write - read : " << (std::memcmp(ptr_w, ptr_r, size) == 0 ? " MATCH " : " !!!MISMATCH!!!") <<">\n";
		}
	}
	{   std::cout << "HDF5 1.10.4 H5CPP APPEND: objects with matching chunk size, data directly written from object memory\n";
		{
		h5::pt_t pt = h5::create<unsigned>(fd,"append matrix"
				,h5::max_dims{H5S_UNLIMITED,nrows,ncols}, h5::chunk{1,nrows,ncols}  | h5::fill_value<unsigned>{0}, h5::high_throughput );
		// use part of the same memory, for details see armadillo advanced constructors
		arma::Mat<unsigned> mat(ptr_w,ncols,nrows, false, true);
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			for( int i=0; i < slices; i++)
				h5::append(pt, mat );
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughput: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
		}
		{
			h5::ds_t ds = h5::open(fd,"append matrix",h5::high_throughput);
			std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			//ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
			h5::read<unsigned>(ds, ptr_r, h5::count{slices,nrows,ncols});
			//ProfilerStop();
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughput: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
		std::cout <<"< write - read : " << (std::memcmp(ptr_w, ptr_r, size) == 0 ? " MATCH " : " !!!MISMATCH!!!") <<">\n";
		}
	}
	{   std::cout << "RAW  BINARY \n";
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
		//	ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
			cube.save("cube_raw",arma::raw_binary);
		//	ProfilerStop();
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughput: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
	}

	{   std::cout << "ARMA BINARY \n";
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			cube.save("cube_arma",arma::arma_binary);
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughput: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
	}
	free(ptr_w); free(ptr_r);
	return 0;
}

//2^22 : 1195.36 966.245 763.869 516.985 460.335 390.358 353.78  298.645 247.94  243.39 195.686 214.894
//2^25 :
//
// 0.000012 
