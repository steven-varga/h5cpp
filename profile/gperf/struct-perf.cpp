#include <vector>
#include <armadillo>
#include <gperftools/profiler.h>

#include <h5cpp/core>
	#include <tests/struct.h>
#include <h5cpp/io>

int main(int argc, char **argv) {
	arma::wall_clock timer;
	std::cout <<"struct size: " << sizeof( sn::struct_type ) << "\n\n";
	unsigned int cd_values[7];
	char *version, *date;

	h5::fd_t fd = h5::create("struct.h5",H5F_ACC_TRUNC);
	h5::pt_t pt = h5::create<sn::struct_type>(fd,"/2017-12-01.tick",
			h5::max_dims{H5S_UNLIMITED},
			h5::gzip{9} | h5::chunk{1024} );

	sn::struct_type data={0};
	long long size = 10'000'000ll;
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
		for(long long i=0; i<size; i++){ // literal needs c++14 !!!
			h5::append(pt, *p++ );
		}
		auto running_time = timer.toc();
		std::cout << size<< " number of seconds: " << running_time << " record per sec: " << size/running_time 
	 		<< " sustained throughput: " << ((size/running_time)*sizeof(sn::struct_type) ) / 1'000'000 <<" Mbyte/sec"	<< std::endl;
		std::cout << sizeof(sn::struct_type) <<"\n";
	}
	ProfilerStop();
	free(ptr);
}


