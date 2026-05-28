# Optimized Inner-Loop I/O

This example shows two equivalent ways to write a slab into a dataset from inside a tight loop. The point is to make the cost of the h5cpp call shape visible — and to show that the obvious, readable version is already cheap.

```cpp
// SUGGESTED                                  // EXTREME
for (short i = 0; i < N; ++i) {               h5::offset offset{0,0};
    M.fill(i + 1);                            h5::count  count {10,1};
    h5::write(ds, M,                          for (short i = 10; i < 15; ++i) {
        h5::offset{0, hsize_t(i)});               offset[1] = hsize_t(i);
}                                                 M.fill(i + 1);
                                                  h5::write(ds, M, offset, count);
                                              }
```

Both produce identical on-disk data. Use the inline version unless a profiler tells you otherwise.

## Why the Inline Version Is Cheap

`h5::offset_t`, `h5::count_t`, `h5::stride_t`, `h5::block_t` are aliases for `h5::impl::array<TAG, H5CPP_MAX_RANK>` — a `(rank + H5CPP_MAX_RANK * hsize_t)` POD that lives on the stack. Constructing one from a brace-init list is a memcpy of at most seven `hsize_t`s.

Inside `h5::write`, the dispatch is driven by `arg::tpos<...>` template metaprogramming. Each argument is detected at compile time:

```cpp
if constexpr (arg::tpos<const h5::offset_t&, args_t...>::present) { /* ... */ }
if constexpr (arg::tpos<const h5::stride_t&, args_t...>::present) { /* ... */ }
```

When an argument isn't passed, the whole branch is eliminated. The generated machine code is what you'd hand-write against the HDF5 C API — no runtime argument parsing, no allocation.

## What the Example Writes

A 10 × ∞ dataset of `short`, chunked `{10, 10}` + gzip-9, fill-value 0.

| Pattern   | Loop range | Writes column...     | Content of that column |
| --------- | ---------- | -------------------- | ---------------------- |
| SUGGESTED | `i = 0..3` | column `i`           | filled with `i + 1`    |
| EXTREME   | `i=10..14` | column `i`           | filled with `i + 1`    |

After both loops, row 0 across columns 0..14 reads `[1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 11, 12, 13, 14, 15]`.

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-optimized
./examples-optimized
```

Expected output:

```
col[0..14](row 0): [1,2,3,4,0,0,0,0,0,0,11,12,13,14,15]
column 11        : [12,12,12,12,12,12,12,12,12,12]
```

Inspect the file:

```sh
h5dump -d "huge dataset" optimized.h5 | head
```

## When to Hoist

The EXTREME pattern only matters when:

- The loop runs millions of iterations.
- A profiler attributes time to argument construction (rare — these are stack memcpys).
- You're chaining many optional arguments and constant-folding still leaves visible cost.

For most code the SUGGESTED pattern is what you want. The compiler inlines the optional argument construction; the only difference at -O2 is a few extra spills, well below noise.

## Why the Old Version Was Subtly Wrong

The previous `optimized.cpp` did `M[0,0] = i` and `M[1,0] = i` to "do your science thing." In C++17 those expressions use the **comma operator** — `M[0,0]` parses as `M[(0,0)]` = `M[0]`, ignoring the second index entirely. The example still ran, but the per-iteration mutation only touched element `M(0)`, so the file ended up with values in row 0 only and zeros elsewhere — not the per-column fill the prose claimed.

Use arma's function-call syntax (`M(r, c) = v`) or `M.fill(v)` when you mean to write the whole vector. C++23 introduces real multi-arg subscript but Armadillo doesn't expose it.

## Cross-References

- **`examples/datasets/`** — full coverage of the `offset` / `count` / `stride` / `block` vocabulary used here.
- **`examples/linalg/arma.cpp`** — the same arma round-trip without the hoisting concern.
- **`h5cpp/H5Sall.hpp`** — defines the `impl::array<TAG>` POD that backs every dispatch argument; see `H5CPP_MAX_RANK`.
