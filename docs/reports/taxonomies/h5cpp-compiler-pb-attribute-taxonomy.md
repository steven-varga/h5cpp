@page reports_compiler_pb_attribute_taxonomy pb.hpp Attribute Vocabulary

**Date:** 2026-05-22
**Authors:** Steven Varga, Winston (Architecture)
**Status:** Design; emission targeting `vargalabs/h5cpp-compiler#30-pb-producer` (partial implementation today, see §9)
**Repos:** `vargalabs/sandbox` (`pb.hpp` runtime) · `vargalabs/h5cpp-compiler` (annotation reader, descriptor emitter)
**Scope:** Standalone protobuf attribute vocabulary. The multi-backend roof in `tasks/h5cpp-compiler-multi-backend-architecture.md` keeps its own `h5::proto::*` sub-namespace; this doc specifies the `pb::*` surface for users who consume pb.hpp directly.
**Companions:**
- `tasks/h5cpp-compiler-multi-backend-architecture.md` — multi-backend roof; `h5::proto::*` stays the namespace there
- `tasks/h5cpp-compiler-scatter-gather-design.md` — the tier-1..4 model and the C++26 migration phases
- `tasks/pb-feature-coverage-and-gaps.md` — `pb.hpp` runtime feature inventory

## TL;DR

User-facing attribute set for protobuf annotations on plain C++ structs. **Vocabulary is intentionally identical to `h5::*` where the concept overlaps** (rename, ignore, doc, version, alias, on_missing, name_all) — different namespace, same words. The protobuf-specific surface lives only under `pb::*`, with `pb::field(N)` echoing the runtime template `pb::field<N, &T::m>{}` so the same word means the same thing at the annotation layer and the descriptor layer.

C++17 attribute syntax today; one-line lift to C++26 typed annotations tomorrow.

| Surface today (C++17 standard-attribute) | C++26 reflection form |
|---|---|
| `[[pb::field(2)]]` | `[[=pb::field{2}]]` |
| `[[pb::field(5, 6, 7)]]` (variadic — variant member → oneof) | `[[=pb::field{5, 6, 7}]]` |
| `[[pb::wire(sint32)]]` | `[[=pb::wire{pb::spec::sint32}]]` |
| `[[pb::adapter(Timestamp)]]` | `[[=pb::adapter{pb::adapter_kind::Timestamp}]]` |
| `[[pb::ignore]]` | `[[=pb::ignore{}]]` |
| `[[pb::name("on_wire")]]` | `[[=pb::name{"on_wire"}]]` |
| `[[pb::doc("comment")]]` | `[[=pb::doc{"comment"}]]` |

Only syntactic shift is `(args)` → `{args}` under the `[[=...]]` form. **Names stay put.**


## 2. Universal vocabulary — same words, `pb::` namespace

These attributes use vocabulary identical to `h5::*` (defined in `h5cpp-compiler-scatter-gather-design.md` §"Tier 1 Must-have" and §"Tier 2 High value, low cost"). They live in `pb::` so the namespace stays self-contained for pb-only users; a user wanting both backends writes both `[[h5::name(...)]]` and `[[pb::name(...)]]` (typically with the same string).

### Universal Tier 1 — must-have

| Attribute | Purpose | Example |
|---|---|---|
| `[[pb::name("on_wire_name")]]` | Rename a field or message for wire emission; decouples C++ identifier from `.proto` field name. Drives the field name in the emitted `.proto` and the JSON serialization name (unless `pb::json_name` overrides). Wire bytes themselves use tag numbers, not names — so a rename is `.proto`-schema-level. | `[[pb::name("temperature_k"), pb::field(2)]] double temperature;` |
| `[[pb::ignore]]` | Skip this field entirely. Maps to `pb::ignore<&T::m>{}` in the emitted descriptor; field absent from the `.proto`; runtime never touches it. | `[[pb::ignore]] int debug_counter;` |
| `[[pb::on_missing(value)]]` | Default value the **decoder** writes into the C++ member when the field is absent on the wire. proto3 has zero-defaults on the wire format itself, so this drives the C++-side initializer in the generated shim, not the schema. | `[[pb::field(3), pb::on_missing(0.0)]] double sample_rate;` |

### Universal Tier 2 — high value, low cost

| Attribute | Purpose | Example |
|---|---|---|
| `[[pb::doc("description")]]` | Trailing `//` comment on the field in the emitted `.proto`. Round-trips through protoc into the `FileDescriptorSet.SourceCodeInfo`. Self-documenting schemas. | `[[pb::doc("nanoseconds since epoch"), pb::field(1)]] std::uint64_t ts;` |
| `[[pb::alias("old_name")]]` | Backward-compat for schema evolution. Emitted as `reserved "old_name";` at message scope so the legacy name can never be reused. | `[[pb::name("temp"), pb::alias("temperature_c"), pb::field(2)]] float temp;` |
| `[[pb::name_all("snake_case" \| "camelCase" \| "PascalCase")]]` | **Class-level** naming convention. Protobuf style guide calls for snake_case fields; `name_all("snake_case")` flips `bookmarkUrl` (C++) to `bookmark_url` (wire) uniformly. Per-field `pb::name` overrides. | `struct [[pb::name_all("snake_case")]] user_event_t { ... };` |

### Universal Tier 3 — nice to have

| Attribute | Purpose |
|---|---|
| `[[pb::version(N)]]` | Schema version; emitted as `option (pb.schema_version) = N;` at message scope. Lets the decoder consult a version policy on read. |

The full universal list mirrors h5cpp-compiler-scatter-gather-design.md §"User-Facing Attribute System". Any universal `h5::*` attribute not listed above has no protobuf semantics (e.g. `h5::chunk`, `h5::compress` are HDF5-storage concerns).

## 3. Pb-specific vocabulary — tier 1..4

### Tier 1 — must-have

Without these, the protobuf producer either can't emit a valid `.proto` (field numbers are mandatory) or loses access to features that are core to proto3 semantics (`oneof`, alternate wire types, well-known-type adapters, map auto-detection).

| Attribute | Purpose | Example |
|---|---|---|
| `[[pb::field(N)]]` | Required for every wire-emitted field. proto3 mandates explicit numbers; auto-assignment is disallowed because source reordering would break wire compatibility. Range `[1, 2^29 - 1]`. **Argument may be an integer literal OR an enum value of any underlying type convertible to `std::uint32_t`** — see §6 for the rationale and worked example. | `[[pb::field(2)]] std::int32_t id;` or `[[pb::field(user_profile_tag::id)]] std::int32_t id;` |
| `[[pb::field(N1, N2, N3, ...)]]` | **Variadic form on `std::variant` members.** Compiler sees a variant + multiple tags → emits `pb::oneof<&T::v, pb::alt<T1,N1>, pb::alt<T2,N2>, ...>{}`. Group name defaults to the C++ member identifier; override with `pb::oneof_name(...)`. Variant alternative 0 must be `std::monostate` (the absent state). Same int-or-enum flexibility as the single-tag form. | `[[pb::field(5, 6, 7)]] std::variant<std::monostate, std::string, std::int64_t, double> payload;` |
| `[[pb::wire(spec)]]` | Pin the wire encoding when the C++ type → proto3 mapping isn't natural. Valid `spec`: `sint32`, `sint64` (zigzag), `fixed32`, `fixed64` (4/8-byte LE unsigned), `sfixed32`, `sfixed64` (4/8-byte LE signed). | `[[pb::field(3), pb::wire(sint32)]] std::int32_t delta;` |
| `[[pb::adapter(name)]]` | Route the field through `pb::<name>_adapter` to bridge a non-protobuf C++ type to a well-known-type message. Today: `Timestamp` (`std::chrono::system_clock::time_point ↔ google.protobuf.Timestamp`), `Duration` (`std::chrono::duration ↔ google.protobuf.Duration`). User-extensible by defining a new `pb::Foo_adapter`. | `[[pb::field(4), pb::adapter(Timestamp)]] std::chrono::system_clock::time_point when;` |

**Map fields** auto-detect from `std::map<K,V>` / `std::unordered_map<K,V>` via `is_pb_map_v` (shipped on `3-pb-map`). The emitted `pb::field<N, &T::m>{}` is unchanged; the runtime trait decides what to do. No new attribute required for maps at tier 1.

### Tier 2 — high value, low cost

| Attribute | Purpose | Example |
|---|---|---|
| `[[pb::reserved(N1, N2, "old_name")]]` | **Class-level.** Reserves tag numbers and/or names so they can't be reused by future fields — proto3's standard schema-evolution mechanism. Mixed numbers and names allowed in one attribute. Per-field reservation isn't a thing in proto3; if listed at field scope, the compiler aggregates into one message-level `reserved` clause. | `struct [[pb::reserved(10, 11, "obsolete")]] event_t { ... };` |
| `[[pb::packed]]` | Force packed wire encoding for `std::vector<scalar>`. proto3 defaults numerics to packed already; this is explicit-intent documentation. Useful for `std::vector<bool>` (which **is** packed but where users sometimes forget). | `[[pb::field(2), pb::packed]] std::vector<bool> flags;` |
| `[[pb::deprecated]]` | Marks the field with `[deprecated = true]` in the emitted `.proto`. Pure metadata; pb.hpp's wire path treats it as a comment. | `[[pb::field(3), pb::deprecated]] std::int32_t legacy_count;` |
| `[[pb::package("foo.bar")]]` | **Class-level.** Emits `package foo.bar;` at the top of the generated `.proto`. Default: anonymous package, which is functional but generally undesirable for any project that ships beyond one binary. | `struct [[pb::package("com.vargalabs.events")]] event_t { ... };` |
| `[[pb::oneof_name("explicit")]]` | Override the auto-derived oneof group name (which otherwise mirrors the C++ variant-member identifier). Useful when the C++ name is private/internal and the wire-side name should be different. | `[[pb::field(5, 6, 7), pb::oneof_name("payload_kind")]] std::variant<...> _v;` |
| `[[pb::map_key_wire(spec)]]` / `[[pb::map_value_wire(spec)]]` | Override the wire encoding for a map's key or value. **Deferred to v1.1 per the `3-pb-map` design call** ("natural mapping only" was Steven's selection 2026-05-22). Listed here for taxonomy completeness; do not implement before user demand surfaces. | *(deferred)* |

### Tier 3 — nice to have

| Attribute | Purpose |
|---|---|
| `[[pb::json_name("alternateJsonName")]]` | Override the JSON serialization name — drives protoc's `json_name = "..."` field option. Useful when matching an existing JS/Python API naming convention without touching the C++ name. |
| `[[pb::unknown_field_set(preserve \| skip)]]` | Per-class policy. `preserve` (future) round-trips unknown fields on decode; `skip` (current default per FR7) drops them. Tracks the open v1.1 decision in `tasks/pb-feature-coverage-and-gaps.md` §5. |
| `[[pb::target_syntax(proto3 \| edition2023)]]` | **Class-level.** Selects the `.proto` syntax line. Default: `proto3`. `edition2023` lands when we move past `libprotoc 25.1` to 26+. |
| `[[pb::descriptor_set_out("file.desc")]]` | **Class-level.** Emit a binary `FileDescriptorSet` (the `.desc` / `.pb` file) alongside the `.proto` source. Matches protoc's `--descriptor_set_out`. Useful for runtime reflection bridges. |
| `[[pb::service("ServiceName")]]` | **Class-level, advanced.** If the struct contains members of type `std::function<Response(Request)>` (or a similar trait-detectable RPC shape), emit a `service ServiceName { ... }` block with RPC declarations. Wire path untouched. Own design pass warranted before implementation. |

### Tier 4 — specialized (defer)

| Attribute | Purpose |
|---|---|
| `[[pb::encode_with(fn)]]` / `[[pb::decode_with(fn)]]` | Custom per-field codec functions. Lighter alternative to defining a full `pb::<Name>_adapter` struct — pass a free function directly. Lands as a thin `adapter_field` variant in the descriptor. |
| `[[pb::tier(N)]]` | Escape hatch for the future tier classifier — force a class into a specific tier when auto-detection is wrong. Mirrors `h5::tier(N)` from the scatter-gather doc. |
| `[[pb::reject]]` | Compile-error if the protobuf producer is asked to emit this type. Counterpart of `h5::reject`. Lets a class be HDF5-only without ever leaking onto a wire. |

## 4. Class-level vs field-level scoping

Same pattern as `h5::` for HDF5: class-level attributes set defaults; field-level overrides them.

```cpp
struct [[pb::name_all("snake_case"),
        pb::package("com.vargalabs.events"),
        pb::name("UserEvent"),                     // .proto message name override
        pb::reserved(10, 11, "legacy_flag"),
        pb::doc("user-level event captured by the gateway"),
        pb::version(2)]]
user_event_t {

    [[pb::name("ts"),
      pb::doc("nanoseconds since epoch"),
      pb::field(1)]]
    std::uint64_t timestamp_ns;

    [[pb::field(2),
      pb::wire(sint32)]]
    std::int32_t delta_from_anchor;                // zigzag, often negative

    [[pb::field(3),
      pb::adapter(Timestamp)]]
    std::chrono::system_clock::time_point captured_at;

    [[pb::field(4)]]
    std::map<std::string, std::int32_t> scores;    // auto-detected as proto3 map

    [[pb::field(5, 6, 7)]]                          // variant + 3 tags → oneof
    std::variant<std::monostate,
                  std::string,
                  std::int64_t,
                  double>                          payload;

    [[pb::ignore,
      pb::doc("runtime cache; never persisted")]]
    void* runtime_handle;
};
```

## 5. C++26 reflection migration

C++26 ships P2996R13 (Reflection for C++26, voted in June 2025) and P3394R4 (Annotations for Reflection, adopted at Sofia 2025). Under the typed annotation form, the same names above are constructor calls of structural-type values.

Implementation sketch for the `pb::` value types (each must be a structural type — `final` struct, public data members, `constexpr` constructors):

```cpp
namespace pb {

    // Tier 1 — must-have
    //
    // Constructors are templated on any type convertible to std::uint32_t,
    // so both integer literals and enum values work uniformly:
    //   [[=pb::field{2}]]                       // integer
    //   [[=pb::field{user_profile_tag::id}]]   // enum
    //   [[=pb::field{tag::a, 7, tag::c}]]      // mixed
    struct field {
        template<class T>
            requires std::convertible_to<T, std::uint32_t>
        constexpr field(T n)
            : tags{{static_cast<std::uint32_t>(n), 0, 0, 0, 0, 0, 0, 0}}, count{1} {}

        template<class... Ts>
            requires (sizeof...(Ts) >= 2)
                  && (std::convertible_to<Ts, std::uint32_t> && ...)
        constexpr field(Ts... ns)
            : tags{{static_cast<std::uint32_t>(ns)...}}, count{sizeof...(Ts)} {}

        std::array<std::uint32_t, 8> tags;       // 8 max alts in a oneof (raise if needed)
        std::size_t                   count;
    };

    enum class spec { sint32, sint64, fixed32, fixed64, sfixed32, sfixed64 };
    struct wire { spec value; constexpr wire(spec s) : value{s} {} };

    enum class adapter_kind { Timestamp, Duration /* user-extensible */ };
    struct adapter { adapter_kind value; constexpr adapter(adapter_kind a) : value{a} {} };

    struct ignore {};

    // Universal (identical vocabulary to h5::, in pb:: namespace)
    struct name {
        std::array<char, 64> str;    // bounded; structural-type-friendly
        std::size_t          len;
        constexpr name(std::string_view s) : str{}, len{s.size()} {
            for (std::size_t i = 0; i < s.size() && i < 64; ++i) str[i] = s[i];
        }
    };
    struct doc { /* same shape as name, longer buffer */ };
    struct alias { /* same shape as name */ };
    struct version { std::uint32_t value; };
    struct on_missing { /* templated structural — see implementation note */ };

    enum class naming { snake_case, camel_case, pascal_case, kebab_case };
    struct name_all { naming value; };

    // Tier 2
    struct reserved {
        std::array<std::uint32_t, 16> numbers;
        std::array<name,           8> names;
        std::size_t                   n_numbers, n_names;
    };
    struct packed {};
    struct deprecated {};
    struct package { /* same shape as name */ };
    struct oneof_name { /* same shape as name */ };

    // Tier 3
    struct json_name { /* same shape as name */ };
    enum class unknown_field_policy { preserve, skip };
    struct unknown_field_set { unknown_field_policy value; };
    enum class proto_target { proto3, edition2023 };
    struct target_syntax { proto_target value; };
    // ...
}
```

Under C++17 attribute syntax `[[pb::field(2)]]`, h5cpp-compiler's Clang-Tooling backend parses the namespace-scoped attribute directly (no `clang::annotate` envelope). The implementation work is mechanical — a single namespace-aware attribute matcher swapped in for the current per-string parser.

Under C++26 typed annotations `[[=pb::field{2}]]`, the value gets reflected via `std::meta::annotations_of(^member)` and read at constexpr time from inside `pb.hpp` itself — h5cpp-compiler becomes an optional convenience, not a required tool.

### Structural-type prerequisite

C++26 annotations require the annotated value to be of *structural type* (no virtual functions, no mutable, no private non-static data, literal type for constexpr construction). The sketches above are designed to satisfy this — fixed-size `std::array` buffers instead of `std::string`, public data members, `constexpr` constructors. **Verification needed** before publishing the C++26 syntax against an actual `g++-16` build, paralleling the verification gate in the scatter-gather doc §"Structural-type prerequisite".

## 6. Tag arguments — integers, enums, or both

`pb::field(...)` accepts integer literals, enum values, and any mix of the two. The compiler (Phase B) folds whatever you pass to its underlying `std::uint32_t` at AST-evaluation time — Clang's `Expr::EvaluateAsInt(ctx)` handles integer literals and enum constant references uniformly — and the emitted descriptor sees plain integers either way. Under C++26 reflection (Phase C) the same flexibility lives in the templated `pb::field` constructor in §5; same vocabulary, same payload.

```cpp
[[pb::field(1)]]                              // integer literal
[[pb::field(user_profile_tag::scores)]]       // enum value
[[pb::field(my_tag::a, 7, my_tag::c)]]        // mixed
```

The runtime is unaffected — the emitted `pb::field<2, &T::m>{}` template gets a plain integer regardless of which form the user wrote.

### When to prefer enums

Enums collapse a class's tag assignments into a single named source of truth. The typical pattern:

```cpp
namespace events {

enum class user_profile_tag : std::uint32_t {
    // active tags
    name      = 1,
    scores    = 2,
    tags      = 3,
    waypoints = 4,
    flags     = 5,
    // 6..9 reserved (v1.0 legacy — never reuse)
};

struct [[pb::package("com.vargalabs.events"),
        pb::reserved(6, 7, 8, 9)]]
user_profile_t {
    [[pb::field(user_profile_tag::name)]]      std::string                                       name;
    [[pb::field(user_profile_tag::scores)]]    std::map<std::string,  std::int32_t>              scores;
    [[pb::field(user_profile_tag::tags)]]      std::unordered_map<std::int64_t, std::string>     tags;
    [[pb::field(user_profile_tag::waypoints)]] std::map<std::string, point_t>                    waypoints;
    [[pb::field(user_profile_tag::flags)]]     std::map<bool, std::string>                       flags;
};

} // namespace events
```

The benefits, vs. raw integers:

- **One source of truth.** Tag assignments live in the enum, not scattered across attribute literals. A reviewer sees the schema in one block.
- **Refactor-safe.** Renaming `user_profile_tag::scores` updates every annotation referencing it — same C++ name-binding as any other identifier; same IDE support as any other rename.
- **Schema-evolution context lives next to the schema.** Gaps in the enum can carry inline `// reserved (was: legacy_email)` comments where they belong. Pairs naturally with `[[pb::reserved(...)]]` at class scope.
- **Strong-typing flexibility.** Wrong-enum references (`[[pb::field(other_struct_tag::name)]]`) compile fine under the permissive default — but tooling, linters, or future opt-in modes can layer stricter binding on top without the library getting in the way.

Variant/oneof works the same way — the variadic `pb::field(...)` form takes any mix of enum and integer arguments:

```cpp
enum class event_payload_tag : std::uint32_t {
    text   = 5,
    number = 6,
    score  = 7,
};

struct event_t {
    [[pb::field(1)]] std::int64_t timestamp_ns;

    [[pb::field(event_payload_tag::text,
                event_payload_tag::number,
                event_payload_tag::score)]]
    std::variant<std::monostate, std::string, std::int64_t, double> payload;
};
```

`[[pb::reserved(...)]]` accepts the same forms:

```cpp
[[pb::reserved(user_profile_tag_legacy::email,
                user_profile_tag_legacy::phone, 100)]]   // enums + raw int, mixed
```

### Stance: permissive by default

The library accepts integers, enums, or any mix. No `#define`-gated strict mode, no class-level "must reference this enum" binding. Whether to commit to an enum convention is a project-level decision; the library stays out of it.

> We are the guides — this is the user's story. The library's job is to clear the path, not to choose the route.

## 7. Worked example — your UserProfile, rewritten

Here is the example from the `3-pb-map` push, transcoded from today's `clang::annotate` form to the proposed `pb::` attribute vocabulary:

### Before (today on `30-pb-producer`)

```cpp
struct user_profile_t {
    [[clang::annotate("pb::field=1")]] std::string                                       name;
    [[clang::annotate("pb::field=2")]] std::map<std::string,  std::int32_t>              scores;
    [[clang::annotate("pb::field=3")]] std::unordered_map<std::int64_t, std::string>     tags;
    [[clang::annotate("pb::field=4")]] std::map<std::string, point_t>                    waypoints;
    [[clang::annotate("pb::field=5")]] std::map<bool, std::string>                       flags;
};
```

### After (Phase B — C++17 standard-attribute syntax with namespace-scoped attributes)

```cpp
struct [[pb::name_all("snake_case"),
        pb::package("com.vargalabs.events"),
        pb::doc("user-facing profile aggregate")]]
user_profile_t {
    [[pb::field(1), pb::doc("display name")]]
    std::string                                       name;

    [[pb::field(2)]]
    std::map<std::string,  std::int32_t>              scores;

    [[pb::field(3)]]
    std::unordered_map<std::int64_t, std::string>     tags;

    [[pb::field(4)]]
    std::map<std::string, point_t>                    waypoints;

    [[pb::field(5)]]
    std::map<bool, std::string>                       flags;

    [[pb::ignore]]
    void*                                             runtime_handle;
};
```

### After (Phase C — C++26 reflection annotations)

```cpp
struct [[=pb::name_all{pb::naming::snake_case},
        =pb::package{"com.vargalabs.events"},
        =pb::doc{"user-facing profile aggregate"}]]
user_profile_t {
    [[=pb::field{1}, =pb::doc{"display name"}]]
    std::string                                       name;

    [[=pb::field{2}]]
    std::map<std::string,  std::int32_t>              scores;

    // ... etc — `()` → `{}`, names unchanged
};
```

### Variant: enum-typed tag arguments (any phase)

Independently of which annotation phase the user is on, the tag argument itself can be an enum value rather than a raw integer. This is the convention recommended in §6 for projects that want a single named source of truth for tag assignments:

```cpp
namespace events {

enum class user_profile_tag : std::uint32_t {
    name      = 1,
    scores    = 2,
    tags      = 3,
    waypoints = 4,
    flags     = 5,
};

struct [[pb::name_all("snake_case"),
        pb::package("com.vargalabs.events"),
        pb::doc("user-facing profile aggregate")]]
user_profile_t {
    [[pb::field(user_profile_tag::name), pb::doc("display name")]]
    std::string                                       name;

    [[pb::field(user_profile_tag::scores)]]
    std::map<std::string,  std::int32_t>              scores;

    [[pb::field(user_profile_tag::tags)]]
    std::unordered_map<std::int64_t, std::string>     tags;

    [[pb::field(user_profile_tag::waypoints)]]
    std::map<std::string, point_t>                    waypoints;

    [[pb::field(user_profile_tag::flags)]]
    std::map<bool, std::string>                       flags;

    [[pb::ignore]]
    void*                                             runtime_handle;
};

} // namespace events
```

Mix and match is allowed — `[[pb::field(my_tag:/home/steven/projects/vargalabs-workspace/tasks/h5cpp-compiler-pb-attribute-taxonomy.md:name)]]` next to `[[pb::field(7)]]` next to `[[pb::field(other_tag::foo, 11)]]`. The library doesn't care; the compiler folds everything to integers before emission.

### The descriptor (unchanged across all phases and forms)

What h5cpp-compiler emits is the same regardless of whether the user wrote raw integers, enum values, or any mix — the descriptor is in terms of compile-time integer NTTPs:

```cpp
template<> struct pb::meta::descriptor_t<user_profile_t> {
    static constexpr auto fields = std::tuple{
        pb::field<1, &user_profile_t::name>{},
        pb::field<2, &user_profile_t::scores>{},
        pb::field<3, &user_profile_t::tags>{},
        pb::field<4, &user_profile_t::waypoints>{},
        pb::field<5, &user_profile_t::flags>{},
        pb::ignore<&user_profile_t::runtime_handle>{},
    };
};
```

## 8. Relationship to the multi-backend doc

Per Steven's call on 2026-05-22:

- **`pb::*`** is the canonical namespace when **pb.hpp is used directly** — the common case. This doc specifies it.
- **`h5::proto::*`** stays in `tasks/h5cpp-compiler-multi-backend-architecture.md` as the namespace for the **multi-backend roof's** protobuf sub-scope (alongside `h5::sql::*`, `h5::json::*`, `h5::avro::*`).

A future cleanup task can either (a) teach the multi-backend roof to also accept `pb::*` directly, or (b) leave the two scopes distinct. No work needed now.

In practice the user-facing impact is small — anyone using pb.hpp standalone writes `[[pb::field(N)]]`; anyone using the multi-backend roof writes `[[h5::proto::field(N)]]`. The two names mean the same thing in their respective scope; they don't compete.

## 9. Gap analysis — what h5cpp-compiler#30 needs to grow

Tier-by-tier delta between today's `30-pb-producer` and this proposed surface:

### Tier 1 — partial

| Attribute | Today | Status |
|---|---|---|
| `pb::field(N)` | `pb::field=N` (string-encoded via `clang::annotate`) | ✔ Implemented; needs syntax-form migration to standard-attribute |
| `pb::field(N1, N2, ...)` (variadic for variant→oneof) | `pb::oneof_tags=N1,N2,...` (separate string-encoded annotation) | ◇ Implemented under a different name; needs consolidation |
| `pb::wire(spec)` | `pb::wire=spec` | ✔ Implemented |
| `pb::adapter(name)` | `pb::adapter=Name` | ✔ Implemented |
| Universal `pb::name(...)` | not recognized | ✘ Missing |
| Universal `pb::ignore` | not recognized; runtime `pb::ignore<>` exists in pb.hpp but the compiler doesn't emit it from annotations | ✘ Missing |
| Universal `pb::doc(...)` | not recognized | ✘ Missing |
| Universal `pb::on_missing(...)` | not recognized | ✘ Missing |

### Tier 2 — none implemented

`pb::reserved`, `pb::packed`, `pb::deprecated`, `pb::package`, `pb::oneof_name`, `pb::alias`, `pb::name_all` — all require a proper `.proto` text emitter alongside the existing `pb::meta::descriptor_t<T>` emitter. The descriptor emitter exists; the `.proto` emitter is the next major piece.

### Tier 3 / Tier 4 — none implemented

`pb::json_name`, `pb::unknown_field_set`, `pb::target_syntax`, `pb::descriptor_set_out`, `pb::service`, `pb::encode_with`/`pb::decode_with`, `pb::tier`, `pb::reject` — all deferred.

### Annotation-syntax migration (Phase B)

Self-contained refactor of `src/consumer_pb.hpp:181-244`. The existing `parse_pb_*_attr_` family wraps `clang::AnnotateAttr` lookups; the new path needs a namespace-scoped-attribute matcher that recognizes `pb::*` attributes parsed by Clang into the AST directly (no `clang::annotate` envelope). Mechanical work; would unblock dropping the `clang::annotate` form entirely.

## 10. Implementation phasing

1. **Phase 1 (smallest, parser-only)**: extend `consumer_pb.hpp` to recognize standard-attribute syntax `[[pb::field(N)]]` etc. alongside the existing `[[clang::annotate("pb::field=N")]]`. Both forms accepted during a transition window; new form preferred in docs and tests. Zero impact on emitted descriptor output. **Argument resolution uses `Expr::EvaluateAsInt(ctx)` so integer literals and enum constant references (§6) are handled by the same code path — no second pass needed.**
2. **Phase 2**: implement the universal-attribute reader (`pb::name`, `pb::ignore`, `pb::doc`, `pb::on_missing`). Today these are unsupported; recognizing them costs ~one helper per attribute and feeds the upcoming `.proto` emitter.
3. **Phase 3 (the foundation)**: add the `.proto` text emitter as a sibling to the existing descriptor emitter. Same AST walk, second producer. Drives the need for class-level attributes (`pb::package`, `pb::name`, `pb::reserved`, `pb::version`) and the universal `pb::doc` / `pb::alias` propagation.
4. **Phase 4**: tier-2 / tier-3 attributes layered on top of the `.proto` emitter. Each is small once the foundation exists.
5. **Phase 5 (C++26)**: typed-annotation form via reflection. h5cpp-compiler becomes optional for pb.hpp users on C++26 toolchains.

## 11. Open questions

1. **Implementation of universal `pb::on_missing(value)` in pb.hpp.** proto3 wire format itself has zero-defaults — there's no syntactic way to encode "default = 0.5" into the `.proto`. The annotation drives the C++ struct's *initializer* in the generated decoder shim. Need to confirm: is the shim a separate header h5cpp-compiler emits (alongside `pb::meta::descriptor_t<T>`)? Or does the decoder default-init from the C++ class's own NSDMI (non-static data member initializer)?
2. **`pb::name(...)` at class level: dataset rename or message rename?** For consistency with `h5::name` which at struct scope renames the HDF5 dataset, `pb::name` at struct scope renames the `.proto` message. Documented above. Should `pb::name` at struct scope also affect the C++ namespace of the emitted descriptor, or strictly the `.proto` schema?
3. **`pb::reserved` at field-level acceptance.** proto3 has no field-level reserved syntax — `reserved` is a message-scope clause. The proposed taxonomy says: tolerate field-level placement, aggregate into one message-level `reserved` clause on emission. Is that the right ergonomics, or should field-level `pb::reserved` be a compile-error (forcing the user to put it on the class)?
4. **`pb::reject` interaction with `pb::ignore`.** Both can appear at the field level. `pb::reject` is class-level (don't emit this type to protobuf at all); `pb::ignore` is field-level (this field, on this type, doesn't go on the wire). The distinction is clear but worth one diagnostic line in the spec.
5. **Attribute argument validation.** `[[pb::field(0)]]` is invalid (proto3 tag range starts at 1). `[[pb::field(19000)]]` falls in the protoc-reserved range `[19000, 19999]`. h5cpp-compiler should emit clean diagnostics, not let bad annotations slip through to runtime `static_assert` failures. Validation happens after enum folding (§6) — an enum value that resolves to 0 or to the reserved range is flagged the same as a raw `0` or `19000`. Worth a tier-1 acceptance criterion.
6. **`pb::version(N)` storage on the wire.** The proposed taxonomy emits it as a message-scope `option (pb.schema_version) = N;`. This requires registering a custom protobuf option (`extend google.protobuf.MessageOptions { optional uint32 schema_version = ...; }`) somewhere — pb.hpp's own well-known options file? Decide ownership before implementation.
7. **Bounded buffers in structural-type sketches.** §5 used `std::array<char, 64>` for `pb::name` to satisfy structural-type constraints. 64 chars is enough for almost every name in practice; 32 might be enough; 128 is safe. Final size before C++26 implementation.

## Sources

- `tasks/h5cpp-compiler-multi-backend-architecture.md` (workspace) — establishes the multi-backend roof and the `h5::proto::*` sub-namespace there
- `tasks/h5cpp-compiler-scatter-gather-design.md` (workspace) — tier model, universal attribute set, C++26 migration phases
- `tasks/pb-feature-coverage-and-gaps.md` (workspace) — `pb.hpp` runtime feature inventory
- `vargalabs/h5cpp-compiler#30-pb-producer` — `src/consumer_pb.hpp` (today's `clang::annotate`-form pb attribute parser)
- `vargalabs/sandbox#3-pb-map` — runtime support for `std::map<K,V>` auto-detection (informs why tier-1 stays free of an explicit map attribute)
- [Protocol Buffers Language Guide (proto3)](https://protobuf.dev/programming-guides/proto3/)
- [Protocol Buffers Style Guide](https://protobuf.dev/programming-guides/style/)
- [P2996R13 — Reflection for C++26](https://wg21.link/p2996)
- [P3394R4 — Annotations for Reflection](https://wg21.link/p3394)
