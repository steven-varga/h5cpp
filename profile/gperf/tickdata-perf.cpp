#include <vector>
#include <chrono>
#include <gperftools/profiler.h>

#include <h5cpp/core>

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

namespace h5{ 
	// specialize template and create a compaund data type
	template<> hid_t inline register_struct<sn::tick>(){
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
}
H5CPP_REGISTER_STRUCT(sn::tick);
#include <h5cpp/io>


int main(int argc, char **argv) {
try {
	h5::fd_t fd = h5::create("tick.h5",H5F_ACC_TRUNC );

	//using chunk_cache = impl::fapl_call< impl::fapl_args<hid_t,size_t, size_t, double>,H5Pset_chunk_cache>;
	h5::pt_t pt = h5::create<sn::tick>(fd,"/2017-12-01.tick",
			h5::max_dims{H5S_UNLIMITED},
		   	h5::chunk{1024*1024}, h5::chunk_cache({521,512,.750})  );
	sn::tick data={0};

	ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );
	{
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
		long long size = 2'00'000'000ll;
//		long long size = 2'000'000ll;
		for(long long i=0; i<size; i++){ // literal needs c++14 !!!
			data.stock = i;
			h5::append(pt, data );
		}
		std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();
		double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
		std::cout << "record size: " << sizeof(sn::Tick) << "\n";
		std::cout << "number of mu seconds: " << running_time << " record per sec: " << size/running_time
	 		<< " sustained throughput: " << ((size/running_time)*sizeof(sn::tick) ) / 1'000'000 <<" Mbyte/sec"	<< std::endl;
		std::cout << "transferred data: " << 1e-6 * size * sizeof(sn::tick) << "Mbyte\n";

	}
	ProfilerStop();

	} catch ( const h5::error::any& e ){
		std::cerr << "ERROR:" << e.what() <<"\n";
	}
}
/*
	    original        compressed:   ratio:    time:       running time:
gzip   12'779'532'000   450'486'002   28.36     9m13.689s   23.322


internal compress2 reported ratio: 28


number of mu seconds: 540.075 record per sec: 370319 sustained throughput: 23.7004 Mbyte/sec
transferred data: 12800Mbyte
PROFILE: interrupts/evictions/bytes = 54006/6236/641416

real	9m0.086s
user	8m59.844s
sys	0m0.248s
-rw-r--r-- 1 steven steven 828237764 Nov  1 15:23 tick.h5 chunk:   32  50MB/sec
-rw-r--r-- 1 steven steven 638016410 Nov  1 15:17 tick.h5 chunk:   64  71MB/sec
-rw-r--r-- 1 steven steven 548228294 Nov  1 15:11 tick.h5 chunk:  128  80MB/sec
-rw-r--r-- 1 steven steven 516510685 Nov  1 15:35 tick.h5 chunk:  256  53MB/sec
-rw-r--r-- 1 steven steven 482170550 Nov  1 15:43 tick.h5 chunk:  512  39MB/sec
-rw-r--r-- 1 steven steven 466494793 Nov  1 15:04 tick.h5 chunk: 1024  29MB/sec
-rw-r--r-- 1 steven steven 450903706 Oct 31 20:58 tick.h5
-rw-r--r-- 1 steven steven 450486002 Oct 31 19:43 tick.h5.gz

-rw-r--r-- 1 steven steven 12807279600 Nov  1 14:55 tick.h5


*/
