@page example_guide_packet_table Packet Tables — Streaming Append

A packet table is a chunk-buffered, append-only view on an HDF5 dataset. You create it like any dataset, then call `h5::append(pt, record)` instead of `h5::write(...)`:

```cpp
h5::pt_t pt = h5::create<int>(fd, "/stream",
    h5::max_dims{H5S_UNLIMITED}, h5::chunk{16} | h5::gzip{6});

for (int x : input)
    h5::append(pt, x);

// pt destructor flushes the trailing partial chunk
```

`h5::pt_t` is `h5::ds_t` with a write-cache on top. Every dataset flag (`h5::chunk`, `h5::gzip`, `h5::fill_value`, `h5::max_dims`) works unchanged. The implicit conversion from `ds_t` to `pt_t` is what makes the call shape above work.

## What It Is For

Packet tables solve the same problem in every streaming domain: you do not know the final size when you start writing.

| Domain | Pattern |
|--------|---------|
| **Trading / market data** | Tick streams, order-book snapshots, or FIX messages arriving at irregular intervals. The dataset grows for the entire trading session; chunk size is tuned to the read pattern (e.g., one chunk per minute of ticks). |
| **Sensor networks** | Telemetry from edge devices: temperature, pressure, vibration samples batched and forwarded when connectivity is available. Each batch is appended; the dataset accumulates indefinitely. |
| **Audio** | Multi-channel PCM streams captured over time. Frames arrive in fixed-size buffers; the packet table appends each buffer as a new row in an unlimited `(time, channel, sample)` dataset. |
| **Video** | Frame sequences from cameras or simulations. Each frame is a 2-D image; the dataset is `(frame, height, width)` with one chunk per frame so random frame access is a single chunk read. |

The common thread: append-only, unbounded growth, and chunk-aligned reads. `h5::append` handles the bookkeeping so your code stays a simple loop.

## Files

| File                 | What it teaches                                                              |
| -------------------- | ---------------------------------------------------------------------------- |
| `struct.h`           | POD layout — pure data types, no h5cpp dependency.                           |
| `generated.h`        | `H5CPP_REGISTER_STRUCT` bodies that map the POD types onto HDF5 compounds.   |
| `packettable.cpp`    | Four append patterns: scalar 1D, scalar 3D, POD struct, fixed-size matrix.   |
| `packet_batches.cpp` | Three timed write patterns over the same 8 × 128k dataset (element-wise, row-wise, raw-pointer), plus a movie-clip streaming demo. |

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-packet-table examples-packet-batches
./examples-packet-table
./examples-packet-batches
```

Expected output of `examples-packet-table`:

```
✔ ok    stream<int> -> 1D unlimited dataset      (80 elements)
✔ ok    stream<int> -> 3D unlimited dataset      (4 x 3 x 5)
✔ ok    stream<POD struct> -> 1D unlimited        (128 records, idx)
✔ ok    stream<Eigen frame> -> 3D unlimited   (20 x 2 x 256)
```

Expected output of `examples-packet-batches` (timings vary by machine):

```
✔ ok    variant 1: element-wise append      shape (8 x 128k)
✔ ok    variant 2: row-wise append           shape (8 x 128k)
✔ ok    variant 3: raw-pointer append        shape (8 x 128k)

timings (microseconds, lower is better):
  variant 1 element-wise : 10554
  variant 2 row-wise     :  4614
  variant 3 raw-pointer  :  3169

✔ ok    variant 4: movie clip               24 x 64 x 64 frames
  variant 4 movie clip (write only): 1636 us
```

The raw-pointer path is fastest because it bypasses the per-element write cache and feeds the HDF5 chunk pipeline directly.

The movie-clip variant demonstrates the canonical applied case: a 3-D unlimited dataset shaped `(frames, height, width)` with one chunk per frame and gzip-6 compression. Each frame is a 64×64 uint8 grayscale image. Per-pixel readback of the first and last frames verifies both shape and content.

## Chunk Alignment

The packet table's on-disk extent always grows by `chunk_dims[0]`. If you append `n` records and `n % chunk_dims[0] != 0`, the trailing partial chunk is flushed on `pt_t` destruction, padded with the configured `h5::fill_value` (or zero if unset). A `h5::read<std::vector<T>>` of the result therefore has `ceil(n / chunk[0]) * chunk[0]` elements, not `n`.

The examples use stream sizes that are exact multiples of the chunk size to keep verification clean. In production you either accept the padding or read with an explicit `h5::count{...}` slice.

## Include Pattern

Use one umbrella header plus the registration file:

```cpp
#include <armadillo>
#include <h5cpp/all>
#include "generated.h"
```

`generated.h` itself pulls `<h5cpp/all>` and `"struct.h"`, so a translation unit including it does not need to repeat them.

## CMake Wiring

```cmake
if(${Eigen3_FOUND} AND ${ARMADILLO_FOUND})
    add_h5cpp_example(packet-table packet-table/packettable.cpp
        LIBRARIES libarmadillo Eigen3::Eigen)
    add_h5cpp_example(packet-batches packet-table/packet_batches.cpp
        LIBRARIES libarmadillo)
endif()
```

No `GENERATED` line — `generated.h` is committed alongside `struct.h` rather than re-emitted by the h5cpp compiler at build time. If you change `struct.h`, regenerate `generated.h` manually (or by running the h5cpp compiler).

## Build State

| Target                    | Status                                                   |
| ------------------------- | -------------------------------------------------------- |
| `examples-packet-table`   | ✔ ok — four stream patterns verified by readback         |
| `examples-packet-batches` | ✔ ok — three timed append patterns, raw-pointer fastest  |

Both gated on `ARMADILLO_FOUND` (`packet-table` additionally on `Eigen3_FOUND`).

## Cross-References

- `examples/compound/` — single-TU version of the POD struct write/read path used here.
- `examples/datasets/` — full `offset` / `count` / `stride` / `block` vocabulary that packet tables build on top of.
- `examples/multi-tu/` — same `generated.h` ODR story applied across multiple translation units.
- `h5cpp/H5Dappend.hpp` — `pt_t::append` overloads + the free-function wrappers.

## Source

- [`generated.h`](generated_8h-example.html) — rendered with syntax highlighting
- [`packet_batches.cpp`](packet_batches_8cpp-example.html) — rendered with syntax highlighting
- [`packettable.cpp`](packettable_8cpp-example.html) — rendered with syntax highlighting
- [`struct.h`](struct_8h-example.html) — rendered with syntax highlighting
