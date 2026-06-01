#include <h5cpp/core>
#include <h5cpp/io>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <iostream>

static std::vector<double> gen(size_t n, int seed){
    std::vector<double> v(n);
    for(size_t i=0;i<n;++i) v[i]= std::sin(0.0005*double(i+seed*1000));   // compressible
    return v;
}
int main(int argc, char** argv){
    int    producers = argc>1? std::atoi(argv[1]) : 1;
    size_t total     = argc>2? std::strtoull(argv[2],0,10) : 16ull*1024*1024;
    size_t chunk     = argc>3? std::strtoull(argv[3],0,10) : 8192;
    int    gz        = argc>4? std::atoi(argv[4]) : 4;
    int    pool      = argc>5? std::atoi(argv[5]) : 4;
    size_t per = total/producers;
    std::vector<std::vector<double>> data(producers);
    for(int k=0;k<producers;++k) data[k]=gen(per,k);

    const char* path="/tmp/append-bench.h5";
    std::remove(path);
    auto t0=std::chrono::steady_clock::now();
    {
        h5::fapl_t fapl = pool>0 ? h5::fapl_t{h5::threads{(unsigned)pool}} : h5::fapl_t{h5::default_fapl};
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC, h5::default_fcpl, fapl);
        std::vector<h5::pt_t> pts;
        for(int k=0;k<producers;++k){
            h5::ds_t ds = gz < 0
                ? h5::create<double>(fd,"pt"+std::to_string(k),
                    h5::current_dims_t{0}, h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{chunk})              // no filter
                : h5::create<double>(fd,"pt"+std::to_string(k),
                    h5::current_dims_t{0}, h5::max_dims_t{H5S_UNLIMITED}, h5::chunk{chunk}|h5::gzip{gz});
            pts.emplace_back(ds);
        }
        std::vector<std::thread> ths;
        for(int k=0;k<producers;++k)
            ths.emplace_back([&,k]{ for(size_t i=0;i<per;++i) h5::append(pts[k], data[k][i]); h5::flush(pts[k]); });
        for(auto&t:ths) t.join();
    }
    auto t1=std::chrono::steady_clock::now();
    double sec = std::chrono::duration<double>(t1-t0).count();
    double mb  = double(total*sizeof(double))/(1024.0*1024.0);
    std::printf("producers=%d pool=%d chunk=%zu gzip=%d  %7.3f s  %8.1f MB/s\n",
                producers, pool, chunk, gz, sec, mb/sec);
    std::remove(path);
    return 0;
}
