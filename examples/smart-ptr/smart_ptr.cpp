// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Smart-pointer integration demo.
//
// Six round-trips proving the smart-pointer paths added in H5Mmemory.hpp /
// H5Mmemory_io.hpp, including composition with the compiler-assisted
// reflection path for POD compound types:
//
//   1. write/read std::unique_ptr<T[]>      (explicit count)
//   2. write/read std::shared_ptr<T[]>      (explicit count)
//   3. write/read std::unique_ptr<T>        (single element)
//   4. write/read std::shared_ptr<T>        (single element)
//   5. auto-allocating return-style read    auto buf = h5::read<std::unique_ptr<T[]>>(fd, path)
//   6. unique_ptr<Record[]> where Record is a compiler-registered POD compound
//      — proves smart pointers compose with H5CPP_REGISTER_STRUCT.
//
// Every block writes a freshly populated buffer, reads it back, and verifies
// element-by-element with the ✔/✘ symbol convention.

#include <h5cpp/all>

#include <cstdint>
#include <iostream>
#include <memory>

// ── Tiny POD record + manual registration ───────────────────────────────────
// Inlined here so the example is self-contained; in real code this body is
// what the h5cpp compiler emits into generated.h after scanning your headers.
namespace sn {
    struct point_t { double x, y, z; };
}
namespace h5 {
    template<> hid_t inline register_struct<sn::point_t>() {
        hid_t ct = H5Tcreate(H5T_COMPOUND, sizeof(sn::point_t));
        H5Tinsert(ct, "x", HOFFSET(sn::point_t, x), H5T_NATIVE_DOUBLE);
        H5Tinsert(ct, "y", HOFFSET(sn::point_t, y), H5T_NATIVE_DOUBLE);
        H5Tinsert(ct, "z", HOFFSET(sn::point_t, z), H5T_NATIVE_DOUBLE);
        return ct;
    }
}
H5CPP_REGISTER_STRUCT(sn::point_t);

int main() {
    h5::fd_t fd = h5::create("smart_ptr.h5", H5F_ACC_TRUNC);

    auto check = [](const char* label, bool ok) {
        std::cout << (ok ? "✔ ok    " : "✘ failed") << "  " << label << "\n";
    };

    // ── 1. std::unique_ptr<T[]> round-trip ─────────────────────────────────
    {
        constexpr std::size_t N = 16;
        std::unique_ptr<double[]> src(new double[N]());
        for (std::size_t i = 0; i < N; ++i) src[i] = i + 1.0;

        h5::write(fd, "/uniq_arr", src, h5::count{N});

        std::unique_ptr<double[]> dst(new double[N]());
        h5::read(fd, "/uniq_arr", dst, h5::count{N});

        bool ok = true;
        for (std::size_t i = 0; ok && i < N; ++i) ok = (dst[i] == src[i]);
        check("std::unique_ptr<double[]>(16)   write + read", ok);
    }

    // ── 2. std::shared_ptr<T[]> round-trip ─────────────────────────────────
    {
        constexpr std::size_t N = 12;
        std::shared_ptr<int[]> src(new int[N], std::default_delete<int[]>());
        for (std::size_t i = 0; i < N; ++i) src[i] = static_cast<int>(i * i);

        h5::write(fd, "/shared_arr", src, h5::count{N});

        std::shared_ptr<int[]> dst(new int[N], std::default_delete<int[]>());
        h5::read(fd, "/shared_arr", dst, h5::count{N});

        bool ok = true;
        for (std::size_t i = 0; ok && i < N; ++i) ok = (dst[i] == src[i]);
        check("std::shared_ptr<int[]>(12)      write + read", ok);
    }

    // ── 3. std::unique_ptr<T> (single element) ─────────────────────────────
    {
        auto src = std::make_unique<double>(3.14159);
        h5::write(fd, "/uniq_scalar", src);

        auto dst = std::make_unique<double>(0.0);
        h5::read(fd, "/uniq_scalar", dst);

        check("std::unique_ptr<double>         single scalar", *dst == *src);
    }

    // ── 4. std::shared_ptr<T> (single element) ─────────────────────────────
    {
        auto src = std::make_shared<std::int64_t>(0xCAFEF00DBABE);
        h5::write(fd, "/shared_scalar", src);

        auto dst = std::make_shared<std::int64_t>(0);
        h5::read(fd, "/shared_scalar", dst);

        check("std::shared_ptr<int64_t>        single scalar", *dst == *src);
    }

    // ── 5. Auto-allocating return-style read<unique_ptr<T[]>> ──────────────
    //    The library allocates `new T[n]` sized to the dataset extent. No
    //    pre-allocation, no length argument — the dataset's HDF5 extent
    //    drives the allocation.
    {
        constexpr std::size_t N = 8;
        std::unique_ptr<float[]> src(new float[N]());
        for (std::size_t i = 0; i < N; ++i) src[i] = float(i) * 0.5f;
        h5::write(fd, "/auto_alloc", src, h5::count{N});

        // The dataset extent is N; auto-allocating read sizes the unique_ptr to N.
        auto buf = h5::read<std::unique_ptr<float[]>>(fd, "/auto_alloc");
        bool ok = true;
        for (std::size_t i = 0; ok && i < N; ++i) ok = (buf[i] == src[i]);
        check("auto h5::read<unique_ptr<float[]>>(fd, path)", ok);
    }

    // ── 6. unique_ptr<Record[]> with compiler-assisted reflection ──────────
    //    The compiler generates the `register_struct<sn::point_t>()` body
    //    (we inlined it above); the smart-pointer mapper carries it through
    //    the same way it carries a `double*`. No new code in the dispatch;
    //    the two layers compose cleanly.
    {
        constexpr std::size_t N = 5;
        std::unique_ptr<sn::point_t[]> src(new sn::point_t[N]());
        for (std::size_t i = 0; i < N; ++i)
            src[i] = sn::point_t{double(i), 2.0 * i, 3.0 * i};

        h5::write(fd, "/points_uniq", src, h5::count{N});

        // Pre-allocated read.
        std::unique_ptr<sn::point_t[]> dst(new sn::point_t[N]());
        h5::read(fd, "/points_uniq", dst, h5::count{N});
        bool pre_ok = true;
        for (std::size_t i = 0; pre_ok && i < N; ++i)
            pre_ok = (dst[i].x == src[i].x && dst[i].y == src[i].y && dst[i].z == src[i].z);
        check("unique_ptr<sn::point_t[]>(5)    pre-alloc + read", pre_ok);

        // Auto-allocating return-style read on a compound POD type.
        auto auto_buf = h5::read<std::unique_ptr<sn::point_t[]>>(fd, "/points_uniq");
        bool auto_ok = true;
        for (std::size_t i = 0; auto_ok && i < N; ++i)
            auto_ok = (auto_buf[i].x == src[i].x
                    && auto_buf[i].y == src[i].y
                    && auto_buf[i].z == src[i].z);
        check("auto h5::read<unique_ptr<sn::point_t[]>>(...)  compound POD", auto_ok);
    }

    return 0;
}
