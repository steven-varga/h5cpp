#include <vector>
#include <armadillo>
#include <h5cpp>
#include <gperftools/profiler.h>


namespace SomeNameSpace {
	typedef struct Tick {
		unsigned long stock;
		double time_stamp;
		float ask_price;
		unsigned long ask_size;
		float bid_price;
		unsigned long bid_size;
		float last_price;
		unsigned long last_size;
	} tick;
}

namespace sn = SomeNameSpace;

namespace h5{ namespace utils { 
	// specialize template and create a compaund data type
	template<> hid_t h5type<sn::tick>(){
			hid_t type = H5Tcreate(H5T_COMPOUND, sizeof (sn::tick));
			// layout
			H5Tinsert(type, "time_stamp", 	HOFFSET(sn::tick, time_stamp),  H5T_NATIVE_DOUBLE);
			H5Tinsert(type, "stock", 		HOFFSET(sn::tick, stock),       H5T_NATIVE_UINT16);
			H5Tinsert(type, "ask_price", 	HOFFSET(sn::tick, ask_price),   H5T_NATIVE_FLOAT);
			H5Tinsert(type, "bid_price", 	HOFFSET(sn::tick, bid_price),   H5T_NATIVE_FLOAT);
			H5Tinsert(type, "last_price", 	HOFFSET(sn::tick, last_price),  H5T_NATIVE_FLOAT);
			H5Tinsert(type, "ask_size", 	HOFFSET(sn::tick, ask_size),  H5T_NATIVE_UINT32);
			H5Tinsert(type, "bid_size", 	HOFFSET(sn::tick, bid_size),  H5T_NATIVE_UINT32);
			H5Tinsert(type, "last_size", 	HOFFSET(sn::tick, last_size), H5T_NATIVE_UINT32);
            return type;
	}
}}
int main(int argc, char **argv) {
	arma::wall_clock timer;

	hid_t fd = h5::create("tick.h5");

	hid_t ds = h5::create<sn::tick>(fd,"/2017-12-01.tick",{H5S_UNLIMITED},{1024}, 0);
	sn::tick data={0};
	long long size = 100'000'000ll;
	sn::tick* ptr = static_cast<sn::tick*>( calloc(size, sizeof(sn::tick)) );
	sn::tick* p = ptr;
	// fill memory with random data
	for(  hsize_t i=0; i<size; i++){ 
			p->stock = i;
			p->time_stamp = std::rand();
			p->ask_price = std::rand(); p->bid_price = std::rand(); p->last_price = std::rand();
			p->ask_size = 2000*std::rand(); p->bid_size = 100*std::rand(); p->last_size = 1000*std::rand();
			p++;
	}

	timer.tic();
	ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
	{
		p = ptr;
		auto ctx = h5::context<sn::tick>(ds);
		for(long long i=0; i<size; i++){ // literal needs c++14 !!!
			h5::append(ctx, *p++ );
		}
		auto running_time = timer.toc();
		std::cout << size<< " number of seconds: " << running_time << " record per sec: " << size/running_time 
	 		<< " sustained throughput: " << ((size/running_time)*sizeof(sn::tick) ) / 1'000'000 <<" Mbyte/sec"	<< std::endl;
		std::cout << sizeof(sn::tick) <<"\n";
	}
	ProfilerStop();
	free(ptr);
	H5Dclose(ds);
	h5::close(fd);
}


