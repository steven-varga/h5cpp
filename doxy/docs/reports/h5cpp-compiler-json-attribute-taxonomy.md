@page reports_compiler_json_attribute_taxonomy json:: Attribute Vocabulary (JSON Schema Backend)

User-facing attribute set for JSON Schema annotations on plain C++ structs. **Vocabulary is intentionally identical to `h5::*` and `pb::*` where the concept overlaps** (rename, ignore, doc, alias) — different namespace, same words. The JSON-specific surface lives only under `json::*`, with validation keywords (`required`, `format`, `pattern`, `min`, `max`) that have no HDF5 or protobuf counterpart.

C++17 attribute syntax today; one-line lift to C++26 typed annotations tomorrow.

| Surface today (C++17 standard-attribute) | C++26 reflection form |
|---|---|
| `[[json::name("on_wire")]]` | `[[=json::name{"on_wire"}]]` |
| `[[json::ignore]]` | `[[=json::ignore{}]]` |
| `[[json::required]]` | `[[=json::required{}]]` |
| `[[json::format("date-time")]]` | `[[=json::format{"date-time"}]]` |
| `[[json::doc("description")]]` | `[[=json::doc{"description"}]]` |
| `[[json::alias("Name")]]` | `[[=json::alias{"Name"}]]` |
| `[[json::version("2020-12")]]` | `[[=json::version{"2020-12"}]]` |
| `[[json::pattern("^\\d{3}$")]]` | `[[=json::pattern{"^\\d{3}$"}]]` |
| `[[json::min(0)]]` | `[[=json::min{0}]]` |
| `[[json::max(100)]]` | `[[=json::max{100}]]` |

Only syntactic shift is `(args)` → `{args}` under the `[[=...]]` form. **Names stay put.**

---

## 2. Universal vocabulary — same words, `json::` namespace

These attributes use vocabulary identical to `h5::*` and `pb::*`. They live in `json::` so the namespace stays self-contained for JSON-only users; a user wanting multiple backends writes both `[[h5::name(...)]]` and `[[json::name(...)]]` (typically with the same string).

### Universal Tier 1 — must-have

| Attribute | Purpose | Example |
|---|---|---|
| `[[json::name("on_wire_name")]]` | Rename a field for the JSON Schema `properties` map. Decouples C++ identifier from the schema property name. Drives the key in `"properties": {"on_wire_name": {...}}`. | `[[json::name("display_name")]] std::string label;` |
| `[[json::ignore]]` | Skip this field entirely. Property absent from `"properties"`. | `[[json::ignore]] int debug_counter;` |

### Universal Tier 2 — high value, low cost

| Attribute | Purpose | Example |
|---|---|---|
| `[[json::doc("description")]]` | Emitted as `"description": "..."` on the field's property object. Self-documenting schemas. | `[[json::doc("nanoseconds since epoch")]] std::uint64_t ts;` |
| `[[json::alias("Name")]]` | **Class-level.** Emitted as `"title": "Name"` at the root schema object. The C++ type name still drives `$defs` keys and `$ref` targets. | `struct [[json::alias("Session")]] session_t { ... };` |

### Universal Tier 3 — nice to have

| Attribute | Purpose |
|---|---|
| `[[json::version("N")]]` | **Class-level.** Emitted as `"$schema": "https://json-schema.org/draft/N/schema"`. Default: `"2020-12"`. Pure metadata today; future versions may support `"2019-09"` or `"draft-07"` for toolchain compatibility. |
| `[[json::name_all("snake_case" \| "camelCase" \| "PascalCase")]]` | **Class-level.** Naming convention applied uniformly to every property key. Per-field `json::name` overrides. | `struct [[json::name_all("snake_case")]] user_event_t { ... };` |

The full universal list mirrors `h5cpp-compiler-h5-attribute-taxonomy.md` §2 and `h5cpp-compiler-pb-attribute-taxonomy.md` §2. Any universal `h5::*` or `pb::*` attribute not listed above has no JSON Schema semantics (e.g. `h5::chunk`, `h5::compress` are HDF5-storage concerns; `pb::field(N)`, `pb::wire` are protobuf-wire concerns).

---

## 3. JSON-specific vocabulary — tier 1..3

### Tier 1 — must-have

Without these, the JSON Schema backend either can't express common validation constraints (`required` is the most frequently used JSON Schema keyword after `type`) or loses access to format validation (`format` drives OpenAPI / Swagger toolchains).

| Attribute | Purpose | Example |
|---|---|---|
| `[[json::required]]` | Include this field in the root object's `"required"` array. The field must be present in any conforming JSON instance. Independent of `json::ignore` — a field can be ignored by the compiler (omitted from the schema entirely) or required (present in `"required"`), but never both. | `[[json::required]] std::int32_t id;` |
| `[[json::format("format_name")]]` | Add `"format": "format_name"` to the field's property object. Standard values: `"date-time"`, `"date"`, `"time"`, `"email"`, `"hostname"`, `"ipv4"`, `"ipv6"`, `"uri"`, `"uuid"`, `"int32"`, `"int64"`, `"float"`, `"double"`. Toolchains (AJV, fastjsonschema, pydantic) use `format` for runtime validation. | `[[json::format("email")]] std::string email;` |

### Tier 2 — high value, low cost

| Attribute | Purpose | Example |
|---|---|---|
| `[[json::pattern("regex")]]` | Add `"pattern": "regex"` to string fields. ECMA-262 regex syntax. | `[[json::pattern("^\\d{3}-\\d{4}$")]] std::string postal_code;` |

### Tier 3 — nice to have

| Attribute | Purpose |
|---|---|
| `[[json::min(V)]]` | Validation bound. Emitted as `"minimum": V` for numeric types, `"minLength": V` for strings, `"minItems": V` for arrays. Type-dispatched by the emitter. |
| `[[json::max(V)]]` | Validation bound. Emitted as `"maximum": V` for numeric types, `"maxLength": V` for strings, `"maxItems": V` for arrays. Type-dispatched by the emitter. |
| `[[json::tool_format("hint")]]` | Tool-specific format hint. Emitted as `"x-tool-format": "hint"` — a vendor extension key that standard validators ignore but custom tooling (e.g. code generators, UI form builders) can read. | `[[json::tool_format("currency:USD")]] double price;` |

---

## 4. Type map — C++ → JSON Schema

| C++ type | JSON Schema | Notes |
|---|---|---|
| `bool` | `{"type": "boolean"}` | |
| `char`, `signed char`, `unsigned char` | `{"type": "integer"}` | |
| `short`, `unsigned short` | `{"type": "integer"}` | |
| `int`, `unsigned int` | `{"type": "integer"}` | |
| `long`, `unsigned long` | `{"type": "integer"}` | |
| `long long`, `unsigned long long` | `{"type": "integer", "format": "int64"}` | |
| `float` | `{"type": "number"}` | |
| `double`, `long double` | `{"type": "number"}` | |
| `std::string` | `{"type": "string"}` | |
| `std::vector<T>` | `{"type": "array", "items": <T>}` | `T` may be primitive or `$ref` |
| `T[N]` (C array) | `{"type": "array", "items": <T>}` | Same emission as `std::vector<T>` |
| `enum class` | `{"type": "string"}` | Enum values are not enumerated in the schema today (future: `"enum": ["Red", "Green", "Blue"]`). |
| Nested struct `S` | `{"$ref": "#/$defs/S"}` | `S` is emitted into `$defs` first. |
| Pointer `T*` | `{"type": "object"}` | Fallback. Pointers have no natural JSON Schema representation. |
| `std::optional<T>` | `{"type": "object"}` | **Gap.** Should be `{<T>, "nullable": true}` or `[{"type": "null"}, <T>]` (union). Not yet implemented. |
| `std::map<K,V>` | `{"type": "object"}` | **Gap.** Should be `{"type": "object", "additionalProperties": <V>}`. Not yet implemented. |
| `std::variant<...>` | `{"type": "object"}` | **Gap.** Should be `{"oneOf": [...]}`. Not yet implemented. |

---

## 5. Attribute wiring status

### Implemented and tested

| Attribute | Where read | Where emitted | Test fixture |
|---|---|---|---|
| `json::ignore` | `h5_attr_reader::has_attr(fld, "json::ignore")` | Skips field in `properties` | `json_primitives` |
| `json::required` | `h5_attr_reader::has_attr(fld, "json::required")` | Appends to `"required"` array | `json_primitives`, `json_strings` |
| `json::format("...")` | `h5_attr_reader::read_field_string(fld, "json::format")` | Inserts `"format": "..."` into property object | `json_primitives`, `json_strings` |
| `json::name("...")` | `h5_attr_reader::read_field_string(fld, "json::name")` | Overrides property key | `json_strings` |
| `json::doc("...")` | `h5_attr_reader::read_class_string(node, "json::doc")` | Emitted as `"description": "..."` at root | `json_primitives`, `json_nested` |
| `json::alias("...")` | `h5_attr_reader::read_class_string(node, "json::alias")` | Emitted as `"title": "..."` at root | `json_primitives` |

### Whitelisted but not yet wired

| Attribute | Intended emission | Status |
|---|---|---|
| `json::pattern("regex")` | `"pattern": "regex"` on string fields | Not implemented |
| `json::min(V)` | `"minimum"` / `"minLength"` / `"minItems"` (type-dispatched) | Not implemented |
| `json::max(V)` | `"maximum"` / `"maxLength"` / `"maxItems"` (type-dispatched) | Not implemented |
| `json::tool_format("hint")` | `"x-tool-format": "hint"` vendor extension | Not implemented |
| `json::version("N")` | `"$schema": "https://json-schema.org/draft/N/schema"` | Not implemented (hardcoded to `"2020-12"`) |
| `json::name_all("convention")` | Naming convention applied to all property keys | Not implemented |

### Not applicable to JSON Schema (validation layer only)

These attributes have no JSON Schema keyword equivalent. They may still drive the **runtime** deserialization layer (Approach B, §6).

| Attribute | Schema reason | Runtime note |
|---|---|---|
| `json::on_missing` | JSON Schema is a validation grammar, not a deserialization runtime. Absence semantics live in the consumer code, not the schema. | **Runtime:** Drives the default value the deserializer writes when a field is absent. Equivalent to `h5::on_missing`. |
| `json::chunk` | HDF5 storage concern. | N/A |
| `json::compress` | HDF5 storage concern. | N/A |
| `json::serialize_full` | HDF5 tier-1 emission concern. | N/A |

---

## 6. Runtime architecture — Approach B with simdjson

The h5cpp architectural pattern is **compiler emits descriptors → runtime consumes descriptors → I/O happens**. The JSON backend follows this exactly.

### Architecture

```
C++ header + [[json::...]] attributes
        ↓
h5cpp-compiler
        ↓
  ┌─────────────────┐
  │ JSON Schema     │  ← sidecar for external validation (§4, §6)
  │ (.schema.json)  │
  └─────────────────┘
        ↓
  ┌─────────────────┐
  │ constexpr desc  │  ← C++17/20 constexpr type descriptor
  │ (generated .hpp)│     emitted into a companion header
  └─────────────────┘
        ↓
  ┌─────────────────┐
  │ json::runtime   │  ← simdjson-powered read / custom write
  │ (header-only)   │     walks constexpr desc at runtime
  └─────────────────┘
        ↓
   JSON text ↔ C++ object
```

### Why Approach B (descriptors) over generated code

Same rationale as HDF5 and protobuf backends:
- **Single source of truth:** One compiler pass produces both schema (for external tools) and descriptors (for C++ runtime).
- **No generated `.cpp` bloat:** Descriptors are `constexpr` tables; no `O(N_structs × M_fields)` lines of generated code to compile.
- **Introspection:** Descriptors can be walked reflectively for debugging, logging, schema migration.
- **C++26 future:** P2996 reflection makes the constexpr descriptor layer optional — the runtime can reflect directly on `T`.

### Why simdjson

| Concern | simdjson | nlohmann::json |
|---|---|---|
| **Parse speed** | ~1–3 GB/s (SIMD, stage-1/2 architecture) | ~100–300 MB/s (DOM tree) |
| **Memory model** | On-demand — fields parsed lazily, no full DOM | Full DOM allocated upfront |
| **Validation** | Strict — invalid UTF-8/numbers are errors by design | Lenient — accepts some invalid inputs |
| **Compile time** | Header-only, but large | Very large header, notorious for compile-time cost |
| **Writing** | Limited; custom serializer needed | Excellent `to_json`/`from_json` |

simdjson is chosen for **reading**. For **writing**, a lightweight custom serializer (also descriptor-driven) emits JSON text directly — no intermediate DOM. This matches the h5cpp philosophy of zero-copy, minimal-allocation I/O.

### Descriptor shape (sketch)

```cpp
namespace json::meta {
    enum class kind_t { boolean, integer, number, string, array, object, null };

    struct field_desc {
        const char* json_name;     // respects json::name
        kind_t kind;
        size_t offset;             // offsetof(T, member)
        const field_desc* item;    // for arrays: element descriptor
        bool required;             // json::required
        const char* format;        // json::format
        const char* pattern;       // json::pattern
        // ... min, max, tool_format
    };

    template<typename T>
    struct record_desc {
        static constexpr field_desc fields[] = { /* compiler-generated */ };
        static constexpr char title[] = "...";
        static constexpr char schema_id[] = "...";
    };
}
```

### Runtime API (sketch)

```cpp
namespace json {
    // Parsing — simdjson on-demand, descriptor-driven
    template<typename T>
    T parse(const char* json_text, size_t len);

    // Serialization — custom writer, descriptor-driven
    template<typename T>
    std::string serialize(const T& obj);

    // Optional: validate against JSON Schema before parsing
    template<typename T>
    bool validate(const char* json_text, size_t len);
}
```

### simdjson on-demand + descriptors

simdjson's `ondemand::document` parser walks JSON text incrementally. The runtime pairs this with the constexpr descriptor:

```cpp
template<typename T>
T parse(const char* json, size_t len) {
    simdjson::ondemand::parser parser;
    auto doc = parser.iterate(json, len);
    T obj{};
    for (auto field : doc.get_object()) {
        auto key = field.key();
        auto meta = lookup_field<T>(key);  // constexpr binary search on json::meta::record_desc<T>::fields
        if (!meta) { /* unknown field — skip or error */ }
        switch (meta->kind) {
            case kind_t::integer:   set_field<int>(obj, *meta, field.value()); break;
            case kind_t::string:    set_field<std::string>(obj, *meta, field.value()); break;
            case kind_t::array:     parse_array(obj, *meta, field.value()); break;
            // ...
        }
    }
    return obj;
}
```

This is **zero-allocation for primitives**, **lazy for nested objects**, and **descriptor-driven for type dispatch** — the exact pattern h5cpp uses for HDF5 compound types.

---

## 7. $defs and $ref semantics

Nested structs are emitted into `$defs` and referenced via `$ref`. The rules:

1. **Root struct** — the first matched struct becomes the root schema. Its `properties` are emitted at the top level.
2. **Dependencies** — any struct used as a field type (direct, via `std::vector<T>`, or via `T[N]`) is collected topologically and emitted into `$defs` before the root.
3. **Deduplication** — a struct appears in `$defs` exactly once, even if referenced from multiple fields.
4. **Self-reference** — a struct containing a pointer to itself (`Node* next`) emits `{"type": "object"}` for the pointer field. True cyclic value types (`struct Node { Node next; }`) are invalid C++ and rejected by Clang before the backend sees them.

Example:

```cpp
struct Inner { int value; };
struct Outer { Inner inner; };
```

Emits:

```json
{
  "$defs": {
    "Inner": {
      "type": "object",
      "properties": { "value": {"type": "integer"} }
    }
  },
  "type": "object",
  "properties": {
    "inner": {"$ref": "#/$defs/Inner"}
  }
}
```
