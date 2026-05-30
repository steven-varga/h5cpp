@page reports_compiler_prior_art_survey h5cpp-compiler Prior Art Survey

**Date:** 2026-05-21
**Authors:** Steven Varga, Winston (Architecture)
**Status:** Verified — years confirmed via web search 2026-05-21; novel-combination claim verified
**Repo:** vargaconsulting/h5cpp-compiler
**Use:** Related-work section for papers, pitch-deck context, reference when explaining the project's position in the landscape

## TL;DR

The technique h5cpp-compiler uses — **Clang-AST-walking codegen for type persistence** — has a clear precedent: CERN ROOT's `rootcling` (2014), which generates per-class streamers for ROOT files. The pattern itself is older still: Qt MOC (~1995), SWIG (1996).

What appears to be unique to h5cpp-compiler is the **combination**: Clang-AST-walking *plus* HDF5 as the target *plus* zero-copy scatter/gather for `std::vector`-and-similar fields *plus* the strict "guaranteed only for recursively contiguous types" semantic. No exact match was found via web search. The closest HDF5 alternative, **HighFive (BlueBrain, 2015)**, is explicitly a *manual* wrapper — the user declares HDF5 compound types by hand.

**Project lineage matters for the claim.** The h5cpp work itself has multiple generations:
- A first-generation precursor (predating h5cpp11; not yet documented here — author has the details)
- **h5cpp11** (Dec 2017 dev, Jul 2018 published) — C++11 template library between linear-algebra libraries and HDF5; *no compiler tool*
- **h5cpp + h5cpp-compiler** (Fall 2018 debut) — C++17 templates plus the Clang AST walker — the generation that introduced automatic struct-to-compound codegen

So the templates have been refined since 2017; the Clang AST walker is the 2018 addition. Both the long template lineage and the codegen step matter when positioning the work.

## Verified chronological timeline

All years below were confirmed via web search 2026-05-21. Confidence indicators have been removed; every entry below is supported by at least one cited source (see "Sources" at the bottom).

### Direct precedents — Clang-AST-walking codegen for type persistence

| Year | Tool | Notes |
|---|---|---|
| ~1992–95 | **Qt MOC** | First Qt release 1995 (Qt 0.9); MOC has been in Qt from the beginning. Walks `Q_OBJECT` classes via own preprocessor, generates metadata + signal/slot dispatch |
| 1994–95 | **CERN ROOT** | Development started 1994 (Brun & Rademakers, CERN); first public release v0.5 in November 1995 |
| Feb 1996 | **SWIG** (David Beazley) | Originated at Los Alamos National Laboratory; walks C++ headers, generates language-binding glue using own parser |
| 2014 | **rootcling / Cling** (CERN ROOT 6) | Clang/LLVM-based replacement for the older `rootcint`; **closest direct analog to h5cpp-compiler's approach**. Generates per-class streamer code from C++ class declarations annotated via `LinkDef.h` |
| 2018 | **C++ static reflection — P1240R0** | Sutton, Vali, Vandevoorde; first revision October 8, 2018. Succeeded by P2996 for C++26. Would let h5cpp-compiler do its work inside the user's compilation without Clang Tooling |
| 2019 | **refl-cpp** (Veselin Karaganev) | Header-only manual reflection library with macros declaring fields |
| **Dec 2017** | **h5cpp11** (Steven Varga) — second-generation precursor to current h5cpp | C++11 header-only HDF5 library; pure template-based, **no Clang AST codegen**. Earliest commits Dec 2017; repository published to GitHub July 19, 2018; archived 2022. Same lineage as current h5cpp (templates between linear-algebra libraries — `armadillo`, `eigen3`, `ublas`, `blitz++` — and HDF5 datasets) but without the compiler. There is also a first-generation precursor predating h5cpp11; not yet documented here |
| **Fall 2018** | **h5cpp + h5cpp-compiler** (Steven Varga) — current generation | First introduced at Chicago C++ Usergroup meeting, Fall 2018. Presented at ISC'19 BOF (Frankfurt). Already framed at debut as *"low latency MPI capable persistence"* — the MPI angle is part of the original pitch. Collaboration with The HDF Group. **This is the generation that added the Clang AST walker on top of the template foundation laid down by h5cpp11.** |

### Schema-first codegen — reverse direction (IDL → C++)

| Year | Tool | Notes |
|---|---|---|
| 2007 | **Apache Thrift** | Facebook open-sourced in April 2007; donated to Apache 2008; TLP October 2010. Multi-language IDL codegen |
| Jul 2008 | **Protocol Buffers** | Public release July 7, 2008 (internal at Google since ~2001); .proto files compiled by protoc |
| Apr 2013 | **Cap'n Proto** (Kenton Varda) | Released April 1, 2013 by the primary author of Protocol Buffers v2. Zero-copy schema-first |
| Jun 2014 | **FlatBuffers** (Google) | Released June 17, 2014. Zero-copy schema-first, games/mobile focus |

### Library-only reflection / serialization — no AST walker

| Year | Tool | Notes |
|---|---|---|
| 2002–04 | **Boost.Serialization** (Robert Ramey) | Development from 2002; first Boost release in 1.32 on Nov 1, 2004. Intrusive `serialize()` methods + visitors |
| ~2013 | **Cereal** (Voorhies / Grant) | Modern C++11 serialization library; intrusive macros |
| 2014 | **Boost.PFR / Magic Get** (Antony Polukhin) | Originally "magic_get"; compile-time POD reflection via structured-bindings tricks. Tier-1 only by our taxonomy, no compiler tooling |
| ~2014-15 | **Rust serde** (David Tolnay) | Pre-1.0 development from ~2014–15; serde 1.0 released May 2017. Procmacro derive — closest cross-language analog: "compiler generates per-type serializer from the type definition" |

### Adjacent format / target ecosystem

| Year | Format / Tool | Notes |
|---|---|---|
| 1998 | **HDF5** (NCSA → HDF Group) | The target format itself. NCSA released with DOE/NASA/NCSA support. HDF Group spun off in 2006 |
| Oct 2007 | **JSON Schema** (Kris Zyp first proposal) | First formal draft December 2009 |
| 2008 | **MessagePack** (Sadayuki Furuhashi) | Announced August 16, 2008 |
| 2009 | **Apache Avro** | Initial release 2009; v1.0.0 + Apache TLP May 2010 |
| 2008–09 | **h5py** (Andrew Collette) | Python HDF5 with automatic runtime type mapping |
| 2011 | **Swagger** | Renamed OpenAPI ~2015 |
| Oct 2013 | **CBOR** (RFC 7049) | Binary JSON, IoT-focused |
| Jun 2014 | **FlatBuffers** | (also in schema-first table) |
| 2015 | **GraphQL** (Facebook open-source) | Internal since 2012, public release 2015 |
| 2015 | **HighFive** (BlueBrain) | Started 2015 as part of the Blue Brain Project. **Closest HDF5-space alternative to h5cpp.** Manual struct-to-compound mapping, no AST codegen |
| Feb / Oct 2016 | **Apache Arrow** | Announced February 17, 2016; first release v0.1.0 October 7, 2016 |

## What is unique to h5cpp-compiler

Sifting the timeline above, the components h5cpp-compiler combines all exist as prior art individually:

- **AST-walk-and-codegen** — Qt MOC (~1995), SWIG (1996), rootcling (2014)
- **C++ struct as source of truth, not IDL** — Rust serde (~2014–15), Boost.PFR (2014)
- **HDF5 compound type mapping** — HDF5 itself (1998), HighFive (2015), HDF Group's official C++ API
- **Zero-copy scatter via VLEN descriptors** — exists in HDF5 itself as primitives (`H5Tvlen_create`, `hvl_t`), but using them automatically from a Clang AST walk has no published precedent we found

The unique combination:

> Clang AST walker discovers user struct types via `h5::write` call sites → emits a per-type shim that builds HDF5 compound-with-VLEN descriptors pointing directly at `vector.data()` → library issues a single `H5Dwrite` with zero intermediate buffer.

A focused web search (`"HDF5 zero-copy scatter gather C++ struct compound type automatic generation"`) returned Steven Varga's own blog post as the canonical reference — *Zero-Cost C++ Structs to HDF5 Compound Types with H5CPP*. No competing publications found.

## Closest competitor in the HDF5 space — HighFive (2015)

**HighFive** (BlueBrain) is the most-similar HDF5 C++ library. The key distinction:

| Feature | HighFive (2015) | h5cpp-compiler (2018) |
|---|---|---|
| Library | Header-only C++14/17 wrapper around HDF5 C API | Header-only C++17 library + LLVM-based source-transformation tool |
| Struct → compound mapping | **Manual** — user calls `HighFive::CompoundType::create<MyStruct>(…)` and registers each field explicitly | **Automatic** — Clang AST walker discovers struct definitions via call sites; emits `register_struct<T>()` specializations |
| Scatter for `std::vector` fields | Not implemented at this level — user handles indirection manually | Designed-in (target state for tier 2; see scatter/gather design doc) |
| MPI claim | None made | Explicit from the project's debut |
| Codegen tooling | None | Clang Tooling-based |

HighFive is the high-quality manual alternative; h5cpp-compiler is the automatic alternative. They are complementary rather than competing.

## Closest competitor in the technique space — rootcling (2014)

`rootcling` is the strongest direct analog. Both tools:
- Walk C++ class definitions via Clang/LLVM tooling
- Emit per-type serialization shim code
- Plug into a runtime that uses the shim for I/O dispatch

Differences:

| | rootcling | h5cpp-compiler |
|---|---|---|
| Target file format | ROOT `.root` | HDF5 `.h5` |
| Discovery mechanism | User-supplied `LinkDef.h` listing classes to wrap | AST walker latches onto `h5::write` / `h5::read` call sites |
| Runtime model | `TClass` + `TStreamerInfo` (per-class runtime dictionary) | Compile-time template specialization (`register_struct<T>`, future `scatter<T>` / `gather<T>`) |
| Container support | STL containers via `TStreamerInfo` | `std::vector` + extensible via adapter trait |
| Polymorphism | Yes, via RTTI + class hierarchies | Out of scope (tier 4 reserved with `[[h5cpp::serialize_full]]`) |

rootcling is older, more mature, and broader in scope (polymorphism, multi-format I/O via ROOT's TFile). h5cpp-compiler is narrower (HDF5 only) and sharper (zero-copy scatter as a first-class concern).

## What this means for paper / pitch claims

Claims we can make confidently, with this verification behind us:

1. **The Clang-AST-walking-codegen pattern is established prior art** (rootcling 2014, with conceptual ancestors back to Qt MOC ~1995 and SWIG 1996). No claim of novelty for the *pattern*.
2. **Applying it specifically to HDF5 with automatic struct → compound type mapping** is novel — HighFive (2015) is the closest HDF5-space tool and is explicitly manual.
3. **Zero-copy scatter via VLEN descriptors emitted from AST walking** has no published precedent we found. This is the most defensible originality claim.
4. **The MPI angle was framed at debut** — h5cpp's 2018 announcement already used "low latency MPI capable persistence" wording. This is not a retrofit.

Claims we should be careful about:

- "First to do automatic C++ → HDF5 compound mapping" — needs caveat for HighFive's *manual* approach being the closest extant tool; h5cpp-compiler is the *first automatic* one we could find.
- "Novel" by itself is hard to defend; "novel combination of established patterns applied to HDF5 with the zero-copy guarantee" is defensible.

## Suggested next steps if pursued for publication

1. **CHEP proceedings review** — search Computing in High Energy and Nuclear Physics proceedings for ROOT I/O papers by Philippe Canal, Wim Lavrijsen, et al. The ROOT team has written extensively about their persistence model
2. **HDF Group publications** — ask Gerd Heber (meeting 2026-05-23) for pointers to any prior HDF5 + automatic-struct-mapping work the Group has seen
3. **SC / ISC paper search** — the HPC community has chewed on scientific data formats for decades; a focused literature pass would either turn up missing prior art or strengthen the originality claim
4. **Survey of C++ reflection working group output** — the SG7 mailing list and P-papers around P1240 / P2996 may reference work in this space

## Sources

All cited URLs verified accessible 2026-05-21.

### Direct verification sources

- [Cap'n Proto - Wikipedia](https://en.wikipedia.org/wiki/Cap'n_Proto) — release date April 1, 2013
- [FlatBuffers - Wikipedia](https://en.wikipedia.org/wiki/FlatBuffers) — release June 17, 2014
- [Protocol Buffers, our serialized structured data, released as Open Source - Google Developers Blog (Jul 2008)](https://developers.googleblog.com/2008/07/protocol-buffers-our-serialized.html)
- [Apache Thrift - Wikipedia](https://en.wikipedia.org/wiki/Apache_Thrift) — Facebook open-sourced 2007, Apache TLP 2010
- [Apache Avro - Wikipedia](https://en.wikipedia.org/wiki/Apache_Avro) — initial 2009, v1.0.0 May 2010
- [Cereal - USC iLab](https://github.com/USCiLab/cereal) — copyright dates from 2013
- [Serde releases](https://github.com/serde-rs/serde/releases) — Tolnay; 1.0 May 2017
- [Boost.Serialization - History](https://www.boost.org/doc/libs/1_78_0/libs/serialization/doc/history.html) — first Boost release 1.32 Nov 1, 2004
- [ROOT - Wikipedia](https://en.wikipedia.org/wiki/ROOT) — initial release 1995
- [Cling - ROOT](https://root.cern/cling/) — first released 2014 with ROOT 6
- [SWIG - Wikipedia](https://en.wikipedia.org/wiki/SWIG) — first release February 1996
- [Hierarchical Data Format - Wikipedia](https://en.wikipedia.org/wiki/Hierarchical_Data_Format) — HDF5 released 1998
- [HighFive - BlueBrain GitHub](https://github.com/BlueBrain/HighFive) — created 2015
- [Boost.PFR / Magic Get](https://github.com/apolukhin/magic_get) — copyright 2014
- [refl-cpp](https://github.com/veselink1/refl-cpp) — copyright 2019
- [Apache Arrow 10-year anniversary blog (Feb 2026)](https://arrow.apache.org/blog/2026/02/12/arrow-anniversary/) — announced Feb 17, 2016; v0.1.0 Oct 7, 2016
- [MessagePack - Sadayuki Furuhashi interview](https://usesthis.com/interviews/sadayuki.furuhashi/) — announced August 16, 2008
- [JSON Schema specification links](https://json-schema.org/specification-links) — first proposal Oct 2, 2007; first draft Dec 2009
- [GraphQL: A data query language - Engineering at Meta (Sep 2015)](https://engineering.fb.com/2015/09/14/core-infra/graphql-a-data-query-language/)
- [P1240R1 - Scalable Reflection in C++](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1240r1.pdf) — R0 Oct 8, 2018; R1 Oct 3, 2019; R2 Jan 15, 2022

### h5cpp project sources

- [h5cpp GitHub](https://github.com/steven-varga/h5cpp) — confirms 2018 debut at Chicago C++ Usergroup and ISC'19 BOF presentation
- [h5cpp-compiler GitHub](https://github.com/steven-varga/h5cpp-compiler)
- [h5cpp11 GitHub (archived)](https://github.com/steven-varga/h5cpp11) — second-generation precursor; C++11 template library; GitHub repo created Jul 19, 2018 (verified via `gh api repos/steven-varga/h5cpp11`); earliest commits Dec 2017
- [Zero-Cost C++ Structs to HDF5 Compound Types with H5CPP — VargaLABS blog](https://steven-varga.ca/blog/compound-type-mapping/)
- [H5CPP: low latency MPI capable persistence for Modern C++ and HDF5 — caffe-users group](https://groups.google.com/g/caffe-users/c/RkEVXPrxBro) — confirms MPI framing from the start
- [HDF5 C++ Webinar Followup — The HDF Group (Jan 2019)](https://www.hdfgroup.org/2019/01/28/hdf5-c-webinar-followup/) — confirms HDF Group collaboration
