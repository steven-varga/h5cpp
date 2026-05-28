@page aliases_reference Doxygen alias vocabulary

H5CPP uses a controlled vocabulary of Doxygen aliases so that repeated `@param` / `@tparam` / `@return` / `@see` fragments are defined **once** in `docs/links/*.txt` and referenced everywhere with a `\name` token.

One source of truth. A wording change in one file propagates automatically to every overload that uses the alias.

Use this page as the developer reference when writing or refactoring docstrings, and as the project reference for what each `\alias` token in the generated API site means.



## How the mechanism works

The Doxyfile pulls in four alias-definition files via `@INCLUDE_PATH` and `@INCLUDE`:

| File | Purpose |
|---|---|
| `docs/links/h5cpp.txt` | the core h5cpp alias set — parameters, returns, see-also bundles, function-group headers |
| `docs/links/hdf5.txt` | HDF5 C-API cross-reference bundles |
| `docs/links/linalg.txt` | Armadillo / Eigen / Blitz / Blaze / dlib / uBLAS cross-reference bundles |
| `docs/links/stl.txt` | `std::vector` / `std::array` / `std::string` / `std::mdspan` cross-reference bundles |

When Doxygen processes a docstring, every `\name` token is expanded inline into the alias's full text. A docstring carrying

```text
\\par_file_path \\par_dataset_path \\par_offset \\par_stride \\par_count \\par_block \\par_dxpl \\tpar_T \\returns_object \\sa_h5cpp \\sa_stl
```

(double-backslash here is a display artefact so this guide can show the syntax literally — write a single `\` in your actual docstrings.) Expands into eleven fully-formed `@param` / `@tparam` / `@return` / `@see` blocks at API-reference generation time.



## Standard docstring template

The canonical shape of an h5cpp API docstring:

```text
/** \\func_read_hdr
 *  One-line summary of what this overload does.
 *
 *  Optional 2–3 sentences elaborating function-specific content. Keep prose
 *  to what's unique about this overload — boilerplate (param descriptions,
 *  return types, exception semantics) comes from aliases.
 *
 *  Embedded example block goes here using Doxygen's code / endcode pair.
 *
 *  \\par_file_path \\par_dataset_path \\par_offset \\par_stride \\par_count \\par_block \\par_dxpl \\tpar_T \\returns_object
 *  \\sa_h5cpp \\sa_stl
 */
```

(Double-backslashes in the template above are a display artefact — write a single `\` in your actual docstrings.)

Conventions:

- The chained-aliases line goes at the **end** of the comment, after any code block.
- One line if it fits in ~120 columns; otherwise one alias per line.
- Order: `par_*` (in declaration order) then `tpar_*` then `returns_*` then `sa_*` aliases.
- The `func_*_hdr` family (one per overload group) goes on the **first** line of the comment.



## Vocabulary catalog

### Parameters

| Alias | Expands to (summary) |
|---|---|
| `\par_file_path` | `@param file_path` — path to the HDF5 container in the OS file system |
| `\par_dataset_path` | `@param dataset_path` — POSIX-style path within the container |
| `\par_attr` | `@param attr` — UTF-8 attribute name (relative to parent object) |
| `\par_fcrt_flags` | `@param flags` — `H5F_ACC_TRUNC` / `EXCL` / `DEBUG` |
| `\par_fopn_flags` | `@param flags` — `H5F_ACC_RDWR` / `RDONLY` |
| `\par_fd` | `@param fd` — open `h5::fd_t` file descriptor |
| `\par_ds` | `@param ds` — open `h5::ds_t` dataset descriptor |
| `\par_sp` | `@param sp` — valid `h5::sp_t` dataspace descriptor |
| `\par_handle` | `@param handle` — any `h5::hid_t<T>` descriptor (`fd_t` / `ds_t` / `gr_t` / `at_t` / `dcpl_t` / `fapl_t`) |
| `\par_ref_t` | `@param ref` — valid `h5::reference_t` handle (RAII) |
| `\par_fcpl` | `@param fcpl` — file creation property list |
| `\par_fapl` | `@param fapl` — file access property list (driver, parallel I/O) |
| `\par_dapl` | `@param dapl` — dataset access property list |
| `\par_dcpl` | `@param dcpl` — dataset creation property list |
| `\par_dxpl` | `@param dxpl` — data transfer property list |
| `\par_lcpl` | `@param lcpl` — link creation property list |
| `\par_ref` | `@param ref` — linalg object or `std::vector<T>` |
| `\par_ptr` | `@param ptr` — caller memory region |
| `\par_offset` | `@param offset` — first-element coordinates in file space |
| `\par_stride` | `@param stride` — step between selected elements |
| `\par_count` | `@param count` — blocks per dimension |
| `\par_block` | `@param block` — block size per dimension |
| `\par_max_dims` | `@param max_dims` — max extent; `H5S_UNLIMITED` for extendable |
| `\par_current_dims` | `@param current_dims` — initial extent |
| `\par_chunk_dims` | `@param chunk` — chunked-storage chunk shape |
| `\par_deflate` | `@param deflate` — gzip level 0–9 |
| `\par_thread_count` | `@param N` — FAPL worker-pool size (0 disables) |
| `\par_args` | `@param args[, ...]` — variadic context-sensitive optionals for dataset I/O (offset / stride / count / block / dxpl / current_dims / max_dims / dcpl / lcpl …) |
| `\par_args_attr` | `@param args[, ...]` — variadic context-sensitive optionals for attribute I/O (`h5::current_dims`, `h5::acpl_t`); offset / stride / count / block / dxpl / max_dims / dcpl / lcpl do **not** apply (no chunking, no partial I/O) |
| `\par_mdspan` | `@param view` — `std::mdspan` over caller storage |
| `\par_sparse` | `@param matrix` — `arma::SpMat` / `Eigen::SparseMatrix` (CSC layout) |

### Template parameters

| Alias | Expands to |
|---|---|
| `\tpar_T` | `@tparam T` — supported elementary type, registered compound, or linalg object |
| `\tpar_FD` | `@tparam fd_t` — `hid_t` or `h5::fd_t` |
| `\tpar_DS` | `@tparam ds_t` — `hid_t` or `h5::ds_t` |
| `\tpar_AP` | `@tparam fcpl_t` — `h5::fcpl_t` / `hid_t` / `H5P_DEFAULT` |
| `\tpar_CP` | `@tparam fapl_t` — `h5::fapl_t` / `hid_t` / `H5P_DEFAULT` |

### Returns

| Alias | Expands to |
|---|---|
| `\returns_fd` | `@return` — open `h5::fd_t`; throws `h5::error` |
| `\returns_ds` | `@return` — open `h5::ds_t`; throws `h5::error` |
| `\returns_err` | `@return` — `void`; errors via `h5::error` exceptions |
| `\returns_object` | `@return` — fully-constructed object of type `T` |
| `\returns_string_vec` | `@return` — `std::vector<std::string>` |
| `\returns_gr` | `@return` — `h5::gr_t` for on-disk group layout |
| `\returns_ref` | `@return` — `h5::reference_t` handle |
| `\returns_paths` | `@return` — `std::vector<std::string>` of object paths |

### See-also bundles

| Alias | Expands to |
|---|---|
| `\sa_h5cpp` | h5cpp entry points: `open` / `read` / `write` / `fd_t` / `ds_t` / `err_t` |
| `\sa_exception` | `@exception std::runtime_error if the dataset or file is not found` |
| `\sa_conversion` | h5cpp ↔ HDF5 implicit conversion explainer |
| `\sa_hdf5` | HDF5 C-API cross-references (defined in `docs/links/hdf5.txt`) |
| `\sa_linalg` | linalg-library cross-references (defined in `docs/links/linalg.txt`) |
| `\sa_stl` | STL cross-references (defined in `docs/links/stl.txt`) |
| `\sa_sparse` | sparse-CSC API cluster + scipy/Julia/10x interop notes |
| `\sa_mdspan` | C++23 `std::mdspan` + libstdc++/libc++ version floors |
| `\sa_async` | FAPL-scoped async executor model |
| `\sa_token` | HDF5 1.12+ token-based identity (`token_t`, `oinfo`, `token_equal`) |

### Function-group headers

| Alias | Expands to | Use on |
|---|---|---|
| `\func_read_hdr` | `@ingroup datasets` | every `h5::read` overload |
| `\func_write_hdr` | `@ingroup datasets` | every `h5::write` overload |
| `\func_create_hdr` | `@ingroup datasets` | every `h5::create` overload |
| `\func_append_hdr` | `@ingroup datasets` | `h5::append` / `h5::flush` / `h5::reset` (packet table) |
| `\func_attr_hdr` | `@ingroup attribute-io` | every `h5::aread` / `h5::awrite` |
| `\func_sparse_hdr` | `@ingroup datasets` | sparse read/write overloads |
| `\func_async_hdr` | `@ingroup async-io` | `h5::async::*` factories |
| `\func_traversal_hdr` | `@ingroup traversal` | `h5::ls` / `h5::dfs` / `h5::bfs` |
| `\func_read_desc` | one-line read description | combine with `func_read_hdr` |
| `\func_write_desc` | one-line write description | combine with `func_write_hdr` |
| `\func_create_links` | a `\sa_*` bundle for create | use after `func_create_hdr` |



## Adding a new alias

1. Confirm it'll appear **≥ 5 times** across the API. If not, write the `@param` inline.
2. Open `docs/links/h5cpp.txt` (or `hdf5.txt` / `linalg.txt` / `stl.txt` if cross-domain).
3. Add the alias under the matching section header (e.g. `############ FILE / DATASET PARAMETERS`).
4. Use the alias at one call site to validate the expansion.
5. Run `cd doxy && doxygen Doxyfile` locally; confirm the generated HTML renders the substituted text correctly.
6. Add an entry to this page (`docs/aliases.md`) so the catalog stays in sync.



## Removing an alias

If an alias has zero callers and offers nothing the existing vocabulary can't express, drop it. Same rule applies if an alias is renamed or replaced — the old name doesn't linger. The deletion goes through `docs/links/*.txt` (and removes the row from this page).

If callers exist, migrate them in the same commit as the removal.
