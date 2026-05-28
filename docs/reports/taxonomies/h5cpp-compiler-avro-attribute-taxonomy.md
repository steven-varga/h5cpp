@page reports_compiler_avro_attribute_taxonomy avro:: Attribute Vocabulary (Avro Backend)

**Date:** 2026-05-24
**Authors:** Steven Varga, Winston (Architecture)
**Status:** Implemented on `vargalabs/h5cpp-compiler#37-avro-backend`
**Repos:** `vargalabs/h5cpp-compiler` (annotation reader + Avro descriptor emitter, issue #37)
**Scope:** Avro backend C++ attribute vocabulary. Defines every `[[avro::...]]` attribute the compiler recognizes, its emission semantics, and its C++26 migration path.
**Companions:**
- `tasks/h5cpp-compiler-h5-attribute-taxonomy.md` — HDF5 attribute vocabulary (same universal words, `h5::` namespace)
- `tasks/h5cpp-compiler-json-attribute-taxonomy.md` — JSON Schema attribute vocabulary (same universal words, `json::` namespace)
- `tasks/h5cpp-compiler-msgpack-attribute-taxonomy.md` — MessagePack attribute vocabulary (same universal words, `msgpack::` namespace)
- `tasks/h5cpp-compiler-cbor-attribute-taxonomy.md` — CBOR attribute vocabulary (same universal words, `cbor::` namespace)
- `tasks/h5cpp-compiler-bson-attribute-taxonomy.md` — BSON attribute vocabulary (same universal words, `bson::` namespace)
- `tasks/h5cpp-compiler-backend-cookbook.md` — transferable recipe for adding any new backend

---

## TL;DR

User-facing attribute set for Avro annotations on plain C++ structs. **Vocabulary is intentionally identical to `h5::*`, `json::*`, `msgpack::*`, `cbor::*`, and `bson::*` where the concept overlaps** (rename, ignore, doc, alias, required) — different namespace, same words. The Avro-specific surface lives only under `avro::*`, with logical types (`datetime`, `timestamp`, `decimal`, `uuid`, `date`, `time`) and the `fixed` type being the only backend-specific keywords.

C++17 attribute syntax today; one-line lift to C++26 typed annotations tomorrow.

| Surface today (C++17 standard-attribute) | C++26 reflection form |
|---|---|
| `[[avro::name("on_wire")]]` | `[[=avro::name{"on_wire"}]]` |
| `[[avro::ignore]]` | `[[=avro::ignore{}]]` |
| `[[avro::required]]` | `[[=avro::required{}]]` |
| `[[avro::datetime]]` | `[[=avro::datetime{}]]` |
| `[[avro::timestamp]]` | `[[=avro::timestamp{}]]` |
| `[[avro::decimal(10, 2)]]` | `[[=avro::decimal{10, 2}]]` |
| `[[avro::uuid]]` | `[[=avro::uuid{}]]` |
| `[[avro::date]]` | `[[=avro::date{}]]` |
| `[[avro::time]]` | `[[=avro::time{}]]` |
| `[[avro::fixed("Name")]]` | `[[=avro::fixed{"Name"}]]` |
| `[[avro::doc("description")]]` | `[[=avro::doc{"description"}]]` |
| `[[avro::alias("Name")]]` | `[[=avro::alias{"Name"}]]` |

Only syntactic shift is `(args)` → `{args}` under the `[[=...]]` form. **Names stay put.**

---

## 2. Universal vocabulary — same words, `avro::` namespace

These attributes use vocabulary identical to `h5::*`, `json::*`, `msgpack::*`, `cbor::*`, and `bson::*`. They live in `avro::` so the namespace stays self-contained for Avro-only users; a user wanting multiple backends writes `[[h5::name(...)]]`, `[[json::name(...)]]`, `[[msgpack::name(...)]]`, `[[cbor::name(...)]]`, `[[bson::name(...)]]`, and `[[avro::name(...)]]` (typically with the same string).

### Universal Tier 1 — must-have

| Attribute | Purpose | Example |
|---|---|---|
| `[[avro::name("on_wire_name")]]` | Rename a field for the Avro wire format. Decouples C++ identifier from the record field name used during encode/decode. Drives the key string in the emitted descriptor's `json_name` field. | `[[avro::name("display_name")]] std::string label;` |
| `[[avro::ignore]]` | Skip this field entirely. Property absent from the descriptor's `fields[]` array; runtime never encodes or decodes it. | `[[avro::ignore]] int debug_counter;` |
| `[[avro::required]]` | Field must be present during deserialization. In Avro, all non-optional fields are implicitly required; this flag is metadata for the runtime. | `[[avro::required]] std::int32_t id;` |

### Universal Tier 2 — high value, low cost

| Attribute | Purpose | Example |
|---|---|---|
| `[[avro::doc("description")]]` | Emitted as the `doc` pointer in the field descriptor. Self-documenting generated code; future tooling may extract it for schema documentation. | `[[avro::doc("nanoseconds since epoch")]] std::uint64_t ts;` |
| `[[avro::alias("Name")]]` | **Class-level.** Emitted as the `alias[]` string in the descriptor. The C++ type name still drives the template specialization; the alias is metadata for tooling. | `struct [[avro::alias("Session")]] session_t { ... };` |

The full universal list mirrors `h5cpp-compiler-h5-attribute-taxonomy.md` §2 and `h5cpp-compiler-pb-attribute-taxonomy.md` §2. Any universal attribute not listed above has no Avro semantics (e.g. `h5::chunk`, `h5::compress` are HDF5-storage concerns; `pb::field(N)`, `pb::wire` are protobuf-wire concerns; `json::format`, `json::pattern` are JSON Schema validation concerns; `msgpack::ext` is a MessagePack-specific concern; `cbor::tag` is a CBOR-specific concern; `bson::binary` is a BSON-specific concern).

---

## 3. Avro-specific vocabulary

### Tier 1 — must-have

Without `avro::datetime`, `avro::timestamp`, `avro::decimal`, `avro::fixed`, and `avro::uuid`, the Avro backend cannot express Avro's rich logical-type system — a core requirement for any Avro codec.

| Attribute | Purpose | Example |
|---|---|---|
| `[[avro::datetime]]` | **Field-level.** Forces the field to `avro_type_t::timestamp_millis` (logical type on `long`). Auto-detected for `std::chrono::system_clock::time_point` without the attribute. | `[[avro::datetime]] std::int64_t created_at;` |
| `[[avro::timestamp]]` | **Field-level.** Forces the field to `avro_type_t::timestamp_micros` (logical type on `long`). | `[[avro::timestamp]] std::int64_t updated_at;` |
| `[[avro::decimal(P, S)]]` | **Field-level.** Forces the field to `avro_type_t::decimal` with precision `P` and scale `S` (`std::uint8_t` each). Avro `decimal` is a logical type on `bytes` or `fixed`. If scale is omitted, it defaults to `0`. | `[[avro::decimal(10, 2)]] double price;` |
| `[[avro::uuid]]` | **Field-level.** Forces the field to `avro_type_t::uuid` (logical type on `string`). | `[[avro::uuid]] std::string id;` |
| `[[avro::date]]` | **Field-level.** Forces the field to `avro_type_t::date` (logical type on `int`, days since epoch). | `[[avro::date]] std::int32_t birth_date;` |
| `[[avro::time]]` | **Field-level.** Forces the field to `avro_type_t::time_millis` (logical type on `int`, milliseconds since midnight). | `[[avro::time]] std::int32_t opening_time;` |
| `[[avro::fixed("Name")]]` | **Field-level.** On `std::array<std::uint8_t, N>`, forces the field to `avro_type_t::fixed` with the given name and size `N` (inferred from the array type). An optional second argument validates the size: `[[avro::fixed("Name", 16)]]`. | `[[avro::fixed("MD5")]] std::array<std::uint8_t, 16> hash;` |

**Datetime semantics.** `std::chrono::system_clock::time_point` is auto-detected as `timestamp_millis` even without `[[avro::datetime]]`. The emitted descriptor carries `avro_type_t::timestamp_millis`. The runtime serializes the time_point as milliseconds since Unix epoch, encoded as a signed 64-bit integer with the Avro `timestamp-millis` logical type.

**Decimal semantics.** Avro `decimal` follows IEEE 754-2008 decimal128 semantics in many implementations. The descriptor carries `decimal_precision` and `decimal_scale` in the `field_desc`. The runtime converts the C++ numeric value into the appropriate decimal encoding (typically a big-endian byte sequence interpreted as an unscaled integer).

**Fixed semantics.** Avro `fixed` is a named, sized binary type. Unlike `bytes` (variable-length), `fixed` has exactly `size` bytes on the wire. The descriptor carries `fixed_name` and `fixed_size`. The name is required because Avro schemas reference `fixed` types by name.

---

## 4. Type map — C++ → Avro

| C++ type | `avro_type_t` | Avro physical type | Notes |
|---|---|---|---|
| `bool` | `boolean` | `boolean` | |
| `char`, `signed char`, `short`, `int` | `int32` | `int` | |
| `long`, `long long` | `int64` | `long` | |
| `unsigned char`, `unsigned short`, `unsigned int` | `int32` | `int` | Unsigned ≤ 32-bit widened to signed 32-bit |
| `unsigned long`, `unsigned long long` | `int64` | `long` | Unsigned 64-bit widened to signed 64-bit |
| `float` | `float32` | `float` | |
| `double`, `long double` | `float64` | `double` | `long double` truncated to 64-bit |
| `std::string` | `string` | `string` | UTF-8 |
| `std::vector<unsigned char>` | `bytes` | `bytes` | Variable-length binary |
| `std::array<unsigned char, N>` | `bytes` or `fixed` | `bytes` or `fixed` | `fixed` if `[[avro::fixed]]` present; otherwise `bytes` |
| `std::vector<T>` | `array` | `array` | `item` descriptor points to element type |
| `T[N]` (C array) | `array` | `array` | Same emission as `std::vector<T>` |
| `std::map<K,V>` | `map` | `map` | `key` and `value` descriptors; Avro requires string keys |
| `std::optional<T>` | `optional` | `[null, T]` union | `item` descriptor points to inner type; `required` is always `false` |
| `enum class` | `int32` | `int` | Emitted as underlying integer type; Avro native enum is a future enhancement |
| Nested struct `S` | `object` | `record` | Recursively serialized as nested Avro record |
| `std::chrono::system_clock::time_point` | `timestamp_millis` | `long` with `timestamp-millis` logicalType | Auto-detected |
| `std::variant<...>` | `null` | `null` | **Gap.** Not yet implemented. |

---

## 5. Descriptor shape

The compiler emits a self-contained C++ header defining `avro::meta::descriptor<T>` specializations. The runtime (deferred to a future issue) will include these headers and walk the descriptors at encode/decode time.

```cpp
namespace avro::meta {
    enum class avro_type_t : std::uint8_t {
        null, boolean, int32, int64, float32, float64,
        bytes, string, array, map, object, optional, fixed,
        date, time_millis, time_micros,
        timestamp_millis, timestamp_micros,
        local_timestamp_millis, local_timestamp_micros,
        decimal, uuid
    };
    struct field_desc {
        const char*   json_name;
        avro_type_t   type;
        std::size_t   offset;
        bool          required;
        const char*   doc;
        const field_desc* item;
        const field_desc* key;
        const field_desc* value;
        const char*   fixed_name;
        std::size_t   fixed_size;
        std::uint8_t  decimal_precision;
        std::uint8_t  decimal_scale;
    };
    template<typename T>
    struct descriptor {
        static constexpr char alias[] = "";
        static constexpr field_desc fields[] = {};
        static constexpr std::size_t field_count = 0;
    };
} // namespace avro::meta
```

**Key differences from the BSON/MessagePack/CBOR descriptors:**
- No `binary_subtype` (BSON-specific) or `ext_type` (MessagePack-specific) or `tag_type` (CBOR-specific).
- `fixed_name` and `fixed_size` support Avro's named `fixed` types.
- `decimal_precision` and `decimal_scale` support Avro's `decimal` logical type.
- `std::optional<T>` emits as `avro_type_t::optional` with `item` pointer; the runtime interprets this as the Avro union `[null, T]`.

Example specialization for a record with logical types and fixed:

```cpp
template<>
struct descriptor<trade_t> {
    static constexpr char alias[] = "Trade";
    static constexpr field_desc item_1 { nullptr, avro_type_t::float64, 0, false, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, 0 };
    static constexpr field_desc opt_2 { nullptr, avro_type_t::int32, 0, false, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, 0 };
    static constexpr field_desc fields[] = {
        { "created_at", avro_type_t::timestamp_millis, offsetof(trade_t, created_at), false, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, 0 },
        { "price", avro_type_t::decimal, offsetof(trade_t, price), false, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 10, 2 },
        { "hash", avro_type_t::fixed, offsetof(trade_t, hash), false, nullptr, nullptr, nullptr, nullptr, "MD5", 16, 0, 0 },
        { "uuid", avro_type_t::uuid, offsetof(trade_t, uuid), false, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, 0 },
        { "tags", avro_type_t::array, offsetof(trade_t, tags), false, nullptr, &item_1, nullptr, nullptr, nullptr, 0, 0, 0 },
        { "flags", avro_type_t::optional, offsetof(trade_t, flags), false, nullptr, &opt_2, nullptr, nullptr, nullptr, 0, 0, 0 }
    };
    static constexpr std::size_t field_count = 6;
};
```

---

## 6. Attribute wiring status

### Implemented and tested

| Attribute | Where read | Where emitted | Test fixture |
|---|---|---|---|
| `avro::ignore` | `h5_attr_reader::has_attr(fld, "avro::ignore")` | Skips field in `fields[]` | `avro_primitives` |
| `avro::required` | `h5_attr_reader::has_attr(fld, "avro::required")` | Sets `required = true` in field desc | `avro_primitives`, `avro_strings` |
| `avro::name("...")` | `h5_attr_reader::read_field_string(fld, "avro::name")` | Overrides `json_name` in field desc | `avro_strings` |
| `avro::doc("...")` | `h5_attr_reader::read_class_string(node, "avro::doc")` | Emitted as `doc` pointer in field desc | `avro_primitives`, `avro_nested` |
| `avro::alias("...")` | `h5_attr_reader::read_class_string(node, "avro::alias")` | Emitted as `alias[]` in descriptor | `avro_primitives` |
| `avro::datetime` | `h5_attr_reader::has_attr(fld, "avro::datetime")` | Emits `avro_type_t::timestamp_millis`; also auto-detected for `std::chrono::time_point` | `avro_datetime` |
| `avro::timestamp` | `h5_attr_reader::has_attr(fld, "avro::timestamp")` | Emits `avro_type_t::timestamp_micros` | `avro_datetime` |
| `avro::decimal(P, S)` | `h5_attr_reader::read_field_ints(fld, "avro::decimal")` | Emits `avro_type_t::decimal` with precision/scale | `avro_decimal` |
| `avro::uuid` | `h5_attr_reader::has_attr(fld, "avro::uuid")` | Emits `avro_type_t::uuid` | *(coverage in doc example)* |
| `avro::date` | `h5_attr_reader::has_attr(fld, "avro::date")` | Emits `avro_type_t::date` | *(coverage in doc example)* |
| `avro::time` | `h5_attr_reader::has_attr(fld, "avro::time")` | Emits `avro_type_t::time_millis` | *(coverage in doc example)* |
| `avro::fixed("Name", N)` | `h5_attr_reader::read_field_string(fld, "avro::fixed")` + `read_field_ints` | Emits `avro_type_t::fixed` with `fixed_name` and `fixed_size` | `avro_fixed` |

### Not applicable to Avro

| Attribute | Reason |
|---|---|
| `avro::on_missing` | Avro has no schema-level default-value mechanism in the descriptor. Absence semantics live in the runtime decoder. (Same as JSON, MessagePack, CBOR, and BSON.) |
| `avro::chunk` | HDF5 storage concern. |
| `avro::compress` | HDF5 storage concern. |
| `avro::serialize_full` | HDF5 tier-1 emission concern. |
| `avro::format` | JSON Schema validation concern. |
| `avro::pattern` | JSON Schema validation concern. |
| `avro::min` / `avro::max` | JSON Schema validation concern. |
| `avro::version` | No Avro schema format to version in the descriptor. |
| `avro::name_all` | No wire naming convention needed; Avro uses record field names. |
| `avro::ext` | MessagePack-specific concern. |
| `avro::tag` | CBOR-specific concern. |
| `avro::binary` | BSON-specific concern. Avro uses `bytes` and `fixed` instead. |

---

## 7. Worked example — sensor event with logical types

### Input (user source)

```cpp
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <chrono>
#include <array>

namespace sn::sensor {
    struct [[avro::doc("Sensor event"), avro::alias("Event")]] event_t {
        [[avro::required]] std::int64_t id;
        [[avro::datetime]] std::chrono::system_clock::time_point when;
        [[avro::decimal(10, 2)]] double price;
        [[avro::fixed("MD5")]] std::array<std::uint8_t, 16> hash;
        [[avro::ignore]] int debug_counter;
        std::string label;
        std::vector<double> readings;
        std::optional<std::uint16_t> flags;
    };
} // namespace sn::sensor
```

### Emitted output (descriptor header)

```cpp
#pragma once

/* Generated by h5cpp-compiler Avro backend */

#include <cstddef>
#include <cstdint>

namespace avro::meta {
    enum class avro_type_t : std::uint8_t {
        null, boolean, int32, int64, float32, float64,
        bytes, string, array, map, object, optional, fixed,
        date, time_millis, time_micros,
        timestamp_millis, timestamp_micros,
        local_timestamp_millis, local_timestamp_micros,
        decimal, uuid
    };
    struct field_desc {
        const char*   json_name;
        avro_type_t   type;
        std::size_t   offset;
        bool          required;
        const char*   doc;
        const field_desc* item;
        const field_desc* key;
        const field_desc* value;
        const char*   fixed_name;
        std::size_t   fixed_size;
        std::uint8_t  decimal_precision;
        std::uint8_t  decimal_scale;
    };
    template<typename T>
    struct descriptor {
        static constexpr char alias[] = "";
        static constexpr field_desc fields[] = {};
        static constexpr std::size_t field_count = 0;
    };
} // namespace avro::meta

// descriptor for sn::sensor::event_t
template<>
struct descriptor<sn::sensor::event_t> {
    static constexpr char alias[] = "Event";
    static constexpr field_desc item_1 { nullptr, avro_type_t::float64, 0, false, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, 0 };
    static constexpr field_desc opt_2 { nullptr, avro_type_t::uint16, 0, false, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, 0 };
    static constexpr field_desc fields[] = {
        { "id",       avro_type_t::int64,            offsetof(sn::sensor::event_t, id),            true,  nullptr, nullptr,       nullptr, nullptr, nullptr, 0, 0, 0 },
        { "when",     avro_type_t::timestamp_millis, offsetof(sn::sensor::event_t, when),          false, nullptr, nullptr,       nullptr, nullptr, nullptr, 0, 0, 0 },
        { "price",    avro_type_t::decimal,          offsetof(sn::sensor::event_t, price),         false, nullptr, nullptr,       nullptr, nullptr, nullptr, 0, 10, 2 },
        { "hash",     avro_type_t::fixed,            offsetof(sn::sensor::event_t, hash),          false, nullptr, nullptr,       nullptr, nullptr, "MD5", 16, 0, 0 },
        { "label",    avro_type_t::string,           offsetof(sn::sensor::event_t, label),         false, nullptr, nullptr,       nullptr, nullptr, nullptr, 0, 0, 0 },
        { "readings", avro_type_t::array,            offsetof(sn::sensor::event_t, readings),      false, nullptr, &item_1,       nullptr, nullptr, nullptr, 0, 0, 0 },
        { "flags",    avro_type_t::optional,         offsetof(sn::sensor::event_t, flags),         false, nullptr, &opt_2,        nullptr, nullptr, nullptr, 0, 0, 0 }
    };
    static constexpr std::size_t field_count = 7;
};
```

### Observations from the emitted descriptors

- `event_t` carries `alias = "Event"` from `[[avro::alias("Event")]]`. The C++ template specialization still uses `sn::sensor::event_t`; the alias is metadata.
- `id` → `avro_type_t::int64` with `required = true` from `[[avro::required]]`.
- `when` → `avro_type_t::timestamp_millis` because `std::chrono::system_clock::time_point` is auto-detected. The runtime will encode it as milliseconds since epoch with the Avro `timestamp-millis` logical type.
- `price` → `avro_type_t::decimal` with `decimal_precision = 10` and `decimal_scale = 2` from `[[avro::decimal(10, 2)]]`.
- `hash` → `avro_type_t::fixed` with `fixed_name = "MD5"` and `fixed_size = 16` from `[[avro::fixed("MD5")]]` on `std::array<std::uint8_t, 16>`. The size is inferred from the array type.
- `debug_counter` → absent entirely (`ignore`).
- `label` → `avro_type_t::string`. Standard UTF-8 string.
- `readings` → `avro_type_t::array` with `item = &item_1` where `item_1.type = float64`. The runtime walks the array, encoding each element as Avro double.
- `flags` → `avro_type_t::optional` with `item = &opt_2` where `opt_2.type = uint16`. The runtime emits Avro `null` when the optional is empty, or the `uint16` value (widened to `int32` per Avro convention) when present. `required` is `false` regardless of any `[[avro::required]]` attribute because the field is `std::optional`.
- `doc` pointer is `nullptr` on all fields because no `[[avro::doc]]` was applied at field scope. Class-level `doc` is not wired into field descriptors today.

---

## 8. Runtime architecture — Approach B (descriptors)

The h5cpp architectural pattern is **compiler emits descriptors → runtime consumes descriptors → I/O happens**. The Avro backend follows this exactly.

### Architecture

```
C++ header + [[avro::...]] attributes
        ↓
h5cpp-compiler
        ↓
  ┌─────────────────┐
  │ constexpr desc  │  ← C++17 constexpr type descriptor
  │ (.avro.hpp)     │     emitted into a single header
  └─────────────────┘
        ↓
  ┌─────────────────┐
  │ avro::runtime   │  ← custom encode/decode (deferred)
  │ (header-only)   │     walks constexpr desc at runtime
  └─────────────────┘
        ↓
   Avro bytes ↔ C++ object
```

### Why descriptors over generated code

Same rationale as HDF5, JSON, MessagePack, CBOR, and protobuf backends:
- **Single source of truth:** One compiler pass produces the descriptor.
- **No generated `.cpp` bloat:** Descriptors are `constexpr` tables.
- **Introspection:** Descriptors can be walked reflectively.
- **C++26 future:** P2996 reflection makes the constexpr descriptor layer optional.

### Runtime API (sketch — deferred)

```cpp
namespace avro {
    // Encoding — descriptor-driven
    template<typename T>
    std::vector<uint8_t> encode(const T& obj);

    // Decoding — descriptor-driven
    template<typename T>
    T decode(const uint8_t* data, size_t len);
}
```

The actual runtime will use a lightweight custom encoder/decoder (not an external library like avro-c++) to maintain the h5cpp philosophy of minimal dependencies and zero-copy where possible.

---

## 9. Open questions and design decisions

### Decided

| # | Decision | Rationale |
|---|---|---|
| 1 | **Runtime architecture:** Approach B (constexpr descriptors + runtime walk) | Consistent with all other backends. |
| 2 | **Output format:** Self-contained C++ header | The emitted file defines `avro_type_t`, `field_desc`, `descriptor` base template, and all specializations. No external runtime header required at compile time. |
| 3 | **Logical types as field-level attributes** | Avro logical types (`timestamp-millis`, `decimal`, `uuid`, etc.) are field-level concerns, not class-level like CBOR tags or MessagePack ext types. |
| 4 | **`std::chrono::system_clock::time_point` auto-detect → `timestamp_millis`** | Matches common Avro convention. The user can also force it explicitly with `[[avro::datetime]]` on non-chrono types. |
| 5 | **`std::vector<uint8_t>` → `bytes`** | Auto-detected from element type. Distinguishes binary blobs from arrays of integers. |
| 6 | **`std::array<uint8_t, N>` → `bytes` or `fixed`** | Without `[[avro::fixed]]`, maps to `bytes` (variable-length). With `[[avro::fixed("Name")]]`, maps to Avro `fixed` with the given name and inferred size. |
| 7 | **`std::optional<T>` → `optional` with `item` pointer** | Runtime interprets `optional` as the Avro union `[null, T]`. `required` is always `false` for optional fields. |
| 8 | **Unsigned integers widened to signed** | Avro has no unsigned integer type. `unsigned int` → `int64` (not `int32`) to avoid overflow. |

### Open

| # | Question | Context |
|---|---|---|
| 1 | **`enum class` native Avro enum support.** | Today enums emit as `int32` (underlying integer). Avro has a native `enum` type with symbols. Should the compiler emit an Avro enum descriptor (with symbol list) for `enum class` types? |
| 2 | **`std::variant<...>` support.** | `std::variant` has no natural Avro representation. Options: (a) emit as a union of the variant alternatives, (b) emit as a record with a discriminant field, (c) defer to user-defined nested structs. |
| 3 | **Packed struct alignment.** | The descriptor carries `offsetof` for each field. If the user compiles with different packing pragmas on different platforms, `offsetof` may disagree. Should the descriptor include `alignof` or `sizeof` for each field as a cross-check? (Same open question as all other backends.) |
| 4 | **Map key type restriction.** | Avro map keys must be strings. The compiler currently emits `avro_type_t::map` for any `std::map<K,V>`, but Avro is only valid when `K` is `std::string`. Should the compiler emit a diagnostic for non-string map keys? |
| 5 | **Optional default value.** | `std::optional<T>` fields have no descriptor-level default. If absent on the wire, the runtime leaves the optional empty. Should `avro::on_missing` be applicable to optionals? (Same open question as all other backends.) |
| 6 | **`avro::decimal` on non-numeric types.** | Today `avro::decimal` can be applied to any field, but the descriptor only carries precision and scale. Should the compiler restrict `avro::decimal` to numeric or `bytes`/`fixed` fields? |
| 7 | **Avro schema JSON emission.** | The current backend emits C++ descriptors. Should there be a parallel mode that emits the actual Avro JSON schema (`.avsc`) for interop with existing Avro tooling? |

---

## 10. Sources

- `tasks/h5cpp-compiler-backend-cookbook.md` (workspace) — transferable recipe for adding any new backend
- `tasks/h5cpp-compiler-h5-attribute-taxonomy.md` (workspace) — HDF5 attribute vocabulary (same universal words, `h5::` namespace)
- `tasks/h5cpp-compiler-json-attribute-taxonomy.md` (workspace) — JSON Schema attribute vocabulary (same universal words, `json::` namespace)
- `tasks/h5cpp-compiler-msgpack-attribute-taxonomy.md` (workspace) — MessagePack attribute vocabulary (same universal words, `msgpack::` namespace)
- `tasks/h5cpp-compiler-cbor-attribute-taxonomy.md` (workspace) — CBOR attribute vocabulary (same universal words, `cbor::` namespace)
- `tasks/h5cpp-compiler-bson-attribute-taxonomy.md` (workspace) — BSON attribute vocabulary (same universal words, `bson::` namespace)
- `tasks/h5cpp-compiler-pb-attribute-taxonomy.md` (workspace) — protobuf attribute vocabulary (same universal words, `pb::` namespace)
- `vargalabs/h5cpp-compiler#37` — `src/consumer_avro.hpp`, `src/h5_attr_translator.hpp`, `src/h5cpp.cpp`
- [Apache Avro Specification](https://avro.apache.org/docs/current/specification/)
- [P2996R13 — Reflection for C++26](https://wg21.link/p2996)
- [P3394R4 — Annotations for Reflection](https://wg21.link/p3394)
