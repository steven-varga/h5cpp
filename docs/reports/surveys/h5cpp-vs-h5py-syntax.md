@page reports_vs_h5py_syntax h5cpp vs h5py — Syntax Comparison

**The claim:** h5cpp is the first C++ library to match h5py's one-line simplicity.

---

## 1. Basic Write / Read

### h5py (Python)
```python
import h5py
import numpy as np

with h5py.File("data.h5", "w") as f:
    f["temperature"] = np.array([293.15, 298.15, 303.15])

with h5py.File("data.h5", "r") as f:
    data = f["temperature"][:]
```

### h5cpp (C++)
```cpp
#include <h5cpp/h5cpp.hpp>

h5::write("data.h5", "temperature", std::vector<double>{293.15, 298.15, 303.15});
auto data = h5::read<std::vector<double>>("data.h5", "temperature");
```

**Verdict:** One line each way. h5cpp matches h5py.

---

## 2. STL Containers (The h5cpp Killer Feature)

### h5py
```python
# h5py handles numpy arrays natively.
# For a list of dicts or complex nested data? You flatten manually.
# For a pandas DataFrame? You use a special library (pytables).
```

### h5cpp
```cpp
// std::vector — automatic
std::vector<Particle> particles = generate_particles();
h5::write("sim.h5", "particles", particles);

// std::map — automatic
std::map<std::string, double> params = {{"dt", 0.01}, {"T", 300.0}};
h5::write("sim.h5", "params", params);

// std::deque, std::set, std::unordered_map — all automatic
std::deque<Event> events = read_events();
h5::write("sim.h5", "events", events);
```

**Verdict:** h5cpp handles any STL container natively. h5py is numpy-centric.

---

## 3. Custom Structs (Compile-Time Reflection)

### h5py
```python
# h5py: You must manually define a numpy structured dtype
dtype = np.dtype([
    ("x", np.float64),
    ("y", np.float64),
    ("z", np.float64),
    ("id", np.int32),
])

particles = np.array([(1.0, 2.0, 3.0, 42)], dtype=dtype)
with h5py.File("sim.h5", "w") as f:
    f["particles"] = particles
```

### h5cpp
```cpp
// Zero registration. Zero macros. Just define your struct.
struct Particle {
    double x, y, z;
    int id;
};

std::vector<Particle> particles = generate_particles();
h5::write("sim.h5", "particles", particles);

// Read back with automatic type reconstruction
auto read_back = h5::read<std::vector<Particle>>("sim.h5", "particles");
```

**Verdict:** h5cpp requires zero manual dtype registration. The C++ compiler reflects the struct at compile time. h5py forces you to maintain a parallel dtype definition.

---

## 4. Attributes

### h5py
```python
with h5py.File("sim.h5", "w") as f:
    dset = f.create_dataset("particles", data=particles)
    dset.attrs["units"] = "meters"
    dset.attrs["timestamp"] = 1716841200
```

### h5cpp
```cpp
auto file = h5::create("sim.h5");
h5::write(file, "particles", particles);
h5::write_attribute(file, "particles", "units", std::string("meters"));
h5::write_attribute(file, "particles", "timestamp", 1716841200);
```

**Verdict:** Equivalent verbosity. Both are one-liners per attribute.

---

## 5. Chunking + Compression

### h5py
```python
with h5py.File("big.h5", "w") as f:
    f.create_dataset("data", data=big_array,
                     chunks=(64, 64, 64),
                     compression="gzip",
                     compression_opts=4)
```

### h5cpp
```cpp
h5::write("big.h5", "data", big_vector,
          h5::chunk{64, 64, 64},
          h5::gzip{4});
```

**Verdict:** h5cpp is actually more concise. Named arguments in C++ via parameter packs.

---

## 6. MPI Parallel I/O

### h5py
```python
# h5py has no native MPI. You use mpi4py + h5py in parallel mode,
# but it's complex and not true collective I/O.
from mpi4py import MPI
import h5py

comm = MPI.COMM_WORLD
rank = comm.Get_rank()

with h5py.File("parallel.h5", "w", driver="mpio", comm=comm) as f:
    dset = f.create_dataset("data", (N,), dtype="f8")
    dset[rank] = local_data  # Not true collective
```

### h5cpp
```cpp
// True MPI collective I/O — one line per write
h5::write(file, "data", local_vector,
          h5::mpi_collective{comm});
```

**Verdict:** h5cpp is dramatically simpler for MPI. h5py doesn't really do HPC parallelism.

---

## 7. S3 / Cloud (ROS3)

### h5py
```python
# h5py has no native S3 support. You need s3fs + local caching,
# or a completely different library (h5coro, etc.).
```

### h5cpp
```cpp
// Read directly from S3 — zero code changes
auto data = h5::read<std::vector<double>>(
    "s3://mybucket/data.h5", "dataset");
```

**Verdict:** h5cpp has built-in S3. h5py does not.

---

## 8. The Real Difference: Scale

### h5py at scale
```python
# Works great for 1 GB. For 1 TB?
# - Python GIL limits threading
# - Interpreter overhead dominates
# - Memory copies between Python and C
# - No packet tables for streaming
```

### h5cpp at scale
```cpp
// Multithreaded compression + I/O pipeline
h5::write("terabyte.h5", "data", massive_vector,
          h5::chunk{1024, 1024},
          h5::gzip{6},
          h5::n_threads{16});

// Packet table for streaming simulation output
h5::packet_table pt(file, "events");
for (const auto& event : simulation_stream) {
    pt.append(event);  // Zero-copy append
}
```

**Verdict:** h5py stops at "easy." h5cpp keeps going to "fast."

---

## Summary Table

| Task | h5py | h5cpp | Winner |
|---|---|---|---|
| Basic array write | 1 line | 1 line | Tie |
| Custom struct | Manual dtype | **Zero registration** | **h5cpp** |
| `std::vector` / `std::map` | Numpy only | **Any STL container** | **h5cpp** |
| Chunking + compression | 4 parameters | 2 named args | Tie / h5cpp |
| MPI | Complex / limited | **1 line, collective** | **h5cpp** |
| S3 | None | **Built-in** | **h5cpp** |
| Multithreaded I/O | GIL-bound | **Native pipeline** | **h5cpp** |
| Performance | Baseline Python | **Beats C API** | **h5cpp** |

---

**Bottom line:** For syntax alone, h5cpp matches h5py's one-line simplicity. For everything else — performance, HPC, cloud, multithreading — h5cpp exceeds it.

## Related examples

- [`examples/basics/basics.cpp`](../../../examples/basics/basics.cpp) — the basic write/read overload set
- [`examples/datasets/datasets.cpp`](../../../examples/datasets/datasets.cpp) — dataset creation with explicit dims / chunking
