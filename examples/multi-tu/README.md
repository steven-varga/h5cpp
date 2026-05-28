@page example_guide_multi_tu Multi-Translation-Unit Compound Types

This example proves a single property of h5cpp: **you can include the same `generated.h` (the file that registers your POD structs as HDF5 compound types) in any number of translation units, and the linker will not complain.**

That property is what makes h5cpp usable in real codebases. A single-TU example wouldn't show it — for that you need at least two `.cpp` files that both touch the type system, linked together into one executable.

```
struct.h         POD layout (no h5cpp dependency)
generated.h      H5CPP_REGISTER_STRUCT bodies for both record types
main.cpp         driver — knows only h5::fd_t, doesn't include generated.h
tu-01.cpp        includes generated.h + armadillo; creates compound datasets + linalg
tu-02.cpp        includes generated.h independently; writes/reads a POD vector
```

The lesson is in the link step: `tu-01.o` and `tu-02.o` each carry their own copy of the `register_struct<sn::example::record_t>()` and `register_struct<sn::typecheck::record_t>()` definitions. The linker keeps one and discards the rest because the bodies are `inline`. Nothing else is required — no `static`, no anonymous namespace, no per-TU header guards.

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-multi-tu
./examples-multi-tu
```

Expected output:

```
tu-01: wrote arma::imat(20x5) and created two compound datasets
tu-02: read back 8 records, idx = [0,1,2,3,4,5,6,7]
```

Inspect the resulting `multi-tu.h5`:

```sh
h5dump -pH multi-tu.h5
```

You should see `/linalg/armadillo` (20×5 i64), `/orm/chunked_2D` (compound, chunked + gzipped), `/orm/typecheck` (unlimited 1-D), and `/orm/partial/{one_shot,custom_dims}` (compound vectors).

## Why a Single Shared `generated.h`

Earlier versions of this example shipped two separate generated headers (`tu-01.h`, `tu-02.h`), each with its own `H5CPP_GUARD_*` include guard. The intent was to show that a code generator could emit a header per TU. In practice that pattern is more confusing than the truth:

- The `H5CPP_REGISTER_STRUCT(...)` body is `inline`, so a single shared header is ODR-safe by construction.
- Per-TU header guards do nothing the macro expansion doesn't already handle.
- One header to maintain is strictly easier than N.

Modern h5cpp examples (`compound/`, `csv/`, `stl/`) all use one shared `generated.h`. This one now matches.

## What Each TU Needs

| TU         | Includes        | Why                                                                             |
| ---------- | --------------- | ------------------------------------------------------------------------------- |
| `main.cpp` | `<h5cpp/all>`   | Only handles `h5::fd_t`; never names a compound type, so `generated.h` is dead weight here. |
| `tu-01.cpp`| `<armadillo>`, `<h5cpp/all>`, `"generated.h"` | Mixes a linalg write with `h5::create<record_t>` calls in the same TU.        |
| `tu-02.cpp`| `<h5cpp/all>`, `"generated.h"` | Pure compound vector path — write, read, project a scalar field.              |

`generated.h` itself pulls in `<h5cpp/all>` and `"struct.h"`, so a TU that includes `generated.h` doesn't need to repeat them.

## Old Pattern → New Pattern

Pre-refactor each `.cpp` was structured like this:

```cpp
#include "struct.h"
#include <h5cpp/core>
    #include "tu-XX.h"      // generator output had to live INSIDE the sandwich
#include <h5cpp/io>
```

The sandwich-between-`core`-and-`io` requirement is gone. The new shape is:

```cpp
#include <h5cpp/all>        // pulls in core + io in the right order
#include "generated.h"      // can sit anywhere after <h5cpp/all>
```

That same shape works whether you have one TU or twenty.

## Build State (as of HEAD)

| Target              | Status                                                |
| ------------------- | ----------------------------------------------------- |
| `examples-multi-tu` | ✔ ok — links two TUs sharing `generated.h`, runs clean |

Gated on `ARMADILLO_FOUND` in `examples/CMakeLists.txt:377-388`.

## Cross-References

- **`examples/compound/`** — the single-TU version, same `record_t` types, same `generated.h` style. Start there if you're new to the compound path.
- **`examples/csv/`** — another example that uses one shared `generated.h` (with a simpler struct).
- **`h5cpp/H5Tall.hpp`** — `H5CPP_REGISTER_STRUCT` and `register_struct<>` live here; this is where the `inline` lives that makes the multi-TU story work.

## Source

- [`generated.h`](generated_8h-example.html) — rendered with syntax highlighting
- [`main.cpp`](main_8cpp-example.html) — rendered with syntax highlighting
- [`struct.h`](struct_8h-example.html) — rendered with syntax highlighting
- [`tu-01.cpp`](tu_01_8cpp-example.html) — rendered with syntax highlighting
- [`tu-02.cpp`](tu_02_8cpp-example.html) — rendered with syntax highlighting
