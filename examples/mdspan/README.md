@page example_guide_mdspan mdspan — Non-Owning N-D Views, C++23 P0009 Round-Trip

`std::mdspan` is a non-owning N-D view — `{ pointer, extents, layout, accessor }` — and h5cpp treats it as a contiguous source/sink. The call shape is identical to Armadillo and Eigen, but the type comes from the standard library instead of a vendored linalg dependency. The trait lives in `h5cpp/H5Mmdspan.hpp` and is gated on `__cpp_lib_mdspan >= 202207L`; on stdlibs without `<mdspan>` the header is a no-op and the example prints a skip notice.

```cpp
std::vector<double> buf(rows * cols);
std::mdspan<double, std::dextents<size_t, 2>> view(buf.data(), rows, cols);
h5::write(fd, "/path", view);                          // structural write
// read side — mdspan can't allocate; buffer-out pattern:
h5::read<double>(fd, "/path", view.data_handle(),
                 h5::count{rows * cols});
```

## Write vs read asymmetry

mdspan is non-owning, so there is no `impl::get<mdspan>::ctor` and `h5::read<mdspan>(fd, path)` cannot allocate. Three viable read patterns:

- **Buffer-out** (what the demo uses): allocate via `std::vector<T>`, wrap with mdspan, `h5::read` into `view.data_handle()` with an explicit `h5::count{}`.
- **Read into the wrapped vector, then construct mdspan over it** — same end result, just inverted call order.
- **Read as a contiguous type then re-view**: `auto buf = h5::read<std::vector<T>>(fd, path); std::mdspan view(buf.data(), ...);` — cleanest when the rank/extents are known at read time.

## How the trait dispatches

| h5cpp trait                  | Value for `std::mdspan<T, Extents, ...>`                   |
| ---------------------------- | ----------------------------------------------------------- |
| `access_traits_t::kind`      | `access_t::contiguous`                                      |
| `storage_representation_v`   | `storage_representation_t::linear_value_dataset`            |
| `is_trivially_packable`      | `true`                                                      |
| `traits::data(view)`         | `view.data_handle()`                                        |
| `traits::size(view)`         | `{view.extent(0), view.extent(1), …}` (rank-N `std::array`) |
| `impl::rank`                 | `Extents::rank()`                                           |

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-mdspan
./examples-mdspan
```

Expected output when the stdlib ships `<mdspan>` (libstdc++ ≥ 15, libc++ ≥ 17):

```
✔ ok      rank-2 mdspan write — element count
✔ ok      rank-2 mdspan write — element (0,0) = 0
✔ ok      rank-2 mdspan write — element (3,5) = 23
✔ ok      buffer-out mdspan read — element (0,0)
✔ ok      buffer-out mdspan read — element (3,5)
✔ ok      buffer-out mdspan read — view(2,3) = 15
✔ ok      rank-3 mdspan write — element count
✔ ok      rank-3 mdspan write — first/last match

✔ all checks passed, errors=0
```

On an older stdlib (libstdc++ 14.3 — what CI currently has):

```
◇ skipped — std::mdspan not available on this stdlib.
  Requires libstdc++ >= 15 or libc++ >= 17 (P0009 / __cpp_lib_mdspan >= 202207L).
  h5cpp's H5Mmdspan.hpp guards on the same feature macro and is inert here.
```

The skip branch always returns 0, so the example does not break CI on older stdlibs.

## Files

| File         | What it covers                                                                                          |
| ------------ | ------------------------------------------------------------------------------------------------------- |
| `mdspan.cpp` | Rank-2 mdspan write + read-back, rank-2 buffer-out read into a caller-allocated mdspan, rank-3 write.   |

## Toolchain availability

| Stdlib                  | `<mdspan>` ships? | Status                                                  |
| ----------------------- | ----------------- | ------------------------------------------------------- |
| libstdc++ 14 (gcc 14)   | ✘ no              | ◇ partial — example compiles, prints skip notice         |
| libstdc++ 15 (gcc 15)   | ✔ yes             | ✔ ok                                                    |
| libc++ 17 (clang 17)    | ✔ yes             | ✔ ok                                                    |
| libc++ 18 (clang 18)    | ✔ yes             | ✔ ok                                                    |
| MSVC STL (VS 17.7+)     | ✔ yes             | ✔ ok                                                    |

## Why mdspan and not Armadillo / Eigen here

`std::mdspan` is in the standard library, so this is the path that does not require any vendored linalg dependency. For numeric workloads that already use Armadillo or Eigen, those mappers (`H5Marma.hpp`, `H5Meigen.hpp`) remain the natural fit — they own their storage and have full constructor support on the read side. For new code that wants the modern C++23 surface without pulling a linalg library, mdspan is the answer.

## Limitations

- `h5::read<std::mdspan<...>>(fd, path)` will not work because mdspan cannot allocate. Use the buffer-out pattern above.
- Custom layouts (`std::layout_stride`, `std::layout_left`) are not specifically tested — the trait treats `view.data_handle()` as contiguous, so non-default layouts that produce non-contiguous strides will silently produce wrong on-disk data. Stick to `std::layout_right` (the default) or convert to contiguous before write.
- Partial-IO (`h5::offset` / `h5::count` / `h5::stride`) follows the same conventions as the rest of h5cpp; see `examples/string/README.md` "Partial-IO semantics" for the `count`-means-block discussion.

## CMake Wiring

```cmake
## mdspan — std::mdspan round-trip (C++23; inert on libstdc++ < 15)
add_h5cpp_example(mdspan mdspan/mdspan.cpp)
target_compile_features(examples-mdspan PRIVATE cxx_std_23)
```

Lives in `examples/CMakeLists.txt:473-475`. No library dependencies — pure stdlib plus `<h5cpp/all>`. The `cxx_std_23` target feature is required so the compile sees `<mdspan>` on toolchains that ship it.

## Build State (as of HEAD)

| Target             | Toolchain                        | Status                                                  |
| ------------------ | -------------------------------- | ------------------------------------------------------- |
| `examples-mdspan`  | libstdc++ ≥ 15 / libc++ ≥ 17     | ✔ ok — 8 round-trip checks pass                          |
| `examples-mdspan`  | libstdc++ 14 (current CI)        | ◇ skipped — feature macro absent, demo exits 0 with notice |

## Cross-References

- **`h5cpp/H5Mmdspan.hpp`** — the trait / `access_traits` / `storage_representation_impl` wiring, gated on `__cpp_lib_mdspan`.
- **`examples/linalg/`** — Armadillo / Eigen / Blaze / xtensor mappers covering the same `h5::write` / `h5::read` shape with library-owning types.
- **P0009 / [`std::mdspan` reference](https://en.cppreference.com/w/cpp/container/mdspan)** — the standard's spec and the cppreference summary.
- **`examples/string/README.md`** "Partial-IO semantics" — the h5cpp `count`/`block` convention that applies to mdspan partial IO too.

## Source

- [`mdspan.cpp`](mdspan_8cpp-example.html) — rendered with syntax highlighting
