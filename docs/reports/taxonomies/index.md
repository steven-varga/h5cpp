@page reports_taxonomies Taxonomies — compiler backends

The h5cpp-compiler emits scatter/gather specialisations from C++ source annotated with `[[h5::*]]` attributes. Each serialisation backend (Avro, BSON, CBOR, JSON, MsgPack, Protobuf, RLP, SQL, and h5's own attribute model) has its own taxonomy of which attributes it understands and how each maps to an on-disk representation.

These documents are the per-backend taxonomies. Together they define the contract between annotated C++ types and the backend-specific output.

## Members

- @subpage reports_compiler_h5_attribute_taxonomy — h5's native attribute model
- @subpage reports_compiler_avro_attribute_taxonomy — Apache Avro
- @subpage reports_compiler_bson_attribute_taxonomy — BSON (MongoDB)
- @subpage reports_compiler_cbor_attribute_taxonomy — RFC 8949 CBOR
- @subpage reports_compiler_json_attribute_taxonomy — JSON
- @subpage reports_compiler_msgpack_attribute_taxonomy — MessagePack
- @subpage reports_compiler_pb_attribute_taxonomy — Protocol Buffers
- @subpage reports_compiler_rlp_attribute_taxonomy — Ethereum RLP
- @subpage reports_compiler_sql_attribute_taxonomy — SQL
