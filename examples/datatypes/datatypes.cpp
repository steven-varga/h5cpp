// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// =============================================================================
// h5cpp custom datatype tutorial
// =============================================================================
//
// The HDF5 type system has more primitives than the obvious native scalars.
// h5cpp surfaces all of them through a single specialization point:
//
//   h5::impl::detail::hid_t<T, H5Tclose, true, true, hdf5::type>
//
// Specialize that struct's default constructor to emit any HDF5 type id, and
// h5cpp will treat T as a first-class value: h5::write(fd, path, v),
// h5::read<T>, h5::read<std::vector<T>>, all of it.
//
// Six sections, one primitive per section:
//
//   1. Strong typedef       — Celsius        H5Tcopy(H5T_NATIVE_FLOAT)
//   2. N-bit packed integer — bitstring::n_bit   H5Tset_precision(t, 2)
//   3. Opaque bytes         — bitstring::two_bit H5Tcreate(H5T_OPAQUE, 1)
//   4. Naming + introspection — h5::name<T> + stream insertion
//   5. Half-precision float — std::float16_t (C++23 gated)
//   6. Strong enum          — Status           H5Tenum_create + H5Tenum_insert
//
// The four specializations live in custom_types.hpp.

#include "custom_types.hpp"
#include <h5cpp/all>

#include <iomanip>
#include <iostream>
#include <vector>

#if defined(__STDCPP_FLOAT16_T__)
    #include <stdfloat>
#endif

namespace {

void section(const char* title) {
    std::cout << "\n" << title << "\n"
              << std::string(std::strlen(title), '-') << "\n";
}

} // namespace

int main() {
    auto fd = h5::create("datatypes.h5", H5F_ACC_TRUNC);

    // ── 1. strong typedef ───────────────────────────────────────────────────
    // Celsius wraps a float. Distinct identity in C++, same memory layout, and
    // the on-disk type descriptor carries the "Celsius" name (see section 4).
    section("1. strong typedef — Celsius");
    {
        std::vector<Celsius> temps = {
            Celsius{-40.0f}, Celsius{0.0f}, Celsius{20.5f}, Celsius{100.0f}};
        h5::write(fd, "/strong_typedef/temps", temps);

        auto back = h5::read<std::vector<Celsius>>(fd, "/strong_typedef/temps");
        std::cout << "  wrote: ";
        for (auto& c : temps) std::cout << c << " ";
        std::cout << "\n  read:  ";
        for (auto& c : back)  std::cout << c << " ";
        std::cout << "\n";
    }

    // ── 2. N-bit packed integer ─────────────────────────────────────────────
    // bitstring::n_bit lives in a uchar but only the bottom two bits are
    // significant. With h5::nbit on the dcpl, h5cpp packs the unused bits out
    // of the chunk — per-cell precision tightening.
    section("2. N-bit packed integer — bitstring::n_bit");
    {
        namespace bs = bitstring;
        std::vector<bs::n_bit> values = {bs::n_bit{0}, bs::n_bit{1},
                                         bs::n_bit{2}, bs::n_bit{3},
                                         bs::n_bit{1}, bs::n_bit{0}};

        h5::ds_t ds = h5::create<bs::n_bit>(fd, "/packed/nbit",
            h5::current_dims{values.size()}, h5::chunk{values.size()} | h5::nbit);   // nbit needs chunked layout
        h5::write(ds, values);

        auto back = h5::read<std::vector<bs::n_bit>>(fd, "/packed/nbit");
        std::cout << "  wrote: ";
        for (auto& v : values) std::cout << v << " ";
        std::cout << "\n  read:  ";
        for (auto& v : back)   std::cout << v << " ";
        std::cout << "\n  storage: H5T_NATIVE_UCHAR with H5Tset_precision(2)\n";
    }

    // ── 3. opaque bytes ─────────────────────────────────────────────────────
    // two_bit packs four 2-bit values into a single byte. H5T_OPAQUE means
    // HDF5 treats the byte as an uninterpreted blob — bring your own arithmetic.
    section("3. opaque bytes — bitstring::two_bit");
    {
        namespace bs = bitstring;
        std::vector<bs::two_bit> blobs = {bs::two_bit{0b00'01'10'11},
                                          bs::two_bit{0b11'11'00'00},
                                          bs::two_bit{0b10'10'01'01}};
        h5::write(fd, "/opaque/two_bit", blobs);

        auto back = h5::read<std::vector<bs::two_bit>>(fd, "/opaque/two_bit");
        std::cout << "  wrote: ";
        for (auto& b : blobs) std::cout << "[" << b << "] ";
        std::cout << "\n  read:  ";
        for (auto& b : back)  std::cout << "[" << b << "] ";
        std::cout << "\n  storage: H5T_OPAQUE (1 byte) tagged \"bitstring::two_bit\"\n";
    }

    // ── 4. naming + introspection ───────────────────────────────────────────
    // h5::name<T>::value is the compile-time string. h5::dt_t<T> instances
    // stream a runtime description of the HDF5 type they wrap — handy for
    // logging and for sanity-checking that the dispatch picked the spec you
    // intended.
    section("4. naming + introspection");
    {
        std::cout << "  compile-time names:\n";
        std::cout << "    " << h5::name<float>::value             << "\n";
        std::cout << "    " << h5::name<Celsius>::value           << "\n";
        std::cout << "    " << h5::name<bitstring::n_bit>::value  << "\n";
        std::cout << "    " << h5::name<bitstring::two_bit>::value << "\n";
        std::cout << "    " << h5::name<Status>::value            << "\n";

        std::cout << "\n  runtime dt_t<T> introspection:\n";
        std::cout << "    float    -> " << h5::dt_t<float>()    << "\n";
        std::cout << "    Celsius  -> " << h5::dt_t<Celsius>()  << "\n";
        std::cout << "    n_bit    -> " << h5::dt_t<bitstring::n_bit>()  << "\n";
        std::cout << "    two_bit  -> " << h5::dt_t<bitstring::two_bit>() << "\n";
        std::cout << "    Status   -> " << h5::dt_t<Status>()   << "\n";
    }

    // ── 5. half-precision float (C++23 std::float16_t) ─────────────────────
    // h5cpp ships an h5::dt_t<std::float16_t> spec in H5Tall.hpp that emits a
    // 16-bit IEEE-754 binary16 layout via H5Tcopy(H5T_NATIVE_FLOAT) +
    // H5Tset_fields/precision/ebias/size. Round-trips through std::vector.
    //
    // Gated on __STDCPP_FLOAT16_T__ (libstdc++ 13+, libc++ 18+). When the
    // toolchain lacks <stdfloat>, see examples/half-float/ for third-party
    // half-float library variants using the same dt_t<T> pattern.
#if defined(__STDCPP_FLOAT16_T__)
    section("5. half-precision float — std::float16_t");
    {
        std::vector<std::float16_t> vec = {std::float16_t{-3.14f},
                                            std::float16_t{0.0f},
                                            std::float16_t{1.0f},
                                            std::float16_t{2.71828f}};
        h5::write(fd, "/half/float16", vec);

        auto back = h5::read<std::vector<std::float16_t>>(fd, "/half/float16");
        std::cout << "  wrote: ";
        for (auto v : vec)  std::cout << static_cast<float>(v) << " ";
        std::cout << "\n  read:  ";
        for (auto v : back) std::cout << static_cast<float>(v) << " ";
        std::cout << "\n  storage: 2-byte IEEE binary16\n";
    }
#else
    section("5. half-precision float — std::float16_t");
    std::cout << "  skipped: this TU was not built with __STDCPP_FLOAT16_T__\n";
    std::cout << "  see examples/half-float/ for third-party variants\n";
#endif

    // ── 6. strong enum → H5T_ENUM ──────────────────────────────────────────
    // Status maps to H5T_ENUM over uint8_t with the four values inserted as
    // named entries. The integer payload round-trips; h5dump shows the names.
    section("6. strong enum — H5T_ENUM");
    {
        std::vector<Status> states = {Status::Active, Status::Pending,
                                       Status::Failed, Status::Active,
                                       Status::Inactive};
        h5::write(fd, "/enum/states", states);

        auto back = h5::read<std::vector<Status>>(fd, "/enum/states");
        std::cout << "  wrote: ";
        for (auto s : states) std::cout << s << " ";
        std::cout << "\n  read:  ";
        for (auto s : back)   std::cout << s << " ";
        std::cout << "\n  storage: H5T_ENUM over H5T_NATIVE_UINT8\n";
        std::cout << "           (h5dump prints names; round-trip preserves values)\n";
    }

    std::cout << "\nWrote everything to ./datatypes.h5\n";
    std::cout << "Inspect with:  h5dump -pH datatypes.h5\n";
    return 0;
}
