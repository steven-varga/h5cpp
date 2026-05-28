# Custom Datatypes

This example walks the HDF5 datatype-customization surface. The point is simple: HDF5 has more type primitives than the obvious native scalars, and every one of them is reachable through a single macro in h5cpp.

```cpp
H5CPP_REGISTER_DATATYPE(YourType, "YourType",
    /* CREATE_EXPR — any expression returning an hid_t */,
    /* BODY        — brace-enclosed setup with `handle` in scope, or {} */)
```

Four arguments:

| Arg            | What                                                                            |
| -------------- | ------------------------------------------------------------------------------- |
| `TYPE`         | C++ type to register                                                            |
| `NAME`         | String literal printed by `h5::name<TYPE>::value`                               |
| `CREATE_EXPR`  | Expression returning an HDF5 `hid_t` (`H5Tcopy`, `H5Tcreate`, `H5Tenum_create`) |
| `BODY`         | Brace-enclosed setup with `handle` in scope; use `{}` if nothing to do          |

Drop that in, and `h5::write(fd, path, v)`, `h5::read<YourType>`, and `h5::read<std::vector<YourType>>` all work as if `YourType` were a native scalar.

The macro expands to the underlying boilerplate (a `dt_t<T>` specialization + `h5::name<T>` spec). The raw form is shown at the bottom of this README under [What the macro expands to](#what-the-macro-expands-to) — useful when debugging a registration or working in environments that disallow macros.

## Files

| File                | Purpose                                                                |
| ------------------- | ---------------------------------------------------------------------- |
| `datatypes.cpp`     | Six sections, one HDF5 datatype primitive per section                  |
| `custom_types.hpp`  | The four user-defined types + their `dt_t<T>` specializations          |
| `README.md`         | This file                                                              |

## Includes

```cpp
#include "custom_types.hpp"
#include <h5cpp/all>
```

The header pulls in `<h5cpp/core>` itself because the `dt_t<T>` specializations need `dt_p<T>` to inherit from. `<h5cpp/all>` provides the IO surface.

## The HDF5 Datatype Primitives

| HDF5 primitive          | When you reach for it                                  | h5cpp pattern                                       |
| ----------------------- | ------------------------------------------------------ | --------------------------------------------------- |
| `H5Tcopy(H5T_NATIVE_*)` | Strong-typedef a native scalar with a domain name      | `dt_t<Celsius>` over float                          |
| `H5Tset_precision`      | Tighten a uchar/uint to N bits (paired with `h5::nbit`)| `dt_t<n_bit>` over uchar with precision 2           |
| `H5T_OPAQUE`            | Bytes that HDF5 should not interpret                   | `dt_t<two_bit>` 1-byte opaque blob                  |
| `H5Tenum_create` + `H5Tenum_insert` | Named enumerated values, integer payload   | `dt_t<Status>` over uint8_t with four entries       |
| Native vendor types     | `std::float16_t` (C++23), other small floats           | already shipped in `h5cpp/H5Tall.hpp`               |

Other primitives belong elsewhere in the example tree:

- `H5T_COMPOUND` from a POD struct → `examples/compound/`
- `H5T_STRING` fixed and VLEN → `examples/string/`, `examples/csv/`
- Third-party half-float vendor libraries → `examples/half-float/`

## 1. Strong Typedef — `Celsius`

A wrapper around `float` with the same memory layout but a distinct identity in C++ and a distinct name on disk:

```cpp
struct Celsius {
    float value;
    explicit Celsius(float v) : value(v) {}
    explicit operator float() const { return value; }
};

H5CPP_REGISTER_DATATYPE(Celsius, "Celsius",
    H5Tcopy(H5T_NATIVE_FLOAT), {})
```

No memory overhead, no runtime cost. `h5dump` shows the dataset's type as `Celsius` rather than `float`.

```cpp
std::vector<Celsius> temps = { Celsius{-40.0f}, Celsius{20.5f}, Celsius{100.0f} };
h5::write(fd, "/strong_typedef/temps", temps);
auto back = h5::read<std::vector<Celsius>>(fd, "/strong_typedef/temps");
```

## 2. N-bit Packed Integer — `bitstring::n_bit`

`n_bit` is a `uchar` with `H5Tset_precision(handle, 2)` — the on-disk type carries only the bottom two bits per element. Combined with `h5::nbit` on the dcpl, the chunked dataset compresses out the unused bits:

```cpp
H5CPP_REGISTER_DATATYPE(bitstring::n_bit, "bitstring::n_bit",
    H5Tcopy(H5T_NATIVE_UCHAR),
    {
        H5Tset_precision(handle, 2);   // 2-bit precision
    })
```

```cpp
h5::ds_t ds = h5::create<bs::n_bit>(fd, "/packed/nbit",
    h5::current_dims{N},
    h5::chunk{N} | h5::nbit);     // h5::nbit needs chunked layout
```

Useful for low-cardinality categorical data, sample-rate-quantised audio, or anywhere you'd otherwise reach for bit-packing by hand.

## 3. Opaque Bytes — `bitstring::two_bit`

`H5T_OPAQUE` tells HDF5 the byte is uninterpreted. h5cpp passes it through. The tag is metadata for tools like `h5dump`:

```cpp
H5CPP_REGISTER_DATATYPE(bitstring::two_bit, "bitstring::two_bit",
    H5Tcreate(H5T_OPAQUE, 1),
    {
        H5Tset_tag(handle, "bitstring::two_bit");
    })
```

`two_bit` packs four 2-bit fields into a single byte; h5cpp doesn't need to know about that. The wrapper exposes `operator[]` so user code can read out the four fields, but the on-disk representation is one uninterpreted byte.

## 4. Naming + Introspection

`h5::name<T>::value` is the compile-time string. `h5::dt_t<T>` instances stream a runtime description of the HDF5 type they wrap — handy for logs and for verifying which spec the dispatch picked:

```cpp
std::cout << h5::name<Celsius>::value << "\n";       // "Celsius"
std::cout << h5::dt_t<Celsius>() << "\n";            // ebias, precision, sizes…
```

Specialize `h5::name<T>` once per custom type. The runtime introspection comes free from `dt_t<T>`'s stream insertion operator.

## 5. Half-Precision Float — `std::float16_t` (C++23)

Already shipped: `h5cpp/H5Tall.hpp` has a `dt_t<std::float16_t>` spec that emits a 16-bit IEEE binary16 layout. Gated on `__STDCPP_FLOAT16_T__` (libstdc++ 13+, libc++ 18+):

```cpp
#if defined(__STDCPP_FLOAT16_T__)
#include <stdfloat>

std::vector<std::float16_t> vec = {
    std::float16_t{-3.14f}, std::float16_t{0.0f},
    std::float16_t{1.0f},   std::float16_t{2.71828f}
};
h5::write(fd, "/half/float16", vec);
auto back = h5::read<std::vector<std::float16_t>>(fd, "/half/float16");
#endif
```

When the toolchain lacks `<stdfloat>`, the example prints "skipped" with a pointer to `examples/half-float/`. That directory contains pre-C++23 demos using `half_float::half` (Christian Rau) and `Imath::half` (OpenEXR) — same `dt_t<T>` pattern, vendored third-party header.

## 6. Strong Enum — `Status` → `H5T_ENUM`

HDF5 has a native enumerated type. The integer payload round-trips through `h5::write` / `h5::read`; `h5dump` prints the names:

```cpp
enum class Status : std::uint8_t {
    Inactive = 0, Active = 1, Pending = 2, Failed = 3,
};

H5CPP_REGISTER_DATATYPE(Status, "Status", H5Tenum_create(H5T_NATIVE_UINT8),
    {
        Status v;
        v = Status::Inactive; H5Tenum_insert(handle, "Inactive", &v);
        v = Status::Active;   H5Tenum_insert(handle, "Active",   &v);
        v = Status::Pending;  H5Tenum_insert(handle, "Pending",  &v);
        v = Status::Failed;   H5Tenum_insert(handle, "Failed",   &v);
    })
```

```cpp
std::vector<Status> states = { Status::Active, Status::Pending, Status::Failed };
h5::write(fd, "/enum/states", states);
```

`h5dump -p datatypes.h5` shows:

```text
DATATYPE  H5T_ENUM {
   H5T_STD_U8LE;
   "Inactive"   0;
   "Active"     1;
   "Pending"    2;
   "Failed"     3;
}
```

## Build Notes

CMake target `examples-datatypes`. Built at C++23 so the `std::float16_t` section can compile; the half-float path is feature-gated on `__STDCPP_FLOAT16_T__`. No linalg dependencies — pure STL.

```sh
cd <build-dir>
./examples-datatypes
h5dump -pH datatypes.h5
```

## What the macro expands to

`H5CPP_REGISTER_DATATYPE(Celsius, "Celsius", H5Tcopy(H5T_NATIVE_FLOAT), {})` expands to:

```cpp
namespace h5 {
    template <> struct name<Celsius> {
        static constexpr char const* value = "Celsius";
    };
}
namespace h5::impl::detail {
    template <>
    struct hid_t<Celsius, H5Tclose, true, true, hdf5::type>
        : public dt_p<Celsius>
    {
        using parent  = dt_p<Celsius>;
        using dt_p<Celsius>::hid_t;
        using hidtype = Celsius;
        hid_t() : parent( H5Tcopy(H5T_NATIVE_FLOAT) ) {}
    };
}
```

12 lines collapsed into 3. Useful to know when:

- Debugging a registration with `-E` preprocessor output
- Writing a registration for a type that takes multiple template arguments (commas inside the macro confuse the preprocessor — alias the type via `using` first, or hand-roll the spec)
- Working in an environment that disallows macros (rare, but worth supporting)

The macro lives in `h5cpp/H5Tall.hpp` next to the existing `H5CPP_REGISTER_STRUCT` and `H5CPP_REGISTER_TYPE` macros — same family, same conventions.

## Mental Model

```text
your C++ type
    │
    ▼
H5CPP_REGISTER_DATATYPE(T, "T", CREATE_EXPR, BODY)
    │
    ▼ expands to a dt_t<T> specialization
HDF5 type descriptor (hid_t produced by CREATE_EXPR, refined by BODY)
    │
    ▼
h5::write(fd, path, value)
h5::read<T>(fd, path)
h5::read<std::vector<T>>(fd, path)
```

One macro per type. The `H5T*` factory call captures *what* the type is on disk; the body captures *how* HDF5 should treat it. Same shape across all four custom types — only the third and fourth arguments change.
