// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Raw-pointer read/write surface.
//
// Most h5cpp examples pass containers (arma::mat, std::vector, ...) so the
// library can deduce shape, element type, and storage from the access traits.
// When you only have a bare pointer and a length, you opt out of that
// machinery and supply the dataspace yourself via h5::count / h5::offset.
// That's what this example demonstrates.
//
// Two distinct stories are layered here:
//
//   1. Dataset creation with the full positional argument set:
//      h5::create<T>(fd, "...", current_dims, max_dims, lcpl, dcpl, dapl).
//      Showing the optional argument vocabulary as a single block.
//
//   2. h5::write<T>(fd, "...", ptr, h5::count{...}, [h5::offset{...}])
//      and the symmetric h5::read.  No container — the raw pointer plus the
//      count/offset describe what to move.
//
// All reads are verified with the ✔/✘ symbol convention.

#include <h5cpp/all>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

int main() {
    h5::fd_t fd = h5::create("raw.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // ── 1. Dataset creation with the full positional argument vocabulary ────
    //    Every property list (lcpl / dcpl / dapl) is positional and optional.
    //    The examples below produce equivalent datasets via different argument
    //    permutations.
    {
        // Inline dcpl built from pipe-chained flags; explicit lcpl + dapl.
        h5::create<short>(fd, "/type/inline_dcpl",
                h5::current_dims{10, 20}, h5::max_dims{10, H5S_UNLIMITED},
                h5::create_path | h5::utf8,                                      // lcpl
                h5::chunk{2, 3} | h5::fill_value<short>{42} | h5::fletcher32
                    | h5::shuffle | h5::nbit | h5::gzip{9},                       // dcpl
                h5::default_dapl);

        // Same content with the dcpl hoisted into a named variable.
        h5::dcpl_t dcpl = h5::chunk{2, 3} | h5::fill_value<short>{42}
                        | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9};
        h5::create<short>(fd, "/type/named_dcpl",
                h5::current_dims{10, 20}, h5::max_dims{10, H5S_UNLIMITED}, dcpl);

        // Same as above with every default spelled out.
        h5::create<short>(fd, "/type/all_defaults_explicit",
                h5::current_dims{10, 20}, h5::max_dims{10, H5S_UNLIMITED},
                h5::default_lcpl, dcpl, h5::default_dapl);

        // max_dims alone — current_dims defaults to max_dims with H5S_UNLIMITED
        // replaced by 0. Suitable as an empty container for packet-table-style
        // appends.
        h5::create<short>(fd, "/type/maxdims_only",
                h5::max_dims{10, H5S_UNLIMITED},
                h5::chunk{10, 1} | h5::gzip{9});

        // Variable-length strings work the same way.
        h5::create<std::string>(fd, "/type/vlen_strings",
                h5::max_dims{H5S_UNLIMITED}, h5::chunk{10} | h5::gzip{9});
    }

    // ── 2. Raw-pointer write into a 2-D filespace ────────────────────────────
    //    The source is a 1-D `double[10]` on the stack; the destination
    //    dataset is created as a 1×10 2-D dataspace (h5::count{1, 10}).
    double src[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    h5::write<double>(fd, "/dataset", src, h5::count{1, 10});

    // ── 3. Raw-pointer read with an offset into the dataset ─────────────────
    //    Reads dataset[0, 2..9] (8 elements) and lands them at buf[0..7].
    //    buf is heap-allocated to make the "raw memory" story explicit; in
    //    practice you'd use stack storage or std::vector::data().
    std::unique_ptr<double[]> buf(new double[10]());        // zero-init
    h5::read<double>(fd, "/dataset", buf.get(),
            h5::count{1, 8}, h5::offset{0, 2});

    {
        std::vector<double> want = {2, 3, 4, 5, 6, 7, 8, 9, 0, 0};
        bool ok = true;
        for (int i = 0; ok && i < 10; ++i) ok = (buf[i] == want[i]);
        check("read offset{0,2} count{1,8} -> buf[0..7]    (tail zero-init)", ok);
    }

    // ── 4. Read into a non-zero destination offset within the buffer ────────
    //    Shifting the destination is done by passing buf.get() + 4, not by
    //    asking HDF5 to do it. h5cpp doesn't have a "memory offset" knob —
    //    the pointer IS the offset.
    h5::read<double>(fd, "/dataset", buf.get() + 4,
            h5::count{1, 3});                              // implicit offset = {0,0}
    {
        // buf was {2,3,4,5,6,7,8,9,0,0}.  Reading 3 elements from dataset[0,0..2]
        // = {0,1,2} into buf+4 overwrites buf[4..6] with {0,1,2}.
        std::vector<double> want = {2, 3, 4, 5, 0, 1, 2, 9, 0, 0};
        bool ok = true;
        for (int i = 0; ok && i < 10; ++i) ok = (buf[i] == want[i]);
        check("read into buf+4: buf[4..6] = first 3 of file", ok);
    }

    // ── 5. Stack-array write with implicit 1-D shape ────────────────────────
    //    Same surface, simpler example: 1-D destination, no offset gymnastics.
    {
        std::int32_t out[16];
        for (int i = 0; i < 16; ++i) out[i] = i * i;
        h5::write<std::int32_t>(fd, "/dataset_1d", out, h5::count{16});

        std::int32_t back[16] = {};
        h5::read<std::int32_t>(fd, "/dataset_1d", back, h5::count{16});
        bool ok = true;
        for (int i = 0; ok && i < 16; ++i) ok = (back[i] == i * i);
        check("write/read int32[16] (squares)", ok);
    }

    return 0;
}
