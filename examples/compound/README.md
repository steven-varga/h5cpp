# Compound Datasets

This example shows the small set of moves needed to store C++ structs in HDF5. The point is simple: a user-defined struct becomes an HDF5 compound type, the compound type is reflected by the H5CPP compiler, and the same struct round-trips through `h5::write` / `h5::read` without anyone touching `H5Tinsert`.

Two tiers are covered:

- **Tier-1 (POD)** — flat, fixed-layout structs with arithmetic fields and C arrays. The compiler emits a `register_struct<T>()` specialization. Single `H5Dwrite`/`H5Dread` per call.
- **Tier-2 (non-POD)** — structs with `std::string` or `std::vector<T>` fields. The compiler emits `h5::scatter<T>` / `h5::gather<T>` specializations that walk the live object and serialise the variable-length parts.

## Files

| File           | Purpose                                                                |
| -------------- | ---------------------------------------------------------------------- |
| `compound.cpp` | Tier-1 + Tier-2 examples — create, write, read, scatter/gather         |
| `pod.h`        | Tier-1 POD struct declarations (`sn::example::record_t` and friends)   |
| `non-pod.h`    | Tier-2 struct with VLEN fields (`sn::sensor::timeseries_t`)            |
| `generated.h`  | H5CPP-compiler output: `register_struct<T>` + `scatter<T>` / `gather<T>` |

## Includes

```cpp
#include <armadillo>     // optional: pull in your linalg lib first
#include "pod.h"
#include "non-pod.h"
#include <h5cpp/all>
#include "generated.h"
```

`<h5cpp/all>` pulls in everything h5cpp needs. The compiler-emitted `generated.h` carries the compound descriptors and the scatter/gather bodies for the structs h5cpp saw in this TU; it follows the h5cpp includes.

## Tier-1: POD Structs

The on-disk layout follows the C++ layout. Nested namespaces, typedefs, fixed-size C arrays, and nested structs are all handled:

```cpp
namespace sn::example {
    struct record_t {
        my_uint_t           idx;
        float               field_02[7];      // fixed C array
        sn::other::record_t field_03[5];      // nested struct
        sn::other::record_t field_04[5];      // duplicate: compiler dedupes the type
        other::record_t     field_05[3][8];   // array of arrays
    };
}
```

### Create / Write / Read

```cpp
h5::fd_t fd = h5::create("compound.h5", H5F_ACC_TRUNC);

// Explicit dataset creation — chunked + gzip, ready for partial I/O.
h5::create<sn::example::record_t>(fd, "/orm/chunked_2D",
    h5::current_dims{NROWS, NCOLS},
    h5::chunk{1, CHUNK_SIZE} | h5::gzip{8});

// One-shot create + write of a vector of structs.
std::vector<sn::example::record_t> records =
    h5::pod<sn::example::record_t>{} | h5::take(8);

h5::write(fd, "orm/partial/vector one_shot", records);

// Read it back into the same vector type.
auto data = h5::read<std::vector<sn::example::record_t>>(
    fd, "/orm/partial/vector one_shot");
```

`h5::pod<T>{} | h5::take(n)` is the same generator pipe used elsewhere in the examples (from `H5Uall.hpp`). It hands back `std::vector<T>` of default-constructed records — handy for round-trip tests.

### Property-List Argument Order

Property-list fragments do not care about order. Both of these are equivalent:

```cpp
h5::write(fd, "path", records,
    h5::max_dims{H5S_UNLIMITED}, h5::gzip{9} | h5::chunk{20});

h5::write(fd, "path", records,
    h5::chunk{20} | h5::gzip{9}, h5::max_dims{H5S_UNLIMITED},
    h5::stride{6}, h5::block{4}, h5::current_dims{100}, h5::offset{2});
```

The dispatch parses by argument type at compile time. No runtime ordering cost.

## Tier-2: Structs with Variable-Length Fields

POD structs are written field-for-field. Structs with `std::string` or `std::vector<T>` cannot be — the variable-length fields are pointers into separately-allocated storage. The H5CPP compiler handles this by emitting scatter/gather specializations that walk the live object.

```cpp
namespace sn::sensor {
    struct [[h5::doc("Time-series sensor reading with variable-length fields"),
             h5::chunk(128),
             h5::compress("gzip", 6)]] timeseries_t {
        unsigned long long              timestamp_ns;
        [[h5::name("label")]] std::string tag;
        [[h5::ignore]] int              internal_id;   // not persisted
        std::vector<double>             readings;
    };
}
```

The C++ attributes drive the on-disk shape:

| Attribute                       | Effect                                                |
| ------------------------------- | ----------------------------------------------------- |
| `[[h5::doc("...")]]`            | Set on the struct's HDF5 documentation                |
| `[[h5::chunk(128)]]`            | Default chunk shape for datasets of this struct      |
| `[[h5::compress("gzip", 6)]]`   | Default filter chain for datasets of this struct     |
| `[[h5::name("label")]]`         | Rename a field on the HDF5 side                       |
| `[[h5::ignore]]`                | Skip this field — not persisted                       |

### Scatter / Gather

For tier-2 structs you call the compiler-generated entry points directly:

```cpp
sn::sensor::timeseries_t ts;
ts.timestamp_ns = 1'700'000'000'000'000'000ULL;
ts.tag          = "accelerometer";
ts.internal_id  = 42;                  // [[h5::ignore]] — not persisted
ts.readings     = {1.0, 2.0, 3.0, 4.0, 5.0};

h5::scatter(fd, "/sensor/timeseries", ts);   // write

sn::sensor::timeseries_t back;
h5::gather(fd, "/sensor/timeseries", back);   // read
```

The declarations live in h5cpp; the *bodies* are emitted by the H5CPP compiler into `generated.h` based on what it sees in `non-pod.h`. If you change the struct, regenerate `generated.h`.

## How the Two Tiers Differ

```text
Tier-1 POD (flat layout):           Tier-2 non-POD (variable-length):
    record_t  →  H5T_COMPOUND           timeseries_t  →  H5T_COMPOUND
    field-for-field memcpy              scatter walks live object,
    single H5Dwrite                     emits H5T_VARIABLE + H5T_VLEN
                                        per field as needed
```

Tier-1 uses the regular `h5::write` / `h5::read` API. Tier-2 uses `h5::scatter` / `h5::gather` because it has work to do per element that the generic path cannot.

## Build Notes

The example is wired into CMake as `examples-compound` and depends on Armadillo (used by some of the compound generator helpers). Running it produces `compound.h5` in the current directory.

```sh
cd <build-dir>
./examples-compound
h5dump -pH compound.h5
```

## Mental Model

```text
struct       →  HDF5 compound type
fixed POD    →  field-for-field memcpy        →  h5::write / h5::read
VLEN fields  →  per-element scatter/gather    →  h5::scatter / h5::gather
attributes   →  on-disk shape / filter / name →  [[h5::chunk(...)]], [[h5::name(...)]], [[h5::ignore]]
```

User code writes a regular C++ struct. The H5CPP compiler reads it, decides which tier it belongs to, and emits the matching HDF5 type descriptors and serialisation bodies into `generated.h`. The dispatch path is then identical to writing any other C++ object.
