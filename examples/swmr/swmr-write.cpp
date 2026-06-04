
#ifndef __linux__
#  error "h5cpp SWMR is Linux-only in this release"
#endif

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <thread>
#include <string>

#include <h5cpp/all>

std::atomic<bool> stop{false};
static void on_signal(int) {
    stop = true;
}

int main() {
    using namespace std::chrono_literals;
    constexpr int batch = 64, batches = 100;
    const std::string container = "swmr-stream.h5", dataset = "stream";
    h5::mute();
    std::signal(SIGINT,  on_signal);
    std::signal(SIGTERM, on_signal);

    try {
        // No-lock FAPL: lets the writer reattach (SWMR_WRITE) on a warm restart while
        // reader processes still hold the file. HDF5's file lock would otherwise block
        // the reopen until every reader closes (see H5Pset_file_locking).
        h5::fapl_t nolock{H5Pcreate(H5P_FILE_ACCESS)};
        H5Pset_file_locking(static_cast<hid_t>(nolock), false, true);

        h5::fd_t fd = std::filesystem::exists(container) ?
            h5::open(container, H5F_ACC_RDWR | H5F_ACC_SWMR_WRITE, nolock) : h5::create(container, H5F_ACC_RDWR | H5F_ACC_SWMR_WRITE);
        h5::ds_t ds = h5::exists(fd, dataset) ? h5::open(fd, dataset) : h5::create<std::uint64_t>(fd, dataset,
            h5::current_dims{0}, h5::max_dims{H5S_UNLIMITED}, h5::chunk{batch});
        
        h5::start_swmr_write(fd);                     // kick off SWMR
        uint64_t count = h5::get_extent(ds)[0];       // current length (rank-1)
        h5::pt_t pt(ds);                       //  
        for (int b = 0; b < batches && !stop; ++b) {  // flush in batches; stop cleanly on Ctrl-C
            for (int i = 0; i < batch; ++i)
                h5::append(pt, count++);
            h5::flush(pt);                          // visible to readers now
            std::cout << count << std::endl;
            std::this_thread::sleep_for(1s); // pace the stream
        }
        std::cout << "writer: done\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
