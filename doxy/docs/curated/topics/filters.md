@page curated_topics_filters FILTERS

@brief HDF5 filter pipeline — gzip, fletcher32, shuffle, nbit,
Gorilla, custom filters — composed via the `|` operator on a
`h5::dcpl_t`.

# Filter pipeline

HDF5 datasets stored in **chunked** layout can transform each chunk
through a pipeline of filters as it's written to (and read from) disk.
Compression, checksums, byte-reordering for better compression, and
domain-specific codecs all hook in through the same mechanism.

h5cpp exposes every filter as a tiny value type that composes onto a
`h5::dcpl_t` via the `|` operator:

```cpp
h5::ds_t ds = h5::create<float>(fd, "/grid/data",
    h5::current_dims{1024, 1024},
    h5::chunk{64, 64}                  // chunk shape — required for any filter
  | h5::shuffle                        // byte-shuffle (improves gzip ratio)
  | h5::gzip{9}                        // deflate level 9 (highest)
  | h5::fletcher32);                   // CRC for read-time integrity check
```

The order in the `|` chain is the order filters apply on write
(`shuffle` → `gzip` → `fletcher32` here, and the reverse on read).
**`h5::chunk{...}` is mandatory** — HDF5 only filters chunked
datasets, not contiguous or compact ones.

## Standard filters

These ship with the HDF5 distribution and are always available:

| Filter            | h5cpp tunable                  | Effect                                                                                  |
| :---------------- | :----------------------------- | :-------------------------------------------------------------------------------------- |
| **deflate** (gzip) | `h5::gzip{N}` where N ∈ [0..9] | Lossless DEFLATE compression. 9 = best ratio + slowest; 1 = fastest + worst ratio. Most-used. |
| **shuffle**       | `h5::shuffle`                   | Re-orders bytes within each chunk to put same-position bytes adjacent. Improves gzip / lz4 ratio dramatically on float data with low-entropy exponents. |
| **fletcher32**    | `h5::fletcher32`                | 32-bit checksum appended per chunk. Read-time integrity check; failures throw `h5::error::io::dataset::read`. Cost is ~negligible. |
| **nbit**          | `h5::nbit`                      | Pack values into the minimum number of bits per element. Lossless. Effective on small integer ranges (e.g. `uint16_t` values that actually fit in 12 bits). |
| **scaleoffset**   | `h5::scaleoffset{factor, offset}` | Multiply + shift before storage. Lossy for floats; lossless for integers within range. |
| **szip**          | `h5::szip{opts, blocks}`        | NASA-licensed compression for scientific data. Older; gzip is usually preferred. Built into HDF5 1.10+. |

The composition is order-sensitive in two ways:

1. **Filter chain order** — write-side runs left-to-right, read-side
   runs right-to-left. Put the entropy-reducing filters first
   (`shuffle` before `gzip`), the integrity check last
   (`fletcher32` at the end).
2. **Chunk shape vs filter** — `h5::chunk` must come *before* any
   filter in the chain; filters reject the dataset otherwise.

## High-throughput pipeline

The stock HDF5 filter chain runs single-threaded inside the chunk
cache. For large compressed datasets, decompression becomes the
bottleneck. h5cpp ships a **pool-parallel pipeline** that runs filters
across a configurable worker pool — activated by tagging the
**dataset access property list** (`dapl_t`) at open time:

```cpp
// Read a heavily-compressed dataset with parallel filter execution
auto dapl = h5::dapl{} | h5::high_throughput{h5::threads{8}};
h5::ds_t ds = h5::open(fd, "/giant/compressed", dapl);
auto v = h5::read<std::vector<float>>(ds);   // pool-parallel decompression
```

The pool's per-chunk cache is pre-warmed inside `h5::open` from the
dataset's element size — see @ref curated_io_api_file_open for the
hook.

The high-throughput pipeline is a pure read-side acceleration; write
paths still go through HDF5's native filter chain.

## Gorilla — time-series compression

Gorilla is Facebook's delta-of-delta time-series codec (originally for
their TSDB), shipped as a custom h5cpp filter. Particularly effective
on:
- Regularly-sampled time-series with smooth-ish values (sensor data,
  metrics, finance ticks)
- XOR-friendly floating-point sequences where consecutive samples
  share most of their high bits

```cpp
h5::ds_t ds = h5::create<double>(fd, "/sensor/temp",
    h5::current_dims{0}, h5::max_dims{H5S_UNLIMITED},
    h5::chunk{1024} | h5::gorilla);
```

Typically achieves 10–20x compression on smooth float streams where
gzip gets 2–3x. The compute cost per sample is small (~tens of ns).

See `examples/custom-pipeline/` for the full setup including the
`H5Z_class_t` registration the filter does at static-init time.

## Custom filters

The HDF5 filter ABI is open — any code can register a new filter ID
with `H5Zregister(const H5Z_class_t*)` and the dataset pipeline picks
it up. The h5cpp recipe:

1. **Pick an unused filter ID** in the range 32768–65535 (HDF Group
   reserves 0–32767 for standard filters).
2. **Write the encode/decode functions** (`size_t (*)(unsigned int
   flags, size_t cd_nelmts, const unsigned int cd_values[], size_t
   nbytes, size_t *buf_size, void **buf)` — the canonical filter
   signature).
3. **Register the `H5Z_class_t`** at static-init time (e.g. inside
   a `namespace { struct registrar { registrar(){ H5Zregister(…); } }
   _r; }` block in your filter's translation unit).
4. **Wrap with a tiny h5cpp `dcpl_t` tag** mirroring the `h5::gzip{N}`
   pattern so call sites compose cleanly.

The `examples/custom-pipeline/` cookbook entry walks through all four
steps with a runnable example.

## When to filter — and when NOT to

| Situation                                              | Filter? |
| :----------------------------------------------------- | :------ |
| Large dataset, mostly cold storage                     | ✔ gzip + shuffle |
| Time-series of slowly-varying floats                   | ✔ Gorilla |
| Small integer range stored as `int32`                  | ✔ nbit  |
| Concerned about silent bit-rot on read                 | ✔ fletcher32 |
| Write-hot small dataset (e.g. attribute-shaped)         | ✘ overhead dominates |
| Already-compressed input (JPEG, MP4 embedded as bytes) | ✘ negative compression ratio |
| Hyperslab reads that touch many chunks                 | ✘ each chunk decompresses fully on read |
| Dataset will be `h5::append`'d hot                     | △ chunk shape matters more than filter |

## Where to go next

- @ref curated_topics_properties — `dcpl_t` is the composition target
- @ref example_guide_custom_pipeline — runnable custom-filter walkthrough
- @ref example_guide_optimized — performance-tuned write paths
- @ref link_property_lists — full DCPL tunable reference
