// Pure codec microbench: compress one 64KB chunk repeatedly, no HDF5.
// Isolates libdeflate vs zlib, and alloc-per-call vs alloc-once.
#include <libdeflate.h>
#include <zlib.h>
#include <vector>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstring>
static double mbps(size_t bytes, double sec){ return double(bytes)/(1024.0*1024.0)/sec; }
int main(){
    const size_t CHUNK = 8192*sizeof(double);   // 64 KB, same as bench chunk
    const int    ITERS = 4096;                  // 4096*64KB = 256 MB total, matches append bench
    const int    LVL   = 4;
    // low-compressibility (the append-bench data)
    std::vector<unsigned char> sinbuf(CHUNK);
    { std::vector<double> d(8192); for(int i=0;i<8192;++i) d[i]=std::sin(0.0005*double(i)); std::memcpy(sinbuf.data(),d.data(),CHUNK); }
    // high-compressibility (mostly-zero / repetitive)
    std::vector<unsigned char> easybuf(CHUNK);
    { std::vector<double> d(8192); for(int i=0;i<8192;++i) d[i]=double(i%17); std::memcpy(easybuf.data(),d.data(),CHUNK); }

    std::vector<unsigned char> out(CHUNK*2);
    size_t totalbytes = CHUNK*(size_t)ITERS;

    auto run=[&](const char* name, auto fn){
        // warm
        fn(sinbuf);
        auto t0=std::chrono::steady_clock::now();
        size_t lastc=0; for(int i=0;i<ITERS;++i) lastc=fn(sinbuf);
        auto t1=std::chrono::steady_clock::now();
        double s1=std::chrono::duration<double>(t1-t0).count();
        auto t2=std::chrono::steady_clock::now();
        for(int i=0;i<ITERS;++i) lastc=fn(easybuf);
        auto t3=std::chrono::steady_clock::now();
        double s2=std::chrono::duration<double>(t3-t2).count();
        std::printf("%-34s  sin: %7.1f MB/s (ratio %.2f)   easy: %7.1f MB/s\n",
                    name, mbps(totalbytes,s1), double(CHUNK)/double(lastc?lastc:1), mbps(totalbytes,s2));
    };

    // A. libdeflate, alloc compressor PER CALL  (== h5cpp zlib_deflate_encode)
    run("libdeflate L4  alloc-per-call", [&](std::vector<unsigned char>& in)->size_t{
        libdeflate_compressor* c=libdeflate_alloc_compressor(LVL);
        size_t n=libdeflate_zlib_compress(c,in.data(),in.size(),out.data(),out.size());
        libdeflate_free_compressor(c); return n;
    });
    // B. libdeflate, alloc ONCE
    { libdeflate_compressor* c=libdeflate_alloc_compressor(LVL);
      run("libdeflate L4  alloc-once", [&](std::vector<unsigned char>& in)->size_t{
        return libdeflate_zlib_compress(c,in.data(),in.size(),out.data(),out.size());
      });
      libdeflate_free_compressor(c);
    }
    // C. zlib compress2  (allocs internally per call) == h5cpp non-libdeflate fallback
    run("zlib       L4  compress2(per-call)", [&](std::vector<unsigned char>& in)->size_t{
        uLongf n=out.size();
        compress2(out.data(),&n,in.data(),in.size(),LVL); return n;
    });
    // D. zlib deflate, reused stream
    run("zlib       L4  reused-stream", [&](std::vector<unsigned char>& in)->size_t{
        z_stream zs; std::memset(&zs,0,sizeof zs);
        deflateInit(&zs,LVL);
        zs.next_in=in.data(); zs.avail_in=in.size();
        zs.next_out=out.data(); zs.avail_out=out.size();
        deflate(&zs,Z_FINISH); size_t n=out.size()-zs.avail_out;
        deflateEnd(&zs); return n;
    });
    return 0;
}
