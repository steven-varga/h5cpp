# Half-Precision Floats — Third-Party Variants

This directory predates C++23's `std::float16_t`. It exists for users on toolchains that don't yet ship `<stdfloat>`.

## The modern path

For C++23 toolchains (libstdc++ 13+, libc++ 18+), use `std::float16_t` directly. h5cpp already ships an `h5::dt_t<std::float16_t>` spec in `h5cpp/H5Tall.hpp` that emits a 16-bit IEEE binary16 layout. See:

- `examples/datatypes/` — section 5 demonstrates the round-trip
- `h5cpp/H5Tall.hpp:181` — the spec itself

```cpp
#include <stdfloat>
#include <h5cpp/all>

std::vector<std::float16_t> vec = { /* ... */ };
h5::write(fd, "/half/float16", vec);
auto back = h5::read<std::vector<std::float16_t>>(fd, "/half/float16");
```

## The third-party variants

The two subdirectories here show the same idea using vendored half-float libraries — useful when your toolchain doesn't have `<stdfloat>` yet:

| Variant         | Backing type            | Source                                                   |
| --------------- | ----------------------- | -------------------------------------------------------- |
| `christian-rau/` | `half_float::half`      | http://half.sourceforge.net/ (header-only)               |
| `open-exr/`      | `Imath::half`           | https://github.com/AcademySoftwareFoundation/Imath       |

Both use exactly the same h5cpp specialization pattern as the C++23 native path — specialize `h5::impl::detail::hid_t<T, H5Tclose, true, true, hdf5::type>` and have the ctor emit a 16-bit IEEE binary16 type id. The wrappers in each subdirectory's `utils.hpp` are the reference for hooking any third-party half-float library that does not ship its own h5cpp glue.

## When to use which

| You have… | Use… |
| --- | --- |
| GCC 13+, Clang 18+, MSVC with `/std:c++latest` | `examples/datatypes/` section 5 (native `std::float16_t`) |
| An older toolchain + Christian Rau's `half.hpp` | `examples/half-float/christian-rau/` |
| OpenEXR / Imath already in your project | `examples/half-float/open-exr/` |

For the general "how does h5cpp let me add a custom datatype?" question, the focused tutorial is in `examples/datatypes/` — these subdirectories are deliberately narrow.
