/* This file is part of the H5CPP project and is licensed under the MIT License.
 *
 * Copyright © 2018–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
 * Contact: info@vargaconsulting.ca
 *
 * h5::view<T>(ds_t) — C++20 ranges streaming view over a rank-1 chunked HDF5 dataset.
 *
 * Only available when compiled with -std=c++20 or later.
 *
 * Usage:
 *   h5::ds_t ds = h5::open(fd, "my_dataset");
 *   for (auto val : h5::view<double>(ds))
 *       std::cout << val << '\n';
 *
 * Constraints:
 *   - Dataset must be rank-1 and chunked.
 *   - Any filter chain supported by h5::impl::basic_pipeline_t is accepted
 *     (uncompressed, gzip/deflate, LZ4, Zstd, …).
 */

#pragma once

#if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)

#include <hdf5.h>
#include "H5config.hpp"
#include "H5Eall.hpp"
#include "H5Iall.hpp"
#include "H5Sall.hpp"
#include "H5capi.hpp"
#include "H5Zpipeline.hpp"
#include "H5Zpipeline_basic.hpp"

#include <iterator>
#include <memory>
#include <numeric>
#include <ranges>
#include <stdexcept>

namespace h5::impl {

    /* Passkey tag — restricts iterator_t's constructor to h5::view<T>. */
    struct view_access {};

    [[nodiscard]] inline hsize_t get_ds_element_count(h5::ds_t ds) {
        hsize_t dims[H5CPP_MAX_RANK];
        h5::sp_t fs = h5::get_space(ds);
        int rank = static_cast<int>(h5::get_simple_extent_dims(fs, dims, nullptr));
        if (rank != 1)
            throw std::runtime_error("h5::view: only rank-1 datasets are supported");
        return dims[0];
    }

    template<typename T>
    struct iterator_t {
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;
        using reference         = const T&;
        using pointer           = const T*;

        /* default-constructed iterator serves as the end sentinel */
        iterator_t() : offset_(0), total_(0), chunk_n_(1) {}

        /* Keyed constructor — only callable by code that holds a view_access key */
        iterator_t(view_access, h5::ds_t ds, hsize_t offset, hsize_t total)
            : offset_(offset), total_(total), chunk_n_(1)
        {
            if (offset_ >= total_)
                return; // end sentinel — no pipeline needed

            pipeline_ = std::make_shared<basic_pipeline_t>();
            h5::dcpl_t dcpl = h5::get_dcpl(ds);
            pipeline_->set_cache(dcpl, sizeof(T));
            pipeline_->ds   = ds;
            pipeline_->dxpl = h5::default_dxpl;
            chunk_n_ = pipeline_->n;
            load_chunk();
        }

        reference operator*() const {
            return reinterpret_cast<const T*>(pipeline_->chunk0)[offset_ % chunk_n_];
        }
        pointer operator->() const { return &**this; }

        iterator_t& operator++() {
            ++offset_;
            if (offset_ < total_ && offset_ % chunk_n_ == 0)
                load_chunk();
            return *this;
        }
        iterator_t operator++(int) {
            iterator_t tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const iterator_t& a, const iterator_t& b) noexcept {
            return a.offset_ == b.offset_;
        }
        friend bool operator!=(const iterator_t& a, const iterator_t& b) noexcept {
            return !(a == b);
        }

    private:
        void load_chunk() {
            hsize_t chunk_offset = (offset_ / chunk_n_) * chunk_n_;
            pipeline_->read_chunk(&chunk_offset, pipeline_->block_size, pipeline_->chunk0);
        }

        std::shared_ptr<basic_pipeline_t> pipeline_;
        hsize_t offset_;
        hsize_t total_;
        hsize_t chunk_n_;
    };

} // namespace h5::impl

namespace h5 {
    /**
     * @brief Lightweight range wrapper that avoids std::ranges::subrange
     *        recursive-inheritance issues with clang + libstdc++-14.
     */
    template<typename Iter>
    struct view_range {
        Iter _M_begin;
        Iter _M_end;

        constexpr Iter begin() const { return _M_begin; }
        constexpr Iter end() const { return _M_end; }
    };

    /**
     * @brief Return a C++20 input range that streams a rank-1 chunked dataset
     *        one chunk at a time.
     *
     * The returned range satisfies std::ranges::input_range and can be used
     * in a range-for loop or passed to any std::ranges algorithm.
     *
     * \tpar_T
     * \par_ds
     * @return view_range over `h5::impl::iterator_t<T>` — satisfies `std::ranges::input_range`; dataset must be rank-1 and chunked.
     */
    template<typename T>
    [[nodiscard]] view_range<impl::iterator_t<T>> view(h5::ds_t ds) {
        using Iter = impl::iterator_t<T>;
        using Key  = impl::view_access;
        hsize_t n  = impl::get_ds_element_count(ds);
        return view_range<Iter>{ Iter(Key{}, ds, 0, n), Iter(Key{}, ds, n, n) };
    }
} // namespace h5

#endif // __cplusplus >= 202002L
