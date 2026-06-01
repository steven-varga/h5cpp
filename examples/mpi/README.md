@page example_guide_mpi MPI examples

Two families of MPI + HDF5, with different dependencies:

| Family                        | Pattern                                  | Needs MPI | Needs parallel HDF5 | Needs parallel FS |
| ----------------------------- | ---------------------------------------- | --------- | ------------------- | ----------------- |
| Parallel HDF5 (3 examples)    | One shared file, MPI-IO collective/independent transfer | ✔ | ✔ | recommended |
| MPI + serial HDF5 (1 example) | One file per rank, plain serial HDF5     | ✔ | ✘ | ✘                 |

If your HDF5 build doesn't have `--enable-parallel` (most distro packages don't), the parallel-HDF5 examples are skipped by CMake with a clear status message; the file-per-rank example still builds and runs.

## Files

| File                  | Tier             | What it teaches                                                                |
| --------------------- | ---------------- | ------------------------------------------------------------------------------ |
| `collective.cpp`      | parallel HDF5    | Collective transfer mode — all ranks must participate in each I/O call         |
| `independent.cpp`     | parallel HDF5    | Independent transfer mode — each rank issues its own MPI-IO op                 |
| `throughput.cpp`      | parallel HDF5    | Aggregate write/read MB/s benchmark across ranks                               |
| `file_per_rank.cpp`   | MPI + serial HDF5 | Embarrassingly-parallel pattern — each rank writes its own private `.h5` file |

## Build

```sh
cd <build-dir>
cmake .            # configure auto-detects MPI and parallel HDF5
```

CMake prints one of:

```
-- MPI found: /usr/local/bin/mpicxx (mpiexec: /usr/local/bin/mpiexec)
-- Parallel HDF5 found: collective / independent / throughput enabled
```

or:

```
-- MPI found: /usr/local/bin/mpicxx (mpiexec: /usr/local/bin/mpiexec)
-- Parallel HDF5 NOT found (HDF5 built without --enable-parallel):
--   skipping mpi-collective / mpi-independent / mpi-throughput
--   mpi-file-per-rank still builds — it uses serial HDF5 per rank
```

or:

```
-- MPI not found: skipping all mpi/* examples
```

Then:

```sh
cmake --build . --target examples-mpi-file-per-rank   # always available if MPI is present
cmake --build . --target examples-mpi-collective      # parallel HDF5 only
cmake --build . --target examples-mpi-independent     # parallel HDF5 only
cmake --build . --target examples-mpi-throughput      # parallel HDF5 only
```

## Run

```sh
mpirun -n 4 ./examples-mpi-file-per-rank
mpirun -n 4 ./examples-mpi-collective
mpirun -n 4 ./examples-mpi-independent
mpirun -n 4 ./examples-mpi-throughput
```

`mpirun -n <N>` launches `N` ranks. For SLURM-managed clusters, the same launch shape is `srun -n <N> ./examples-mpi-<name>` — CMake autodetects SLURM and prints the right command at configure time.

## Tier 1 — parallel HDF5 (one shared file)

Each rank writes its own slab into a single shared `.h5` file via HDF5's MPI-IO virtual driver. Two transfer modes:

| Mode                | API           | When to use                                                           |
| ------------------- | ------------- | --------------------------------------------------------------------- |
| `h5::collective`    | `H5FD_MPIO_COLLECTIVE` | Regular, predictable slabs (every rank touches every collective call). Highest throughput on a parallel filesystem. |
| `h5::independent`   | `H5FD_MPIO_INDEPENDENT` | Irregular workloads where ranks may opt out of individual calls. Lower latency, lower aggregate throughput. |

Open the file with `h5::mpiio({MPI_COMM_WORLD, MPI_INFO_NULL})` to attach the parallel driver. Pass the transfer mode as the last argument to `h5::write` / `h5::read`.

**Achievable throughput** scales linearly with the number of OSTs/stripes on a Lustre/GPFS/BeeGFS volume; on a node-local POSIX disk it plateaus at the disk's sequential bandwidth divided by `world_size`.

Sample output from `mpi-throughput` (4 ranks, 80 MB / rank = 320 MB total, local SSD + page cache):

```
WRITE: 4199.81 MB/s aggregate (4 ranks)
READ:  4970.11 MB/s aggregate (4 ranks)
```

Numbers this high reflect the Linux page cache absorbing the 320 MB working set — real disk bandwidth only becomes the bottleneck when the per-rank slab exceeds available RAM. To benchmark the parallel filesystem itself (not the cache), bump `nrows` until the total dataset is at least 2× system RAM, or run with `posix_fadvise(DONTNEED)` between write and read.

## Tier 2 — MPI + serial HDF5 (file per rank)

Each rank writes to `output_<rank>.h5`. No cross-rank file coordination, so a stock serial HDF5 build is enough — the same library that ships in `libhdf5-dev` on Debian/Ubuntu.

This is the right pattern for:

- **Workstations with MPI but no parallel filesystem.** Most laptops + desktops fall here.
- **Embarrassingly-parallel jobs**: Monte Carlo trials, ensemble simulations, hyperparameter sweeps.
- **Containerised runs** where setting up parallel HDF5 + MPI-IO inside the image is overkill.

Each output file is a complete, standalone HDF5 container:

```sh
$ mpirun -n 4 ./examples-mpi-file-per-rank
rank 0/4  ✔ ok     wrote 1000 samples to output_000.h5
rank 1/4  ✔ ok     wrote 1000 samples to output_001.h5
rank 2/4  ✔ ok     wrote 1000 samples to output_002.h5
rank 3/4  ✔ ok     wrote 1000 samples to output_003.h5
wrote 4 files: output_000.h5 .. output_003.h5

$ h5dump -H output_002.h5
DATASET "samples" {
   DATATYPE  H5T_IEEE_F64LE
   DATASPACE  SIMPLE { ( 1000 ) / ( 1000 ) }
   ATTRIBUTE "generator" { ... }
   ATTRIBUTE "rank"       { DATATYPE H5T_STD_I32LE  DATASPACE SCALAR }
   ATTRIBUTE "world_size" { DATATYPE H5T_STD_I32LE  DATASPACE SCALAR }
}
```

To present the per-rank files as a single logical dataset post-run, use an HDF5 **virtual dataset** (VDS) — see HDF5 docs; outside the scope of this example.

## Choosing between the tiers

| Question                                                | Use         |
| ------------------------------------------------------- | ----------- |
| Do you have a parallel filesystem (Lustre/GPFS/BeeGFS)? | Tier 1      |
| Are ranks contributing to a single canonical dataset?   | Tier 1      |
| Are ranks doing independent compute (MC, ensembles)?    | Tier 2      |
| Is the HDF5 build serial (`--enable-parallel` off)?     | Tier 2 (only choice) |

Tier 2's "post-process the per-rank files into one" overhead is usually amortised quickly when ranks run on heterogeneous nodes or different disks.

## Build State (as of HEAD)

All four targets ✔ ok on this machine — OpenMPI + parallel HDF5 (HDF5 1.12.3 at `/usr/local/HDF_Group/HDF5/1.12.3/` with `--enable-parallel`).

| Target                       | Status | Notes                                                                                   |
| ---------------------------- | ------ | --------------------------------------------------------------------------------------- |
| `examples-mpi-file-per-rank` | ✔ ok   | Per-rank file written + readback-verified                                               |
| `examples-mpi-collective`    | ✔ ok   | 4 ranks → `(10 × 4)` shared dataset, each rank reads its own column back                |
| `examples-mpi-independent`   | ✔ ok   | Same shape, independent transfer mode                                                   |
| `examples-mpi-throughput`    | ✔ ok   | 4 ranks × 80 MB = 320 MB; ~4.2 GB/s write, ~5.0 GB/s read (local SSD + page cache)     |

Gated on `MPI_FOUND` (all four) and `HDF5_IS_PARALLEL` (first three). When either is missing CMake prints exactly which dep is unavailable and skips the affected targets.

## Cross-references

- **`examples/raw_memory/`** — raw-pointer write/read shape that file-per-rank uses under the hood
- **`examples/packet-table/`** — streaming append; an alternative for ranks producing data over time without coordinating offsets
- **HDF5 manual: Parallel HDF5** — https://docs.hdfgroup.org/hdf5/develop/group___p_h5_p.html
- **HDF5 virtual datasets (VDS)** — for post-merging file-per-rank outputs into one logical view

## Source

- [`collective.cpp`](collective_8cpp-example.html) — rendered with syntax highlighting
- [`file_per_rank.cpp`](file_per_rank_8cpp-example.html) — rendered with syntax highlighting
- [`independent.cpp`](independent_8cpp-example.html) — rendered with syntax highlighting
- [`throughput.cpp`](throughput_8cpp-example.html) — rendered with syntax highlighting
