#include <vector>
#include <armadillo>
#include <h5cpp/all>
#include <gperftools/profiler.h>


int main(int argc, char **argv) {
	arma::wall_clock timer;
	h5::fd_t fd = h5::create("stl.h5",H5F_ACC_TRUNC);
	hsize_t size = 10'000;
	hsize_t col = 100'000;
	h5::ds_t ds = h5::create<float>(fd,"/float", h5::current_dims{size,col}, h5::chunk{1,col} );

	std::vector<float> data( size*col );
	hsize_t data_size = data.size()*sizeof(float);

	ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
	{
		timer.tic();
		for(hsize_t i = 0; i<size; i++)
			h5::write<float>(ds, data, h5::offset{i,0}, h5::count{1,col} );
		auto running_time = timer.toc();
		std::cout << "number of seconds: " << running_time << " record per sec: " << size/running_time 
	 		<< " sustained throughput: " << (data_size/running_time) / 1'000'000 <<" Mbyte/sec"	<< std::endl;
	}
	ProfilerStop();
}


