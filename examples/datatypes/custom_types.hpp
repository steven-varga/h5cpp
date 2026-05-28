// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Custom datatype definitions for the datatypes tutorial.
//
// Pattern: H5CPP_REGISTER_DATATYPE(TYPE, NAME, CREATE_EXPR, BODY)
//
//   TYPE         — your C++ type
//   NAME         — string literal for h5dump / introspection
//   CREATE_EXPR  — any expression returning an HDF5 hid_t
//   BODY         — brace-enclosed setup with `handle` in scope (or {})
//
// Four primitive paths are shown:
//
//   1. Celsius          — strong typedef over float.   H5Tcopy(H5T_NATIVE_FLOAT)
//   2. bitstring::n_bit — N-bit packed integer.        H5Tcopy + H5Tset_precision
//   3. bitstring::two_bit — opaque bytes.              H5Tcreate(H5T_OPAQUE, ...)
//   4. Status (enum class) — named enumerated value.   H5Tenum_create + H5Tenum_insert
//
#pragma once

#include <cstdint>
#include <ostream>

// ─── 1. strong typedef ──────────────────────────────────────────────────────
// Wraps a float to give it a distinct identity in C++ and on disk. Zero memory
// overhead, no runtime cost.
struct Celsius {
    float value;
    Celsius() = default;
    explicit Celsius(float v) : value(v) {}
    explicit operator float() const { return value; }
};

inline std::ostream& operator<<(std::ostream& os, const Celsius& c) {
    return os << c.value << "°C";
}

// ─── 2. N-bit packed integer ────────────────────────────────────────────────
// Lives in a uchar, but H5Tset_precision(2) tells HDF5 the dataset only carries
// the bottom 2 bits per element. Paired with h5::nbit on the dcpl, the file
// compresses the unused bits away — a per-cell alternative to gzip.
namespace bitstring {
    struct n_bit {
        std::uint8_t value;
        n_bit() = default;
        n_bit(std::uint8_t v) : value(v) {}
        explicit operator std::uint8_t() const  { return value; }
        explicit operator unsigned int()  const { return value; }
    };
}

inline std::ostream& operator<<(std::ostream& os, const bitstring::n_bit& v) {
    return os << static_cast<unsigned int>(v.value);
}

// ─── 3. opaque bytes ────────────────────────────────────────────────────────
// H5T_OPAQUE — h5cpp doesn't interpret the payload, it just stores the bytes
// verbatim. The tag is metadata for tools like h5dump; HDF5 itself treats two
// opaque buffers as equal-or-not by tag + size, not by content.
namespace bitstring {
    struct two_bit {
        std::uint8_t value;
        two_bit() = default;
        two_bit(std::uint8_t v) : value(v) {}
        unsigned operator[](int i) const {
            switch (i) {
                case 0: return  value       & 0b11;
                case 1: return (value >> 2) & 0b11;
                case 2: return (value >> 4) & 0b11;
                case 3: return (value >> 6) & 0b11;
            }
            return 0;
        }
    };
}

inline std::ostream& operator<<(std::ostream& os, const bitstring::two_bit& v) {
    return os << v[3] << v[2] << v[1] << v[0];
}

// ─── 4. enum class ─────────────────────────────────────────────────────────
// HDF5 has a native enumerated type (H5T_ENUM): integer payload with named
// values stored in the type descriptor. Tools (h5dump, hdfview) show the
// names, not the integers — useful for status codes, machine state, etc.
enum class Status : std::uint8_t {
    Inactive = 0,
    Active   = 1,
    Pending  = 2,
    Failed   = 3,
};

inline std::ostream& operator<<(std::ostream& os, Status s) {
    switch (s) {
        case Status::Inactive: return os << "Inactive";
        case Status::Active:   return os << "Active";
        case Status::Pending:  return os << "Pending";
        case Status::Failed:   return os << "Failed";
    }
    return os << "?";
}

// ═══ h5cpp registrations ═══════════════════════════════════════════════════
// One H5CPP_REGISTER_DATATYPE call per type. Everything that varies between
// custom datatypes lives in the third and fourth arguments — the rest of the
// boilerplate (specialization head, inheriting ctor, name<T>, hidtype typedef)
// is taken care of by the macro.
#include <h5cpp/core>

H5CPP_REGISTER_DATATYPE(Celsius, "Celsius",
    H5Tcopy(H5T_NATIVE_FLOAT),
    {})

H5CPP_REGISTER_DATATYPE(bitstring::n_bit, "bitstring::n_bit",
    H5Tcopy(H5T_NATIVE_UCHAR),
    {
        H5Tset_precision(handle, 2);
    })

H5CPP_REGISTER_DATATYPE(bitstring::two_bit, "bitstring::two_bit",
    H5Tcreate(H5T_OPAQUE, 1),
    {
        H5Tset_tag(handle, "bitstring::two_bit");
    })

H5CPP_REGISTER_DATATYPE(Status, "Status",
    H5Tenum_create(H5T_NATIVE_UINT8),
    {
        Status v;
        v = Status::Inactive; H5Tenum_insert(handle, "Inactive", &v);
        v = Status::Active;   H5Tenum_insert(handle, "Active",   &v);
        v = Status::Pending;  H5Tenum_insert(handle, "Pending",  &v);
        v = Status::Failed;   H5Tenum_insert(handle, "Failed",   &v);
    })
