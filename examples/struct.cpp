#include <vector>
#include <armadillo>
#include <gperftools/profiler.h>
#include <h5cpp/core>
	#include <tests/struct.h>  //< make sure this precedes (create|read|write|append).hpp
#include <h5cpp/io>

int main(int argc, char **argv) {
	arma::wall_clock timer;

	hid_t fd = h5::create("struct.h5");

	hid_t ds = h5::create<sn::struct_type>(fd,"/2017-12-01.struct",{H5S_UNLIMITED},{1024}, 0);
	sn::struct_type data={0};
	timer.tic();

	ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
	{ // APPEND
		long long size = 20'000'000ll;
		auto ctx = h5::context<sn::struct_type>(ds);
		for(long long i=0; i<size; i++){ // literal needs c++14 !!!
			data.field1 = i;
			h5::append(ctx, data );
		}
		auto running_time = timer.toc();
		std::cout << "number of seconds: " << running_time << " record per sec: " << size/running_time 
	 		<< " sustained throughput: " << ((size/running_time)*sizeof(sn::struct_type) ) / 1'000'000 <<" Mbyte/sec"	<< std::endl;

	}
	ProfilerStop();
	{ //TODO: able to read dataset in one shot: forward declaration???
		// FIXME: std::vector<sn::tick> tick	= h5::read<std::vector<sn::tick>>(fd, "/2017-12-01.tick");
		// FIXME: std::vector<sn::tick> tick	= h5::read<std::vector<sn::tick>>(fd, "/2017-12-01.tick",{0},{100});
	//	std::vector<sn::struct_type> tick	= h5::read<std::vector<sn::struct_type>>(ds);
		

		//typedef BaseType = h5::utils::base<std::vector<sn::tick>>::type;
	}


	H5Dclose(ds);
	h5::close(fd);
}


