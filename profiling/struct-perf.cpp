#include <vector>
#include <armadillo>
#include <gperftools/profiler.h>

#include <h5cpp/core>
	#include <tests/struct.h>
#include <h5cpp/io>

int main(int argc, char **argv) {
	arma::wall_clock timer;

	hid_t fd = h5::create("struct.h5");

	hid_t ds = h5::create<sn::struct_type>(fd,"/2017-12-01.struct",{H5S_UNLIMITED},{1024}, 0);
	sn::struct_type data={0};
	long long size = 100'000'000ll;
	sn::struct_type* ptr = static_cast<sn::struct_type*>( calloc(size, sizeof(sn::struct_type)) );
	sn::struct_type* p = ptr;
	// fill memory with random data
	for(  hsize_t i=0; i<size; i++){ 
			p->field1 = i;
			p->field2 = std::rand();
			p->field3 = std::rand(); p->field4 = std::rand(); p->field5 = std::rand();
			p->field6 = std::rand(); p->field7 = std::rand(); p->field8 = std::rand();
			p->field9 = std::rand(); 
			p++;
	}

	timer.tic();
	ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
	{
		p = ptr;
		auto ctx = h5::context<sn::struct_type>(ds);
		for(long long i=0; i<size; i++){ // literal needs c++14 !!!
			h5::append(ctx, *p++ );
		}
		auto running_time = timer.toc();
		std::cout << size<< " number of seconds: " << running_time << " record per sec: " << size/running_time 
	 		<< " sustained throughput: " << ((size/running_time)*sizeof(sn::struct_type) ) / 1'000'000 <<" Mbyte/sec"	<< std::endl;
		std::cout << sizeof(sn::struct_type) <<"\n";
	}
	ProfilerStop();
	free(ptr);
	H5Dclose(ds);
	h5::close(fd);
}


