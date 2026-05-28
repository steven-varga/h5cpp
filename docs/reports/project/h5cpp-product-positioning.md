@page reports_product_positioning Task: H5CPP Product Positioning

**Owner:** Mary
**Status:** Parked — do not activate during the current engineering stack
**Scope:** Documentation and messaging only — no code changes

## Goal

Define the public-facing positioning of H5CPP for the VargaLabs ecosystem.
Output is intended for: `README.md`, `docs/index.md`, and the GitHub repository
description/topics.

## Positioning Constraints

Public-facing content must reference only Layer 1:

| Asset | May reference? | Notes |
|-------|---------------|-------|
| `iex-download` | Yes | Cite DOI 10.5281/zenodo.17188420 |
| `iex2h5` | Yes | Cite DOI 10.5281/zenodo.15677290 |
| H5CPP throughput benchmarks | Yes, with caveats | See below |
| `delta` | No | Internal |
| `sigma` | No | Internal |
| Commercial products | No | Future only |

## Required Benchmark Wording Caveats

Every throughput figure must carry all four caveats:

1. "Burst-input measurement; may not reflect sustained-load performance."
2. "Measured on ThinkPad X1 Carbon Gen 12 (Intel Core Ultra 5 125U),
   g++ 14.2.0, Linux Mint 22.1."
3. "Figures produced by iex2h5. H5CPP does not publish canonical benchmarks."
4. "IEX market data © Investors Exchange. Attribution required."

## Parked Until

Do not activate this task until PR #99 is merged and the #85–#90 refactor
chain begins. The public API surface will change significantly during that
chain; positioning work before then risks becoming stale.

## Output Format

When unparked, append a `## Draft` section to this file with proposed wording
before opening any docs PR or editing any public-facing file.

---

## Draft — Product Strategy & Messaging Framework (John, PM)

*Based on hdf5-cpp-field-study: 796 repos, 756 end-user projects. Data from 2026-05-27.*

---

### Executive Summary

The field study reveals a market in pain, not in denial. **76.2% of C++ projects using HDF5 are still writing raw C or mixed C/C++ code.** The "official" C++ API (H5Cpp.h) is not winning — it accounts for only **8.39% of all HDF5 calls** and actively creates a "mixed mode" trap that makes codebases **worse** than pure C. Meanwhile, **24 out of 796 repos** show teams trying to build their own wrappers and failing.

H5CPP's opportunity is not to be "another HDF5 wrapper." It is to be **the C++ standard library for HDF5** — full API coverage, zero-cost abstraction, and native fluency in the HPC ecosystem (MPI, OpenMP, CUDA, Eigen) that dominates the co-dependency graph.

---

### 1. Core Value Proposition

**H5CPP gives you the entire HDF5 C API through modern C++ — without the raw handles, without the mixed-mode schizophrenia, and without sacrificing a single feature.**

Key pillars:

| Pillar | What it means | Data anchor |
|---|---|---|
| **Completeness** | Every C API function has a typed, RAII C++ equivalent. No dropping to `hid_t` because the wrapper ran out of ambition. | C++ API accounts for only 8.39% of real-world HDF5 calls; the other 91.61% is C. |
| **Zero-cost** | Template metaprogramming resolves at compile time. No runtime overhead vs hand-rolled C. | HPC users (MPI: 266 projects, CUDA: 178) will not tolerate abstraction tax. |
| **Ecosystem-native** | First-class support for Eigen, Boost, OpenMP, MPI, and async I/O. Not bolted-on adapters — architectural citizens. | Top co-occurring libraries in the study match H5CPP's supported type matrix exactly. |
| **Leak-proof** | RAII owns every handle. Mixed-mode codebases in the study expose **4.4× more raw `hid_t` handles** than pure C. H5CPP eliminates that category of bug entirely. | 28.8% of projects are in mixed mode; they are the highest-risk segment. |

**What H5CPP should NOT claim:** "Easier than the C API." That positions us against HighFive's convenience play and invites benchmark fights we don't need. H5CPP is not "easier" — it is *correct*, *complete*, and *modern*.

---

### 2. The Killer Talking Point

> **"Mixed C/C++ HDF5 code has 4.4× more raw handle exposure than pure C. The official C++ API doesn't solve the problem — it is the problem."**

Why this lands:
- It names the enemy precisely: **mixed mode**, not "the C API."
- It weaponizes the data. 28.8% of projects are mixed; they feel this pain but blame themselves.
- It re-frames H5Cpp.h from "the standard choice" to "the trap that forces you into raw C for 92% of your calls."
- It creates urgency: every mixed-mode repo is a migration candidate.

**Secondary talking point (for HPC audiences):**
> **"If your codebase already uses MPI, OpenMP, or Eigen, H5CPP is the only wrapper designed for your stack from the ground up."**

---

### 3. Competitive Differentiation

#### vs H5Cpp.h (the "official" C++ API)

| Dimension | H5Cpp.h | H5CPP |
|---|---|---|
| Coverage | ~8% of API surface; forces drop to C for advanced features | 100% surface via `h5::` abstractions |
| Handle model | Thin RAII around `hid_t`; still exposes raw handles heavily | Typed descriptors; async mode even `=delete`s `operator ::hid_t()` to prevent accidental raw calls |
| Error handling | Exceptions only; no structured error paths | Exception + error-code interop; respects HDF5's error stack |
| C++ standard | C++98-era design | C++17 baseline, progressively gates C++20/23/26 features (lock-free queues, `std::float16_t`, ranges views) |
| Ecosystem | None (no Eigen, no MPI-aware types, no async) | Native Eigen, Armadillo, Blaze, MPI parallel I/O, FAPL-scoped worker pools |

**Message:** *"H5Cpp.h is a compatibility shim from 2002. H5CPP is a modern C++ library for 2025."*

#### vs HighFive

HighFive is the most popular third-party wrapper (68 repos in our sample). It is H5CPP's nearest competitor, but the positioning should be **orthogonal**, not antagonistic.

| Dimension | HighFive | H5CPP |
|---|---|---|
| Design goal | "User-friendly" — simplification layer | "Full-spectrum" — modernization layer |
| API philosophy | Hide complexity; opinionated defaults | Expose power; type-safe control |
| Thread safety | Not thread-safe (acknowledged flaw) | Lock-free pipelines, async dispatch, TSan-clean |
| HPC features | MPI supported, but not architecturally central | MPI, OpenMP, SIMD filters, parallel decompression, async I/O as first-class |
| Struct reflection | Manual compound-type construction | `h5cpp-compiler` auto-generates reflection for C++ structs |
| C++ standard | C++14 | C++17 → C++26 |

**Message:** *"HighFive makes simple HDF5 simple. H5CPP makes hard HDF5 possible — in C++."*

**Do not trash-talk HighFive.** It serves a different job-to-be-done (basic I/O for non-HPC users). Instead, frame H5CPP as the upgrade path when HighFive's guardrails become walls.

#### vs DIY Wrappers

24 suspicious wrapper attempts detected in 796 repos. These are teams with the same instinct as Steven — "this API should be better" — but without the time or expertise to finish.

**Message:** *"Your team is already trying to build H5CPP. Stop."*

Proof point: publish a "wrapper anatomy" blog post showing the 5 most common anti-patterns in DIY HDF5 wrappers (handle leaks, exception-unsafe close paths, missing `H5Dvlen_reclaim`, wrong dataspace rank matching) and how H5CPP solves each one.

---

### 4. How to Win the 28.8% "Mixed" Segment

Mixed users are the highest-value target. They have already rejected pure C and tried the official C++ API — and failed. They are frustrated, not ignorant.

**What they need to hear:**

1. **"You don't have to rewrite everything."**
   - Provide an explicit interop bridge: `h5::fd_t::from_hid_t(hid_t)` and `h5::fd_t::hid_t()` (except in async mode where it's deleted).
   - Sell incremental migration: replace one dataset operation at a time. H5CPP handles can adopt existing `hid_t` values and manage their lifetime.

2. **"Show me the handles."**
   - Offer a static-analysis script or clang-tidy check that counts raw `hid_t` usage and `H5*close` mismatches in a codebase.
   - Marketing equivalent: a one-liner script they can run on their repo to get a "leak score." Mixed-mode repos will score badly; the emotional impact drives migration.

3. **"No feature left behind."**
   - For every common "I had to drop to C for this" scenario, publish a 5-line before/after code snippet.
   - Target the gaps: SWMR, virtual datasets, reference types, custom filters, chunk iteration, attribute iteration, object references. These are exactly where H5Cpp.h forces mixed mode.

4. **"Your HPC stack is already supported."**
   - 266 projects use MPI; 264 use OpenMP; 178 use CUDA; 155 use Eigen.
   - H5CPP's matrix of supported types and parallel modes should be front-and-center in all mixed-mode messaging.

5. **"The compiler does the boring part."**
   - `h5cpp-compiler` auto-reflects structs. Mixed-mode users are often hand-writing `H5Tinsert` blocks. Show them the `#include <generated>` workflow.

---

### 5. Elevator Pitch

> **HDF5's official C++ API covers 8% of the library and forces the other 92% into raw C. The result? C++ projects that mix APIs leak 4.4 times more `hid_t` handles than teams that stayed in C. H5CPP is a zero-overhead C++17 library that exposes the entire HDF5 API — every feature, no raw handles, no mixed-mode trap. If your project already runs MPI, OpenMP, or Eigen, H5CPP is the only wrapper built for your stack from day one.**

(22 seconds spoken.)

---

### 6. Messaging Do's and Don'ts

| Do | Don't |
|---|---|
| Lead with **data** (4.4×, 8.39%, 28.8%, 24 failed wrappers) | Use vague superlatives ("best," "fastest," "easiest") |
| Name the enemy: **mixed mode** and **incomplete wrappers** | Trash-talk the C API itself — 47.4% of users still choose it |
| Speak HPC fluently: MPI rank 0, chunk cache, FAPL, DXPL | Dumb it down for "general" audiences; our user is a domain scientist who writes C++ |
| Emphasize **completeness** and **correctness** | Emphasize "easy" — that is HighFive's territory |
| Use "H5CPP" (all caps) to differentiate from `ess-dmsc/h5cpp` | Confuse the namespace by calling us just "h5cpp" without context |
| Acknowledge the C escape hatch as a **bridge**, not a crutch | Pretend raw C interop doesn't exist — users know it does |

---

### 7. Proof Points to Collect Before Public Messaging

The strategy above depends on these claims being verifiable. Collect or generate:

1. **Handle-exposure audit** — A script that runs on any GitHub repo and produces a "mixed-mode score." This turns the 4.4× finding into an interactive tool.
2. **Coverage matrix** — A public page listing every HDF5 C API function and its H5CPP equivalent. Update it automatically from source.
3. **Migration guide** — 10 real-world mixed-mode patterns (from the field study) and their H5CPP equivalents.
4. **Performance micro-benchmarks** — `h5::write` vs raw `H5Dwrite` for contiguous, chunked, and filtered datasets. Show assembly or at least `perf stat` parity.
5. **"Suspicious wrapper" case studies** — Anonymized examples of the 24 DIY wrapper attempts, highlighting the anti-patterns H5CPP prevents.
6. **HighFive escape-hatch guide** — A documented path for teams that outgrow HighFive and need H5CPP's feature depth.

---

### Next Step

Steven — review this framework. If it lands, the next workstream is a public-facing "State of C++ HDF5" report based on the field-study data. That report becomes the content engine for README, conference talks, and blog posts. We own the narrative by owning the data.


---

## Draft — Revised Positioning Framework (Post-Pivot)

*Based on updated field-study data: 939 repos. Steven's directive: embrace `hid_t`, don't erase it.*

---

### 1. Core Value Proposition (One Sentence)

**h5cpp is the only C++ HDF5 library that wraps the entire C API in zero-overhead RAII without hiding the `hid_t` — so you can write modern C++ where you want it and drop to raw C exactly where you need it, with no rewrite, no wrappers, and no leaks.**

---

### 2. The Reframed Killer Talking Point

> **"The average C++ HDF5 project uses raw `hid_t` 531 times per codebase. That isn't a leak — it's proof that developers need the full C API. h5cpp is the only wrapper that doesn't force you to throw away working `hid_t` code to get RAII."**

**Why this lands now:**
- It validates the user's existing expertise instead of shaming it. The old "4.4× more leaks" frame made mixed-mode users feel like they were doing something wrong; this frame says they were doing something *right* and their tools failed them.
- It weaponizes the 531 number as evidence of H5Cpp.h's incompleteness, not the user's incompetence.
- It inoculates against HighFive: "If you love your `hid_t`, HighFive makes you choose between purity and power. h5cpp gives you both."

---

### 3. Competitive Differentiation

#### vs H5Cpp.h (the "official" C++ API)

H5Cpp.h tries to replace the C API and fails — covering only ~11.3% of real-world usage before forcing an unprotected drop into raw C. It is a half-finished port that leaves you with the worst of both worlds: C++ syntax without C++ safety, and raw `hid_t` without a migration path.

| Dimension | H5Cpp.h | h5cpp |
|---|---|---|
| C API coverage | Partial (~11% of real usage); unprotected drop to C for advanced features | 100% surface; every C function has a type-safe C++ equivalent |
| `hid_t` interop | Exposes raw handles accidentally; no lifetime bridge | First-class bidirectional interop: adopt a `hid_t` into RAII, or extract a `hid_t` for C calls, safely |
| Error paths | Exceptions or raw C error codes, never both | Unified: C++ exceptions with structured C error-stack access |
| Migration story | "Rewrite everything" | "Wrap what you want; keep what works" |

**Message:** *"H5Cpp.h makes you choose C++ or C, then abandons you halfway through. h5cpp lets you use both, safely, in the same function."*

#### vs HighFive

HighFive is a genuine convenience layer — for simple I/O, it works. But its core design decision is to **erase `hid_t` entirely**. That is fine until it isn't: when you need a custom HDF5 filter, when you need to pass a file handle to a C library, when you need a feature HighFive hasn't wrapped yet, or when you need to incrementally migrate a 100k-line C codebase. At that moment, HighFive's abstraction becomes a wall, not a floor.

| Dimension | HighFive | h5cpp |
|---|---|---|
| Design goal | "Pure C++" — hide `hid_t` to simplify | "Full-spectrum" — wrap `hid_t` in RAII, keep it accessible |
| Interoperability | Zero `hid_t` access; breaks C code interop | Bidirectional `hid_t` adoption/extraction at any boundary |
| Feature ceiling | Bounded by wrapper coverage; escape hatch is awkward | Unbounded; raw C API is always one `.hid_t()` away |
| Migration path | All-or-nothing rewrite | Incremental: wrap one dataset, one file, one call at a time |

**Message:** *"HighFive gives you a sandbox. h5cpp gives you the keys to the engine — with a seatbelt."*

**Do not trash-talk HighFive.** Acknowledge it honestly: if you have a greenfield project with simple HDF5 needs and zero C legacy, HighFive is reasonable. h5cpp is for everyone else — which is **88.7% of the market**.

#### vs Raw C

Raw C is complete, fast, and ubiquitous. It is also manually memory-managed, exception-unsafe, and verbose. The 48.2% of pure C users are not ideological — they are pragmatic. They have not switched because no C++ wrapper gave them a reason to that was worth the migration cost.

**Message:** *"Keep your `hid_t`. Lose the leaks."*

---

### 4. The New 30-Second Elevator Pitch

> **"Seventy-seven percent of C++ HDF5 projects are still writing raw C or mixed C/C++ code — not because they love malloc-style error handling, but because HDF5's official C++ API is incomplete and HighFive hides `hid_t` so completely it breaks interoperability with existing code. h5cpp is the only C++17 wrapper that wraps the entire C API in RAII while keeping `hid_t` fully accessible. You get modern C++ safety and ergonomics where you want them, and raw C interoperability exactly where you need them — with zero migration cliff. Stop rewriting working code to fit your wrapper. Get a wrapper that fits your code."**

*(~29 seconds at natural pace.)*

---

### 5. The ONE Proof Point / Demo to Build First

**"The Mixed-Mode Migration Demo"** — a single, copy-pasteable `examples/interop_demo.cpp` that does the following in order:

1. **Opens a file with raw C:** `hid_t file = H5Fcreate(...);`
2. **Adopts it into h5cpp:** `h5::fd_t h5file = h5::fd_t::adopt(file);`
3. **Creates a chunked dataset via h5cpp RAII:** using `h5::ds_t`, `h5::dapl_t`, modern C++ syntax.
4. **Extracts the raw `hid_t` back out:** `hid_t ds = h5file.create_dataset(...).hid_t();`
5. **Writes data through raw C:** `H5Dwrite(ds, ...);`
6. **Triggers an exception mid-write:** a forced error path where the raw C call fails.
7. **Proves no leaks:** the `h5::fd_t` and `h5::ds_t` destructors fire correctly, closing all handles even though raw C was in the call stack.
8. **Prints the `hid_t` at each step:** so the user can see it is the *same* underlying handle, not a copy or wrapper facade.

**Why this demo first:** It is the entire pivot in 80 lines of code. It proves the one claim no competitor can make: *you can mix C and C++ freely without rewriting and without leaking.* Every other proof point (performance, coverage matrix, HPC features) is secondary until this interop story is viscerally credible.

---

### 6. Why This Pivot Is Smarter Than "Zero hid_t"

**1. It targets the addressable market, not the fantasy market.**
The "zero `hid_t`" narrative implicitly positions against the 11.3% of projects using pure H5Cpp.h — a segment so small it is practically a rounding error. The "preserve `hid_t`" narrative speaks directly to the **77%** who are actively using C or mixed C/C++ today. That is a **7× larger TAM**.

**2. It lowers activation energy from "rewrite" to "wrap."**
Telling a team with 531 raw `hid_t` calls to "go zero-`hid_t`" is a declaration of war on their codebase. Telling them "we can put seatbelts on those 531 calls" is an offer of help. Incremental adoption beats rip-and-replace in infrastructure libraries.

**3. It avoids an unwinnable positioning fight with HighFive.**
HighFive already owns "simple, pure C++ HDF5." Competing on that axis forces benchmark battles and API beauty contests where HighFive has home-field advantage. Competing on "full C API + C++ safety + `hid_t` interop" puts h5cpp on an axis where HighFive literally cannot follow without a ground-up redesign.

**4. It turns the strongest competitor feature into a liability.**
HighFive's proudest claim — "you never see a raw `hid_t`" — becomes a warning label for any team with existing C code, custom plugins, or advanced HDF5 needs. The pivot reframes HighFive's abstraction not as elegance, but as a cage.

**5. It aligns with how HDF5 is actually used.**
The 531 `hid_t` number is not an anomaly; it is structural. HDF5 is an ecosystem of filters, tools, and legacy code built around the C API. A C++ wrapper that pretends that ecosystem doesn't exist is a wrapper that guarantees its own irrelevance. h5cpp's job is to make the C API safer, not to pretend there is a parallel universe where it doesn't exist.

---

### Next Step (Post-Pivot)

Steven — the old "State of C++ HDF5" report frame needs a rewrite. The headline is no longer "Mixed mode is a trap." The headline is **"Your `hid_t` is an asset. Every other C++ wrapper treats it as debt."** If this lands, the first engineering deliverable is the `interop_demo.cpp` proof point (Section 5). Everything else — coverage matrix, performance benchmarks, migration guides — follows once the interop story is code, not just words.
