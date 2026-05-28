@page reports_compiler_bson_attribute_taxonomy bson:: Attribute Vocabulary (BSON Backend)

**Date:** 2026-05-24
**Authors:** Steven Varga, Winston (Architecture)
**Status:** Implemented on `vargalabs/h5cpp-compiler#36-bson-backend`
**Repos:** `vargalabs/h5cpp-compiler` (annotation reader + BSON descriptor emitter, issue #36)
**Scope:** BSON backend C++ attribute vocabulary. Defines every `[[bson::...]]` attribute the compiler recognizes, its emission semantics, and its C++26 migration path.
**Companions:**
- `tasks/h5cpp-compiler-h5-attribute-taxonomy.md` — HDF5 attribute vocabulary (same universal words, `h5::` namespace)
- `tasks/h5cpp-compiler-json-attribute-taxonomy.md` — JSON Schema attribute vocabulary (same universal words, `json::` namespace)
- `tasks/h5cpp-compiler-msgpack-attribute-taxonomy.md` — MessagePack attribute vocabulary (same universal words, `msgpack::` namespace)
- `tasks/h5cpp-compiler-cbor-attribute-taxonomy.md` — CBOR attribute vocabulary (same universal words, `cbor::` namespace)
- `tasks/h5cpp-compiler-backend-cookbook.md` — transferable recipe for adding any new backend

---

## TL;DR

User-facing attribute set for BSON annotations on plain C++ structs. **Vocabulary is intentionally identical to `h5::*`, `json::*`, `msgpack::*`, `cbor::*`, and `pb::*` where the concept overlaps** (rename, ignore, doc, alias, required) — different namespace, same words. The BSON-specific surface lives only under `bson::*`, with MongoDB-native types: `datetime`, `timestamp`, `decimal`, and `binary(N)`.

C++17 attribute syntax today; one-line lift to C++26 typed annotations tomorrow.

| Surface today (C++17 standard-attribute) | C++26 reflection form |
|---|---|
| `[[bson::name("on_wire")]]` | `[[=bson::name{"on_wire"}]]` |
| `[[bson::ignore]]` | `[[=bson::ignore{}]]` |
| `[[bson::required]]` | `[[=bson::required{}]]` |
| `[[bson::datetime]]` | `[[=bson::datetime{}]]` |
| `[[bson::timestamp]]` | `[[=bson::timestamp{}]]` |
| `[[bson::decimal]]` | `[[=bson::decimal{}]]` |
| `[[bson::binary(4)]]` | `[[=bson::binary{4}]]` |
| `[[bson::doc("description")]]` | `[[=bson::doc{"description"}]]` |
| `[[bson::alias("Name")]]` | `[[=bson::alias{"Name"}]]` |

Only syntactic shift is `(args)` → `{args}` under the `[[=...]]` form. **Names stay put.**

---

## 2. Universal vocabulary — same words, `bson::` namespace

These attributes use vocabulary identical to `h5::*`, `json::*`, `msgpack::*`, `cbor::*`, and `pb::*`. They live in `bson::` so the namespace stays self-contained for BSON-only users; a user wanting multiple backends writes `[[h5::name(...)]]`, `[[json::name(...)]]`, `[[msgpack::name(...)]]`, `[[cbor::name(...)]]`, and `[[bson::name(...)]]` (typically with the same string).

### Universal Tier 1 — must-have

| Attribute | Purpose | Example |
|---|---|---|
| `[[bson::name("on_wire_name")]]` | Rename a field for the BSON wire format. Decouples C++ identifier from the map key used during encode/decode. Drives the key string in the emitted descriptor's `json_name` field. | `[[bson::name("display_name")]] std::string label;` |
| `[[bson::ignore]]` | Skip this field entirely. Property absent from the descriptor's `fields[]` array; runtime never encodes or decodes it. | `[[bson::ignore]] int debug_counter;` |
| `[[bson::required]]` | Field must be present during deserialization. The runtime can use this to emit an error (or a default value) when the key is absent from the BSON document. | `[[bson::required]] std::int32_t id;` |

### Universal Tier 2 — high value, low cost

| Attribute | Purpose | Example |
|---|---|---|
| `[[bson::doc("description")]]` | Emitted as the `doc` pointer in the field descriptor. Self-documenting generated code; future tooling may extract it for schema documentation. | `[[bson::doc("nanoseconds since epoch")]] std::uint64_t ts;` |
| `[[bson::alias("Name")]]` | **Class-level.** Emitted as the `alias[]` string in the descriptor. The C++ type name still drives the template specialization; the alias is metadata for tooling. | `struct [[bson::alias("Session")]] session_t { ... };` |

The full universal list mirrors `h5cpp-compiler-h5-attribute-taxonomy.md` §2 and `h5cpp-compiler-pb-attribute-taxonomy.md` §2. Any universal attribute not listed above has no BSON semantics (e.g. `h5::chunk`, `h5::compress` are HDF5-storage concerns; `pb::field(N)`, `pb::wire` are protobuf-wire concerns; `json::format`, `json::pattern` are JSON Schema validation concerns; `msgpack::ext` is a MessagePack-specific concern; `cbor::tag` is a CBOR-specific concern).

---

## 3. BSON-specific vocabulary

### Tier 1 — must-have

Without `bson::datetime`, `bson::timestamp`, `bson::decimal`, and `bson::binary`, the BSON backend cannot express MongoDB's native extended types — a core requirement for any BSON codec.

| Attribute | Purpose | Example |
|---|---|---|
| `[[bson::datetime]]` | **Field-level.** Forces the field to `bson_type_t::utc_datetime`. Auto-detected for `std::chrono::system_clock::time_point` without the attribute, but explicit `[[bson::datetime]]` guarantees the type regardless of the C++ type. | `[[bson::datetime]] std::int64_t created_at;` |
| `[[bson::timestamp]]` | **Field-level.** Forces the field to `bson_type_t::timestamp`. BSON Timestamp is an internal MongoDB type (not a user-facing datetime); it carries both a seconds component and an increment. | `[[bson::timestamp]] std::uint64_t op_ts;` |
| `[[bson::decimal]]` | **Field-level.** Forces the field to `bson_type_t::decimal128`. Maps any C++ numeric type to IEEE 754-2008 decimal128. | `[[bson::decimal]] double price;` |
| `[[bson::binary(N)]]` | **Field-level.** Forces `std::vector<std::uint8_t>` to `bson_type_t::bin` with BSON binary subtype `N` (`std::uint8_t`, range `[0, 255]`). Standard subtypes: `0` generic, `1` function, `2` binary (old), `3` UUID (old), `4` UUID, `5` MD5, `128` user-defined. | `[[bson::binary(4)]] std::vector<std::uint8_t> uuid;` |

**Datetime semantics.** `std::chrono::system_clock::time_point` is auto-detected as `utc_datetime` even without `[[bson::datetime]]`. The emitted descriptor carries `bson_type_t::utc_datetime`. The runtime serializes the time_point as milliseconds since Unix epoch (BSON convention), encoded as a signed 64-bit integer in the BSON `utc_datetime` wire format.

**Timestamp semantics.** BSON Timestamp is a 64-bit composite: high 32 bits are seconds since epoch, low 32 bits are an increment ordinal. It is not interchangeable with `utc_datetime`. The runtime packs the field value into this composite form.

**Decimal semantics.** BSON Decimal128 follows IEEE 754-2008 decimal128 (34 significant digits, exponent range −6143 to +6144). The runtime converts the C++ numeric value into the 128-bit BSON decimal128 encoding.

**Binary semantics.** `std::vector<std::uint8_t>` without `[[bson::binary(N)]]` still emits as `bson_type_t::bin` with subtype `0` (generic binary). The attribute overrides the subtype. Unlike MessagePack `ext`, BSON binary does not describe payload layout — it is always an opaque byte vector.

---

## 4. Type map — C++ → BSON

| C++ type | `bson_type_t` | BSON wire type | Notes |
|---|---|---|---|
| `bool` | `boolean` | `0x08` (boolean) | |
| `char`, `signed char`, `short`, `int` | `int32` | `0x10` (int32) | Signed integers ≤ 32-bit |
| `long`, `long long` | `int64` | `0x12` (int64) | Signed 64-bit integers |
| `unsigned char`, `unsigned short`, `unsigned int`, `unsigned long`, `unsigned long long` | `int64` | `0x12` (int64) | Unsigned values widened to signed 64-bit (BSON has no unsigned types) |
| `float`, `double`, `long double` | `float64` | `0x01` (double) | `long double` truncated to 64-bit |
| `std::string` | `string` | `0x02` (string) | UTF-8 string |
| `std::vector<unsigned char>` | `bin` | `0x05` (binary) | Raw binary blob; subtype `0` unless overridden by `[[bson::binary(N)]]` |
| `std::vector<T>` | `array` | `0x04` (array) | `item` descriptor points to element type |
| `T[N]` (C array) | `array` | `0x04` (array) | Same emission as `std::vector<T>` |
| `std::map<K,V>` | `map` | `0x03` (document) | `key` and `value` descriptors; keys must be strings for valid BSON |
| `std::optional<T>` | `optional` | absent or `<T>` | `item` descriptor points to inner type; encoded as absent when empty |
| `enum class` | `int32` | `0x10` (int32) | Emitted as underlying integer type; no string mapping today |
| Nested struct `S` | `object` | `0x03` (document) | Recursively serialized as nested BSON document |
| `std::chrono::system_clock::time_point` | `utc_datetime` | `0x09` (utc_datetime) | Auto-detected; milliseconds since epoch |
| `std::variant<...>` | `nil` | `0x0A` (null) | **Gap.** Not yet implemented. |

---

## 5. Descriptor shape

The compiler emits a self-contained C++ header defining `bson::meta::descriptor<T>` specializations. The runtime (deferred to a future issue) will include these headers and walk the descriptors at encode/decode time.

```cpp
namespace bson::meta {
    enum class bson_type_t : std::uint8_t {
        nil, boolean, int32, int64, float64,
        string, bin, array, map, object, optional,
        utc_datetime, timestamp, decimal128
    };
    struct field_desc {
        const char*   json_name;
        bson_type_t   type;
        std::uint8_t  binary_subtype;
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
        static constexpr field_desc fields[] = {};
        static constexpr std::size_t field_count = 0;
    };
} // namespace bson::meta
```

**Key differences from the CBOR/MessagePack descriptors:**
- `binary_subtype` is `std::uint8_t` (not `tag_type` or `ext_type`) because BSON binary carries a subtype byte per the BSON spec.
- No `float16` or `*_indef` variants — BSON has no half-precision float or indefinite-length containers.
- No class-level tag/ext ID — BSON does not use tags or extension types. Instead, field-level attributes (`datetime`, `timestamp`, `decimal`, `binary`) drive the BSON type directly.
- `utc_datetime`, `timestamp`, and `decimal128` are first-class enum values because they are native BSON wire types.

Example specialization for a record with MongoDB extended types:

```cpp
template<>
struct descriptor<session_t> {
    static constexpr char alias[] = "Session";
    static constexpr field_desc item_1 { nullptr, bson_type_t::float64, 0, 0, false, nullptr, nullptr, nullptr, nullptr };
    static constexpr field_desc fields[] = {
        { "created", bson_type_t::utc_datetime, 0, offsetof(session_t, created), false, nullptr, nullptr, nullptr, nullptr },
        { "oplog_ts", bson_type_t::timestamp, 0, offsetof(session_t, oplog_ts), false, nullptr, nullptr, nullptr, nullptr },
        { "price", bson_type_t::decimal128, 0, offsetof(session_t, price), false, nullptr, nullptr, nullptr, nullptr },
        { "uuid", bson_type_t::bin, 4, offsetof(session_t, uuid), false, nullptr, nullptr, nullptr, nullptr },
        { "raw", bson_type_t::bin, 0, offsetof(session_t, raw), false, nullptr, nullptr, nullptr, nullptr },
        { "tags", bson_type_t::array, 0, offsetof(session_t, tags), false, nullptr, &item_1, nullptr, nullptr }
    };
    static constexpr std::size_t field_count = 6;
};
```

---

## 6. Attribute wiring status

### Implemented and tested

| Attribute | Where read | Where emitted | Test fixture |
|---|---|---|---|
| `bson::ignore` | `h5_attr_reader::has_attr(fld, "bson::ignore")` | Skips field in `fields[]` | `bson_primitives` |
| `bson::required` | `h5_attr_reader::has_attr(fld, "bson::required")` | Sets `required = true` in field desc | `bson_primitives`, `bson_strings` |
| `bson::name("...")` | `h5_attr_reader::read_field_string(fld, "bson::name")` | Overrides `json_name` in field desc | `bson_strings` |
| `bson::doc("...")` | `h5_attr_reader::read_class_string(node, "bson::doc")` | Emitted as `doc` pointer in field desc | `bson_primitives`, `bson_nested` |
| `bson::alias("...")` | `h5_attr_reader::read_class_string(node, "bson::alias")` | Emitted as `alias[]` in descriptor | `bson_primitives` |
| `bson::datetime` | `h5_attr_reader::has_attr(fld, "bson::datetime")` | Emits `bson_type_t::utc_datetime`; also auto-detected for `std::chrono::time_point` | `bson_datetime` |
| `bson::timestamp` | `h5_attr_reader::has_attr(fld, "bson::timestamp")` | Emits `bson_type_t::timestamp` | `bson_datetime` |
| `bson::decimal` | `h5_attr_reader::has_attr(fld, "bson::decimal")` | Emits `bson_type_t::decimal128` | `bson_decimal` |
| `bson::binary(N)` | `h5_attr_reader::read_field_ints(fld, "bson::binary")` | Emits `bson_type_t::bin` with `binary_subtype = N` | `bson_binary` |

### Not applicable to BSON

| Attribute | Reason |
|---|---|
| `bson::on_missing` | BSON has no schema-level default-value mechanism. Absence semantics live in the runtime decoder, not the descriptor. (Same as JSON, MessagePack, and CBOR.) |
| `bson::chunk` | HDF5 storage concern. |
| `bson::compress` | HDF5 storage concern. |
| `bson::serialize_full` | HDF5 tier-1 emission concern. |
| `bson::format` | JSON Schema validation concern. |
| `bson::pattern` | JSON Schema validation concern. |
| `bson::min` / `bson::max` | JSON Schema validation concern. |
| `bson::version` | No BSON schema format to version. |
| `bson::name_all` | No wire naming convention needed; BSON uses document keys, not field names. |
| `bson::ext` | MessagePack-specific concern. BSON uses `binary` with subtypes instead. |
| `bson::tag` | CBOR-specific concern. BSON does not have tagged values. |

---

## 7. Worked example — MongoDB document with extended types

### Input (user source)

```cpp
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <chrono>

namespace sn::mongo {
    struct [[bson::doc("Trade record"), bson::alias("Trade")]] trade_t {
        [[bson::required]] std::int64_t id;
        [[bson::name("created_at")]] std::chrono::system_clock::time_point created;
        [[bson::timestamp]] std::uint64_t oplog_ts;
        [[bson::decimal]] double price;
        [[bson::binary(4)]] std::vector<std::uint8_t> uuid;
        std::vector<std::uint8_t> raw;
        [[bson::ignore]] int debug_counter;
        std::string symbol;
        std::vector<double> tags;
        std::optional<std::uint16_t> flags;
    };
} // namespace sn::mongo
```

### Emitted output (descriptor header)

```cpp
#pragma once

/* Generated by h5cpp-compiler BSON backend */

#include <cstddef>
#include <cstdint>

namespace bson::meta {
    enum class bson_type_t : std::uint8_t {
        nil, boolean, int32, int64, float64,
        string, bin, array, map, object, optional,
        utc_datetime, timestamp, decimal128
    };
    struct field_desc {
        const char*   json_name;
        bson_type_t   type;
        std::uint8_t  binary_subtype;
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
        static constexpr field_desc fields[] = {};
        static constexpr std::size_t field_count = 0;
    };
} // namespace bson::meta

// descriptor for sn::mongo::trade_t
template<>
struct descriptor<sn::mongo::trade_t> {
    static constexpr char alias[] = "Trade";
    static constexpr field_desc item_1 { nullptr, bson_type_t::float64, 0, 0, false, nullptr, nullptr, nullptr, nullptr };
    static constexpr field_desc opt_2 { nullptr, bson_type_t::uint16, 0, 0, false, nullptr, nullptr, nullptr, nullptr };
    static constexpr field_desc fields[] = {
        { "id",         bson_type_t::int64,      0, offsetof(sn::mongo::trade_t, id),          true,  nullptr, nullptr, nullptr, nullptr },
        { "created_at", bson_type_t::utc_datetime, 0, offsetof(sn::mongo::trade_t, created),   false, nullptr, nullptr, nullptr, nullptr },
        { "oplog_ts",   bson_type_t::timestamp,  0, offsetof(sn::mongo::trade_t, oplog_ts),    false, nullptr, nullptr, nullptr, nullptr },
        { "price",      bson_type_t::decimal128, 0, offsetof(sn::mongo::trade_t, price),       false, nullptr, nullptr, nullptr, nullptr },
        { "uuid",       bson_type_t::bin,        4, offsetof(sn::mongo::trade_t, uuid),        false, nullptr, nullptr, nullptr, nullptr },
        { "raw",        bson_type_t::bin,        0, offsetof(sn::mongo::trade_t, raw),         false, nullptr, nullptr, nullptr, nullptr },
        { "symbol",     bson_type_t::string,     0, offsetof(sn::mongo::trade_t, symbol),      false, nullptr, nullptr, nullptr, nullptr },
        { "tags",       bson_type_t::array,      0, offsetof(sn::mongo::trade_t, tags),        false, nullptr, &item_1, nullptr, nullptr },
        { "flags",      bson_type_t::optional,   0, offsetof(sn::mongo::trade_t, flags),       false, nullptr, &opt_2,  nullptr, nullptr }
    };
    static constexpr std::size_t field_count = 9;
};
```

### Observations from the emitted descriptors

- `trade_t` carries `alias = "Trade"` from `[[bson::alias("Trade")]]`. The C++ template specialization still uses `sn::mongo::trade_t`; the alias is metadata.
- `id` → `bson_type_t::int64` with `required = true` from `[[bson::required]]`.
- `created` → `bson_type_t::utc_datetime` because `std::chrono::system_clock::time_point` is auto-detected. The key is renamed to `"created_at"` via `[[bson::name("created_at")]]`.
- `oplog_ts` → `bson_type_t::timestamp` from `[[bson::timestamp]]`.
- `price` → `bson_type_t::decimal128` from `[[bson::decimal]]`.
- `uuid` → `bson_type_t::bin` with `binary_subtype = 4` from `[[bson::binary(4)]]` (RFC 4122 UUID).
- `raw` → `bson_type_t::bin` with `binary_subtype = 0` (generic binary) because no `[[bson::binary]]` attribute is present.
- `debug_counter` → absent entirely (`ignore`).
- `symbol` → `bson_type_t::string`. Standard UTF-8 string (BSON type `0x02`).
- `tags` → `bson_type_t::array` with `item = &item_1` where `item_1.type = float64`. The runtime walks the array, encoding each element as BSON double.
- `flags` → `bson_type_t::optional` with `item = &opt_2` where `opt_2.type = uint16`. The runtime omits the field when the optional is empty, or emits it as `int32` (widened from `uint16`) when present.
- `doc` pointer is `nullptr` on all fields because no `[[bson::doc]]` was applied at field scope. Class-level `doc` is not wired into field descriptors today.

---

## 8. Runtime architecture — Approach B (descriptors)

The h5cpp architectural pattern is **compiler emits descriptors → runtime consumes descriptors → I/O happens**. The BSON backend follows this exactly.

### Architecture

```
C++ header + [[bson::...]] attributes
        ↓
h5cpp-compiler
        ↓
  ┌─────────────────┐
  │ constexpr desc  │  ← C++17 constexpr type descriptor
  │ (.bson.hpp)     │     emitted into a single header
  └─────────────────┘
        ↓
  ┌─────────────────┐
  │ bson::runtime   │  ← custom encode/decode (deferred)
  │ (header-only)   │     walks constexpr desc at runtime
  └─────────────────┘
        ↓
   BSON bytes ↔ C++ object
```

### Why descriptors over generated code

Same rationale as HDF5, JSON, MessagePack, CBOR, and protobuf backends:
- **Single source of truth:** One compiler pass produces the descriptor.
- **No generated `.cpp` bloat:** Descriptors are `constexpr` tables.
- **Introspection:** Descriptors can be walked reflectively.
- **C++26 future:** P2996 reflection makes the constexpr descriptor layer optional.

### Runtime API (sketch — deferred)

```cpp
namespace bson {
    // Encoding — descriptor-driven
    template<typename T>
    std::vector<uint8_t> encode(const T& obj);

    // Decoding — descriptor-driven
    template<typename T>
    T decode(const uint8_t* data, size_t len);
}
```

The actual runtime will use a lightweight custom encoder/decoder (not an external library like libbson) to maintain the h5cpp philosophy of minimal dependencies and zero-copy where possible.

---

## 9. Open questions and design decisions

### Decided

| # | Decision | Rationale |
|---|---|---|
| 1 | **Runtime architecture:** Approach B (constexpr descriptors + runtime walk) | Consistent with all other backends. |
| 2 | **Output format:** Self-contained C++ header | The emitted file defines `bson_type_t`, `field_desc`, `descriptor` base template, and all specializations. No external runtime header required at compile time. |
| 3 | **Extended types as field-level attributes** | MongoDB types (`datetime`, `timestamp`, `decimal`, `binary`) are field-level concerns in BSON, not class-level like CBOR tags or MessagePack ext types. |
| 4 | **`std::chrono::system_clock::time_point` auto-detect → `utc_datetime`** | Matches BSON convention (milliseconds since epoch). The user can also force it explicitly with `[[bson::datetime]]` on non-chrono types. |
| 5 | **`std::vector<uint8_t>` → `bin` with subtype `0`** | Auto-detected from element type. Distinguishes binary blobs (BSON type `0x05`) from arrays of integers (BSON type `0x04`). |
| 6 | **Unsigned integers widened to `int64`** | BSON has no unsigned integer type. All unsigned C++ integers emit as `int64` to avoid overflow. |

### Open

| # | Question | Context |
|---|---|---|
| 1 | **`enum class` string mapping.** | Today enums emit as `int32` (underlying integer). MongoDB drivers often store enums as strings for readability. Should the compiler support an optional `bson::enum_as_string` attribute? |
| 2 | **`std::variant<...>` support.** | `std::variant` has no natural BSON representation. Options: (a) emit as a tagged document with a `"_type"` discriminant, (b) defer to user-defined nested structs, (c) use BSON `0x06` (undefined) as a sentinel. |
| 3 | **Packed struct alignment.** | The descriptor carries `offsetof` for each field. If the user compiles with different packing pragmas on different platforms, `offsetof` may disagree. Should the descriptor include `alignof` or `sizeof` for each field as a cross-check? (Same open question as all other backends.) |
| 4 | **Map key type restriction.** | BSON document keys must be strings. The compiler currently emits `bson_type_t::map` for any `std::map<K,V>`, but BSON is only valid when `K` is `std::string`. Should the compiler emit a diagnostic for non-string map keys? |
| 5 | **Optional default value.** | `std::optional<T>` fields have no descriptor-level default. If absent on the wire, the runtime leaves the optional empty. Should `bson::on_missing` be applicable to optionals? (Same open question as MessagePack and CBOR.) |
| 6 | **`bson::datetime` on `std::chrono::time_point` precision.** | Today the compiler emits `utc_datetime` but does not specify millisecond vs. second precision in the descriptor. Should the attribute accept an optional precision argument (e.g. `[[bson::datetime(ms)]]` vs `[[bson::datetime(s)]]`)? |

---

## 10. Sources

- `tasks/h5cpp-compiler-backend-cookbook.md` (workspace) — transferable recipe for adding any new backend
- `tasks/h5cpp-compiler-h5-attribute-taxonomy.md` (workspace) — HDF5 attribute vocabulary (same universal words, `h5::` namespace)
- `tasks/h5cpp-compiler-json-attribute-taxonomy.md` (workspace) — JSON Schema attribute vocabulary (same universal words, `json::` namespace)
- `tasks/h5cpp-compiler-msgpack-attribute-taxonomy.md` (workspace) — MessagePack attribute vocabulary (same universal words, `msgpack::` namespace)
- `tasks/h5cpp-compiler-cbor-attribute-taxonomy.md` (workspace) — CBOR attribute vocabulary (same universal words, `cbor::` namespace)
- `tasks/h5cpp-compiler-pb-attribute-taxonomy.md` (workspace) — protobuf attribute vocabulary (same universal words, `pb::` namespace)
- `vargalabs/h5cpp-compiler#36` — `src/consumer_bson.hpp`, `src/h5_attr_translator.hpp`, `src/h5cpp.cpp`
- [BSON Specification](https://bsonspec.org/)
- [P2996R13 — Reflection for C++26](https://wg21.link/p2996)
- [P3394R4 — Annotations for Reflection](https://wg21.link/p3394)
