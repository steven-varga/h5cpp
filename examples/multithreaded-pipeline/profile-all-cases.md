# Detailed profile — all `h5::append` cases

Consolidated, single-methodology snapshot of every case in `profile-report.md`, plus
derived metrics. This is the **baseline** the `exp-vyukov` experiment is measured against.

**Method.** 256 MB of `double` streamed via `h5::append`, chunk 8192 (= 4096 chunks of
64 KB), HDF5 1.12.3. Box: i7-11700K — 8 physical cores / 16 SMT threads. Harness **and**
bundled libdeflate compiled `-O3 -DNDEBUG`. Each cell is the **median of 9** runs with
the **(min..max)** range; `µs/chunk` = wall-time ÷ 4096. Filter = HDF5 DEFLATE (gzip)
level 4 via libdeflate, or none. `classic` = no `-DH5CPP_MULTITHREAD`; `MT` = global lock on.

## A. Throughput — gzip-4 (compression-bound)

| case | config | MB/s (median) | range | µs/chunk | speedup |
|---|---|--:|---|--:|--:|
| baseline | C-API deflate-4 (HDF5 gzip plugin, zlib) | 43.4 | 43.3..44.0 | 1440 | 1.00× |
| basic | h5cpp, no pool, classic | 118.7 | 115.5..119.0 | 527 | 2.74× |
| basic | h5cpp, no pool, MT | 119.1 | 117.7..119.3 | 525 | 2.74× |
| pool | `threads{1}` | 124.1 | 122.6..125.3 | 504 | 2.86× |
| pool | `threads{2}` | 246.9 | 238.6..248.7 | 253 | 5.69× |
| pool | `threads{4}` | 469.1 | 425.0..478.9 | 133 | 10.81× |
| pool | `threads{8}` | 883.1 | 793.3..895.1 | 70.8 | 20.35× |
| pool | `threads{16}` (SMT) | 904.9 | 778.2..921.4 | 69.1 | 20.85× |

Speedup is vs the **C-API baseline**. Vs h5cpp basic (119): `threads{8}` = **7.4×**
(≈93% of 8 cores). `threads{16}` adds ~2% — SMT on saturated cores, no real headroom.

## B. Throughput — no filter (memory/IO-bound)

| case | config | MB/s (median) | range | µs/chunk |
|---|---|--:|---|--:|
| basic | no pool, 1 producer | 1903.2 | 1837..1940 | 32.8 |
| pool | `threads{1}` | 1702.5 | 1604..1727 | 36.7 |
| pool | `threads{2}` | 1611.9 | 1323..1656 | 38.8 |
| pool | `threads{4}` | 1624.0 | 1548..1636 | 38.5 |
| pool | `threads{8}` | 1593.4 | 1533..1668 | 39.2 |
| basic | no pool, **4 producers** | 2106.2 | 1950..2138 | 29.7 |

No compute to parallelize, so the pool is a **net tax (−10..−16%)**; fastest is no pool,
and only the no-pool path scales with producers (1903 → 2106 at 4).

## C. Lock cost — classic vs MT (global HDF5 lock)

| workload | classic | MT | Δ |
|---|--:|--:|--:|
| gzip-4 `threads{4}` | 473.1 (459..480) | 470.6 (460..476) | −0.5% |
| no filter `threads{4}` | 1607.2 (1525..1658) | 1591.9 (1549..1648) | −1.0% |

Within run-to-run noise — the global lock is **free**.

## D. Concurrency (gzip-4, `threads{8}`, 256 MB total)

| producers | MB/s | range |
|--:|--:|---|
| 1 | 867.2 | 805..897 |
| 2 | 858.4 | 798..886 |
| 4 | 853.6 | 795..863 |
| 8 | 773.0 | 724..822 |

Flat-to-slightly-down (one producer already saturates the 8-core pool); the value is
**TSan-clean concurrent appenders**, not extra speed.

## E. Producers vs pool — the parallelism source (4 producers, 256 MB total)

| workload | `threads{4}` pool | no pool | 1-producer ref |
|---|--:|--:|--:|
| gzip-4 | 460.8 (446..468) | **116.0 (115..118)** | 469 / 119 |
| no filter | 1847.1 (1701..1863) | 2106.2 (1950..2138) | 1903 (no pool) |

`4 producers no pool` (116) = `1 producer no pool` (119): for gzip the **pool is the only
parallelism** — producer threads buy nothing, because basic compresses inline under the lock.

## F. Dispatch machinery — per-operation cost (2 M ops, single thread)

| operation | ns/op (median) | notes |
|---|--:|---|
| inline call | 4.3 | baseline |
| `std::mutex` + `std::queue` push+pop | 28 | **the live pool queue** |
| Vyukov MPSC push+pop | 12.7 | `H5Qall.hpp` — dead but compiled (build is `-std=c++20`) |
| full `worker_pool::submit` + run (1 worker) | ~500 (474..514) | packaged_task + future + handoff |

The thread-safe **queue is ~28 ns = ~6% of the ~500 ns/task** dispatch; alloc + future +
handoff dominate. Pool=1 vs basic: **gzip +4.2%** (compress/IO overlap beats overhead),
**no-filter −10.5%** (overhead, nothing to overlap).

## Headline numbers (for the exp-vyukov comparison)

| metric | value |
|---|--:|
| codec: libdeflate vs zlib (L4, isolated) | 132 vs 47 MB/s (2.8×) |
| single-thread append vs C-API baseline | 119 vs 43 MB/s (2.74×) |
| pool scaling, gzip-4, 8 cores | 7.4× (883 MB/s, ~93% eff) |
| global lock cost | −0.5..−1.0% (free) |
| **live queue op cost (mutex+std::queue)** | **28 ns** |
| **Vyukov MPSC op cost (isolated)** | **12.5 ns** |
| **full per-task dispatch cost** | **~500 ns** |

The exp-vyukov question: swapping the pool's `mutex+std::queue` (28 ns) for a bounded
Vyukov queue (~13 ns) saves ~15 ns of the ~500 ns/task — **predicted ≤3% of dispatch,
i.e. invisible end-to-end**. The experiment measures whether reality matches.
