
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

	unsigned slices = 200, nrows=720, ncols=1280;

	std::array<unsigned,3> chunk{1,nrows,ncols};
	std::array<unsigned,3>  dims{slices,nrows,ncols};
	h5::count_t count( dims );
	h5::offset_t offset{0,0,0};
	size_t size = 1; for( auto n : dims ) size*=n;
	size *= sizeof(unsigned);
	std::cout <<"\n------------------------ THROUGHPUT TEST ----------------------------\n";
	std::cout <<"MEMORY SIZE of DATASET:" <<  size / 1e6 << "MB\n";
	unsigned * ptr = static_cast<unsigned*>( malloc( size ) );
	std::iota(ptr, ptr+slices*nrows*ncols, 0);
	//std::generate(ptr, ptr+slices*nrows*ncols, [&dist,&gen]{ return dist(gen); } );
	arma::Cube<unsigned> cube(ptr,ncols,nrows,slices, false, true);

	h5::fd_t fd = h5::create("example.h5",H5F_ACC_TRUNC);
	h5::current_dims current_dims = static_cast<h5::current_dims>( dims );
	h5::max_dims max_dims = static_cast<h5::max_dims>( dims );
	max_dims[0] = H5S_UNLIMITED;

	h5::ds_t ds = h5::create<unsigned>(fd,"huge dataset"
				,current_dims,max_dims, h5::chunk{chunk}  | h5::fill_value<unsigned>{0}, h5::high_throughput );
	h5::mute();
	std::cout << "DATASET: " << dims[0]<<"x"<<dims[1]<<"x"<<dims[2] << " CHUNK: "<<chunk[0]<<"x"<<chunk[1]<<"x"<<chunk[2]<<"\n";
	{   std::cout << "HDF5 1.10.4 PIPELINE:\n";
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
			h5::write<unsigned>(ds, ptr, h5::count{slices,nrows,ncols});
			ProfilerStop();
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughout: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
	}
/*
	{   std::cout << "PROPOSED PIPELINE: BLOCK\n";
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
				h5::impl::write_chunk( ds, count, offset, ptr );
			ProfilerStop();
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughout: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
	}

	{   std::cout << "PROPOSED PIPELINE: FOR LOOP 1-n\n";
		h5::impl::pipeline filter( ds );
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
		//	ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
			h5::count_t count( dims ); count[0] = 1;
			h5::offset_t offset{0,0,0};

			for(int i=0; i<slices; i++ ){
				offset[0] = i;
				filter.apply( offset.begin(), count.begin(), ptr );
			}
		//	ProfilerStop();
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughout: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
	}

	{   std::cout << "RAW  BINARY \n";
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
		//	ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
			cube.save("cube_raw",arma::raw_binary);
		//	ProfilerStop();
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughout: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
	}
	
	{   std::cout << "ARMA BINARY \n";
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			cube.save("cube_arma",arma::arma_binary);
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << running_time <<" throughout: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;
	}
*/
	free(ptr);
	return 0;
}

//2^22 : 1195.36 966.245 763.869 516.985 460.335 390.358 353.78  298.645 247.94  243.39 195.686 214.894
//2^25 :
//
// 0.000012 
