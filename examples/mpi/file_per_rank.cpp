// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// MPI compute + serial HDF5 file-per-rank.
//
// The pragmatic pattern when you have MPI but NO parallel HDF5 build (no
// --enable-parallel) and/or NO parallel filesystem.  Each rank operates on
// its own slice of the work and writes to its own private `.h5` file —
// rank 0 → `output_000.h5`, rank 1 → `output_001.h5`, etc.  No cross-rank
// file coordination is required, so a stock serial HDF5 build is enough.
//
// This is what the user gets when they have a workstation with OpenMPI but
// no Lustre/GPFS, or when they want to scale embarrassingly-parallel jobs
// (Monte Carlo, ensemble simulations, ML hyperparameter sweeps) without
// the operational overhead of parallel HDF5.
//
// RUN: mpirun -n <N> ./examples-mpi-file-per-rank
// OUT: output_000.h5, output_001.h5, ..., output_<N-1>.h5
//
// Each file is a complete, standalone HDF5 container that can be read
// later via `h5dump`, h5py, or another h5cpp program — no MPI required to
// read them back.  Post-process with a small "merge" pass if a single
// combined output is desired (or use a virtual dataset to expose the
// per-rank files as a single logical dataset).

#include <mpi.h>
#include <h5cpp/all>

#include <array>
#include <cstdio>           // std::snprintf
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, world_size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Per-rank filename — zero-padded so a directory listing sorts cleanly.
    char path[32];
    std::snprintf(path, sizeof(path), "output_%03d.h5", rank);

    // ── compute: each rank "simulates" 1000 samples of its own slice ──
    // (replace with whatever your real workload is — Monte Carlo trial,
    // ensemble member, hyperparameter sweep point, etc.)
    constexpr std::size_t n_samples = 1000;
    std::vector<double> samples(n_samples);
    for (std::size_t i = 0; i < n_samples; ++i)
        samples[i] = double(rank) + double(i) / n_samples;   // deterministic stand-in

    // ── write the rank's data to its own file (pure serial HDF5) ──
    {
        h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC);
        h5::write(fd, "/samples", samples);

        // Provenance attributes — useful when files end up shuffled across
        // a results directory:
        h5::ds_t ds = h5::open(fd, "/samples");
        ds["rank"]       = rank;
        ds["world_size"] = world_size;
        ds["generator"]  = std::string("examples-mpi-file-per-rank");
    }

    // ── barrier so rank 0's summary lands after every rank has written ──
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "wrote " << world_size << " files: output_000.h5 .. output_"
                  << std::string(world_size < 10 ? "00" : world_size < 100 ? "0" : "")
                  << (world_size - 1) << ".h5\n";
    }

    // ── self-check: every rank reads its own file back and verifies ──
    {
        h5::fd_t fd = h5::open(path, H5F_ACC_RDONLY);
        auto back = h5::read<std::vector<double>>(fd, "/samples");
        bool ok = back.size() == samples.size();
        for (std::size_t i = 0; ok && i < samples.size(); ++i)
            ok = (back[i] == samples[i]);
        std::cout << "rank " << rank << "/" << world_size
                  << "  " << (ok ? "✔ ok   " : "✘ failed")
                  << "  wrote " << back.size() << " samples to " << path << "\n";
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
