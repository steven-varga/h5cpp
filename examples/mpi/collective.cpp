// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Parallel HDF5 — COLLECTIVE write/read across MPI ranks.
//
// Each rank contributes a column of a (nrows × world_size) dataset; HDF5
// orchestrates the cross-rank I/O via MPI-IO with the COLLECTIVE transfer
// mode (all ranks must participate in every collective call).
//
// REQUIRES: HDF5 built with --enable-parallel (HDF5_IS_PARALLEL=ON).  On
// systems without parallel HDF5, CMake skips this target with a status
// message; use examples/mpi/file_per_rank.cpp instead for the realistic
// MPI-on-workstation pattern.
//
// RUN: mpirun -n <N> ./examples-mpi-collective
//
// A parallel filesystem (Lustre / GPFS / BeeGFS) gives the best throughput
// but is not required for correctness — a single shared POSIX file on a
// local disk works for ~4-8 ranks on one node.

#include <mpi.h>
#include <h5cpp/all>
#include <vector>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, world_size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    constexpr int nrows = 10;

    // ── CREATE + WRITE ─────────────────────────────────────────────────────
    {
        std::vector<double> v(nrows, double(rank + 2));
        auto fd = h5::create("collective.h5", H5F_ACC_TRUNC, h5::default_fcpl,
            h5::mpiio({MPI_COMM_WORLD, MPI_INFO_NULL}));
        // Single-call chunked write — every rank writes its own (nrows × 1)
        // slab into the (nrows × world_size) dataset at offset {0, rank}.
        h5::write(fd, "dataset", v,
            h5::current_dims{nrows, world_size},
            h5::chunk{nrows, 1},
            h5::offset{0, rank},
            h5::count{nrows, 1},
            h5::collective);
    }

    // ── READ ──────────────────────────────────────────────────────────────
    {
        auto fd = h5::open("collective.h5", H5F_ACC_RDWR,
            h5::mpiio({MPI_COMM_WORLD, MPI_INFO_NULL}));
        // Single-shot return-style read; convenient for small slabs.
        auto data = h5::read<std::vector<double>>(fd, "dataset",
            h5::offset{0, rank}, h5::count{nrows, 1}, h5::collective);
        std::cout << "rank " << rank << " of " << world_size
                  << " read " << data.size() << " elements, first = " << data[0] << "\n";

        // For tight loops, open the dataset once outside the loop and reuse
        // a pre-allocated buffer:
        std::vector<double> buffer(nrows);
        auto ds = h5::open(fd, "dataset");
        h5::read(ds, buffer.data(),
            h5::offset{0, rank}, h5::count{nrows, 1}, h5::collective);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
