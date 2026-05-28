@page reports_hdf5_112_feature_report h5cpp HDF5 1.12 Feature Delta Report

**Date:** 2026-05-14  
**Scope:** HDF5 1.12 series features introduced after the HDF5 1.10 line, with h5cpp architectural implications  
**Primary sources:**  
- HDF Group, HDF5 1.12 release-specific information: https://support.hdfgroup.org/documentation/hdf5/latest/rel_spec_112.html  
- HDF Group, HDF5 1.12 release-to-release software changes: https://portal.hdfgroup.org/documentation/hdf5/latest/rel_spec_112_change.html  
- HDF Group, HDF5 1.12.0 release notes: https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.12/hdf5-1.12.0/src/hdf5-1.12.0-RELEASE.txt  

---

## Executive Summary

HDF5 1.12 is best treated as a **connector and identity model modernization release**, not just a minor storage-feature upgrade over HDF5 1.10.

The architectural change that matters most to h5cpp is the move from native-file-address assumptions toward **VOL-compatible object identity**:

- HDF5 1.12 introduces the **Virtual Object Layer (VOL)**.
- Object identity moves from address-centric APIs toward **`H5O_token_t`**.
- References are redesigned to support attributes, cross-file targets, explicit destruction, and richer query/open operations.
- Dataspace selection APIs become substantially more capable.
- S3 and HDFS VFDs make file-access-property configuration more strategically important.

For h5cpp, the safe path is **version-gated adapter layers**, not exposing raw 1.12 APIs directly through existing 1.10-era wrappers.

---

## 1. Virtual Object Layer (VOL)

### What changed

HDF5 1.12 introduced the Virtual Object Layer. VOL is an abstraction layer that intercepts HDF5 object-model operations and forwards them to connector plugins. A connector may store data in native HDF5 files, object storage, remote services, or other backend formats while preserving the HDF5 data model at the API level.

### Why it matters

The VOL model weakens assumptions that are natural in native HDF5:

- An object may not have a meaningful native file address.
- Object metadata may not be represented by native object-header structures.
- Some operations may be optional connector capabilities rather than universally supported native-file operations.

### h5cpp implication

h5cpp should keep ordinary dataset/group/attribute APIs backend-neutral where possible. Any API that assumes native HDF5 layout should be named or guarded accordingly.

Recommended direction:

| Layer | Recommendation |
|-------|----------------|
| Core object wrappers | Avoid storing or exposing native addresses as stable identity. |
| Introspection | Split generic object info from native-file info. |
| Property lists | Add explicit FAPL support for VOL connector setup only when there is a concrete use case. |
| Error handling | Treat connector-unsupported operations as a first-class failure mode. |

---

## 2. Object Tokens Replace Address-Centric Identity

### What changed

HDF5 1.12 introduced `H5O_token_t` as a VOL-compatible object identity token. This replaces or supersedes address-based identity in newer APIs.

Important new or changed APIs include:

| Area | 1.12 API direction |
|------|--------------------|
| Link info and traversal | `H5Lget_info2`, `H5Literate2`, `H5Lvisit2`, and related `_by_name` / `_by_idx` forms |
| Object info and traversal | `H5Oget_info3`, `H5Ovisit3`, and related variants |
| Object open by identity | `H5Oopen_by_token` |
| Token utilities | `H5Otoken_cmp`, `H5Otoken_to_str`, `H5Otoken_from_str` |
| Native-only details | `H5Oget_native_info`, `H5Oget_native_info_by_name`, `H5Oget_native_info_by_idx` |

### Why it matters

In HDF5 1.10, code could often treat object address information as a practical identity or traversal primitive. In HDF5 1.12, that assumption is no longer portable across VOL connectors.

### h5cpp implication

Any h5cpp code that traverses groups, prints object identity, caches object locations, or compares object identity should be reviewed for address assumptions.

Recommended direction:

```cpp
#if H5_VERSION_GE(1,12,0)
// Use token-aware H5L/H5O APIs.
#else
// Preserve the existing 1.10-compatible address-backed path.
#endif
```

Do not silently change public behavior for HDF5 1.10 consumers. Add adapter helpers first, then migrate call sites.

---

## 3. Reference System Redesign

### What changed

HDF5 1.12 extended references to support:

- Attribute references.
- Object and dataset-region references that may target another HDF5 file.
- Explicit reference destruction via `H5Rdestroy`.
- Reference copy/equality/query/open operations.

Important APIs include:

| Capability | APIs |
|------------|------|
| Create references | `H5Rcreate_object`, `H5Rcreate_region`, `H5Rcreate_attr` |
| Destroy/copy/equality | `H5Rdestroy`, `H5Rcopy`, `H5Requal` |
| Query | `H5Rget_type`, `H5Rget_file_name`, `H5Rget_obj_name`, `H5Rget_attr_name`, `H5Rget_obj_type3` |
| Open targets | `H5Ropen_object`, `H5Ropen_region`, `H5Ropen_attr` |

`H5Dvlen_reclaim` was deprecated in favor of `H5Treclaim`, which also applies to internally allocated buffers from new reference types.

### Why it matters

References now have stronger lifetime and ownership implications. Old-style "opaque reference value" handling is not enough for the 1.12 model.

### h5cpp implication

The h5cpp reference layer should treat 1.12 references as RAII-managed resources.

Recommended direction:

| Concern | Recommendation |
|---------|----------------|
| Lifetime | Add a small reference wrapper with destructor calling `H5Rdestroy` under HDF5 >= 1.12. |
| Reclaim | Prefer `H5Treclaim` where available for vlen/reference read buffers. |
| Compatibility | Keep old `hobj_ref_t` / `hdset_reg_ref_t` paths version-gated for HDF5 1.10. |
| Scope | Do not mix reference modernization with dataset I/O refactors. |

---

## 4. Dataspace Selection and Hyperslab Improvements

### What changed

HDF5 1.12 added selection APIs that make it easier to combine, adjust, project, iterate, and compare dataspace selections.

Important APIs include:

| Capability | APIs |
|------------|------|
| Combine/refine selections | `H5Scombine_hyperslab`, `H5Scombine_select`, `H5Smodify_select` |
| Move/project selections | `H5Sselect_adjust`, `H5Sselect_project_intersection` |
| Compare/check selections | `H5Sselect_shape_same`, `H5Sselect_intersect_block` |
| Iterate selections | `H5Ssel_iter_create`, `H5Ssel_iter_get_seq_list`, `H5Ssel_iter_reset`, `H5Ssel_iter_close` |

HDF5 1.12 also reworked hyperslab selection internals for major performance improvements.

### Why it matters

These APIs are a better foundation for complex gather/scatter, chunk projection, virtual layout, and high-throughput slicing than hand-rolled selection arithmetic.

### h5cpp implication

This is the most immediately useful 1.12 feature area for h5cpp's existing direction.

Recommended direction:

1. Add internal selection helper wrappers under `h5cpp/H5Sall.hpp` or a focused `H5Sselect.hpp`.
2. Keep public API surface minimal until gather/materialize semantics settle.
3. Use the helpers behind dataset slicing, chunk-aware transfer, or future high-throughput paths.
4. Preserve HDF5 1.10 fallback behavior.

---

## 5. Encoding Format Updates

### What changed

HDF5 1.12 introduced updated encoding functions:

| Old compatibility path | New 1.12 path |
|------------------------|---------------|
| `H5Sencode1` | `H5Sencode2` |
| `H5Pencode1` | `H5Pencode2` |

The newer encoding supports 64-bit selection encodings and dataspace selections tied to a file.

### h5cpp implication

Any future h5cpp serialized-dataspace or serialized-property-list helper should version-gate these functions. Do not call the compatibility macro blindly if stable cross-version output matters.

---

## 6. New S3 and HDFS Virtual File Drivers

### What changed

HDF5 1.12.0 added VFD support for:

- AWS S3 access.
- Apache HDFS access.

These are configured through file-access-property-list machinery rather than ordinary path-only file open calls.

### h5cpp implication

Cloud/object-store access should be modeled as explicit FAPL configuration, not hidden behind `h5::open("s3://...")` until the semantics are clear.

Recommended direction:

| Concern | Recommendation |
|---------|----------------|
| API shape | Prefer explicit FAPL builder/helpers. |
| Dependencies | Keep libcurl/OpenSSL/HDFS requirements outside the baseline h5cpp dependency path. |
| Testing | Gate tests on HDF5 build features and credentials/environment. |
| Portability | Treat S3/HDFS VFDs as optional integration targets, not core I/O behavior. |

---

## 7. Other 1.12 Series Additions

### 1.12.0 additions

| API | Purpose |
|-----|---------|
| `H5Fdelete` | Delete an HDF5 file. |
| `H5Fget_fileno` | Retrieve a unique file number for an open file. |
| `H5Fis_accessible` | Check whether a file can be opened with a given FAPL. |
| `H5Iiterate` | Iterate identifiers of a specified type. |
| `H5Pset_vol`, `H5Pget_vol_id`, `H5Pget_vol_info` | Configure/query VOL connector use. |
| `H5VL*` connector APIs | Register, unregister, inspect, and close VOL connectors. |

### 1.12.1 additions

| API | Purpose |
|-----|---------|
| `H5Pset_fapl_splitter`, `H5Pget_fapl_splitter` | Configure/query splitter VFD. |
| `H5Pset_file_locking`, `H5Pget_file_locking` | Configure/query file locking. |
| `H5get_free_list_sizes` | Inspect memory free-list sizes. |
| `H5Ssel_iter_reset` | Reset dataspace selection iterator. |
| `H5VLquery_optional` | Query whether a VOL connector supports an optional operation. |

### 1.12.2 additions

| API | Purpose |
|-----|---------|
| `H5DSwith_new_ref` | Detect whether dimension scales use new references. |
| `H5LTget_attribute_ullong`, `H5LTset_attribute_ullong` | Read/write unsigned long long attributes via high-level helpers. |
| `H5VLobject_is_native` | Determine whether an object ID represents a native VOL connector object. |

---

## 8. Compatibility Risk for h5cpp

| Risk | Why it matters | Recommended handling |
|------|----------------|----------------------|
| Macro remapping of APIs | HDF5 1.12 maps legacy names to newer variants by default in some cases. | Prefer explicit versioned calls inside compatibility wrappers. |
| Object identity semantics | `haddr_t` is not portable across VOL connectors. | Use tokens under HDF5 >= 1.12; isolate native-address helpers. |
| Reference lifetime | New references may need explicit destruction. | Add RAII reference wrappers before broad reference use. |
| Native introspection | VOL objects may not expose native file-format details. | Keep native info APIs separate from generic object info. |
| Optional connectors | VOL/VFD features depend on HDF5 build configuration and runtime plugins. | Feature-test and gate at CMake/runtime boundaries. |
| File-format compatibility | Some 1.12 features create files unreadable by HDF5 1.10 and earlier. | Avoid enabling 1.12-only file-format features implicitly. |

---

## Recommended h5cpp Work Plan

### Phase 1: Compatibility scaffolding

Add a small internal HDF5-version adapter layer for:

- `H5O_token_t` versus address-backed object identity.
- `H5L*` traversal API variants.
- `H5O*` info/traversal API variants.
- `H5Sencode1`/`H5Sencode2` and `H5Pencode1`/`H5Pencode2`.
- `H5Dvlen_reclaim` versus `H5Treclaim`.

Keep this internal and test-only at first.

### Phase 2: Reference modernization

Create a narrow RAII reference wrapper for HDF5 >= 1.12:

- Own/destroy new references.
- Copy with `H5Rcopy`.
- Compare with `H5Requal`.
- Open referenced object/region/attribute through explicit methods.

Preserve the HDF5 1.10 path.

### Phase 3: Selection helper layer

Wrap the new H5S selection APIs behind internal helpers:

- Combine selections.
- Project intersections.
- Iterate selection sequences.
- Compare selection shape.

Use this as groundwork for gather/materialize and chunk-aware I/O, not as a public API expansion on day one.

### Phase 4: Optional connector/FAPL work

Add explicit FAPL-level entry points for:

- File locking configuration.
- VOL connector selection/query.
- Splitter VFD.
- S3/HDFS VFDs only if HDF5 was built with the required support.

Avoid making cloud paths magical. The API should show that these are configured storage backends.

---

## Bottom Line

HDF5 1.12 matters to h5cpp in four places:

1. **Object identity:** tokens replace native address assumptions.
2. **References:** new references need RAII and explicit version handling.
3. **Selections:** new H5S APIs can simplify future high-throughput slicing and gather/scatter work.
4. **Connectors:** VOL and new VFDs require explicit property-list and feature-gating boundaries.

The best h5cpp move is not to bump the minimum HDF5 version immediately. Keep HDF5 1.10 as the baseline, then add 1.12-aware internal adapters where they remove real compatibility risk or unlock useful selection/reference functionality.

## Related examples

- [`examples/reference/reference.cpp`](../../../examples/reference/reference.cpp) — HDF5 1.12 reference handle (rule-of-five RAII)
- [`examples/mdspan/mdspan.cpp`](../../../examples/mdspan/mdspan.cpp) — C++23 std::mdspan round-trip (gated on libstdc++ ≥ 15)
