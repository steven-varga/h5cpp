/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

// FAPL-scoped parallel filter pipeline — second CRTP descendant of
// pipeline_t<Derived> alongside basic_pipeline_t (synchronous).
//
// pool_pipeline_t owns a strong reference to an external worker_pool_t
// (resolved from the file's FAPL) and submits compress closures to it;
// H5Dwrite_chunk runs on the calling thread.
//
// The pipeline reuses pipeline_t<>::write/read (and via that
// split_to_chunk_write/read) for chunk decomposition, so consumer sites
// — pt_t, h5::write, h5::read — never see the pool dispatch directly.
// They just construct a pool_pipeline_t when their file's FAPL has one.

#include "H5Zpipeline.hpp"
#include "H5Pthreads.hpp"

#include <array>
#include <cstring>
#include <deque>
#include <future>
#include <memory>

namespace h5::impl {

struct pool_pipeline_t : public pipeline_t<pool_pipeline_t> {
    pool_pipeline_t() = default;   // variant requires default-constructible
                                   // when this is the chosen alternative; pool_ stays null
                                   // and dispatch fast-fails (no pool installed).

    pool_pipeline_t(std::shared_ptr<worker_pool_t> pool, unsigned cap)
        : pool_(std::move(pool)), cap_(cap) {}

    ~pool_pipeline_t() {
        // Drain any remaining in-flight work in submission order before the
        // pipeline (and its shared_ptr to the pool) goes away.  Blocks the
        // destructor on each front future.
        while (!in_flight_.empty()) drain_in_flight(/*blocking=*/true);
        while (!read_in_flight_.empty()) drain_in_flight_read(nullptr, /*blocking=*/true);
    }

    pool_pipeline_t(const pool_pipeline_t&) = delete;
    pool_pipeline_t& operator=(const pool_pipeline_t&) = delete;
    pool_pipeline_t(pool_pipeline_t&&) = delete;
    pool_pipeline_t& operator=(pool_pipeline_t&&) = delete;

    // CRTP entry points called by pipeline_t<>::write / pipeline_t<>::read
    // via split_to_chunk_write / split_to_chunk_read.
    void write_chunk_impl(const hsize_t* offset, std::size_t nbytes, const void* src);
    void read_chunk_impl (const hsize_t* offset, std::size_t nbytes, void*       dst);

    // Parallel read override — shadows pipeline_t<pool_pipeline_t>::read.
    // Rank-1 chunked datasets are decompressed across the worker pool;
    // rank>1 falls back to the synchronous base-class path.
    void read(const h5::ds_t& ds, const h5::offset_t& offset,
              const h5::stride_t& stride, const h5::block_t& block,
              const h5::count_t& count, const h5::dxpl_t& dxpl, void* ptr);

    // Public quiesce — block until every in-flight future has been
    // drained.  Called by pt_t::flush so users get the expected
    // "data on disk after flush returns" semantics for the pool path.
    void drain() {
        while (!in_flight_.empty()) drain_in_flight(/*blocking=*/true);
        while (!read_in_flight_.empty()) drain_in_flight_read(nullptr, /*blocking=*/true);
    }

private:
    // Worker-produced compressed chunk, ready for H5Dwrite_chunk on the
    // calling thread.  unique_ptr<std::byte[]> keeps it move-only and
    // aligned to operator new[]'s default alignment.
    struct result_t {
        std::unique_ptr<std::byte[]>        data;
        std::size_t                          nbytes{0};
        std::uint32_t                        mask{0};
        std::array<hsize_t, H5CPP_MAX_RANK>  offset{};
    };

    void drain_in_flight(bool blocking);

    // Result of a worker decompression task for the parallel read path.
    struct read_result_t {
        std::unique_ptr<std::byte[]> data;
        std::size_t                   copy_size{0};
        std::size_t                   dst_offset{0};
    };

    void drain_in_flight_read(char* dst, bool blocking);

    std::shared_ptr<worker_pool_t>       pool_;
    unsigned                              cap_{0};
    std::deque<std::future<result_t>>     in_flight_;
    std::deque<std::future<read_result_t>> read_in_flight_;
    char*                                 read_dst_ = nullptr;
};

// ─── write path ──────────────────────────────────────────────────────────────

inline void pool_pipeline_t::drain_in_flight(bool blocking) {
    using namespace std::chrono_literals;
    if (blocking) {
        if (in_flight_.empty()) return;
        auto r = in_flight_.front().get();   // blocks
        H5Dwrite_chunk(static_cast<::hid_t>(this->ds), static_cast<::hid_t>(this->dxpl),
                       r.mask, r.offset.data(), r.nbytes, r.data.get());
        in_flight_.pop_front();
        return;
    }
    while (!in_flight_.empty() &&
           in_flight_.front().wait_for(0s) == std::future_status::ready) {
        auto r = in_flight_.front().get();
        H5Dwrite_chunk(static_cast<::hid_t>(this->ds), static_cast<::hid_t>(this->dxpl),
                       r.mask, r.offset.data(), r.nbytes, r.data.get());
        in_flight_.pop_front();
    }
}

inline void pool_pipeline_t::write_chunk_impl(const hsize_t* offset_in,
                                              std::size_t nbytes,
                                              const void* src) {
    if (!pool_) {
        // No pool was wired in (default-constructed alternative).  Fall
        // through to the synchronous filter chain identical to
        // basic_pipeline_t::write_chunk_impl.  This branch shouldn't be
        // reached in normal use; it exists to keep the variant safe.
        size_t length = nbytes;
        void *in = chunk0, *out = chunk1, *tmp = chunk0;
        std::uint32_t mask = 0;
        switch (tail) {
            case 0:
                H5Dwrite_chunk(static_cast<::hid_t>(ds), static_cast<::hid_t>(dxpl),
                               0, offset_in, nbytes, src);
                return;
            case 1:
                length = filter[0](out, src, nbytes, flags[0], cd_size[0], cd_values[0]);
                if (!length) mask = 1u;
                [[fallthrough]];
            default:
                for (hsize_t j = 1; j < tail; ++j) {
                    tmp = in; in = out; out = tmp;
                    length = filter[j](out, in, length, flags[j], cd_size[j], cd_values[j]);
                    if (!length) mask |= (1u << j);
                }
                H5Dwrite_chunk(static_cast<::hid_t>(ds), static_cast<::hid_t>(dxpl),
                               mask, offset_in, length, out);
        }
        return;
    }

    // Snapshot filter chain into a POD captured by value in the closure.
    struct filter_chain_t {
        filter::call_t filter[H5CPP_MAX_FILTER];
        unsigned       flags[H5CPP_MAX_FILTER];
        std::size_t    cd_size[H5CPP_MAX_FILTER];
        unsigned       cd_values[H5CPP_MAX_FILTER][H5CPP_MAX_FILTER_PARAM];
        hsize_t        tail;
    };
    filter_chain_t fc{};
    fc.tail = this->tail;
    std::memcpy(fc.filter,    this->filter,    sizeof(fc.filter));
    std::memcpy(fc.flags,     this->flags,     sizeof(fc.flags));
    std::memcpy(fc.cd_size,   this->cd_size,   sizeof(fc.cd_size));
    std::memcpy(fc.cd_values, this->cd_values, sizeof(fc.cd_values));

    // Worker-owned input buffer.
    auto raw = std::make_unique<std::byte[]>(nbytes);
    std::memcpy(raw.get(), src, nbytes);

    std::array<hsize_t, H5CPP_MAX_RANK> off{};
    std::copy(offset_in, offset_in + this->rank, off.begin());

    auto fut = pool_->submit(
        [raw = std::move(raw), nbytes, off, fc]() mutable -> result_t {
            result_t out;
            out.offset = off;

            if (fc.tail == 0) {
                out.data   = std::move(raw);
                out.nbytes = nbytes;
                out.mask   = 0;
                return out;
            }

            const std::size_t scratch = filter::filter_scratch_bound(nbytes);
            auto wbuf0 = std::make_unique<std::byte[]>(scratch);
            auto wbuf1 = std::make_unique<std::byte[]>(scratch);

            std::size_t   length = nbytes;
            std::uint32_t mask   = 0;

            length = fc.filter[0](wbuf0.get(), raw.get(), length,
                                  fc.flags[0], fc.cd_size[0], fc.cd_values[0]);
            if (!length) mask |= 1u;

            void* in_buf  = wbuf0.get();
            void* out_buf = wbuf1.get();
            for (hsize_t j = 1; j < fc.tail; ++j) {
                length = fc.filter[j](out_buf, in_buf, length,
                                      fc.flags[j], fc.cd_size[j], fc.cd_values[j]);
                if (!length) mask |= (1u << j);
                std::swap(in_buf, out_buf);
            }

            out.data = std::make_unique<std::byte[]>(length);
            std::memcpy(out.data.get(), in_buf, length);
            out.nbytes = length;
            out.mask   = mask;
            return out;
        });

    in_flight_.push_back(std::move(fut));

    // Opportunistic drain.
    drain_in_flight(/*blocking=*/false);

    // Bounded back-pressure: block on the front future when the deque
    // reaches the cap.  Producer memory ≤ cap × chunk_size per pipeline.
    while (in_flight_.size() >= cap_)
        drain_in_flight(/*blocking=*/true);
}

// ─── read path ───────────────────────────────────────────────────────────────
//
// Phase 1.3.3 kept read synchronous.  Phase 1.5 adds parallel decompression
// for rank-1 chunked datasets by running the reverse filter chain on the
// worker pool while the calling thread performs sequential H5Dread_chunk I/O.

inline void pool_pipeline_t::drain_in_flight_read(char* dst, bool blocking) {
    using namespace std::chrono_literals;
    char* base = dst ? dst : read_dst_;
    if (!base) return;  // no destination available (shouldn't happen outside read())

    if (blocking) {
        if (read_in_flight_.empty()) return;
        auto r = read_in_flight_.front().get();
        std::memcpy(base + r.dst_offset, r.data.get(), r.copy_size);
        read_in_flight_.pop_front();
        return;
    }
    while (!read_in_flight_.empty() &&
           read_in_flight_.front().wait_for(0s) == std::future_status::ready) {
        auto r = read_in_flight_.front().get();
        std::memcpy(base + r.dst_offset, r.data.get(), r.copy_size);
        read_in_flight_.pop_front();
    }
}

inline void pool_pipeline_t::read(const h5::ds_t& ds, const h5::offset_t& offset,
                                  const h5::stride_t& stride, const h5::block_t& block,
                                  const h5::count_t& count, const h5::dxpl_t& dxpl, void* ptr) {
    // Fallback to synchronous base-class path when no pool is present or when
    // the dataset is not rank-1.  Rank>1 parallel decompression is future work.
    if (!pool_ || rank != 1) {
        pipeline_t<pool_pipeline_t>::read(ds, offset, stride, block, count, dxpl, ptr);
        return;
    }

    this->dxpl = dxpl;
    this->ds   = ds;

    h5::offset_t offset_;
    h5::count_t  count_;
    for (hsize_t i = 0; i < rank; ++i) {
        offset_[i] = offset[i];
        count_[i]  = count[i] * block[i];
    }

    // Rank-1 chunk decomposition (mirrors split_to_chunk_read).
    hsize_t n  = count_[0];          // total elements to read
    hsize_t b  = B[0];               // elements per chunk
    hsize_t ry = n % b;              // trailing partial chunk

    // Snapshot filter chain into a POD closure capture (same pattern as write).
    struct filter_chain_t {
        filter::call_t filter[H5CPP_MAX_FILTER];
        unsigned       flags[H5CPP_MAX_FILTER];
        std::size_t    cd_size[H5CPP_MAX_FILTER];
        unsigned       cd_values[H5CPP_MAX_FILTER][H5CPP_MAX_FILTER_PARAM];
        hsize_t        tail;
    };
    filter_chain_t fc{};
    fc.tail = this->tail;
    std::memcpy(fc.filter,    this->filter,    sizeof(fc.filter));
    std::memcpy(fc.flags,     this->flags,     sizeof(fc.flags));
    std::memcpy(fc.cd_size,   this->cd_size,   sizeof(fc.cd_size));
    std::memcpy(fc.cd_values, this->cd_values, sizeof(fc.cd_values));

    const std::size_t scratch = filter::filter_scratch_bound(block_size);
    char* dst = static_cast<char*>(ptr);
    read_dst_ = dst;
    const std::size_t block_sz = this->block_size;

    for (hsize_t j = 0; j < n; j += b) {
        hsize_t chunk_offset = j + offset_[0];
        hsize_t copy_size = (ry != 0 && j == n - ry) ? ry * element_size : b * element_size;
        hsize_t dst_offset = j * element_size;

        // Main thread performs HDF5 I/O (H5Dread_chunk is not thread-safe).
        auto compressed = std::make_unique<std::byte[]>(scratch);
        std::uint32_t filter_mask = 0;
        hsize_t C[1] = {chunk_offset};
        std::size_t compressed_size = block_sz;   // pre-2.0 default (deflate stream self-terminates)
#if H5_VERSION_GE(2,0,0)
        compressed_size = scratch;
        H5Dread_chunk2(static_cast<::hid_t>(ds), static_cast<::hid_t>(dxpl),
                       C, &filter_mask, compressed.get(), &compressed_size);   // -> actual compressed bytes
#else
        H5Dread_chunk(static_cast<::hid_t>(ds), static_cast<::hid_t>(dxpl),
                      C, &filter_mask, compressed.get());
#endif

        // Submit decompression to the worker pool.
        auto fut = pool_->submit([
            compressed = std::move(compressed), block_sz, copy_size,
            dst_offset, scratch, fc, compressed_size, filter_mask
        ]() mutable -> read_result_t {
            read_result_t result;
            result.copy_size  = copy_size;
            result.dst_offset = dst_offset;

            if (fc.tail == 0) {
                result.data = std::make_unique<std::byte[]>(copy_size);
                std::memcpy(result.data.get(), compressed.get(), copy_size);
                return result;
            }

            auto work_buf = std::make_unique<std::byte[]>(scratch);
            void* src = compressed.get();
            void* dst_buf = work_buf.get();
            std::size_t length = compressed_size;   // compressed input length for the first reverse filter
            (void)block_sz;

            for (hsize_t fi = fc.tail; fi > 0; --fi) {
                const hsize_t idx = fi - 1;
                if (filter_mask & (1u << idx))     // chunk stored without this filter — pass through
                    std::memcpy(dst_buf, src, length);
                else
                    length = fc.filter[idx](dst_buf, src, length,
                                            fc.flags[idx] | H5Z_FLAG_REVERSE,
                                            fc.cd_size[idx], fc.cd_values[idx]);
                std::swap(src, dst_buf);
            }

            result.data = std::make_unique<std::byte[]>(copy_size);
            std::memcpy(result.data.get(), src, copy_size);
            return result;
        });

        read_in_flight_.push_back(std::move(fut));

        // Opportunistic drain — if front future is ready, copy and free buffer.
        drain_in_flight_read(dst, /*blocking=*/false);

        // Bounded back-pressure: block when the deque reaches the cap.
        while (read_in_flight_.size() >= cap_)
            drain_in_flight_read(dst, /*blocking=*/true);
    }

    // Drain any remaining decompression tasks.
    while (!read_in_flight_.empty())
        drain_in_flight_read(dst, /*blocking=*/true);

    read_dst_ = nullptr;
}

inline void pool_pipeline_t::read_chunk_impl(const hsize_t* offset_in,
                                              std::size_t nbytes,
                                              void* /*dst*/) {
    std::size_t length = nbytes;
    std::uint32_t filter_mask;

    if (tail == 0) {
#if H5_VERSION_GE(2,0,0)
        std::size_t buf_size = nbytes;
        H5Dread_chunk2(static_cast<::hid_t>(ds), dxpl, offset_in, &filter_mask, chunk0, &buf_size);
#else
        H5Dread_chunk(static_cast<::hid_t>(ds), dxpl, offset_in, &filter_mask, chunk0);
#endif
        return;
    }

    void* read_target = (tail % 2 == 1) ? chunk1 : chunk0;
#if H5_VERSION_GE(2,0,0)
    std::size_t buf_size = filter::filter_scratch_bound(nbytes);   // buffer capacity (expanding filters); see H5Zpipeline_basic.hpp
    H5Dread_chunk2(static_cast<::hid_t>(ds), dxpl, offset_in, &filter_mask, read_target, &buf_size);
    length = buf_size;   // OUT: stored chunk bytes — the reverse-filter input size
#else
    H5Dread_chunk(static_cast<::hid_t>(ds), dxpl, offset_in, &filter_mask, read_target);
#endif

    void* src = read_target;
    void* dst = (read_target == chunk0) ? static_cast<void*>(chunk1)
                                        : static_cast<void*>(chunk0);
    for (hsize_t j = tail; j > 0; --j) {
        const hsize_t fi = j - 1;
        if (filter_mask & (1u << fi))      // chunk stored without this filter — pass through
            std::memcpy(dst, src, length);
        else
            length = filter[fi](dst, src, length,
                                flags[fi] | H5Z_FLAG_REVERSE,
                                cd_size[fi], cd_values[fi]);
        std::swap(src, dst);
    }
}

} // namespace h5::impl
