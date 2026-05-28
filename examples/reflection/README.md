# Reflection — Compiler-Assisted HDF5 Serialisation

C++ already knows the layout of your structs. HDF5 does not. Bridging the two by hand means maintaining a parallel schema in `H5Tinsert` calls that drifts out of sync every time a field is added, renamed, or reordered.

h5cpp removes that tax. You write ordinary C++ structs with small `[[h5::...]]` annotations; the H5CPP compiler scans them, classifies each struct by its fields, and emits the correct HDF5 compound descriptors and serialisation bodies into `generated.h`. Your C++ type stays the single source of truth.

## The Problem

Storing a struct in HDF5 without reflection looks like this:

```cpp
hid_t ct = H5Tcreate(H5T_COMPOUND, sizeof(my_struct));
H5Tinsert(ct, "field_a", HOFFSET(my_struct, field_a), H5T_NATIVE_INT);
H5Tinsert(ct, "field_b", HOFFSET(my_struct, field_b), H5T_NATIVE_DOUBLE);
// ... repeat for every field, every nested struct, every array dimension
```

Change one member and the descriptor silently corrupts. Add `std::string` or `std::vector<T>` and the problem escalates: you now need `hvl_t` relays, chunked extendable datasets, append logic, unpack logic, and HDF5 memory reclamation — all by hand.

## What h5cpp Does

Write the struct once, with annotations where the default is wrong:

```cpp
namespace sn::iot {
    struct [[h5::alias("dev"), h5::version("1.0.0")]] device_t {
        unsigned long long       device_id;
        [[h5::name("fw")]] float firmware_version[3];
        double                   calibration[6];
        short                    region_code;
    };
}
```

The compiler emits the descriptor into `generated.h`:

```cpp
namespace h5 {
    template<> hid_t inline register_struct<sn::iot::device_t>() {
        hid_t at_00 = H5Tarray_create(H5T_NATIVE_FLOAT, 1, hsize_t[]{3});
        hid_t at_01 = H5Tarray_create(H5T_NATIVE_DOUBLE, 1, hsize_t[]{6});
        hid_t ct = H5Tcreate(H5T_COMPOUND, sizeof(sn::iot::device_t));
        H5Tinsert(ct, "device_id",    HOFFSET(sn::iot::device_t, device_id),    H5T_NATIVE_ULLONG);
        H5Tinsert(ct, "fw",           HOFFSET(sn::iot::device_t, firmware_version), at_00);
        H5Tinsert(ct, "calibration",  HOFFSET(sn::iot::device_t, calibration),  at_01);
        H5Tinsert(ct, "region_code",  HOFFSET(sn::iot::device_t, region_code),  H5T_NATIVE_SHORT);
        H5Tclose(at_00); H5Tclose(at_01);
        return ct;
    };
}
H5CPP_REGISTER_STRUCT(sn::iot::device_t);
```

You do not write, review, or maintain that code. The compiler generates it when `types.h` changes.

## Tier Model

h5cpp classifies every struct into one of two tiers before emission.

```text
Tier-1 (POD)                          Tier-2 (VLEN)
    C++ aggregate                         C++ aggregate with std::string / std::vector<T>
    -> HDF5 compound type                 -> generated row_t mirror with hvl_t fields
    -> h5::write / h5::read               -> generated scatter<T> / gather<T>
                                          -> chunked, extendable HDF5 dataset
```

**Tier-1** covers arithmetic fields, fixed C arrays, nested POD structs, and the `serialize_full` escape hatch. The emitted `register_struct<T>()` registers a compound type with h5cpp's normal `h5::write` / `h5::read` path.

**Tier-2** covers structs with `std::string` or `std::vector<T>` fields. The compiler emits a `row_t` mirror, a `compound_type()` helper that creates `H5T_VARIABLE` and `H5T_VLEN` base types, and `scatter<T>` / `gather<T>` bodies that marshal the live object. Example:

```cpp
sn::iot::event_t event{...};
h5::scatter(fd, "/events", event);   // append row

sn::iot::event_t back;
h5::gather(fd, "/events", back);     // read last row
```

## Files

| File             | Purpose                                                                          |
| ---------------- | -------------------------------------------------------------------------------- |
| `types.h`        | Annotated C++ record declarations — tier-1 POD, tier-2 VLEN, nested POD, `serialize_full`, `name_all`, `on_missing` |
| `reflection.cpp` | Round-trip demo for reflected structs and generic library types                  |
| `generated.h`    | H5CPP compiler output: compound descriptors, VLEN mirrors, scatter/gather bodies |

Include order:

```cpp
#include "types.h"
#include <h5cpp/all>
#include "generated.h"   // must follow <h5cpp/all>
```

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-reflection
./examples-reflection
```

The executable writes `reflection.h5` in the current directory. Inspect the schema:

```sh
h5dump -H reflection.h5
```

## Expected Output

```text
tier-1 (POD compound):
    wrote 4 device_t rows
    read  4 device_t rows back
    devices[0].region_code = 0

tier-2 (VLEN compound):
    last event timestamp     = 1700000001000000000
    source (renamed in file) = "rack-3.sensor-2"
    connection_attempts      = 0   (zero-init; [[h5::ignore]])
    payload.size      = 2
    temperatures.size = 1
    vibrations.size   = 0
    error_codes.size  = 1

tier-1 with name_all:
    wrote 3 sensor_t rows
    read  3 sensor_t rows back
    sensors[0].value = 98.6

tier-2 with name_all:
    last session label   = "session-alpha"
    readings.size        = 3
    internal_id          = 0   (zero-init; [[h5::ignore]])

on_missing("ignore"):
    gather on absent path returned early
    probe.codes.size = 0

tier-1 nested POD:
    wrote 2 install_t rows
    read  2 install_t rows back
    installs[0].device.device_id = 0xdeadbeef

tier-1 serialize_full (opaque blob):
    wrote raw_blob_t as opaque compound (sizeof = 72)
    non-POD fields (label, samples) skipped by compiler

std::tuple scalar:
    read back = (42, 3.14, 'x')

std::vector<std::tuple<int,float>>:
    wrote 3 tuples
    read  3 tuples back

std::vector<std::complex<double>>:
    read back = [(1, 2), ...]

std::map<int,double>:
    wrote 3 entries
    read  3 entries back
    m[2] = 2.2

std::set<int>:
    wrote 5 unique entries
    read  5 unique entries back

std::vector<std::string>:
    read back = ["alpha", "beta", "gamma"]

std::vector<std::vector<double>> (ragged VLEN):
    rows = 3
    row[0].size = 1
    row[1].size = 2
    row[2].size = 3
```

## Annotation Vocabulary

| Attribute                        | Scope  | Effect                                                            |
| -------------------------------- | ------ | ----------------------------------------------------------------- |
| `[[h5::doc("...")]]`             | struct | Documentation propagated into `generated.h`                       |
| `[[h5::alias("...")]]`           | struct | Logical name for the generated namespace                          |
| `[[h5::version("...")]]`         | struct | Schema version metadata in generated output                       |
| `[[h5::name("...")]]`            | field  | Rename one field on disk; overrides `name_all`                    |
| `[[h5::name_all("pre", "suf")]]` | struct | Prefix/suffix applied to every field name                         |
| `[[h5::ignore]]`                 | field  | Omit the field from persistence                                   |
| `[[h5::chunk(N)]]`               | struct | Chunk size for tier-2 datasets                                    |
| `[[h5::compress("gzip", N)]]`    | struct | Compression filter for tier-2 datasets                            |
| `[[h5::on_missing("create")]]`   | struct | Create dataset when missing                                       |
| `[[h5::on_missing("ignore")]]`   | struct | Return early when dataset is missing                              |
| `[[h5::serialize_full]]`         | struct | Force tier-1 emission; non-POD fields are silently skipped        |

## Type-System Dispatch (No Compiler Needed)

The second half of the example demonstrates h5cpp's generic `access_traits_t` dispatch. These types round-trip through `h5::write` / `h5::read` without compiler assistance:

| Type                                  | Storage model            | Mechanism                          |
| ------------------------------------- | ------------------------ | ---------------------------------- |
| `std::tuple<int, double, char>`       | scalar compound          | `pack` / `unpack` flat buffer      |
| `std::vector<std::tuple<int, float>>` | rank-1 compound          | `elem_traits::pack` each element   |
| `std::vector<std::complex<double>>`   | rank-1 compound/native   | direct write/read                  |
| `std::map<int, double>`               | key-value compound       | `{ key, value }` rows              |
| `std::set<int>`                       | rank-1 dataset           | staged iterator write              |
| `std::vector<std::string>`            | VLEN text dataset        | `char*` relay + reclaim            |
| `std::vector<std::vector<double>>`    | ragged VLEN dataset      | `hvl_t` relay + reclaim            |

The compiler is for your domain structs — the things HDF5 cannot infer and C++17 cannot reflect. Standard containers are already handled by the library.

## Manual vs. Compiler-Assisted

| Change                     | Manual HDF5 cost                 | Compiler-assisted cost            |
| -------------------------- | -------------------------------- | --------------------------------- |
| Add POD field              | Add `H5Tinsert` manually         | Rebuild                           |
| Rename field on disk       | Edit string literal manually     | Add `[[h5::name("...")]]`         |
| Add fixed array            | Create `H5Tarray_create`         | Rebuild                           |
| Add nested struct          | Build nested compound            | Rebuild                           |
| Add `std::string`          | Write VLEN string plumbing       | Rebuild                           |
| Add `std::vector<T>`       | Write `hvl_t` packing/unpacking  | Rebuild                           |
| Add chunking               | Edit DCPL manually               | Add `[[h5::chunk(N)]]`            |
| Add compression            | Edit DCPL manually               | Add `[[h5::compress("gzip", N)]]` |
| Skip internal field        | Remember not to insert it        | Add `[[h5::ignore]]`              |
| Change missing-path policy | Edit scatter/gather logic        | Add `[[h5::on_missing("...")]]`   |

## Relation to C++26 Reflection

Today's implementation uses a Clang-based compiler tool that parses `[[h5::...]]` attributes and emits `generated.h`. Under C++26 (P2996 + P3394), the same work moves into the language itself:

```cpp
// C++26 — no external compiler tool needed
struct [[=h5::name_all{"sn_", ""}]] sensor_t {
    [[=h5::name{"lbl"}]] float label;
    // ...
};
```

`std::meta::members_of(^sensor_t)` will enumerate fields at compile time, read annotations via `std::meta::annotations_of`, and produce the same `dt_t<T>` and `scatter<T>` specialisations that `generated.h` contains today. The H5CPP compiler becomes an optional convenience; the vocabulary stays identical. This example is therefore both a practical C++17 tool and a preview of the zero-tooling path that C++26 unlocks.

## CMake Wiring

```cmake
add_h5cpp_example(reflection reflection/reflection.cpp
    GENERATED reflection/generated.h
    COMPILER_SOURCES reflection/types.h)
```

The `GENERATED` line tells CMake to invoke the H5CPP compiler before compiling the example. When `types.h` changes, `generated.h` is re-emitted automatically.

## Build State

| Target                | Status                                                                 |
| --------------------- | ---------------------------------------------------------------------- |
| `examples-reflection` | OK — tier-1, tier-2, annotations, and type-system round-trips verified |

No external dependencies.

## Cross-References

- `examples/compound/` — smaller tier-1 / tier-2 reflection example
- `examples/attributes/` — exhaustive type-check matrix for `register_struct`
- `examples/container/` — generic STL and structural container dispatch
- `examples/multi-tu/` — generated descriptor use across multiple translation units
- `tasks/h5cpp-compiler-h5-attribute-taxonomy.md` — full annotation vocabulary
- `tasks/h5cpp-type-system-architecture-notes.md` — kind × storage dispatch matrix
