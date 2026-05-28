@page reports_compiler_backend_cookbook h5cpp-compiler Backend Cookbook

**Author:** Winston (System Architect)
**Date:** 2026-05-23
**Subject:** the transferable recipe for adding a new backend to h5cpp-compiler — distilled from the pb backend's Phase 1–4 + Commits 1–4 evolution
**Companions:**
- `tasks/h5cpp-compiler-multi-backend-architecture.md` — the multi-backend roof
- `tasks/h5cpp-compiler-pb-attribute-taxonomy.md` — the pb vocabulary specification
- `tasks/h5cpp-compiler-scatter-gather-design.md` — the HDF5 tier model + universal attribute set

---

## What this document is

A step-by-step recipe for adding a NEW format backend (JSON Schema, SQL DDL, Avro, FlatBuffers, GraphQL SDL, …) to h5cpp-compiler. Distilled from the four-commit pb-backend series on branch `31-pb-attribute-vocabulary`. The same shape applies to any future backend.

This is the **transferable skill**. Once you've done one backend you've done them all — they differ only in vocabulary and emission.

---

## The mental model: one walker, many producers

```
                ┌──────────────────────────────────────────┐
                │ Annotated C++ struct (your foo.cpp)     │
                │   [[pb::field(1)]]   std::string name;  │
                │   [[h5::chunk(1024)]] std::vector<v> xs; │
                │   [[sql::primary_key]] std::int64_t id;  │
                └────────────────────┬─────────────────────┘
                                     │
                          src/pb_attr_translator.hpp
                                     │
                                     ▼
                ┌──────────────────────────────────────────┐
                │ Source rewriter — lifts [[ns::name(args)]] │
                │ into [[clang::annotate("ns::name", args)]] │
                │ so Clang preserves args in the AST.       │
                └────────────────────┬─────────────────────┘
                                     │
                                     ▼
                ┌──────────────────────────────────────────┐
                │ Clang Tooling AST walk (single pass)     │
                └──────────────┬───────────────────────────┘
                               │
              ┌────────────────┴────────────────────────┐
              │       MatchFinder fan-out               │
              │  registers ONE callback per backend     │
              └─┬───────────────┬───────────────────┬───┘
                │               │                   │
                ▼               ▼                   ▼
       PbTemplateCallback  ProtoTemplateCb    JsonTemplateCb  …
       (consumer_pb.hpp)   (consumer_proto)   (consumer_json)
                │               │                   │
                ▼               ▼                   ▼
            foo_pb.hpp      foo.proto         foo.schema.json
```

Each backend = **one callback class + one emitter + per-backend goldens**. They share the source rewriter, the attribute readers, and the topological-dependency walker.

---

## The recipe — 6 steps

### Step 1. Pick the namespace + vocabulary

Decide what `[[<ns>::<name>(<args>)]]` attribute set users will write to drive your backend.

- Universal attributes (`pb::name`, `pb::doc`, `pb::ignore`, `pb::on_missing`, `pb::version`, `pb::alias`, `pb::name_all`) should be shared across backends where the meaning is identical. The pb backend already owns these in the `pb::` namespace — see `tasks/h5cpp-compiler-pb-attribute-taxonomy.md` §2.
- Backend-specific attributes go in your own namespace: `json::format`, `json::required`, `sql::primary_key`, etc.
- Write the spec FIRST (analogous to the pb attribute taxonomy). Decide tier 1/2/3/4 — must-have / high-value / nice-to-have / specialized.

### Step 2. Register the namespace with the source rewriter

`src/pb_attr_translator.hpp` has an `is_pb_attr_name()` predicate listing every recognized attribute leaf name. Add yours (or generalize the rewriter into multi-namespace if your backend uses a different prefix — today the rewriter is pb-prefix only).

```cpp
// In src/pb_attr_translator.hpp::is_pb_attr_name(), or in a new
// is_<ns>_attr_name() if your backend uses a different prefix:
return name == "field"
    || name == "wire"
    // ... your new attribute leaf names go here
    || name == "your_new_attr";
```

The rewriter lowers every `[[<ns>::your_new_attr(args)]]` to `[[clang::annotate("<ns>::your_new_attr", args)]]` before Clang's AST walker sees the file. From the AST's perspective, every backend's attributes look the same: a `clang::AnnotateAttr` whose first string is `"<ns>::<name>"` and whose trailing args are `clang::Expr*` nodes.

### Step 3. Write the callback (consumer)

Create `src/consumer_<backend>.hpp` containing a `MatchFinder::MatchCallback` subclass. The skeleton:

```cpp
#include "pb_attr_reader.hpp"        // SHARED readers — re-use don't copy
// + clang AST + tooling includes

class FooTemplateCallback : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    explicit FooTemplateCallback(const std::string& output_path)
        : output_path_(output_path) {}

    ~FooTemplateCallback() {
        // Assemble the output file in the right order. Most backends
        // buffer messages-body text during the walk and emit:
        //   1. format header (syntax / version preamble)
        //   2. imports / includes (collected during walk)
        //   3. package / namespace declaration
        //   4. message bodies (the buffered text)
        //   5. trailing tooling hints
        std::ofstream io(output_path_);
        io << "// Generated by h5cpp-compiler --<backend>.\n";
        // ... assemble per the format's grammar
    }

    void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override {
        const auto* node = result.Nodes.getNodeAs<clang::CXXRecordDecl>("cxxRecordDecl");
        if (!node) return;
        node = node->getDefinition();
        if (!node) return;

        // Topological walk: dependencies emit before referents.
        std::vector<const clang::CXXRecordDecl*> order;
        collect_deps_(node, order);
        for (const clang::CXXRecordDecl* r : order) {
            if (!emitted_.insert(r).second) continue;
            emit_message_(r);
        }
    }

private:
    std::string output_path_;
    std::ostringstream body_buf_;
    std::set<const clang::CXXRecordDecl*> emitted_;
    std::set<const clang::CXXRecordDecl*> seen_for_deps_;

    void collect_deps_(const clang::CXXRecordDecl*, std::vector<const clang::CXXRecordDecl*>&);
    void emit_message_(const clang::CXXRecordDecl*);
    std::string type_(clang::QualType qt) const;   // your C++ → format type map
};
```

The topological-walk machinery is identical across backends (records with user-type field dependencies emit those first). The differences live in `emit_message_` and `type_()`.

### Step 4. Build the C++ → format type map

This is the format-specific core. For each C++ type, decide what to emit.

| C++ shape | proto3 example | JSON Schema | SQL DDL | Avro |
|---|---|---|---|---|
| `bool` | `bool` | `{type: "boolean"}` | `BOOLEAN` | `boolean` |
| `std::int32_t` | `int32` | `{type: "integer", format: "int32"}` | `INTEGER` | `int` |
| `std::int64_t` | `int64` | `{type: "integer", format: "int64"}` | `BIGINT` | `long` |
| `std::string` | `string` | `{type: "string"}` | `TEXT` | `string` |
| `std::vector<T>` | `repeated T` | `{type: "array", items: T}` | `T[]` (Postgres) | `{type: "array", items: T}` |
| `std::optional<T>` | `optional T` | `{type: T, nullable: true}` | `T NULL` | `["null", T]` (union) |
| `std::map<K,V>` | `map<K, V>` | `{type: "object", additionalProperties: V}` | junction table | `{type: "map", values: V}` |
| `std::variant<...>` | `oneof { ... }` | `{oneOf: [...]}` | n/a (discriminator pattern) | `union: [...]` |
| `chrono::system_clock::time_point` | `google.protobuf.Timestamp` | `{type: "string", format: "date-time"}` | `TIMESTAMP` | `{type: "long", logicalType: "timestamp-micros"}` |
| `enum class` | `enum Foo { ... }` decl | `{enum: [...]}` | `CREATE TYPE foo AS ENUM (...)` | `{type: "enum", symbols: [...]}` |
| nested user struct | message reference | `$ref` | foreign key | reference |

Each backend's `type_()` returns a string in the format's vocabulary. The pb backend's `consumer_proto.hpp::proto3_type_` is the worked example; mimic its structure for a new backend.

### Step 5. Plumb the CLI + the test harness

Two changes to glue your backend in:

**`src/h5cpp.cpp`** — add a `--<backend>-out <file>` flag and register your callback alongside the existing pb backend:

```cpp
static llvm::cl::opt<std::string> FooOutputFile("foo-out",
    llvm::cl::desc("Output file for the new <backend> emitter"),
    llvm::cl::value_desc("file"),
    llvm::cl::cat(MyToolCategory));

// In main(), after the pb_attr_translator::install_virtual_files call:
std::optional<FooTemplateCallback> foo_cb;
if (!FooOutputFile.empty()) {
    foo_cb.emplace(FooOutputFile);
    Finder.addMatcher(pbTemplateMatcher, &*foo_cb);
}
```

Same `MatchFinder`, same `pbTemplateMatcher`. The callback fan-out gives you a single AST walk producing multiple artifacts.

**`tests/run_fixture.cmake`** — extend the harness with an optional `<BACKEND>_GOLDEN` argument, mirroring the existing `PROTO_GOLDEN`:

```cmake
set(foo_observed "${OUTPUT_DIR}/${fixture_name}.foo.observed")
set(foo_flags "")
if(DEFINED FOO_GOLDEN AND NOT FOO_GOLDEN STREQUAL "")
  list(APPEND foo_flags "--foo-out" "${foo_observed}")
endif()
# (pass foo_flags to h5cpp; diff against FOO_GOLDEN if set)
```

`tests/CMakeLists.txt`'s `add_h5cpp_pb_fixture_test` auto-picks up `<name>.foo.expected` goldens if they exist. Same pattern.

### Step 6. Add fixtures + goldens

Every existing pb fixture (`pb_primitives.cpp`, `pb_strings_enums.cpp`, `pb_composites.cpp`, …) is already a multi-backend test source — the source itself doesn't change between backends; only what's emitted differs. For each fixture, add a `<name>.<backend>.expected` golden showing your backend's emission.

For backend-specific attributes (e.g. `[[json::format("date-time")]]`), add new fixtures exercising them: `<backend>_<feature>.cpp`.

---

## What you reuse — and what's already done for you

| Piece | Where it lives | Already done? |
|---|---|---|
| Source rewriter (`[[ns::attr(args)]]` → `[[clang::annotate(...)]]`) | `src/pb_attr_translator.hpp` | ✔ (extend `is_pb_attr_name` for new attribute names) |
| Shared attribute readers | `src/pb_attr_reader.hpp` | ✔ — `find_annotate`, `read_string_arg`, `read_int_args`, `read_first_arg_text`, class/field/enum convenience wrappers |
| MatchFinder + AST walk plumbing | `src/h5cpp.cpp::main` | ✔ — add one `optional<FooTemplateCallback>` next to the existing pb callback |
| `pbTemplateMatcher` (matches structs via `pb::encode/decode` call sites) | `src/h5cpp.cpp` | ✔ — same matcher works for every backend; new backends just register a different callback |
| Topological dependency walk | mimic `consumer_proto.hpp::collect_deps_` | partial — copy the pattern (helper extraction across backends is a future cleanup) |
| Test harness | `tests/run_fixture.cmake` | ✔ — extend with one new `<BACKEND>_GOLDEN` param, mirror the existing `PROTO_GOLDEN` block |
| CMake fixture loop | `tests/CMakeLists.txt::add_h5cpp_pb_fixture_test` | ✔ — already auto-detects sibling goldens |

---

## What you have to write — for any new backend

| Piece | Approximate size (lines) |
|---|---|
| `src/consumer_<backend>.hpp` | 300–500 (callback + emit logic + type map) |
| `src/h5cpp.cpp` CLI flag + callback registration | 10 |
| `tests/run_fixture.cmake` `<BACKEND>_GOLDEN` extension | 20 |
| Fixtures + goldens | per fixture: 20 lines of source + 1 golden file |

That's it. The biggest piece is your emitter — and that's the part that has to be different anyway, because each format's grammar is different.

---

## Worked example — what the pb backend looks like under this recipe

Walking through the pb backend as a concrete instance of the recipe:

| Recipe step | pb backend artifact |
|---|---|
| 1. Namespace + vocabulary | `pb::*` (Tier 1–4 in `h5cpp-compiler-pb-attribute-taxonomy.md`) |
| 2. Rewriter registration | `pb_attr_translator.hpp::is_pb_attr_name` lists every pb attribute name (field, wire, adapter, ignore, name, doc, on_missing, version, alias, name_all, reserved, packed, deprecated, package, oneof_name, json_name, target_syntax, descriptor_set_out, service, encode_with, decode_with, tier, reject, enum_zero, unknown_field_set) |
| 3. Callback | `consumer_pb.hpp::PbTemplateCallback` (emits `.hpp` shim) + `consumer_proto.hpp::ProtoTemplateCallback` (emits `.proto` schema) — TWO callbacks share the same source rewriter and reader infrastructure |
| 4. Type map | `consumer_pb.hpp` defers to `pb.hpp`'s runtime trait dispatch (no `.hpp`-side type rewriting needed); `consumer_proto.hpp::proto3_type_` is the explicit C++ → proto3 map |
| 5. CLI plumbing | `h5cpp.cpp` registers PbTemplateCallback (always when `--protocol-buffers`) + optionally ProtoTemplateCallback (when `--proto-out <file>` is set) — both observe the same `pbTemplateMatcher` |
| 6. Fixtures + goldens | `tests/fixtures/pb_*.cpp` + `.expected` + `.proto.expected` siblings; `tests/CMakeLists.txt` auto-detects |

The pb backend's evolution across Phases 1–4 + Commits 1–4 was driven by **adding more attributes to the vocabulary** and **extending the emit_* methods** — never by changing the recipe. The recipe is the constant.

---

## Anti-patterns to avoid (caught the hard way)

1. **Don't duplicate the attribute readers across backends.** Commit 3 extracted them into `pb_attr_reader.hpp`. Adding a third backend without using the shared header doubles your maintenance surface for no gain.

2. **Don't try to drive multiple backends through one callback.** Two callbacks observing the same `MatchFinder` is cleaner than one mega-callback emitting to N output streams. The pb backend learned this in Phase 3.

3. **Don't emit C++ that references symbols your runtime library doesn't define.** Commit 3 was held back to single-feature scope precisely because emitting `pb::custom_field<>` references would break the build of every user of pb.hpp. The cross-repo runtime work (sandbox `4-pb-default-for`) had to land FIRST before the h5cpp-compiler emission could be wired.

4. **Don't dispatch on `Format ==` discriminators in `h5cpp.cpp::main`.** That's a leaky abstraction — adding a backend means editing the if/else chain. Instead: per-flag `optional<Callback>` instances that opt in. The pb backend's `ProtoTemplateCallback` is registered this way.

5. **Don't put format-specific type knowledge in `consumer_<backend>.hpp` body. Put it in a private `type_()` method.** The pb backend's `consumer_proto.hpp::proto3_type_` is the worked example — one method, one switch, easy to extend.

6. **Don't forget the topological walk.** Records referenced as field types must emit before the records that reference them, so message-name references resolve. The pb backend's `collect_deps_` recurses into stdlib template args (`std::vector<UserType>`, `std::map<K, UserType>`) — replicate.

7. **Don't skip the test harness extension.** Without `<BACKEND>_GOLDEN` in `run_fixture.cmake`, the proto golden tests would be cosmetic — they'd write `.observed` files but never diff against goldens. Commit 3's emergency fix taught this.

---

## Where to next

Once a backend is built following this recipe, the natural extension is:

- **Multi-format CMake helper** (`h5cpp_compiler_generate(FORMATS protocol-buffers json sql avro ...)`) — the multi-backend architecture's vision. Drop a single `add_custom_command` that drives all backends in one h5cpp invocation.
- **Universal attribute lift** — move `name`/`doc`/`ignore`/etc. from per-backend namespaces (`pb::name`, `h5::name`, `json::name`) into a shared root namespace. The multi-backend architecture doc envisions `h5::name` as the universal version. Cross-backend coordination needed.
- **C++26 reflection migration** — once `[[=ns::attr{args}]]` typed annotations ship, the source rewriter retires (Clang's reflection machinery does the lift natively). The recipe simplifies: callbacks read annotations via `std::meta::annotations_of(...)` instead of via `AnnotateAttr`.

The recipe stays the same. The vehicle changes.

## Related examples

- [`examples/multi-tu/tu-01.cpp`](../../../examples/multi-tu/tu-01.cpp) — multi-TU compilation pattern (the compiler's emission target)
- [`examples/compound/compound.cpp`](../../../examples/compound/compound.cpp) — compound type registration end-to-end
