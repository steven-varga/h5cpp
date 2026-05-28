@page reports_reflection_cpp26_roadmap h5cpp + C++26 Reflection Roadmap

**Date:** 2026-05-21
**Authors:** Steven Varga, Winston (Architecture)
**Status:** Strategy + prototyping plan; needs validation pass on Compiler Explorer before implementation
**Repos:** vargaconsulting/h5cpp, vargaconsulting/h5cpp-compiler

## Vision — the pitch

> **h5cpp covers you for tomorrow with C++26 reflection-based header-only persistence; here's what you can do today with h5cpp-compiler — backward-compatible to C++17. A solid investment.**

Two vehicles, one user-facing surface:

- **Tomorrow (C++26):** add a reflection-based header-only path to h5cpp itself. Uses `std::meta::*` (P2996) to walk user types at constexpr time; reads `[[=h5::name{...}]]` annotations (P3394) for customization. **No external tool.** State of the art among HDF5 C++ libraries.
- **Today (C++17/20/23):** the existing h5cpp-compiler keeps doing the same work via Clang Tooling. Same attribute surface (token form), same generated output shape, same library dispatch.

The user adopts h5cpp today on whatever compiler they have. When they move to GCC 16.1+ (or any C++26 compiler), the same code keeps working — they can opt into the no-external-tool path at their pace, or never. **Investment-grade tooling**: their code doesn't have to change as the C++ standard advances.

## Strategic plan

### Thread 1 — Add reflection support to h5cpp (forward-looking, header-only)

Build a constexpr reflection layer inside h5cpp itself. Concretely:

- New header(s) under `h5cpp/reflection/` (or similar) that walk a type via `std::meta::nonstatic_data_members_of`, `std::meta::type_of`, `std::meta::offset_of`, `std::meta::identifier_of`, `std::meta::annotations_of`
- Lazy-built compound type id per `T`: `template<class T> hid_t compound_type_for()` returning a `static const hid_t` constructed once
- Tier-1 first: pure PODs → `H5T_COMPOUND` of natives, identical on-disk shape to what h5cpp-compiler produces today
- Tier-2 next: walk vector/string/etc. fields, build `H5T_VLEN` members + `hvl_t` scatter at constexpr time
- Annotation handles defined in the `h5::` namespace using **bare names** (no `_t` suffix on the user-facing surface): `name`, `ignore`, `on_missing`, `compress`, `tag`, `doc`, etc. — match the existing attribute surface from `h5cpp-compiler-scatter-gather-design.md`. The `_t` suffix only appears in contexts that genuinely name a type as a type (template parameters, traits, `decltype`).
- **Reuse existing property types as annotations.** Several handles are already in h5cpp's runtime API as property bag objects: `h5::chunk`, `h5::max_dims`, `h5::offset`, `h5::stride`, `h5::block`, `h5::count`, plus the filter/compression family. Under C++26 annotations they become attachable to declarations (`[[=h5::chunk{1024}]] std::vector<double> samples;`) using the *same constructors* users already know from the call-site form (`h5::write(fd, "ds", obj, h5::chunk{1024} | h5::gzip{8})`). One vocabulary, two usage sites — only the syntax envelope changes (`(...)` at call sites, `{...}` inside `[[=...]]`).

**Result:** A user with GCC 16.1+ can `#include <h5cpp/all>`, define a POD, call `h5::write(fd, "ds", obj)`, and have the compound type built automatically at first use — no Clang Tooling step, no `generated.h` file, no makefile glue.

### Thread 2 — Keep h5cpp-compiler in lockstep (back-port / parity)

The external Clang Tooling path stays the C++17/20/23 vehicle. After Thread 1 lands, port the same conceptual code generator back into h5cpp-compiler so that:

- The header emitted by h5cpp-compiler is **functionally identical** to what the reflection path would produce at runtime
- Users on older compilers get the same on-disk format, same dispatch path, same library hooks
- The two paths are interchangeable from the user's perspective

This is the "back-port" pillar: whatever the reflection vehicle does, the external tool does too, just earlier (build time vs first-use time).

### Thread 3 — Messaging / positioning

The investment narrative needs three lines on the README and in any pitch deck:

1. **Today (C++17):** include `<h5cpp/all>`, install `h5cpp-compiler`, get automatic struct persistence
2. **Tomorrow (C++26):** drop `h5cpp-compiler`, include `<h5cpp/all>`, get the same thing
3. **Your code doesn't change.** Annotations stay; call sites stay; on-disk format stays. The only thing that changes is whether a separate binary runs at build time

This is the "solid investment" message. h5cpp is the persistence layer you adopt once and ride forward.

### Thread 4 — Status of nearby competitors (state of the art claim)

Verified 2026-05-21:
- **HighFive (BlueBrain, 2015)** — manual struct mapping; no reflection plan visible
- **HDF Group official C++ API** — manual `H5Tinsert` with `HOFFSET`; no reflection plan visible
- **rootcling (CERN ROOT 6, 2014)** — Clang Tooling codegen for `.root` files; not HDF5-targeted
- **h5cpp + h5cpp-compiler** — only HDF5 library with auto-generated persistence; adding the reflection path makes it the **first HDF5 library with both header-only reflection codegen AND external-tool fallback**

The state-of-the-art claim holds for the HDF5 niche specifically.

## Compiler installation tutorial

**Current machine state (verified earlier in this session):** GCC 14.2.0 default on Linux Mint 22.2 / Ubuntu 24.04 base. C++26 reflection requires **GCC 16.1+** (or Bloomberg's clang-p2996 fork). Four paths in order of speed-to-result:

### Path 1 — Compiler Explorer (fastest, zero install)

Use this for **syntax validation only**. No HDF5 link possible; pure language-level exploration.

1. Open <https://godbolt.org/>
2. In the compiler dropdown, select **GCC 16.1** (or **gcc trunk**) — the dropdown lists every GCC version supported
3. In compiler flags, add: `-std=c++26 -freflection`
4. For Bloomberg's clang fork (more bleeding-edge features), select **clang-p2996** instead and use `-freflection-latest`
5. Paste a tiny reflection example (see "Validation snippet" below)
6. Verify compile output

**Time:** 5 minutes. **Limitation:** no libhdf5; only validates that the syntax and reflection API exist and behave as expected.

### Path 2 — Docker container (recommended for local dev)

Clean isolation; no global system change. Two flavors:

#### 2a — Official GCC image

```bash
# Pull GCC 16.1
docker pull gcc:16

# Run a shell with your h5cpp checkout mounted
docker run --rm -it \
  -v /home/steven/projects/h5cpp:/h5cpp \
  -v /home/steven/projects/vargalabs-workspace:/ws \
  gcc:16 bash

# Inside the container — install HDF5 dev:
apt-get update && apt-get install -y libhdf5-dev pkg-config

# Compile a reflection example:
g++ -std=c++26 -freflection -I/h5cpp $(pkg-config --cflags hdf5) \
    test.cpp -o test $(pkg-config --libs hdf5)
```

**Verify the image tag exists**: `docker pull gcc:16` should succeed; if not, try `gcc:16.1` or `gcc:latest` and check `g++ --version`.

#### 2b — Bloomberg clang-p2996 image (for `-freflection-latest`)

The Bloomberg fork ships a Dockerfile in the repo. From their README:

```bash
git clone https://github.com/bloomberg/clang-p2996.git
cd clang-p2996
# Build the image (slow — clones LLVM, builds clang)
docker build -t clang-p2996 -f Dockerfile .
docker run --rm -it -v $PWD:/work clang-p2996 bash
```

**Time:** image pull ~5 min (option 2a) or build ~1-2 hr (option 2b). **Result:** full end-to-end including HDF5 link.

### Path 3 — Ubuntu PPA install (system-wide)

**Caveat:** verify the PPA hosts gcc-16 before running these. As of May 2026 the `ubuntu-toolchain-r/test` PPA typically tracks the latest GCC, but check `apt-cache search gcc-16` after adding the PPA to confirm.

```bash
# Add the toolchain PPA
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update

# Check what's available
apt-cache search '^gcc-1[6-9]$'

# Install (assuming gcc-16 is present)
sudo apt install gcc-16 g++-16

# Use directly:
g++-16 --version
g++-16 -std=c++26 -freflection ...

# Or set as default via update-alternatives (optional)
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-16 60
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-16 60
```

**Time:** ~10 min if the PPA has it; fail-fast otherwise.

### Path 4 — Build GCC 16.1 from source (last resort)

Only if Paths 1-3 don't work and you need a specific configuration:

```bash
# Download release tarball
wget https://ftp.gnu.org/gnu/gcc/gcc-16.1.0/gcc-16.1.0.tar.xz
tar -xJf gcc-16.1.0.tar.xz
cd gcc-16.1.0

# Get prerequisites (downloads gmp, mpfr, mpc, isl)
./contrib/download_prerequisites

# Configure + build in a separate directory (out-of-tree build is GCC convention)
mkdir ../build && cd ../build
../gcc-16.1.0/configure --prefix=$HOME/opt/gcc-16.1 \
    --enable-languages=c,c++ --disable-multilib
make -j$(nproc)
make install

# Use from $HOME/opt/gcc-16.1/bin/g++ — add to PATH if desired
```

**Time:** 1-2 hours on a modern multicore machine.

### Validation snippet (for Compiler Explorer)

Paste this into godbolt with GCC 16.1 + `-std=c++26 -freflection` to confirm reflection works. **Note the header path and API names may differ slightly in the shipped GCC 16.1 release** — verify against [GCC's C++26 status page](https://gcc.gnu.org/projects/cxx-status.html) before relying on them:

```cpp
// API names below match P2996R13 as I last reviewed; verify against
// the exact headers shipped in the GCC 16.1 you install.
#include <meta>          // header name TBD — may be <experimental/meta>
#include <iostream>

struct cell_t {
    [[=h5::name{"id"}]]      unsigned long identifier;
    [[=h5::doc{"position"}]] double         axes[3];
                             float          temperature;
};

namespace h5 {
    // Underlying types named with _t internally if desired,
    // but the user-facing handle is the bare name.
    struct name { const char* on_disk; };
    struct doc  { const char* description; };
}

int main() {
    constexpr auto T = ^cell_t;
    template for (constexpr auto field : std::meta::nonstatic_data_members_of(T)) {
        std::cout << std::meta::identifier_of(field) << '\n';
        // Check for annotations:
        for (constexpr auto ann : std::meta::annotations_of(field)) {
            // ... extract typed value via std::meta::extract<T>(ann) or similar
        }
    }
}
```


┌──────────────────────────────┬──────────────────────────────────────────────────────────────────────────────────────────────────────────┬────────────────────────────────────────────┐
│           Category           │                                                  Names                                                   │                   Status                   │
├──────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────────────────────┼────────────────────────────────────────────┤
│ Pre-existing in h5:: (reused │ chunk_t, max_dims_t, current_dims_t, offset_t, stride_t, block_t, count_t, filter_t, filter_chain_t,     │ ✔ Reuse — same constructor, two contexts  │
│  as annotations)             │ pipeline_t                                                                                               │ (call site + annotation)                   │
├──────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────────────────────┼────────────────────────────────────────────┤
│ New annotations introduced   │ name_t, ignore_t, doc_t, tag_t, alias_t, serialize_full_t, default_t, compress_t, fixed_string_t,        │ ✔ No collision per grep against           │
│ (verified clear)             │ storage_type_t, version_t, inline_fields_t, name_all_t, upgrade_from_t, tier_t, reject_t                 │ h5cpp/h5cpp/*.hpp                          │
└──────────────────────────────┴──────────────────────────────────────────────────────────────────────────────────────────────────────────┴────────────────────────────────────────────┘




If this compiles and prints `identifier`, `axes`, `temperature` (the C++ field names), reflection works. If it also surfaces the annotations (the `[[=h5::name{...}]]` values), then we have everything we need for Thread 1.

## Prototyping plan (after compiler is installed)

### Milestone 1 — Validate reflection on a POD (tier 1)
- Write `compound_type_for<cell_t>()` using reflection
- Build the `H5T_COMPOUND` at runtime by walking fields
- Compare the resulting compound type with what h5cpp-compiler would emit for the same POD
- **Success criteria:** identical on-disk layout

### Milestone 2 — Validate annotation reading
- Define `h5::name`, `h5::ignore` annotation handles (bare names; underlying type may be aliased internally)
- Tag a POD's fields with `[[=h5::name{"x"}]]`
- Reflection walker reads the annotation and uses the on-disk name instead of the C++ identifier
- **Success criteria:** `h5dump -H` shows the renamed field

### Milestone 3 — Round-trip end to end
- Reflection-built compound type → `H5Dwrite(POD bytes)` → `H5Dread` → byte-equal
- **Success criteria:** parity with current h5cpp tier-1 output

### Milestone 4 — Tier 2 (vector field via reflection)
- Walk a struct with `std::vector<double>` field
- Build `H5T_VLEN`-bearing compound row, populate `hvl_t.p` with `vec.data()`
- **Success criteria:** zero-copy write of variable-length data via reflection

### Milestone 5 — Back-port equivalence
- Verify that h5cpp-compiler (Clang Tooling path) emits a header that produces byte-identical HDF5 output to the reflection path
- **Success criteria:** `diff <(reflection_run) <(compiler_run)` is empty for both tier 1 and tier 2 datasets

## Phased rollout

| Phase | Scope | Duration estimate |
|---|---|---|
| **P0 — Validation** | Compiler Explorer + Docker; confirm syntax + API; Milestones 1-2 | 1-2 days |
| **P1 — Reflection tier 1 in h5cpp** | Header-only reflection layer for POD types; Milestone 3 | 1-2 weeks |
| **P2 — Reflection tier 2 in h5cpp** | Vector / string fields via VLEN; Milestone 4 | 2-3 weeks |
| **P3 — h5cpp-compiler back-port parity** | Verify byte-equivalence; Milestone 5 | 1 week |
| **P4 — Documentation + messaging** | README rewrite; "tomorrow / today" framing; example walkthroughs | 1-2 weeks |
| **P5 — Tier 3 + tier 4 in both vehicles** | Spine + leaves; variant via `[[h5::serialize_full]]` (or `[[=h5::serialize_full{}]]` under C++26) | 4-6 weeks |

## Risks and open questions

1. **Compile-time cost of reflection at scale.** Walking large structs with many fields via constexpr reflection can balloon compile time. Need to measure on a non-trivial struct (~50 fields) and compare with the external-tool path.
2. **`std::meta::*` API stability in GCC 16.1.** Early implementations may diverge from final P2996R13 in small ways. Need to track the GCC bug tracker for reflection-related issues.
3. **Annotation argument restrictions.** P3394 requires structural types only. Make sure all annotation types in `h5::` satisfy structural-type constraints (no virtual functions, no mutable, no private non-static data, literal type for constexpr construction). Existing applicator types like `h5::chunk` (= `aprop_t<...>`) need verification — they look structural by design but `prop_t`'s details and constructor `constexpr`-ness need a check on GCC 16.1.
4. **HDF5 errors in constexpr context.** The reflection path will build compound types at runtime (first use), so HDF5 errors are runtime errors. Same as the external tool. Not a regression but worth flagging.
5. **Documentation surface doubles.** Two vehicles means two install paths, two debugging stories. Mitigation: present them as one unified API with two delivery mechanisms; document the API once, mention vehicles separately.
6. **Adoption pacing.** C++26 reflection landed April 2026 in GCC; widespread production use will take 1-2 years. The back-port to h5cpp-compiler is what gives users the "today" answer during that window.
7. **MPI compatibility unchanged.** Reflection doesn't change the underlying HDF5 mechanics. Tier-1 stays ✔ MPI; tier-2 stays ◇; tier-3 stays ✘. Reflection is a delivery mechanism, not a performance feature.
8. **Annotation / call-site composition policy is macro-controlled.** When a field carries layout annotations and the user also passes call-site properties, the policy is set project-wide via `H5CPP_LAYOUT_CASCADE` (opt-in cascade with call-site override) or unset (strict default, compile error on same-property duplication). This mirrors the existing `H5CPP_CONVERSION_IMPLICIT` toggle. Both vehicles (reflection-based and Clang Tooling-based) must honour the same macro. See `h5cpp-compiler-scatter-gather-design.md` § "Annotation + call-site composition (macro-controlled)" for the full spec.

## Open follow-up tasks

- [ ] Validate exact `std::meta::*` API on Compiler Explorer (header name, query function signatures)
- [ ] Determine if GCC 16.1 supports `template for` (P1306 / "expansion statements") or if a workaround is needed
- [ ] Verify Ubuntu PPA `ubuntu-toolchain-r/test` actually hosts `gcc-16` as of May 2026
- [ ] Confirm Bloomberg `clang-p2996` Dockerfile builds clean on current host
- [ ] Sketch the annotation handle list (bare names) with full constructor signatures; confirm each is a structural type per C++26 rules
- [ ] Decide whether to ship reflection layer in main h5cpp repo or a sibling `h5cpp-reflection` repo

## Sources (verified 2026-05-21)

- [GCC 16.1 released: C++26 reflection / contracts / safety hardening — isocpp.org](https://isocpp.org/blog/2026/04/gcc-16.1)
- [Reflection for C++26 — P2996R13 (final)](https://isocpp.org/files/papers/P2996R13.html)
- [Annotations for Reflection — P3394R4 (final)](https://isocpp.org/files/papers/P3394R4.html)
- [Reflection in C++26 (P2996) — Learn Modern C++ tutorial](https://learnmoderncpp.com/2025/07/31/reflection-in-c26-p2996/)
- [bloomberg/clang-p2996 — Experimental clang fork with P2996 support](https://github.com/bloomberg/clang-p2996)
- [C++ Standards Support in GCC](https://gcc.gnu.org/projects/cxx-status.html)
- [Compiler support for C++26 — cppreference](https://en.cppreference.com/cpp/compiler_support/26)
- [C++26 is done! — Herb Sutter trip report, March 2026](https://herbsutter.com/2026/03/29/c26-is-done-trip-report-march-2026-iso-c-standards-meeting-london-croydon-uk/)
- [C++26 P2996 Reflection Support — Glaze documentation](https://stephenberry.github.io/glaze/p2996-reflection/)
- [C++ reflection (P2996) and moc — Qt Wiki](https://wiki.qt.io/C++_reflection_(P2996)_and_moc)

## Related documents in this workspace

- `tasks/h5cpp-compiler-scatter-gather-design.md` — the codegen design that this roadmap extends; contains the attribute surface that both vehicles share
- `tasks/h5cpp-compiler-prior-art-survey.md` — verified prior art including the lineage from 2013 to today
- `tasks/h5cpp-compiler-ai-roadmap` (memory pointer) — multi-format backend plan (protobuf / JSON / SQL / Avro) that runs alongside this reflection work
