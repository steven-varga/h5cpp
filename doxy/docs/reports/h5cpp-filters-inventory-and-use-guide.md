@page reports_filters_inventory H5CPP Filter Inventory & Use Guide

**Scope:** All compression, checksum, shuffle, and pre-processing filters that h5cpp implements natively or ships via third-party dependencies.  
**Target audience:** Developers choosing filters for dataset creation pipelines.  
**Last updated:** 2026-05-24 (branch `262-gorilla-xor-filter`)

---

## 1. Quick Reference Table

| Filter | HDF5 ID | Type | Source | h5cpp property | Best for |
|--------|---------|------|--------|----------------|----------|
| **Deflate / Gzip** | 1 | Lossless | zlib (opt. libdeflate) | `h5::deflate{level}` `h5::gzip{level}` | General purpose, text, HDF5 default |
| **Shuffle** | 2 | Pre-processing | Native | `h5::shuffle` | Multi-byte numeric arrays before entropy codec |
| **Fletcher32** | 3 | Checksum | Native | `h5::fletcher32` | Data integrity verification |
| **SZIP** | 4 | Lossless | Vendored libaec/szip | `h5::szip` (C API) | Scientific integer imagery (Rice/Golomb) |
| **N-Bit** | 5 | Pre-processing | HDF5 C (passthrough) | `h5::nbit` | Compact integer storage |
| **Scale-Offset** | 6 | Pre-processing | HDF5 C (passthrough) | `h5::scaleoffset` | Quantised float/int storage |
| **LZ4** | 32004 | Lossless | System liblz4 | `h5::lz4` | Extreme speed, modest ratio |
| **Zstd** | 32015 | Lossless | Vendored zstd | `h5::zstd` | High ratio, tunable speed |
| **Gorilla** | 32016 | Lossless | Native | `h5::gorilla{4\|8}` | Smooth floating-point time series |

*IDs 1-6 are assigned by The HDF Group. IDs 32004, 32015, 32016 are community-registered filter slots.*

---

## 2. Native Implementations (in `h5cpp/H5Zall.hpp`)

### 2.1 Deflate / Gzip (`H5Z_FILTER_DEFLATE`, id=1)
- **Algorithm:** LZ77 + Huffman (RFC 1951 via zlib, or raw deflate via libdeflate).
- **Implementation:** `filter::deflate` / `filter::gzip` (aliases).
- **Fast path:** When `H5CPP_HAS_LIBDEFLATE` is defined, h5cpp uses the vendored **libdeflate** (v1.25.0) instead of zlib.  libdeflate is typically 2-5× faster for both encode and decode at the cost of slightly larger output.
- **Fallback:** Standard `compress2` / `uncompress` from system zlib.
- **Parameters:** `params[0]` = compression level (0-9, default from `H5CPP_DEFAULT_COMPRESSION`).
- **Property syntax:**
  ```cpp
  h5::create<T>(fd, "ds", dims, h5::chunk{1024} | h5::gzip{6});
  ```
- **Use when:** You need compatibility with every HDF5 reader on earth.  This is the safest default.

### 2.2 Shuffle (`H5Z_FILTER_SHUFFLE`, id=2)
- **Algorithm:** Byte-level transposition (AoS → SoA).  For `N` elements of `B` bytes, byte `k` of every element becomes contiguous.
- **Implementation:** `filter::shuffle` — pure C++ nested loops, no external deps.
- **Parameters:** `params[0]` = element size in bytes (inferred by HDF5).
- **Property syntax:**
  ```cpp
  h5::chunk{1024} | h5::shuffle | h5::gzip{6}
  ```
- **Use when:** Pre-processing multi-byte integers or floats before Deflate/LZ4/Zstd.  Shuffle exposes byte-plane redundancy that entropy coders exploit aggressively.  Almost always a net win for numeric arrays.

### 2.3 Fletcher32 (`H5Z_FILTER_FLETCHER32`, id=3)
- **Algorithm:** Fletcher-32 checksum (16-bit modular arithmetic, two running sums).
- **Implementation:** `filter::fletcher32` — native loop over 16-bit words.
- **Encode:** Appends 4-byte big-endian checksum → `size + 4`.
- **Decode:** Verifies checksum, strips 4 bytes.  Returns `0` on mismatch (HDF5 treats this as filter failure).
- **Property syntax:**
  ```cpp
  h5::chunk{1024} | h5::fletcher32 | h5::gzip{6}
  ```
- **Use when:** End-to-end data integrity is required.  Lightweight compared to cryptographic hashes.

### 2.4 Gorilla XOR (`H5Z_FILTER_GORILLA`, id=32016)
- **Algorithm:** XOR consecutive IEEE-754 values, suppress leading/trailing zero bits.
- **Implementation:** `filter::gorilla` — native bit-packed encoder/decoder with 6-bit leading and 6-bit meaningful length descriptors.
- **Heuristic:** Uses the **Chimp** cost check (`prev_meaningful <= 12 + meaningful`) to avoid pathological expansion when a sign-bit flip creates a 64-bit-wide block.
- **Supported widths:** `float32` (4) and `float64` (8).  Other sizes fall back to memcpy.
- **Parameters:** `params[0]` = element size (must be explicit; auto-detect is intentionally disabled to avoid 4-vs-8 ambiguity).
- **Property syntax:**
  ```cpp
  h5::chunk{1024} | h5::gorilla{8};  // double
  h5::chunk{1024} | h5::gorilla{4};  // float
  ```
- **Use when:** Smooth, correlated floating-point time series (sensor data, finance ticks, simulation outputs).  Identical values compress to ~1.6 % of raw size.  Smooth sine waves achieve ~85-87 % of raw size — modest but deterministic, and decode is fast (~1 GB/s).
- **Avoid when:** Random or uncorrelated floats, integer data, or data with frequent zero-crossings and exponent swings.

---

## 3. Third-Party / Vendored Filters

### 3.1 SZIP (`H5Z_FILTER_SZIP`, id=4)
- **Algorithm:** Rice/Golomb lossless coding with optional nearest-neighbour pre-processing.
- **Source:** Vendored **libaec/szip** in `thirdparty/szip/`.
- **Implementation:** `filter::szip` — thin wrapper around `SZ_BufftoBuffCompress` / `SZ_BufftoBuffDecompress`.
- **Parameters:** `params[0-3]` = `options_mask`, `bits_per_pixel`, `pixels_per_block`, `pixels_per_scanline`.
- **Build flag:** `H5CPP_USE_SZIP` (ON by default).
- **Use when:** Scientific imagery (satellite, medical) where Rice coding outperforms LZ variants on low-entropy integer raster data.

### 3.2 LZ4 (`H5Z_FILTER_LZ4`, id=32004)
- **Algorithm:** LZ4 block compression (byte-level dictionary + literal encoding).
- **Source:** **System library** (`find_library(lz4)` + `find_path(lz4.h)`).  Not vendored.
- **Implementation:** `filter::lz4` — calls `LZ4_compress_default` / `LZ4_decompress_safe`.
- **Build flag:** `H5CPP_USE_LZ4` (ON by default).  If not found, compiles as passthrough.
- **Parameters:** None exposed; h5cpp uses default LZ4 acceleration.
- **Use when:** Speed is paramount and you can accept lower compression ratios than Zstd or Deflate.  Common in HPC write paths where I/O bandwidth, not CPU, is the bottleneck.

### 3.3 Zstd (`H5Z_FILTER_ZSTD`, id=32015)
- **Algorithm:** Zstandard (finite-state entropy + LZ77, Facebook).
- **Source:** Vendored **zstd** v1.5.7 in `thirdparty/zstd/`.
- **Implementation:** `filter::zstd` — calls `ZSTD_compress` / `ZSTD_decompress`.
- **Build flag:** `H5CPP_USE_ZSTD` (ON by default).
- **Parameters:** `params[0]` = compression level (1-22).
- **Use when:** You want better ratios than gzip at similar or faster decode speeds.  Excellent general-purpose replacement for gzip in new archives.

### 3.4 libdeflate (acceleration layer for id=1)
- **Not a distinct filter ID.**  libdeflate is a drop-in faster implementation of the Deflate/Gzip bitstream.  When `H5CPP_HAS_LIBDEFLATE` is defined, `filter::deflate` routes encode/decode through libdeflate instead of zlib.
- **Source:** Vendored **libdeflate** v1.25.0 in `thirdparty/libdeflate/`.
- **Build flag:** `H5CPP_USE_LIBDEFLATE` (ON by default).
- **Use when:** You are already using `h5::gzip` or `h5::deflate` but need higher single-threaded throughput.

---

## 4. HDF5-Native Passthrough Filters

These filters are **registered and executed by the HDF5 C library** itself during `H5Dread` / `H5Dwrite`.  h5cpp provides property-list wrappers but does not reimplement the algorithm.

| Filter | ID | Wrapper | Why passthrough |
|--------|----|---------|-----------------|
| **N-Bit** | 5 | `h5::nbit` | Bit-precision packing for integers; used rarely in trading/HPC paths. |
| **Scale-Offset** | 6 | `h5::scaleoffset` | Quantised float/int pre-processing; delegated to HDF5 C to avoid reverse-engineering internal `cd_values` descriptors. |

Both are valid in property chains and will be applied correctly by the underlying HDF5 library.

---

## 5. Build-Time Configuration Matrix

| Dependency | CMake option | Vendored? | Location | Compile def |
|------------|-------------|-----------|----------|-------------|
| zlib (gzip) | — (required) | No | System | — |
| libdeflate | `H5CPP_USE_LIBDEFLATE` | **Yes** | `thirdparty/libdeflate/` | `H5CPP_HAS_LIBDEFLATE` |
| LZ4 | `H5CPP_USE_LZ4` | No | System | `H5CPP_HAS_LZ4` |
| Zstd | `H5CPP_USE_ZSTD` | **Yes** | `thirdparty/zstd/` | `H5CPP_HAS_ZSTD` |
| SZIP | `H5CPP_USE_SZIP` | **Yes** | `thirdparty/szip/` | `H5CPP_HAS_SZIP` |
| Gorilla | — (always) | N/A | Native in `H5Zall.hpp` | — |
| Shuffle | — (always) | N/A | Native in `H5Zall.hpp` | — |
| Fletcher32 | — (always) | N/A | Native in `H5Zall.hpp` | — |

*Vendored libraries are built with `EXCLUDE_FROM_ALL` and linked statically into test executables; they do not install shared libraries unless explicitly requested.*

---

## 6. Recommended Filter Pipelines by Workload

### 6.1 General-Purpose Numeric Arrays
```cpp
h5::chunk{1024} | h5::shuffle | h5::gzip{6}
```
*Shuffle exposes byte-plane redundancy; gzip compresses it.  This is the HDF5 "sweet spot" for float/int matrices.*

### 6.2 High-Throughput HPC Checkpointing
```cpp
h5::chunk{4096} | h5::shuffle | h5::lz4
```
*LZ4 encode at ~500 MB/s+, decode at ~1-2 GB/s.  Accept modest ratio for speed.*

### 6.3 Archival / Long-Term Storage
```cpp
h5::chunk{1024} | h5::shuffle | h5::zstd{12}
```
*Zstd level 12 gives better ratios than gzip-6 at comparable decode speed.*

### 6.4 Smooth Time-Series (Sensor, Market Data)
```cpp
h5::chunk{1024} | h5::gorilla{8}
```
*For 1 M double sine wave: ~86.7 % of raw size, decode ~1 GB/s.  If chunk size is large, follow with LZ4 for entropy coding: `h5::gorilla{8} | h5::lz4`.*

### 6.5 Integrity-Critical Financial Data
```cpp
h5::chunk{1024} | h5::fletcher32 | h5::shuffle | h5::gzip{6}
```
*Fletcher32 should be **first** in the pipeline so it checksums the raw data, not the compressed bitstream.*

### 6.6 Integer Satellite Imagery
```cpp
h5::chunk{256,256} | h5::szip
```
*SZIP's Rice coder is optimised for spatially correlated integer raster data.*

---

## 7. Filter Dispatch Architecture

At runtime, `h5::impl::filter::get_callback(H5Z_filter_t id)` maps canonical HDF5 filter IDs to C++ function pointers:

```cpp
call_t get_callback(H5Z_filter_t filter_id) {
    switch(filter_id) {
        case H5Z_FILTER_DEFLATE:    return filter::gzip;
        case H5Z_FILTER_SHUFFLE:    return filter::shuffle;
        case H5Z_FILTER_FLETCHER32: return filter::fletcher32;
        case H5Z_FILTER_SZIP:       return filter::szip;
        case H5Z_FILTER_NBIT:       return filter::nbit;        // passthrough
        case H5Z_FILTER_SCALEOFFSET:return filter::scaleoffset; // passthrough
        case H5Z_FILTER_LZ4:        return filter::lz4;
        case H5Z_FILTER_ZSTD:       return filter::zstd;
        case H5Z_FILTER_GORILLA:    return filter::gorilla;
        default:                    return filter::error;
    }
}
```

Community filters (LZ4, Zstd, Gorilla) must be **registered** with HDF5 before use.  h5cpp handles this automatically:
- **Gorilla:** `gorilla_register_filter()` is called inside `h5::gorilla::copy_impl()` via `std::call_once`.
- **LZ4 / Zstd:** Registration is assumed to be handled by the system's HDF5 filter plugin infrastructure (`.h5pl` paths or pre-loaded shared libraries).

---

## 8. Summary Decision Tree

```
Data type?
├── Integer (8-64 bit)
│   ├── Spatial correlation (images)? → SZIP or Shuffle+Zstd
│   └── General? → Shuffle+Gzip or Shuffle+Zstd
├── Float / Double
│   ├── Smooth time-series? → Gorilla (optionally + LZ4)
│   ├── General numeric arrays? → Shuffle+Gzip or Shuffle+Zstd
│   └── Random / already compressed? → No filter, or Shuffle only
└── String / blob
    └── General-purpose? → Gzip or Zstd

Speed requirement?
├── Fastest possible write? → LZ4
├── Balanced? → Zstd{3-6}
├── Maximum ratio? → Zstd{12-19} or Gzip{9}

Integrity requirement?
└── Yes → prepend Fletcher32
```

---

*End of report.*

## Related examples

- [`examples/custom-pipeline/pipeline.cpp`](../../../examples/custom-pipeline/pipeline.cpp) — custom filter pipeline composition
- [`examples/datasets/datasets.cpp`](../../../examples/datasets/datasets.cpp) — dataset creation with chunking + compression options
