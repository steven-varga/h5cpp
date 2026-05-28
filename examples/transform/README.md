@page example_guide_transform Transform — DXPL Expressions and Type-Conversion Callbacks

HDF5's data transfer property list (DXPL) carries two orthogonal in-flight numeric knobs: an element-wise linear expression on the read/write boundary, and a callback HDF5 invokes when an implicit type conversion would lose information. h5cpp wraps both with the same `|`-composable property idiom used everywhere else:

```cpp
arma::mat M(4, 7); M.ones();
h5::write(fd, "/scale_offset/write", M, h5::data_transform{"2*x + 5"});   // disk = 7.0 everywhere
auto m = h5::read<arma::mat>(fd, "/scale_offset/write",
                             h5::data_transform{"x/2 - 1"});              // memory = 2.5

auto narrowed = h5::read<arma::Mat<std::int16_t>>(fd, "/narrow/source",
                  h5::type_conv_cb{{clamp_to_int16, nullptr}});           // clamp instead of throw
```

`data_transform` is HDF5's `H5Pset_data_transform`; the same expression is applied on write (`stored = expr(memory)`) and on read (`memory = expr(stored)`). `type_conv_cb` is HDF5's `H5Pset_type_conv_cb`; HDF5 calls the function once per element whose conversion would otherwise raise an overflow / truncate / NaN / INF exception. Both pass through the DXPL, so they compose with chunk / gzip / MPI collective settings via `|`.

This example wires six self-checking stages into one binary. Every stage prints `✔ ok` or `✘ failed`; the final tally returns non-zero from `main` if any check disagrees.

## HDF5 `data_transform` expression language

| Element | Allowed                                                                                  |
| ------- | ---------------------------------------------------------------------------------------- |
| Variable | `x` — the in-flight element value, evaluated element-wise during `H5Dread` / `H5Dwrite`. |
| Operators | `+`, `-`, `*`, `/`, unary minus, parentheses.                                          |
| Literals | Integer and floating-point constants, e.g. `273.15`, `2`, `-1`.                         |
| Whitespace | Free-form between tokens.                                                              |

The expression is **linear only** per the `H5Pset_data_transform` spec — no `sin`, `log`, `pow`, `exp`, `sqrt`, conditionals, or cross-element references. For anything nonlinear, materialize in C++ after the read.

## `type_conv_cb` callback contract

```cpp
H5T_conv_ret_t cb(H5T_conv_except_t except_type,
                  hid_t src_id, hid_t dst_id,
                  void* src_buf, void* dst_buf,
                  void* op_data);
```

| Return value          | Meaning                                                            |
| --------------------- | ------------------------------------------------------------------ |
| `H5T_CONV_ABORT`      | Propagate the exception; the enclosing `H5Dread`/`H5Dwrite` fails. |
| `H5T_CONV_HANDLED`    | Continue; `dst_buf` carries the value the callback wrote.          |
| `H5T_CONV_UNHANDLED`  | Let HDF5 apply its default behavior for this exception.            |

| `except_type`                  | Triggered when                                                  |
| ------------------------------ | --------------------------------------------------------------- |
| `H5T_CONV_EXCEPT_RANGE_HI`     | Source value exceeds the destination type's max.                |
| `H5T_CONV_EXCEPT_RANGE_LOW`    | Source value is below the destination type's min.               |
| `H5T_CONV_EXCEPT_TRUNCATE`     | Float-to-integer fractional part is dropped.                    |
| `H5T_CONV_EXCEPT_PRECISION`    | Significand bits lost in a narrowing float-to-float conversion. |
| `H5T_CONV_EXCEPT_PINF`         | Source is `+inf`, destination has no representation.            |
| `H5T_CONV_EXCEPT_NINF`         | Source is `-inf`, ditto.                                        |
| `H5T_CONV_EXCEPT_NAN`          | Source is `NaN`, ditto.                                         |

The example's `clamp_to_int16` callback handles `RANGE_HI` / `RANGE_LOW` by writing `INT16_MAX` / `INT16_MIN` into `dst_buf` and returning `H5T_CONV_HANDLED`; everything else falls through to `H5T_CONV_UNHANDLED`.

## Files

| File           | What it covers                                                                                                                                                                                                                                                                                                                                                                                            |
| -------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `transform.cpp` | Six stages: (1) write-side `2*x+5` transform; (2) read-side `x/2-1` transform on the same dataset; (3) round-trip identity via composed write `x*3+2` and read `(x-2)/3`; (4) Celsius ↔ Kelvin unit conversion on the IO boundary; (5) `double` → `int16` narrowing with the `clamp_to_int16` callback handling overflow at both ends; (6) DXPL composition — transform `x*4` on a chunked + gzip dataset. |

The Armadillo dependency is incidental — only `arma::mat` and `arma::Mat<std::int16_t>` are used as containers. The transform and callback features are general h5cpp / HDF5 features and work with any container h5cpp binds (Eigen, `std::vector`, raw pointer + `h5::count`).

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-transform
./examples-transform
```

Expected output:

```
✔ ok      write transform 2*x+5 stored as 7  (expected=7  got=7)
✔ ok      read transform x/2-1 of stored 7  (expected=2.5  got=2.5)
✔ ok      round-trip via (3x+2 write) ∘ ((x-2)/3 read) → identity  (expected=10  got=10)
✔ ok      Celsius 0 → Kelvin 273.15 on disk  (expected=273.15  got=273.15)
✔ ok      Celsius 100 → Kelvin 373.15 on disk  (expected=373.15  got=373.15)
✔ ok      Celsius -273.15 → Kelvin 0 on disk  (expected=0  got=0)
✔ ok      Kelvin → Celsius round-trip  (expected=25  got=25)
✔ ok      int16 in-range value 0  (expected=0  got=0)
✔ ok      int16 in-range value 100  (expected=100  got=100)
✔ ok      int16 overflow clamped to INT16_MAX  (expected=32767  got=32767)
✔ ok      int16 underflow clamped to INT16_MIN  (expected=-32768  got=-32768)
✔ ok      transform composed with chunk + gzip on write  (expected=10  got=10)

✔ all checks passed, errors=0
```

Exit code is the number of failed checks; the example fails its own gate if any round-trip disagrees with the expected value.

## Use cases

- **Unit conversion at the IO boundary.** Application code stays in one unit system; the file is stored in another. The Celsius/Kelvin stage is the canonical pattern — `h5::data_transform{"x + 273.15"}` on write, `h5::data_transform{"x - 273.15"}` on read. Same idea for radians ↔ degrees, USD ↔ cents, meters ↔ millimeters.
- **Scale-and-offset on-disk encoding.** Store a float value as a small integer plus a known affine map (`disk = a*x + b`). Saves space when the dynamic range is small; the transform is applied transparently on read.
- **Overflow clamping when narrowing types.** Reading a `double` column into an `int16_t` container without a callback throws on overflow; the callback lets you choose: clamp, saturate to a sentinel, or abort with a domain-specific error.
- **Filtering NaN / INF on read.** Replace `NaN` with zero (or any value) at the IO boundary via a `type_conv_cb` that handles `H5T_CONV_EXCEPT_NAN` / `PINF` / `NINF` and writes a substitute into `dst_buf`.
- **Quick affine probes during analysis.** Apply `h5::data_transform{"x*1e-6"}` on read to convert micro-units without touching the file or the C++ container.

## Known limitations / gotchas

- **`data_transform` is linear only.** No `sin`, `cos`, `log`, `exp`, `pow`, `sqrt`, conditionals, or absolute value. The HDF5 `H5Pset_data_transform` parser rejects them. For nonlinear transforms, do them in C++ after the read.
- **Element-wise only.** The expression operates on one element at a time; multidimensional or cross-element operations (gradients, sums, neighbor differences) are not expressible. Use `h5::read` followed by C++ for those.
- **Composition order of `h5::data_transform | h5::type_conv_cb` is type-conversion-first, then transform.** Empirically determined under HDF5 1.12, not what one might expect from reading the HDF5 docs. When the transform would push values outside the destination type's range, the callback does **not** fire on the post-transform value — instead the callback fires on the raw conversion overflow first, and the transform is applied to the clamped result. If you need overflow handling on transformed values, materialize in two steps: read into a wider type with the transform applied, then narrow in C++ with your own bounds check.
- **`type_conv_cb` only catches lossy implicit conversions.** A `double` → `double` read with an out-of-range `data_transform` result does not trigger the callback — there is no conversion exception to catch. Range-check the post-transform values in C++ if the transform's output domain is unbounded.
- **No symmetric inverse is enforced.** Writing with `"2*x + 5"` and reading without a transform returns the encoded values, not the original ones. Pair the write transform with its inverse on every read path, or write the inverse expression as an attribute next to the dataset so downstream readers can recover it.

## CMake Wiring

```cmake
if(${ARMADILLO_FOUND})
    add_h5cpp_example(transform transform/transform.cpp
        LIBRARIES libarmadillo)
endif()
```

Lives in `examples/CMakeLists.txt:440-442`. Gated on `ARMADILLO_FOUND` because the example's containers are Armadillo matrices; the transform and callback features themselves require nothing beyond `<h5cpp/all>` and the HDF5 library.

## Build State (as of HEAD)

| Target               | Status                                          |
| -------------------- | ----------------------------------------------- |
| `examples-transform` | ✔ ok — 12 checks pass, exit 0                   |

## Cross-References

- **`h5cpp/H5Pall.hpp:346`** — `h5::type_conv_cb` typedef and its `H5T_conv_ret_t (*)(...)` callback signature.
- **`h5cpp/H5Pall.hpp:351`** — `h5::data_transform` typedef and the `H5Pset_data_transform` wiring.
- **HDF5 DXPL reference** — https://support.hdfgroup.org/documentation/hdf5/latest/group___d_x_p_l.html — the authoritative list of DXPL knobs (`H5Pset_data_transform`, `H5Pset_type_conv_cb`, chunk cache, MPI collective mode, etc.).
- **`examples/datasets/`** — full `h5::chunk` / `h5::gzip` / `h5::offset` / `h5::count` vocabulary; the same `|` composition that pulls `h5::data_transform` into a DXPL pulls those into a DCPL.
- **`examples/optimized/`** — DXPL composition for performance tuning (chunk cache, MPI), the closest neighbor to the stage-6 chunked + gzip + transform pattern in this example.

## Source

- [`transform.cpp`](transform_8cpp-example.html) — rendered with syntax highlighting
