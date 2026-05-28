@page reports_compiler_hdf5_vs_protobuf_comparison h5cpp-compiler — HDF5 vs. Protobuf Side-by-Side Comparison

**Author:** Winston (System Architect)
**Date:** 2026-05-23
**Subject:** the two compiler-assisted backends of `vargalabs/h5cpp-compiler` (`--hdf5` and `--protocol-buffers`), compared row-by-row
**Companions:**
- `tasks/h5cpp-compiler-multi-backend-architecture.md` — the multi-backend roof
- `tasks/h5cpp-compiler-backend-cookbook.md` — the recipe shared by both backends
- `tasks/h5cpp-compiler-scatter-gather-design.md` — HDF5 tier-1..4 + universal attribute set
- `tasks/h5cpp-compiler-pb-attribute-taxonomy.md` — protobuf `pb::*` vocabulary

## TL;DR

Both backends are driven by the same `h5cpp` binary, share the same source rewriter + AST readers + topological walker, and follow the same six-step backend cookbook. They diverge in **vocabulary** (`h5::*` vs `pb::*`), **type map** (HDF5 native types vs proto3 wire types), and **output artifacts** (binary HDF5 header vs C++ descriptor + `.proto` schema).

This document is the row-by-row companion to the prose discussion in `tasks/pb-feature-coverage-and-gaps.md` §5.

---

## Shared foundations — what both backends ride on

| Layer | What it does | File |
|---|---|---|
| Source rewriter | Lowers `[[ns::attr(args)]]` → `[[clang::annotate("ns::attr", args)]]` so Clang preserves args in the AST | `src/pb_attr_translator.hpp` |
| AST readers | `find_annotate`, `read_string_arg`, `read_int_args`, `read_first_arg_text`, class/field/enum convenience wrappers | `src/pb_attr_reader.hpp` |
| Matcher | Matches structs via `h5::*` / `pb::*` call sites in user code | `src/h5cpp.cpp` (`h5templateMatcher`, `pbTemplateMatcher`) |
| MatchFinder fan-out | One AST pass dispatches to N registered callbacks | `src/h5cpp.cpp::main` |
| Test harness | Single fixture file produces multiple per-backend goldens | `tests/run_fixture.cmake` |
| Six-step cookbook | Vocabulary → rewriter → callback → type map → CLI → fixtures | `tasks/h5cpp-compiler-backend-cookbook.md` |

---

## Invocation + output artifacts

| | HDF5 backend | Protobuf backend |
|---|---|---|
| CLI mode | `--hdf5` (default when no other flag) | `--protocol-buffers` |
| Primary output flag | `-o <file.h>` | `-o <file_pb.hpp>` |
| Schema output flag | ○ na — HDF5 stores type info inside the file | `--proto-out <file.proto>` (opt-in) |
| Primary artifact | C++ header with `register_struct<T>()` (tier 1) or `scatter<T>` + `gather<T>` (tier 2+) | C++ header with `pb::meta::descriptor_t<T>` + optional `pb::default_for_t<>` sidecars |
| Schema artifact | binary HDF5 type table embedded in the dataset itself | text `.proto` schema + (future) `.desc` binary `FileDescriptorSet` |
| Runtime library | `vargaconsulting/h5cpp` | `vargalabs/sandbox` (`pb.hpp` + `pb_compression.hpp` + `pb_async.hpp`) |
| Status | ✔ ok — shipping today on staging | ✔ ok — branches `30-pb-producer` + `31-pb-attribute-vocabulary` pushed, awaiting PR |

---

## Universal vocabulary — same semantics across both

These attributes carry the SAME concept in each backend, with the corresponding format-specific emission.

| Concept | `h5::` | `pb::` | h5cpp emits | pb emits |
|---|---|---|---|---|
| Rename field/struct | `h5::name("disk_name")` | `pb::name("on_wire")` | HDF5 dataset/field name | `.proto` field/message name |
| Skip field | `h5::ignore` | `pb::ignore` | absent from compound | `pb::ignore<&T::m>{}` + absent from `.proto` |
| Doc string | `h5::doc("desc")` | `pb::doc("desc")` | HDF5 attribute on dataset | `//` comment in `.proto` |
| Default on missing | `h5::on_missing(value)` | `pb::on_missing(value)` | HDF5 fill value | `pb::default_for_t<>` sidecar template |
| Schema version | `h5::version(N)` | `pb::version(N)` | HDF5 file attribute | `option (h5cpp.schema_version) = N;` |
| Legacy name | `h5::alias("old")` | `pb::alias("old")` | accept on read | `reserved "old";` in `.proto` |
| Naming convention | `h5::name_all("snake_case")` | `pb::name_all("snake_case")` | uniform field names | uniform `.proto` field names |

All seven concepts ✔ ok on both backends.

---

## Field-level C++ → format type mapping

| C++ type | HDF5 emission | Protobuf emission |
|---|---|---|
| `bool` | `H5T_NATIVE_HBOOL` | `bool` |
| `int32_t` | `H5T_NATIVE_INT32` | `int32` |
| `int64_t` | `H5T_NATIVE_INT64` | `int64` |
| `uint32_t` | `H5T_NATIVE_UINT32` | `uint32` |
| `uint64_t` | `H5T_NATIVE_UINT64` | `uint64` |
| `float` | `H5T_NATIVE_FLOAT` | `float` |
| `double` | `H5T_NATIVE_DOUBLE` | `double` |
| `std::string` | VLEN string | `string` |
| fixed-size `T[N]` / `std::array<T,N>` | `H5Tarray_create(base, rank, dims)` | ○ na — proto3 has no fixed-size array |
| `std::vector<T>` | `H5Tvlen_create(T)` (tier 2 scatter/gather) | `repeated T` |
| `std::optional<T>` | tier 3 — discriminated | `optional T` (proto3 explicit presence) |
| `std::map<K,V>` / `std::unordered_map<K,V>` | tier 3 — decomposed group OR nested VLEN | `map<K, V>` |
| `std::variant<...>` | tier 4 — `H5T_OPAQUE` + tag dataset | `oneof { ... }` |
| `std::chrono::system_clock::time_point` | int64 + epoch convention | `google.protobuf.Timestamp` (via `pb::adapter("Timestamp")`) |
| `std::chrono::nanoseconds` | int64 | `google.protobuf.Duration` (via `pb::adapter("Duration")`) |
| `enum class` | persisted as underlying integer | `enum Foo { VAL = N; ... }` declaration block + reference by name |
| nested user struct | nested `H5T_COMPOUND` | `message Foo { ... }` block (topologically ordered) |

---

## Class-level / file-level attributes

| Concern | `h5::` | `pb::` |
|---|---|---|
| Storage layout | `h5::chunk(N, M, ...)` (tier 2+ requires it) | ○ na — wire format, not stored |
| Compression | `h5::compress(gzip, 6)` / `h5::filter(...)` | ○ na — use `pb::write_gzip` at I/O time instead |
| Max dims (growable) | `h5::max_dims(unlimited)` | ○ na |
| Inline nested fields | `h5::inline_fields` | ○ na — proto3 always nests |
| Package / namespace | ○ na — HDF5 uses path hierarchy | `pb::package("com.acme.events")` |
| Reserved tags / names | ○ na | `pb::reserved(10, 11, "old_field")` |
| Syntax / edition selector | ○ na | `pb::target_syntax("proto3"\|"edition2023")` |
| Service / RPC stubs | ○ na — HDF5 is file storage | `pb::service("UserService")` → `service` block + `rpc` lines from `std::function<R(A)>` members |
| Binary descriptor companion | ○ na | `pb::descriptor_set_out("file.desc")` |
| Force-skip emission | (use `h5::ignore` at field scope only) | `pb::reject` — diagnostic + absent from both `.hpp` AND `.proto` |
| Enum zero-value synthesis | ○ na — HDF5 enums use the C++ value as-is | `pb::enum_zero("UNSPECIFIED")` — proto3 requires zero as first entry |

---

## Field-level wire / storage overrides

| Concern | `h5::` | `pb::` |
|---|---|---|
| Native type override | `h5::storage_type(H5T_NATIVE_FLOAT)` (precision trade) | `pb::wire("sint32"\|"sint64"\|"fixed32"\|"fixed64"\|"sfixed32"\|"sfixed64")` |
| String representation | `h5::fixed_string(N)` (vs VLEN — parallel-friendlier) | ○ na — `string` is always length-delimited |
| Packed encoding | ○ na — HDF5 is always packed | `pb::packed` (force `[packed = true]` field option) |
| Deprecated marker | ○ na | `pb::deprecated` (emits `[deprecated = true]`) |
| JSON-side alt name | ○ na | `pb::json_name("alternateJsonName")` |
| Custom user codec | `h5::serialize_with(fn)` / `h5::deserialize_with(fn)` | `pb::encode_with(fn)` / `pb::decode_with(fn)` ◇ cancelled — deferred to a follow-up cross-repo PR (sandbox `pb::custom_field<>` template needed first) |

---

## Tier classification + escape hatches

| Concept | `h5::` | `pb::` |
|---|---|---|
| Tier classification (1=POD / 2=VLEN / 3=spine+leaves / 4=opaque) | ✔ ok — the scatter/gather design's whole point | ○ na — proto3 has no tier model |
| Force tier classification | `h5::tier(N)` (escape hatch) | ○ na |
| Full buffer-serialize | `h5::serialize_full` — tier-4 opt-in | ○ na |
| Schema-evolution discipline | `h5::alias`, `h5::version`, `h5::upgrade_from(N, func)` | `pb::reserved`, `pb::alias`, `pb::version`, `pb::deprecated` |

---

## Parallel / runtime concerns

| Aspect | HDF5 | Protobuf |
|---|---|---|
| Tier-1 (POD) parallelism | ✔ ok — full MPI collective + independent I/O | ○ na — wire format is per-message |
| Tier-2 (VLEN-bearing) parallelism | ◇ cancelled — collective writes serialize internally; reads typically fail collective | ○ na |
| Tier-3+ parallelism | ✘ failed — effectively serial only | ○ na |
| Compression filters | ✔ ok — gzip, szip, custom plug-in | ○ na — use `pb::write_gzip` at I/O time instead |
| Streaming append | ✔ ok — via `h5::max_dims(unlimited)` + chunked dataset | ✔ ok — via `pb::append` framed-IO operator |
| RPC / service stubs | ✘ failed — HDF5 is file storage | ✔ ok — `pb::service` emits `service` block; downstream gRPC plumbing is separate |

---

## Implementation status — which features are wired end-to-end

| Backend feature | h5cpp / HDF5 | h5cpp / Protobuf |
|---|---|---|
| Tier 1 POD scalars + nested compounds | ✔ ok | ✔ ok |
| Tier 2 VLEN strings + vectors | ◇ cancelled — scatter/gather codegen target state, not shipped | ✔ ok (via pb.hpp's `repeated` / `string` runtime) |
| Tier 3 sparse / ragged + maps | ◇ cancelled — target state | ✔ ok (`std::map<K,V>` shipped on `3-pb-map`) |
| Tier 4 variant / opaque | ◇ cancelled — target state | ✔ ok (variant → oneof) |
| chrono adapters | ◇ cancelled — manual int64 | ✔ ok (Timestamp + Duration) |
| Universal `name` / `ignore` / `doc` | ✔ ok | ✔ ok |
| Universal `on_missing` | ✔ ok | ✔ ok — runtime backed by sandbox `pb::default_for_t` (Commit 3) |
| Universal `version` / `alias` / `name_all` | ✔ ok | ✔ ok |
| Schema artifact emission | ✔ ok — emitted into the binary HDF5 file at write time | ✔ ok — text `.proto` via `--proto-out` |
| `enum Foo { ... }` declaration emission | ✔ ok (via H5T_ENUM) | ✔ ok (Commit 1) |
| Reserved tags / names | ○ na | ✔ ok (Commit/Phase 4) |
| Wire/storage overrides | ✔ ok (`fixed_string`, `storage_type`) | ✔ ok (`wire`, `packed`, `deprecated`, `json_name`) |
| Service / RPC stubs | ○ na | ✔ ok (Commit 4) |
| Custom user codec | ✔ ok (`serialize_with` / `deserialize_with`) | ◇ cancelled — `encode_with` / `decode_with` deferred (needs sandbox `pb::custom_field<>`) |
| Compile-error class skip | ◇ cancelled — no `h5::reject` today (`h5::ignore` is field-scope only) | ✔ ok (`pb::reject`, Commit 2) |
| Tier-N forcing | ✔ ok (`h5::tier(N)`) | ○ na — pb has no tier model |
| Argument-range diagnostics | ◇ cancelled — taxonomy §11 open question | ◇ cancelled — same |

---

## The unified user-facing surface (when both backends are wanted)

The multi-backend architecture envisions a single annotated struct driving multiple emitters. Today the two backends use separate namespaces (`h5::*` vs `pb::*`) so a user needing both writes both:

```cpp
struct [[h5::name("events"), h5::chunk(1024), h5::compress(gzip, 6), h5::doc("user-level event captured by the gateway"),
        pb::name("UserEvent"), pb::package("com.acme.events"), pb::doc("user-level event captured by the gateway")]]
user_event_t {

    [[h5::name("ts"),h5::doc("nanoseconds since epoch"),
      pb::field(1), pb::doc("nanoseconds since epoch")]]
    std::uint64_t timestamp_ns;

    [[h5::name("user"),h5::on_missing(0),
      pb::field(2), pb::on_missing(0) ]]
    std::uint32_t user_id;

    [[h5::doc("payload samples"), h5::chunk(1024), h5::compress(gzip, 9),
      pb::field(3), pb::packed]]
    std::vector<double> values;

    [[h5::ignore,h5::doc("runtime cache; never persisted")
      pb::ignore ]]
    void* runtime_handle;
};
```

Then:

```
h5cpp -o user_event.h  --hdf5 user_event.cpp -- -std=c++23
h5cpp -o user_event_pb.hpp --proto-out user_event.proto --protocol-buffers user_event.cpp -- -std=c++23
```

Two AST walks, two artifacts (plus the `.proto` schema). The multi-backend architecture doc lays out the future where this becomes ONE invocation producing both — `h5cpp_compiler_generate(FORMATS hdf5 protocol-buffers ...)`. Tracking under "outstanding #10" in `tasks/pb-feature-coverage-and-gaps.md` §6.

A near-future cleanup would also unify the duplicated universal attributes — write `[[name("ts")]]` once and have both backends read it. The cookbook §"Where to next" tracks this.

---

## Reading guide — which backend to pick for what

| If your data ... | use ... |
|---|---|
| Lives on disk; large; needs random access; analytics workload | HDF5 |
| Crosses a wire (RPC, message bus, sensor uplink); needs schema discipline | Protobuf |
| Lives in both — disk archive AND wire transport | both backends; one annotated struct drives both artifacts |
| Has unbounded growth (streaming append) | HDF5 with `h5::max_dims(unlimited)` OR protobuf with `pb::append` framed-IO |
| Needs MPI / parallel reads & writes | HDF5 tier-1 only (POD layouts); avoid VLEN-bearing types if collective ops matter |
| Has tagged-union payloads (variant) | protobuf (cleaner `oneof` story) — HDF5 tier-4 requires `serialize_full` opt-in |
| Needs schema-evolution discipline (deprecation, reserved tags, version stamps) | protobuf — `pb::reserved`, `pb::deprecated`, `pb::version` |
| Has RPC service handlers (`std::function<R(A)>`) | protobuf — `pb::service` block emission |
| Has nested map-of-vector / sparse columns | HDF5 tier-3 (decomposed multi-dataset group) OR protobuf `map<K, V>` |

---

## References

- `tasks/h5cpp-compiler-multi-backend-architecture.md` — multi-backend roof, namespace layout (`h5::`, `h5::proto::`, `h5::json::`, `h5::sql::`, `h5::avro::`)
- `tasks/h5cpp-compiler-backend-cookbook.md` — the transferable six-step recipe (vocabulary → rewriter → callback → type map → CLI → fixtures+goldens)
- `tasks/h5cpp-compiler-scatter-gather-design.md` — HDF5 tier-1..4 model + universal attribute set
- `tasks/h5cpp-compiler-pb-attribute-taxonomy.md` — protobuf `pb::*` vocabulary, full tier 1..4 surface
- `tasks/pb-feature-coverage-and-gaps.md` — pb backend status + outstanding roadmap
- `vargalabs/h5cpp-compiler` — the binary; branches `30-pb-producer`, `31-pb-attribute-vocabulary`
- `vargaconsulting/h5cpp` — HDF5 runtime
- `vargalabs/sandbox` — `pb.hpp` runtime (branches `2-pb-operators` merged, `3-pb-map` pushed, `4-pb-default-for` pushed)
