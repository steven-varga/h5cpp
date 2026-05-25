// SPDX-License-Identifier: MIT
// Copyright (c) 2025-2026 Varga Labs, Toronto, ON, Canada.
//
// Bench: h5::high_throughput direct-chunk vs standard HDF5 path

#define ANKERL_NANOBENCH_IMPLEMENT
#include "../harness/nanobench.h"
#include "../harness/config.hpp"
#include "../fixtures/synthetic.hpp"
#include <h5cpp/all>
#include <vector>
#include <cstdio>

static const char* const kFile = "/dev/shm/h5cpp_bench_ht.h5";
static void cleanup() { std::remove(kFile); }

int main() {
    bench::fixture::Synthetic gen(42);
    const std::vector<std::size_t> sizes = bench::payload_sizes();

    for (std::size_t n : sizes) {
        auto data = gen.doubles(n);
        const std::size_t bytes = data.size() * sizeof(double);
        const std::string label = std::to_string(n);

        // Standard path (no filter)
        ankerl::nanobench::Bench().unit("byte").batch(bytes).run(
            "high_throughput/standard/write/" + label, [&] {
                cleanup();
                h5::fd_t fd = h5::create(kFile, H5F_ACC_TRUNC);
                h5::write(fd, "data", data);
            });

        // Standard path + gzip{3} (chained DCPL)
        ankerl::nanobench::Bench().unit("byte").batch(bytes).run(
            "high_throughput/standard_gzip3/write/" + label, [&] {
                cleanup();
                h5::fd_t fd = h5::create(kFile, H5F_ACC_TRUNC);
                h5::ds_t ds = h5::create<double>(fd, "data",
                    h5::current_dims{n}, h5::chunk{1024} | h5::gzip{3});
                h5::write(ds, data);
            });

        // high_throughput path (no filter)
        ankerl::nanobench::Bench().unit("byte").batch(bytes).run(
            "high_throughput/direct/write/" + label, [&] {
                cleanup();
                h5::fd_t fd = h5::create(kFile, H5F_ACC_TRUNC);
                h5::ds_t ds = h5::create<double>(fd, "data",
                    h5::current_dims{n}, h5::max_dims{n}, h5::chunk{1024}, h5::high_throughput);
                h5::write(ds, data);
            });

        // high_throughput path + gzip{3} (chained DCPL)
        ankerl::nanobench::Bench().unit("byte").batch(bytes).run(
            "high_throughput/direct_gzip3/write/" + label, [&] {
                cleanup();
                h5::fd_t fd = h5::create(kFile, H5F_ACC_TRUNC);
                h5::ds_t ds = h5::create<double>(fd, "data",
                    h5::current_dims{n}, h5::max_dims{n}, h5::chunk{1024} | h5::gzip{3}, h5::high_throughput);
                h5::write(ds, data);
            });

        cleanup();
    }
}
