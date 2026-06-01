// Isolated cost of the thread-safe dispatch machinery: full worker_pool submit
// path vs the raw queue primitives (the live mutex+std::queue vs the dead Vyukov MPSC).
#include <h5cpp/core>
#include <h5cpp/io>
#include <chrono>
#include <cstdio>
#include <queue>
#include <mutex>
#include <functional>
#include <atomic>
using clk = std::chrono::steady_clock;
static double nsop(size_t M, clk::time_point a, clk::time_point b){
    return std::chrono::duration<double>(b-a).count()*1e9/double(M);
}
int main(){
    const size_t M = 2'000'000;
    std::atomic<size_t> sink{0};

    // 1. inline call baseline
    { auto t0=clk::now();
      for(size_t i=0;i<M;++i) sink.fetch_add(1,std::memory_order_relaxed);
      printf("inline no-op                         %6.1f ns/op\n", nsop(M,t0,clk::now())); }

    // 2. FULL worker_pool submit path (1 worker): packaged_task alloc + future + mutex + doorbell + thread hop
    { h5::impl::worker_pool_t pool(1);
      auto t0=clk::now();
      for(size_t i=0;i<M;++i) pool.submit([&]{ sink.fetch_add(1,std::memory_order_relaxed); });
      pool.wait_idle();
      printf("worker_pool::submit + run (1 worker)  %6.1f ns/task\n", nsop(M,t0,clk::now())); }

    // 3. live primitive: mutex + std::queue<std::function> push+pop (single thread, uncontended)
    { std::mutex m; std::queue<std::function<void()>> q;
      auto t0=clk::now();
      for(size_t i=0;i<M;++i){
        { std::lock_guard<std::mutex> lk(m); q.emplace([&]{ sink.fetch_add(1,std::memory_order_relaxed); }); }
        std::function<void()> f;
        { std::lock_guard<std::mutex> lk(m); f=std::move(q.front()); q.pop(); }
        f();
      }
      printf("mutex + std::queue push+pop+run       %6.1f ns/op\n", nsop(M,t0,clk::now())); }

    // 4. dead primitive: Vyukov bounded MPSC push+pop (single thread)
    { h5::impl::mpsc_queue_t<int,1024> q;
      auto t0=clk::now();
      for(size_t i=0;i<M;++i){ q.push((int)i); int v=0; q.pop(v); sink.fetch_add((size_t)(v>=0)); }
      printf("Vyukov mpsc_queue_t push+pop          %6.1f ns/op\n", nsop(M,t0,clk::now())); }

    printf("(sink=%zu)\n", sink.load());
    return 0;
}
