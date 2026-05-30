@page reports_compiler_h5_attribute_taxonomy h5:: Attribute Vocabulary (HDF5 Backend)

User-facing attribute set for HDF5 annotations on plain C++ structs. **Vocabulary is intentionally identical to `pb::*` where the concept overlaps** (rename, ignore, doc, version, alias, on_missing, name_all) — different namespace, same words. The HDF5-specific surface lives only under `h5::*`, with `h5::chunk` and `h5::compress` being storage-layer concerns that have no protobuf counterpart.

C++17 attribute syntax today; one-line lift to C++26 typed annotations tomorrow.

| Surface today (C++17 standard-attribute) | C++26 reflection form |
|---|---|
| `[[h5::name("on_disk")]]` | `[[=h5::name{"on_disk"}]]` |
| `[[h5::ignore]]` | `[[=h5::ignore{}]]` |
| `[[h5::chunk(256)]]` | `[[=h5::chunk{256}]]` |
| `[[h5::compress("gzip", 6)]]` | `[[=h5::compress{h5::algo::gzip, 6}]]` |
| `[[h5::doc("description")]]` | `[[=h5::doc{"description"}]]` |
| `[[h5::alias("Session")]]` | `[[=h5::alias{"Session"}]]` |
| `[[h5::version("1")]]` | `[[=h5::version{"1"}]]` |
| `[[h5::name_all("pfx_", "_sfx")]]` | `[[=h5::name_all{"pfx_", "_sfx"}]]` |
| `[[h5::on_missing("ignore")]]` | `[[=h5::on_missing{h5::missing_policy::ignore}]]` |
| `[[h5::serialize_full]]` | `[[=h5::serialize_full{}]]` |

Only syntactic shift is `(args)` → `{args}` under the `[[=...]]` form. **Names stay put.**


## 2. Universal vocabulary — same words, `h5::` namespace

These attributes use vocabulary identical to `pb::*` (defined in `tasks/h5cpp-compiler-pb-attribute-taxonomy.md` §2). They live in `h5::` so the namespace stays self-contained for HDF5-only users; a user wanting both backends writes both `[[h5::name(...)]]` and `[[pb::name(...)]]` (typically with the same string).

### Universal Tier 1 — must-have

| Attribute | Purpose | Example |
|---|---|---|
| `[[h5::name("on_disk_name")]]` | Rename a field for on-disk storage. Decouples C++ identifier from `H5Tinsert` field name. Drives the column name in HDF5 compound types and the dataset member name in VLEN layouts. | `[[h5::name("temp_K")]] double temperature;` |
| `[[h5::ignore]]` | Skip this field entirely. No `H5Tinsert` emitted; field absent from the generated `row_t` mirror and the compound type. | `[[h5::ignore]] int debug_counter;` |

### Universal Tier 2 — high value, low cost

| Attribute | Purpose | Example |
|---|---|---|
| `[[h5::doc("description")]]` | Trailing `//` comment in the generated header. Self-documenting generated code. | `[[h5::doc("nanoseconds since epoch")]] std::uint64_t ts;` |
| `[[h5::alias("Name")]]` | Alternative name for the generated namespace. The C++ template specialization still uses the real qualified type name; the alias only affects `h5::generated::Name_::row_t` and `h5::generated::Name_::compound_type()`. | `struct [[h5::alias("Session")]] session_t { ... };` |
| `[[h5::name_all("prefix", "suffix")]]` | **Class-level** naming convention. Applies prefix + field_name + suffix to every field's on-disk name. Per-field `h5::name` overrides. | `struct [[h5::name_all("sn_", "")]] sensor_t { ... };` |

### Universal Tier 3 — nice to have

| Attribute | Purpose |
|---|---|
| `[[h5::version("N")]]` | Schema version; emitted as a `// version: "N"` comment in the generated header. Pure metadata today; future h5cpp library versions may expose it through `H5Oget_comment` or custom attributes. |
| `[[h5::on_missing("error" \| "create" \| "ignore")]]` | **Class-level.** Runtime behavior when the target dataset does not exist. `"create"` (default) creates a chunked, extendable dataset. `"error"` throws `std::runtime_error`. `"ignore"` returns early from `scatter<T>` / `gather<T>`. Only affects tier-2 scatter/gather emission where the compiler emits dataset open/create logic. |

The full universal list mirrors `tasks/h5cpp-compiler-pb-attribute-taxonomy.md` §2. Any universal `pb::*` attribute not listed above has no HDF5 semantics (e.g. `pb::field(N)`, `pb::wire`, `pb::adapter` are protobuf-wire concerns).

---

## 3. HDF5-specific vocabulary — tier 1..2

### Tier 1 — must-have

Without these, the HDF5 backend either can't emit valid VLEN storage (`chunk` is mandatory for extendable datasets) or loses access to features that are core to HDF5 storage semantics (compression filters).

| Attribute | Purpose | Example |
|---|---|---|
| `[[h5::chunk(N)]]` | **Class-level.** Dataset chunk size for tier-2 scatter/gather emission. Required because VLEN datasets must be chunked in HDF5. Defaults to `64` if omitted. | `struct [[h5::chunk(256)]] session_t { ... };` |
| `[[h5::compress("gzip", level)]]` | **Class-level.** Add a compression filter to the dataset creation property list. Today only `"gzip"` is supported, emitting `H5Pset_deflate(dcpl, level)`. Future algorithms (shuffle, szip, zstd) layer in as additional string → H5P dispatch entries. | `struct [[h5::compress("gzip", 6)]] session_t { ... };` |

**`compress` algorithm inference.** If the user writes `[[h5::compress(6)]]` (integer only, no algorithm string), the compiler infers `"gzip"`. This is the common case and avoids forcing users to quote `"gzip"` for the overwhelming majority of usage.

### Tier 2 — high value, low cost

| Attribute | Purpose | Example |
|---|---|---|
| `[[h5::serialize_full]]` | **Class-level.** Force `register_struct<T>` (POD compound-type) emission even when the struct contains `std::vector` or `std::string` fields. Non-POD fields are implicitly skipped (as if `[[h5::ignore]]`); the resulting compound type has size `sizeof(T)` and no members, effectively serializing the full struct memory layout as an opaque blob. Useful when the user wants raw-byte round-trip fidelity and accepts platform-dependent memory layout. | `struct [[h5::serialize_full]] raw_session_t { std::uint64_t ts; std::vector<double> v; };` |

---

## 4. Class-level vs field-level scoping

Same pattern as `pb::` for protobuf: class-level attributes set defaults; field-level overrides them.

```cpp
struct [[h5::doc("Sensor session data"), h5::alias("Session"), h5::version("1"),  h5::chunk(1024),
        h5::compress("gzip", 9), h5::name_all("sn_", ""),  h5::on_missing("ignore")]]
session_t {
    [[h5::doc("nanoseconds since epoch")]] std::uint64_t timestamp_ns;
    [[h5::name("lbl")]] std::string label;
    [[h5::ignore]] std::vector<int> debug_samples;
    std::vector<double> readings;
};
```

**Field-level `h5::name` overrides `h5::name_all`.** In the example above:
- `timestamp_ns` → on-disk name `sn_timestamp_ns` (from `name_all`)
- `label` → on-disk name `lbl` (from `name`, overriding `name_all`)
- `debug_samples` → skipped entirely (from `ignore`)
- `readings` → on-disk name `sn_readings` (from `name_all`)

**Class-level `h5::doc`, `h5::alias`, `h5::version`** are emitted as C++ comments in the generated header. They do not affect HDF5 API calls.

**Class-level `h5::chunk`, `h5::compress`, `h5::on_missing`** affect the `scatter<T>` / `gather<T>` specializations emitted for tier-2 types. They have no effect on tier-1 `register_struct<T>()` emission because `register_struct` only creates a compound type, not a dataset.

---

## 5. Tier classification and attribute interaction

The compiler classifies every matched struct into one of three tiers before emission:

| Tier | Criteria | Emission path | Attributes that apply |
|---|---|---|---|
| **Tier 1 (POD)** | All fields are builtin scalars, enums, fixed-size arrays, or nested POD structs. No `std::vector`, no `std::string`. | `register_struct<T>()` — `H5Tcreate(H5T_COMPOUND, sizeof(T))` + `H5Tinsert` per field. | `name`, `ignore`, `name_all`, `doc`, `alias`, `version`, `serialize_full` (no-op on already-POD types) |
| **Tier 2 (scatter)** | Contains at least one `std::vector<T>` (where `T` is scalar/enum) or `std::string` field; all other fields are POD-compatible. | `scatter<T>()` + `gather<T>()` — `row_t` mirror with `hvl_t`, chunked extendable dataset, VLEN compound type. | **All attributes** |
| **Invalid** | Contains unsupported field types (e.g. `std::vector<std::vector<T>>`, pointers, non-POD nested classes without `serialize_full`). | Skipped — no emission. | None |

**`serialize_full` escape hatch.** When present at class scope, `serialize_full` bypasses tier classification and forces tier-1 emission. Non-POD fields are silently skipped. The emitted compound type has `sizeof(T)` but zero members, making it an opaque blob:

```cpp
// Input
struct [[h5::serialize_full]] mixed_t {
    std::uint64_t timestamp_ns;
    std::string   label;         // skipped
    std::vector<double> samples; // skipped
};

// Emitted output by h5cpp-compiler
namespace h5 {
template<> hid_t inline register_struct<mixed_t>(){
    hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (mixed_t));
    H5Tinsert(ct_00, "timestamp_ns", HOFFSET(mixed_t,timestamp_ns),H5T_NATIVE_ULLONG);
    // label and samples omitted
    return ct_00;
};
}
H5CPP_REGISTER_STRUCT(mixed_t);
```

---

## 6. C++26 reflection migration

C++26 ships P2996R13 (Reflection for C++26) and P3394R4 (Annotations for Reflection). Under the typed annotation form, the same names above are constructor calls of structural-type values.

Implementation sketch for the `h5::` value types (each must be a structural type — `final` struct, public data members, `constexpr` constructors):

```cpp
namespace h5 {

    struct name {
        std::array<char, 64> str;
        std::size_t          len;
        constexpr name(std::string_view s) : str{}, len{s.size()} {
            for (std::size_t i = 0; i < s.size() && i < 64; ++i) str[i] = s[i];
        }
    };

    struct ignore {};

    struct chunk {
        std::uint32_t value;
        constexpr chunk(std::uint32_t n) : value{n} {}
    };

    enum class algo { gzip };
    struct compress {
        algo      algorithm;
        std::uint32_t level;
        constexpr compress(algo a, std::uint32_t l) : algorithm{a}, level{l} {}
        // infer gzip when only level is provided
        constexpr compress(std::uint32_t l) : algorithm{algo::gzip}, level{l} {}
    };

    struct doc    { /* same shape as name, longer buffer */ };
    struct alias  { /* same shape as name */ };
    struct version{ /* same shape as name */ };

    struct name_all {
        std::array<char, 64> prefix;
        std::array<char, 64> suffix;
        std::size_t          pre_len, suf_len;
        constexpr name_all(std::string_view p, std::string_view s) : prefix{}, suffix{},
            pre_len{p.size()}, suf_len{s.size()} {
            for (std::size_t i = 0; i < p.size() && i < 64; ++i) prefix[i] = p[i];
            for (std::size_t i = 0; i < s.size() && i < 64; ++i) suffix[i] = s[i];
        }
        constexpr name_all(std::string_view p) : name_all(p, "") {}
    };

    enum class missing_policy { error, create, ignore };
    struct on_missing { missing_policy value; };

    struct serialize_full {};

} // namespace h5
```

Under C++17 attribute syntax `[[h5::chunk(256)]]`, h5cpp-compiler's Clang-Tooling backend parses the namespace-scoped attribute directly (today via the rewriter → `clang::annotate` envelope). The implementation work to drop the rewriter is mechanical — a single namespace-aware attribute matcher swapped in for the current per-string parser.

Under C++26 typed annotations `[[=h5::chunk{256}]]`, the value gets reflected via `std::meta::annotations_of(^member)` and read at constexpr time from inside `h5cpp` itself — h5cpp-compiler becomes an optional convenience, not a required tool.

### Structural-type prerequisite

C++26 annotations require the annotated value to be of *structural type* (no virtual functions, no mutable, no private non-static data, literal type for constexpr construction). The sketches above are designed to satisfy this — fixed-size `std::array` buffers instead of `std::string`, public data members, `constexpr` constructors.

---

## 7. Worked example — mixed sensor session

### Input (user source)

```cpp
#include <string>
#include <vector>

namespace sn::sensor {
    struct [[h5::doc("Sensor session with VLEN payload"), h5::alias("Session"), h5::version("2"),
            h5::chunk(256),  h5::compress("gzip", 6), h5::name_all("sn_", ""), h5::on_missing("ignore")]]
    session_t {
        [[h5::doc("capture timestamp")]] std::uint64_t timestamp_ns;
        [[h5::name("lbl")]] std::string label;
        [[h5::ignore]] std::vector<int> debug_samples;
        std::vector<double> readings;
    };
} // namespace sn::sensor
```

### Emitted output (tier-2 scatter/gather)

```cpp
#pragma once

#include <hdf5.h>
#include <h5cpp/all>

// doc: "Sensor session with VLEN payload"
// alias: "Session"
// version: "2"
namespace h5::generated::Session_ {
    struct row_t {
        std::uint64_t timestamp_ns;
        hvl_t    label;
        hvl_t    readings;
    };

    inline hid_t compound_type() {
        static const hid_t ct = []{
            hid_t v_label = H5Tcopy(H5T_C_S1);
            H5Tset_size(v_label, H5T_VARIABLE);
            hid_t v_readings = H5Tvlen_create(H5T_NATIVE_DOUBLE);
            hid_t ct = H5Tcreate(H5T_COMPOUND, sizeof(row_t));
            H5Tinsert(ct, "sn_timestamp_ns", HOFFSET(row_t, timestamp_ns), H5T_NATIVE_ULLONG);
            H5Tinsert(ct, "lbl", HOFFSET(row_t, label), v_label);
            H5Tinsert(ct, "sn_readings", HOFFSET(row_t, readings), v_readings);
            return ct;
        }();
        return ct;
    }
} // namespace h5::generated::Session_

namespace h5 {
    template<> inline h5::ds_t scatter<sn::sensor::session_t>(
        hid_t fd, const std::string& path, const sn::sensor::session_t& obj) {
        using namespace ::h5::generated::Session_;
        h5::ds_t ds;
        h5::mute();
        bool exists = H5Lexists(fd, path.c_str(), H5P_DEFAULT) > 0;
        h5::unmute();
        if (!exists) {
            return ds;
        }
        ds = h5::open(fd, path, h5::default_dapl);
        // ... write row via h5::detail::write_one_row ...
    }
} // namespace h5

namespace h5 {
    template<> inline void gather<sn::sensor::session_t>(
        hid_t fd, const std::string& path, sn::sensor::session_t& obj) {
        using namespace ::h5::generated::Session_;
        h5::mute();
        bool exists = H5Lexists(fd, path.c_str(), H5P_DEFAULT) > 0;
        h5::unmute();
        if (!exists) {
            return;
        }
        h5::ds_t ds = h5::open(fd, path, h5::default_dapl);
        // ... read row via h5::detail::read_one_row ...
    }
} // namespace h5

H5CPP_REGISTER_SCATTER(sn::sensor::session_t);
```

Observations from the emitted code:
- `timestamp_ns` → `sn_timestamp_ns` (`name_all` applied)
- `label` → `lbl` (`name` overrides `name_all`)
- `debug_samples` → absent entirely (`ignore`)
- `readings` → `sn_readings` (`name_all` applied)
- `on_missing("ignore")` → early return in both `scatter` and `gather` when dataset absent
- `alias("Session")` → generated namespace is `h5::generated::Session_` instead of `sn__sensor__session_t_`
- `chunk(256)` + `compress("gzip", 6)` → emitted in the dataset-creation branch (not shown above for brevity)

---

## 8. Relationship to the multi-backend doc

Per Steven's architecture:

- **`h5::*`** is the canonical namespace when **h5cpp is used directly** — the common case. This doc specifies it.
- **`h5::proto::*`** stays in `tasks/h5cpp-compiler-multi-backend-architecture.md` as the namespace for the **multi-backend roof's** protobuf sub-scope.

A user writing pure HDF5 code uses `[[h5::name(...)]]`, `[[h5::chunk(...)]]`, etc. A user writing multi-backend code uses `[[h5::proto::field(N)]]` for protobuf concerns and `[[h5::name(...)]]` for HDF5 concerns on the same struct. The two scopes don't compete.

---

## 9. Gap analysis — what exists today vs. what the taxonomy specifies

### Tier 1 — fully implemented

| Attribute | Today | Status |
|---|---|---|
| `h5::name("x")` | Rewriter + consumer + producer wired for tier-1 and tier-2 | ✔ Complete |
| `h5::ignore` | Rewriter + consumer + producer wired for tier-1 and tier-2 | ✔ Complete |
| `h5::chunk(N)` | Class-level int read; passed to `scatter_type_impl`; emits `H5Pset_chunk` | ✔ Complete |
| `h5::compress("gzip", level)` | Class-level string+int read; emits `H5Pset_deflate` | ✔ Complete |

### Tier 2 — fully implemented

| Attribute | Today | Status |
|---|---|---|
| `h5::doc("...")` | Class-level string read; emitted as `// doc: "..."` comment | ✔ Complete |
| `h5::alias("Name")` | Class-level string read; emitted as comment; drives generated namespace name | ✔ Complete |
| `h5::version("N")` | Class-level string read; emitted as `// version: "N"` comment | ✔ Complete |
| `h5::name_all("pre", "suf")` | Class-level strings read; applied to all field on-disk names; per-field `h5::name` overrides | ✔ Complete |
| `h5::on_missing("error" \| "ignore" \| "create")` | Class-level string read; modifies `scatter`/`gather` exists logic | ✔ Complete |
| `h5::serialize_full` | Class-level flag; forces tier-1 emission; non-POD fields implicitly skipped | ✔ Complete |

### Tier 3 / future — none requested

No additional attributes are currently in scope for issue #32. Potential future additions (not designed, not requested):

| Attribute | Potential Purpose |
|---|---|
| `h5::shuffle` | Enable byte-shuffle filter before compression (common pre-filter for `h5::compress`). |
| `h5::fletcher32` | Enable Fletcher32 checksum on the dataset. |
| `h5::scaleoffset(int, int)` | Enable scale-offset filter for lossy compression of floating-point data. |
| `h5::dimension_labels("time", "channel")` | Label dataset dimensions for self-describing data cubes. |
| `h5::reject` | Compile-error if the HDF5 producer is asked to emit this type. Counterpart of `pb::reject`. |
| `h5::tier(N)` | Escape hatch for the tier classifier — force a class into a specific tier when auto-detection is wrong. |

---

## 10. Implementation phasing (historical — all complete)

1. **Phase 1**: Source rewriter (`h5_attr_translator.hpp`) + AST reader (`h5_attr_reader.hpp`). Rewrites `[[h5::xxx(...)]]` → `[[clang::annotate("h5::xxx", ...)]]`. Generic enough to share across backends.
2. **Phase 2**: Consumer extension (`consumer.hpp`) — tier classification (`utils::determine_tier`), attribute reading, routing to correct emission path.
3. **Phase 3**: Producer extension (`producer_h5.hpp`) — `register_struct` for tier-1, `scatter_type` / `gather_type` for tier-2. Wired `name`, `ignore`, `chunk`, `compress`.
4. **Phase 4**: Metadata attributes — `doc`, `alias`, `version`, `name_all`, `on_missing`, `serialize_full`. All wired into both tier-1 and tier-2 paths.
5. **Phase 5 (C++26)**: Typed-annotation form via reflection. h5cpp-compiler becomes optional for h5cpp users on C++26 toolchains.
