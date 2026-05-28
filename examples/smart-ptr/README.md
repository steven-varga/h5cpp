# Smart-Pointer Support

h5cpp accepts `std::unique_ptr<T[]>`, `std::shared_ptr<T[]>`, `std::unique_ptr<T>`, and `std::shared_ptr<T>` anywhere a raw `T*` is accepted today — and also supports a return-style read that allocates the smart pointer for you, sized to the dataset extent.

```cpp
// Forwarding path — same call shape as h5::write/read for any other type:
std::unique_ptr<double[]> src(new double[16]());
h5::write(fd, "/path", src, h5::count{16});

std::unique_ptr<double[]> dst(new double[16]());
h5::read(fd, "/path", dst, h5::count{16});

// Auto-allocating return-style read — the library sizes the buffer for you:
auto buf = h5::read<std::unique_ptr<float[]>>(fd, "/some/path");
```

All four smart-pointer flavours are supported (`unique_ptr<T[]>`, `shared_ptr<T[]>`, `unique_ptr<T>`, `shared_ptr<T>`).

## Files

| File              | What it teaches                                                         |
| ----------------- | ----------------------------------------------------------------------- |
| `smart_ptr.cpp`   | Five round-trips covering every supported smart-pointer flavour, including the auto-allocating return-style read. |

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-smart-ptr
./examples-smart-ptr
```

Expected output:

```
✔ ok    std::unique_ptr<double[]>(16)   write + read
✔ ok    std::shared_ptr<int[]>(12)      write + read
✔ ok    std::unique_ptr<double>         single scalar
✔ ok    std::shared_ptr<int64_t>        single scalar
✔ ok    auto h5::read<unique_ptr<float[]>>(fd, path)
✔ ok    unique_ptr<sn::point_t[]>(5)    pre-alloc + read
✔ ok    auto h5::read<unique_ptr<sn::point_t[]>>(...)  compound POD
```

## The Four Stories

### 1. Forwarding overloads — array smart pointers

`h5::write` / `h5::read` overloads for `std::unique_ptr<T[]>` and `std::shared_ptr<T[]>` call `.get()` internally and delegate to the existing raw-pointer paths. The smart pointer is just a typed wrapper around the same `T*` that those paths already accept.

```cpp
std::shared_ptr<int[]> v(new int[N], std::default_delete<int[]>());
h5::write(fd, "/v", v, h5::count{N});      // forwards to write<int>(fd, "/v", v.get(), count)
h5::read (fd, "/v", v, h5::count{N});      // forwards to read <int>(fd, "/v", v.get(), count)
```

The smart pointer carries **no length**, so the explicit `h5::count{...}` is mandatory. There is no shape deduction from the pointer alone — the caller must say what to move.

### 2. Auto-allocating return-style read

`h5::read<std::unique_ptr<T[]>>(fd, path)` constructs a freshly-allocated buffer sized to the dataset's HDF5 extent, then reads into it. Useful when you want a clean owning handle without pre-allocating.

```cpp
auto buf = h5::read<std::unique_ptr<float[]>>(fd, "/path");
// buf is now a std::unique_ptr<float[]> sized to product(dataset.shape).
```

Under the hood the dispatch in `H5Dread.hpp` line 638 calls `impl::get<std::unique_ptr<T[]>>::ctor(count)` (registered in `H5Mmemory.hpp`), which does `new T[total]` with the size derived from the dataset extent.

This path is **read-only** — `h5::write` cannot use it because the smart pointer carries no length on write. Always pair the auto-alloc read with a separate length variable if you need the size downstream:

```cpp
auto buf = h5::read<std::unique_ptr<float[]>>(fd, "/path");
auto ds  = h5::open(fd, "/path");
h5::current_dims_t shape;
h5::get_simple_extent_dims(h5::get_space(ds), shape);
// shape[0] now carries the length you need.
```

### 3. Single-element smart pointers

`std::unique_ptr<T>` / `std::shared_ptr<T>` (no `[]`) are treated as a 1-element dataset. The forwarding overload synthesises `h5::count{1}` automatically:

```cpp
auto p = std::make_unique<double>(3.14159);
h5::write(fd, "/scalar", p);             // dataset is shape (1,) double
auto q = std::make_unique<double>(0.0);
h5::read (fd, "/scalar", q);             // *q == 3.14159
```

Useful for niche cases (heap-resident scalar protocol fields, weak-ref scratch variables). For most code use a stack scalar and the existing `h5::write(fd, path, value)` surface.

### 4. Composition with the compiler-assisted reflection path

Smart pointers compose cleanly with `H5CPP_REGISTER_STRUCT` — the registration declares the HDF5 compound type for `T`, and the smart-pointer mapper carries `T*` through the same way it would for any other primitive.

```cpp
struct point_t { double x, y, z; };
H5CPP_REGISTER_STRUCT(point_t);   // h5cpp compiler emits this into generated.h

std::unique_ptr<point_t[]> records(new point_t[N]());
// ... populate ...
h5::write(fd, "/points", records, h5::count{N});

auto back = h5::read<std::unique_ptr<point_t[]>>(fd, "/points");
```

On disk this lands as:

```
DATASET "/points" {
   DATATYPE  H5T_COMPOUND {
      H5T_IEEE_F64LE "x";
      H5T_IEEE_F64LE "y";
      H5T_IEEE_F64LE "z";
   }
   DATASPACE  SIMPLE { ( N ) / ( N ) }
}
```

Section 6 of `smart_ptr.cpp` exercises both a pre-allocated read and the auto-allocating return-style read on a compound POD type.

**What does *not* work**: a compound struct whose *field* is a smart pointer.

```cpp
struct broken_t {
    int id;
    std::unique_ptr<double> weight;   // ✘ failed — non-POD, non-standard-layout
};
// H5CPP_REGISTER_STRUCT(broken_t);   // would not compile / would not produce a useful mapping
```

`unique_ptr` / `shared_ptr` aren't standard-layout, they have non-trivial destructors, and HDF5's compound type model has no representation for pointer-typed fields. Smart pointers can hold a struct or an array of structs; they cannot live *inside* one.

### 5. Mixed ownership

`std::shared_ptr<T[]>` makes the most sense when the buffer outlives the immediate I/O scope — for example, a reader that hands the buffer to multiple consumers, or a producer ring-buffer where readers all hold a `shared_ptr`. The library treats `shared_ptr` and `unique_ptr` identically at the I/O boundary; ownership semantics are entirely the caller's concern.

## What's Where in the Library

| Layer                      | File                                  | What it adds                                                                  |
| -------------------------- | ------------------------------------- | ----------------------------------------------------------------------------- |
| Trait registrations        | `h5cpp/H5Mmemory.hpp`                 | `impl::rank`, `impl::decay`, `impl::get<>::ctor`, `impl::data`, `access_traits_t`, `storage_representation_impl` for all four smart-pointer flavours. Included from `h5cpp/core`. |
| Forwarding write/read      | `h5cpp/H5Mmemory_io.hpp`              | Free-function overloads of `h5::write` / `h5::read` for each smart-pointer flavour. Pulled in from `h5cpp/io` so the raw-pointer paths they delegate to are already declared. |

Including `<h5cpp/all>` brings both in automatically.

## Limitations

| Limitation                                              | Why                                                                                   |
| ------------------------------------------------------- | ------------------------------------------------------------------------------------- |
| `h5::write` always needs an explicit `h5::count{...}`.  | The smart pointer doesn't store a length. There's nothing for the library to deduce.  |
| Auto-allocating read uses `new T[n]()`.                 | Value-initialises every element to zero. If you want unintialised memory for speed, pre-allocate yourself and use the forwarding overload. |
| `std::unique_ptr<T>` / `shared_ptr<T>` (single) maps to a 1-element dataset. | Not a scalar attribute — it's a `(1,)` 1-D dataset. Use h5cpp's scalar surface for true scalar attributes. |
| No support for `std::weak_ptr`.                         | Read/write require live ownership; the weak handle would have to be locked first.     |

## Build State (as of HEAD)

| Target                | Status                                                          |
| --------------------- | --------------------------------------------------------------- |
| `examples-smart-ptr`  | ✔ ok — five round-trips, all four smart-pointer flavours pass.  |

No external library dependencies.

## Cross-References

- **`h5cpp/H5Mmemory.hpp` / `H5Mmemory_io.hpp`** — implementation.
- **`examples/raw_memory/`** — the raw-pointer surface this builds on. Same `h5::write` / `h5::read` shape with `T*` instead of `unique_ptr<T[]>`.
- **`examples/container/`** — the same I/O dispatch surface for STL containers (which carry their own size and don't need an explicit `h5::count`).
