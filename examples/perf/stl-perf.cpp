#include <vector>
#include <armadillo>
#include <h5cpp>
#include <gperftools/profiler.h>


int main(int argc, char **argv) {
	arma::wall_clock timer;

	hid_t fd = h5::create("stl.h5");
	hsize_t size = 1'000'000;
	hsize_t col = 100;
	hid_t ds = h5::create<float>(fd,"/float",{size,col},{1024,100}, 0);
	std::vector<float> data( size*col );
	hsize_t data_size = data.size()*sizeof(float);

	timer.tic();

	ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
	{
		h5::write(ds, data,{0,0},{size,col});
		auto running_time = timer.toc();
		std::cout << "number of seconds: " << running_time << " record per sec: " << size/running_time 
	 		<< " sustained throughput: " << (data_size/running_time) / 1'000'000 <<" Mbyte/sec"	<< std::endl;

	}
	ProfilerStop();

	H5Dclose(ds);
	h5::close(fd);
}


