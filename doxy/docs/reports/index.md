@page reports_index Project Reports

Curated design notes, taxonomies, inventories, and surveys that document
why h5cpp looks the way it does — and where it's going. These complement
the auto-generated API reference (function signatures and class trees)
with the prose that explains the design decisions behind them.

## Architecture & Design

The "why does this layer exist and how was it built" reports.

- @subpage reports_type_system_map — the live type-system map (every C++ source type → HDF5 type → storage mechanism)
- @subpage reports_multithreading_pipeline_state — current state of the multithreaded filter pipeline in v1.12.6 (`h5::high_throughput` DAPL + `h5::append` packet-table paths)
- @subpage reports_compiler_multi_backend_architecture — h5cpp-compiler's plug-in serialisation backend framework (11 backends: HDF5 + Protobuf + JSON + MsgPack + CBOR + BSON + Avro + RLP + SQL ×3 dialects)

## Inventories & Guides

Exhaustive catalogues of what's available and how to use it.

- @subpage reports_handle_inventory — every RAII handle wrapper (16 property lists + 5 async-mode variants + the 6 object handles), pretty-print status
- @subpage reports_filters_inventory — gzip / shuffle / fletcher32 / nbit / scale-offset / SZIP / Gorilla / custom filters, with use-case guidance
- @subpage reports_stl_pretty_print_guide — `h5::cout` / `h5::pprint` formatting recipes for the supported container family

## h5cpp-compiler Attribute Taxonomies

One taxonomy per backend — exhaustive attribute vocabulary that the
h5cpp-compiler's AST walker parses to produce each artifact.

- @subpage reports_compiler_h5_attribute_taxonomy — `[[h5::*]]` (the HDF5 / cross-backend universals)
- @subpage reports_compiler_pb_attribute_taxonomy — `[[pb::*]]` (Protocol Buffers — fields, services, RPC, wire knobs)
- @subpage reports_compiler_json_attribute_taxonomy — `[[json::*]]` (JSON Schema — formats, validation, naming)
- @subpage reports_compiler_avro_attribute_taxonomy — `[[avro::*]]` (Apache Avro — logical types, aliases, defaults)
- @subpage reports_compiler_bson_attribute_taxonomy — `[[bson::*]]` (BSON — binary, datetime, decimal, timestamp)
- @subpage reports_compiler_cbor_attribute_taxonomy — `[[cbor::*]]` (CBOR — RFC 8949 semantic tags)
- @subpage reports_compiler_msgpack_attribute_taxonomy — `[[msgpack::*]]` (MessagePack — extension types)
- @subpage reports_compiler_rlp_attribute_taxonomy — `[[rlp::*]]` (Ethereum RLP — minimal annotation set)
- @subpage reports_compiler_sql_attribute_taxonomy — `[[sql::*]]` (SQL DDL — column types, constraints, dialects)

## Surveys & Comparisons

- @subpage reports_compiler_prior_art_survey — competitive landscape: rootcling, serde, nlohmann/json codegen, etc.
- @subpage reports_study_hdf5_bugs_top10 — top 10 most frequent HDF5-related bugs h5cpp's design avoids by construction

## Project Assessment

- @subpage reports_usability_evaluation — v1.12.6 per-category feature scorecard with rationale (✔ / ◇ / ✘ status across 13 dimensions)

---

## Source-of-thought vs source-of-truth

The reports in this section are the **published** reference — what
consumers of the docs site should know. The corresponding workspace-
internal source-of-thought documents live in
`vargalabs-workspace/tasks/h5cpp-*.md` and may carry more deliberation
(rejected alternatives, open questions, ephemeral analysis). Treat the
published version as the canonical statement; treat the workspace
version as the history of how we got here.
