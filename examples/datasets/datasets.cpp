// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// =============================================================================
// h5cpp datasets tutorial
// =============================================================================
//
// A dataset is the unit HDF5 stores typed multi-dimensional arrays in. In h5cpp
// the surface is small:
//
//   - h5::create<T>(fd, path, ...)       create with explicit shape/policy
//   - h5::write(fd, path, data, ...)     one-shot create-or-write
//   - h5::read<T>(fd, path, ...)         typed read into T
//   - h5::append(pt, data)               packet-table style row appender
//
// This example walks the dataset surface in ten numbered sections:
//
//   1. One-shot create + write
//   2. Explicit create then write
//   3. Reading back — three reader shapes
//   4. Chunking + filter chain
//   5. Fill values
//   6. Unlimited dimensions + append (packet table)
//   7. Hyperslab selection — offset / count / stride / block
//   8. Partial read — windowed read from a larger dataset
//   9. Reusable property lists
//  10. std::mdspan (C++23) — non-owning view as a write/read shape

#include <h5cpp/all>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

namespace {
    void section(const char* title) {
        std::cout << "\n" << title << "\n"
                << std::string(std::strlen(title), '-') << "\n";
    }
    // h5dump-style printer for a flat row-major buffer of shape (rows, cols).
    // Used instead of arma::mat printing to avoid the column-major vs row-major
    // transposition that would otherwise muddy the hyperslab demo.
    void show(const char* label, const std::vector<double>& buf, std::size_t rows, std::size_t cols) {
        std::cout << "  " << label << " (" << rows << "x" << cols << ")\n";
        for (std::size_t r = 0; r < rows; ++r) {
            std::cout << "    ";
            for (std::size_t c = 0; c < cols; ++c)
                std::cout << std::setw(7) << std::fixed << std::setprecision(2)  << buf[r * cols + c] << " ";
            std::cout << "\n";
        }
    }
} // namespace

int main() {
    auto fd = h5::create("datasets.h5", H5F_ACC_TRUNC);

    // ── 1. one-shot create + write ──────────────────────────────────────────
    // The dispatcher picks the dataset shape from the argument, allocates the
    // dataset, and writes in one call. Defaults: contiguous storage, no
    // compression, fixed dimensions taken from the value.
    section("1. one-shot create + write");
    {
        // Generator pipe from H5Uall.hpp: any distribution | h5::take(n)
        // returns a std::vector<value_type> ready for h5::write.
        std::vector<double> v = h5::uniform<double>{0.0, 100.0} | h5::take(5);
        h5::write(fd, "/one_shot/vec", v);
        std::cout << "  wrote " << v.size() << " uniform doubles to /one_shot/vec\n";
    }

    // ── 2. explicit create, then write ──────────────────────────────────────
    // Sometimes you want the dataset's shape and policy fixed up front (e.g.,
    // to attach attributes before the first write, or to enable chunking and
    // compression). h5::create<T> returns a managed ds_t; h5::write(ds, ...)
    // writes into it.
    section("2. explicit create then write");
    {
        h5::ds_t ds = h5::create<double>(fd, "/explicit/mat",
            h5::current_dims{4, 5}, h5::chunk{2, 5} | h5::gzip{6});

        // Attributes are written on the ds_t — pt_t is the packet-table view.
        ds["units"]    = "meters";
        ds["captured"] = "2026-05-27";

        // Row-major 4x5 of values 0,1,...,19.
        std::vector<double> M(4 * 5);
        std::iota(M.begin(), M.end(), 0.0);
        h5::write(ds, M);
        std::cout << "  wrote 4x5 to /explicit/mat with chunk{2,5} | gzip{6}\n";
    }

    // ── 3. reading back — three reader shapes ───────────────────────────────
    // Same dataset, three reader shapes. h5::read<T> dispatches on T:
    //   - std::vector<T>      contiguous, h5cpp allocates
    //   - T*  + h5::count{}   raw memory, caller owns the buffer
    //   - partial read        offset + count return just a region
    section("3. reading back — three reader shapes");
    {
        auto v = h5::read<std::vector<double>>(fd, "/explicit/mat");

        std::vector<double> buf(20);
        h5::read<double>(fd, "/explicit/mat", buf.data(), h5::count{4, 5});

        std::vector<double> col0(4);
        h5::read<double>(fd, "/explicit/mat", col0.data(),
            h5::offset{0, 0}, h5::count{4, 1});

        std::cout << "  vector<double>:  size = " << v.size() << "  first = " << v[0] << "\n";
        std::cout << "  raw double[20]:  first = " << buf[0]  << "  last = " << buf[19] << "\n";
        std::cout << "  column 0 (4x1):  " << col0[0] << " " << col0[1] << " " << col0[2] << " " << col0[3] << "\n";
    }

    // ── 4. chunking + filter chain ──────────────────────────────────────────
    // Chunking is *required* when you want compression, unlimited dimensions,
    // or extendability. The filter chain runs per chunk.
    //
    //   h5::chunk{r,c}           — rectangular chunks
    //   h5::gzip{N}              — DEFLATE level N (1..9)
    //   h5::shuffle              — byte-shuffle before compression
    //   h5::fletcher32           — chunk checksum
    //   h5::nbit                 — strip insignificant bits
    //   h5::fill_value<T>{v}     — pre-fill value for uninitialised regions
    section("4. chunking + filter chain");
    {
        // Slowly-varying sine — compresses well so the ratio readout is
        // meaningful. h5::normal / h5::uniform give high-entropy noise,
        // which compresses ~1:1 (good for pipeline-overhead tests, not here).
        std::vector<double> v(10000);
        for (size_t i = 0; i < v.size(); ++i) v[i] = std::sin(i * 0.01);

        h5::ds_t ds = h5::create<double>(fd, "/chunked/sine",
            h5::current_dims{100, 100},
            h5::chunk{20, 20} | h5::shuffle | h5::gzip{6} | h5::fletcher32);
        h5::write(ds, v);

        // Storage size vs. raw size — gzip+shuffle on a slowly-varying signal
        // is typically a 3-10x reduction.
        hsize_t storage = H5Dget_storage_size(static_cast<hid_t>(ds));
        std::cout << "  raw size:     " << v.size() * sizeof(double) << " bytes\n";
        std::cout << "  on-disk size: " << storage << " bytes\n";
        std::cout << "  ratio:        "
                  << std::fixed << std::setprecision(2)
                  << double(v.size() * sizeof(double)) / double(storage) << "x\n";
    }

    // ── 5. fill values ──────────────────────────────────────────────────────
    // Pre-create a dataset with a fill value, then read it back to see the
    // uninitialised cells. Common idioms: NaN for floats, sentinel for ints.
    section("5. fill values");
    {
        h5::create<double>(fd, "/fill/preset",
            h5::current_dims{3, 4}, h5::chunk{3, 4} | h5::fill_value<double>{std::nan("")});

        auto buf = h5::read<std::vector<double>>(fd, "/fill/preset");
        show("/fill/preset (read back before write)", buf, 3, 4);
    }

    // ── 6. unlimited dimensions + append (packet table) ─────────────────────
    // Set max_dims to H5S_UNLIMITED on the axis you want to grow. Chunking is
    // mandatory. h5::pt_t is the packet-table view — it buffers appends and
    // flushes them as chunks.
    section("6. unlimited dimensions + append (packet table)");
    {
        // Stream a generated sequence into the packet table. Each *dist
        // dereference returns a single sample, so this is the row-at-a-time
        // append pattern h5::pt_t was designed for.
        h5::uniform<int> dist{0, 1000};

        {   // Inner scope so the pt destructor flushes its buffer before we read.
            // Use a chunk size that divides the append count (20 * 5 = 100)
            // — partial trailing chunks are zero-padded in the current bank.
            h5::pt_t pt = h5::create<int>(fd, "/stream/values",
                h5::max_dims{H5S_UNLIMITED}, h5::chunk{20} | h5::gzip{4});
            for (int i = 0; i < 100; ++i) h5::append(pt, *dist);
        }
        auto out = h5::read<std::vector<int>>(fd, "/stream/values");
        std::cout << "  appended " << out.size() << " random ints" << "  (last = " << out.back() << ")\n";
    }

    // ── 7. hyperslab selection — offset / count / stride / block ────────────
    // Write a small block into a larger dataset using a hyperslab selection.
    //
    //   offset  — coordinate of the first selected cell
    //   count   — number of (block, block) groups
    //   stride  — distance between successive group starts
    //   block   — shape of each group
    //
    // With no stride/block, count is the simple "size of the selection".
    section("7. hyperslab selection — offset / count / stride / block");
    {
        // Background filled with 0.0.
        h5::ds_t ds = h5::create<double>(fd, "/hyperslab/grid",
            h5::current_dims{6, 8}, h5::chunk{3, 4} | h5::fill_value<double>{0.0});

        // Write a 2x3 block of 9.0 into the interior, starting at row=1,col=1.
        // Use a flat row-major buffer + explicit count so the on-disk shape
        // matches the source layout unambiguously.
        std::vector<double> patch(2 * 3, 9.0);
        h5::write(ds, patch.data(), h5::offset{1, 1}, h5::count{2, 3});

        auto buf = h5::read<std::vector<double>>(fd, "/hyperslab/grid");
        show("/hyperslab/grid", buf, 6, 8);
    }

    // ── 8. partial read — windowed read from a larger dataset ──────────────
    // Read just a window from the dataset created in section 7. Same
    // hyperslab surface; the caller supplies offset+count and h5cpp allocates
    // a value of the requested shape.
    section("8. partial read");
    {
        // Read a 3x4 window starting at (0,0): captures the patch and a
        // border of zeros so the position is visible.
        std::vector<double> sub(3 * 4);
        h5::read<double>(fd, "/hyperslab/grid", sub.data(),
            h5::offset{0, 0}, h5::count{3, 4});
        show("/hyperslab/grid [0:3, 0:4]", sub, 3, 4);
    }

    // ── 9. reusable property lists ──────────────────────────────────────────
    // Build a dcpl once, reuse it across many datasets. The | composition
    // produces a fully-formed property list at the call site.
    section("9. reusable property lists");
    {
        h5::dcpl_t fast_chunked = h5::chunk{64, 64} | h5::shuffle | h5::gzip{6};
        h5::lcpl_t deep_path    = h5::create_path | h5::utf8;

        for (int i = 0; i < 3; ++i) {
            std::string path = "/group/depth/" + std::to_string(i) + "/data";
            h5::create<float>(fd, path,
                h5::current_dims{128, 128}, deep_path, fast_chunked);
        }
        std::cout << "  created 3 datasets sharing the same dcpl + lcpl\n";
    }

    // ── 10. std::mdspan (C++23) — non-owning view as a write/read shape ─────
    // std::mdspan<T, Extents, ...> is a non-owning view over a contiguous
    // buffer. Shape lives in the type. h5cpp's mdspan mapper (H5Mmdspan.hpp,
    // gated on __cpp_lib_mdspan) wires it as kind=contiguous and routes it
    // through the same path as std::vector. Round-trip uses a user-owned
    // buffer for both ends — mdspan is non-owning by design.
#if defined(H5CPP_HAS_MDSPAN)
    section("10. std::mdspan (C++23) — non-owning view");
    {
        constexpr std::size_t rows = 3, cols = 4;

        // Source view over an owned buffer.
        std::vector<double> storage(rows * cols);
        std::iota(storage.begin(), storage.end(), 100.0);  // 100, 101, ...
        std::mdspan<double, std::dextents<std::size_t, 2>>
            view(storage.data(), rows, cols);

        // Write the view directly — shape comes from extents, data from .data_handle().
        h5::write(fd, "/mdspan/view", view);

        // Read back into a fresh buffer + view. mdspan is non-owning, so the
        // caller pre-allocates and uses the raw-pointer overload.
        std::vector<double> back_buf(rows * cols);
        std::mdspan<double, std::dextents<std::size_t, 2>>
            back(back_buf.data(), rows, cols);
        h5::read<double>(fd, "/mdspan/view", back.data_handle(),
            h5::count{rows, cols});

        show("/mdspan/view (read back into 3x4 mdspan)", back_buf, rows, cols);
    }
#else
    section("10. std::mdspan (C++23)");
    std::cout << "  skipped: this TU was not built with __cpp_lib_mdspan\n";
    std::cout << "  (libstdc++ 14+, libc++ 17+, compiled at C++23)\n";
#endif

    std::cout << "\nWrote everything to ./datasets.h5\n";
    std::cout << "Inspect with:  h5dump -pH datasets.h5\n";
    return 0;
}
