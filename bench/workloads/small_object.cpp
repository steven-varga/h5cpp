// SPDX-License-Identifier: MIT
// Copyright (c) 2025-2026 Varga Labs, Toronto, ON, Canada.
//
// Bench: small-object write latency (single values, small vectors, structs)

#define ANKERL_NANOBENCH_IMPLEMENT
#include "../harness/nanobench.h"
#include <h5cpp/all>
#include <vector>
#include <cstdio>

static const char* const kFile = "/dev/shm/h5cpp_bench_small.h5";
static void cleanup() { std::remove(kFile); }

struct Particle {
    double x, y, z;
    float vx, vy, vz;
    int id;
    char type;
};
H5CPP_REGISTER_STRUCT(Particle);

int main() {
    // Single double
    ankerl::nanobench::Bench().unit("op").run("small/write/single_double", [&] {
        cleanup();
        h5::fd_t fd = h5::create(kFile, H5F_ACC_TRUNC);
        double v = 3.14159;
        h5::write(fd, "val", v);
    });

    // Small vector (4 KB)
    std::vector<double> vec(512, 1.0);
    ankerl::nanobench::Bench().unit("op").run("small/write/vec_4kb", [&] {
        cleanup();
        h5::fd_t fd = h5::create(kFile, H5F_ACC_TRUNC);
        h5::write(fd, "vec", vec);
    });

    // Single struct
    Particle p = {1.0, 2.0, 3.0, 0.1f, 0.2f, 0.3f, 42, 'e'};
    ankerl::nanobench::Bench().unit("op").run("small/write/struct_8field", [&] {
        cleanup();
        h5::fd_t fd = h5::create(kFile, H5F_ACC_TRUNC);
        h5::write(fd, "particle", p);
    });

    cleanup();
}
