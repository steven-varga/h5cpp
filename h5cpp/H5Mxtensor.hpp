/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include <vector>

// xtensor never defines a single root macro like `XTENSOR_HPP`; it uses per-header
// guards (XTENSOR_XARRAY_HPP, XTENSOR_ARRAY_HPP, etc.). XTENSOR_ARRAY_HPP is
// defined by <xtensor/xarray.hpp>, which is the canonical entry point for users
// of xt::xarray<T> — the type the mapper specializations target.
#if defined(XTENSOR_ARRAY_HPP) || defined(XTENSOR_XARRAY_HPP) || defined(H5CPP_USE_XTENSOR)

namespace h5::impl {
	// 1.) object -> H5T_xxx
	template <class T> struct detail::has_explicit_decay<xt::xarray<T>> : std::true_type {};
	template <class T, size_t N> struct detail::has_explicit_decay<xt::xtensor<T,N>> : std::true_type {};
	template <class T> struct decay<xt::xarray<T>>{ using type = T; };
	template <class T, size_t N> struct decay<xt::xtensor<T, N>>{ using type = T; };

	// get read access to datastore
	template <class T> inline
	const T* data(const xt::xarray<T>& ref ){
			return ref.data();
	}
	template <class T> inline
	T* data(xt::xarray<T>& ref ){
			return ref.data();
	}
	template <class T, size_t N> inline
	const T* data(const xt::xtensor<T, N>& ref ){
			return ref.data();
	}
	template <class T, size_t N> inline
	T* data(xt::xtensor<T, N>& ref ){
			return ref.data();
	}

	// rank
	template<class T, size_t N> struct rank<xt::xtensor<T, N>> : public std::integral_constant<size_t, N>{};
	// xt::xarray has runtime rank; the compile-time value just needs to be > 0 so that
	// H5Aread's `if constexpr (impl::rank<T>::value > 0)` branch fires and triggers
	// `impl::get<xt::xarray<T>>::ctor(count)` — that ctor reads the actual rank from
	// the h5::count_t at runtime and calls xt::xarray::from_shape with all dimensions.
	template<class T> struct rank<xt::xarray<T>> : public std::integral_constant<size_t, 1>{};

	// determine rank and dimensions
	template <class T> inline h5::count_t size( const xt::xarray<T>& ref ){
		h5::count_t count;
		count.rank = static_cast<int>(ref.dimension());
		for(size_t i = 0; i < ref.dimension(); ++i)
			count[i] = ref.shape()[i];
		return count;
	}
	template <class T, size_t N> inline std::array<size_t,N> size( const xt::xtensor<T, N>& ref ){
		std::array<size_t,N> dims;
		for(size_t i = 0; i < N; ++i)
			dims[i] = ref.shape()[i];
		return dims;
	}

	// CTOR-s
	template <class T> struct get<xt::xarray<T>> {
		// Templated on the dims type so any h5::impl::array<TAG> (count_t,
		// current_dims_t, etc.) works without explicit conversion — they all
		// expose .rank and operator[].
		template <class Dims>
		static inline xt::xarray<T> ctor( const Dims& dims ){
			std::vector<size_t> shape(dims.rank);
			for(int i = 0; i < dims.rank; ++i)
				shape[i] = dims[i];
			return xt::xarray<T>::from_shape(shape);
	}};
	template <class T, size_t N> struct get<xt::xtensor<T, N>> {
		static inline xt::xtensor<T, N> ctor( std::array<size_t,N> dims ){
			return xt::xtensor<T, N>::from_shape(dims);
	}};
}

namespace h5::meta {
    template <class T> struct decay<xt::xarray<T>> : h5::impl::decay<xt::xarray<T>> {};
    template <class T, size_t N> struct decay<xt::xtensor<T, N>> : h5::impl::decay<xt::xtensor<T, N>> {};

    template <class T, size_t N> struct rank<xt::xtensor<T, N>> : h5::impl::rank<xt::xtensor<T, N>> {};

    template <class T> inline
    const T* data(const xt::xarray<T>& ref ){ return ref.data(); }
    template <class T> inline
    T* data(xt::xarray<T>& ref ){ return ref.data(); }
    template <class T, size_t N> inline
    const T* data(const xt::xtensor<T, N>& ref ){ return ref.data(); }
    template <class T, size_t N> inline
    T* data(xt::xtensor<T, N>& ref ){ return ref.data(); }

    template <class T> inline h5::count_t size( const xt::xarray<T>& ref ){
        return h5::impl::size( ref );
    }
    template <class T, size_t N> inline std::array<size_t,N> size( const xt::xtensor<T, N>& ref ){
        return h5::impl::size( ref );
    }

    template <class T> struct get<xt::xarray<T>> {
        // Same dims-templating as h5::impl::get<xt::xarray<T>>::ctor — accept any
        // impl::array tag variant.
        template <class Dims>
        static inline xt::xarray<T> ctor( const Dims& dims ){
            std::vector<size_t> shape(dims.rank);
            for(int i = 0; i < dims.rank; ++i)
                shape[i] = dims[i];
            return xt::xarray<T>::from_shape(shape);
    }};
    template <class T, size_t N> struct get<xt::xtensor<T, N>> : h5::impl::get<xt::xtensor<T, N>> {};

    template <class T> struct is_contiguous<xt::xarray<T>> : std::true_type {};
    template <class T, size_t N> struct is_contiguous<xt::xtensor<T, N>> : std::true_type {};

    // Explicit access_traits_t: the generic contiguous fallback reports size = {c.size()}
    // (rank-1 element count), which causes a 2-D xt::xarray to be written as a flat 1-D
    // attribute and read back as a 1-D xarray.  These specs report the actual runtime
    // shape via xt::shape(), preserving rank on round-trip.
    template <class T> struct detail::has_explicit_access_traits<xt::xarray<T>> : std::true_type {};
    template <class T, size_t N> struct detail::has_explicit_access_traits<xt::xtensor<T,N>> : std::true_type {};

    template <class T>
    struct access_traits_t<xt::xarray<T>> {
        using element_t = T;
        using pointer_t = const T*;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static const T* data(const xt::xarray<T>& c) noexcept { return c.data(); }
        static T*       data(xt::xarray<T>& c)       noexcept { return c.data(); }
        static h5::current_dims_t size(const xt::xarray<T>& c) noexcept {
            h5::current_dims_t dims;
            dims.rank = static_cast<int>(c.dimension());
            for (std::size_t i = 0; i < c.dimension(); ++i)
                dims[i] = c.shape()[i];
            return dims;
        }
        static std::size_t bytes(const xt::xarray<T>& c) noexcept { return c.size() * sizeof(T); }
    };
    template <class T, std::size_t N>
    struct access_traits_t<xt::xtensor<T,N>> {
        using element_t = T;
        using pointer_t = const T*;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static const T* data(const xt::xtensor<T,N>& c) noexcept { return c.data(); }
        static T*       data(xt::xtensor<T,N>& c)       noexcept { return c.data(); }
        static std::array<std::size_t, N> size(const xt::xtensor<T,N>& c) noexcept {
            std::array<std::size_t, N> dims;
            for (std::size_t i = 0; i < N; ++i) dims[i] = c.shape()[i];
            return dims;
        }
        static std::size_t bytes(const xt::xtensor<T,N>& c) noexcept { return c.size() * sizeof(T); }
    };
}
// Explicit storage_representation so h5::awrite's static_assert(storage != unsupported)
// always passes for vendored xtensor types.
namespace h5::meta::detail_capabilities {
    template <class T> struct has_explicit_storage_repr<xt::xarray<T>> : std::true_type {};
    template <class T, size_t N> struct has_explicit_storage_repr<xt::xtensor<T, N>> : std::true_type {};
    template <class T> struct storage_representation_impl<xt::xarray<T>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    template <class T, size_t N> struct storage_representation_impl<xt::xtensor<T, N>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
}

#endif
