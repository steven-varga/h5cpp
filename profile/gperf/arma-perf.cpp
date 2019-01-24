#include <vector>
#include <chrono>
#include <gperftools/profiler.h>
#include <armadillo>
#include <h5cpp/all>


int main(int argc, char **argv) {

		h5::fd_t fd = h5::create("arma.h5",H5F_ACC_TRUNC, 	h5::file_space_page_size{8*4096} | h5::userblock{512} );

	const size_t nrows  = 1280;
	const size_t ncols  = 720;
	const size_t nchunk = 10;

	ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
	{
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
		size_t nrows = 3, ncols=500, nframes=1000000;
		h5::pt_t pt = h5::create<double>(fd, "stream of matrices", // 1280 x 720
				h5::max_dims{H5S_UNLIMITED,nrows,500},
				h5::chunk{2,nrows,ncols}  // chunk size controls h5::append internal cache size
			   	| h5::gzip{9} );
		arma::mat M(nrows,ncols);

		for( int i = 1; i < nframes; i++)
			h5::append( pt, M);

		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();
		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
		std::cout << "number of mu seconds: " << running_time << " record per sec: " << nframes/running_time
	 		<< " sustained throughput: " << ((nframes/running_time)*sizeof( double )*nrows*ncols ) / 1'000'000 <<" Mbyte/sec"	<< std::endl;

	}
	ProfilerStop();
}


