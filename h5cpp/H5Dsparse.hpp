/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include "H5capi.hpp"
#include "H5Tsparse.hpp"
#include "H5Gcreate.hpp"
#include "H5Gopen.hpp"
#include "H5Awrite.hpp"
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

// Sparse matrix / vector I/O.
//
// Public API:
//   h5::gr_t h5::write(parent, "name", spmat);            // writes CSC group
//   Sparse   h5::read<Sparse>(parent, "name");            // reads it back
//
// Parent may be h5::fd_t, h5::gr_t, or hid_t. The dense h5::write / h5::read
// overloads (in H5Dwrite.hpp / H5Dread.hpp) exclude sparse T via the
// is_sparse_v guard so these overloads win without ambiguity.

namespace h5 {
namespace impl::sparse {

    // Shadow the h5::impl::hid_t<T,F> template alias with the HDF5 C typedef
    // so HDF5 macros like H5S_ALL / H5P_DEFAULT (which expand to `((hid_t)0)`)
    // resolve to the C type inside this namespace.
    using hid_t = ::hid_t;

    // Write a contiguous 1-D dataset of `n` elements of type T under `loc`.
    // Uses the raw HDF5 C API so this helper works for any location id
    // (group, file). Returns nothing; throws on HDF5 error.
    template <class T>
    inline void write_1d(::hid_t loc, const char* name, const T* ptr, hsize_t n) {
        h5::meta::resolved_type_t<T> type;
        hsize_t dims[1] = { n };
        ::hid_t space = H5Screate_simple(1, dims, nullptr);
        ::hid_t ds = H5Dcreate2(loc, name, static_cast<::hid_t>(type), space,
                                H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (ds < 0) {
            H5Sclose(space);
            throw h5::error::io::dataset::create(
                std::string("h5::write(sparse): failed to create dataset '") + name + "'");
        }
        herr_t err = H5Dwrite(ds, static_cast<::hid_t>(type),
                              H5S_ALL, H5S_ALL, H5P_DEFAULT, ptr);
        H5Dclose(ds);
        H5Sclose(space);
        if (err < 0)
            throw h5::error::io::dataset::write(
                std::string("h5::write(sparse): H5Dwrite failed for '") + name + "'");
    }

    // Read a 1-D dataset of element type T into a std::vector<T>. Throws if
    // the dataset is not 1-D.
    template <class T>
    inline std::vector<T> read_1d(::hid_t loc, const char* name) {
        ::hid_t ds = H5Dopen2(loc, name, H5P_DEFAULT);
        if (ds < 0)
            throw h5::error::io::dataset::read(
                std::string("h5::read(sparse): dataset '") + name + "' not found");
        ::hid_t space = H5Dget_space(ds);
        int rank = H5Sget_simple_extent_ndims(space);
        if (rank != 1) {
            H5Sclose(space); H5Dclose(ds);
            throw h5::error::io::dataset::read(
                std::string("h5::read(sparse): dataset '") + name + "' is not 1-D");
        }
        hsize_t n = 0;
        H5Sget_simple_extent_dims(space, &n, nullptr);
        std::vector<T> out(static_cast<std::size_t>(n));
        h5::meta::resolved_type_t<T> type;
        herr_t err = H5Dread(ds, static_cast<::hid_t>(type),
                             H5S_ALL, H5S_ALL, H5P_DEFAULT, out.data());
        H5Sclose(space); H5Dclose(ds);
        if (err < 0)
            throw h5::error::io::dataset::read(
                std::string("h5::read(sparse): H5Dread failed for '") + name + "'");
        return out;
    }

    // Read a scalar VLEN string attribute. We can't reuse h5::aread<std::string>
    // because it is keyed to h5::ds_t — but the underlying H5A* C API accepts
    // any location, so this helper is a thin wrapper.
    inline std::string read_string_attr(::hid_t loc, const char* name) {
        ::hid_t attr = H5Aopen(loc, name, H5P_DEFAULT);
        if (attr < 0)
            throw h5::error::io::attribute::read(
                std::string("h5::read(sparse): attribute '") + name + "' not found");
        ::hid_t atype = H5Aget_type(attr);
        // copy to a known VLEN UTF-8 string type so the read produces a char*
        ::hid_t mem_type = H5Tcopy(H5T_C_S1);
        H5Tset_size(mem_type, H5T_VARIABLE);
        H5Tset_cset(mem_type, H5T_CSET_UTF8);
        char* raw = nullptr;
        herr_t err = H5Aread(attr, mem_type, &raw);
        std::string out = (err >= 0 && raw) ? std::string(raw) : std::string{};
        // reclaim if HDF5 allocated
        if (raw) {
            ::hid_t sp = H5Aget_space(attr);
#if H5_VERSION_GE(1,12,0)
            H5Treclaim(mem_type, sp, H5P_DEFAULT, &raw);
#else
            H5Dvlen_reclaim(mem_type, sp, H5P_DEFAULT, &raw);
#endif
            H5Sclose(sp);
        }
        H5Tclose(mem_type);
        H5Tclose(atype);
        H5Aclose(attr);
        if (err < 0)
            throw h5::error::io::attribute::read(
                std::string("h5::read(sparse): H5Aread failed for '") + name + "'");
        return out;
    }

} // namespace impl::sparse

    // -------- write -----------------------------------------------------
    // SFINAE: this overload activates only for sparse T; the dense
    // h5::write(fd, path, ref) overload excludes is_sparse_v<T>.
    template <class T, class LOC,
              class = std::enable_if_t<h5::impl::is_valid_group_parent<LOC>::value
                                    && h5::meta::is_sparse_v<T>>>
    inline h5::gr_t write(const LOC& parent, const std::string& path, const T& src) try {
        using S = h5::meta::sparse_traits<T>;
        using value_type = typename S::value_type;

        const std::size_t n_rows = S::rows(src);
        const std::size_t n_cols = S::cols(src);
        const std::size_t nnz    = S::nnz(src);

        constexpr std::size_t U32_MAX = std::numeric_limits<std::uint32_t>::max();
        if (n_rows > U32_MAX || n_cols > U32_MAX || nnz > U32_MAX)
            throw h5::error::io::dataset::write(
                "h5::write(sparse): on-disk indices are uint32; shape or nnz exceeds 2^32-1.");

        // Stage the CSC arrays into the on-disk uint32 buffers.
        std::vector<std::uint32_t> indptr(n_cols + 1);
        S::to_u32_outer(src, indptr.data());
        std::vector<std::uint32_t> indices(nnz);
        if (nnz > 0) S::to_u32_inner(src, indices.data());

        std::array<std::uint64_t, 2> shape = {
            static_cast<std::uint64_t>(n_rows),
            static_cast<std::uint64_t>(n_cols)
        };

        h5::gr_t gr = h5::gcreate(parent, path);
        const ::hid_t gid = static_cast<::hid_t>(gr);

        // Empty matrices still need a `data` dataset so the group is a
        // complete CSC record; HDF5 accepts a zero-length simple dataspace.
        impl::sparse::write_1d<value_type>(gid, h5::meta::sparse::k_data,
                                           S::values(src), static_cast<hsize_t>(nnz));
        impl::sparse::write_1d<std::uint32_t>(gid, h5::meta::sparse::k_indices,
                                              indices.data(), static_cast<hsize_t>(nnz));
        impl::sparse::write_1d<std::uint32_t>(gid, h5::meta::sparse::k_indptr,
                                              indptr.data(), static_cast<hsize_t>(n_cols + 1));
        impl::sparse::write_1d<std::uint64_t>(gid, h5::meta::sparse::k_shape,
                                              shape.data(), 2u);

        // Self-describing attributes so scipy / h5sparse / 10x readers can
        // dispatch without out-of-band knowledge.
        h5::awrite(gr, h5::meta::sparse::k_format, std::string(h5::meta::sparse::v_csc));
        h5::awrite(gr, h5::meta::sparse::k_axis,   std::string(h5::meta::sparse::v_column));
        return gr;
    } catch (const std::exception& err) {
        throw h5::error::io::dataset::write(err.what());
    }

    // -------- read ------------------------------------------------------
    // SFINAE: activates only for sparse T; the dense h5::read<T>(loc, path)
    // overload excludes is_sparse_v<T>.
    template <class T, class LOC,
              class = std::enable_if_t<h5::impl::is_valid_group_parent<LOC>::value
                                    && h5::meta::is_sparse_v<T>>>
    inline T read(const LOC& parent, const std::string& path) try {
        using S = h5::meta::sparse_traits<T>;
        using value_type = typename S::value_type;

        h5::gr_t gr = h5::gopen(parent, path);
        const ::hid_t gid = static_cast<::hid_t>(gr);

        // Validate format; future RowMajor / BCRS / etc. support will branch here.
        const std::string fmt = impl::sparse::read_string_attr(gid, h5::meta::sparse::k_format);
        if (fmt != h5::meta::sparse::v_csc)
            throw h5::error::io::dataset::read(
                "h5::read<sparse>: expected @format='csc', got '" + fmt + "'");

        auto shape   = impl::sparse::read_1d<std::uint64_t>(gid, h5::meta::sparse::k_shape);
        if (shape.size() != 2)
            throw h5::error::io::dataset::read(
                "h5::read<sparse>: @shape must have exactly 2 elements [rows, cols]");
        const std::size_t n_rows = static_cast<std::size_t>(shape[0]);
        const std::size_t n_cols = static_cast<std::size_t>(shape[1]);

        auto indptr  = impl::sparse::read_1d<std::uint32_t>(gid, h5::meta::sparse::k_indptr);
        auto indices = impl::sparse::read_1d<std::uint32_t>(gid, h5::meta::sparse::k_indices);
        auto data    = impl::sparse::read_1d<value_type>   (gid, h5::meta::sparse::k_data);

        if (indptr.size() != n_cols + 1)
            throw h5::error::io::dataset::read(
                "h5::read<sparse>: indptr length doesn't match cols+1");
        if (indices.size() != data.size())
            throw h5::error::io::dataset::read(
                "h5::read<sparse>: indices/data length mismatch");

        return S::construct(n_rows, n_cols, indices.size(),
                            indptr.data(), indices.data(), data.data());
    } catch (const std::exception& err) {
        throw h5::error::io::dataset::read(err.what());
    }

} // namespace h5
