@page reports_compiler_msgpack_attribute_taxonomy msgpack:: Attribute Vocabulary (MessagePack Backend)

User-facing attribute set for MessagePack annotations on plain C++ structs. **Vocabulary is intentionally identical to `h5::*`, `json::*`, and `pb::*` where the concept overlaps** (rename, ignore, doc, alias, required) — different namespace, same words. The MessagePack-specific surface lives only under `msgpack::*`, with `msgpack::ext(N)` being the only backend-specific keyword (extension types are core to MessagePack's type system).

C++17 attribute syntax today; one-line lift to C++26 typed annotations tomorrow.

| Surface today (C++17 standard-attribute) | C++26 reflection form |
|---|---|
| `[[msgpack::name("on_wire")]]` | `[[=msgpack::name{"on_wire"}]]` |
| `[[msgpack::ignore]]` | `[[=msgpack::ignore{}]]` |
| `[[msgpack::required]]` | `[[=msgpack::required{}]]` |
| `[[msgpack::ext(1)]]` | `[[=msgpack::ext{1}]]` |
| `[[msgpack::doc("description")]]` | `[[=msgpack::doc{"description"}]]` |
| `[[msgpack::alias("Name")]]` | `[[=msgpack::alias{"Name"}]]` |

Only syntactic shift is `(args)` → `{args}` under the `[[=...]]` form. **Names stay put.**

---

## 2. Universal vocabulary — same words, `msgpack::` namespace

These attributes use vocabulary identical to `h5::*`, `json::*`, and `pb::*`. They live in `msgpack::` so the namespace stays self-contained for MessagePack-only users; a user wanting multiple backends writes `[[h5::name(...)]]`, `[[json::name(...)]]`, and `[[msgpack::name(...)]]` (typically with the same string).

### Universal Tier 1 — must-have

| Attribute | Purpose | Example |
|---|---|---|
| `[[msgpack::name("on_wire_name")]]` | Rename a field for the MessagePack wire format. Decouples C++ identifier from the map key used during pack/unpack. Drives the key string in the emitted descriptor's `json_name` field. | `[[msgpack::name("display_name")]] std::string label;` |
| `[[msgpack::ignore]]` | Skip this field entirely. Property absent from the descriptor's `fields[]` array; runtime never packs or unpacks it. | `[[msgpack::ignore]] int debug_counter;` |
| `[[msgpack::required]]` | Field must be present during deserialization. The runtime can use this to emit an error (or a default value) when the key is absent from the MessagePack map. | `[[msgpack::required]] std::int32_t id;` |

### Universal Tier 2 — high value, low cost

| Attribute | Purpose | Example |
|---|---|---|
| `[[msgpack::doc("description")]]` | Emitted as the `doc` pointer in the field descriptor. Self-documenting generated code; future tooling may extract it for schema documentation. | `[[msgpack::doc("nanoseconds since epoch")]] std::uint64_t ts;` |
| `[[msgpack::alias("Name")]]` | **Class-level.** Emitted as the `alias[]` string in the descriptor. The C++ type name still drives the template specialization; the alias is metadata for tooling. | `struct [[msgpack::alias("Session")]] session_t { ... };` |

The full universal list mirrors `h5cpp-compiler-h5-attribute-taxonomy.md` §2 and `h5cpp-compiler-pb-attribute-taxonomy.md` §2. Any universal attribute not listed above has no MessagePack semantics (e.g. `h5::chunk`, `h5::compress` are HDF5-storage concerns; `pb::field(N)`, `pb::wire` are protobuf-wire concerns; `json::format`, `json::pattern` are JSON Schema validation concerns).

---

## 3. MessagePack-specific vocabulary

### Tier 1 — must-have

Without `msgpack::ext`, the MessagePack backend can't express extension types — a core MessagePack feature for custom binary payloads, timestamps, and user-defined types.

| Attribute | Purpose | Example |
|---|---|---|
| `[[msgpack::ext(N)]]` | **Class-level.** Marks the struct as a MessagePack extension type with type byte `N` (range `[-128, 127]`). The struct's fields describe the binary payload layout; the runtime packs the payload as ext data with the given type byte. Nested structs without `ext` are emitted as nested maps. | `struct [[msgpack::ext(1)]] timestamp_t { std::int64_t seconds; std::int32_t nanos; };` |

**Extension type semantics.** When a field's type is a struct annotated with `[[msgpack::ext(N)]]`, the descriptor emits `mp_type_t::ext` with `ext_type == N`. The runtime serializes the struct's fields into a binary payload, wraps it in a MessagePack ext format (`fixext`, `ext8`, `ext16`, or `ext32` depending on payload size), and writes it to the wire. On deserialization, the runtime reads the ext type byte, verifies it matches `N`, then unpacks the payload according to the struct's descriptor.

---

## 4. Type map — C++ → MessagePack

| C++ type | `mp_type_t` | MessagePack format | Notes |
|---|---|---|---|
| `bool` | `boolean` | `true` / `false` | |
| `char`, `signed char` | `int8` | `fixint` / `int8` | |
| `unsigned char` | `uint8` | `fixint` / `uint8` | |
| `short` | `int16` | `int16` | |
| `unsigned short` | `uint16` | `uint16` | |
| `int` | `int32` | `fixint` / `int32` | |
| `unsigned int` | `uint32` | `fixint` / `uint32` | |
| `long` | `int64` | `fixint` / `int64` | Platform-dependent width; canonicalized to `long`'s actual size by Clang |
| `unsigned long` | `uint64` | `fixint` / `uint64` | Platform-dependent width; canonicalized |
| `long long` | `int64` | `int64` | |
| `unsigned long long` | `uint64` | `uint64` | |
| `float` | `float32` | `float32` | |
| `double`, `long double` | `float64` | `float64` | `long double` is truncated to 64-bit |
| `std::string` | `str` | `fixstr` / `str8` / `str16` / `str32` | UTF-8 string |
| `std::vector<unsigned char>` | `bin` | `bin8` / `bin16` / `bin32` | Raw binary blob |
| `std::vector<T>` | `array` | `fixarray` / `array16` / `array32` | `item` descriptor points to element type |
| `T[N]` (C array) | `array` | `fixarray` / `array16` / `array32` | Same emission as `std::vector<T>` |
| `std::map<K,V>` | `map` | `fixmap` / `map16` / `map32` | `key` and `value` descriptors |
| `std::optional<T>` | `optional` | `nil` or `<T>` | `item` descriptor points to inner type |
| `enum class` | `int32` | `fixint` / `int32` | Emitted as underlying integer type; no string mapping today |
| Nested struct `S` (no `ext`) | `object` | `fixmap` / `map16` / `map32` | Recursively serialized as nested map |
| Nested struct `S` (`[[msgpack::ext(N)]]`) | `ext` | `fixext` / `ext8` / `ext16` / `ext32` | Binary payload described by `S`'s descriptor |
| Pointer `T*` | `nil` | `nil` | Fallback. Pointers have no natural MessagePack representation. |
| `std::variant<...>` | `nil` | `nil` | **Gap.** Not yet implemented. |

---

## 5. Descriptor shape

The compiler emits a self-contained C++ header defining `msgpack::meta::descriptor<T>` specializations. The runtime (deferred to a future issue) will include these headers and walk the descriptors at pack/unpack time.

```cpp
namespace msgpack::meta {

enum class mp_type_t : std::uint8_t {
    nil, boolean,
    int8, int16, int32, int64,
    uint8, uint16, uint32, uint64,
    float32, float64,
    str, bin, array, map, object, ext, optional
};

struct field_desc {
    const char*   json_name;
    mp_type_t     type;
    std::uint8_t  ext_type;
    std::size_t   offset;
    bool          required;
    const char*   doc;
    const field_desc* item;     // array element, optional inner type
    const field_desc* key;      // map key
    const field_desc* value;    // map value
};

template<typename T>
struct descriptor {
    static constexpr char alias[] = "";
    static constexpr std::uint8_t ext_id = 0;
    static constexpr field_desc fields[] = {};
    static constexpr std::size_t field_count = 0;
};

} // namespace msgpack::meta
```

Example specialization for a struct with an extension type:

```cpp
template<>
struct descriptor<timestamp_t> {
    static constexpr std::uint8_t ext_id = 1;
    static constexpr field_desc fields[] = {
        { "seconds", mp_type_t::int64, 0, offsetof(timestamp_t, seconds), false, nullptr, nullptr, nullptr, nullptr },
        { "nanos",   mp_type_t::int32, 0, offsetof(timestamp_t, nanos),   false, nullptr, nullptr, nullptr, nullptr }
    };
    static constexpr std::size_t field_count = 2;
};

template<>
struct descriptor<event_t> {
    static constexpr field_desc fields[] = {
        { "when", mp_type_t::ext, 1, offsetof(event_t, when), true, nullptr, nullptr, nullptr, nullptr },
        { "name", mp_type_t::str, 0, offsetof(event_t, name), false, nullptr, nullptr, nullptr, nullptr }
    };
    static constexpr std::size_t field_count = 2;
};
```

---

## 6. Attribute wiring status

### Implemented and tested

| Attribute | Where read | Where emitted | Test fixture |
|---|---|---|---|
| `msgpack::ignore` | `h5_attr_reader::has_attr(fld, "msgpack::ignore")` | Skips field in `fields[]` | `msgpack_primitives` |
| `msgpack::required` | `h5_attr_reader::has_attr(fld, "msgpack::required")` | Sets `required = true` in field desc | `msgpack_primitives`, `msgpack_strings` |
| `msgpack::name("...")` | `h5_attr_reader::read_field_string(fld, "msgpack::name")` | Overrides `json_name` in field desc | `msgpack_strings` |
| `msgpack::doc("...")` | `h5_attr_reader::read_class_string(node, "msgpack::doc")` | Emitted as `doc` pointer in field desc | `msgpack_primitives`, `msgpack_nested` |
| `msgpack::alias("...")` | `h5_attr_reader::read_class_string(node, "msgpack::alias")` | Emitted as `alias[]` in descriptor | `msgpack_primitives` |
| `msgpack::ext(N)` | `h5_attr_reader::read_class_int(node, "msgpack::ext")` | Emitted as `ext_id` in descriptor; referenced as `ext_type` in field desc | `msgpack_ext` |

### Not applicable to MessagePack

| Attribute | Reason |
|---|---|
| `msgpack::on_missing` | MessagePack has no schema-level default-value mechanism. Absence semantics live in the runtime decoder, not the descriptor. (Same as JSON.) |
| `msgpack::chunk` | HDF5 storage concern. |
| `msgpack::compress` | HDF5 storage concern. |
| `msgpack::serialize_full` | HDF5 tier-1 emission concern. |
| `msgpack::format` | JSON Schema validation concern. |
| `msgpack::pattern` | JSON Schema validation concern. |
| `msgpack::min` / `msgpack::max` | JSON Schema validation concern. |
| `msgpack::version` | No MessagePack schema format to version. |
| `msgpack::name_all` | No wire naming convention needed; MessagePack uses map keys, not field names. |

---

## 7. Worked example — sensor event with timestamp

### Input (user source)

```cpp
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace sn::sensor {
    struct [[msgpack::ext(1)]] timestamp_t {
        std::int64_t seconds;
        std::int32_t nanos;
    };
    struct [[msgpack::doc("Sensor event"), msgpack::alias("Event")]] event_t {
        [[msgpack::required]] timestamp_t when;
        [[msgpack::name("sensor_id")]] std::uint32_t id;
        [[msgpack::ignore]] int debug_counter;
        std::string label;
        std::vector<double> readings;
        std::optional<std::uint16_t> flags;
    };
} // namespace sn::sensor
```

### Emitted output (descriptor header)

```cpp
#pragma once

/* Generated by h5cpp-compiler MessagePack backend */

#include <cstddef>
#include <cstdint>

namespace msgpack::meta {
    enum class mp_type_t : std::uint8_t {
        nil, boolean,
        int8, int16, int32, int64,
        uint8, uint16, uint32, uint64,
        float32, float64,
        str, bin, array, map, object, ext, optional
    };
    struct field_desc {
        const char*   json_name;
        mp_type_t     type;
        std::uint8_t  ext_type;
        std::size_t   offset;
        bool          required;
        const char*   doc;
        const field_desc* item;
        const field_desc* key;
        const field_desc* value;
    };
    template<typename T>
    struct descriptor {
        static constexpr char alias[] = "";
        static constexpr std::uint8_t ext_id = 0;
        static constexpr field_desc fields[] = {};
        static constexpr std::size_t field_count = 0;
    };
} // namespace msgpack::meta

// descriptor for sn::sensor::timestamp_t
template<>
struct descriptor<sn::sensor::timestamp_t> {
    static constexpr std::uint8_t ext_id = 1;
    static constexpr field_desc fields[] = {
        { "seconds", mp_type_t::int64, 0, offsetof(sn::sensor::timestamp_t, seconds), false, nullptr, nullptr, nullptr, nullptr },
        { "nanos",   mp_type_t::int32, 0, offsetof(sn::sensor::timestamp_t, nanos),   false, nullptr, nullptr, nullptr, nullptr }
    };
    static constexpr std::size_t field_count = 2;
};

// descriptor for sn::sensor::event_t
template<>
struct descriptor<sn::sensor::event_t> {
    static constexpr char alias[] = "Event";
    static constexpr field_desc item_1 { nullptr, mp_type_t::float64, 0, 0, false, nullptr, nullptr, nullptr, nullptr };
    static constexpr field_desc opt_2 { nullptr, mp_type_t::uint16, 0, 0, false, nullptr, nullptr, nullptr, nullptr };
    static constexpr field_desc fields[] = {
        { "when",      mp_type_t::ext,      1, offsetof(sn::sensor::event_t, when),           true,  nullptr, nullptr,       nullptr, nullptr },
        { "sensor_id", mp_type_t::uint32,   0, offsetof(sn::sensor::event_t, id),             false, nullptr, nullptr,       nullptr, nullptr },
        { "label",     mp_type_t::str,      0, offsetof(sn::sensor::event_t, label),          false, nullptr, nullptr,       nullptr, nullptr },
        { "readings",  mp_type_t::array,    0, offsetof(sn::sensor::event_t, readings),       false, nullptr, &item_1,       nullptr, nullptr },
        { "flags",     mp_type_t::optional, 0, offsetof(sn::sensor::event_t, flags),          false, nullptr, &opt_2,        nullptr, nullptr }
    };
    static constexpr std::size_t field_count = 5;
};
```

### Observations from the emitted descriptors

- `timestamp_t` carries `ext_id = 1` from `[[msgpack::ext(1)]]`. The runtime will pack its two fields into an ext payload with type byte `1`.
- `event_t` carries `alias = "Event"` from `[[msgpack::alias("Event")]]`. The C++ template specialization still uses `sn::sensor::event_t`; the alias is metadata.
- `when` → `mp_type_t::ext` with `ext_type = 1`. The runtime sees `ext`, looks up `descriptor<timestamp_t>::ext_id`, and routes through the ext codec.
- `id` → renamed to `"sensor_id"` via `[[msgpack::name("sensor_id")]]`.
- `debug_counter` → absent entirely (`ignore`).
- `label` → `mp_type_t::str`. Standard UTF-8 string.
- `readings` → `mp_type_t::array` with `item = &item_1` where `item_1.type = float64`. The runtime walks the array, packing each element as `float64`.
- `flags` → `mp_type_t::optional` with `item = &opt_2` where `opt_2.type = uint16`. The runtime emits `nil` when the optional is empty, or the `uint16` value when present.
- `doc` pointer is `nullptr` on all fields because no `[[msgpack::doc]]` was applied at field scope. Class-level `doc` is not wired into field descriptors today.

---

## 8. Runtime architecture — Approach B (descriptors)

The h5cpp architectural pattern is **compiler emits descriptors → runtime consumes descriptors → I/O happens**. The MessagePack backend follows this exactly.

### Architecture

```
C++ header + [[msgpack::...]] attributes
        ↓
h5cpp-compiler
        ↓
  ┌─────────────────┐
  │ constexpr desc  │  ← C++17 constexpr type descriptor
  │ (.msgpack.hpp)  │     emitted into a single header
  └─────────────────┘
        ↓
  ┌─────────────────┐
  │ msgpack::runtime│  ← custom pack/unpack (deferred)
  │ (header-only)   │     walks constexpr desc at runtime
  └─────────────────┘
        ↓
   MessagePack bytes ↔ C++ object
```

### Why descriptors over generated code

Same rationale as HDF5, JSON, and protobuf backends:
- **Single source of truth:** One compiler pass produces the descriptor.
- **No generated `.cpp` bloat:** Descriptors are `constexpr` tables; no `O(N_structs × M_fields)` lines of generated code to compile.
- **Introspection:** Descriptors can be walked reflectively for debugging, logging, schema migration.
- **C++26 future:** P2996 reflection makes the constexpr descriptor layer optional — the runtime can reflect directly on `T`.

### Runtime API (sketch — deferred)

```cpp
namespace msgpack {
    // Packing — descriptor-driven
    template<typename T>
    std::vector<uint8_t> pack(const T& obj);

    // Unpacking — descriptor-driven
    template<typename T>
    T unpack(const uint8_t* data, size_t len);
}
```

The actual runtime will use a lightweight custom serializer (not an external library like msgpack-c) to maintain the h5cpp philosophy of minimal dependencies and zero-copy where possible.

---

## 9. Open questions and design decisions

### Decided

| # | Decision | Rationale |
|---|---|---|
| 1 | **Runtime architecture:** Approach B (constexpr descriptors + runtime walk) | Consistent with HDF5, JSON, and protobuf backends. |
| 2 | **Output format:** Self-contained C++ header | The emitted file defines `mp_type_t`, `field_desc`, `descriptor` base template, and all specializations. No external runtime header required at compile time. |
| 3 | **Extension types:** `[[msgpack::ext(N)]]` class-level attribute | Matches MessagePack's native ext type byte. The struct descriptor carries `ext_id`; referencing fields carry `ext_type`. |
| 4 | **`std::vector<uint8_t>` → `bin`** | Auto-detected from element type. Distinguishes binary blobs from arrays of integers. |

### Open

| # | Question | Context |
|---|---|---|
| 1 | **`enum class` string mapping.** | Today enums emit as `int32` (underlying integer). MessagePack has no native enum type, but many protocols use string enums for readability. Should the compiler support an optional `msgpack::enum_as_string` attribute? |
| 2 | **`std::variant<...>` support.** | `std::variant` has no natural MessagePack representation. Options: (a) emit as `ext` with a discriminant byte, (b) emit as a map with a `"_type"` discriminant, (c) defer to user-defined `msgpack::ext` types. |
| 3 | **Packed struct alignment.** | The descriptor carries `offsetof` for each field. If the user compiles with different packing pragmas on different platforms, `offsetof` may disagree. Should the descriptor include `alignof` or `sizeof` for each field as a cross-check? |
| 4 | **Timestamp ext type collision.** | `msgpack::ext(1)` is the same type byte as MessagePack's built-in timestamp ext (`-1` in signed form). Should the compiler reserve certain ext type bytes (e.g. `-1` for timestamp) and map `std::chrono::system_clock::time_point` automatically? |
| 5 | **Map key type restriction.** | MessagePack map keys can be any type, but most protocols restrict keys to strings or integers. Should the compiler emit a diagnostic for non-string/non-integer map keys, or is that the runtime's concern? |
| 6 | **Optional default value.** | `std::optional<T>` fields have no descriptor-level default. If absent on the wire, the runtime leaves the optional empty. Should `msgpack::on_missing` be applicable to optionals (e.g. `[[msgpack::on_missing(42)]] std::optional<int> x;` → absent on wire → `x = 42` instead of `x = std::nullopt`)? |
