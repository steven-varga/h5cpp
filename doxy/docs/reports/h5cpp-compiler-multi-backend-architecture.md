@page reports_compiler_multi_backend_architecture h5cpp-compiler Multi-Backend Architecture

One C++ struct → many on-disk and over-the-wire artifacts. The h5cpp-compiler walks each user type with Clang LibTooling and dispatches to a set of independent **producers/consumers**, each emitting one artifact: HDF5 type registrations, Protobuf `.proto` + descriptor, JSON Schema, MsgPack/CBOR/BSON/Avro/RLP descriptors, and SQL DDL in three dialects (PostgreSQL, MySQL, SQLite3). Each backend reads its own top-level attribute namespace; HDF5 attributes (`[[h5::*]]`) seed cross-backend universals where they apply.

The same struct can be persisted to HDF5, serialised over MsgPack/CBOR/BSON/Avro/RLP, advertised as a Protobuf message + service, exposed as a JSON Schema (for LLM tool calls or contract validation), and migrated into a SQL warehouse — from one source of truth.

## Architecture

```
                  ┌────────────────────────────┐
                  │   AST walker (Clang        │   one pass per
                  │   LibTooling) — one        │   user type per
                  │   matcher per backend      │   invocation
                  └──────────────┬─────────────┘
                                 │ same type, same fields
   ┌──────┬──────┬──────┬───────┬┴───────┬──────┬──────┬──────┬─────┐
   │      │      │      │       │        │      │      │      │     │
   ▼      ▼      ▼      ▼       ▼        ▼      ▼      ▼      ▼     ▼
┌─────┐┌─────┐┌─────┐┌──────┐┌──────┐┌──────┐┌─────┐┌─────┐┌──────────────┐
│ H5  ││ PB  ││JSON ││MsgPk ││ CBOR ││ BSON ││Avro ││ RLP ││SQL × {pg,my,    │
└──┬──┘└──┬──┘└──┬──┘└──┬───┘└──┬───┘└──┬───┘└──┬──┘└──┬──┘│  sqlite3}        │
   │      │      │      │       │       │       │      │   └────────┬─────────┘
   ▼      ▼      ▼      ▼       ▼       ▼       ▼      ▼            ▼
generated  user_event   schema  binary  binary  binary schema schema  schema.sql
   .h     .pb.h+.proto  .json   descr   descr   descr   .avsc  descr  (postgres/
                                                                       mysql/
                                                                       sqlite3)
```

**Properties of this design:**

- **One walker, one matcher set, many emitters.** The Clang AST traversal infrastructure is shared. Each backend supplies a `*TemplateCallback` (consumer) and/or `*Producer` (emitter). The HDF5 and SQL backends share `H5TemplateCallback<P>` parameterised on the producer; other backends each have a dedicated consumer class.
- **Producers are independent.** Adding a new backend (e.g., FlatBuffers, Cap'n Proto) is a new `consumer_<fmt>.hpp` or `producer_<fmt>.hpp` plus an `OutputFormat` enum value and a `case` in the dispatch switch. No changes to the walker or other producers.
- **Attributes are scoped at the top level.** Each backend reads its own top-level attribute namespace — `[[pb::*]]`, `[[json::*]]`, `[[msgpack::*]]`, etc. No nesting under `h5::`. `[[h5::*]]` carries HDF5-specific knobs **and** the cross-backend universals (`h5::name`, `h5::doc`, `h5::ignore`, `h5::on_missing`); other backends consult these where applicable.
- **One format per invocation.** The CLI selects exactly one `--<format>` per run; CMake re-invokes the tool per output file. This keeps the dispatch matrix simple and lets each backend be debugged in isolation. The C++26 reflection vehicle (when it lands) will run all enabled emitters in a single TU compile.
- **SQL dialects via compile-time template.** `SqlProducer<SqlDialect::postgres|mysql|sqlite3>` — three explicit instantiations, one producer source.

## Attribute namespace layout

| Namespace | Scope | Read by |
|---|---|---|
| `[[h5::*]]` | HDF5 + cross-backend universals (`name`, `doc`, `ignore`, `on_missing`, `name_all`) | HDF5 producer; SQL producer (via shared `H5TemplateCallback`); other backends pick up `h5::name` / `h5::doc` / `h5::ignore` where applicable |
| `[[pb::*]]` | Protocol Buffers (fields, services, RPC, wire-level knobs) | `consumer_pb` + `consumer_proto` + `producer_pb` |
| `[[json::*]]` | JSON Schema (and LLM tool-calling envelopes) | `consumer_json` |
| `[[msgpack::*]]` | MessagePack | `consumer_msgpack` |
| `[[cbor::*]]` | CBOR (RFC 8949) | `consumer_cbor` |
| `[[bson::*]]` | BSON (MongoDB extended JSON) | `consumer_bson` |
| `[[avro::*]]` | Apache Avro | `consumer_avro` |
| `[[rlp::*]]` | Ethereum RLP | `consumer_rlp` |
| `[[ns::*]]` | C++ namespace-name override (cross-backend) | All backends — affects the emitted symbol's namespace |

There is no `[[sql::*]]` namespace yet: the SQL backend currently reuses the `[[h5::*]]` matcher and consumes `h5::name`, `h5::doc`, `h5::ignore`, `h5::on_missing`. A future `[[sql::*]]` set is planned for column-type overrides, dialect-specific defaults, and table-level constraints; for now the SQL DDL is derived from the C++ types and the universal subset.

## Universal attributes (cross-backend semantics)

These live in `[[h5::*]]` and have meaningful semantics across every backend that consults them.

| Attribute | Semantics across backends |
|---|---|
| `h5::name("on_disk_name")` | Field/struct rename — HDF5 dataset/field name, JSON property name, Avro field name, MsgPack/CBOR/BSON map-key, SQL column name. Per-backend `[[<backend>::name]]` overrides take precedence. |
| `h5::ignore` | Skip this field in HDF5 + SQL. Other backends use their own `[[<backend>::ignore]]` (currently: `pb::ignore`, `json::ignore`). |
| `h5::doc("description")` | HDF5 attribute, JSON Schema `description`, Avro `doc`, Protobuf trailing comment, SQL `COMMENT ON COLUMN`. |
| `h5::name_all("snake_case" \| "camelCase" \| "PascalCase" \| "kebab-case")` | Class-level naming convention applied to all fields uniformly across emitted artifacts. |
| `h5::on_missing(value)` | Default when field missing on read — HDF5 fill value, plus a hint that per-backend default mechanisms (e.g., `avro::default`, SQL `DEFAULT`) should match. |

## Backend-specific attributes (current state)

The lists below reflect what the current `*_attr_reader.hpp` and fixture suite actually parse. Each backend's attribute vocabulary is documented in its taxonomy report under `docs/reports/taxonomies/` (in this docs tree) and in the per-backend `*-attribute-taxonomy.md` file in the h5cpp-compiler repo.

### `[[h5::*]]` — HDF5

| Attribute | Purpose |
|---|---|
| `h5::name("on_disk_name")` | Field/struct on-disk rename |
| `h5::name_all("snake_case" \| ...)` | Class-level naming convention |
| `h5::ignore` | Skip field |
| `h5::doc("description")` | Documentation string (becomes HDF5 attribute) |
| `h5::chunk(N)` / `h5::chunk(N, M, ...)` | Dataset chunking |
| `h5::compress(filter, level)` | Compression filter |
| `h5::on_missing(value)` | Fill value on read |
| `h5::serialize_full` | Force inline serialisation for VLEN-eligible fields |

See `docs/reports/architecture/h5cpp-compiler-scatter-gather-design.md` for the full HDF5 attribute reference.

### `[[pb::*]]` — Protocol Buffers

| Attribute | Purpose |
|---|---|
| `pb::field(N)` | Field number (required by proto3) |
| `pb::name("wire_name")` | Wire-level field rename |
| `pb::ignore` | Skip field |
| `pb::doc("description")` | Trailing comment in `.proto` |
| `pb::reserved(N, ...)` / `pb::reserved("old_name", ...)` | Reserve field numbers or names |
| `pb::package("com.example.events")` | Class-level: target Protobuf package |
| `pb::service("Name")` | Mark struct as a `service Name { ... }` block (members of `std::function<Resp(Req)>` become `rpc` methods) |
| `pb::version(N)` | Schema version |
| `pb::on_missing(value)` | Default on read |
| `pb::wire("varint" \| "fixed32" \| ...)` | Wire-type override |
| `pb::packed` | Packed encoding for repeated scalars |
| `pb::enum_zero("VALUE")` | Specify proto3 enum zero value |
| `pb::target_syntax("proto2" \| "proto3")` | Class-level: syntax to emit |
| `pb::adapter(...)` / `pb::encode_with(...)` / `pb::reject(...)` | Codec customisation hooks |
| `pb::descriptor_set_out("path.fds")` | Emit a FileDescriptorSet alongside the `.proto` |

### `[[json::*]]` — JSON Schema

| Attribute | Purpose |
|---|---|
| `json::name("propertyName")` | Property name override (defaults to field name) |
| `json::ignore` | Omit field |
| `json::doc("description")` | JSON Schema `description` |
| `json::required` | Mark as required (default in 2020-12 draft is optional) |
| `json::format("uri" \| "date-time" \| "uuid" \| ...)` | JSON Schema `format` annotation |

The JSON producer currently emits a plain JSON Schema document (2020-12 draft). LLM tool-calling envelopes (OpenAI / Anthropic / MCP) are a planned class-level wrapper — not yet shipped.

### `[[avro::*]]` — Apache Avro

| Attribute | Purpose |
|---|---|
| `avro::name("FieldName")` | Field name override |
| `avro::doc("description")` | Avro `doc` field |
| `avro::required` | Mark required (Avro defaults non-null) |
| `avro::alias("OldName")` | Avro field alias (for schema evolution / multi-rename) |
| `avro::default(value)` | Avro field default |
| `avro::fixed(N)` | Avro `fixed` type with size N |
| `avro::decimal(precision, scale)` | Avro logical-type `decimal` |
| `avro::timestamp("millis" \| "micros" \| "nanos")` | Logical-type `timestamp-*` |
| `avro::datetime("date" \| "time-millis" \| ...)` | Logical-type `date`/`time-*` |

### `[[msgpack::*]]`, `[[cbor::*]]`, `[[bson::*]]`, `[[rlp::*]]`

These four share the same minimal pattern. Each backend recognises the keys below; backend-specific keys are listed beneath.

| Attribute | Purpose |
|---|---|
| `<fmt>::name("on_wire_name")` | Map-key / field rename |
| `<fmt>::alias("old_name")` | Read-compat alias |
| `<fmt>::doc("description")` | Description annotation |
| `<fmt>::required` | Mark required |

Backend-specifics:

- **CBOR:** `cbor::tag(N)` — semantic tag (RFC 8949 § 3.4)
- **MsgPack:** `msgpack::ext(type_byte)` — MsgPack extension type
- **BSON:** `bson::binary(subtype)`, `bson::datetime`, `bson::decimal(p, s)`, `bson::timestamp`
- **Avro:** see Avro table above (richer set)
- **RLP:** `rlp::timestamp` — RLP doesn't carry types, so the few hints that exist are about decoded interpretation only

### `[[ns::name("path::to::ns")]]`

Class-level attribute that overrides the C++ namespace path emitted for the type. Read by all backends; affects emitted symbol's qualified name in `generated.h` (HDF5), `.proto` package fallback (Protobuf), Avro `namespace`, etc.

## Worked example

One struct, multiple artifacts:

```cpp
struct [[h5::name_all("snake_case"),
        h5::doc("user-level event captured by the gateway"),
        pb::package("com.vargalabs.events"),
        pb::target_syntax("proto3"),
        pb::reserved(10, 11),
        avro::doc("user-level gateway event")]]
user_event_t {

    [[h5::name("ts"),
      h5::doc("nanoseconds since epoch"),
      pb::field(1),
      json::format("uint64"),
      avro::timestamp("nanos")]]
    std::uint64_t timestamp;

    [[h5::name("user"),
      pb::field(2),
      json::format("uuid"),
      h5::on_missing(0)]]
    std::uint32_t user_id;

    [[h5::doc("payload samples"),
      h5::chunk(1024),
      h5::compress(gzip, 9),
      pb::field(3),
      pb::packed,
      avro::doc("microvolts, 1 kHz sample rate")]]
    std::vector<double> values;

    [[h5::ignore,
      pb::ignore,
      json::ignore,
      h5::doc("runtime cache; never persisted")]]
    void* runtime_handle;
};
```

The producers emit:

- **HDF5 (`generated.h`)** — compound type with VLEN field for `values`, chunked dataset with gzip-9, `runtime_handle` skipped, `timestamp` and `user_id` as native types. Field names taken from `h5::name`; `h5::doc` becomes attributes on the dataset.
- **Protobuf (`user_event.pb.h` + `user_event.proto`)** — message `UserEvent` in package `com.vargalabs.events`, syntax `proto3`, fields numbered 1/2/3 with 10/11 reserved. `values` is `repeated double [packed = true]`. `runtime_handle` absent (the `pb::ignore` hides it from the proto schema too).
- **JSON Schema (`user_event.schema.json`)** — Draft 2020-12 schema, properties `ts`, `user`, `values` with `format` hints. `runtime_handle` absent.
- **Avro (`user_event.avsc`)** — `{"type": "record", "name": "user_event_t", "fields": [...]}` with logical-type tags on `timestamp` and per-field `doc` annotations.
- **MsgPack / CBOR / BSON / RLP** — type-tagged binary descriptors for each format, derived from the struct layout. No fixture-driven knobs touched in this example, so the output uses field names from `h5::name` where present.
- **SQL (`events.sql`)** — `CREATE TABLE user_event_t (ts BIGINT, user INTEGER DEFAULT 0, values DOUBLE PRECISION[]);` (PostgreSQL dialect — actual statement varies per `--sql-postgres` / `--sql-mysql` / `--sql-lite3`). `runtime_handle` skipped via `h5::ignore`.

`runtime_handle` is absent from every artifact because of the universal `h5::ignore` + the explicit per-backend `pb::ignore` and `json::ignore`. `h5::doc` text propagates to Avro `doc` and HDF5 attributes; per-backend `<fmt>::doc` overrides take precedence where present.

## CLI invocation

One format per invocation. The compiler dispatches on a single `--<format>` selector:

```bash
# HDF5 compound type registrations (the canonical use case)
h5cpp-compiler --hdf5 -o generated.h \
    user_event.cpp -- -std=c++17 -I/usr/include

# Protocol Buffers — descriptor header + .proto schema
h5cpp-compiler --protobuf -o user_event.pb.h --proto-out user_event.proto \
    user_event.cpp -- -std=c++17 -I/usr/include

# JSON Schema
h5cpp-compiler --json -o user_event.schema.json \
    user_event.cpp -- -std=c++17

# Binary serialisation descriptors
h5cpp-compiler --msgpack -o user_event.msgpack.h  user_event.cpp -- -std=c++17
h5cpp-compiler --cbor    -o user_event.cbor.h     user_event.cpp -- -std=c++17
h5cpp-compiler --bson    -o user_event.bson.h     user_event.cpp -- -std=c++17
h5cpp-compiler --avro    -o user_event.avsc       user_event.cpp -- -std=c++17
h5cpp-compiler --rlp     -o user_event.rlp.h      user_event.cpp -- -std=c++17

# SQL — one invocation per dialect
h5cpp-compiler --sql-postgres -o events.postgres.sql  events.cpp -- -std=c++17
h5cpp-compiler --sql-mysql    -o events.mysql.sql     events.cpp -- -std=c++17
h5cpp-compiler --sql-lite3    -o events.sqlite.sql    events.cpp -- -std=c++17
```

A `--check` mode verifies that an existing generated file is up to date (exit code 1 if stale) — useful as a CI / pre-commit gate.

## CMake helper status

The shipped `h5cpp_compiler_generate()` helper currently accepts **two** of the eleven backends:

```cmake
# cmake/H5CPPCompilerFunctions.cmake — current public API
h5cpp_compiler_generate(
    INPUT  ${CMAKE_CURRENT_SOURCE_DIR}/user_event.cpp
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated.h
    FORMAT hdf5                      # or: protocol-buffers
    [STD c++17]
    [STUB_DIR /path/to/stubs]
)
```

`FORMAT` is validated against the regex `^(hdf5|protocol-buffers)$`. The other nine backends (json, msgpack, cbor, bson, avro, rlp, sql-postgres, sql-mysql, sql-lite3) are invoked directly via `add_custom_command(COMMAND h5cpp-compiler --<format> ...)`. Wiring them all into the CMake helper is a planned follow-up — straightforward extension to the FORMAT regex plus per-format output-extension defaults.

## Reflection vs Clang Tooling — both vehicles, same producers

Under C++26 reflection (the "tomorrow" vehicle from `h5cpp-reflection-cpp26-roadmap.md`), each producer collapses into a constexpr-time template function inside h5cpp itself:

```cpp
namespace h5 {
template <class T> constexpr auto emit_hdf5_compound_type();  // uses std::meta::*
template <class T> constexpr auto emit_proto_schema();
template <class T> constexpr auto emit_json_schema();
template <class T> constexpr auto emit_avro_schema();
template <class T> constexpr auto emit_sql_ddl(SqlDialect = SqlDialect::postgres);
// ... msgpack / cbor / bson / rlp ...
}
```

Under Clang Tooling (today), the same producers are header-only `*Producer` / `*TemplateCallback` classes in `h5cpp-compiler/src/`. Same per-backend logic, different traversal mechanism.

The user-facing surface — annotations on user structs, call to `h5::write(...)` for HDF5, build-system steps for other artifacts — is identical across both vehicles. See the reflection roadmap doc for the transition plan.
