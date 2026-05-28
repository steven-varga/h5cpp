@page reports_compiler_next_backends_difficulty_ranking h5cpp-compiler — Next-Backend Difficulty Ranking

**Author:** Winston (System Architect)
**Date:** 2026-05-23 (initial); revised 2026-05-23 — Arrow split into two approaches, HDF5-storage variant moved to rank 6
**Subject:** which wire/serialization formats could be added next to `vargalabs/h5cpp-compiler` following the six-step backend cookbook recipe, ranked from least to most difficult
**Companions:**
- `tasks/h5cpp-compiler-backend-cookbook.md` — the recipe each new backend would follow
- `tasks/h5cpp-compiler-multi-backend-architecture.md` — the multi-backend roof + namespace layout
- `tasks/h5cpp-compiler-hdf5-vs-protobuf-comparison.md` — the two backends shipping today

## TL;DR

Ten candidate format/approach pairs, ranked by integration difficulty against the cookbook recipe:

| Rank | Format | Wire shape | Schema? | Cookbook fit | Effort estimate |
|---|---|---|---|---|---|
| 1 | **JSON Schema** | text JSON | the schema IS the artifact | ✔ ok | 2–3 days |
| 2 | **MessagePack** | binary, schemaless tagged | ○ na | ✔ ok | 3–4 days |
| 3 | **CBOR** | binary, schemaless tagged (RFC 8949) | ○ na | ✔ ok | 4–5 days |
| 4 | **BSON** | binary, document-oriented | ○ na | ✔ ok | 4–5 days |
| 5 | **Avro** | binary, schema-required (JSON schema) | ✔ ok — Avro IDL or JSON | ✔ ok | 1 week |
| 6 | **Apache Arrow (on HDF5)** | columnar, HDF5-group-of-datasets storage | ✔ ok — Arrow Schema in group attribute | ✔ ok — reuses the HDF5 backend's emission | ~2 weeks |
| 7 | **Thrift** | binary (TBinary / TCompact), schema-required | ✔ ok — `.thrift` IDL | ◇ cancelled — IDL emitter is substantial | ~2 weeks |
| 8 | **FlatBuffers** | binary zero-copy, schema-required | ✔ ok — `.fbs` IDL | ✘ failed — user's struct ≠ wire layout, needs Builder emission | 3–4 weeks |
| 9 | **Cap'n Proto** | binary zero-copy + capabilities + RPC | ✔ ok — `.capnp` IDL | ✘ failed — same as FlatBuffers + RPC + capabilities | 4–6 weeks |
| 10 | **Apache Arrow (IPC from scratch)** | columnar IPC stream + FlatBuffers schema | ✔ ok — Arrow Schema (a FlatBuffers binary) | ✘ failed — competes with Parquet; heavy runtime dep | 6+ weeks |

The first six reuse the row-struct view of user data unchanged AND reuse the existing HDF5 emission infrastructure where applicable. Ranks 7–10 either require substantial schema-emission work (Thrift) or fundamentally re-shape what "the user's struct" means on the wire (FlatBuffers, Cap'n Proto, Arrow-IPC-from-scratch).

**Arrow's two paths.** Apache Arrow appears twice because there are two genuinely different ways to ship it:
- **Rank 6 (Arrow-on-HDF5):** each Arrow column becomes one HDF5 dataset inside a group; the Arrow Schema lives as a group attribute. Reuses h5cpp's existing scatter/gather + filter infrastructure verbatim — adds only the column-layout convention. Drops from a 6+-week project to ~2 weeks. **Recommended path.**
- **Rank 10 (Arrow IPC from scratch):** ship a full Arrow IPC stream encoder + FlatBuffers schema builder. Competes with Parquet (which Arrow already recommends for persistence). Worth doing only if a zero-HDF5 Arrow-only deployment becomes a real requirement.

See §6 below for the Arrow-on-HDF5 design; the columnar-transpose problem that made rank-9 hard in the original ranking is sidestepped because h5cpp's existing row-struct-to-storage relationship already handles half the work.

---

## 1. JSON Schema — `--json-out <file.schema.json>`

The easiest possible new backend. There is no wire encoding/decoding to implement; the artifact IS the schema document.

**Format characteristics**
- Text JSON (RFC 8259)
- Schema document conforming to JSON Schema 2020-12 draft
- Used by API documentation, LLM tool-calling envelopes (OpenAI / Anthropic / MCP), form validation

**Output artifact**
- `<name>.schema.json` — a JSON object describing the user's struct as a JSON Schema

**Type map (C++ → JSON Schema)**

| C++ | JSON Schema |
|---|---|
| `bool` | `{"type": "boolean"}` |
| `int32_t`/`int64_t` | `{"type": "integer", "format": "int32"\|"int64"}` |
| `float`/`double` | `{"type": "number"}` |
| `std::string` | `{"type": "string"}` |
| `std::vector<T>` | `{"type": "array", "items": <T>}` |
| `std::optional<T>` | `<T>` with `"nullable": true` |
| `std::map<K,V>` | `{"type": "object", "additionalProperties": <V>}` (K limited to string) |
| `std::variant<...>` | `{"oneOf": [<T1>, <T2>, ...]}` |
| `enum class` | `{"enum": [<v1>, <v2>, ...]}` |
| nested user struct | `{"$ref": "#/$defs/<Name>"}` + define in `$defs` |

**Cookbook fit**
- ✔ ok — same source rewriter + readers + walker
- ✔ ok — type map straight-line
- ✔ ok — no runtime library needed (artifact-only)
- ✔ ok — bonus: `json::tool_format("openai"\|"anthropic"\|"mcp")` class-level wraps the schema in the relevant tool-calling envelope

**Backend-specific attributes worth shipping**
- `json::required` — by default JSON Schema fields are optional; flip via attribute
- `json::format("uri"\|"date-time"\|"uuid"\|"email"\|...)` — JSON Schema's format hint
- `json::pattern("regex")` — string validation
- `json::min(v)` / `json::max(v)` — numeric bounds
- `json::camel_case` / `json::snake_case` — per-field naming (overrides class-level `name_all`)
- `json::tool_format(...)` — wrap output in LLM tool-call envelope

**Effort estimate:** 2–3 days. ~300 lines of new emitter code + fixtures.

**Status:** ○ na — not started; **highest-leverage next backend** per the multi-backend architecture roadmap.

---

## 2. MessagePack — `--msgpack-out` not needed; `pack`/`unpack` runtime

Binary, schemaless, tagged. The "schema" is implicit in the type tags written into each byte stream.

**Format characteristics**
- Binary, big-endian
- Tagged single-byte type prefixes for primitives + length prefixes
- Schemaless — type info is in the bytes themselves
- Used by Redis, RPC frameworks, Erlang/Elixir bridges

**Output artifact**
- `<name>_msgpack.hpp` — a C++ header with `msgpack::pack<T>(buf, msg)` / `msgpack::unpack<T>(buf, size)` specializations
- No schema file — schemaless

**Type map (C++ → MessagePack)**

| C++ | MessagePack tag |
|---|---|
| `bool` | 0xc2 (false) / 0xc3 (true) |
| `int32_t`/`int64_t` | 0xd2 / 0xd3 (signed int) |
| `uint32_t`/`uint64_t` | 0xce / 0xcf (unsigned int) |
| `float`/`double` | 0xca / 0xcb |
| `std::string` | `str` family (0xa0–0xbf for ≤31 chars, 0xd9/0xda/0xdb for longer) |
| `std::vector<T>` | `array` (0x90–0x9f / 0xdc / 0xdd) |
| `std::map<K,V>` | `map` (0x80–0x8f / 0xde / 0xdf) |
| nested user struct | `array` of fixed length (positional) OR `map` keyed by field name |

**Cookbook fit**
- ✔ ok — straight-line type map
- ✔ ok — header-only runtime feasible (similar to `pb.hpp`'s shape)
- ◇ cancelled — design decision: positional `array`-style serialization vs named `map`-style; tradeoff is size vs schema-evolution friendliness

**Backend-specific attributes worth shipping**
- `msgpack::ext(N)` — MessagePack extension types (0xd4–0xd8, 0xc7–0xc9)
- `msgpack::positional` / `msgpack::keyed` — per-class layout choice
- `msgpack::timestamp` — extension type 0xff (built-in MessagePack timestamp format)

**Effort estimate:** 3–4 days. ~500 lines + sibling header runtime.

**Status:** ○ na — interesting candidate for the Enclave BLE path (compact wire) and sigma queue serialization.

---

## 3. CBOR — `--cbor-out` not needed; `encode`/`decode` runtime

RFC 8949 standardized binary format. MessagePack's spiritual successor with IETF blessing.

**Format characteristics**
- Binary, big-endian
- Major types in the top 3 bits of the first byte; arg in the bottom 5
- Schemaless, like MessagePack
- Rich tag system (RFC 8949 §3.4) for type semantics (date-time, decimal fraction, regex, MIME, UUID, …)
- Used by COSE (CBOR Object Signing & Encryption), IoT protocols, IETF specs

**Output artifact**
- `<name>_cbor.hpp` — encode/decode specializations
- No schema file — schemaless

**Type map (C++ → CBOR)**

Like MessagePack but with the RFC-defined major-type encoding. Notably:
- Major type 0/1: unsigned/negative integer (varint-style sized prefix)
- Major type 2/3: byte string / text string
- Major type 4/5: array / map
- Major type 6: tag (semantic annotation on the following value)
- Major type 7: simple/float/break

**Cookbook fit**
- ✔ ok — type map straight-line
- ✔ ok — header-only runtime feasible
- ◇ cancelled — design decisions: which CBOR tags to emit (e.g. tag 0 / 1 for `chrono::time_point`?), how indefinite-length forms are handled

**Backend-specific attributes worth shipping**
- `cbor::tag(N)` — emit a specific RFC 8949 semantic tag wrapping the value
- `cbor::canonical` — class-level: emit in CBOR Canonical encoding (sorted map keys, shortest-form integers) for deterministic byte output

**Effort estimate:** 4–5 days. ~600 lines (similar to MessagePack + tag dispatch).

**Status:** ○ na — could pair with COSE for signed/encrypted Enclave payloads.

---

## 4. BSON — `--bson-out` not needed; `encode`/`decode` runtime

MongoDB's binary JSON. Document-oriented; length-prefixed.

**Format characteristics**
- Binary, little-endian (unlike most of the others)
- Document-oriented: top-level is always a key-value map
- Length-prefixed at the document level
- Used by MongoDB drivers, BSON-RPC frameworks

**Output artifact**
- `<name>_bson.hpp` — encode/decode specializations
- No schema file

**Type map (C++ → BSON)**

| C++ | BSON type byte |
|---|---|
| `bool` | 0x08 |
| `int32_t` | 0x10 |
| `int64_t` | 0x12 |
| `double` | 0x01 |
| `std::string` | 0x02 (UTF-8 string) |
| `std::vector<T>` | 0x04 (array — actually a document with integer string keys) |
| `std::map<std::string, V>` | 0x03 (embedded document) |
| `std::chrono::system_clock::time_point` | 0x09 (UTC datetime, int64 ms since epoch) |
| `std::variant<...>` | n/a — BSON has no native discriminated union; emit as embedded doc with a "type" key |
| nested user struct | 0x03 (embedded document) |

**Cookbook fit**
- ✔ ok — straight-line type map
- ✔ ok — header-only feasible
- ○ na — variant/oneof has no native BSON shape; emit a discriminated-document convention

**Backend-specific attributes worth shipping**
- `bson::object_id` — emit a member as an ObjectId (type 0x07)
- `bson::decimal128` — IEEE 754-2008 decimal (type 0x13)
- `bson::binary_subtype(N)` — BSON binary subtype tag

**Effort estimate:** 4–5 days. Similar shape to MessagePack/CBOR but document-oriented requires field-name encoding.

**Status:** ○ na — most useful if a MongoDB ingestion path matters (probably not for current vargalabs use cases).

---

## 5. Avro — `--avro-out <file.avsc>`

Schema-required binary format. Schema itself is a JSON document.

**Format characteristics**
- Binary wire format (compact varint-style)
- Schema document in JSON (`.avsc` file)
- Schema-on-write — the schema must travel with the data OR be agreed out-of-band
- Rich type system: records, enums, arrays, maps, unions, fixed-size byte arrays, logical types (decimal, date, time, timestamp, UUID)
- Used by Apache Kafka, Hadoop ecosystem, Confluent Schema Registry

**Output artifact**
- `<name>.avsc` — JSON Avro schema
- Optionally `<name>_avro.hpp` — encode/decode specializations against the schema (could defer to apache avro-cpp instead)

**Type map (C++ → Avro)**

| C++ | Avro |
|---|---|
| `bool` | `boolean` |
| `int32_t` | `int` |
| `int64_t` | `long` |
| `float` | `float` |
| `double` | `double` |
| `std::string` | `string` |
| `std::vector<T>` | `{"type": "array", "items": <T>}` |
| `std::map<std::string, V>` | `{"type": "map", "values": <V>}` |
| `std::optional<T>` | `["null", <T>]` (union) |
| `std::variant<...>` | union of the alternative types |
| `enum class` | `{"type": "enum", "symbols": [...]}` |
| `std::chrono::system_clock::time_point` | `{"type": "long", "logicalType": "timestamp-micros"}` |
| nested user struct | `{"type": "record", "name": "Foo", "fields": [...]}` |

**Cookbook fit**
- ✔ ok — type map cleaner than proto3's (Avro unions are natural for `optional`/`variant`)
- ✔ ok — schema file is JSON; same emission infrastructure as the JSON Schema backend (run them in sequence)
- ✔ ok — runtime can be deferred to apache avro-cpp; h5cpp-compiler ships just the schema artifact

**Backend-specific attributes worth shipping**
- `avro::namespace("com.acme.events")` — Avro namespace
- `avro::aliases({"old_name_1", "old_name_2"})` — Avro multi-rename history (more flexible than proto3 reserved)
- `avro::logical_type("decimal"\|"uuid"\|"timestamp-millis"\|...)` — logical type annotation
- `avro::default(value)` — Avro field default (schema-resolution mechanism)

**Effort estimate:** ~1 week. ~600 lines of schema emitter; runtime deferred to library.

**Status:** ○ na — natural sibling to the proto3 backend; useful for Kafka-bridged pipelines.

---

## 6. Apache Arrow (on HDF5) — `--arrow-on-h5-out <file.h5>`

**The recommended Arrow path.** Use HDF5 as the storage layer; layout convention follows the cookbook recipe verbatim. Drops Arrow from a 6+-week project (rank 10) to ~2 weeks because the existing HDF5 backend already does the load-bearing work (chunked datasets, filter plugins, attribute emission, parallel I/O). The columnar-transpose problem that made the standalone Arrow path hard becomes a small "transpose at the read/write boundary" function instead of a novel codegen problem.

**Format characteristics**
- Storage layer: HDF5 file (so all of HDF5's strengths — parallel I/O, hierarchy, mature ecosystem — come along)
- Wire view: columnar (each Arrow column → one HDF5 dataset inside a "table" group)
- Schema descriptor: stored as a JSON string in a group attribute
- Per-column compression via HDF5's Blosc-ZSTD / Blosc-LZ4 filter plug-ins (Parquet's main codecs)
- Row-group abstraction: implicit via uniform chunk shape across the column datasets
- Predicate pushdown: optional, via min/max attributes on each chunk

**Output artifact**
- `<name>.h5` — one HDF5 file containing one or more table groups, each laid out as:
  ```
  /readings/                        ← table group
      @arrow_schema_json: "..."     ← Arrow Schema (JSON form)
      @arrow_version: "16.0"
      @row_count: 1_000_000
      device_id                      ← HDF5 dataset = Arrow column
          @arrow_type: "utf8"
          @dictionary_ref: "/readings/_dicts/device_id"
          [chunked, blosc-zstd]
      timestamp_ns
          @arrow_type: "timestamp(NANO, UTC)"
          @min: 1717000000000000000  ← predicate-pushdown stat
          @max: 1717999999000000000
          [chunked, blosc-zstd]
      value
          @arrow_type: "float64"
          [chunked, blosc-zstd]
      _validity/device_id            ← null-bitmap sidecar
      _dicts/device_id               ← dictionary-encoded values sidecar
  ```

**Type map (Arrow column type → HDF5 dataset type)**

| Arrow | HDF5 |
|---|---|
| `boolean`, `int*`, `uint*`, `float*` | `H5T_NATIVE_*` |
| `utf8`, `binary` | VLEN string / VLEN binary |
| `timestamp(unit, tz)` | int64 + `@arrow_type` attribute |
| `decimal128` | opaque 16-byte fixed-length |
| `list<T>` | `H5Tvlen_create(T)` |
| `map<K,V>` | group with `keys` + `values` subdatasets |
| `struct<...>` | `H5T_COMPOUND` |
| `dense_union` / `sparse_union` | compound with discriminant + opaque value |
| `dictionary(K, V)` | enum type, OR `indices` + `values` sibling datasets |

**Cookbook fit**
- ✔ ok — reuses h5cpp's existing HDF5 backend; the Arrow-on-HDF5 emitter is a layout convention + attribute schema, not a new format implementation
- ✔ ok — HDF5 attribute emission already exists (`h5::doc(...)` uses the same code path); storing the Arrow Schema JSON is one more attribute
- ✔ ok — chunked datasets ARE row groups (set uniform chunk shape across the column datasets)
- ✔ ok — HDF5 filter plug-ins (Blosc-ZSTD, Blosc-LZ4) match Parquet's main compression codecs
- ◇ cancelled — predicate-pushdown is library-side work (emit min/max attributes during write; consult them during read); not a format limitation
- ◇ cancelled — dictionary encoding needs explicit `_dicts/` subgroup convention; not automatic like Parquet
- ✔ ok — parallel I/O ✔ ok via HDF5's MPI integration for the POD subset

**Backend-specific attributes worth shipping**
- `arrow::table_name("name")` — table name in the HDF5 group hierarchy
- `arrow::dictionary_encode` — request dictionary encoding for a column (compact representation of repeated strings)
- `arrow::extension_type("uuid"\|"json"\|...)` — Arrow Extension Types
- `arrow::chunk_size(N)` — explicit row-group size (defaults to the column's `h5::chunk` value)
- `arrow::null_handling("validity"\|"sentinel")` — class-level: bitmap dataset vs reserved sentinel value

**The user-facing shape**

```cpp
struct [[h5::chunk(65536),
        arrow::table_name("readings")]]
sensor_reading_t {
    [[arrow::dictionary_encode]] std::string  device_id;
    [[h5::name("ts_ns")]]         std::int64_t timestamp_ns;
    [[h5::name("value")]]         double       value;
};
```

The same annotated struct drives both the HDF5 row-storage AND the Arrow columnar view. h5cpp-compiler emits:
- HDF5 register_struct / scatter / gather (for row-oriented writes via the existing tier 2 codegen)
- Arrow-on-HDF5 layout emitter (writes the column-per-dataset layout for analytics consumers)
- A `transpose<T>` library helper that materializes `arrow::RecordBatch` from the column datasets at read time

**Effort estimate:** ~2 weeks. Mostly layout-convention + attribute-emission code on top of the existing HDF5 backend. The expensive piece (HDF5 plumbing) is already done.

**Compared to Parquet**

| Parquet feature | Arrow-on-HDF5 feature |
|---|---|
| Per-column storage | ✔ ok one HDF5 dataset per column |
| Per-column compression | ✔ ok HDF5 filter plug-ins (Blosc-ZSTD matches Parquet ZSTD; Blosc-LZ4 matches Parquet LZ4) |
| Row groups | ✔ ok uniform chunk shape implicit row-group |
| Predicate pushdown | ◇ cancelled — needs explicit min/max attributes (library-side) |
| Dictionary encoding | ◇ cancelled — explicit `_dicts/` subgroup (library-side) |
| Bloom filters | ✘ failed — sidecar dataset needed; not automatic |
| Parallel I/O | ✔ ok — HDF5's MPI integration (Parquet has no canonical parallel-write story) |
| Hierarchical files (multiple tables in one file) | ✔ ok — HDF5 nests groups freely (Parquet is one-table-per-file) |
| Ecosystem (pandas, Spark, DuckDB, …) | ◇ cancelled — needs Arrow-on-HDF5 readers in those tools; Vaex demonstrates the model works |

**Status:** ○ na — strongest near-term Arrow path. Aligns with the "h5cpp as analytics storage" thesis. Vaex already demonstrates the model is viable at scale.

---

## 7. Thrift — `--thrift-out <file.thrift>`

Apache Thrift. Has its own IDL AND wire format.

**Format characteristics**
- Multiple wire protocols: `TBinaryProtocol` (verbose, debuggable), `TCompactProtocol` (varint-encoded, like proto3), `TJSONProtocol` (JSON-style)
- Has its own IDL (`.thrift` files) parsed by `thrift` codegen tool
- Service definitions like proto3 + gRPC
- Used by Facebook, Apache Cassandra, Apache HBase

**Output artifact**
- `<name>.thrift` — IDL file
- The `.thrift` is then run through the `thrift` compiler to produce C++ (or other language) bindings — h5cpp-compiler doesn't replace this; it just produces the IDL

**Type map (C++ → Thrift IDL)**

Similar to proto3:
- `bool`/`byte`/`i16`/`i32`/`i64`/`double`/`string`/`binary` — natural primitives
- `list<T>` / `set<T>` / `map<K,V>` — container types
- `struct` for nested records
- `union` for tagged unions (similar to proto3 oneof)
- `enum` for enum class
- `service` for RPC

**Cookbook fit**
- ✔ ok — type map mostly straight-line
- ◇ cancelled — IDL emitter is substantial (Thrift IDL has slightly richer syntax than proto3: `optional`/`required`, multiple containers, includes)
- ✔ ok — service emission similar to `pb::service`

**Backend-specific attributes worth shipping**
- `thrift::protocol("binary"\|"compact"\|"json")` — class-level: selects which protocol is the intended wire
- `thrift::optional` / `thrift::required` — field qualifier (proto3 dropped `required`; Thrift kept it)
- `thrift::namespace("cpp", "com.acme")` — language-specific namespace selectors
- `thrift::service("Name")` — RPC service block

**Effort estimate:** ~2 weeks. ~800 lines of IDL emitter; runtime fully deferred to apache-thrift-cpp.

**Status:** ○ na — pb backend already covers most of the same use cases; Thrift mainly relevant for orgs already on a Thrift stack.

---

## 8. FlatBuffers — `--flatbuffers-out <file.fbs>`

Zero-copy serialization. The wire bytes ARE the in-memory layout.

**Format characteristics**
- Schema-required (`.fbs` IDL)
- Wire bytes are LITERALLY the in-memory representation — readers access fields via pointer arithmetic + offset table, no parse step
- Schema-driven Builder pattern: you don't serialize a struct, you BUILD the wire layout directly via `Builder` API
- Used by Google for performance-critical paths, game engines (LDtk), Apache Arrow's metadata (yes, Arrow uses FlatBuffers internally)

**Output artifact**
- `<name>.fbs` — FlatBuffers IDL
- The `.fbs` is processed by `flatc` to produce a Builder API and accessor classes — these don't match the user's struct shape

**Type map (C++ → FlatBuffers)**

Type system is rich but the codegen pattern is fundamentally different:
- `bool`/`int*`/`float`/`double`/`string` — natural
- `[T]` — vector
- `table Foo { ... }` — primary aggregate type (offset-table-driven, all fields optional)
- `struct Foo { ... }` — fixed-layout aggregate (no optional fields, no growth)
- `union` for tagged unions
- `enum` for enums

**Cookbook fit**
- ✘ failed — the user's natural struct definition (`struct foo_t { int x; std::string y; }`) cannot be serialized AS IS. FlatBuffers requires a Builder API. h5cpp-compiler would have to emit a parallel set of `FooBuilder` / `FooReader` classes
- ◇ cancelled — IDL emission piece is doable, similar to Thrift
- ✘ failed — the runtime piece (Builder API generation) is genuinely large — flatc is a substantial code generator in itself

**Backend-specific attributes worth shipping**
- `flatbuffers::table` / `flatbuffers::struct` — class-level: which aggregate type to emit
- `flatbuffers::file_identifier("FOOO")` — 4-byte file identifier for type-checking the wire bytes
- `flatbuffers::root_type` — class-level: marks the root table

**Effort estimate:** 3–4 weeks. ~2000 lines if we just emit the IDL and defer Builder generation to `flatc`; substantially more if we generate the Builder ourselves.

**Status:** ○ na — better to defer to `flatc` for the Builder generation. h5cpp-compiler's role would be ONLY the `.fbs` emitter from annotated C++.

---

## 9. Cap'n Proto — `--capnp-out <file.capnp>`

Like FlatBuffers but with capabilities + RPC.

**Format characteristics**
- Schema-required (`.capnp` IDL)
- Zero-copy wire format (same philosophy as FlatBuffers)
- Capability references (object handles that survive serialization)
- Built-in RPC system with promise pipelining
- Used by sandstorm.io, distributed-system frameworks

**Output artifact**
- `<name>.capnp` — IDL
- Processed by `capnp` codegen to produce reader/builder classes

**Type map (C++ → Cap'n Proto)**

Similar to FlatBuffers but with extra dimensions:
- Primitives: `Bool`/`UInt8..64`/`Int8..64`/`Float32/64`/`Text`/`Data`
- `List<T>` for vectors
- `struct Foo { ... }` for aggregates
- `union { ... }` inside a struct for variant
- `enum Foo { ... }`
- `interface Foo { ... }` for RPC

**Cookbook fit**
- ✘ failed — same Builder-API problem as FlatBuffers
- ✘ failed — capability references have no direct C++-struct analog (they're object IDs in a shared memory or RPC session)
- ✘ failed — the RPC system is its own substantial scope (promise pipelining, distributed object references)

**Backend-specific attributes worth shipping**
- `capnp::interface("Name")` — RPC interface block
- `capnp::struct` — annotation choosing struct vs. inline encoding
- `capnp::const(value)` — Cap'n Proto-style file-level constants

**Effort estimate:** 4–6 weeks if including RPC; ~3 weeks for IDL-emission only.

**Status:** ○ na — relevant only if a Cap'n Proto-based system is already in the stack.

---

## 10. Apache Arrow (IPC from scratch) — `--arrow-ipc-out <file.arrow>`

The fallback Arrow path when HDF5 is not an option. This is the rank-6 §6 Arrow-on-HDF5 entry's expensive twin: same columnar format, no HDF5 storage layer to lean on. You write the IPC stream/file format yourself, link against `libarrow`, and own the transpose codegen.

**Pick rank 6 (Arrow-on-HDF5) over rank 10 unless:** the consumer pipeline explicitly requires `.arrow` IPC files (e.g., Arrow Flight endpoints), HDF5 is banned (rare — but happens in some pure-cloud-object-store stacks), or you want the canonical `.arrow` file extension for tool-discoverability.

**Format characteristics**
- Columnar layout: per-column contiguous arrays, not per-row
- Schema descriptors in FlatBuffers (Arrow uses FlatBuffers for its metadata — yes, two formats deep)
- IPC stream / file format for serialization between processes
- Used by pandas, Spark, DataFusion, DuckDB, ClickHouse, Polars — every modern analytics stack

**Output artifact**
- `<name>.arrow` — Arrow IPC file (RecordBatch sequence + footer)
- `<name>_arrow.hpp` — a `transpose<T>(std::vector<T>) → arrow::RecordBatch` helper that h5cpp-compiler generates from the annotated struct

**Type map (C++ → Arrow)**

| C++ row | Arrow column type |
|---|---|
| `bool` | `arrow::boolean()` |
| `int32_t` | `arrow::int32()` |
| `int64_t` | `arrow::int64()` |
| `float`/`double` | `arrow::float32()` / `arrow::float64()` |
| `std::string` | `arrow::utf8()` |
| `std::vector<T>` | `arrow::list(T)` |
| `std::map<K,V>` | `arrow::map(K, V)` |
| `std::optional<T>` | T with null bitmap |
| `std::variant<...>` | `arrow::dense_union(...)` |
| `std::chrono::system_clock::time_point` | `arrow::timestamp(NANO)` |
| `enum class` | `arrow::dictionary(int32, utf8)` (or plain int32) |
| nested user struct | `arrow::struct_({...})` |

**Cookbook fit**
- ✘ failed — Arrow's COLUMNAR layout means the user's row-struct can't be serialized as-is. h5cpp-compiler has to emit a `transpose<T>` builder that pivots `std::vector<T>` into Arrow column arrays at runtime
- ✘ failed — runtime dep on `libarrow` is heavy (~50MB shared library)
- ✘ failed — IPC file format requires writing FlatBuffers-encoded Schema headers + RecordBatch dictionaries; reuses none of h5cpp's existing infrastructure
- ◇ cancelled — schema-only mode (no transpose, no IPC writer) is doable in ~2 weeks but barely useful: just emits a `.arrow.schema` file the user can pass to `pyarrow` for ground-truth schema checks

**Backend-specific attributes worth shipping**
- (same as the Arrow-on-HDF5 set; `arrow::table_name`, `arrow::dictionary_encode`, `arrow::extension_type`, etc.)

**Effort estimate:** 6+ weeks. Transpose codegen is non-trivial (must handle nested structs, vlen lists, dense unions, null bitmaps) and `libarrow` linkage adds a heavy runtime dependency. Schema-only is ~2 weeks but barely justifies a separate backend.

**Status:** ○ na — defer until a concrete Arrow Flight or DuckDB integration demands `.arrow` IPC files specifically. The Arrow-on-HDF5 path (rank 6) covers the same analytics-storage thesis for ~3× less effort.

---

## Recommended order of attack

If the goal is **maximum backend coverage with least incremental effort**, ship in this order:

1. **JSON Schema** (rank 1) — lays the universal-attribute / multi-backend roof groundwork; LLM tool-calling envelope is a high-leverage cross-cutting feature
2. **MessagePack** (rank 2) — useful for sigma queue / Enclave compact-wire paths; trains the team on header-only binary emit/decode
3. **CBOR** (rank 3) — small additional surface over MessagePack; opens COSE-signed payloads
4. **Avro** (rank 5) — schema-only is largely a JSON variant; pairs with the existing Kafka/Confluent ecosystem
5. **Apache Arrow (on HDF5)** (rank 6) — analytics storage via the existing HDF5 backend + layout convention + attribute emission; opens the pandas/Spark/DuckDB consumer surface at ~2 weeks effort. Demonstrates "h5cpp as Parquet" in the cookbook
6. **BSON** (rank 4) — only if a MongoDB ingest path becomes relevant
7. **Thrift** (rank 7) — only if a Thrift-stack consumer surfaces; pb backend covers ~85% of the same use cases
8. **FlatBuffers** / **Cap'n Proto** / **Apache Arrow (IPC from scratch)** — deferred until the Builder-API question is designed and a concrete IPC consumer materializes; out of scope for the v1 multi-backend roof per the multi-backend architecture's "ship order". Apache Arrow IPC specifically is superseded by the Arrow-on-HDF5 path for most analytics use cases

If the goal is **proving the cookbook recipe really transfers**, ship ranks 1 + 2 + 5 in that order (JSON Schema, MessagePack, Avro). Three backends covering text-schema, binary-schemaless, and binary-schemaful — every combinatorial corner of the design space exercised, the recipe is then known-good. Then rank 6 (Arrow-on-HDF5) demonstrates "the recipe extends to analytics storage too" without leaving the HDF5 comfort zone.

---

## References

- `tasks/h5cpp-compiler-backend-cookbook.md` — the recipe each new backend follows
- `tasks/h5cpp-compiler-multi-backend-architecture.md` — `h5::json::*` / `h5::sql::*` / `h5::avro::*` namespace layout (this doc proposes `json::*` / `avro::*` / etc. as standalone namespaces; same Phase A→D migration story as `pb::*`)
- `tasks/h5cpp-compiler-hdf5-vs-protobuf-comparison.md` — the two shipping backends; this doc's row-by-row reference
- JSON Schema 2020-12 draft: https://json-schema.org/draft/2020-12
- MessagePack spec: https://github.com/msgpack/msgpack/blob/master/spec.md
- CBOR (RFC 8949): https://www.rfc-editor.org/rfc/rfc8949.html
- BSON spec: https://bsonspec.org/spec.html
- Apache Avro spec: https://avro.apache.org/docs/current/specification/
- Apache Thrift: https://thrift.apache.org/docs/idl
- FlatBuffers schema: https://flatbuffers.dev/flatbuffers_guide_writing_schema.html
- Cap'n Proto schema: https://capnproto.org/language.html
- Apache Arrow format: https://arrow.apache.org/docs/format/Columnar.html
