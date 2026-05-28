@page reports_handle_inventory H5CPP Handle Inventory & Pretty-Print Status

**Status as of HEAD on `prototype` branch.** Scope: every RAII handle wrapper h5cpp ships — file, dataset, attribute, group, object, dataspace, datatype, 14 property-list variants, and the 5 async-mode counterparts.

All handle types are aliases of the common base
`h5::impl::detail::hid_t<T, capi_close, FromCapi, ToCapi, Kind>` defined in
`h5cpp/H5Iall.hpp:50-372`. The macros `H5CPP__defhid_t / __defpid_t /
__defdid_t / __defaid_t` at `H5Iall.hpp:325-343` stamp out the typedefs.

## Type-system layout

| Discriminator      | Values                                                          |
| ------------------ | --------------------------------------------------------------- |
| `T`                | empty tag struct per handle family (`impl::fd_t`, `impl::ds_t`, …)  |
| `capi_close`       | the `H5*close` function pointer that runs in the destructor      |
| `FromCapi / ToCapi`| classic = `true,true` (operator from/to `::hid_t` available); async = `false,false` (deleted) |
| `Kind`             | `hdf5::any` (0) / `property` (1) / `type` (2) / `dataset` (4) / `attribute` (5) — extra members per category |

The classic variants store `::hid_t handle` as **protected**; async variants
(false,false) store it as **public** because their `operator ::hid_t()` is
`= deleted`. Generic printer code must dispatch on `if constexpr (FromCapi && ToCapi)`
to choose between `static_cast<::hid_t>(h)` and `h.handle` — see the
`h5::impl::detail::operator<<(...)` template in `H5cout.hpp` for the pattern.

## Handle inventory — full list

Status legend: ✔ ok = dedicated specialization with deep introspection; ◇ partial = generic printer only (handle id + validity + refcount).

### Object handles (RAII over an HDF5 id)

| Handle      | Close fn   | Kind tag                | Pretty-print status                                                                                       |
| ----------- | ---------- | ----------------------- | --------------------------------------------------------------------------------------------------------- |
| `h5::fd_t`  | `H5Fclose` | `hdf5::any`             | ✔ ok — path / mode / libver bounds / file size                                                            |
| `h5::ds_t`  | `H5Dclose` | `hdf5::dataset`         | ✔ ok — path / dtype class+size / rank / dims / layout / chunk dims / filter count / attribute count       |
| `h5::at_t`  | `H5Aclose` | `hdf5::attribute`       | ✔ ok — name / dtype class / rank                                                                          |
| `h5::gr_t`  | `H5Gclose` | `hdf5::attribute`*      | ✔ ok — path / child count / attribute count                                                               |
| `h5::ob_t`  | `H5Oclose` | `hdf5::any`             | ◇ partial — generic only (could grow type=group/dataset/named-type discriminator + path)                  |
| `h5::sp_t`  | `H5Sclose` | `hdf5::any`             | ✔ ok — rank / current+max dims / selection bounds / validity / hyperslab block list (long-standing)        |

\* Group uses the `aid_t` (attribute-id) variant because it carries the same
attribute subscript machinery as datasets/attributes.

### Property-list handles (`H5Pclose`)

All 14 share `Kind = hdf5::property`. The classic variants implement the `|`
composition operator that lets you chain `h5::chunk{...} | h5::gzip{6}`.

| Handle      | Role                          | Pretty-print status                                                                                       |
| ----------- | ----------------------------- | --------------------------------------------------------------------------------------------------------- |
| `h5::acpl_t` | attribute creation            | ◇ partial — generic only (char encoding + creation-order tracking would be the natural extension)         |
| `h5::dapl_t` | dataset access                | ◇ partial — generic only (chunk_cache / virtual_view / efile_prefix / high_throughput flag)               |
| `h5::dxpl_t` | data transfer                 | ✔ ok — handle id + MPI mode decoder when `H5_HAVE_PARALLEL` (long-standing)                                |
| `h5::dcpl_t` | dataset creation              | ✔ ok — layout / chunk dims / alloc time / fill value status / filter pipeline (deflate, shuffle, …)        |
| `h5::tapl_t` | datatype access               | ◇ partial — generic only (rarely tuned)                                                                   |
| `h5::tcpl_t` | datatype creation             | ◇ partial — generic only (rarely tuned)                                                                   |
| `h5::fapl_t` | file access                   | ✔ ok — driver id / libver bounds / cache (mdc + rdcc) / alignment threshold                                |
| `h5::fcpl_t` | file creation                 | ◇ partial — generic only (user block size, symbol-table B-tree parameters)                                |
| `h5::fmpl_t` | file mount                    | ◇ partial — generic only (rarely tuned)                                                                   |
| `h5::gapl_t` | group access                  | ◇ partial — generic only (rarely tuned)                                                                   |
| `h5::gcpl_t` | group creation                | ◇ partial — generic only (link-storage thresholds, creation-order tracking)                               |
| `h5::lapl_t` | link access                   | ◇ partial — generic only (external-link prefix, NLinks follow limit)                                      |
| `h5::lcpl_t` | link creation                 | ◇ partial — generic only (char encoding, create_intermediate_group flag)                                  |
| `h5::ocrl_t` | object copy                   | ◇ partial — generic only (without_attr, expand_soft_link, expand_ext_link, …)                              |
| `h5::ocpl_t` | object creation               | ◇ partial — generic only (obj_track_times, attr_phase_change)                                             |
| `h5::scpl_t` | string creation               | ◇ partial — generic only (rarely tuned)                                                                   |

### Datatype handles

| Handle              | Pretty-print status                                                                                                                                                                                                                                                                                                                                                                |
| ------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `h5::dt_t<T>`        | ◇ partial — generic only. Has its own specialization point in `H5Tall.hpp:50-75` (per-T `operator dt_t()` constructor) rather than the macro-generated typedef route. A meaningful printer would switch on `H5Tget_class` (INTEGER / FLOAT / STRING / COMPOUND / ARRAY / VLEN / REFERENCE / ENUM / OPAQUE) and recurse into compound member layouts. This is the highest-effort specialization to write and the most useful for debugging custom POD round-trips. |

### Argument-array handles (rank+dims types — not "handles" strictly, but printable)

`h5::impl::array<T>` (defined in `H5Sall.hpp`) is the shared base for these
fixed-rank descriptors. All print as `{a,b,c,...}`; rank-0 prints `{n/a}`;
dimensions equal to `hsize_t-max` print `inf`.

| Type                      | Tag                  | Pretty-print status |
| ------------------------- | -------------------- | ------------------- |
| `h5::offset_t`             | `impl::offset`        | ✔ ok                |
| `h5::count_t`              | `impl::count`         | ✔ ok                |
| `h5::stride_t`             | `impl::stride`        | ✔ ok                |
| `h5::block_t`              | `impl::block`         | ✔ ok                |
| `h5::current_dims_t`       | `impl::current_dims`  | ✔ ok                |
| `h5::max_dims_t`           | `impl::max_dims`      | ✔ ok                |

### Async-mode variants (Phase II)

Five async wrappers shadow the classic counterparts; defined in
`H5Iall.hpp:353-359`. Same underlying `hid_t<T, capi_close, false, false, Kind>`
template but with `operator ::hid_t() = delete` so async descriptors can't
slip into raw HDF5 C API calls. Each carries a `std::shared_ptr<executor_t>`
field for the worker-pool dispatch.

| Handle            | Classic counterpart | Pretty-print status                                                                          |
| ----------------- | ------------------- | -------------------------------------------------------------------------------------------- |
| `h5::async::fd_t`  | `h5::fd_t`           | ◇ partial — generic only (via `h.handle` direct access since `operator ::hid_t()` is deleted) |
| `h5::async::ds_t`  | `h5::ds_t`           | ◇ partial — generic only                                                                      |
| `h5::async::at_t`  | `h5::at_t`           | ◇ partial — generic only                                                                      |
| `h5::async::gr_t`  | `h5::gr_t`           | ◇ partial — generic only                                                                      |
| `h5::async::ob_t`  | `h5::ob_t`           | ◇ partial — generic only                                                                      |

A future pass that mirrors the six classic specializations onto the async
variants is mechanical — same HDF5 introspection calls, just read the raw
`handle` field instead of casting. The `class_tag<T>()` mechanism already
gives them the right name in the generic printer (`h5::fd_t`, `h5::ds_t`, …)
since they share the impl tag struct with their classic counterparts.

## Pretty-print machinery (what shipped)

Location: `h5cpp/H5cout.hpp` (extended this pass).

| Layer | Where | What it provides |
| ----- | ----- | ----------------- |
| Tag dictionary | `H5cout.hpp:138-172` | `class_tag<T>()` per `impl::*` tag struct via the `H5CPP__class_tag(T_, NAME_)` macro. All 22 tag structs (`fd_t`, `ds_t`, `at_t`, `gr_t`, `ob_t`, `sp_t`, `acpl_t`, `dapl_t`, `dxpl_t`, `dcpl_t`, `tapl_t`, `tcpl_t`, `fapl_t`, `fcpl_t`, `fmpl_t`, `gapl_t`, `gcpl_t`, `lapl_t`, `lcpl_t`, `ocrl_t`, `ocpl_t`, `scpl_t`) are registered. |
| Generic printer | `H5cout.hpp:174-194` | `template <T, F, From, To, K> operator<<(ostream&, const hid_t<T,F,From,To,K>&)` — handle id + validity + refcount. Works for classic and async. Async-aware via `if constexpr (FromCapi && ToCapi)` branch. |
| 6 specializations | `H5cout.hpp:197-454` | `fd_t`, `ds_t`, `gr_t`, `at_t`, `dcpl_t`, `fapl_t` — see status table above. All defined in `h5::impl::detail` (the namespace ADL searches for the underlying template). |
| Live since 2018 | `H5cout.hpp:15-110` | `dxpl_t` (handle + MPI mode), `sp_t` (rank/dims/selection), `impl::array<T>` (offset/count/stride/block/dims). |

## Demo

`examples/cout/cout.cpp` exercises all of the above. Sample output of stage 4
("handle printers") against a representative file:

```
fd_t   : h5::fd_t{ handle=72057594037927936 path='cout.h5' mode=RDWR libver=[0,3] size=4096 }
ds_t   : h5::ds_t{ handle=360287970189639682 path='/grid' dtype=FLOAT elsize=8 rank=2 dims={10,20} layout=CONTIGUOUS filters=0 attrs=0 }
gr_t   : h5::gr_t{ handle=144115188075855872 path='/sensors' children=2 attrs=1 }
at_t   : h5::at_t{ handle=504403158265495553 name='units' dtype=STRING rank=0 }
dcpl_t : h5::dcpl_t{ handle=792633534417207391 layout=CHUNKED chunk={4,8} alloc=INCR fill=default filters=[deflate(1)] }
fapl_t : h5::fapl_t{ handle=792633534417207392 driver_id=576460752303423488 libver=[1,3] cache={mdc=0 rdcc_slots=521 rdcc_bytes=1048576 w0=0.75} align={threshold=1 align=1} }
```

## ADL gotcha (worth documenting once)

Type aliases in `namespace h5` (`h5::fd_t = h5::impl::hid_t<...>`) **do not**
introduce `namespace h5` into the associated-namespace set for ADL on the
alias. The actual associated namespaces are those of the underlying template
type: `h5::impl::detail` (where `hid_t<...>` is defined) and `h5::impl`
(where the tag struct `fd_t` lives). Free-function overloads that need to be
found via ADL on `h5::fd_t` must live in `h5::impl::detail` (or `h5::impl`),
**not** in `h5`.

This is why the 6 specializations live in `namespace h5::impl::detail` rather
than the more natural-looking `namespace h5`. Putting them in `h5` results in
the generic template winning every overload resolution because the
specializations were invisible.

## Constraints to know about

| Constraint | Where it lives | Why it matters |
| ---------- | -------------- | -------------- |
| Classic `handle` is protected | `H5Iall.hpp:122-123` | Generic printer can't `h.handle` directly on classic variants; uses `static_cast<::hid_t>(h)` via the explicit `operator ::hid_t()`. |
| Async `operator ::hid_t()` is deleted | `H5Iall.hpp:99-101` | Generic printer can't `static_cast<::hid_t>(h)` on async variants; reads `h.handle` directly (it's public there). |
| Mute/unmute discipline | `h5::mute()` / `h5::unmute()` in `H5Eall.hpp` | Every printer must bracket its HDF5 introspection calls so invalid or `H5I_UNINIT` handles don't dump HDF5 error stacks into the output. All six specializations follow this. |
| HDF5 work per print | `H5Iget_name`, `H5Sget_simple_extent_dims`, `H5Pget_chunk`, `H5Pget_filter2`, `H5Oget_info3`, … | Printers are not free. Fine for debug logs; avoid in tight loops. |
| ADL namespace placement | `h5::impl::detail` | See section above. Don't move printers to `namespace h5`. |
| `H5VLget_driver_name` missing in 1.12 | `H5Pall.hpp` history | The `fapl_t` printer originally tried this; pulled it. Driver id alone is shown (`driver_id=N`). 1.14 has the symbol; revisit when the project drops 1.12 support. |
| `H5_HAVE_ROS3_VFD` + `H5_VERSION_GE(1,12,1)` exposes a latent build break | `H5Pall.hpp:419` references `H5FD_ros3_fapl_t::session_token` which doesn't exist in some 1.12.x builds. Unrelated to the printer work but discovered during the pass — track separately. |

## Recommended next pass (if/when scope grows)

In rough priority order:

1. **`h5::dt_t<T>`** — type-class-dispatched printer (INTEGER / FLOAT / STRING / COMPOUND / ARRAY / VLEN / REFERENCE / ENUM / OPAQUE) with member-list recursion for compounds. Biggest debugging payoff after the six already landed.
2. **`h5::ob_t`** — `H5Oget_info3`-based type discriminator (`H5O_TYPE_GROUP` / `H5O_TYPE_DATASET` / `H5O_TYPE_NAMED_DATATYPE`) + path. Cheap to add.
3. **`h5::async::*`** specializations — mirror the six classic specializations, replacing `static_cast<::hid_t>(h)` with `h.handle`. Add a `[async]` flag in the output so log diffs can distinguish.
4. **`h5::dapl_t`** — chunk cache config, virtual view, `efile_prefix`, `high_throughput` flag. Useful for tuning chunked reads.
5. **`h5::lcpl_t`** — char encoding (ASCII vs UTF-8) + create_intermediate_group flag. Surfaces the UTF-8 default that's currently invisible.

The remaining 8 property lists (`acpl_t`, `tapl_t`, `tcpl_t`, `fcpl_t`,
`fmpl_t`, `gapl_t`, `gcpl_t`, `ocrl_t`, `ocpl_t`, `scpl_t`) carry almost no
commonly-tuned state and the generic printer suffices.

## Files touched in this pass

| File | Change |
| ---- | ------ |
| `h5cpp/H5cout.hpp` | +332 lines: tag dictionary, generic printer, 6 specializations (fd_t / ds_t / gr_t / at_t / dcpl_t / fapl_t), ADL namespace placement |
| `examples/cout/cout.cpp` | +43 lines: new stage 4 exercising all six handle printers |
| `examples/cout/README.md` | (pending refresh to reflect the new handle-printer section) |

## Build state

| Target | Status |
| ------ | ------ |
| `examples-cout` | ✔ ok — all 5 stages exercise cleanly, all 6 specializations fire |
| `examples-string` / `examples-reference` / `examples-sparse-arma` / `examples-sparse-eigen` / `examples-transform` / `examples-utf` / `examples-mdspan` | ✔ ok — no regressions from the H5cout.hpp changes |

## Related examples

- [`examples/cout/cout.cpp`](../../../examples/cout/cout.cpp) — handle pretty-printing demo, 5 stages from raw hid_t through deep `fd_t`/`ds_t`/`gr_t`/`at_t`/`dcpl_t`/`fapl_t` formatters
- [`examples/reference/reference.cpp`](../../../examples/reference/reference.cpp) — `h5::reference_t` lifecycle + region references
