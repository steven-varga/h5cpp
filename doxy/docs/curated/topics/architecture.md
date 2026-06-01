@page curated_topics_architecture ARCHITECTURE

@brief Design notes — type system, scatter/gather, async-mode
machinery, compiler-emitted reflection, performance evaluation
framework, multi-backend architecture.

The `docs/reports/architecture/` tree carries 12+ design documents
covering the internal mechanisms behind h5cpp. They explain the
**why** behind the API surface — useful for contributors, advanced
users tuning for performance, and anyone integrating h5cpp into a
larger system.

→ @ref reports_index "Project reports — full index"

Highlights:
- **Type system** — how `storage_representation_v<T>` is computed at
  compile time, the access-traits dispatch matrix
- **Scatter / gather** — compiler-emitted decomposition for deeply
  nested compounds
- **Async mode** — type-level discrimination of serial vs async
  descriptors, FAPL-scoped executor
- **Reflection** — h5cpp-compiler clang tooling that emits per-type
  HDF5 compound descriptors
- **Performance framework** — methodology + benchmarks

This stub page exists to anchor the **ARCHITECTURE** entry in the
TOPICS nav. The real content lives in the linked reports tree.
