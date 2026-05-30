@page curated_topics_mpi MPI

@brief Parallel HDF5 I/O via MPI-IO — collective and independent
transfer, the FAPL setup, and the often-missed fact that you don't
need a parallel filesystem to use it.

# MPI parallel I/O

## Why MPI HDF5 at all

A single-threaded `H5Dwrite` from one process can absorb maybe
1–2 GB/s on modern NVMe. That's fast — until you have hundreds of
ranks producing data and a single coordinator becomes the bottleneck
that wastes the rest of the cluster. The two standard ways to scale
out:

| Pattern             | Setup                                  | Trade-off                                                     |
| :------------------ | :------------------------------------- | :------------------------------------------------------------ |
| **File-per-rank**   | Each rank writes its own `.h5`         | No coordination cost. Many files (post-process to merge).     |
| **Collective MPI**  | All ranks write to ONE shared file      | Coordinated single output. Needs MPI-IO + (usually) a parallel FS. |

HDF5's MPI integration handles the second case: every rank sees the
same `h5::fd_t`, they coordinate behind the scenes through MPI-IO,
and the result is one HDF5 container regardless of how many ranks
wrote to it.

## The setup — FAPL with the MPI driver

```cpp
#include <mpi.h>
#include <h5cpp/all>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, nranks;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    // FAPL with MPI-IO driver — every rank opens the same FAPL
    h5::fapl_t fapl = h5::fapl{h5::driver::mpi{MPI_COMM_WORLD, MPI_INFO_NULL}};

    // All ranks collectively create / open the SAME file
    h5::fd_t fd = h5::create("shared.h5", H5F_ACC_TRUNC, h5::default_fcpl, fapl);

    // All ranks collectively create the dataset
    const hsize_t per_rank = 1'000'000;
    h5::ds_t ds = h5::create<double>(fd, "/grid/data",
        h5::current_dims{nranks * per_rank});

    // Each rank writes ITS slab of the same dataset
    std::vector<double> my_chunk(per_rank, double(rank));
    h5::write(ds, my_chunk,
        h5::offset{rank * per_rank},
        h5::count{per_rank});

    MPI_Finalize();
    return 0;
}
```

What's happening:
1. **Symmetric setup** — every rank runs the same code; the only
   per-rank state is `rank`.
2. **Collective metadata operations** — `h5::create` for the file
   and `h5::create<double>` for the dataset are collective; all
   ranks must call them or HDF5 deadlocks.
3. **Per-rank hyperslab writes** — each rank picks its own `offset`
   and `count`, and the underlying `H5Dwrite` coordinates writes
   without forcing inter-rank synchronisation per element.
4. **Symmetric close** — when each rank's `h5::fd_t` destructs, it
   participates in the collective `H5Fclose`.

## Collective vs independent transfer

The default transfer mode is **independent** — each rank writes its
slab whenever it's ready, no inter-rank coordination. Switch to
**collective** mode by composing a tuned `dxpl_t`:

```cpp
auto dxpl = h5::dxpl{} | h5::collective;
h5::write(ds, my_chunk, h5::offset{rank * per_rank}, h5::count{per_rank}, dxpl);
```

| Mode             | Coordination       | When it wins                                                  |
| :--------------- | :----------------- | :------------------------------------------------------------ |
| **Independent**  | None per write     | Sparse writes; ranks write to disjoint regions; small batches |
| **Collective**   | Barrier per write  | Dense writes covering the full extent; large slabs; aligned chunks |

Collective writes can be 2-10x faster on dense workloads because the
MPI-IO layer aggregates per-rank writes into a single large I/O
request. They're slower than independent on sparse / unbalanced
workloads because the barrier waits for the slowest rank.

## ✱ The fact that surprises people: parallel FS is NOT required

> **You can run MPI HDF5 on a single laptop, with one rank, on a
> POSIX filesystem.** And on multiple ranks against any filesystem
> the MPI implementation supports — Lustre, GPFS, BeeGFS, *or*
> plain NFS, *or* even local `/tmp` shared via symlinks.

What MPI HDF5 actually needs:

| Requirement                                     | Required? | Notes                                                          |
| :---------------------------------------------- | :-------- | :------------------------------------------------------------- |
| HDF5 built with MPI support (`--enable-parallel`) | ✔ yes   | Most distro packages ship a separate `libhdf5-mpi-dev`         |
| An MPI implementation linked into the binary    | ✔ yes     | OpenMPI / MPICH / Intel MPI / etc.                             |
| Multiple physical machines                      | ✘ no      | Single-host, single-rank is a fine smoke test                  |
| Parallel filesystem (Lustre / GPFS / BeeGFS)    | ✘ no      | The MPI-IO driver works against any filesystem MPI can mount   |
| Network with RDMA / IB                          | ✘ no      | Performance benefits at scale, but optional                    |

Why it works on a regular filesystem: the MPI-IO layer in your MPI
runtime serialises writes through the operating system the same way
POSIX `pwrite` does. You don't get the bandwidth amplification a
true parallel FS provides, but the semantics are correct — multiple
ranks writing disjoint slabs of the same file land where they
should.

What this means in practice:

- **CI smoke tests** — run `mpirun -n 4 ./my_mpi_test` against a
  tmpfs filesystem in the CI runner. No HPC cluster needed to
  validate the code paths.
- **Single-rank dev loop** — develop your MPI HDF5 code on a laptop
  with `mpirun -n 1`. Behaviour is identical to multi-rank apart
  from the lack of contention.
- **NFS-backed shared storage** — works for moderate rank counts.
  Performance plateaus once NFS write coalescing saturates, but
  semantics stay correct.

The performance ceiling on a non-parallel FS is roughly the
filesystem's single-writer bandwidth divided by the number of ranks
hitting it. That's bad for production HPC; fine for testing,
prototyping, and small-cluster work.

## Common patterns

### File-per-rank (no MPI HDF5 needed)

Simpler alternative when ranks don't actually need to share a file:

```cpp
auto path = "result_" + std::to_string(rank) + ".h5";
h5::fd_t fd = h5::create(path, H5F_ACC_TRUNC);   // plain POSIX FAPL
h5::write(fd, "/data", my_chunk);
```

Zero coordination cost, no MPI-IO required, simpler. Post-process to
merge the per-rank files if you need a single artifact.

### Async mode within a rank

MPI parallelism scales across ranks; the **async-mode** machinery
(`h5::async::*`) scales within a rank by overlapping HDF5 I/O with
compute. Compose them — an MPI rank can run an async write while
working on the next batch:

```cpp
h5::async::fd_t afd = h5::async::create(path, H5F_ACC_TRUNC,
    h5::async{h5::threads{4}});
// ... async writes overlap with rank-local compute ...
```

See @ref handle_ref_async "Async-mode handles" for the type-level
machinery; the @ref reports_fapl_multithreading_workplan report
covers the FAPL-scoped worker pool and the async-mode dispatch
strategy.

## Where to go next

- @ref example_guide_mpi — runnable parallel-write cookbook with
  collective and independent transfer
- @ref curated_topics_properties — FAPL/DXPL deeper reference
- @ref handle_ref_async — the async-mode handle taxonomy (per-rank
  concurrency)
- @ref example_guide_s3 — ROS3 read-only S3 access (different
  parallelism story — read-side scale-out)
