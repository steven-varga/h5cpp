/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <iostream>
#include <vector>

// h5cpp's std::mdspan trait is in H5Mmdspan.hpp, gated on __cpp_lib_mdspan.
// The probe must come BEFORE <h5cpp/all> so the trait sees the feature macro.
#if __has_include(<mdspan>)
#  include <mdspan>
#endif

#include <h5cpp/all>

// =========================================================================
// std::mdspan round-trip demo (C++23, P0009).
//
// mdspan is a non-owning N-D view: { pointer, extents, layout, accessor }.
// h5cpp treats it as a contiguous source/sink:
//
//   write side: h5::write(fd, "/path", view) — fully structural
//   read side : mdspan can't allocate its own buffer; use the buffer-out
//               idiom: allocate a vector, wrap with a fresh mdspan, then
//               h5::read into view.data_handle() with explicit h5::count.
//
// The dispatch lives in H5Mmdspan.hpp:
//   - access_traits_t<mdspan> kind = contiguous
//   - storage_representation_impl<mdspan> = linear_value_dataset
//
// Build requires libstdc++ ≥ 15 or libc++ ≥ 17 (the version that ships
// <mdspan>). On older stdlibs h5cpp's trait is inert; this example prints
// a skip notice and exits 0 — matches the same pattern h5cpp itself uses.
// =========================================================================

#if defined(__cpp_lib_mdspan) && __cpp_lib_mdspan >= 202207L

namespace {
    int errors = 0;

    template <class A, class B>
    void check(const char* tag, const A& expected, const B& got) {
        const bool ok = (expected == got);
        if (!ok) ++errors;
        std::cout << (ok ? "✔ ok      " : "✘ failed  ") << tag << "\n";
    }
}

int main() {
    h5::fd_t fd = h5::create("mdspan.h5", H5F_ACC_TRUNC);

    // -----------------------------------------------------------------
    // 1. Rank-2 mdspan write — view over a flat vector, 4 x 6.
    // -----------------------------------------------------------------
    {
        std::vector<double> buf(24);
        for (std::size_t i = 0; i < buf.size(); ++i) buf[i] = static_cast<double>(i);

        std::mdspan<double, std::dextents<std::size_t, 2>> view(buf.data(), 4, 6);
        h5::write(fd, "/matrix/A", view);

        // Read back as a flat std::vector; verify the contiguous bytes match.
        auto back = h5::read<std::vector<double>>(fd, "/matrix/A");
        check("rank-2 mdspan write — element count",        std::size_t(24), back.size());
        check("rank-2 mdspan write — element (0,0) = 0",    0.0,             back[0]);
        check("rank-2 mdspan write — element (3,5) = 23",   23.0,            back[23]);
    }

    // -----------------------------------------------------------------
    // 2. Buffer-out read into a caller-allocated mdspan.
    //    mdspan is non-owning, so h5::read<mdspan<...>> can't allocate.
    //    Pattern: own the storage via std::vector, then construct an
    //    mdspan view over it and h5::read into view.data_handle() with
    //    explicit h5::count to drive the dispatch.
    // -----------------------------------------------------------------
    {
        std::vector<double> sink(24);
        std::mdspan<double, std::dextents<std::size_t, 2>> view(sink.data(), 4, 6);
        h5::read<double>(fd, "/matrix/A", view.data_handle(), h5::count{24});
        check("buffer-out mdspan read — element (0,0)",  0.0,  sink[0]);
        check("buffer-out mdspan read — element (3,5)",  23.0, sink[23]);
        // Indexed access via the view (sanity check that the layout matches).
        check("buffer-out mdspan read — view(2,3) = 15", 15.0, view[2, 3]);
    }

    // -----------------------------------------------------------------
    // 3. Rank-3 mdspan write — a 2 x 3 x 4 cube.
    // -----------------------------------------------------------------
    {
        std::vector<float> cube(24);
        for (std::size_t i = 0; i < cube.size(); ++i) cube[i] = static_cast<float>(i);
        std::mdspan<float, std::dextents<std::size_t, 3>> view(cube.data(), 2, 3, 4);
        h5::write(fd, "/cube/B", view);
        auto back = h5::read<std::vector<float>>(fd, "/cube/B");
        check("rank-3 mdspan write — element count",     std::size_t(24), back.size());
        check("rank-3 mdspan write — first/last match",  23.0f,           back[23]);
    }

    std::cout << "\n"
              << (errors == 0 ? "✔ all checks passed"
                              : "✘ some checks failed")
              << ", errors=" << errors << "\n";
    return errors;
}

#else   // mdspan not available on this stdlib

int main() {
    std::cout << "◇ skipped — std::mdspan not available on this stdlib.\n"
              << "  Requires libstdc++ >= 15 or libc++ >= 17 (P0009 / __cpp_lib_mdspan >= 202207L).\n"
              << "  h5cpp's H5Mmdspan.hpp guards on the same feature macro and is inert here.\n";
    return 0;
}

#endif  // __cpp_lib_mdspan
