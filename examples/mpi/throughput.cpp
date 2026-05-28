// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Parallel HDF5 — write/read throughput benchmark across MPI ranks.
//
// Each rank writes (default) 80 MB to its own row of a (world_size × nrows)
// chunked dataset, then reads it back.  Aggregate MB/s is gathered on rank
// 0 via MPI_Gather and printed.  Use this as the "what's my actual disk
// bandwidth across N processes" sanity check before profiling real code.
//
// REQUIRES: HDF5 built with --enable-parallel (HDF5_IS_PARALLEL=ON).  On a
// node-local filesystem the achievable throughput plateaus at the disk's
// sequential write limit divided by world_size; on Lustre/GPFS it scales
// with the number of OSTs/stripes.
//
// RUN: mpirun -n <N> ./examples-mpi-throughput

#include <mpi.h>
#include <h5cpp/all>
#include <chrono>
#include <numeric>
#include <vector>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, world_size = 0;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &world_size);

    // 80 MB per rank — adjust nchunk if the test should run smaller.
    // The chunk_size matches nrows to keep each rank's slab a single chunk.
    constexpr std::size_t nchunk = 1024 * 1024;
    constexpr std::size_t nrows  = 10 * nchunk;
    const std::size_t vbytes = nrows * sizeof(double);

    // ── WRITE ──────────────────────────────────────────────────────────────
    {
        std::vector<double> v(nrows, double(rank + 2));
        auto fd = h5::create("throughput.h5", H5F_ACC_TRUNC, h5::default_fcpl,
            h5::mpiio({comm, MPI_INFO_NULL}));
        // alloc_time_early forces space allocation at create-time so the
        // write benchmark below measures pure I/O, not metadata + alloc.
        h5::ds_t ds = h5::create<double>(fd, "dataset",
            h5::max_dims{static_cast<hsize_t>(world_size), nrows},
            h5::chunk{1, nchunk} | h5::alloc_time_early);

        auto t0 = std::chrono::steady_clock::now();
        h5::write(ds, v,
            h5::current_dims{static_cast<hsize_t>(world_size), nrows},
            h5::offset{static_cast<hsize_t>(rank), 0},
            h5::count{1, nrows},
            h5::collective);
        auto t1 = std::chrono::steady_clock::now();

        const double seconds = std::chrono::duration<double>(t1 - t0).count();
        const double MB_per_s = (double(vbytes) / 1e6) / seconds;

        std::vector<double> rates(world_size);
        MPI_Gather(&MB_per_s, 1, MPI_DOUBLE, rates.data(), 1, MPI_DOUBLE, 0, comm);
        if (rank == 0) {
            double total = std::accumulate(rates.begin(), rates.end(), 0.0);
            std::cout << "WRITE: " << total << " MB/s aggregate ("
                      << world_size << " ranks)\n";
        }
    }

    // ── READ ───────────────────────────────────────────────────────────────
    {
        std::vector<double> v(nrows);
        auto fd = h5::open("throughput.h5", H5F_ACC_RDWR,
            h5::mpiio({comm, MPI_INFO_NULL}));
        auto ds = h5::open(fd, "/dataset");

        auto t0 = std::chrono::steady_clock::now();
        h5::read(ds, v.data(),
            h5::offset{static_cast<hsize_t>(rank), 0},
            h5::count{1, nrows},
            h5::collective);
        auto t1 = std::chrono::steady_clock::now();

        const double seconds = std::chrono::duration<double>(t1 - t0).count();
        const double MB_per_s = (double(vbytes) / 1e6) / seconds;

        std::vector<double> rates(world_size);
        MPI_Gather(&MB_per_s, 1, MPI_DOUBLE, rates.data(), 1, MPI_DOUBLE, 0, comm);
        if (rank == 0) {
            double total = std::accumulate(rates.begin(), rates.end(), 0.0);
            std::cout << "READ:  " << total << " MB/s aggregate ("
                      << world_size << " ranks)\n";
        }
    }

    MPI_Barrier(comm);
    MPI_Finalize();
    return 0;
}
