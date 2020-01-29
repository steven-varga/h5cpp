

#include <armadillo>
#include <Eigen/Dense>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <gperftools/profiler.h>
#include <mutex>

/**
 * bash :get prime numbers from range
 * factor {100..1000} | cut -d':' -f2 | tr ' ' \\n |sort -g|uniq
 */
template<class T> using Matrix   = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;


using pipeline_t = void* (*)(void* src, void* dst, size_t size);


#define h5cpp_outer( idx ) for( j##idx =0; j##idx < n##idx; j##idx += b##idx)
#define h5cpp_inner( idx ) for( i##idx = j##idx; i##idx < std::min(j##idx+b##idx,n##idx); i##idx++)
#define h5cpp_def( idx ) int i##idx=0, j##idx = 0, s##idx=0, n##idx=N[idx], b##idx=B[idx], r##idx=R[idx];

template <class T, size_t rank >
struct pipeline {

	pipeline(std::array<int,rank> dims, std::array<int,rank> block ) : n(1),tail(0),cache(12000){

		for(auto i:block ) n *= i;
		int i=0;
		for(; i<rank; i++ ){
			N[i] = dims[rank-i-1]; B[i] = block[rank-i-1];
		   	R[i] = N[i] % B[i];
		}
		for(; i<7; i++)
			N[i] = B[i] = 1, R[i] = 0;
		buf = static_cast<T*>( malloc(n*sizeof(T)) );
		dst0 = static_cast<T*>( malloc(cache) );
		dst1 = static_cast<T*>( malloc(cache) );
	}

	~pipeline(){
		free(buf); free(dst0); free(dst1);
	}

	bool block( T* ptr ){
		return true;
	}

	void tile( T* ptr ){
		h5cpp_def(0) h5cpp_def(1) h5cpp_def(2) h5cpp_def(3) h5cpp_def(4) h5cpp_def(5) h5cpp_def(6)

		h5cpp_outer( 6 ){ h5cpp_outer( 5 ){ h5cpp_outer( 4 ){ h5cpp_outer( 3 ){
		h5cpp_outer( 2 ){ h5cpp_outer( 1 ){ h5cpp_outer( 0 ){
			T* p = buf;

			// reset memory page only on the edges
			if( j0 + b0 > n0 || j1 + b1 > n1 || j2 + b2 > n2 ||
				j3 + b3 > n3 || j4 + b4 > n4 || j5 + b5 > n5 || j6 + b6 > n6 ) memset(p,0, n*sizeof(T));

				h5cpp_inner(6){	h5cpp_inner(5){ h5cpp_inner(4){
				h5cpp_inner(3){ h5cpp_inner(2){ h5cpp_inner(1){
				int offset = i6*n5*n4*n3*n2*n1*n0 + i5*n4*n3*n2*n1*n0 +
						i4*n3*n2*n1*n0 + i3*n2*n1*n0 + i2*n1*n0 + i1*n0 + j0;
				if( j0 != n0 - r0 )
					memcpy(p, ptr+offset, b0*sizeof(T) );
				else
					memcpy(p, ptr+offset, r0*sizeof(T) );
				p+=b0;
			}}}}}}
			std::cout << "-------------------------------------\n";
			forward( buf );
		}}}}}}}
	}
	void push( pipeline_t filter_  ){
		filter[tail++] = filter_;
	}
	void pop( pipeline_t filter_  ){
		tail++;
	}

	void forward( void* buf ){
		std::cout << " " << n << " --------------------- \n";
		// break up block into cache size
		for(int i=0; i<n; i+=cache ){
			void *in = static_cast<char*>(buf)+i, *out, *tmp;
			// handle the tail remainder if any
			int length = i + cache > n ? cache - i : cache;
			if( tail ){ // apply all filter chain in order
				in = filter[0](in, dst0, length);
				out = dst1;
				for(int j=1; j<tail; j++){
					tmp = filter[j](in, dst0, length);
					out = in; in = tmp;
				}
			}
		}
	}

	int N[7],B[7],R[7];
	pipeline_t filter[16];
	T* buf, *dst0,*dst1;
	int n,cache,tail;
};

void* filter_gzip( void* src, void* dst, size_t size ){
	memcpy(src,dst,size);
	return dst;
}
void* filter_add( void* src, void* dst, size_t size ){
	memcpy(src,dst,size);
	return dst;
}
void* filter_shuffle( void* src, void* dst, size_t size ){
	memcpy(src,dst,size);
	return dst;
}
void* filter_jpeg( void* src, void* dst, size_t size ){
	memcpy(src,dst,size);
	return dst;
}
void* filter_disperse( void* src, void* dst, size_t size ){
	memcpy(src,dst,size);
	return dst;
}

// 8*3 * 10^5 * 10^6 = 24*10^11 = 2.4 * 10^12 = 2.4TB 
int main(int argc, char **argv) {

	std::array<int,3> chunk{1,72,128};
	std::array<int,3>  dims{1000,720,1280};
	size_t size = 1; for( auto n : dims ) size*=n;
	std::cout << size / 1e6 << "MB\n";
	unsigned * ptr = static_cast<unsigned*>( malloc( size * sizeof(unsigned) ) );
	std::iota(ptr, ptr+size, 0);
	std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
		pipeline<unsigned> filter(3dims,chunk);
	std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();

	filter.push( filter_gzip );
	filter.push( filter_add );
	filter.push( filter_shuffle );
	filter.push( filter_jpeg );

	ProfilerStart( (std::string(argv[0]) + std::string(".prof")).data() );

	filter.tile(ptr);
	//	H5Dwrite_chunk(ds, H5P_DEFAULT, filter.mask, filter.offset, filter.size, filter.buff);
	//ProfilerStop();

	double running_time = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() ;
	std::cout << running_time <<" throughout: " << (size / 1e6) / running_time <<" MB/s" <<std::endl;

	free(ptr);
	return 0;
}

