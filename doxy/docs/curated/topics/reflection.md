@page curated_topics_reflection REFLECTION

@brief Compiler-assisted reflection — h5cpp's strategy for turning
user-defined C++ types into HDF5 compound descriptors without
intrusive macros. C++26 horizon, today's h5cpp-compiler, and the
POD shortcut.

# Reflection in h5cpp

C++ doesn't (yet) ship reflection in its standard library. h5cpp
needs reflection to map user-defined struct types onto HDF5
`H5T_COMPOUND` descriptors automatically — without forcing users
to write boilerplate registration macros for every type.

H5CPP's answer is **two parallel paths to the same user-facing
surface**, organised by what the compiler in your hand can do:

| Path | Mechanism | C++ standard needed | What you write |
| :--- | :-------- | :----------------- | :------------- |
| **Native reflection** (header-only, future) | `std::meta::*` from P2996 + annotations from P3394 | C++26 | Just your struct. Optionally `[[=h5::name{"x"}]]` annotations. |
| **External tooling** (today) | h5cpp-compiler (Clang-based AST walker) | C++17 / 20 / 23 | Just your struct. Pre-build step emits the descriptor. |
| **POD macro** (today, no tooling) | `H5CPP_REGISTER_STRUCT(T)` macro at runtime registration time | C++17+ | Your struct + one macro call. |

The user-visible API (`h5::write(fd, "x", my_struct)`) is identical
across all three. Migrating from h5cpp-compiler to the C++26 path
when your compiler catches up requires changing no application code.

Top-down — from the most general future state, down to the
narrowest current shortcut:

## 1. C++26 state — native reflection

C++26 will (finally) ship language-level reflection. Two proposals
drive it:

- **P2996** — *Reflection for C++26*. Adds `std::meta::*`
  introspection: walk a type's non-static data members, query their
  types, names, offsets, all at constexpr time.
- **P3394** — *Annotations for reflection*. Lets users attach
  arbitrary values to declarations via `[[=annotation_value]]`,
  readable through `std::meta::annotations_of`.

Combined, these are enough for h5cpp to walk any user type at
constexpr time and emit the HDF5 compound descriptor with **no
external tool**:

```cpp
// C++26 — annotations attached to fields are h5cpp's customization surface
struct record {
    int                   id;
    [[=h5::name{"first"}]] std::string first_name;
    [[=h5::name{"last"}]]  std::string last_name;
    [[=h5::chunk{1024} | h5::gzip{8}]] std::vector<double> samples;
};

h5::write(fd, "/people", record_vec);   // unchanged user-facing call
```

At constexpr time, `h5::compound_type_for<record>()` walks
`std::meta::nonstatic_data_members_of(^^record)`, reads each
member's `std::meta::offset_of`, `std::meta::type_of`,
`std::meta::identifier_of`, and any attached
`std::meta::annotations_of`, then assembles the
`H5T_COMPOUND` descriptor as a `static const hid_t` per type
(lazy, one-shot).

**Key strategic point**: the annotation handles are the same value
types users already use at call sites. `h5::chunk{1024}` works the
same way attached to a field (C++26) as it does inside an
`h5::write(...)` call (today). One vocabulary, two usage sites —
the syntax envelope changes (`(...)` at call sites, `{...}` inside
`[[=...]]`), but the semantics don't.

**Timeline**: P2996 expected to land in C++26 (committee draft 2025,
ratification 2026). GCC 16.1+, Clang 21+, MSVC 2026 are the likely
first compilers with the feature complete. h5cpp is ready to add a
`h5cpp/reflection/` header layer the day the first major compiler
ships a complete implementation.

See @ref reports_compiler_multi_backend_architecture for the
broader strategy document, including the multi-backend rollout
plan that the C++26 reflection path will eventually feed.

## 2. Today — attribute list + non-POD types (h5cpp-compiler)

The same descriptor that C++26 reflection will eventually emit
inline, today's **h5cpp-compiler** emits via Clang Tooling at
pre-build time. The user-facing experience is identical — write
your struct, build, get HDF5 read/write support — but the type
walker runs in a separate process.

> **Project:** [`vargalabs/h5cpp-compiler`](https://github.com/vargalabs/h5cpp-compiler)
> — Clang LibTooling-based AST walker. Runs as a pre-build step,
> emits the descriptor + scatter/gather specialisations as a header
> the main h5cpp dispatch picks up automatically.

### What gets emitted

For a non-trivial type — one with `std::vector`, `std::string`,
nested compounds, `std::map<scalar, vector>`, etc. — the compiler
emits three pieces of generated code:

1. **A compound type descriptor** mapping each field to its HDF5 type
2. **A `gather<T>` specialisation** that walks the struct, packs
   variable-length fields into `hvl_t` relays, and hands the
   compound buffer to `H5Dwrite` — **zero-copy on write**
   (`hvl_t.p` points directly into `vector.data()`)
3. **A `scatter<T>` specialisation** that reverses the process
   after `H5Dread` — one copy on read (HDF5's VLEN allocator
   produces buffers; scatter `.assign()`s into the user vector,
   then `H5Treclaim`s)

### The user-facing surface — annotated struct

```cpp
// Your source — what you actually write:
//
// Class-level attributes carry storage defaults + schema metadata;
// field-level attributes drive per-column behaviour (renames,
// chunking, compression, missing-field policy, documentation).

[[h5::dataset("/records"), h5::version("2.1"), h5::doc("sensor sample stream")]]
struct record {

    [[h5::name("ID"), h5::index, h5::doc("monotonic packet id")]]
    int id;

    [[h5::name("display_name"), h5::on_missing("default")]]
    std::string name;

    [[h5::name("waveform"),
      h5::chunk(1024),
      h5::gzip(8) | h5::shuffle,
      h5::doc("raw sensor readings, microvolts, 1 kHz sample rate")]]
    std::vector<double> samples;

    [[h5::name("metadata_tags"),
      h5::on_missing("default"),
      h5::tag("schema_v2")]]
    std::map<std::string, std::vector<int>> tags;

    [[h5::ignore]]    // computed; not persisted
    double cached_rms;
};
```

### What h5cpp-compiler emits — generated header

```cpp
// === GENERATED — do not edit ===
// Source:   record.hpp:14
// Producer: h5cpp-compiler 1.4.2 (clang-tooling LibTooling)
// Backend:  h5 (HDF5 native)
// Tier:     2 (non-POD — scatter/gather)
//
// You don't write any of this — it's emitted at pre-build time and
// h5cpp's dispatch picks it up via the has_scatter<T> trait.

namespace h5 {

// (1) Tier-2 marker — opts `record` out of the contiguous POD path
//     and into the scatter/gather route.
template<> struct has_scatter<record> : std::true_type {};

// (2) Compound type descriptor — built once, cached as static.
template<> inline ::hid_t dt_t<record>::id() {
    static const ::hid_t tid = []{
        struct alignas(record) staging {
            int32_t            id;          // [[h5::name("ID")]]
            hvl_t              name;        // std::string → VLEN char
            hvl_t              samples;     // std::vector<double> → VLEN double
            hvl_t              tags;        // std::map<…,vector<int>> → VLEN of compound
        };

        ::hid_t tid = H5Tcreate(H5T_COMPOUND, sizeof(staging));
        H5Tinsert(tid, "ID",            offsetof(staging, id),      H5T_NATIVE_INT32);
        H5Tinsert(tid, "display_name",  offsetof(staging, name),    h5::vlen_string());
        H5Tinsert(tid, "waveform",      offsetof(staging, samples), h5::vlen_of<double>());
        H5Tinsert(tid, "metadata_tags", offsetof(staging, tags),    h5::vlen_of<kv_t>());
        return tid;
    }();
    return tid;
}

// (3) Gather — zero-copy on write.
//     hvl_t.p points DIRECTLY into the source vector / string, no
//     intermediate copy. HDF5 reads through these pointers in H5Dwrite.
template<> inline void gather<record>(const record& src, void* dst) {
    auto* s = static_cast<staging*>(dst);
    s->id            = src.id;
    s->name.len      = src.name.size();
    s->name.p        = const_cast<char*>(src.name.data());           // VLEN string relay
    s->samples.len   = src.samples.size();
    s->samples.p     = const_cast<double*>(src.samples.data());      // VLEN double relay
    s->tags.len      = src.tags.size();
    s->tags.p        = pack_map_to_kv_buffer(src.tags);              // map → compound buffer
    // h5::name("ID") + h5::index emit a side-band index registration too:
    h5::detail::register_index<record, &record::id>(src.id);
}

// (4) Scatter — one-copy on read.
//     HDF5's default VLEN allocator owns the staging buffers; we
//     assign() into the user's containers, then H5Treclaim.
template<> inline void scatter<record>(const void* src, record& dst) {
    const auto* s = static_cast<const staging*>(src);
    dst.id      = s->id;
    dst.name.assign(static_cast<const char*>(s->name.p), s->name.len);
    dst.samples.assign(
        static_cast<const double*>(s->samples.p),
        static_cast<const double*>(s->samples.p) + s->samples.len);
    unpack_kv_buffer_to_map(s->tags, dst.tags);
    // [[h5::ignore]] field: dst.cached_rms intentionally not populated
    // [[h5::on_missing("default")]] kicks in if the on-disk record
    // lacks "display_name" or "metadata_tags" — uses T{} default.
}

// (5) Chunk/filter pipeline registration — driven by [[h5::chunk]] +
//     [[h5::gzip]] on the `samples` field. Applied at create time
//     to the column's storage plan.
template<> inline dcpl_t default_dcpl_for<record>() {
    return h5::chunk{1024} | h5::gzip{8} | h5::shuffle;
}

} // namespace h5
```

### At the call site — unchanged

```cpp
std::vector<record> records = load_data();

// Same one-liner as the POD path. The dispatch sees has_scatter<record>
// and routes through the generated gather/scatter; the dcpl from
// default_dcpl_for<record>() applies the field-level chunk + filter spec.
h5::write(fd, "/records", records);
auto round = h5::read<std::vector<record>>(fd, "/records");
```

### What each attribute drove

| Attribute on `record` / its fields | Emitted into                                             |
| :-------------------------------- | :------------------------------------------------------- |
| `[[h5::dataset("/records")]]`     | Default storage path baked into the call-site dispatch   |
| `[[h5::version("2.1")]]`          | `@version` attribute on the dataset; consumed by on-read schema-migration logic |
| `[[h5::doc(...)]]` (class)        | Dataset-level description attribute                      |
| `[[h5::name("ID")]]`              | HDF5 field name `"ID"` instead of `"id"` in the compound |
| `[[h5::index]]`                   | Side-band index registration in `gather`                 |
| `[[h5::doc(...)]]` (field)        | Per-field description attribute                          |
| `[[h5::chunk(1024)]]`             | `default_dcpl_for<record>()` chunk shape                 |
| `[[h5::gzip(8)]] \| h5::shuffle`  | Filter pipeline composed into the dcpl                   |
| `[[h5::name("display_name")]]`    | HDF5 field name override                                 |
| `[[h5::on_missing("default")]]`   | `scatter` falls back to `T{}` when the on-disk record lacks this field — backward-compat with v1 readers |
| `[[h5::tag("schema_v2")]]`        | Schema migration tag — drives the multi-backend producer to emit equivalent versioning in Protobuf / JSON Schema / SQL |
| `[[h5::ignore]]`                  | Field omitted from the staging compound; not persisted   |

### Attribute list — customisation surface

h5cpp-compiler reads C++ attributes attached to fields to drive
per-field behaviour. The same vocabulary that becomes annotations
under C++26:

| Attribute                           | Effect                                                   |
| :---------------------------------- | :------------------------------------------------------- |
| `[[h5::name("x")]]`                 | Override the on-disk field name                          |
| `[[h5::ignore]]`                    | Skip this field — don't include in the compound          |
| `[[h5::chunk(1024)]]`               | Set chunk shape (per-dataset, applied to vector fields)  |
| `[[h5::gzip(8)]]`                   | Compress this field                                      |
| `[[h5::on_missing("default")]]`     | Behaviour when an older file lacks this field            |
| `[[h5::tag("schema_v2")]]`          | Per-field schema tag for migrations                      |
| `[[h5::doc("...")]]`                | Attach a documentation string visible to other backends  |

### Multi-backend bonus

h5cpp-compiler walks the type **once** but can emit artefacts for
multiple backends in the same pass: HDF5 compound descriptor,
Protobuf `.proto`, JSON Schema, SQL DDL, Avro. Same struct, many
on-disk and over-the-wire forms — one source of truth. See
@ref reports_compiler_multi_backend_architecture for the design.

### Zero-copy guarantee — small print

Zero-copy is guaranteed for the **write side** of any tier. On
the **read side**:

| Type tier            | Read-side zero-copy?  | Why                                                |
| :------------------- | :-------------------- | :------------------------------------------------- |
| POD (contiguous)     | ✔                     | `H5Dread` writes into the destination directly     |
| Non-POD with VLEN    | ✘ (one copy)          | HDF5's VLEN allocator owns the buffer; scatter assigns to user vector then calls `H5Treclaim` |

The read-side copy on tier-2 types can be eliminated later via
`H5Pset_vlen_mem_manager`, but that's follow-up work. **Don't
promise zero-copy reads for non-POD types** — promise zero-copy
writes and one-copy reads.

## 3. Today — POD shortcut (`H5CPP_REGISTER_STRUCT`)

For pure POD structs — trivially-copyable, standard-layout, no
virtuals, no private members beyond plain data — neither C++26
reflection nor the external compiler is strictly necessary. The
in-memory layout already equals the on-disk layout, so a single
runtime registration macro is enough:

```cpp
struct sample {       // POD: arithmetic fields only
    int      ts;
    double   value;
    uint32_t flags;
};

H5CPP_REGISTER_STRUCT(sample);   // one line, sets up the compound type

std::vector<sample> data = collect();
h5::write(fd, "/samples", data);                              // zero-copy
auto round = h5::read<std::vector<sample>>(fd, "/samples");   // also zero-copy
```

The macro:
1. Specialises `h5::dt_t<sample>` to build an `H5T_COMPOUND` whose
   field offsets come from `offsetof(sample, field)` and whose
   field types come from `dt_t<decltype(field)>`
2. Records the type in a static registry so re-registration is a
   no-op
3. Costs zero at the call site — `h5::write(fd, "/x", samples)`
   uses the registered compound exactly as it would for any
   built-in type

### What "POD enough" means

The macro works on any type satisfying:

| Constraint                | Check                                                  |
| :------------------------ | :----------------------------------------------------- |
| Trivially copyable        | `std::is_trivially_copyable_v<T>`                      |
| Standard layout           | `std::is_standard_layout_v<T>`                         |
| No virtuals               | (subset of standard-layout)                            |
| No `std::vector` / `std::string` / smart pointers in fields | Those require scatter/gather (tier 2) |
| All fields recognisable to `dt_t<F>` | arithmetic, enum, nested registered struct, `std::array<T,N>`, `std::complex<T>` |

If any of these are violated, switch to the C++26 path or the
external h5cpp-compiler. The `static_assert` inside
`H5CPP_REGISTER_STRUCT` will tell you which constraint you missed.

## Choosing the right path

| You have… | Use… |
| :------- | :--- |
| C++26 compiler (when shipped) | C++26 reflection (header-only, zero external tools) |
| C++17/20/23 + a clang toolchain available | h5cpp-compiler (full reflection, all type tiers) |
| C++17/20/23 + only POD types to persist | `H5CPP_REGISTER_STRUCT` (no external tools, one macro per type) |
| Already on h5cpp-compiler, migrating to C++26 | No application code change — same vocabulary, syntax envelope changes |

## Where to go next

- [vargalabs/h5cpp-compiler](https://github.com/vargalabs/h5cpp-compiler)
  — the Clang LibTooling-based reflector project (source, build
  instructions, issue tracker)
- @ref reports_compiler_multi_backend_architecture — one AST
  walker, many output producers (HDF5 + Protobuf + JSON Schema +
  SQL DDL + Avro + CBOR + BSON + MessagePack + RLP)
- @ref reports_compiler_h5_attribute_taxonomy — the `h5::*`
  attribute vocabulary the HDF5 backend consumes
- @ref reports_compiler_prior_art_survey — landscape survey of
  reflection / serialisation tooling h5cpp-compiler positions against
- @ref curated_topics_typesystem — the storage-representation
  taxonomy reflection feeds into
- @ref curated_topics_stl — the trait machinery that recognises
  non-reflected types (Walter Brown feature detection)
- @ref example_guide_reflection — runnable cookbook example
