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
    }

    pool_pipeline_t(const pool_pipeline_t&) = delete;
    pool_pipeline_t& operator=(const pool_pipeline_t&) = delete;
    pool_pipeline_t(pool_pipeline_t&&) = delete;
    pool_pipeline_t& operator=(pool_pipeline_t&&) = delete;

    // CRTP entry points called by pipeline_t<>::write / pipeline_t<>::read
    // via split_to_chunk_write / split_to_chunk_read.
    void write_chunk_impl(const hsize_t* offset, std::size_t nbytes, const void* src);
    void read_chunk_impl (const hsize_t* offset, std::size_t nbytes, void*       dst);

    // Public quiesce — block until every in-flight future has been
    // drained.  Called by pt_t::flush so users get the expected
    // "data on disk after flush returns" semantics for the pool path.
    void drain() {
        while (!in_flight_.empty()) drain_in_flight(/*blocking=*/true);
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

    std::shared_ptr<worker_pool_t>       pool_;
    unsigned                              cap_{0};
    std::deque<std::future<result_t>>     in_flight_;
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

// ─── read path (synchronous for v1) ──────────────────────────────────────────
//
// Phase 1.3.3 keeps read synchronous, identical to basic_pipeline_t::read_chunk_impl.
// Parallel decompression is a deliberate follow-up: it requires read-ahead
// (the read path consumes chunks in order, but the user's buffer slots are
// known up-front, so prefetching can fan out across the pool).  Tracked in
// Phase 1.5+ work.

inline void pool_pipeline_t::read_chunk_impl(const hsize_t* offset_in,
                                              std::size_t nbytes,
                                              void* /*dst*/) {
    std::size_t length = nbytes;
    std::uint32_t filter_mask;

    if (tail == 0) {
#if H5_VERSION_GE(2,0,0)
        std::size_t buf_size = nbytes;
        H5Dread_chunk2(ds, dxpl, offset_in, &filter_mask, chunk0, &buf_size);
#else
        H5Dread_chunk(ds, dxpl, offset_in, &filter_mask, chunk0);
#endif
        return;
    }

    void* read_target = (tail % 2 == 1) ? chunk1 : chunk0;
#if H5_VERSION_GE(2,0,0)
    std::size_t buf_size = nbytes;
    H5Dread_chunk2(ds, dxpl, offset_in, &filter_mask, read_target, &buf_size);
#else
    H5Dread_chunk(ds, dxpl, offset_in, &filter_mask, read_target);
#endif

    void* src = read_target;
    void* dst = (read_target == chunk0) ? static_cast<void*>(chunk1)
                                        : static_cast<void*>(chunk0);
    for (hsize_t j = tail; j > 0; --j) {
        const hsize_t fi = j - 1;
        length = filter[fi](dst, src, length,
                            flags[fi] | H5Z_FLAG_REVERSE,
                            cd_size[fi], cd_values[fi]);
        std::swap(src, dst);
    }
}

} // namespace h5::impl
