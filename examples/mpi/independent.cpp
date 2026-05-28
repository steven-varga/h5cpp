// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Parallel HDF5 — INDEPENDENT write/read across MPI ranks.
//
// Same dataset shape as collective.cpp but uses h5::independent for the
// transfer mode.  In INDEPENDENT mode each rank issues its own MPI-IO
// operation without cross-rank coordination — ranks may opt out of any
// given call.  Lower-latency for irregular workloads; lower throughput
// than COLLECTIVE for regular slabs.
//
// REQUIRES: HDF5 built with --enable-parallel (HDF5_IS_PARALLEL=ON).
//
// RUN: mpirun -n <N> ./examples-mpi-independent

#include <mpi.h>
#include <h5cpp/all>
#include <vector>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, world_size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    constexpr int nrows = 10;

    // ── CREATE + WRITE (independent transfer mode) ────────────────────────
    {
        std::vector<double> v(nrows, double(rank + 2));
        auto fd = h5::create("independent.h5", H5F_ACC_TRUNC, h5::default_fcpl,
            h5::mpiio({MPI_COMM_WORLD, MPI_INFO_NULL}));
        h5::write(fd, "dataset", v,
            h5::current_dims{nrows, world_size},
            h5::chunk{nrows, 1},
            h5::offset{0, rank},
            h5::count{nrows, 1},
            h5::independent);
    }

    // ── READ (independent transfer mode — matches write) ─────────────────
    {
        auto fd = h5::open("independent.h5", H5F_ACC_RDWR,
            h5::mpiio({MPI_COMM_WORLD, MPI_INFO_NULL}));
        auto data = h5::read<std::vector<double>>(fd, "dataset",
            h5::offset{0, rank}, h5::count{nrows, 1}, h5::independent);
        std::cout << "rank " << rank << " of " << world_size
                  << " read " << data.size() << " elements, first = " << data[0] << "\n";

        std::vector<double> buffer(nrows);
        auto ds = h5::open(fd, "dataset");
        h5::read(ds, buffer.data(),
            h5::offset{0, rank}, h5::count{nrows, 1}, h5::independent);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
