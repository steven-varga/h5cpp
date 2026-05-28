@page reports_index Project Reports

Curated design notes, taxonomies, inventories, and surveys that document why h5cpp looks the way it does — and where it's going. These complement the auto-generated API reference (function signatures and class trees) with the prose that explains the design decisions behind them.

Each report lives under one category in the sidebar; cross-references to runnable code in the [Examples](examples.html) section appear at the end of relevant reports.

## Categories

- @subpage reports_architecture — the type-system, threading, async, and scatter/gather designs that underpin the I/O layer
- @subpage reports_inventories — exhaustive catalogues: handles, filters, alias vocabulary, STL pretty-print, HDF5 1.12 feature coverage
- @subpage reports_taxonomies — attribute models for the h5cpp-compiler's pluggable serialisation backends (Avro / BSON / CBOR / JSON / MsgPack / Protobuf / RLP / SQL + h5 native)
- @subpage reports_surveys — comparisons against h5py, Protobuf, and the prior art; backend cookbooks and difficulty rankings
- @subpage reports_compatibility — compatibility mechanisms, known HDF5 bugs, struct falsification, Doxygen alias support
- @subpage reports_project — product positioning, study summaries, usability evaluation

## Source-of-thought vs source-of-truth

The reports in this section are the **published** reference — what consumers of the docs site should know. The corresponding workspace-internal source-of-thought documents live in `vargalabs-workspace/tasks/h5cpp-*.md` and may carry more deliberation (rejected alternatives, open questions, ephemeral analysis). Treat the published version as the canonical statement; treat the workspace version as the history of how we got here.
