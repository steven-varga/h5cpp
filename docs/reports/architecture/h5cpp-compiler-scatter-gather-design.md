@page reports_compiler_scatter_gather_design h5cpp-compiler Scatter/Gather Design

**Date:** 2026-05-21
**Authors:** Steven Varga, Winston (Architecture)
**Status:** Design approved; implementation pending
**Repo:** vargaconsulting/h5cpp-compiler (branch: staging)

## Decision

h5cpp-compiler will extend its AST matcher to "color" types into tiers and emit different template specializations per tier. The h5cpp library dispatches between them via a `has_scatter<T>` trait at compile-time. User-facing API (`h5::write`, `h5::read`) remains unchanged.

## Why

The current compiler handles only PODs — emitting `register_struct<T>()` specializations. Non-trivial types with `std::vector`, `std::map<scalar,vector>`, etc., need scatter-gather I/O to avoid the worst-case "serialize into a temp buffer, then write" path.

The matcher already labels nodes (`cxxRecordDecl(isStruct()).bind(...)`); adding a second label for non-POD types is mechanical, not a rewrite. It reuses the existing reflection sandwich (core + compiler-generated shim + io) without introducing a new abstraction layer.

This is the moat against Boost.Serialization / Cereal / nlohmann::json — all of which require intrusive macros per type. AST-walked codegen produces equivalents from unmodified C++ source.

## Zero-Copy Guarantee Scope (load-bearing)

Zero-copy is **only guaranteed for contiguous types (tier 1)** — pure PODs where in-memory layout equals on-disk layout, so the library can `H5Dwrite` the bytes directly.

For tier 2+ (vector-bearing):

- **Write:** zero-copy. `hvl_t.p` points directly into `vector.data()`; HDF5 reads from there in `H5Dwrite`.
- **Read:** one copy. HDF5's default VLEN allocator allocates buffers during `H5Dread`; the generated `gather` copies into the user's vector via `assign()`, then calls `H5Treclaim`.

The read-side copy can be eliminated later via `H5Pset_vlen_mem_manager`, but that is a follow-up. **Do not promise zero-copy reads for non-contiguous types in user-facing docs.**

## Matcher Tiers — Overview

**Tier is a property of user-defined struct/class types** — the C++ classes the AST matcher visits and the compiler emits codegen for. It is *not* a property of library containers (`std::vector`, `std::map`, `std::string`), nor a property of call sites or top-level invocations.

A library container appearing as a **field** of a user class determines that class's tier (e.g. `struct S { std::vector<T> field; }` is a tier-2 struct because `std::vector<T>` is single-indirection contiguous). A library container at the **top level** of `h5::write(fd, "ds", v)` is handled by the existing library template, not by compiler-generated codegen — what matters there is the element type's tier.

The tier classification is monotone: each tier strictly subsumes the previous, with one new building-block shape that the previous tier could not represent. A class's tier is the max tier of any of its fields, transitively.

| Tier | Pattern | HDF5 mapping | I/O cost | MPI | Compiler emits |
|---|---|---|---|---|---|
| 1 — POD *(today)* | All fields are `H5T_NATIVE_*`, fixed arrays, or nested PODs | `H5T_COMPOUND` of natives + `H5Tarray_create` for fixed-rank fields. Contiguous or chunked. Filters available. | 0 copies | ✔ Full collective + independent | `H5CPP_REGISTER_STRUCT(T)` + `register_struct<T>()` |
| 2 — Scatter *(new)* | POD with `std::vector<T>` / `std::string` / single-indirection contiguous fields | `H5T_COMPOUND` with `H5Tvlen_create` members + variable-length strings. **Chunked required.** VLEN payloads on the per-file global heap. | 0 copies write, 1 copy read | ◇ Degraded (collective VLEN writes serialize internally; reads typically fail collective) | `scatter<T>` + `gather<T>` + `H5CPP_REGISTER_SCATTER(T)` |
| 3 — Tree walk | `std::map<scalar,vector>`, `vector<string>`, ragged / multi-level structures | Nested VLEN compound OR decomposed multi-dataset (keys / offsets / values group). Chunked. Heavy global heap. | 1 copy spine, 0/1 copies leaves | ✘ Effectively serial only | Same as tier 2; multi-pass internal traversal + spine buffer |
| 4 — Opaque | `std::variant`, polymorphism, `unordered_map`, `list`, cycles | `H5T_OPAQUE` for payload + sibling tag dataset, OR union-style `H5T_COMPOUND` with discriminant. Loses HDF5-native introspection. | Full copy; user opts in via `[[h5::serialize_full]]` | ✘ Per-element runtime dispatch incompatible with collective ops | Same specializations; internally buffer-serialize |

## Per-Tier Specification

Each tier section explains *what makes it harder than the previous one*, *which HDF5 features map to it*, *whether parallel HDF5 works*, and *which C++ type families fall into it*.

### Tier 1 — Contiguous PODs (zero-copy guaranteed)

**The problem.** None — this is the baseline. POD types map 1:1 to HDF5 compound types: in-memory layout equals on-disk layout, so the library `H5Dwrite`s the bytes directly. Any constraints come from HDF5's type system itself (no wide chars, no bit fields, native byte order assumed unless explicitly converted), not from the C++ side.

**HDF5 mapping.** `H5T_COMPOUND` constructed with `H5Tcreate`. Each scalar field becomes one `H5Tinsert(t, name, offset, H5T_NATIVE_*)` call. Fixed arrays become `H5Tarray_create(base, rank, dims)`. Nested PODs become nested `H5T_COMPOUND` types inserted recursively. Storage: contiguous (default) or chunked (`h5::chunk{...}`). Filters — gzip, szip, custom — require chunked storage and apply directly to the data bytes.

**MPI.** ✔ **Full collective + independent I/O.** Compound types with fixed-size members are parallel-safe; aggregate writes scale to thousands of ranks. This is the native HPC parallel-write case — simulators dump state, training runs append batches, ranks coordinate via collective hyperslab writes.

**Type families.**

| Family | Examples |
|---|---|
| Arithmetic | `int`, `uint64_t`, `float`, `double`, `long double`, `bool` |
| Fixed-width integers | `int8_t … int64_t`, `uint8_t … uint64_t` |
| C arrays | `T[N]` where T is tier 1 |
| `std::array<T,N>` | T tier 1, N fixed |
| `std::complex<float/double>` | guaranteed layout-compatible with `T[2]` |
| `std::pair<A,B>` | A and B both standard-layout, no padding holes |
| Enums (scoped + unscoped) | persisted as underlying type |
| `std::bitset<N>` | implementation-defined byte order, stable per platform |
| Nested PODs | recursively tier 1 |

### Tier 2 — Single indirect contiguous (zero-copy write, one-copy read)

**The problem.** The user's type has heap indirection: `std::vector.data()` points elsewhere, `std::string.data()` points elsewhere. The bytes you want on disk are *not* a contiguous run starting at `&obj`; they're scattered across the obj's body and the heap allocations its fields reference. Writing them naively requires either (a) gathering into a contiguous staging buffer — one copy plus one allocation per write, bad — or (b) describing the scatter pattern to HDF5 so it reads from the heap directly during `H5Dwrite` (zero-copy write). h5cpp-compiler chooses (b).

**HDF5 mapping.** Variable-length types: `H5Tvlen_create(base)` for vectors, `H5Tcopy(H5T_C_S1)` + `H5Tset_size(t, H5T_VARIABLE)` for strings. The compound row becomes a struct of `hvl_t { size_t len; void* p; }` descriptors (one per indirect field) plus the inline scalars. At write time, the generated `scatter<T>` populates each `hvl_t.p` with the field's `vector.data()` — HDF5 reads from there directly in `H5Dwrite`. **Storage must be chunked**: HDF5's contiguous layout cannot hold variable-length data. The variable payloads themselves go to a per-file **global heap**; the chunked dataset only stores the `hvl_t` descriptor pairs and the inline scalars. Filters apply to the descriptor chunks, not to the global heap payloads. At read time, HDF5's default VLEN allocator allocates new buffers for the variable data; the generated `gather<T>` copies into the user's vector/string via `assign()`, then `H5Treclaim`s the HDF5 allocations.

**MPI.** ◇ **Degraded.** HDF5 ≤ 1.12: collective VLEN writes are serialized internally (defeats parallel scaling); collective reads typically fail. HDF5 1.14+: limited collective VLEN support, but the global heap is a shared-resource bottleneck across ranks. Practical advice: use independent I/O for VLEN-bearing datasets; if collective ops are required, drop those fields to per-rank single-file output and merge offline.

**Type families.**

| Family | Examples |
|---|---|
| Owning vectors | `std::vector<T>` (T tier 1) |
| Owning strings | `std::string`, `std::wstring`, `std::u8/16/32string`, `std::basic_string<CharT>` |
| Owning numerics | `std::valarray<T>` |
| Single-element heap | `std::unique_ptr<T>` (T tier 1, single element) |
| Linear algebra | `Eigen::Vector`, `Eigen::Matrix<T,R,C>` |
| Numerical arrays | `xt::xarray<T>`, `xt::xtensor<T,N>`, `xt::xfixed<T,Shape>` |
| Multi-dim views | `std::mdspan<T,Extents,layout_right,…>` (dense layouts only) |
| Non-owning views *(write-only)* | `std::span<T>`, `std::string_view` |
| Boost contiguous | `boost::container::vector`, `static_vector`, `small_vector`, `flat_map`, `flat_set` |
| Custom contiguous | any user type with `data()` + `size()` via adapter trait |

### Tier 3 — Spine + leaves (mixed zero-copy)

**The problem.** Multi-level indirection. The user's type has fields like `std::map<K, std::vector<V>>` — the outer map's nodes are scattered red-black tree allocations (not contiguous), the inner vectors are heap-allocated contiguous runs. Or `std::vector<std::string>` — outer vector is one heap block of string headers, each header points to its own character buffer elsewhere. There's no single pointer chase to follow. The scatter pattern becomes two-level: walk the spine (map nodes, vector indices) to collect (key, length) pairs into a small staging buffer, then point HDF5 at each leaf's data.

**HDF5 mapping.** Two viable approaches:
- **(a) Nested VLEN compound.** `H5Tvlen_create(H5Tvlen_create(base))` for `vector<vector<T>>`. Supported by HDF5 but each level adds a global-heap roundtrip; performance degrades quickly with depth.
- **(b) Decomposed multi-dataset group.** Write the user's structure as a *group* of datasets: `/path/keys` (1-D `H5T_NATIVE_*` array), `/path/offsets` (1-D `H5T_NATIVE_HSIZE` array marking value-vector boundaries), `/path/values` (1-D compound array of all leaf elements concatenated). Recombination on read uses the offsets to slice. More HDF5-native and parallel-friendlier, but loses the "one dataset per object" mental model. Storage: chunked.

The generated `scatter`/`gather` pair picks (a) by default; the user can opt into (b) via a `[[h5::decomposed]]` annotation on the field. Heavy global-heap traffic in (a) is the main runtime cost.

**MPI.** ✘ **Effectively serial only.** Nested VLEN compounds tier-2's parallel limitations across levels; the decomposed multi-dataset approach can work with collective ops per dataset but loses cross-rank atomicity (writing keys + offsets + values is no longer one transaction). If parallel scaling matters, restructure the user type to tier 1 (denormalize) or accept serial-driver output.

**Type families.**

| Pattern | Examples |
|---|---|
| Ragged 2D arrays | `std::vector<std::vector<T>>`, `std::vector<std::string>` |
| Sparse columns | `std::map<scalar, std::vector<T>>`, `std::map<scalar, scalar>` |
| Single discriminated | `std::optional<T>` where T is tier 1 or 2 |
| Ordered sets | `std::set<T>`, `std::multiset<T>` (sorted then serialized) |
| Segmented arrays | `std::deque<T>` (chunks copied to flat region) |
| Pair-bearing maps | `std::map<K,V>` with scalar K, V |
| Sparse matrices | `Eigen::SparseMatrix` (decomposes to value/row/col vectors) |
| Nested PODs containing tier 2/3 fields | recursive scatter |

### Tier 4 — Opaque / full-serialize (zero-copy lost)

**The problem.** The on-disk shape can't be determined at compile time. A `std::variant<A, B, C>` can be any of three alternatives at any element; deciding which to write per element requires runtime dispatch on `.index()`. `std::unordered_map` iterates in implementation-defined order — not stable across runs, defeats reproducibility. Polymorphism through `Base*` with virtuals requires a type registry (which derived classes exist?) and per-element type tags. Cyclic graphs need visited-ID tables to prevent infinite recursion. None of these compose with HDF5's "one type per dataset" model directly; all require either runtime tagging or full opaque serialization.

**HDF5 mapping.** Two options, both lossy in different ways:
- **(a) `H5T_OPAQUE` + sibling tag.** The generated `scatter` serializes the variant payload into a `H5T_OPAQUE` byte blob (`H5Tcreate(H5T_OPAQUE, n)`) and writes a separate `/path/tags` 1-D integer dataset alongside, recording which alternative is active per element. Compact but loses HDF5-native introspection — `h5dump` shows opaque bytes for the payload.
- **(b) Union-style `H5T_COMPOUND`.** HDF5 allows overlapping field offsets in a compound. Insert all variant alternatives at the same offset, plus a discriminant integer. Keeps introspection but wastes space (all alternatives must fit; payload is sized to the largest). Useful when the variant alternatives are small and similar; pathological when they're not.

The generated `scatter`/`gather` defaults to (a); the user opts into (b) via `[[h5::union_compound]]`. Both require the `[[h5::serialize_full]]` opt-in on the variant-bearing field — explicit acknowledgement of the zero-copy loss.

**MPI.** ✘ **Not supported.** Per-element runtime dispatch breaks the collective-op model (every rank must agree on the write pattern in advance). Workaround: per-rank serial writes to per-rank files, merge offline with `h5merge`. Defeats the purpose of having parallel I/O at all.

**Type families.**

| Pattern | Examples |
|---|---|
| Hash tables (unstable order) | `std::unordered_map`, `std::unordered_set`, `std::unordered_multimap` |
| Linked lists | `std::list<T>`, `std::forward_list<T>` |
| Sum types | `std::variant<A,B,…>` (runtime tag + per-alternative dispatch) |
| Polymorphism | `Base*` with virtuals, `std::shared_ptr<Base>`, `std::any` (need type registry) |
| Pointer-rich custom containers | boost::graph, intrusive containers, raw `T*` members |
| Self-referential / cyclic | trees with parent pointers, DAGs, graphs (need visited-ID table) |

### Out of scope (compiler refuses unless `[[h5::ignore]]`)

| Family | Why |
|---|---|
| Resource handles | `std::fstream`, file descriptors, sockets, `std::thread`, `std::mutex`, `std::atomic<T>` |
| Function-shaped | `std::function`, lambdas with captures, function pointers, member function pointers |
| Implementation details | vtable pointers (auto-excluded), padding bytes |
| Allocator state | custom allocators bound to live process state |
| Local-clock time points | `std::chrono::steady_clock::time_point` (no stable epoch; use `system_clock` or store nanoseconds explicitly) |

## Adding Support for New Types

The matcher isn't hardwired to specific class names. Adding a new container is a single adapter trait on the compiler side; no library change needed:

```cpp
// Tells the AST walker how to extract a contiguous region from this type.
template <typename T> struct sg_adapter<MyContainer<T>> {
    static constexpr auto data(const MyContainer<T>& c) { return c.raw_buffer(); }
    static constexpr auto size(const MyContainer<T>& c) { return c.element_count(); }
    static constexpr auto resize(MyContainer<T>& c, std::size_t n) { c.allocate(n); }
};
```

This is how `std::vector`, `std::string`, `Eigen::Matrix`, `xt::xarray`, and Boost containers all become tier-2 support — same matcher, different adapter, same generated code shape. Third-party type support ships as a header-only adapter pack.

## User-Facing Attribute System

The attributes are user-facing knobs that drive per-type emission. All live under the `h5::` attribute namespace and use the standard C++17 attribute syntax today (`[[h5::name(...)]]`), with a clean migration path to C++26 reflection annotations (see "C++26 reflection migration" below).

**Naming rule (load-bearing).** Across both syntaxes, user code uses **bare names** — `h5::chunk`, `h5::name`, `h5::ignore`, `h5::compress`. The `_t` suffix appears only when a context genuinely names a type *as a type* (template parameters, traits, `decltype`). Value construction reads the same in both eras:

- Call site (today): `h5::write(fd, "ds", obj, h5::chunk{1024} | h5::gzip{8});`
- Annotation (C++26): `[[=h5::chunk{1024}]] std::vector<double> samples;`
- Type reference (rare): `template <class T> auto build() -> h5::chunk_t { … }`

**Why `h5::` and not a separate namespace.** Several attribute types in this list — `h5::chunk`, `h5::max_dims`, `h5::offset`, `h5::stride`, `h5::block`, `h5::count`, plus the filter/compression family — **already exist** in h5cpp's runtime API as property objects users construct at call sites. Under C++26 reflection annotations, the *same constructors* become attachable to declarations using the *same syntax*. One vocabulary, two usage sites. Newly-introduced annotation handles (`h5::name`, `h5::ignore`, `h5::doc`, `h5::tag`, `h5::on_missing`, `h5::serialize_full`, `h5::alias`, etc.) have been verified collision-free against the current h5cpp namespace as of 2026-05-21. (`h5::default` would collide with the C++ keyword `default`; we use `h5::on_missing` instead.)

### Tier 1 — Must-have

| Attribute | Purpose | Example |
|---|---|---|
| `[[h5::name("on_disk_name")]]` | Rename a field/struct for on-disk storage; decouples C++ identifier from HDF5 dataset/field name | `[[h5::name("temp_K")]] double temperature;` |
| `[[h5::ignore]]` | Skip this field entirely (write and read) | `[[h5::ignore]] int debug_counter;` |
| `[[h5::on_missing(value)]]` | Fill this value if field is missing on read (avoids the `default` keyword) | `[[h5::on_missing(0.0)]] double new_field;` |
| `[[h5::chunk(N, M, ...)]]` | Chunked layout with given dimensions; required for variable-length data | `[[h5::chunk(1024)]] std::vector<sample_t> samples;` |
| `[[h5::compress(gzip, 6)]]` | Apply filter to chunked storage | `[[h5::compress(gzip, 9)]] std::vector<double> values;` |
| `[[h5::serialize_full]]` | Opt in to buffer-serialize (the tier-4 acknowledgement) | `[[h5::serialize_full]] std::variant<a,b,c> payload;` |

### Tier 2 — High value, low cost

| Attribute | Purpose | Example |
|---|---|---|
| `[[h5::tag("field_name")]]` | Name the discriminant field for variant/tagged payloads | `[[h5::tag("kind")]] std::variant<a,b,c> payload;` |
| `[[h5::doc("description")]]` | Attach HDF5 metadata (an actual HDF5 attribute on the dataset) for self-documenting files | `[[h5::doc("body temperature in Celsius")]] float temperature;` |
| `[[h5::inline_fields]]` | Flatten a nested struct's fields into the parent compound | `[[h5::inline_fields]] base_record_t base;` |
| `[[h5::name_all("snake_case")]]` | Struct-level naming convention applied to all fields (overridable per-field) | `struct [[h5::name_all("snake_case")]] my_event_t { ... };` |
| `[[h5::alias("old_name")]]` | Accept a legacy name on read; for schema evolution | `[[h5::name("temp"), h5::alias("temperature_c")]] float temp;` |
| `[[h5::max_dims(unlimited)]]` | Mark dataset as growable (`H5S_UNLIMITED`) for streaming appends | `[[h5::max_dims(unlimited), h5::chunk(64)]] std::vector<event_t> log;` |

### Tier 3 — Nice to have

| Attribute | Purpose |
|---|---|
| `[[h5::storage_type(H5T_NATIVE_FLOAT)]]` | Override the on-disk native type (precision trade) |
| `[[h5::fixed_string(N)]]` | Store as fixed-length string rather than VLEN (parallel-friendlier) |
| `[[h5::version(N)]]` | Schema version for upgrade hooks |
| `[[h5::upgrade_from(N, func)]]` | Read-time conversion from older schema version |

### Tier 4 — Specialized (defer)

| Attribute | Purpose |
|---|---|
| `[[h5::serialize_with(funcptr)]]` / `[[h5::deserialize_with(funcptr)]]` | Custom user-provided I/O functions per field |
| `[[h5::tier(N)]]` | Force tier classification (escape hatch — let the compiler decide normally) |
| `[[h5::reject]]` | Compile-error if anyone tries to serialize this |

### Class-level vs field-level scoping

Standard pattern: class-level attributes set defaults; field-level overrides them.

```cpp
struct [[h5::name_all("snake_case"),
        h5::chunk(1024),
        h5::compress(gzip, 6)]] log_entry_t {
    uint64_t timestamp;
    std::string message;
    [[h5::compress(gzip, 9)]] std::vector<double> samples;  // overrides default
    [[h5::ignore]]            int internal_state;
};
```

### Annotation + call-site composition (macro-controlled)

When a field carries layout annotations *and* the user also passes call-site properties to `h5::write(...)`, there are two possible semantics. The choice is controlled by a preprocessor macro, mirroring the existing `H5CPP_CONVERSION_IMPLICIT` pattern h5cpp uses for type-conversion policy.

**Default (no macro defined) — strict mode.**

Annotations and call-site properties **compose by property kind**. Orthogonal properties (one in annotation, the other at the call site) are merged. **Specifying the same property in both places is a compile-time error**, with a diagnostic naming the duplicated property and both source locations. This is the symmetric counterpart to `H5CPP_CONVERSION_EXPLICIT` (the strict-conversion default) — the safer choice; mirrors h5cpp's overall "no surprises" disposition.

```cpp
struct S { [[=h5::chunk{1024}]] std::vector<double> samples; };

h5::write(fd, "ds", obj);                          // OK — chunk=1024 from annotation
h5::write(fd, "ds", obj, h5::gzip{9});             // OK — orthogonal compose; chunk=1024 + gzip=9
h5::write(fd, "ds", obj, h5::chunk{2048});         // COMPILE ERROR: h5::chunk specified
                                                   //                in both annotation
                                                   //                and call-site
```

**Opt-in cascade mode — `#define H5CPP_LAYOUT_CASCADE` before including h5cpp headers.**

Annotations become *declared defaults*; call-site properties **override** them. Same-property duplication silently resolves to the call-site value. Same semantic as the existing `H5CPP_CONVERSION_IMPLICIT` opt-in for type-conversion: looser, more convenient, the user is taking responsibility for not shadowing their own annotations by accident.

```cpp
#define H5CPP_LAYOUT_CASCADE       // before #include <h5cpp/all>
#include <h5cpp/all>

struct S { [[=h5::chunk{1024}]] std::vector<double> samples; };

h5::write(fd, "ds", obj);                          // chunk=1024 from annotation
h5::write(fd, "ds", obj, h5::chunk{2048});         // chunk=2048; call-site overrides
h5::write(fd, "ds", obj, h5::chunk{4096} | h5::gzip{9});  // chunk=4096, gzip=9
```

**Why a macro and not an annotation.** The strict/cascade choice is a *project-wide* policy, not a per-type decoration. Setting it at the build-system level (compile flag or single `#define`) ensures every translation unit in a project follows the same rule and matches the precedent set by `H5CPP_CONVERSION_IMPLICIT`. Annotations are for per-type/per-field intent; macros are for global behavior toggles.

**Conservative recipe for the docs:** lead with the strict default; introduce `H5CPP_LAYOUT_CASCADE` as the opt-in for users who want call-site override convenience.

### C++26 reflection migration

C++26 shipped two relevant features (verified 2026-05-21):

- **P2996R13 — Reflection for C++26** (Vandevoorde, Sutton, Childers, Vali) — voted into C++26 June 2025; adds `^Type` operator and `std::meta::info`; queryable at constexpr time
- **P3394R4 — Annotations for Reflection** — adopted into C++26 at the 2025 Sofia meeting; attaches first-class constexpr values to declarations, queryable via `std::meta::annotations_of(reflection)`. Syntax: `[[=expr]]` where `expr` is a constant-expression of structural type

Under C++26 annotations, the same attribute set above is expressed as braced-init value constructions of the same bare names:

| Today (C++17 standard attribute syntax) | C++26 (annotation value, same bare name) |
|---|---|
| `[[h5::name("x")]]` | `[[=h5::name{"x"}]]` |
| `[[h5::ignore]]` | `[[=h5::ignore{}]]` |
| `[[h5::chunk(1024)]]` | `[[=h5::chunk{1024}]]` |
| `[[h5::compress(gzip, 6)]]` | `[[=h5::compress{filter::gzip, 6}]]` |
| `[[h5::on_missing(value)]]` | `[[=h5::on_missing{value}]]` |
| `[[h5::serialize_full]]` | `[[=h5::serialize_full{}]]` |

The only syntactic shift is `(args)` → `{args}` under the `[[=...]]` form. The name stays put.

**Three benefits of the reflection-annotation form:**

1. **Type-safe arguments** — `h5::compress{filter::gzip, 6}` is a real constructor call, not a token list the compiler parses by hand. Compile errors at the annotation site, not deep in the code generator.
2. **Removes the external tool** — the same code h5cpp-compiler emits today can be generated by constexpr code inside the user's compilation. No separate binary; no build-system glue.
3. **Annotations interoperate with the rest of the program** — a constexpr function can read an annotation and use it elsewhere; standard attributes are a one-way ticket to the codegen.

### Structural-type prerequisite (open verification)

C++26 annotations require the annotated value to be of *structural type* (no virtual functions, no mutable, no private non-static data, literal type for constexpr construction). h5cpp's existing property types (`h5::chunk` = `impl::dcpl_acall<...>` = `aprop_t<...>`) are already designed as value types — `final` struct with a public fixed-size array member and initializer-list constructors. **Verification needed** before publishing the C++26 syntax:

- Confirm `aprop_t`'s base `prop_t<…>` has only public structural members
- Confirm constructors are `constexpr`-usable in constant-evaluated context

If a small refactor is required to make the existing types reflection-friendly, that's a tractable internal change with no user-facing impact. If not possible, fall back to a sibling annotation type with the same bare name in a sub-namespace. Validation target: GCC 16.1 on Compiler Explorer with the actual h5cpp header.

### Migration phases

Phase A (today, C++17): ship the `[[h5::name("...")]]` token-style attributes; parse via Clang Tooling.

Phase B (C++20/23, still external tool): keep the same syntax; the tool stays the same.

Phase C (C++26, dual-mode): support both syntaxes. Users on older standards keep the token form; users on C++26 can switch to the typed annotation form. h5cpp-compiler becomes an *optional* tool — users wanting the no-external-binary path can opt into pure-reflection codegen via library templates.

Phase D (C++26+ widespread): the external tool becomes legacy; reflection-based codegen is the canonical path. `[[h5::...]]` syntax stays available as a `[[deprecated]]` alias for one major version.

**Net effect:** the attribute set ships today survives the transition. The *vehicle* changes from Clang Tooling to in-language reflection; the *user-facing surface* stays nearly identical.

## Library Dispatch

```cpp
namespace h5 {

template <typename T> struct has_scatter : std::false_type {};

template <typename T>
herr_t write(hid_t fd, const std::string& path, const T& obj) {
    if constexpr (has_scatter<T>::value) {
        auto dset = detail::open_or_create(fd, path);
        return scatter(dset, detail::next_row(dset), obj);
    } else {
        return detail::write_compound(fd, path, obj, register_struct<T>());
    }
}

} // namespace h5
```

The POD path and scatter path coexist in the same entry point. The trait is the only switch.

## Per-Type Generated Artifacts (Scatter Path)

For each scatter-eligible type T, the compiler emits exactly four artifacts:

1. **`row_t` struct + `compound_type()` factory** in private namespace `h5::generated::T_` — mirror of on-disk layout (scalars + `hvl_t` per indirect field) plus lazy-init HDF5 compound type id.
2. **`template<> herr_t h5::scatter<T>(hid_t dset, hsize_t row, const T& obj)`** — write path. Builds `hvl_t` descriptors pointing at `obj.vector.data()`; calls `h5::detail::write_one_row`.
3. **`template<> herr_t h5::gather<T>(hid_t dset, hsize_t row, T& obj)`** — read path. `H5Dread` into local `row_t`; `assign()` vectors from VLEN buffers; `H5Treclaim`.
4. **`H5CPP_REGISTER_SCATTER(T);`** at file scope — macro that sets `h5::has_scatter<T>::value = true` so the library dispatches to the scatter path.

### What stays in the library (not generated per-type)

- `h5::write` / `h5::read` entry points
- `h5::detail::write_one_row` / `h5::detail::read_one_row` helpers
- `h5::has_scatter<T>` trait template
- `H5CPP_REGISTER_SCATTER` macro itself
- Dataset open/create/chunking policy

## Worked Example

### Input (user code)

```cpp
struct frame_t {
    uint64_t              timestamp_ns;
    std::vector<double>   px, py, pz, vx, vy, vz;
    std::vector<uint32_t> id;
};
```

### Generated header (sketch)

```cpp
// frame_h5.hpp — emitted by h5cpp-compiler
#pragma once
#include <hdf5.h>
#include <h5cpp/core>
#include "frame.hpp"

// 1. row layout + compound type
namespace h5::generated::frame_t_ {

struct row_t {
    uint64_t timestamp_ns;
    hvl_t    px, py, pz, vx, vy, vz, id;
};

inline hid_t compound_type() {
    static const hid_t ct = []{
        hid_t v_d = H5Tvlen_create(H5T_NATIVE_DOUBLE);
        hid_t v_u = H5Tvlen_create(H5T_NATIVE_UINT32);
        hid_t ct  = H5Tcreate(H5T_COMPOUND, sizeof(row_t));
        H5Tinsert(ct, "timestamp_ns", HOFFSET(row_t, timestamp_ns), H5T_NATIVE_UINT64);
        H5Tinsert(ct, "px", HOFFSET(row_t, px), v_d);
        H5Tinsert(ct, "py", HOFFSET(row_t, py), v_d);
        H5Tinsert(ct, "pz", HOFFSET(row_t, pz), v_d);
        H5Tinsert(ct, "vx", HOFFSET(row_t, vx), v_d);
        H5Tinsert(ct, "vy", HOFFSET(row_t, vy), v_d);
        H5Tinsert(ct, "vz", HOFFSET(row_t, vz), v_d);
        H5Tinsert(ct, "id", HOFFSET(row_t, id), v_u);
        return ct;
    }();
    return ct;
}

} // namespace

// 2. scatter (write) — zero-copy
namespace h5 {
template<> inline herr_t scatter<::frame_t>(hid_t dset, hsize_t row, const ::frame_t& o) {
    using namespace ::h5::generated::frame_t_;
    row_t r{
        o.timestamp_ns,
        { o.px.size(), (void*)o.px.data() },
        { o.py.size(), (void*)o.py.data() },
        { o.pz.size(), (void*)o.pz.data() },
        { o.vx.size(), (void*)o.vx.data() },
        { o.vy.size(), (void*)o.vy.data() },
        { o.vz.size(), (void*)o.vz.data() },
        { o.id.size(), (void*)o.id.data() },
    };
    return h5::detail::write_one_row(dset, compound_type(), row, &r);
}

// 3. gather (read) — one copy through VLEN allocator
template<> inline herr_t gather<::frame_t>(hid_t dset, hsize_t row, ::frame_t& o) {
    using namespace ::h5::generated::frame_t_;
    row_t r{};
    if (auto rc = h5::detail::read_one_row(dset, compound_type(), row, &r); rc < 0) return rc;

    o.timestamp_ns = r.timestamp_ns;
    o.px.assign((double*)r.px.p, (double*)r.px.p + r.px.len);
    o.py.assign((double*)r.py.p, (double*)r.py.p + r.py.len);
    o.pz.assign((double*)r.pz.p, (double*)r.pz.p + r.pz.len);
    o.vx.assign((double*)r.vx.p, (double*)r.vx.p + r.vx.len);
    o.vy.assign((double*)r.vy.p, (double*)r.vy.p + r.vy.len);
    o.vz.assign((double*)r.vz.p, (double*)r.vz.p + r.vz.len);
    o.id.assign((uint32_t*)r.id.p, (uint32_t*)r.id.p + r.id.len);
    return H5Treclaim(compound_type(), H5S_ALL, H5P_DEFAULT, &r);
}
} // namespace h5

// 4. trait marker
H5CPP_REGISTER_SCATTER(::frame_t);
```

### User call site (unchanged)

```cpp
frame_t f = simulate();
h5::write(fd, "frames", f);   // dispatch chosen at compile-time via has_scatter trait
```

## Relationships

- **AI roadmap** (separate document): scatter/gather is orthogonal to the protobuf/JSON/SQL/Avro backend roadmap. It is a property of the *walker* and the *library dispatch*, applicable to every backend. The same matcher emits scatter glue alongside whatever schema artifact is requested.
- **h5cpp reflection sandwich**: this design slots into the existing core + compiler-generated shim + io layering. The shim already holds `register_struct<T>` specializations; adding `scatter<T>` / `gather<T>` is a same-layer extension.
- **Non-HDF5 backends**: the write-side approach generalizes — `hvl_t` becomes `iovec` (for `writev(2)`), `ibv_sge` (for RDMA), or `rte_mbuf` chains (for DPDK). The matcher tier classification is reusable; only the per-tier producer changes.

## Implementation Notes

- Cycles and polymorphism in tier 4 require runtime visited-ID tables and type registries. The generator can scaffold the call sites, but the user must accept the buffer copy.
- For tier 2+ types, do not add new entries to `cpp2hid` — that table is for `H5T_NATIVE_*` only. STL types like `std::complex`, `std::array`, `std::pair` belong in h5cpp's library-side type traits, not in the compiler.
- The compiler is a **multi-trait generator**, not a single-format emitter. One AST walk emits whatever specializations the type needs.

## Reference Examples

Four canonical input examples — one per tier — live in the h5cpp-compiler repo at `examples/tier-{one,two,three,four}/`. Each directory mirrors tier-one's structure: `CMakeLists.txt`, `README.md`, the user's class header, `vector.cpp` driver, and `generated.h` (real for tier 1, stub bootstrap placeholder for tiers 2–4 until scatter/gather codegen lands).

Each row below shows the **user class** the example defines (this is what the tier classification applies to), the HDF5 layout the compiler will emit for that class, and current implementation status.

| Directory | User class (the tier-classified C++ type) | HDF5 the compiler will emit | Status |
|---|---|---|---|
| `examples/tier-one/` | `sn::sensor::reading_t` — scalars + `double[3]` axes + scalar temperature; tier 1 because all fields are POD | `H5T_COMPOUND { uint64, uint32, ARRAY[3] double, float }`, chunked + gzip variant | ✔ Implemented; build + run verified |
| `examples/tier-two/` | `sn::sensor::session_t` — `std::string` label + 2× `std::vector<double>`; tier 2 because of the string and vector fields | `H5T_COMPOUND { uint64, uint64, VLEN_STRING, VLEN<double>, VLEN<double> }`, chunked, global-heap payloads | 🚧 Target state; build fails until scatter/gather lands |
| `examples/tier-three/` | `sn::sensor::network_t` — `std::vector<std::string>` + `std::map<uint32_t, std::vector<sample_t>>`; tier 3 because of the map and ragged vector-of-string | Either nested VLEN compound OR decomposed `/scans/.../keys`, `/offsets`, `/values` dataset group | 🚧 Target state |
| `examples/tier-four/` | `sn::sensor::log_t` — `std::vector<event_t>` where `event_t` has `std::variant<reading_t, calibration_t, fault_t>`; tier 4 because of the variant payload | `H5T_OPAQUE` payload + sibling tag dataset, or union-style compound with discriminant; field carries `[[h5::serialize_full]]` | 🚧 Target state; opt-in required |

Each file shows the *user-facing input*: the user's class definitions plus the `h5::write` / `h5::read` call site that "colors" the class for the AST matcher. The shim code that the compiler emits in response is described above (Per-Type Generated Artifacts). For tiers 2–4 the `generated.h` placeholder carries an explicit bootstrap comment naming the missing feature, so the failing build serves as the visible checkpoint.

**Note on top-level container wrapping.** When the user invokes `h5::write(fd, "ds", std::vector<T>{…})`, the *vector itself* is not a tier-classified user class — the library's existing top-level template handles it by iterating per element. The tier of `T` (the user class) is what determines the HDF5 layout per row and the MPI compatibility:

- `std::vector<reading_t>` (tier-1 element) → 1-D dataset of fixed compound, MPI ✔
- `std::vector<session_t>` (tier-2 element) → 1-D dataset of VLEN compound per row, MPI ✘
- `std::vector<network_t>` (tier-3 element) → ditto, worse

Wrapping a tier-N class in `std::vector` does not change N. It only repeats the per-element layout across rows.
