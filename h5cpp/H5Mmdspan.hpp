/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

// std::mdspan mapper — non-owning multi-dimensional view (P0009, C++23).
//
// Gated on the feature-test macro __cpp_lib_mdspan (libstdc++ 14+, libc++ 17+).
// If the standard library doesn't ship <mdspan>, this header is a no-op and
// h5cpp builds at C++17/20 without complaint.
//
// Dispatch:
//   - access_traits_t<mdspan>           kind = contiguous
//   - storage_representation_impl<mdspan>  linear_value_dataset
//   - impl::data / impl::size / impl::rank for the legacy raw paths
//
// mdspan is *non-owning*. There is no impl::get<mdspan>::ctor and there's no
// way for h5::read<mdspan<T>>(fd, path) to allocate the underlying buffer.
// The supported read pattern is buffer-out:
//
//     std::vector<double> buf(rows * cols);
//     std::mdspan<double, std::dextents<size_t, 2>> view(buf.data(), rows, cols);
//     h5::read<double>(fd, path, view.data_handle(), h5::count{rows, cols});
//
// Write side is fully structural: hand the view to h5::write and it goes.

#if defined(__cpp_lib_mdspan) && __cpp_lib_mdspan >= 202207L
#include <mdspan>
#include <array>
#include <cstddef>

// Mark mdspan as having explicit access_traits so the generic structural
// fallback (which probes .data() not .data_handle()) doesn't claim it.
namespace h5::meta {
    template <class T, class Extents, class LayoutPolicy, class AccessorPolicy>
    struct detail::has_explicit_access_traits<
        std::mdspan<T, Extents, LayoutPolicy, AccessorPolicy>> : std::true_type {};
}

// Explicit storage policy: rank-N contiguous → linear_value_dataset.
namespace h5::meta::detail_capabilities {
    template <class T, class Extents, class LayoutPolicy, class AccessorPolicy>
    struct has_explicit_storage_repr<
        std::mdspan<T, Extents, LayoutPolicy, AccessorPolicy>> : std::true_type {};

    template <class T, class Extents, class LayoutPolicy, class AccessorPolicy>
    struct storage_representation_impl<
        std::mdspan<T, Extents, LayoutPolicy, AccessorPolicy>>
        : std::integral_constant<storage_representation_t,
            storage_representation_t::linear_value_dataset> {};
}

// access_traits_t<mdspan> — contiguous kind, data via .data_handle(),
// size from the extents object.
namespace h5::meta {
    template <class T, class Extents, class LayoutPolicy, class AccessorPolicy>
    struct access_traits_t<std::mdspan<T, Extents, LayoutPolicy, AccessorPolicy>> {
        using mdspan_t   = std::mdspan<T, Extents, LayoutPolicy, AccessorPolicy>;
        using element_t  = T;
        using pointer_t  = const T*;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;

        static const T* data(const mdspan_t& s) noexcept { return s.data_handle(); }
        static T*       data(mdspan_t& s)       noexcept { return s.data_handle(); }

        static std::array<std::size_t, Extents::rank()> size(const mdspan_t& s) noexcept {
            std::array<std::size_t, Extents::rank()> dims{};
            for (std::size_t i = 0; i < Extents::rank(); ++i)
                dims[i] = s.extent(i);
            return dims;
        }
        static std::size_t bytes(const mdspan_t& s) noexcept {
            return s.size() * sizeof(T);
        }
    };
}

// Legacy raw paths used by the dataset/attribute dispatch fallbacks.
namespace h5::impl {
    template <class T, class Extents, class LayoutPolicy, class AccessorPolicy>
    struct rank<std::mdspan<T, Extents, LayoutPolicy, AccessorPolicy>>
        : public std::integral_constant<std::size_t, Extents::rank()> {};

    template <class T, class Extents, class LayoutPolicy, class AccessorPolicy>
    inline T* data(std::mdspan<T, Extents, LayoutPolicy, AccessorPolicy>& ref) noexcept {
        return ref.data_handle();
    }
    template <class T, class Extents, class LayoutPolicy, class AccessorPolicy>
    inline const T* data(
        const std::mdspan<T, Extents, LayoutPolicy, AccessorPolicy>& ref) noexcept {
        return ref.data_handle();
    }

    template <class T, class Extents, class LayoutPolicy, class AccessorPolicy>
    inline std::array<std::size_t, Extents::rank()> size(
        const std::mdspan<T, Extents, LayoutPolicy, AccessorPolicy>& ref) noexcept {
        std::array<std::size_t, Extents::rank()> dims{};
        for (std::size_t i = 0; i < Extents::rank(); ++i)
            dims[i] = ref.extent(i);
        return dims;
    }
}

#define H5CPP_HAS_MDSPAN 1
#endif // __cpp_lib_mdspan
