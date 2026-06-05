
#ifndef __linux__
#  error "h5cpp SWMR is Linux-only in this release"
#endif

#include <h5cpp/all>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <ranges>
#include <thread>


int main() {
    using namespace std::chrono_literals;
    const std::string container = "swmr-stream.h5", dataset = "stream";
    h5::mute();
    try {
        h5::fd_t fd = h5::open(container, H5F_ACC_RDONLY | H5F_ACC_SWMR_READ);
        h5::ds_t ds = h5::open(fd, dataset);

        std::size_t seen = 0;
        for (int idle = 0; idle < 40; ++idle) {     // stop after ~4 s of no growth
            h5::refresh(ds);                        // pull the latest flushed state FIRST
            auto snap = h5::view<std::uint64_t>(ds);  // matches the writer's std::uint64_t stream
            std::size_t fresh = 0, last = 0;
            for (std::uint64_t v : snap | std::views::drop(seen))   // stream only the new tail
                last = v, ++fresh, ++seen;
            std::cout << "read " << seen << std::endl;
            std::this_thread::sleep_for(1s);
        }
        std::cout << "view-reader: stopped after " << seen << " samples\n";
    } catch(const std::exception& err) {
        std::cerr << err.what() <<std::endl;
    }
}
