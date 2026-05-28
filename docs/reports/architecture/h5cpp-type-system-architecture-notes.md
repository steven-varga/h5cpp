@page reports_type_system_architecture_notes h5cpp Type System Architecture Notes

> **Accuracy note:** Reflects the `prototype` working-tree bank as of 2026-05-26.
> Files modified (uncommitted, banked for sequencing): `h5cpp/H5Tmeta.hpp`,
> `h5cpp/H5Tall.hpp`, `h5cpp/H5Dwrite.hpp`, `h5cpp/H5Dread.hpp`, `h5cpp/H5Awrite.hpp`,
> `h5cpp/H5Aread.hpp`. The bank goes beyond `#276`/`#274`: `std::tuple` is folded into
> `access_traits_t` as a new `kind = composite`, four compile-time stoppers are present
> on every dispatch entry, the attribute side has full kind × storage parity, and
> `storage_representation_impl<T>` is now complete for scalar object/text types
> (string, string_view, char*, complex, registered aggregates).
>
> **Bank status:** internally consistent. All four static_assert stoppers are precise.
> Review item A7 (unregistered-aggregate silent crash) preserved as a compile-time
> error via the new `has_registered_compound<T>` trait. Ready for commit slicing.

---

## Layer Map: Current State

```
User container (T)
      │
      ▼
[L1] storage_representation_v<T>        ← H5Tmeta.hpp                              ✔ DONE
      scalar / c_array / linear_value_dataset /
      key_value_dataset / ragged_vlen_dataset /
      fixed_inner_extent_dataset / vlen_text_dataset / unsupported
      Structural fallbacks (Gap 4): is_sequential_like → linear_value_dataset
                                    is_set_like        → linear_value_dataset  ◆ NEW
                                    is_map_like        → key_value_dataset
      Tuple/pair explicit:           → scalar
      Arithmetic/enum:               → scalar
      W4 additions (scalar object/text completion):
            basic_string<char,...>          → vlen_text_dataset
            basic_string_view<char,...>     → vlen_text_dataset
            char* / const char*             → vlen_text_dataset
            complex<T> (T floating)         → scalar
            aggregate fallback gated on has_registered_compound<T> → scalar
                (registered via H5CPP_REGISTER_STRUCT / H5CPP_REGISTER_TYPE_;
                 unregistered aggregates fall through to 'unsupported' →
                 caught by the dispatch stopper — preserves review item A7)
      │
      ▼
[L2] storage_traits_t<T>::create_type() ← H5Tmeta.hpp                              ✔ DONE (element types)
      Returns hid_t for: arithmetic, text, arrays, reflected compounds.
      NOT called for containers — callers extract element type first.
      Overlap with `dt_t<T>` for tuple/pair/complex/float16 — see Open Items D.1.
      │
      ▼
[L3] access_traits_t<T>                 ← H5Tmeta.hpp                              ✔ DONE
      object     — arithmetic, std-layout aggregates, pair<K,V>, complex<T>
      composite  — non-std-layout aggregates (std::tuple<Ts...>): pack/unpack via traits
      contiguous — .data() + trivial element (vector, array, span, vector<array<T,N>>,
                                              vector<complex<T>>)
      pointers   — .data() + non-trivial element (vector<string>, vector<vector<T>>,
                                                  vector<tuple<Ts...>>, vector<NonTrivialPod>)
      iterators  — begin/end only (list, set, map, deque, fwd_list)
      text       — std::string, std::string_view
      Composite interface: traits::pack(ref, char*), traits::unpack(ref, const char*),
                           traits::bytes() (compile-time size), traits::size(ref) (scalar).
      │
      ▼
[L4] impl::{decay, size, data, get}     ← H5Mstl.hpp    ◇ PARTIAL
      decay<T>:           structural fallback exists (value_type unwrap)
      size(T):            structural fallback exists (has_size)
      data(T):            by-name only — no path for list / set / map (no .data())
      get<T>::ctor:       by-name only — no structural fallback in this surface
      structural_data(T): ◆ NEW — used by H5Dread fallback when rank<T>==0;
                          returns ref.data() for any T with a .data() member.
                          Kept off the impl::data overload set to avoid ambiguity
                          with the per-linalg-mapper `data(Object&)` templates.
      │
      ▼
[L5] h5::write dispatch                 ← H5Dwrite.hpp                             ✔ DONE (write side)

      Stoppers (all four dispatch headers, at entry):
        static_assert(!has_scatter<T>)          → ds-overload only; scatter routes via fd-gateway
        static_assert(storage != unsupported)   → blocks unregistered/nested cases
        static_assert(!is_stl_like<element_t>
                      || storage in vlen)        → containers-of-containers via VLEN only
        static_assert(is_standard_layout_v<E>)   → in iter-staging branch, catches non-std-layout escapes

      Dispatch overload write(ds, ref, args...):
        composite               → traits::pack → H5Dwrite (scalar dataspace)               ✔
        contiguous|object|text  → traits::data/size → H5Dwrite                             ✔
        pointers:
          vlen_text_dataset     → char* relay → H5Dwrite (H5T_VARIABLE)                    ✔
          ragged_vlen_dataset   → hvl_t relay → H5Dwrite (H5Tvlen_create)                  ✔
          access_kind_v<elem>=composite → elem_traits::pack each → H5Dwrite (rank-1 compound) ✔
          fallthrough           → h5::gather → flat pointer write                          ✔
        iterators:
          key_value_dataset     → kv_t compound → H5Dwrite                                 ✔
          access_kind_v<elem>=composite → elem_traits::pack each → H5Dwrite (rank-1 compound) ✔
          fallthrough           → static_assert(std-layout) + staging vector → flat write  ✔

      Gateway overload write(fd, path, ref, args...):
        scatter path            → has_scatter<T> → h5::scatter<T>                          ✔
        composite scalar        → resolved_type_t<T> + scalar createds                     ✔
        container-of-composite  → resolved_type_t<element_t> + rank-1 createds             ✔
        vlen_text_dataset       → H5Tcopy(H5T_C_S1)+H5T_VARIABLE+createds                  ✔
        ragged_vlen_dataset     → H5Tvlen_create+createds                                  ✔
        key_value_dataset       → H5T_COMPOUND{key,value}+createds                         ✔
        all others              → h5::create<element_t> (existing path)                    ✔

      h5::awrite dispatch               ← H5Awrite.hpp                             ✔ DONE (attribute write — full parity)
      h5::aread dispatch                ← H5Aread.hpp                              ✔ DONE (attribute read — full parity)
      h5::read dispatch                 ← H5Dread.hpp                              ✔ DONE (read side)

      Dispatch overload read(ds, ref, args...):
        composite               → H5Dread → traits::unpack                                 ✔
        contiguous|object|text  → traits::data/size → H5Dread                              ✔
        pointers:
          vlen_text_dataset     → char* relay → H5Dread + reclaim                          ✔
          ragged_vlen_dataset   → hvl_t relay → H5Dread + reclaim                          ✔
          access_kind_v<elem>=composite → H5Dread → elem_traits::unpack each               ✔
        iterators:
          key_value_dataset     → kv_t compound H5Dread → map::insert                      ✔
          access_kind_v<elem>=composite → H5Dread → elem_traits::unpack each → inserter    ✔
          fallthrough           → static_assert(std-layout) + staging vector → inserter    ✔

      Object-return overload read<T>(ds, args...):
        composite               → H5Dread → traits::unpack → return T                      ✔
        container-of-composite  → H5Dread → elem_traits::unpack each → range-construct T   ✔
        vlen_text_dataset       → char* relay + reclaim → vector<string>                   ✔
        ragged_vlen_dataset     → hvl_t relay + reclaim → vector<vector<T>>                ✔
        key_value_dataset       → kv_t flat buffer → map<K,V>                              ✔
        iterators               → static_assert(std-layout) + staging vector → range-ctor  ✔
        contiguous/object/text  (rank<T> > 0)  → impl::get<T>::ctor + impl::data → H5Dread ✔
        contiguous/object/text  (rank<T> == 0) → ◆ NEW: detection-driven:
                                  if T has .data() + .size() + T(size_t) ctor →
                                  T(n) + impl::structural_data(ref) + H5Dread.
                                  Lands custom vector-shaped containers
                                  without registering impl::rank / impl::get.

      Gateway overload read(fd, path, ref, args...):
        gather path             → has_scatter<T> → h5::gather<T>                           ✔
        non-scatter else        → static_assert(storage != unsupported) + open ds + dispatch ✔
      │
      ▼
[L6] pipeline_t<Derived>               ← H5Zpipeline.hpp
      basic_pipeline_t:                                                            ✔ functional (chunk tiling + filter chain)
      pool_pipeline_t:                                                             ✔ functional (threaded worker pool, Phase 1.3.3)
      threaded_pipeline_t:                                                         ✘ STUB (write_chunk_impl / read_chunk_impl empty)
      romio_pipeline_t:                                                            ✘ STUB
      hadoop_pipeline_t:                                                           ✘ STUB
```

---

## Compile-time stoppers

The current bank installs four classes of `static_assert` guards across all four
dispatch headers. They convert previously-silent failure modes into compile errors.

| Stopper | Files | Catches |
|---|---|---|
| `static_assert(!has_scatter<std::decay_t<T>>::value, ...)` | `H5Dwrite.hpp` ds-overload, `H5Dread.hpp` ref + return overloads | Tier-2 scatter types passed to the ds-overload bypass `h5::scatter<T>` and fall through to the aggregate path with `register_struct<T>() == H5I_UNINIT`. The stopper directs the caller to the fd-gateway. |
| `static_assert(storage != sr_t::unsupported, ...)` | All four dispatch headers, both ds and fd entries | Unregistered POD aggregate, `std::vector<bool>`, container nesting beyond `vector<vector<T>>` / `vector<string>`. Without this, dispatch lands in `H5Dwrite` with `H5I_UNINIT` and silently corrupts. **Precise after W4** — see `has_registered_compound<T>` design below. |
| `static_assert(!is_stl_like<element_t> \|\| storage in {ragged_vlen, vlen_text}, ...)` | All four dispatch headers | Containers of containers (e.g., `vector<list<int>>`, `list<list<int>>`) where the inner container isn't a string and the storage spec didn't route to VLEN. The iterator-staging and pointers-gather paths can't faithfully serialize nested containers. |
| `static_assert(std::is_standard_layout_v<element_t>, ...)` | `H5Dwrite.hpp`, `H5Dread.hpp` (both overloads), `H5Awrite.hpp`, `H5Aread.hpp` iterator-staging branches | Non-std-layout `element_t` in a staging-vector copy path. Composite element types (`std::tuple`) route through the dedicated `access_kind_v<element_t> == composite` sub-branch before reaching the staging else; this guard catches any future escape. |

### Stopper precision (W4 — closed)

The `storage != unsupported` stopper depends on `storage_representation_impl<T>`
covering every legitimate type. Five scalar object/text types fell through to the
primary `unsupported` until W4 landed:

| Type | kind | storage (post-W4) | Mechanism |
|---|---|---|---|
| `std::basic_string<char, Tr, A>` | `text` | `vlen_text_dataset` | explicit storage_representation_impl spec |
| `std::basic_string_view<char, Tr>` | `text` | `vlen_text_dataset` | explicit spec |
| `char*`, `const char*` (as dataset T) | `text` | `vlen_text_dataset` | explicit spec |
| `std::complex<T>` (T floating-point) | `object` | `scalar` | explicit spec (gated on `is_floating_point<T>`) |
| User aggregate via `H5CPP_REGISTER_STRUCT(Foo)` | `object` | `scalar` | aggregate fallback gated on `has_registered_compound<T>` |

**Design: `has_registered_compound<T>` trait.** New trait in `h5::meta` defaults to
`std::false_type`. The `H5CPP_REGISTER_TYPE_` macro (which `H5CPP_REGISTER_STRUCT`
expands through) sets it to `std::true_type` for every registered C++ type — both
the built-in arithmetic types in `H5Tall.hpp` and user-defined POD structs. The
aggregate fallback in `storage_representation_impl<T>` gates on this trait:

```cpp
template <class T>
struct storage_representation_impl<T, std::enable_if_t<
    has_registered_compound<T>::value &&
    !std::is_arithmetic_v<T> && !std::is_enum_v<T> &&
    !is_array_like<T>::value && !is_text_like<T>::value &&
    !is_iterable<T>::value && !has_explicit_storage_repr<T>::value>>
    : std::integral_constant<storage_representation_t, storage_representation_t::scalar> {};
```

**Result:** registered compounds resolve to `scalar`; unregistered aggregates fall
through to `unsupported` and the dispatch stopper catches them at compile time.
Review item A7 preserved.

---

## What Changed in this prototype bank (post-`#276`, post-`#274`)

| Item | Before (`#276` tip) | After (current bank) |
|---|---|---|
| `access_t` enum | object / contiguous / pointers / iterators / text / unsupported | adds `composite` between `object` and `contiguous` |
| `std::tuple<Ts...>` dispatch | `is_tuple_v<T>` early-exit in 4 dispatch sites; `kind = unsupported` (primary template) | `access_traits_t<std::tuple<Ts...>>` explicit spec with `kind = composite`, `pack`/`unpack`/`bytes()` interface; no more early-exits |
| Tuple/pair scalar dispatch | special-case branch with `tuple_layout_t<T>::to_buffer` | unified `kind == composite` branch using `traits::pack(ref, buf)` |
| `vector<tuple<Ts...>>` dispatch | special-case `is_tuple_v<element_t>` branch (vector only — `list<tuple>` would fail to compile due to `ref[i]` use) | `access_kind_v<element_t> == composite` sub-branch inside `kind == pointers` and `kind == iterators` — supports vector / list / set / deque / forward_list of tuples uniformly |
| Iterator-staging non-std-layout escapes | silent on-disk corruption (e.g., `list<some_non_std_layout_type>`) | `static_assert(std::is_standard_layout_v<element_t>)` in the staging branch |
| Attribute side full matrix | structurally collapsed onto `access_traits_t` but matrix coverage was missing: `vector<string>`-as-attr wrote `string*` raw, `map`-as-attr fell through to uninitialised `dt_t`, no composite/ragged_vlen/key_value paths | `H5Awrite.hpp` and `H5Aread.hpp` now mirror the dataset-side dispatch: composite scalar, vlen_text, ragged_vlen, key_value, vector<composite>, iter<composite>, iter-staging — all wired with the same four stoppers |
| Scatter-bypass on ds-overload | silent fall-through to aggregate path with `H5I_UNINIT` | `static_assert(!has_scatter<T>)` blocks the ds-overload for tier-2 types |
| Unregistered-aggregate detection (review A7) | silent runtime crash at H5Dwrite with H5I_UNINIT | blocks via `storage != unsupported`; legitimate registered/scalar types resolve to `scalar`/`vlen_text_dataset` via W4 specs and the `has_registered_compound<T>` trait |
| Nested-container guard (from `#274`) | dropped by `#276` during refactor | reinstated as `static_assert(!is_stl_like<element_t> \|\| storage in vlen)` |

---

## H5Dgather.hpp: Now Requires Re-examination

`H5Dgather.hpp` has seven explicit per-container overloads (list, forward_list, deque,
set, multiset, unordered_set, unordered_multiset) plus a `vector<string>` overload.

Current routing after this bank (unchanged from `#276`):

| gather overload | Called by | Status |
|---|---|---|
| `gather(list<T>)`, `gather(set<T>)`, `gather(deque<T>)`, etc. (7 overloads) | Nothing — iterators branch uses inline staging | **Dead code** |
| `gather(vector<string>)` → `char**` | Nothing — pointers branch uses inline char* relay | **Dead code** |
| Generic `gather(T, vector<E>&)` (fallthrough) | `pointers` fallthrough branch in dispatch overload, for `vector<NonTrivialPod>` | Still live — narrow case |

The per-container overloads in `H5Dgather.hpp` can be removed. The generic fallthrough
stays until the `pointers`/`linear_value_dataset` case is re-examined (it is the only
caller). The `h5::scatter<T>` path (`has_scatter<T>`) is separate — that is
compiler-generated code for reflected types and is unaffected.

---

## Complete Write-Side Type Matrix (current bank)

| Type | `kind` | `storage` | Write mechanism |
|---|---|---|---|
| `int`, `float`, enums | `object` | `scalar` | direct H5Dwrite |
| `std::string`, `std::string_view` | `text` | `vlen_text_dataset` | direct H5Dwrite via `dt_t<char*>` |
| `std::vector<T>`, `std::array<T,N>`, `T[N]` | `contiguous` | `linear_value_dataset`/`c_array` | direct H5Dwrite |
| `std::vector<std::array<T,N>>` | `contiguous` | `fixed_inner_extent_dataset` | direct H5Dwrite (sized rows×N) |
| `std::vector<std::complex<T>>` | `contiguous` | `linear_value_dataset` | direct H5Dwrite |
| Linear algebra (Arma/Blaze/Blitz/uBLAS/valarray/Eigen/IT++/xtensor) | `contiguous` | `linear_value_dataset` | direct H5Dwrite |
| `std::vector<std::string>` | `pointers` | `vlen_text_dataset` | `char*` relay + H5T_VARIABLE |
| `std::vector<std::vector<T>>` | `pointers` | `ragged_vlen_dataset` | `hvl_t` relay + H5Tvlen_create |
| `std::vector<NonTrivialPod>` | `pointers` | `linear_value_dataset` | h5::gather → flat write |
| `std::list<T>`, `std::deque<T>`, `std::forward_list<T>` | `iterators` | `linear_value_dataset` | static_assert(std-layout) + staging vector → flat write |
| `std::set<T>`, `std::multiset<T>` | `iterators` | `linear_value_dataset` | static_assert(std-layout) + staging vector → flat write |
| `std::unordered_set<T>`, `std::unordered_multiset<T>` | `iterators` | `linear_value_dataset` | static_assert(std-layout) + staging vector → flat write |
| `std::map<K,V>`, `std::multimap<K,V>` | `iterators` | `key_value_dataset` | `kv_t` compound + H5T_COMPOUND |
| `std::unordered_map<K,V>`, `std::unordered_multimap<K,V>` | `iterators` | `key_value_dataset` | `kv_t` compound + H5T_COMPOUND |
| **`std::tuple<Ts...>`** (scalar) | **`composite`** | `scalar` | **`traits::pack(ref, buf)` → H5Dwrite (compound, scalar dataspace)** |
| **`std::vector<std::tuple<Ts...>>`** | `pointers` | `linear_value_dataset` | **`elem_traits::pack` each → H5Dwrite (compound, rank-1)** |
| **`std::list<std::tuple<Ts...>>`** | `iterators` | `linear_value_dataset` | **`elem_traits::pack` each → H5Dwrite (compound, rank-1)** |
| **`std::set<std::tuple<Ts...>>`**, **`std::deque<...>`**, **`std::forward_list<...>`** | `iterators` | `linear_value_dataset` | same as above |
| `std::pair<K,V>` (scalar) | `object` | `scalar` | direct H5Dwrite (compound via `dt_t<pair>`, `offsetof`) |
| `std::vector<std::pair<K,V>>` (K,V trivially copyable) | `contiguous` | `linear_value_dataset` | direct H5Dwrite (compound, `offsetof`) |
| `std::complex<T>` (T float/double/long double) | `object` | `scalar` | direct H5Dwrite via `dt_t<complex>` (compound or native H5T_COMPLEX) |
| User aggregate registered via `H5CPP_REGISTER_STRUCT(Foo)` | `object` | `scalar` | direct H5Dwrite via `dt_t<Foo>` from registered compound |
| Compiler-reflected tier-2 (`has_scatter<T>`) | — | — | `h5::scatter<T>` (generated; fd-gateway only) |
| ◆ **Custom vector-shape** (`.data() + .size() + value_type`, e.g. `absl::FixedArray`, `folly::small_vector`, `boost::container::vector`, user types) | `contiguous` | `linear_value_dataset` | direct H5Dwrite via generic `access_traits_t` contiguous spec |
| ◆ **Custom iterator-only sequence** (`begin/end + value_type`, no `.data()`) | `iterators` | `linear_value_dataset` | static_assert(std-layout) + staging vector → flat write |
| ◆ **Custom set-shape** (`key_type + value_type`, no `mapped_type`, e.g. `absl::flat_hash_set`, `tsl::robin_set`, `boost::flat_set`) | `iterators` | `linear_value_dataset` | static_assert(std-layout) + staging vector → flat write |
| ◆ **Custom map-shape** (`key_type + mapped_type + value_type`, e.g. `absl::flat_hash_map`, `tsl::robin_map`, `boost::flat_map`) | `iterators` | `key_value_dataset` | `kv_t` compound + H5T_COMPOUND |
| `std::vector<bool>` | — | `unsupported` | static_assert fires (correctly) |
| `std::vector<std::list<T>>`, `std::list<std::list<T>>`, etc. | `pointers`/`iterators` | various (mostly `linear_value`) | nested-container static_assert fires |
| `std::array<std::string, N>`, `std::array<std::vector<T>, N>` | — | `unsupported` | static_assert fires (per `#274`'s array-of-container guard in storage_representation) |

---

## Complete Read-Side Type Matrix (current bank)

| Type | `kind` | `storage` | Read mechanism |
|---|---|---|---|
| `int`, `float`, enums | `object` | `scalar` | direct H5Dread |
| `std::string`, `std::string_view` | `text` | `vlen_text_dataset` | direct H5Dread via `dt_t<char*>` |
| `std::vector<T>`, `std::array<T,N>`, `T[N]` | `contiguous` | `linear_value_dataset`/`c_array` | direct H5Dread |
| `std::vector<std::array<T,N>>` | `contiguous` | `fixed_inner_extent_dataset` | direct H5Dread (sized rows×N) |
| `std::vector<std::complex<T>>` | `contiguous` | `linear_value_dataset` | direct H5Dread |
| Linear algebra mappers | `contiguous` | `linear_value_dataset` | direct H5Dread |
| `std::vector<std::string>` | `pointers` | `vlen_text_dataset` | `char*` relay + reclaim |
| `std::vector<std::vector<T>>` | `pointers` | `ragged_vlen_dataset` | `hvl_t` relay + reclaim |
| `std::list<T>`, `std::deque<T>`, `std::forward_list<T>` | `iterators` | `linear_value_dataset` | static_assert(std-layout) + staging vector → assign/copy |
| `std::set<T>`, `std::multiset<T>` | `iterators` | `linear_value_dataset` | static_assert(std-layout) + staging vector → insert |
| `std::unordered_set<T>`, `std::unordered_multiset<T>` | `iterators` | `linear_value_dataset` | static_assert(std-layout) + staging vector → insert |
| `std::map<K,V>`, `std::multimap<K,V>` | `iterators` | `key_value_dataset` | `kv_t` compound H5Dread → insert |
| `std::unordered_map<K,V>`, `std::unordered_multimap<K,V>` | `iterators` | `key_value_dataset` | `kv_t` compound H5Dread → insert |
| **`std::tuple<Ts...>`** (scalar) | **`composite`** | `scalar` | **H5Dread → `traits::unpack(ref, buf)`** |
| **`std::vector<std::tuple<Ts...>>`** | `pointers` | `linear_value_dataset` | **H5Dread → `elem_traits::unpack` each** |
| **`std::list<std::tuple<Ts...>>`**, **`set<...>`**, **`deque<...>`**, **`forward_list<...>`** | `iterators` | `linear_value_dataset` | **H5Dread → `elem_traits::unpack` each → inserter / assign** |
| `std::pair<K,V>` (scalar) | `object` | `scalar` | direct H5Dread (compound, `offsetof`) |
| `std::vector<std::pair<K,V>>` | `contiguous` | `linear_value_dataset` | direct H5Dread (compound, `offsetof`) |
| `std::complex<T>` | `object` | `scalar` | direct H5Dread via `dt_t<complex>` |
| User aggregate registered via `H5CPP_REGISTER_STRUCT` | `object` | `scalar` | direct H5Dread via `dt_t<T>` |
| Compiler-reflected tier-2 (`has_scatter<T>`) | — | — | `h5::gather<T>` (generated; fd-gateway only) |
| ◆ **Custom vector-shape** (`.data() + .size() + T(size_t) ctor`) | `contiguous` | `linear_value_dataset` | detection-driven: `T(n)` + `impl::structural_data` + H5Dread |
| ◆ **Custom iterator-only sequence** | `iterators` | `linear_value_dataset` | iterator-staging path round-trips into `std::` counterpart (range-ctor target — generic non-`std::` reconstruction not yet implemented) |
| ◆ **Custom set-shape**, **map-shape** | `iterators` | `linear_value_dataset` / `key_value_dataset` | same caveat: insert path needs `std::` target today |
| `std::vector<bool>`, nested containers, array-of-container, unregistered POD | — | `unsupported` | static_assert fires (correctly) |

---

## Complete Attribute-Side Type Matrix (NEW: parity with dataset side)

`H5Awrite.hpp` and `H5Aread.hpp` now dispatch on the same `kind × storage` matrix as
the dataset side. Each cell is implemented; no silent garbage cells.

| Type | Mechanism |
|---|---|
| `int`, `float`, enums | direct H5Awrite/H5Aread |
| `std::string`, `std::string_view` | `dt_t<char*>` vlen string + H5Awrite/H5Aread + reclaim |
| `std::vector<T>`, `std::array<T,N>`, `T[N]` | direct |
| `std::vector<std::string>` | `char*` relay + H5T_VARIABLE |
| `std::vector<std::vector<T>>` | `hvl_t` relay + H5Tvlen_create + reclaim |
| `std::tuple<Ts...>` (scalar) | `traits::pack` → compound attribute |
| `std::vector<std::tuple<Ts...>>` | `elem_traits::pack` each → compound rank-1 attribute |
| `std::list<std::tuple<...>>` etc. | same |
| `std::pair<K,V>` (scalar) | direct via `dt_t<pair>` |
| `std::map<K,V>` etc. | `kv_t` compound attribute |
| `std::list<T>` etc. with std-layout T | staging vector → H5Awrite/H5Aread |
| `std::complex<T>` | direct via `dt_t<complex>` |
| User aggregate via `H5CPP_REGISTER_STRUCT` | direct via `dt_t<T>` |

---

## Remaining Write-Side Work

### W1 — H5Dgather.hpp: dead dispatch code → issue #277
The seven per-container `gather` overloads and the `vector<string>` char** overload are
no longer called by `H5Dwrite.hpp`'s dispatch path. `test/H5Dgather.cpp` exercises them
directly as a standalone utility API, so they cannot be silently deleted. Cleanup
(deprecation, test migration, or replacement) is deferred to its own issue.

### ~~W2 — `vlen_text_dataset` and `ragged_vlen_dataset` read side~~ — Done

### ~~W3 — `key_value_dataset` read side~~ — Done

### ~~W4 — `storage_representation_impl<T>` completion~~ — Done
Closed. Five explicit specs landed in `H5Tmeta.hpp` for `basic_string<char,...>`,
`basic_string_view<char,...>`, `char*` / `const char*`, and `complex<T>`. The aggregate
fallback is gated on the new `has_registered_compound<T>` trait, set by the
`H5CPP_REGISTER_TYPE_` macro (and therefore by `H5CPP_REGISTER_STRUCT`). The
`storage != unsupported` stopper is now precise.

---

## Remaining Work Map (all layers)

| ID | Item | File | Status |
|---|---|---|---|
| W1 | Remove dead gather overloads — dispatch dead, tests use directly | `H5Dgather.hpp` | Deferred → #277 |
| W2 | vlen_text + ragged_vlen read | `H5Dread.hpp` | ✔ Done |
| W3 | key_value read | `H5Dread.hpp` | ✔ Done |
| W4 | `storage_representation_impl` specs for string/string_view/complex/char*/aggregate + `has_registered_compound<T>` trait | `H5Tmeta.hpp` + `H5Tall.hpp` (macro change) | ✔ Done |
| R1 | Collapse H5Dread pointer/ref overloads to `if constexpr` on `access_traits_t` | `H5Dread.hpp` | ✔ Done |
| R2 | Replace `impl::data/size/get<T>::ctor` legacy calls in H5Dread | `H5Dread.hpp` | ✔ Done |
| T1 | `std::tuple<Ts...>` + `std::pair<K,V>` compound type support | `H5Tmeta.hpp`, `H5Tall.hpp`, `H5Dwrite.hpp`, `H5Dread.hpp` | ✔ Done (absorbed from #274) |
| A1 | Collapse H5Awrite enable_if overloads to `if constexpr` on `access_traits_t` | `H5Awrite.hpp` | ✔ Done |
| A2 | Collapse H5Aread enable_if overloads to `if constexpr` on `access_traits_t` | `H5Aread.hpp`, `H5Tmeta.hpp` | ✔ Done |
| A3 | Attribute matrix parity (vlen_text / ragged_vlen / key_value / composite / iter-composite / iter-staging) | `H5Awrite.hpp`, `H5Aread.hpp` | ✔ Done (new in current bank) |
| C1 | Fold `std::tuple<Ts...>` into `access_traits_t` as `kind = composite`, delete `is_tuple_v` early-exits | `H5Tmeta.hpp` + all four dispatch headers | ✔ Done (new in current bank) |
| C2 | Iter-staging `static_assert(std::is_standard_layout_v<element_t>)` guard | All four dispatch headers | ✔ Done (new in current bank) |
| S1 | Scatter-bypass `static_assert` on ds-overload | `H5Dwrite.hpp`, `H5Dread.hpp` | ✔ Done (new in current bank) |
| S2 | Unsupported-storage `static_assert` on all dispatch entries | All four dispatch headers | ✔ Done (precise after W4) |
| S3 | Nested-container `static_assert` (borrowed from `#274`) | All four dispatch headers | ✔ Done |
| ◆ G1 | `is_set_like` structural fallback in `storage_representation_impl` | `H5Tmeta.hpp` | ✔ Done (current session) |
| ◆ G2 | Read-side structural fallback for custom contiguous T (rank<T>==0 + .data() + T(size_t)) | `H5Dread.hpp`, `H5Mstl.hpp` (`impl::structural_data`) | ✔ Done (current session) |
| ◆ G3 | Pretty-printer veto on `has_scalar_alias<T>` so Eigen non-vector matrices keep their own operator<< | `H5Uall.hpp` | ✔ Done (current session) |
| G4 | Generic read-side reconstruction for custom iterator-only / set / map shapes (insert-target detection) | `H5Dread.hpp`, `H5Aread.hpp` | Open — generic inserter detection needed |
| M3 | `forward_list` size fix in impl::size | `H5Mstl.hpp` | ✔ Done (already in baseline at line 142) |
| M6 | `threaded_pipeline_t` via ring | `H5Zpipeline.hpp` | Gated on sigma dependency decision |
| D1 | `storage_traits_t` vs `dt_t<>` policy — pick canonical, retire the other | `H5Tmeta.hpp`, `H5Tall.hpp` | **Open — separate architectural pass** |
| D2 | Unify twin traits: `impl::decay`/`meta::decay`, `is_contiguous`/`is_transport_contiguous_t`, two `h5::gather` overloads | `H5Mstl.hpp`, `H5Tmeta.hpp`, `H5Dgather.hpp` | **Open — separate refactor pass** |
| D3 | Gateway runtime type-check: reconcile existing-dataset HDF5 type with C++ container type | `H5Dwrite.hpp`, `H5Dread.hpp` | **Open — runtime check** |

---

## Implementation Order

| Step | What | Effort |
|---|---|---|
| 1 | ~~W1: delete dead gather overloads~~ — deferred to own issue | — |
| 2 | ~~M3: forward_list size one-liner~~ — already in H5Mstl.hpp:142 | — |
| 3 | ~~R1+R2+W2+W3: rewire H5Dread all overload groups~~ | — |
| 4 | ~~T1: std::tuple + std::pair compound type support~~ | — |
| 5 | ~~A1+A2: collapse attribute IO overloads~~ | — |
| 6 | ~~A3: attribute matrix full parity~~ — done in current bank | — |
| 7 | ~~C1+C2+S1+S3: tuple fold + iter-staging guard + scatter-bypass + nested-container stoppers~~ — done | — |
| 8 | ~~W4: storage_representation_impl completion + has_registered_compound<T> trait~~ — done | — |
| 9 | ~~S2 verification: confirm `storage != unsupported` is precise after W4~~ — verified | — |
| 10 | D1 / D2 / D3 — separate architectural passes | post-merge |

All steps through 9 complete. Bank is internally consistent and ready for commit slicing.

---

## C++26 Path (unchanged)

`compiler_meta_t<T>` is a C++17 polyfill of `nonstatic_data_members_of(^T)`.
Migration is additive: add `else if constexpr (natively_reflectable<T>)` branch to
`storage_traits_t::create_type()`. Plugin-generated files become optional. No API break.

---

## Architectural items deferred (review report items D.1–D.3)

These were identified in the architecture review of the prior `#276` bank but are
**not** addressed in the current prototype bank. They survive deliberately, scoped to
follow-up issues:

### D.1 — `storage_traits_t` vs `dt_t<>` overlap

`storage_traits_t<T>::create_type()` and the legacy `dt_t<T>` specialization registry
both produce HDF5 type ids. `resolved_type_t<T>` adapts between them. Several types
have entries in both registries (float16, reference_t), others only in one
(tuple, pair, complex have `dt_t<>` only; arithmetic has both via the storage_traits
arithmetic spec and the auto-generated REGISTER_TYPE_ macros).

No policy currently dictates which to extend for new types. Recommended: extend
`storage_traits_t` to be the canonical path, retire `dt_t<>` specializations for tuple
/ pair / complex / float16, keep `dt_t<>` only for the user-facing
`H5CPP_REGISTER_STRUCT` macro.

### D.2 — Twin traits

| Concept | Locations |
|---|---|
| Decay (value_type unwrap) | `h5::impl::decay<T>` (`H5Mstl.hpp:38`) and `h5::meta::decay<T>` (`H5Tmeta.hpp:43`); both with their own `has_explicit_decay` registry |
| Contiguity | `h5::meta::is_contiguous<T>` (`H5Tmeta.hpp:81`, legacy, used by H5Mxxx mappers) and `h5::meta::is_transport_contiguous_t<T>` (`H5Tmeta.hpp:400`, gates `kind = contiguous`) |
| `h5::gather` | `h5::gather<T>(hid_t, string, T&)` for compiler-generated tier-2 (`H5Dscatter.hpp:106`) and `h5::gather(container, vector<E>&)` for L4 pointer collection (`H5Dgather.hpp`) — share namespace and ADL set |

Each pair must stay synchronized by hand. Refactor pass should pick one canonical name
per concept and migrate call sites.

### D.3 — Gateway runtime type check

`h5::write(fd, path, ref)` and `h5::read(fd, path, ref)` open an existing dataset
without verifying its HDF5 type matches the would-be `resolved_type_t<T>`. A user
writing `std::vector<std::string>` into a dataset previously created as
`H5T_NATIVE_DOUBLE` gets an opaque HDF5 error rather than a typed exception with both
type names. Runtime check, modest scope.

---

## One-Line Summary

> All four IO headers wired via `access_traits_t<T>` `kind × storage_representation_v<T>`
> `if constexpr` dispatch. `std::tuple` folded into `access_traits_t` as a new
> `kind = composite` with `pack`/`unpack` interface — no more `is_tuple_v` early-exits.
> Attribute side has full matrix parity. Four compile-time stoppers in place
> (scatter-bypass, unsupported-storage, nested-container, iter-staging std-layout)
> and all four are now precise: `storage_representation_impl<T>` covers every legitimate
> scalar object/text type, and `has_registered_compound<T>` lets the aggregate fallback
> distinguish registered from unregistered POD aggregates. ◆ Detection-idiom coverage
> extended: `is_set_like` structural fallback + read-side structural ctor + structural
> data accessor now land any third-party container with the right surface (vec /
> flist / set / dict / abseil / boost.container / folly), with the read side
> round-tripping into the custom type for contiguous vector-shapes and via the
> `std::` counterpart for the iterator-only paths. Bank ready for commit slicing.
