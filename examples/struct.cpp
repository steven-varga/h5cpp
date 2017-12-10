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
	// specialize template and create a compound data type
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
	timer.tic();

	ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
	{ // APPEND
		long long size = 20'000'000ll;
		auto ctx = h5::context<sn::tick>(ds);
		for(long long i=0; i<size; i++){ // literal needs c++14 !!!
			data.stock = i;
			h5::append(ctx, data );
		}
		auto running_time = timer.toc();
		std::cout << "number of seconds: " << running_time << " record per sec: " << size/running_time 
	 		<< " sustained throughput: " << ((size/running_time)*sizeof(sn::tick) ) / 1'000'000 <<" Mbyte/sec"	<< std::endl;

	}
	ProfilerStop();

	{ //READ
		//auto data = read<std::vector<sn::tick> >( ds );
	}

	H5Dclose(ds);
	h5::close(fd);
}


