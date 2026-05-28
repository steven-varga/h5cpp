# Raw-Pointer Read / Write

Most h5cpp examples write containers (`arma::mat`, `std::vector<T>`, `Eigen::Matrix`, ...) so the library can deduce shape, element type, and storage layout from per-container access traits. When you only have a bare pointer and a length — embedded code, FFI boundary, scratch buffer — you opt out of that machinery and describe the move yourself:

```cpp
double src[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
h5::write<double>(fd, "/dataset", src, h5::count{1, 10});

double buf[8] = {};
h5::read<double>(fd, "/dataset", buf, h5::count{1, 8}, h5::offset{0, 2});
// buf is now {2, 3, 4, 5, 6, 7, 8, 9}
```

The `h5::count{...}` is the dataspace shape; the optional `h5::offset{...}` is the file-space anchor. There is no separate "memory offset" — to write into the middle of a buffer, pass `ptr + k`.

## Files

| File          | What it teaches                                                         |
| ------------- | ----------------------------------------------------------------------- |
| `raw.cpp`     | The full positional argument vocabulary for `h5::create`, plus raw-pointer `write` / `read` with offset. |

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-raw
./examples-raw
```

Expected output:

```
✔ ok    read offset{0,2} count{1,8} -> buf[0..7]    (tail zero-init)
✔ ok    read into buf+4: buf[4..6] = first 3 of file
✔ ok    write/read int32[16] (squares)
```

## The Positional Argument Vocabulary

`h5::create<T>` has up to seven positional arguments:

```cpp
h5::create<T>(
    fd,                          // 1. h5::fd_t   file handle
    "/path",                     // 2. dataset path
    h5::current_dims{...},       // 3. optional   initial extent
    h5::max_dims{...},           // 4. optional   growth ceiling
    h5::lcpl_t   (link creation)        // 5. optional
    h5::dcpl_t   (dataset creation)     // 6. optional
    h5::default_dapl);           // 7. optional   access property list
```

Any argument can be omitted; the order is enforced. The example shows four equivalent ways to create the same dataset by combining or hoisting these blocks:

| Form                        | What's spelled out                                  |
| --------------------------- | --------------------------------------------------- |
| inline pipe-chained dcpl    | `h5::chunk{...} \| h5::gzip{...} \| ...`            |
| named `h5::dcpl_t` variable | reuse across multiple `h5::create` calls            |
| all defaults explicit       | `h5::default_lcpl`, `dcpl`, `h5::default_dapl`      |
| max_dims only               | `current_dims` = `max_dims` with `H5S_UNLIMITED → 0` |

The `max_dims`-only form is what packet tables build on top of: a dataset starts empty and grows along its unlimited dimension as `h5::append` lands chunks.

## How Shape Maps Across the Read / Write

```
file space:   shape (1, 10)        ← h5::count{1, 10}
                ┌─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐
                │0│1│2│3│4│5│6│7│8│9│
                └─┴─┴─┴─┴─┴─┴─┴─┴─┴─┘
                       ↑
                       h5::offset{0, 2}

memory:       1-D pointer
              buf:      ┌─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐
                        │ │ │ │ │ │ │ │ │ │ │
                        └─┴─┴─┴─┴─┴─┴─┴─┴─┴─┘
              after read(count{1,8}, offset{0,2}):
                        ┌─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐
                        │2│3│4│5│6│7│8│9│0│0│  ← tail is zero-init (or whatever
                        └─┴─┴─┴─┴─┴─┴─┴─┴─┴─┘    was there before)
```

The library doesn't auto-resize; the buffer's `count[0] * count[1] * ... * count[rank-1]` elements must be addressable from the pointer you pass.

## Read Into a Non-Zero Destination Offset

There is no `h5::dst_offset{...}` argument. The destination offset is the **pointer**:

```cpp
h5::read<double>(fd, "/dataset", buf + 4, h5::count{1, 3});  // writes into buf[4..6]
```

The same trick works for `h5::write` if you want to write a slice of a larger memory buffer.

## When to Use the Raw Path

Pick raw-pointer I/O when:

- You're crossing an FFI boundary (`extern "C"`, CUDA host buffer, MPI scratch space).
- You have an existing allocator (custom pool, ring buffer, `mmap` view) and don't want h5cpp to re-allocate.
- You're writing tightly into a known memory layout (image buffers, audio frames) and the container abstraction adds overhead you can't tolerate.

For everything else, use the container path — h5cpp picks the right shape automatically and the call shape stays cleaner.

## Build State (as of HEAD)

| Target          | Status                                                  |
| --------------- | ------------------------------------------------------- |
| `examples-raw`  | ✔ ok — write/read round-trips verified for `double*` and `int32*` paths. |

No external library dependencies.

## Cross-References

- **`examples/datasets/`** — the full `offset`/`count`/`stride`/`block` hyperslab vocabulary used here, with prose.
- **`examples/optimized/`** — the inner-loop pattern of repeated raw-pointer writes into a chunked dataset.
- **`examples/packet-table/packet_batches.cpp`** — raw-pointer `h5::append(pt, ptr)` used to stream chunks at maximum throughput.
- **`h5cpp/H5Dwrite.hpp` / `H5Dread.hpp`** — the pointer overloads invoked by `h5::write<T>(fd, path, T*, ...)` / `h5::read<T>(fd, path, T*, ...)`.
