// Pure HDF5 C-API baseline: stock deflate (HDF5's own zlib) filter pipeline,
// single thread.  Mirrors the h5::append workload — unlimited 1-D dataset,
// chunk-by-chunk extend + hyperslab write.
#include <hdf5.h>
#include <vector>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
int main(int argc, char** argv){
    size_t total = argc>1? std::strtoull(argv[1],0,10) : 33554432ull; // 32Mi doubles = 256MB
    hsize_t chunk = argc>2? std::strtoull(argv[2],0,10) : 8192;
    int gz = argc>3? std::atoi(argv[3]) : 4;
    int bulk = argc>4? std::atoi(argv[4]) : 0;  // 1 = one H5Dwrite, 0 = stream per chunk

    std::vector<double> data(total);
    for(size_t i=0;i<total;++i) data[i]=std::sin(0.0005*double(i));   // same generator as append bench
    const char* path="/tmp/capi-gzip.h5";
    std::remove(path);

    auto t0=std::chrono::steady_clock::now();
    hid_t fid = H5Fcreate(path,H5F_ACC_TRUNC,H5P_DEFAULT,H5P_DEFAULT);
    hsize_t dims = bulk? (hsize_t)total : 0, maxdims = H5S_UNLIMITED;
    hid_t space = H5Screate_simple(1,&dims,&maxdims);
    hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(dcpl,1,&chunk);
    H5Pset_deflate(dcpl,(unsigned)gz);
    hid_t ds = H5Dcreate2(fid,"data",H5T_NATIVE_DOUBLE,space,H5P_DEFAULT,dcpl,H5P_DEFAULT);

    if(bulk){
        H5Dwrite(ds,H5T_NATIVE_DOUBLE,H5S_ALL,H5S_ALL,H5P_DEFAULT,data.data());
    } else {
        hsize_t cur=0;
        for(size_t off=0; off<total; off+=chunk){
            hsize_t n = std::min((size_t)chunk, total-off);
            cur += n; H5Dset_extent(ds,&cur);
            hid_t fspace = H5Dget_space(ds);
            hsize_t start=off, count=n;
            H5Sselect_hyperslab(fspace,H5S_SELECT_SET,&start,NULL,&count,NULL);
            hid_t mspace = H5Screate_simple(1,&count,NULL);
            H5Dwrite(ds,H5T_NATIVE_DOUBLE,mspace,fspace,H5P_DEFAULT,data.data()+off);
            H5Sclose(mspace); H5Sclose(fspace);
        }
    }
    H5Dclose(ds);H5Pclose(dcpl);H5Sclose(space);H5Fclose(fid);
    auto t1=std::chrono::steady_clock::now();
    double sec=std::chrono::duration<double>(t1-t0).count();
    double mb=double(total*sizeof(double))/(1024.0*1024.0);
    std::printf("CAPI %-6s gzip=%d chunk=%llu  %7.3f s  %8.1f MB/s\n",
                bulk?"bulk":"stream", gz, (unsigned long long)chunk, sec, mb/sec);
    std::remove(path);
    return 0;
}
