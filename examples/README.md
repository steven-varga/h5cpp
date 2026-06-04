@page examples_guides_index Cook book

Each h5cpp example ships a `README.md` that explains *why* the code looks the way it does — the design choices, the on-disk layout, the trade-offs, the gotchas. Each cookbook page links out to the rendered example source at the bottom, so you can move from "what this does" to the actual code in one click.

## Core call surface

- @subpage example_guide_basics — opening files, reading and writing the supported elementary types
- @subpage example_guide_datasets — explicit dataset creation, dims, chunking
- @subpage example_guide_attributes — attribute creation, read/write, fixed-length vs VLEN strings
- @subpage example_guide_groups — group create / open / traversal
- @subpage example_guide_datatypes — registered datatypes, custom types, n-bit / two-bit
- @subpage example_guide_compound — compound (struct) types, packing, padding
- @subpage example_guide_container — supported container shapes
- @subpage example_guide_stl — STL types (strings / containers / tuples / arrays)
- @subpage example_guide_string — std::string / std::string_view / char[N] / std::array<char,N>
- @subpage example_guide_utf — UTF-8 dataset names, paths, attribute values

## Linalg & numerics

- @subpage example_guide_linalg — armadillo / eigen / blaze / blitz / dlib / ublas / xtensor / itpp
- @subpage example_guide_sparse — Armadillo / Eigen sparse matrix CSC round-trip
- @subpage example_guide_mdspan — C++23 `std::mdspan` round-trip

## I/O patterns & performance

- @subpage example_guide_transform — read-time data transforms
- @subpage example_guide_optimized — performance-tuned write paths
- @subpage example_guide_raw_memory — raw pointer overloads
- @subpage example_guide_packet_table — packet table (append) interface
- @subpage example_guide_swmr — SWMR single-writer/multiple-reader streaming (Linux-only)
- @subpage example_guide_custom_pipeline — custom filter pipeline composition

## Tooling & debugging

- @subpage example_guide_cout — `h5::cout` handle pretty-printers
- @subpage example_guide_pprint — STL container pretty-printers
- @subpage example_guide_reference — `h5::reference_t` lifecycle
- @subpage example_guide_smart_ptr — smart-pointer interop
- @subpage example_guide_reflection — compile-time reflection (h5cpp-compiler emitted)
- @subpage example_guide_multi_tu — multi-TU compilation pattern

## Format & file features

- @subpage example_guide_half_float — IEEE 754 half precision
- @subpage example_guide_csv — CSV importer (h5cpp-compiler tier-2)
- @subpage example_guide_s3 — ROS3 read-only S3 access
- @subpage example_guide_mpi — parallel HDF5 + MPI patterns

## How the Cook book relates to the rest of the docs site

| If you want… | Look under |
|---|---|
| The narrative / design intent of an example | this section (Cook book) |
| The raw source of an example, syntax-highlighted | linked at the bottom of each cookbook page ("Source") |
| API reference for `h5::read` / `h5::write` etc. | the **IO API** or **Files** section |
| Architecture / inventory / taxonomy reports | **Project Reports** (separate section) |

## A note on extracting code snippets into prose

Doxygen supports `\snippet <file> <anchor>` — annotate a region of a `.cpp` with `//! [anchor_name]` markers and `\snippet` lifts that exact block into the rendered page. The Cook book pages currently link out to the full example source; selective snippet-lifting (showing only the relevant lines, similar to MkDocs `:::include`) is a per-leaf opt-in we can add as the examples mature.
