/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
 *
 * Smart-pointer mapper.
 *
 * Three integration layers:
 *
 *   1. Forwarding overloads — h5::write(fd, path, smart_ptr, args...) and
 *      h5::read(fd, path, smart_ptr, args...) accept std::unique_ptr<T[]>,
 *      std::shared_ptr<T[]>, std::unique_ptr<T>, std::shared_ptr<T> and
 *      delegate to the existing raw-pointer write/read paths via .get().
 *
 *   2. Traits glue — impl::rank / decay / get / data plus
 *      meta::access_traits_t and storage_representation specs so the
 *      return-style read<std::unique_ptr<T[]>>(fd, path, ...) constructs a
 *      freshly-allocated buffer sized to the dataset extent.
 *
 *   3. Single-element smart pointers — write/read of a single T pointed to
 *      by unique_ptr<T> / shared_ptr<T>, treated as a 1-element dataset.
 *
 * NOTE: the smart pointer does NOT carry a length. For h5::write the caller
 * MUST pass an explicit h5::count{...}; there is no shape deduction from the
 * pointer alone. The auto-allocating read derives the shape from the
 * dataset's HDF5 extent.
 */
#pragma once

#include <memory>

namespace h5::impl {
    // ── decay: unique_ptr<T[]> / shared_ptr<T[]> → T ────────────────────────
    template <class T> struct detail::has_explicit_decay<std::unique_ptr<T[]>> : std::true_type {};
    template <class T> struct detail::has_explicit_decay<std::shared_ptr<T[]>> : std::true_type {};
    template <class T> struct decay<std::unique_ptr<T[]>> { using type = T; };
    template <class T> struct decay<std::shared_ptr<T[]>> { using type = T; };

    // Single-T variants. T must itself be a scalar (numeric/POD struct).
    template <class T> struct detail::has_explicit_decay<std::unique_ptr<T>> : std::true_type {};
    template <class T> struct detail::has_explicit_decay<std::shared_ptr<T>> : std::true_type {};
    template <class T> struct decay<std::unique_ptr<T>> { using type = T; };
    template <class T> struct decay<std::shared_ptr<T>> { using type = T; };

    // ── data(): forward to .get() ───────────────────────────────────────────
    template <class T> inline const T* data(const std::unique_ptr<T[]>& p) { return p.get(); }
    template <class T> inline       T* data(      std::unique_ptr<T[]>& p) { return p.get(); }
    template <class T> inline const T* data(const std::shared_ptr<T[]>& p) { return p.get(); }
    template <class T> inline       T* data(      std::shared_ptr<T[]>& p) { return p.get(); }
    template <class T> inline const T* data(const std::unique_ptr<T>&   p) { return p.get(); }
    template <class T> inline       T* data(      std::unique_ptr<T>&   p) { return p.get(); }
    template <class T> inline const T* data(const std::shared_ptr<T>&   p) { return p.get(); }
    template <class T> inline       T* data(      std::shared_ptr<T>&   p) { return p.get(); }

    // ── rank ────────────────────────────────────────────────────────────────
    // Array smart pointers report rank=1 so the read<T>(ds) dispatch takes the
    // by-name path (impl::get<T>::ctor + impl::data) at H5Dread.hpp:638
    // instead of the detection-driven .data()/.size() path.
    template <class T> struct rank<std::unique_ptr<T[]>> : public std::integral_constant<size_t, 1> {};
    template <class T> struct rank<std::shared_ptr<T[]>> : public std::integral_constant<size_t, 1> {};
    template <class T> struct rank<std::unique_ptr<T>>   : public std::integral_constant<size_t, 0> {};
    template <class T> struct rank<std::shared_ptr<T>>   : public std::integral_constant<size_t, 0> {};

    // ── size(): smart pointer has no stored length ─────────────────────────
    // Returns a sentinel {0}. Code paths that need an actual size must derive
    // it from the dataset extent or from an explicit h5::count argument — the
    // forwarding overloads below ensure traits::size is never read for these
    // types.
    template <class T> inline std::array<size_t, 1> size(const std::unique_ptr<T[]>&) { return {0}; }
    template <class T> inline std::array<size_t, 1> size(const std::shared_ptr<T[]>&) { return {0}; }
    template <class T> inline std::array<size_t, 0> size(const std::unique_ptr<T>&)   { return {}; }
    template <class T> inline std::array<size_t, 0> size(const std::shared_ptr<T>&)   { return {}; }

    // ── get<>::ctor — allocate fresh storage sized to dataset extent ───────
    template <class T> struct get<std::unique_ptr<T[]>> {
        template <class Dims>
        static inline std::unique_ptr<T[]> ctor(const Dims& dims) {
            std::size_t n = 1;
            for (std::size_t i = 0; i < dims.size(); ++i) n *= static_cast<std::size_t>(dims[i]);
            return std::unique_ptr<T[]>(new T[n]());
        }
    };
    template <class T> struct get<std::shared_ptr<T[]>> {
        template <class Dims>
        static inline std::shared_ptr<T[]> ctor(const Dims& dims) {
            std::size_t n = 1;
            for (std::size_t i = 0; i < dims.size(); ++i) n *= static_cast<std::size_t>(dims[i]);
            return std::shared_ptr<T[]>(new T[n](), std::default_delete<T[]>());
        }
    };
    template <class T> struct get<std::unique_ptr<T>> {
        template <class Dims>
        static inline std::unique_ptr<T> ctor(const Dims&) { return std::unique_ptr<T>(new T()); }
    };
    template <class T> struct get<std::shared_ptr<T>> {
        template <class Dims>
        static inline std::shared_ptr<T> ctor(const Dims&) { return std::make_shared<T>(); }
    };
}

namespace h5::meta {
    template <class T> struct is_contiguous<std::unique_ptr<T[]>> : std::true_type {};
    template <class T> struct is_contiguous<std::shared_ptr<T[]>> : std::true_type {};
    template <class T> struct is_contiguous<std::unique_ptr<T>>   : std::true_type {};
    template <class T> struct is_contiguous<std::shared_ptr<T>>   : std::true_type {};

    template <class T> struct detail::has_explicit_access_traits<std::unique_ptr<T[]>> : std::true_type {};
    template <class T> struct detail::has_explicit_access_traits<std::shared_ptr<T[]>> : std::true_type {};
    template <class T> struct detail::has_explicit_access_traits<std::unique_ptr<T>>   : std::true_type {};
    template <class T> struct detail::has_explicit_access_traits<std::shared_ptr<T>>   : std::true_type {};
}

namespace h5::meta::detail_capabilities {
    template <class T> struct has_explicit_storage_repr<std::unique_ptr<T[]>> : std::true_type {};
    template <class T> struct has_explicit_storage_repr<std::shared_ptr<T[]>> : std::true_type {};
    template <class T> struct has_explicit_storage_repr<std::unique_ptr<T>>   : std::true_type {};
    template <class T> struct has_explicit_storage_repr<std::shared_ptr<T>>   : std::true_type {};

    template <class T> struct storage_representation_impl<std::unique_ptr<T[]>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    template <class T> struct storage_representation_impl<std::shared_ptr<T[]>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    template <class T> struct storage_representation_impl<std::unique_ptr<T>>
        : std::integral_constant<storage_representation_t, storage_representation_t::scalar> {};
    template <class T> struct storage_representation_impl<std::shared_ptr<T>>
        : std::integral_constant<storage_representation_t, storage_representation_t::scalar> {};
}

namespace h5::meta {
    template <class T> struct access_traits_t<std::unique_ptr<T[]>> {
        using element_t = T;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static auto data(const std::unique_ptr<T[]>& c) noexcept { return h5::impl::data(c); }
        static auto data(std::unique_ptr<T[]>& c)       noexcept { return h5::impl::data(c); }
        // size() returns the sentinel from impl::size; callers that consume
        // this (the by-ref read path) should use the forwarding overload below.
        static auto size(const std::unique_ptr<T[]>& c) noexcept { return h5::impl::size(c); }
        static std::size_t bytes(const std::unique_ptr<T[]>&) noexcept { return 0; }
    };
    template <class T> struct access_traits_t<std::shared_ptr<T[]>> {
        using element_t = T;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static auto data(const std::shared_ptr<T[]>& c) noexcept { return h5::impl::data(c); }
        static auto data(std::shared_ptr<T[]>& c)       noexcept { return h5::impl::data(c); }
        static auto size(const std::shared_ptr<T[]>& c) noexcept { return h5::impl::size(c); }
        static std::size_t bytes(const std::shared_ptr<T[]>&) noexcept { return 0; }
    };
    template <class T> struct access_traits_t<std::unique_ptr<T>> {
        using element_t = T;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static auto data(const std::unique_ptr<T>& c) noexcept { return h5::impl::data(c); }
        static auto data(std::unique_ptr<T>& c)       noexcept { return h5::impl::data(c); }
        static auto size(const std::unique_ptr<T>&)   noexcept { return std::array<size_t, 1>{1}; }
        static std::size_t bytes(const std::unique_ptr<T>&) noexcept { return sizeof(T); }
    };
    template <class T> struct access_traits_t<std::shared_ptr<T>> {
        using element_t = T;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static auto data(const std::shared_ptr<T>& c) noexcept { return h5::impl::data(c); }
        static auto data(std::shared_ptr<T>& c)       noexcept { return h5::impl::data(c); }
        static auto size(const std::shared_ptr<T>&)   noexcept { return std::array<size_t, 1>{1}; }
        static std::size_t bytes(const std::shared_ptr<T>&) noexcept { return sizeof(T); }
    };
}
