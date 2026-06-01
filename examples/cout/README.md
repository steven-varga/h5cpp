@page example_guide_cout cout — IO-Debug Pretty-Printers for HDF5 Property Lists, Dataspaces, Hyperslab Arguments, and Handles

Drop `std::cout << x` into any IO call site for instant visibility into property lists, dataspaces, hyperslab arguments, and the RAII handles themselves — the values you'd otherwise chase through `h5dump` or a debugger. h5cpp ships `operator<<` overloads for the argument types you already pass to `h5::read` / `h5::write` (offset, count, stride, block, current_dims, max_dims), for the dataspace, for the data-transfer property list, and for six high-information handles, with a generic fallback covering every other handle type. Container pretty-printing for `std::vector` / `map` / `tuple` / etc. lives in a separate header and has its own example under `examples/pprint/`.

```cpp
h5::sp_t space = h5::get_space(ds);
h5::select_hyperslab(space, h5::offset{2,3}, h5::count{4,4});
std::cout << "file:      "  << fd                       << "\n"
          << "dataset:   "  << ds                       << "\n"
          << "selection: "  << space                    << "\n"
          << "dxpl:      "  << h5::default_dxpl         << "\n";
```

## Coverage

**6 specializations are live** (`fd_t`, `ds_t`, `gr_t`, `at_t`, `dcpl_t`, `fapl_t`) **plus 3 legacy printers** (`sp_t`, `dxpl_t`, `impl::array<T>`); every other handle gets the generic `{ handle=N valid=yes refs=M }` fallback. Total: 9 ✔ ok + 22 ◇ partial. See `tasks/h5cpp-handle-inventory.md` for the authoritative per-handle breakdown; the tables below are the summary view.

### Object handles (6)

| Handle       | Status                  | What the printer shows                                                                |
| ------------ | ----------------------- | -------------------------------------------------------------------------------------- |
| `h5::fd_t`   | ✔ ok specialization     | path / mode / libver bounds / file size                                                |
| `h5::ds_t`   | ✔ ok specialization     | path / dtype class+elsize / rank / dims / layout / chunk dims / filter count / attribute count |
| `h5::at_t`   | ✔ ok specialization     | name / dtype class / dataspace rank                                                    |
| `h5::gr_t`   | ✔ ok specialization     | path / child count / attribute count                                                   |
| `h5::ob_t`   | ◇ generic fallback      | handle id / valid / refs                                                               |
| `h5::sp_t`   | ✔ ok (legacy)           | rank / dims / selection bounds / hyperslab blocks                                      |

### Property-list handles (14)

| Handle       | Status                                                                                                                                                                |
| ------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `h5::dxpl_t` | ✔ ok (legacy) — handle + MPI mode                                                                                                                                     |
| `h5::dcpl_t` | ✔ ok specialization — layout / chunk / alloc / fill / filters                                                                                                         |
| `h5::fapl_t` | ✔ ok specialization — driver id / libver / cache / alignment                                                                                                          |
| `h5::acpl_t` / `h5::dapl_t` / `h5::tapl_t` / `h5::tcpl_t` / `h5::fcpl_t` / `h5::fmpl_t` / `h5::gapl_t` / `h5::gcpl_t` / `h5::lapl_t` / `h5::lcpl_t` / `h5::ocrl_t` / `h5::ocpl_t` / `h5::scpl_t` | ◇ generic fallback (11 handles) |

### Datatype handle (1)

| Handle         | Status                                                                                                                            |
| -------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| `h5::dt_t<T>`  | ◇ generic fallback — type-class-dispatched specialization is the biggest remaining unlock (see the report's "Recommended next pass") |

### Argument-array handles (6) — not handles strictly, but printable

| Type                                                                                                          | Status                                              |
| ------------------------------------------------------------------------------------------------------------- | --------------------------------------------------- |
| `h5::offset_t` / `h5::count_t` / `h5::stride_t` / `h5::block_t` / `h5::current_dims_t` / `h5::max_dims_t`      | ✔ ok via `operator<<(h5::impl::array<T>)` (legacy)  |

### Async-mode variants (5)

| Handle                                                                                            | Status                                                                                  |
| ------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------- |
| `h5::async::fd_t` / `ds_t` / `at_t` / `gr_t` / `ob_t`                                              | ◇ generic fallback — same `class_tag` as classic counterparts; specializations are mechanical follow-up work |

## Build & Run

```sh
cd <build-dir>
cmake --build . --target examples-cout
./examples-cout
```

Expected output:

```
[1] argument-type printers
  h5::offset{2,3}        = {2,3}
  h5::count{10,20}       = {10,20}
  h5::stride{1,2}        = {1,2}
  h5::block{4,4}         = {4,4}
  h5::current_dims{50,60}= {50,60}
  h5::max_dims{H5S_UNLIMITED,40} = {inf,40}  (note 'inf' for unlimited)

[2] dataspace printer
  fresh dataspace (all selected):
[rank]	2	[total elements]	200
[dimensions]	current: {10,20}	maximum: {10,20}
[selection]	start: {0,0}	end:{9,19}
[selection]	within extent

  after select_hyperslab(offset{2,3}, count{4,4}):
[rank]	2	[total elements]	200
[dimensions]	current: {10,20}	maximum: {10,20}
[selection]	start: {2,3}	end:{5,6}
[selection]	within extent
[selected element count]	-1
[selected block count]	1
[selected blocks]	[{2,3}{5,6}] 
[3] dxpl printer
  default dxpl    : handle: 0

[4] handle printers
  fd_t   : h5::fd_t{ handle=72057594037927936 path='cout.h5' mode=RDWR libver=[0,3] size=4096 }
  ds_t   : h5::ds_t{ handle=360287970189639682 path='/grid' dtype=FLOAT elsize=8 rank=2 dims={10,20} layout=CONTIGUOUS filters=0 attrs=0 }
  gr_t   : h5::gr_t{ handle=144115188075855872 path='/sensors' children=2 attrs=1 }
  at_t   : h5::at_t{ handle=504403158265495553 name='units' dtype=STRING rank=0 }
  dcpl_t : h5::dcpl_t{ handle=792633534417207391 layout=CHUNKED chunk={4,8} alloc=INCR fill=default filters=[deflate(1)] }
  fapl_t : h5::fapl_t{ handle=792633534417207392 driver_id=576460752303423488 libver=[1,3] cache={mdc=0 rdcc_slots=521 rdcc_bytes=1048576 w0=0.75} align={threshold=1 align=1} }

[5] STL pretty-printer (H5Uall.hpp)
  vector<string>      : [alpha,beta,gamma]
  vector<vector<int>> : [[1,2],[3,4,5],[6]]

cout.h5 written; structure visible via `h5ls -r cout.h5` or `h5dump cout.h5`.
```

## ADL gotcha

The six handle specializations live in `h5::impl::detail`, **not** in `h5`; placing them in `h5` makes the generic template win because the aliases never introduce `h5` into the associated-namespace set. See `tasks/h5cpp-handle-inventory.md` for the full rationale.

## Files

| File       | What it covers                                                                                                                                                  |
| ---------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `cout.cpp` | Five stages: argument-type printers, dataspace printer (all-selected vs hyperslab), dxpl printer, the six handle printers (stage 4), and an STL-printer nod.    |

## Use cases

- **Debugging hyperslab selections.** The `[2]` block tells you exactly what HDF5 sees: rank, dims, selection bounds, validity, resolved `[{start}{end}]` block list. When `H5Dread` returns less data than expected, stream the dataspace.
- **Auditing chunked-write composition.** Stage `[4]` shows the `dcpl_t` filter pipeline by name (e.g. `filters=[deflate(1)]`) plus the chunk dims HDF5 actually used — the values to check when a gzip/szip/shuffle stack silently fails to take effect.
- **Verifying DXPL composition in MPI runs.** Under `H5_HAVE_PARALLEL` the dxpl printer decodes the actual IO mode after a collective transfer — confirms chunk-collective vs independent vs mixed vs contiguous-collective.
- **Inline tracing during partial-IO bug hunts.** Stream the `fd_t` / `ds_t` / offset / count / space arguments on one line right before the IO call; no debugger session, no `h5dump` round-trip.

## Notes

The dxpl printer's MPI-mode decoding is gated on `H5_HAVE_PARALLEL`; on non-MPI builds it just prints the handle id (e.g. `handle: 0`). The dataspace printer's `[selected element count]	-1` line means "no irregular point selection" — `H5Sget_select_elem_npoints` returns `-1` for hyperslab selections; the block list below is the relevant information. For container pretty-printing (`vector`, `map`, `tuple`, nested forms) the dedicated example is `examples/pprint/`.

## CMake Wiring

```cmake
## cout — operator<< pretty-printers from H5cout.hpp / H5Uall.hpp
add_h5cpp_example(cout cout/cout.cpp)
```

Lives in `examples/CMakeLists.txt:470-471`. No library dependencies beyond `<h5cpp/all>` and HDF5.

## Build State (as of HEAD)

| Target          | Status                                                                          |
| --------------- | ------------------------------------------------------------------------------- |
| `examples-cout` | ✔ ok — five printer families exercised (args, sp, dxpl, handles, STL), exit 0   |

## Roadmap

In priority order, the report's recommended next pass:

1. **`h5::dt_t<T>`** — type-class dispatch (INTEGER / FLOAT / STRING / COMPOUND / ARRAY / VLEN / REFERENCE / ENUM / OPAQUE) with compound-member recursion; biggest debugging payoff after the six already landed.
2. **`h5::ob_t`** — `H5Oget_info3`-based type discriminator (group / dataset / named-datatype) plus path; cheap to add.
3. **`h5::async::*`** specializations — mirror the six classic specializations, reading `h.handle` directly since `operator ::hid_t()` is deleted on async variants.
4. **`h5::dapl_t`** — chunk cache config, virtual view, `efile_prefix`, `high_throughput` flag; useful for tuning chunked reads.
5. **`h5::lcpl_t`** — char encoding (ASCII vs UTF-8) and `create_intermediate_group` flag; surfaces the UTF-8 default that is currently invisible.

## Cross-References

- **`tasks/h5cpp-handle-inventory.md`** — authoritative full inventory of every h5cpp handle and its current printer status (✔ ok specializations vs ◇ partial fallbacks). This README is the quick reference; the inventory report is the source of truth.
- **`h5cpp/H5cout.hpp`** — `operator<<` for `h5::dxpl_t`, `h5::impl::array<T>`, `h5::sp_t`, the six handle specializations (`fd_t` / `ds_t` / `gr_t` / `at_t` / `dcpl_t` / `fapl_t`), and the generic fallback template. Tag dictionary at `H5cout.hpp:138-172`, generic printer at `:174-194`, six specializations at `:197-454`.
- **`h5cpp/H5Uall.hpp`** — STL container pretty-printer family (`vector` / `list` / `set` / `map` / `array` / `deque` / `pair` / `tuple` / `stack` / `queue`).
- **`examples/pprint/`** — dedicated STL pretty-printer demo.
- **`examples/datasets/`** — the `offset` / `count` / `stride` / `block` arguments these printers help debug.

## Source

- [`cout.cpp`](cout_8cpp-example.html) — rendered with syntax highlighting
