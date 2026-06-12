
#ifndef __linux__
#  error "h5cpp SWMR is Linux-only in this release"
#endif

#include <iostream>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <h5cpp/all>


int main() {
    using namespace std::chrono_literals;
    const std::string container = "swmr-stream.h5", dataset = "stream";
    h5::mute();
    try {
        h5::fd_t fd = h5::open(container, H5F_ACC_RDONLY | H5F_ACC_SWMR_READ);
        h5::ds_t ds = h5::open(fd, dataset);

        for (int idle = 0; idle < 40; ++idle) {     // stop after ~4 s of no growth
            h5::refresh(ds);                                  // pull the latest flushed state
            auto data = h5::read<std::vector<int>>(ds);       // current contents of the dataset
            std::cout << "read " << data.size() << std::endl;
            std::this_thread::sleep_for(1s);
        }
    }catch(const std::exception& err) {
        std::cerr << err.what() <<std::endl;
    }
}
