@page reports_compiler_rlp_attribute_taxonomy rlp:: Attribute Vocabulary (RLP Backend)

**Date:** 2026-05-24
**Authors:** Steven Varga, Winston (Architecture)
**Status:** Implemented on `vargalabs/h5cpp-compiler#38-rlp-backend`
**Repos:** `vargalabs/h5cpp-compiler` (annotation reader + RLP descriptor emitter, issue #38)
**Scope:** RLP backend C++ attribute vocabulary. Defines every `[[rlp::...]]` attribute the compiler recognizes, its emission semantics, and its C++26 migration path.
**Companions:**
- `tasks/h5cpp-compiler-h5-attribute-taxonomy.md` — HDF5 attribute vocabulary (same universal words, `h5::` namespace)
- `tasks/h5cpp-compiler-json-attribute-taxonomy.md` — JSON Schema attribute vocabulary (same universal words, `json::` namespace)
- `tasks/h5cpp-compiler-msgpack-attribute-taxonomy.md` — MessagePack attribute vocabulary (same universal words, `msgpack::` namespace)
- `tasks/h5cpp-compiler-cbor-attribute-taxonomy.md` — CBOR attribute vocabulary (same universal words, `cbor::` namespace)
- `tasks/h5cpp-compiler-bson-attribute-taxonomy.md` — BSON attribute vocabulary (same universal words, `bson::` namespace)
- `tasks/h5cpp-compiler-avro-attribute-taxonomy.md` — Avro attribute vocabulary (same universal words, `avro::` namespace)
- `tasks/h5cpp-compiler-backend-cookbook.md` — transferable recipe for adding any new backend

---

## TL;DR

User-facing attribute set for RLP (Recursive Length Prefix) annotations on plain C++ structs. **Vocabulary is intentionally identical to `h5::*`, `json::*`, `msgpack::*`, `cbor::*`, `bson::*`, and `avro::*` where the concept overlaps** (rename, ignore, doc, alias, required) — different namespace, same words. The RLP-specific surface is minimal: `rlp::timestamp` is the only backend-specific keyword. RLP's wire format is extraordinarily simple — only byte strings and lists exist on the wire — so the descriptor reflects that simplicity.

C++17 attribute syntax today; one-line lift to C++26 typed annotations tomorrow.

| Surface today (C++17 standard-attribute) | C++26 reflection form |
|---|---|
| `[[rlp::name("on_wire")]]` | `[[=rlp::name{"on_wire"}]]` |
| `[[rlp::ignore]]` | `[[=rlp::ignore{}]]` |
| `[[rlp::required]]` | `[[=rlp::required{}]]` |
| `[[rlp::timestamp]]` | `[[=rlp::timestamp{}]]` |
| `[[rlp::doc("description")]]` | `[[=rlp::doc{"description"}]]` |
| `[[rlp::alias("Name")]]` | `[[=rlp::alias{"Name"}]]` |

Only syntactic shift is `(args)` → `{args}` under the `[[=...]]` form. **Names stay put.**

---

## 2. Universal vocabulary — same words, `rlp::` namespace

These attributes use vocabulary identical to `h5::*`, `json::*`, `msgpack::*`, `cbor::*`, `bson::*`, and `avro::*`. They live in `rlp::` so the namespace stays self-contained for RLP-only users; a user wanting multiple backends writes `[[h5::name(...)]]`, `[[json::name(...)]]`, `[[msgpack::name(...)]]`, `[[cbor::name(...)]]`, `[[bson::name(...)]]`, `[[avro::name(...)]]`, and `[[rlp::name(...)]]` (typically with the same string).

### Universal Tier 1 — must-have

| Attribute | Purpose | Example |
|---|---|---|
| `[[rlp::name("on_wire_name")]]` | Rename a field for documentation/tooling. **RLP wire is positional, so this does not affect the byte stream.** It drives the `json_name` field in the emitted descriptor for introspection. | `[[rlp::name("display_name")]] std::string label;` |
| `[[rlp::ignore]]` | Skip this field entirely. Property absent from the descriptor's `fields[]` array; runtime never encodes or decodes it. Subsequent fields shift position. | `[[rlp::ignore]] int debug_counter;` |
| `[[rlp::required]]` | Field must be present during deserialization. In RLP, all non-optional fields are implicitly required; this flag is metadata for the runtime. | `[[rlp::required]] std::int32_t id;` |

### Universal Tier 2 — high value, low cost

| Attribute | Purpose | Example |
|---|---|---|
| `[[rlp::doc("description")]]` | Emitted as the `doc` pointer in the field descriptor. Self-documenting generated code; future tooling may extract it for schema documentation. | `[[rlp::doc("nanoseconds since epoch")]] std::uint64_t ts;` |
| `[[rlp::alias("Name")]]` | **Class-level.** Emitted as the `alias[]` string in the descriptor. The C++ type name still drives the template specialization; the alias is metadata for tooling. | `struct [[rlp::alias("Session")]] session_t { ... };` |

The full universal list mirrors `h5cpp-compiler-h5-attribute-taxonomy.md` §2 and `h5cpp-compiler-pb-attribute-taxonomy.md` §2. Any universal attribute not listed above has no RLP semantics (e.g. `h5::chunk`, `h5::compress` are HDF5-storage concerns; `pb::field(N)`, `pb::wire` are protobuf-wire concerns; `json::format`, `json::pattern` are JSON Schema validation concerns; `msgpack::ext` is a MessagePack-specific concern; `cbor::tag` is a CBOR-specific concern; `bson::binary` is a BSON-specific concern; `avro::decimal`, `avro::fixed` are Avro-specific concerns).

---

## 3. RLP-specific vocabulary

### Tier 1 — must-have

Without `rlp::timestamp`, the RLP backend cannot express Ethereum-style timestamp encoding, which is a common use case for RLP (block headers, transaction receipts).

| Attribute | Purpose | Example |
|---|---|---|
| `[[rlp::timestamp]]` | **Field-level.** Forces the field to `rlp_type_t::timestamp`. Auto-detected for `std::chrono::system_clock::time_point` without the attribute. The runtime serializes the time_point as nanoseconds since Unix epoch encoded as a big-endian `uint64_t` byte string. | `[[rlp::timestamp]] std::int64_t created_at;` |

**Timestamp semantics.** `std::chrono::system_clock::time_point` is auto-detected as `timestamp` even without `[[rlp::timestamp]]`. The emitted descriptor carries `rlp_type_t::timestamp`. The runtime applies `rlp::meta::integer_codec_t<std::chrono::system_clock::time_point>` which converts to/from `uint64_t` nanoseconds. The user can also force `timestamp` explicitly on non-chrono types (e.g. a raw `std::uint64_t` that represents nanoseconds).

---

## 4. Type map — C++ → RLP

RLP has only two wire types: **byte strings** and **lists**. Everything else is a payload interpretation.

| C++ type | `rlp_type_t` | RLP wire type | Notes |
|---|---|---|---|
| `bool` | `bytes` | byte string | `false` → empty string (`0x80`); `true` → `0x01` |
| `char`, `signed char`, `short`, `int`, `long`, `long long` | `bytes` | byte string | Signed integer, big-endian minimal length |
| `unsigned char`, `unsigned short`, `unsigned int`, `unsigned long`, `unsigned long long` | `bytes` | byte string | Unsigned integer, big-endian minimal length |
| `float`, `double`, `long double` | `bytes` | byte string | IEEE 754 bytes, platform-endian (typically little-endian) |
| `std::string` | `bytes` | byte string | Raw UTF-8 bytes |
| `std::vector<unsigned char>` | `bytes` | byte string | Raw binary bytes |
| `std::array<unsigned char, N>` | `bytes` | byte string | Raw binary bytes |
| `std::vector<T>` | `list` | list | Each element encoded recursively |
| `T[N]` (C array) | `list` | list | Same emission as `std::vector<T>` |
| `std::array<T, N>` (non-byte) | `list` | list | Each element encoded recursively |
| `std::map<K,V>` | `bytes` | byte string | **Not natively supported.** Fallback to `bytes`; user should use `std::vector<std::pair<K,V>>` for list-of-pairs semantics. |
| `std::optional<T>` | `optional` | byte string or list | Empty bytes (`0x80`) for null, encoded `T` for value |
| `enum class` | `bytes` | byte string | Underlying integer, big-endian minimal length |
| Nested struct `S` | `list` | list | Fields encoded positionally as a list |
| `std::chrono::system_clock::time_point` | `timestamp` | byte string | Auto-detected; `uint64_t` nanoseconds since epoch |
| `std::variant<...>` | `bytes` | byte string | **Gap.** Not yet implemented. |

---

## 5. Descriptor shape

The compiler emits a self-contained C++ header defining `rlp::meta::descriptor<T>` specializations. The runtime (deferred to a future issue) will include these headers and walk the descriptors at encode/decode time.

```cpp
namespace rlp::meta {
    enum class rlp_type_t : std::uint8_t {
        bytes, list, optional, timestamp
    };
    struct field_desc {
        const char*   json_name;
        rlp_type_t    type;
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
} // namespace rlp::meta
```

**Key differences from all other backends:**
- Only **4 enum values** (`bytes`, `list`, `optional`, `timestamp`) vs. 15+ in Avro/CBOR/BSON.
- No `key`/`value` semantics for maps — RLP has no native map type.
- `json_name` is metadata-only; RLP wire encoding is purely positional.
- No `fixed_size`, `decimal_precision`, `binary_subtype`, `ext_type`, `tag_type`, or other backend-specific descriptor fields.

Example specialization for a sensor record:

```cpp
template<>
struct descriptor<sensor_t> {
    static constexpr char alias[] = "Sensor";
    static constexpr field_desc item_1 { nullptr, rlp_type_t::bytes, 0, false, nullptr, nullptr, nullptr, nullptr };
    static constexpr field_desc opt_2 { nullptr, rlp_type_t::bytes, 0, false, nullptr, nullptr, nullptr, nullptr };
    static constexpr field_desc fields[] = {
        { "id",       rlp_type_t::bytes,     offsetof(sensor_t, id),       true,  nullptr, nullptr, nullptr, nullptr },
        { "when",     rlp_type_t::timestamp, offsetof(sensor_t, when),     false, nullptr, nullptr, nullptr, nullptr },
        { "readings", rlp_type_t::list,      offsetof(sensor_t, readings), false, nullptr, &item_1, nullptr, nullptr },
        { "flags",    rlp_type_t::optional,  offsetof(sensor_t, flags),    false, nullptr, &opt_2,  nullptr, nullptr }
    };
    static constexpr std::size_t field_count = 4;
};
```

---

## 6. Attribute wiring status

### Implemented and tested

| Attribute | Where read | Where emitted | Test fixture |
|---|---|---|---|
| `rlp::ignore` | `h5_attr_reader::has_attr(fld, "rlp::ignore")` | Skips field in `fields[]` | `rlp_primitives` |
| `rlp::required` | `h5_attr_reader::has_attr(fld, "rlp::required")` | Sets `required = true` in field desc | `rlp_primitives`, `rlp_strings` |
| `rlp::name("...")` | `h5_attr_reader::read_field_string(fld, "rlp::name")` | Overrides `json_name` in field desc | `rlp_strings` |
| `rlp::doc("...")` | `h5_attr_reader::read_class_string(node, "rlp::doc")` | Emitted as `doc` pointer in field desc | `rlp_primitives`, `rlp_nested` |
| `rlp::alias("...")` | `h5_attr_reader::read_class_string(node, "rlp::alias")` | Emitted as `alias[]` in descriptor | `rlp_primitives` |
| `rlp::timestamp` | `h5_attr_reader::has_attr(fld, "rlp::timestamp")` | Emits `rlp_type_t::timestamp`; also auto-detected for `std::chrono::time_point` | `rlp_timestamp` |

### Not applicable to RLP

| Attribute | Reason |
|---|---|
| `rlp::on_missing` | RLP has no schema-level default-value mechanism. Absence semantics live in the runtime decoder. (Same as JSON, MessagePack, CBOR, BSON, and Avro.) |
| `rlp::chunk` | HDF5 storage concern. |
| `rlp::compress` | HDF5 storage concern. |
| `rlp::serialize_full` | HDF5 tier-1 emission concern. |
| `rlp::format` | JSON Schema validation concern. |
| `rlp::pattern` | JSON Schema validation concern. |
| `rlp::min` / `rlp::max` | JSON Schema validation concern. |
| `rlp::version` | No RLP schema format to version. |
| `rlp::name_all` | No wire naming convention needed; RLP is positional. |
| `rlp::ext` | MessagePack-specific concern. |
| `rlp::tag` | CBOR-specific concern. |
| `rlp::binary` | BSON-specific concern. |
| `rlp::decimal` / `rlp::fixed` / `rlp::uuid` / `rlp::date` / `rlp::time` | Avro-specific logical type concerns. RLP has no logical type system beyond `timestamp`. |

---

## 7. Worked example — Ethereum-style transaction envelope

### Input (user source)

```cpp
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <chrono>

namespace sn::eth {
    struct [[rlp::doc("Ethereum transaction envelope"), rlp::alias("Tx")]] tx_t {
        [[rlp::required]] std::uint64_t nonce;
        [[rlp::required]] std::uint64_t gas_price;
        [[rlp::required]] std::uint64_t gas_limit;
        [[rlp::name("to_addr")]] std::string to;
        [[rlp::required]] std::uint64_t value;
        [[rlp::name("data_bytes")]] std::vector<std::uint8_t> data;
        [[rlp::timestamp]] std::chrono::system_clock::time_point created_at;
        [[rlp::ignore]] int debug_counter;
        std::optional<std::uint64_t> chain_id;
    };
} // namespace sn::eth
```

### Emitted output (descriptor header)

```cpp
#pragma once

/* Generated by h5cpp-compiler RLP backend */

#include <cstddef>
#include <cstdint>

namespace rlp::meta {
    enum class rlp_type_t : std::uint8_t {
        bytes, list, optional, timestamp
    };
    struct field_desc {
        const char*   json_name;
        rlp_type_t    type;
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
} // namespace rlp::meta

// descriptor for sn::eth::tx_t
template<>
struct descriptor<sn::eth::tx_t> {
    static constexpr char alias[] = "Tx";
    static constexpr field_desc opt_1 { nullptr, rlp_type_t::bytes, 0, false, nullptr, nullptr, nullptr, nullptr };
    static constexpr field_desc fields[] = {
        { "nonce",      rlp_type_t::bytes,     offsetof(sn::eth::tx_t, nonce),      true,  nullptr, nullptr, nullptr, nullptr },
        { "gas_price",  rlp_type_t::bytes,     offsetof(sn::eth::tx_t, gas_price),  true,  nullptr, nullptr, nullptr, nullptr },
        { "gas_limit",  rlp_type_t::bytes,     offsetof(sn::eth::tx_t, gas_limit),  true,  nullptr, nullptr, nullptr, nullptr },
        { "to_addr",    rlp_type_t::bytes,     offsetof(sn::eth::tx_t, to),         true,  nullptr, nullptr, nullptr, nullptr },
        { "value",      rlp_type_t::bytes,     offsetof(sn::eth::tx_t, value),      true,  nullptr, nullptr, nullptr, nullptr },
        { "data_bytes", rlp_type_t::bytes,     offsetof(sn::eth::tx_t, data),       false, nullptr, nullptr, nullptr, nullptr },
        { "created_at", rlp_type_t::timestamp, offsetof(sn::eth::tx_t, created_at), false, nullptr, nullptr, nullptr, nullptr },
        { "chain_id",   rlp_type_t::optional,  offsetof(sn::eth::tx_t, chain_id),   false, nullptr, &opt_1,  nullptr, nullptr }
    };
    static constexpr std::size_t field_count = 8;
};
```

### Observations from the emitted descriptors

- `tx_t` carries `alias = "Tx"` from `[[rlp::alias("Tx")]]`. The C++ template specialization still uses `sn::eth::tx_t`; the alias is metadata.
- All scalar fields (`nonce`, `gas_price`, `gas_limit`, `value`) → `rlp_type_t::bytes` with `required = true`. The runtime encodes each as a big-endian minimal-length byte string.
- `to_addr` → `rlp_type_t::bytes` (the `to` field renamed via `[[rlp::name("to_addr")]]`). On the wire it is still the 5th positional element.
- `data_bytes` → `rlp_type_t::bytes` (`std::vector<uint8_t>` is a byte string in RLP, not a list).
- `created_at` → `rlp_type_t::timestamp` from `[[rlp::timestamp]]`. The runtime serializes it as nanoseconds-since-epoch via `integer_codec_t`.
- `debug_counter` → absent entirely (`ignore`). Subsequent fields do NOT shift position on the wire because RLP encoding is driven by the descriptor, not by the C++ struct layout. The descriptor simply omits the ignored field.
- `chain_id` → `rlp_type_t::optional` with `item = &opt_1` where `opt_1.type = bytes`. The runtime emits RLP empty string (`0x80`) when the optional is null, or the `uint64_t` value when present.
- `doc` pointer is `nullptr` on all fields because no `[[rlp::doc]]` was applied at field scope. Class-level `doc` is not wired into field descriptors today.

---

## 8. Runtime architecture — Approach B (descriptors)

The h5cpp architectural pattern is **compiler emits descriptors → runtime consumes descriptors → I/O happens**. The RLP backend follows this exactly.

### Architecture

```
C++ header + [[rlp::...]] attributes
        ↓
h5cpp-compiler
        ↓
  ┌─────────────────┐
  │ constexpr desc  │  ← C++17 constexpr type descriptor
  │ (.rlp.hpp)      │     emitted into a single header
  └─────────────────┘
        ↓
  ┌─────────────────┐
  │ rlp::runtime    │  ← custom encode/decode (deferred)
  │ (header-only)   │     walks constexpr desc at runtime
  └─────────────────┘
        ↓
   RLP bytes ↔ C++ object
```

### Why descriptors over generated code

Same rationale as HDF5, JSON, MessagePack, CBOR, BSON, and Avro backends:
- **Single source of truth:** One compiler pass produces the descriptor.
- **No generated `.cpp` bloat:** Descriptors are `constexpr` tables.
- **Introspection:** Descriptors can be walked reflectively.
- **C++26 future:** P2996 reflection makes the constexpr descriptor layer optional.

### Relationship to the existing `rlp.hpp` header

The existing `rlp.hpp` at `/home/steven/projects/sigma-grant/include/crypto/rlp.hpp` implements **compile-time** RLP encode/decode via template metaprogramming (`is_unsigned_integral_v`, `is_tuple_like_v`, `has_as_tuple_v`, etc.). The h5cpp-compiler backend does not replace it; it complements it:

| | `rlp.hpp` (existing) | `rlp::meta::descriptor` (new) |
|---|---|---|
| Encode | `rlp::encode(obj)` — templates resolve at compile time | `rlp::runtime::encode(obj, descriptor)` — runtime walk |
| Decode | `rlp::decode<T>(bytes)` — templates resolve at compile time | `rlp::runtime::decode(bytes, descriptor)` — runtime walk |
| Reflection | No | Yes — `descriptor<T>::fields[]` is introspectable |
| Dynamic | No — type must be known at compile time | Yes — any struct given its descriptor |
| Size | Header-only, ~700 lines | Header-only descriptor + deferred runtime |

The two can coexist. A runtime implementation can call into `rlp.hpp`'s low-level `append_string_header`, `append_list_header`, `parse_item`, etc. primitives while using the descriptor for type routing.

### Runtime API (sketch — deferred)

```cpp
namespace rlp {
    // Encoding — descriptor-driven
    template<typename T>
    bytes_t encode(const T& obj);

    // Decoding — descriptor-driven
    template<typename T>
    T decode(const byte_t* data, size_t len);
}
```

The actual runtime will use the low-level primitives already present in `rlp.hpp` (`impl::append_into`, `impl::parse_item`, `impl::decode_item`) while routing through the descriptor table instead of compile-time template dispatch.

---

## 9. Open questions and design decisions

### Decided

| # | Decision | Rationale |
|---|---|---|
| 1 | **Runtime architecture:** Approach B (constexpr descriptors + runtime walk) | Consistent with all other backends. |
| 2 | **Output format:** Self-contained C++ header | The emitted file defines `rlp_type_t`, `field_desc`, `descriptor` base template, and all specializations. No external runtime header required at compile time. |
| 3 | **Only two wire types:** `bytes` and `list` | RLP has no other wire types. The descriptor faithfully reflects this minimalism. |
| 4 | **`std::chrono::system_clock::time_point` auto-detect → `timestamp`** | Matches Ethereum convention (timestamps in block headers). The user can also force it explicitly with `[[rlp::timestamp]]` on non-chrono types. |
| 5 | **`std::vector<uint8_t>` → `bytes`** | In RLP, a byte vector is a byte string, not a list. Distinguishes binary data from arrays of integers. |
| 6 | **`std::map<K,V>` → `bytes` fallback** | RLP has no native map type. The user should use `std::vector<std::pair<K,V>>` for list-of-pairs semantics. |
| 7 | **Positional encoding** | RLP has no field names on the wire. `json_name` in the descriptor is metadata-only. |

### Open

| # | Question | Context |
|---|---|---|
| 1 | **`std::map<K,V>` support.** | Should the compiler emit a diagnostic when it encounters `std::map<K,V>` in an RLP-annotated struct, or is the silent `bytes` fallback acceptable? |
| 2 | **Signed integer encoding edge cases.** | RLP's signed integer encoding strips leading zero bytes while preserving the sign bit. The descriptor emits `bytes` for all integers; the runtime must know whether to use signed or unsigned stripping rules. Should the descriptor carry a `signed` flag? |
| 3 | **Floating-point byte order.** | RLP has no native float type. The descriptor emits `bytes` for `float`/`double`. Should the runtime encode floats in big-endian (network order) or little-endian (native)? The existing `rlp.hpp` does not support floats. |
| 4 | **`as_tuple` / `from_tuple` custom types.** | The existing `rlp.hpp` supports user-defined types via `as_tuple()` and `from_tuple()` free functions. The compiler backend cannot see these custom functions in the AST. Should the compiler emit a placeholder descriptor that the user manually wires, or should we require structs to be plain aggregates? |
| 5 | **RLP canonical encoding.** | Ethereum requires canonical RLP (no leading zero bytes in integers, shortest possible length prefix). Should the descriptor carry a `canonical` flag, or should canonical encoding always be the default? |
| 6 | **Packed struct alignment.** | The descriptor carries `offsetof` for each field. If the user compiles with different packing pragmas on different platforms, `offsetof` may disagree. Should the descriptor include `alignof` or `sizeof` for each field as a cross-check? (Same open question as all other backends.) |

---

## 10. Sources

- `tasks/h5cpp-compiler-backend-cookbook.md` (workspace) — transferable recipe for adding any new backend
- `tasks/h5cpp-compiler-h5-attribute-taxonomy.md` (workspace) — HDF5 attribute vocabulary (same universal words, `h5::` namespace)
- `tasks/h5cpp-compiler-json-attribute-taxonomy.md` (workspace) — JSON Schema attribute vocabulary (same universal words, `json::` namespace)
- `tasks/h5cpp-compiler-msgpack-attribute-taxonomy.md` (workspace) — MessagePack attribute vocabulary (same universal words, `msgpack::` namespace)
- `tasks/h5cpp-compiler-cbor-attribute-taxonomy.md` (workspace) — CBOR attribute vocabulary (same universal words, `cbor::` namespace)
- `tasks/h5cpp-compiler-bson-attribute-taxonomy.md` (workspace) — BSON attribute vocabulary (same universal words, `bson::` namespace)
- `tasks/h5cpp-compiler-avro-attribute-taxonomy.md` (workspace) — Avro attribute vocabulary (same universal words, `avro::` namespace)
- `tasks/h5cpp-compiler-pb-attribute-taxonomy.md` (workspace) — protobuf attribute vocabulary (same universal words, `pb::` namespace)
- `vargalabs/h5cpp-compiler#38` — `src/consumer_rlp.hpp`, `src/h5_attr_translator.hpp`, `src/h5cpp.cpp`
- `/home/steven/projects/sigma-grant/include/crypto/rlp.hpp` — existing compile-time RLP encode/decode implementation
- [Ethereum Yellow Paper — Appendix B: Recursive Length Prefix](https://ethereum.github.io/yellowpaper/paper.pdf)
- [P2996R13 — Reflection for C++26](https://wg21.link/p2996)
- [P3394R4 — Annotations for Reflection](https://wg21.link/p3394)
