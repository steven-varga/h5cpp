@page reports_compiler_multi_backend_architecture h5cpp-compiler Multi-Backend Architecture

**Date:** 2026-05-21
**Authors:** Steven Varga, Winston (Architecture)
**Status:** Design; tier-1 (HDF5) implemented today; tier-2 backends (Protobuf, JSON Schema, SQL DDL, Avro) on the AI roadmap
**Repos:** vargaconsulting/h5cpp, vargaconsulting/h5cpp-compiler

## TL;DR

One C++ struct → many on-disk and over-the-wire artifacts. The h5cpp-compiler (or the C++26 reflection-based equivalent inside h5cpp) walks each user type exactly once and dispatches to a set of independent **producers**, each emitting its own artifact: HDF5 type registrations, Protobuf `.proto`, JSON Schema, SQL DDL, Avro schemas. Each producer reads its own attribute namespace; universal attributes apply across all.

The same struct can be persisted to disk as HDF5, exposed as an LLM tool-call schema via JSON Schema, advertised as a Protobuf message to an RPC server, and migrated into a SQL warehouse — from one source of truth.

## Architecture

```
                  ┌────────────────────────────┐
                  │   AST walker / reflection  │   one pass per
                  │   visits each user type    │   user type
                  └──────────────┬─────────────┘
                                 │ same type, same fields
       ┌─────────────┬───────────┼───────────┬─────────────┐
       │             │           │           │             │
       ▼             ▼           ▼           ▼             ▼
   ┌───────┐    ┌─────────┐  ┌───────┐  ┌───────┐    ┌─────────┐
   │ HDF5  │    │  proto  │  │ JSON  │  │  SQL  │    │  Avro   │
   │producer    │producer │  │producer  │producer    │producer │
   └───┬───┘    └────┬────┘  └───┬───┘  └───┬───┘    └────┬────┘
       │             │           │          │             │
       ▼             ▼           ▼          ▼             ▼
generated.h     mytype.proto  schema.json  schema.sql  mytype.avsc
```

**Properties of this design:**

- **One walk, multiple artifacts.** The AST traversal is shared; each producer is a separate emission strategy.
- **Producers are independent.** Adding a new backend (e.g., FlatBuffers, GraphQL SDL) is a single new producer + an entry in the dispatch table. No changes to the walker or other producers.
- **Attributes are scoped.** Each producer reads only its own attribute namespace (plus the universal set). No cross-talk; orthogonal annotations don't interfere.
- **Both vehicles work.** Whether the dispatch happens in `h5cpp-compiler` (Clang Tooling, today) or via C++26 reflection (header-only, future), the producer set and attribute model are identical. See `h5cpp-reflection-cpp26-roadmap.md` for the vehicle split.

## Attribute namespace layout

| Namespace | Scope | Who reads it |
|---|---|---|
| `h5::` (bare) | Universal **and** HDF5-specific | All producers (universal subset) + HDF5 producer |
| `h5::proto::` | Protobuf-specific | Protobuf producer only |
| `h5::json::` | JSON Schema-specific | JSON producer only |
| `h5::sql::` | SQL DDL-specific | SQL producer only |
| `h5::avro::` | Avro-specific | Avro producer only |

HDF5 sits at the top level (no `h5::h5::` redundancy) because h5cpp is the project; other backends get clearly-demarcated sub-namespaces. Producers ignore attributes outside their scope.

## Universal attributes (read by every producer)

These live directly in `h5::` and have meaningful semantics across every backend.

| Attribute | Semantics across backends |
|---|---|
| `h5::name("on_disk_name")` | Field/struct rename — applied uniformly: HDF5 dataset/field name, JSON property name, Protobuf field name, SQL column name, Avro field name |
| `h5::ignore` | Skip this field in **all** backends |
| `h5::doc("description")` | HDF5 attribute, JSON Schema `description`, Protobuf trailing comment, SQL `COMMENT ON COLUMN`, Avro `doc` |
| `h5::version(N)` | Schema version (used by every backend's evolution hooks) |
| `h5::alias("old_name")` | Backward-compat read; applied where the backend supports name aliases (Avro `aliases`, JSON Schema `$ref` evolution, Protobuf `reserved`) |
| `h5::on_missing(value)` | Default when field missing on read — JSON `default`, Avro `default`, SQL `DEFAULT`, HDF5 fill value |
| `h5::name_all("snake_case" \| "camelCase" \| "PascalCase" \| "kebab-case")` | Class-level naming convention applied to all fields uniformly |

## Backend-specific attributes (working sketches)

These are first-pass sketches; final attribute lists per backend will be tightened during implementation of each producer.

### `h5::` — HDF5 (already specified)

See `h5cpp-compiler-scatter-gather-design.md` § "User-Facing Attribute System" for the full list. Highlights: `h5::chunk`, `h5::max_dims`, `h5::current_dims`, `h5::compress`, `h5::filter`, `h5::storage_type`, `h5::fixed_string`, `h5::serialize_full`.

### `h5::proto::` — Protocol Buffers

| Attribute | Purpose |
|---|---|
| `proto::field_number(N)` | Required for protobuf — `.proto` mandates explicit numbers |
| `proto::reserved(N1, N2, ...)` | Reserve field numbers for backward compatibility |
| `proto::oneof("group_name")` | Group fields into a `oneof` |
| `proto::packed` | Packed encoding for repeated scalars |
| `proto::deprecated` | Mark field deprecated |
| `proto::map_key` / `proto::map_value` | For `map<K,V>` field decomposition |
| `proto::package("com.example.events")` | Class-level: target package |

### `h5::json::` — JSON Schema (and LLM tool-calling envelopes)

| Attribute | Purpose |
|---|---|
| `json::camel_case` / `json::snake_case` / `json::pascal_case` | Per-field naming convention (overrides class-level) |
| `json::required` | Mark required (default: optional in JSON Schema) |
| `json::format("uri" \| "date-time" \| "uuid" \| ...)` | Format hint for validation |
| `json::pattern("regex")` | Regex constraint on string fields |
| `json::min(v)` / `json::max(v)` | Numeric bounds |
| `json::enum_values(v1, v2, ...)` | Enumerated value set |
| `json::tool_format("openai" \| "anthropic" \| "mcp")` | Class-level: wrap output in the tool-calling envelope for the chosen LLM API |

### `h5::sql::` — SQL DDL

| Attribute | Purpose |
|---|---|
| `sql::primary_key` | Mark column(s) as primary key |
| `sql::foreign_key("other_table.col")` | Foreign key constraint |
| `sql::index(unique = true)` | Index hint; `unique` optional |
| `sql::nullable` / `sql::not_null` | Nullability override (default derived from C++ type) |
| `sql::default(value)` | SQL `DEFAULT` clause (distinct from `h5::on_missing` — SQL DEFAULT applies at insert time) |
| `sql::column_type("VARCHAR(255)")` | Explicit SQL type override |
| `sql::table_name("foo")` | Class-level: table name override (default: snake_case of struct name) |
| `sql::dialect("postgres" \| "mysql" \| "sqlite")` | Class-level: dialect-specific syntax |

### `h5::avro::` — Apache Avro

| Attribute | Purpose |
|---|---|
| `avro::default(value)` | Avro field default (used for schema evolution) |
| `avro::logical_type("timestamp-millis" \| "decimal" \| "uuid" \| ...)` | Avro logical-type annotation |
| `avro::aliases({"old_name_1", "old_name_2"})` | Avro field aliases (multi-rename history) |
| `avro::namespace("com.example.events")` | Class-level: Avro namespace |

## Worked example

One struct, five artifacts:

```cpp
struct [[h5::name_all("snake_case"),
        h5::doc("user-level event captured by the gateway"),
        h5::version(2),
        h5::sql::table_name("events"),
        h5::sql::dialect("postgres"),
        h5::proto::reserved(10, 11),
        h5::proto::package("com.vargalabs.events"),
        h5::avro::namespace("com.vargalabs.events"),
        h5::json::tool_format("anthropic")]]
user_event_t {

    [[h5::name("ts"),
      h5::doc("nanoseconds since epoch"),
      h5::proto::field_number(1),
      h5::sql::primary_key,
      h5::sql::index,
      h5::json::format("uint64"),
      h5::avro::logical_type("timestamp-nanos")]]
    uint64_t timestamp;

    [[h5::name("user"),
      h5::proto::field_number(2),
      h5::json::format("uuid"),
      h5::sql::index,
      h5::on_missing(0)]]
    uint32_t user_id;

    [[h5::doc("payload samples"),
      h5::chunk(1024),
      h5::compress(gzip, 9),
      h5::proto::field_number(3),
      h5::proto::packed,
      h5::json::min(-1e6),
      h5::json::max( 1e6),
      h5::sql::column_type("DOUBLE PRECISION[]")]]
    std::vector<double> values;

    [[h5::ignore,
      h5::doc("runtime cache; never persisted")]]
    void* runtime_handle;
};
```

The producers emit:

- **HDF5 (`generated.h`)** — compound type with VLEN field for `values`, chunked dataset with gzip-9, `runtime_handle` skipped, `timestamp` and `user_id` as native types. Uses `h5::name` for field names (`ts`, `user`), `h5::doc` becomes HDF5 attributes on the dataset.
- **Protobuf (`user_event.proto`)** — message `UserEvent` (PascalCase by proto convention; the JSON naming convention doesn't apply) in package `com.vargalabs.events` with fields numbered 1, 2, 3 and 10/11 reserved. `values` is `repeated double [packed = true]`. `runtime_handle` absent.
- **JSON Schema (`user_event.schema.json`)** — wrapped in Anthropic tool-call envelope (`{"name": "...", "description": "...", "input_schema": { ... }}`). Properties `ts`, `user`, `values` with `format`, `min`/`max` constraints applied. `runtime_handle` absent.
- **SQL (`events.sql`)** — `CREATE TABLE events (ts BIGINT PRIMARY KEY, user INTEGER DEFAULT 0, values DOUBLE PRECISION[], ...)` with indexes on `ts` and `user`. Postgres-dialect array column for `values`.
- **Avro (`user_event.avsc`)** — `{"type": "record", "namespace": "com.vargalabs.events", "name": "UserEvent", "fields": [...]}` with logical-type tags and defaults.

`runtime_handle` is absent from every artifact because of the universal `h5::ignore`. Same `h5::doc` content seeds the per-backend documentation field. The C++ struct stays one place; no parallel schema files to maintain.

## CMake / command-line invocation

The compiler accepts a multi-valued `FORMATS` option and emits one artifact per requested format:

```bash
h5cpp --hdf5 --protocol-buffers --json --sql --avro \
    -o generated.h \
    --proto-out user_event.proto \
    --json-out  user_event.schema.json \
    --sql-out   events.sql \
    --avro-out  user_event.avsc \
    user_event.cpp -- -std=c++17 -I/usr/include
```

In CMake, extending the existing `h5cpp_compiler_generate` helper:

```cmake
h5cpp_compiler_generate(
    INPUT  ${CMAKE_CURRENT_SOURCE_DIR}/user_event.cpp
    FORMATS hdf5 protocol-buffers json sql avro
    OUTPUT_HDF5  ${CMAKE_CURRENT_BINARY_DIR}/generated.h
    OUTPUT_PROTO ${CMAKE_CURRENT_BINARY_DIR}/user_event.proto
    OUTPUT_JSON  ${CMAKE_CURRENT_BINARY_DIR}/user_event.schema.json
    OUTPUT_SQL   ${CMAKE_CURRENT_BINARY_DIR}/events.sql
    OUTPUT_AVRO  ${CMAKE_CURRENT_BINARY_DIR}/user_event.avsc
)
```

The single-FORMAT form already exists (today's `h5cpp_compiler_generate` with `FORMAT hdf5`). Multi-value `FORMATS` is the addition.

## Policy macros (per-backend cascade vs strict)

The annotation + call-site composition policy from the scatter/gather design is per-backend. Each producer has its own toggle, mirroring the existing `H5CPP_CONVERSION_*` pattern:

| Backend | Strict-default macro implied | Opt-in cascade |
|---|---|---|
| HDF5 | (default — no macro) | `H5CPP_LAYOUT_CASCADE` |
| Protobuf | (default) | `H5CPP_PROTO_CASCADE` |
| JSON | (default) | `H5CPP_JSON_CASCADE` |
| SQL | (default) | `H5CPP_SQL_CASCADE` |
| Avro | (default) | `H5CPP_AVRO_CASCADE` |

In practice the call-site override pattern is most relevant for HDF5 (where `h5::write(fd, "ds", obj, h5::chunk{1024})` exists today). Most other backends generate artifacts at build time and have no call-site equivalent — their cascade macros exist for symmetry and future call-site I/O paths (e.g., `h5::write_json(fd, obj, json::camel_case{})` if that ever ships).

## Reflection vs Clang Tooling — both vehicles, same producers

Under C++26 reflection (the "tomorrow" vehicle from `h5cpp-reflection-cpp26-roadmap.md`), each producer lives as a constexpr-time template function inside h5cpp itself:

```cpp
namespace h5 {
template <class T> constexpr auto emit_hdf5_compound_type()  { /* uses std::meta::* */ }
template <class T> constexpr auto emit_proto_schema()        { /* same walk, different output */ }
template <class T> constexpr auto emit_json_schema()         { /* … */ }
template <class T> constexpr auto emit_sql_ddl()             { /* … */ }
template <class T> constexpr auto emit_avro_schema()         { /* … */ }
}
```

Under Clang Tooling (the "today" vehicle), the same producers are C++ classes inside `h5cpp-compiler` that walk the AST and emit text. Same per-backend logic, different traversal mechanism.

The user-facing surface — annotations on user structs, call to `h5::write(...)` for HDF5, build-system steps for other artifacts — is identical across both vehicles. See the reflection roadmap doc for the full transition plan.

## Relation to other workspace documents

- **`tasks/h5cpp-compiler-scatter-gather-design.md`** — defines tier classification, attribute system at the field level (the `h5::` namespace), and the cascade/strict policy macro for HDF5. This doc extends that to multiple backends.
- **`tasks/h5cpp-reflection-cpp26-roadmap.md`** — defines the dual-vehicle strategy (reflection vs Clang Tooling) and the C++26 transition. This doc shows that strategy holds across all backends.
- **`memory: project_h5cpp_compiler_ai_roadmap`** (Claude auto-memory pointer) — captures the four-backend ship order: Protobuf → JSON → SQL → Avro. This doc is the technical spec the roadmap pointed at.
- **`tasks/h5cpp-compiler-prior-art-survey.md`** — competitive landscape; relevant because the multi-backend story is what differentiates h5cpp-compiler from `rootcling` (ROOT-only) and from serde (Rust, format-agnostic but no native HDF5).

## Open questions and follow-ups

1. **Per-backend default attribute resolution order.** When `h5::name`, `h5::sql::name`, and `h5::sql::column_name` are all present (varying specificity), which wins? Need to spec a precedence rule per backend.
2. **Cross-backend type-system mismatches.** Some C++ types map cleanly to some backends and awkwardly to others (e.g., `std::variant<A,B,C>` is natural in Avro union, contortion in SQL, opaque in HDF5). Document the per-backend coping strategy for each tier-2/tier-3/tier-4 type.
3. **Producer registration mechanism.** How is a new backend added? A static registry in `h5cpp-compiler`? A header-only producer template under `h5cpp/codegen/<backend>/`? Decision affects the extensibility story for third parties.
4. **Class-level naming convention propagation.** `h5::name_all("snake_case")` at class level — does it propagate into per-backend producers automatically, or does each backend have its own `*_name_all`? Suggest: propagate by default; backend-specific override available.
5. **`h5::tool_format("openai" | "anthropic" | "mcp")` envelope wrappers.** The JSON producer wrapping output in tool-calling envelopes is what makes h5cpp-compiler an AI-friendly tool. Worth its own design pass: what does an "MCP server tool descriptor" envelope look like, exactly? Reference: Anthropic's MCP spec.
6. **CMake API stability.** The multi-format `h5cpp_compiler_generate` invocation is a breaking change to the helper. Worth a major-version bump for the CMake helper module; document migration.
7. **Compilation cost.** Five producers running on every walk multiplies cost by ~5x in the worst case. Mitigation: emit only the requested FORMATS; cache producer outputs; in the reflection vehicle, each backend producer is independent and parallelizable.

## Implementation phasing (cross-reference)

The order from `project_h5cpp_compiler_ai_roadmap`:

1. **Protobuf** — finish the stub already wired in `h5cpp-compiler` (`--protocol-buffers` flag added 2026-05-21)
2. **JSON Schema + C++ codec** — highest reuse: schema for contracts, codec for actual JSON I/O
3. **SQL DDL** — smallest type-mapping surface; most users have a DB
4. **Avro** — nearly free once JSON Schema is done; same type catalogue, different envelope

Each step is one new producer + an entry in the dispatch table. The walker and attribute infrastructure stay the same.

## Sources

- `tasks/h5cpp-compiler-scatter-gather-design.md` (in-workspace; design source for tier classification and h5cpp:: attribute set)
- `tasks/h5cpp-reflection-cpp26-roadmap.md` (in-workspace; dual-vehicle strategy)
- `tasks/h5cpp-compiler-prior-art-survey.md` (in-workspace; competitive context)
- [Protocol Buffers Style Guide](https://protobuf.dev/programming-guides/style/)
- [JSON Schema 2020-12](https://json-schema.org/draft/2020-12)
- [Apache Avro Specification](https://avro.apache.org/docs/current/specification/)
- [Anthropic MCP Specification](https://modelcontextprotocol.io/specification)
- [OpenAI Function Calling JSON Schema format](https://platform.openai.com/docs/guides/function-calling)
