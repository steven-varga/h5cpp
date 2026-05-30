@page reports_compiler_cbor_attribute_taxonomy cbor:: Attribute Vocabulary (CBOR Backend)


User-facing attribute set for CBOR annotations on plain C++ structs. **Vocabulary is intentionally identical to `h5::*`, `json::*`, `msgpack::*`, and `pb::*` where the concept overlaps** (rename, ignore, doc, alias, required) — different namespace, same words. The CBOR-specific surface lives only under `cbor::*`, with `cbor::tag(N)` being the only backend-specific keyword. CBOR tags are `uint64_t` (not `int8` like MessagePack ext types), and the type system includes half-precision floats and indefinite-length containers.

C++17 attribute syntax today; one-line lift to C++26 typed annotations tomorrow.

| Surface today (C++17 standard-attribute) | C++26 reflection form |
|---|---|
| `[[cbor::name("on_wire")]]` | `[[=cbor::name{"on_wire"}]]` |
| `[[cbor::ignore]]` | `[[=cbor::ignore{}]]` |
| `[[cbor::required]]` | `[[=cbor::required{}]]` |
| `[[cbor::tag(1)]]` | `[[=cbor::tag{1}]]` |
| `[[cbor::doc("description")]]` | `[[=cbor::doc{"description"}]]` |
| `[[cbor::alias("Name")]]` | `[[=cbor::alias{"Name"}]]` |

Only syntactic shift is `(args)` → `{args}` under the `[[=...]]` form. **Names stay put.**

---

## 2. Universal vocabulary — same words, `cbor::` namespace

These attributes use vocabulary identical to `h5::*`, `json::*`, `msgpack::*`, and `pb::*`. They live in `cbor::` so the namespace stays self-contained for CBOR-only users; a user wanting multiple backends writes `[[h5::name(...)]]`, `[[json::name(...)]]`, `[[msgpack::name(...)]]`, and `[[cbor::name(...)]]` (typically with the same string).

### Universal Tier 1 — must-have

| Attribute | Purpose | Example |
|---|---|---|
| `[[cbor::name("on_wire_name")]]` | Rename a field for the CBOR wire format. Decouples C++ identifier from the map key used during encode/decode. Drives the key string in the emitted descriptor's `json_name` field. | `[[cbor::name("display_name")]] std::string label;` |
| `[[cbor::ignore]]` | Skip this field entirely. Property absent from the descriptor's `fields[]` array; runtime never encodes or decodes it. | `[[cbor::ignore]] int debug_counter;` |
| `[[cbor::required]]` | Field must be present during deserialization. The runtime can use this to emit an error (or a default value) when the key is absent from the CBOR map. | `[[cbor::required]] std::int32_t id;` |

### Universal Tier 2 — high value, low cost

| Attribute | Purpose | Example |
|---|---|---|
| `[[cbor::doc("description")]]` | Emitted as the `doc` pointer in the field descriptor. Self-documenting generated code; future tooling may extract it for schema documentation. | `[[cbor::doc("nanoseconds since epoch")]] std::uint64_t ts;` |
| `[[cbor::alias("Name")]]` | **Class-level.** Emitted as the `alias[]` string in the descriptor. The C++ type name still drives the template specialization; the alias is metadata for tooling. | `struct [[cbor::alias("Session")]] session_t { ... };` |

The full universal list mirrors `h5cpp-compiler-h5-attribute-taxonomy.md` §2 and `h5cpp-compiler-pb-attribute-taxonomy.md` §2. Any universal attribute not listed above has no CBOR semantics (e.g. `h5::chunk`, `h5::compress` are HDF5-storage concerns; `pb::field(N)`, `pb::wire` are protobuf-wire concerns; `json::format`, `json::pattern` are JSON Schema validation concerns; `msgpack::ext` is a MessagePack-specific concern).

---

## 3. CBOR-specific vocabulary

### Tier 1 — must-have

Without `cbor::tag`, the CBOR backend can't express tagged values — a core CBOR feature for timestamps, big numbers, and user-defined types.

| Attribute | Purpose | Example |
|---|---|---|
| `[[cbor::tag(N)]]` | **Class-level.** Marks the struct as a CBOR tagged value with tag `N` (`uint64_t`). The struct's fields describe the payload layout; the runtime encodes the payload as a CBOR value, then wraps it with the tag. Unlike MessagePack `ext`, CBOR tags annotate any value type (not just binary payloads). | `struct [[cbor::tag(1)]] timestamp_t { std::int64_t seconds; std::int32_t nanos; };` |

**Tag semantics.** When a field's type is a struct annotated with `[[cbor::tag(N)]]`, the descriptor emits `cbor_type_t::tag` with `tag_type == N`. The runtime serializes the struct according to its descriptor (as a map or array), then wraps the resulting CBOR value with the tag `N` using major type 6. On deserialization, the runtime reads the tag, verifies it matches `N`, then unpacks the payload according to the struct's descriptor.

**Tag vs. MessagePack ext.** The key difference is that CBOR tags wrap *any* CBOR value, while MessagePack ext types wrap a *binary payload*. For example, `[[cbor::tag(1)]]` on a struct with two integer fields produces a tagged *map* `{seconds: ..., nanos: ...}` on the wire. The equivalent MessagePack `[[msgpack::ext(1)]]` would produce an ext frame containing a binary blob that the runtime unpacks according to the struct's layout.

---

## 4. Type map — C++ → CBOR

| C++ type | `cbor_type_t` | CBOR major type | Notes |
|---|---|---|---|
| `bool` | `boolean` | 7 (true/false) | |
| `char`, `signed char` | `int8` | 0/1 (unsigned/signed int) | |
| `unsigned char` | `uint8` | 0 (unsigned int) | |
| `short` | `int16` | 1 (signed int) | |
| `unsigned short` | `uint16` | 0 (unsigned int) | |
| `int` | `int32` | 1 (signed int) | |
| `unsigned int` | `uint32` | 0 (unsigned int) | |
| `long` | `int64` | 1 (signed int) | Platform-dependent width; canonicalized by Clang |
| `unsigned long` | `uint64` | 0 (unsigned int) | Platform-dependent width; canonicalized |
| `long long` | `int64` | 1 (signed int) | |
| `unsigned long long` | `uint64` | 0 (unsigned int) | |
| `float` | `float32` | 7 (float) | |
| `double`, `long double` | `float64` | 7 (float) | `long double` is truncated to 64-bit |
| `_Float16` / `__fp16` | `float16` | 7 (float) | **Platform-dependent.** Only emitted when the C++ type system exposes a 16-bit float. Today the compiler recognizes it if Clang does; no fixture exists because C++17 lacks a portable half-float type. |
| `std::string` | `str` | 3 (text string) | UTF-8 string, definite-length |
| `std::string` | `str_indef` | 3 (text string) | UTF-8 string, indefinite-length. Not yet selectable via attribute; reserved for future `[[cbor::indefinite]]`. |
| `std::vector<unsigned char>` | `bin` | 2 (byte string) | Raw binary blob, definite-length |
| `std::vector<unsigned char>` | `bin_indef` | 2 (byte string) | Raw binary blob, indefinite-length. Not yet selectable via attribute. |
| `std::vector<T>` | `array` | 4 (array) | `item` descriptor points to element type; definite-length |
| `std::vector<T>` | `array_indef` | 4 (array) | Indefinite-length. Not yet selectable via attribute. |
| `T[N]` (C array) | `array` | 4 (array) | Same emission as `std::vector<T>` |
| `std::map<K,V>` | `map` | 5 (map) | `key` and `value` descriptors; definite-length |
| `std::map<K,V>` | `map_indef` | 5 (map) | Indefinite-length. Not yet selectable via attribute. |
| `std::optional<T>` | `optional` | `nil` or `<T>` | `item` descriptor points to inner type; encoded as `nil` when empty |
| `enum class` | `int32` | 0/1 (unsigned/signed int) | Emitted as underlying integer type; no string mapping today |
| Nested struct `S` (no `tag`) | `object` | 5 (map) | Recursively serialized as nested map |
| Nested struct `S` (`[[cbor::tag(N)]]`) | `tag` | 6 (tag) | Annotated value described by `S`'s descriptor |
| Pointer `T*` | `nil` | 7 (null) | Fallback. Pointers have no natural CBOR representation. |
| `std::variant<...>` | `nil` | 7 (null) | **Gap.** Not yet implemented. |

---

## 5. Descriptor shape

The compiler emits a self-contained C++ header defining `cbor::meta::descriptor<T>` specializations. The runtime (deferred to a future issue) will include these headers and walk the descriptors at encode/decode time.

```cpp
namespace cbor::meta {
    enum class cbor_type_t : std::uint8_t {
        nil, boolean,
        int8, int16, int32, int64,
        uint8, uint16, uint32, uint64,
        float16, float32, float64,
        str, str_indef, bin, bin_indef,
        array, array_indef, map, map_indef,
        object, tag, optional
    };
    struct field_desc {
        const char*   json_name;
        cbor_type_t   type;
        std::uint64_t tag_type;
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
        static constexpr std::uint64_t tag_id = 0;
        static constexpr field_desc fields[] = {};
        static constexpr std::size_t field_count = 0;
    };
} // namespace cbor::meta
```

**Key differences from the MessagePack descriptor:**
- `tag_type` is `std::uint64_t` (not `std::uint8_t`) because CBOR tags are unsigned 64-bit values per RFC 8949.
- `tag_id` is `std::uint64_t` for the same reason.
- `float16` and `*_indef` variants exist in the enum but are not yet selectable through attributes.

Example specialization for a tagged timestamp:

```cpp
template<>
struct descriptor<timestamp_t> {
    static constexpr std::uint64_t tag_id = 1;
    static constexpr field_desc fields[] = {
        { "seconds", cbor_type_t::int64, 0, offsetof(timestamp_t, seconds), false, nullptr, nullptr, nullptr, nullptr },
        { "nanos",   cbor_type_t::int32, 0, offsetof(timestamp_t, nanos),   false, nullptr, nullptr, nullptr, nullptr }
    };
    static constexpr std::size_t field_count = 2;
};

template<>
struct descriptor<event_t> {
    static constexpr field_desc fields[] = {
        { "when", cbor_type_t::tag, 1, offsetof(event_t, when), true, nullptr, nullptr, nullptr, nullptr },
        { "name", cbor_type_t::str, 0, offsetof(event_t, name), false, nullptr, nullptr, nullptr, nullptr }
    };
    static constexpr std::size_t field_count = 2;
};
```

---

## 6. Attribute wiring status

### Implemented and tested

| Attribute | Where read | Where emitted | Test fixture |
|---|---|---|---|
| `cbor::ignore` | `h5_attr_reader::has_attr(fld, "cbor::ignore")` | Skips field in `fields[]` | `cbor_primitives` |
| `cbor::required` | `h5_attr_reader::has_attr(fld, "cbor::required")` | Sets `required = true` in field desc | `cbor_primitives`, `cbor_strings` |
| `cbor::name("...")` | `h5_attr_reader::read_field_string(fld, "cbor::name")` | Overrides `json_name` in field desc | `cbor_strings` |
| `cbor::doc("...")` | `h5_attr_reader::read_class_string(node, "cbor::doc")` | Emitted as `doc` pointer in field desc | `cbor_primitives`, `cbor_nested` |
| `cbor::alias("...")` | `h5_attr_reader::read_class_string(node, "cbor::alias")` | Emitted as `alias[]` in descriptor | `cbor_primitives` |
| `cbor::tag(N)` | `h5_attr_reader::read_class_int(node, "cbor::tag")` | Emitted as `tag_id` in descriptor; referenced as `tag_type` in field desc | `cbor_tags` |

### Not applicable to CBOR

| Attribute | Reason |
|---|---|
| `cbor::on_missing` | CBOR has no schema-level default-value mechanism. Absence semantics live in the runtime decoder, not the descriptor. (Same as JSON and MessagePack.) |
| `cbor::chunk` | HDF5 storage concern. |
| `cbor::compress` | HDF5 storage concern. |
| `cbor::serialize_full` | HDF5 tier-1 emission concern. |
| `cbor::format` | JSON Schema validation concern. |
| `cbor::pattern` | JSON Schema validation concern. |
| `cbor::min` / `cbor::max` | JSON Schema validation concern. |
| `cbor::version` | No CBOR schema format to version. |
| `cbor::name_all` | No wire naming convention needed; CBOR uses map keys, not field names. |
| `cbor::ext` | MessagePack-specific concern. CBOR uses `tag` instead. |

---

## 7. Worked example — sensor event with tagged timestamp

### Input (user source)

```cpp
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace sn::sensor {
    struct [[cbor::tag(1)]] timestamp_t {
        std::int64_t seconds;
        std::int32_t nanos;
    };

    struct [[cbor::doc("Sensor event"), cbor::alias("Event")]] event_t {
        [[cbor::required]] timestamp_t when;
        [[cbor::name("sensor_id")]] std::uint32_t id;
        [[cbor::ignore]] int debug_counter;
        std::string label;
        std::vector<double> readings;
        std::optional<std::uint16_t> flags;
    };
} // namespace sn::sensor
```

### Emitted output (descriptor header)

```cpp
#pragma once

/* Generated by h5cpp-compiler CBOR backend */

#include <cstddef>
#include <cstdint>

namespace cbor::meta {
    enum class cbor_type_t : std::uint8_t {
        nil, boolean,
        int8, int16, int32, int64,
        uint8, uint16, uint32, uint64,
        float16, float32, float64,
        str, str_indef, bin, bin_indef,
        array, array_indef, map, map_indef,
        object, tag, optional
    };

    struct field_desc {
        const char*   json_name;
        cbor_type_t   type;
        std::uint64_t tag_type;
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
        static constexpr std::uint64_t tag_id = 0;
        static constexpr field_desc fields[] = {};
        static constexpr std::size_t field_count = 0;
    };
} // namespace cbor::meta

// descriptor for sn::sensor::timestamp_t
template<>
struct descriptor<sn::sensor::timestamp_t> {
    static constexpr std::uint64_t tag_id = 1;
    static constexpr field_desc fields[] = {
        { "seconds", cbor_type_t::int64, 0, offsetof(sn::sensor::timestamp_t, seconds), false, nullptr, nullptr, nullptr, nullptr },
        { "nanos",   cbor_type_t::int32, 0, offsetof(sn::sensor::timestamp_t, nanos),   false, nullptr, nullptr, nullptr, nullptr }
    };
    static constexpr std::size_t field_count = 2;
};

// descriptor for sn::sensor::event_t
template<>
struct descriptor<sn::sensor::event_t> {
    static constexpr char alias[] = "Event";
    static constexpr field_desc item_1 { nullptr, cbor_type_t::float64, 0, 0, false, nullptr, nullptr, nullptr, nullptr };
    static constexpr field_desc opt_2 { nullptr, cbor_type_t::uint16, 0, 0, false, nullptr, nullptr, nullptr, nullptr };
    static constexpr field_desc fields[] = {
        { "when",      cbor_type_t::tag,      1, offsetof(sn::sensor::event_t, when),     true,  nullptr, nullptr,       nullptr, nullptr },
        { "sensor_id", cbor_type_t::uint32,   0, offsetof(sn::sensor::event_t, id),       false, nullptr, nullptr,       nullptr, nullptr },
        { "label",     cbor_type_t::str,      0, offsetof(sn::sensor::event_t, label),    false, nullptr, nullptr,       nullptr, nullptr },
        { "readings",  cbor_type_t::array,    0, offsetof(sn::sensor::event_t, readings), false, nullptr, &item_1,       nullptr, nullptr },
        { "flags",     cbor_type_t::optional, 0, offsetof(sn::sensor::event_t, flags),    false, nullptr, &opt_2,        nullptr, nullptr }
    };
    static constexpr std::size_t field_count = 5;
};
```

### Observations from the emitted descriptors

- `timestamp_t` carries `tag_id = 1` from `[[cbor::tag(1)]]`. The runtime will encode its two fields as a CBOR map `{seconds: ..., nanos: ...}`, then wrap the whole value with CBOR tag `1` (epoch-based date/time per RFC 8949).
- `event_t` carries `alias = "Event"` from `[[cbor::alias("Event")]]`. The C++ template specialization still uses `sn::sensor::event_t`; the alias is metadata.
- `when` → `cbor_type_t::tag` with `tag_type = 1`. The runtime sees `tag`, looks up `descriptor<timestamp_t>::tag_id`, and routes through the tag codec.
- `id` → renamed to `"sensor_id"` via `[[cbor::name("sensor_id")]]`.
- `debug_counter` → absent entirely (`ignore`).
- `label` → `cbor_type_t::str`. UTF-8 text string (CBOR major type 3).
- `readings` → `cbor_type_t::array` with `item = &item_1` where `item_1.type = float64`. The runtime walks the array, encoding each element as CBOR float64.
- `flags` → `cbor_type_t::optional` with `item = &opt_2` where `opt_2.type = uint16`. The runtime emits CBOR `null` when the optional is empty, or the `uint16` value when present.
- `doc` pointer is `nullptr` on all fields because no `[[cbor::doc]]` was applied at field scope. Class-level `doc` is not wired into field descriptors today.

---

## 8. Runtime architecture — Approach B (descriptors)

The h5cpp architectural pattern is **compiler emits descriptors → runtime consumes descriptors → I/O happens**. The CBOR backend follows this exactly.

### Architecture

```
C++ header + [[cbor::...]] attributes
        ↓
h5cpp-compiler
        ↓
  ┌─────────────────┐
  │ constexpr desc  │  ← C++17 constexpr type descriptor
  │ (.cbor.hpp)     │     emitted into a single header
  └─────────────────┘
        ↓
  ┌─────────────────┐
  │ cbor::runtime   │  ← custom encode/decode (deferred)
  │ (header-only)   │     walks constexpr desc at runtime
  └─────────────────┘
        ↓
   CBOR bytes ↔ C++ object
```

### Why descriptors over generated code

Same rationale as HDF5, JSON, MessagePack, and protobuf backends:
- **Single source of truth:** One compiler pass produces the descriptor.
- **No generated `.cpp` bloat:** Descriptors are `constexpr` tables.
- **Introspection:** Descriptors can be walked reflectively.
- **C++26 future:** P2996 reflection makes the constexpr descriptor layer optional.

### Runtime API (sketch — deferred)

```cpp
namespace cbor {
    // Encoding — descriptor-driven
    template<typename T>
    std::vector<uint8_t> encode(const T& obj);

    // Decoding — descriptor-driven
    template<typename T>
    T decode(const uint8_t* data, size_t len);
}
```

The actual runtime will use a lightweight custom encoder/decoder (not an external library like libcbor) to maintain the h5cpp philosophy of minimal dependencies and zero-copy where possible.

