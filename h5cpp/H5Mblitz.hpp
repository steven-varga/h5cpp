/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

#if defined(BZ_ARRAY_H) || defined(H5CPP_USE_BLITZ)
namespace h5::blitz {
		template<class T, int N> using array = ::blitz::Array<T,N>;

		template <class Object, class T = typename impl::decay<Object>::type, int N = Object::rank_>
		using is_supported = std::bool_constant<std::is_same_v<Object,h5::blitz::array<T,N>>>;
}

namespace h5::meta {
		template <class T, int N> struct is_contiguous<h5::blitz::array<T,N>> : std::true_type {};

		// Register types so generic access_traits_t fallbacks don't create ambiguous partial specializations
		template <class T, int N> struct detail::has_explicit_access_traits<h5::blitz::array<T,N>> : std::true_type {};
}
// Explicit storage_representation so h5::awrite's static_assert(storage != unsupported)
// always passes for vendored blitz arrays.
namespace h5::meta::detail_capabilities {
		template <class T, int N> struct has_explicit_storage_repr<h5::blitz::array<T,N>> : std::true_type {};
		template <class T, int N> struct storage_representation_impl<h5::blitz::array<T,N>>
			: std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
}

namespace h5::impl {
	// 1.) object -> H5T_xxx
	template <class T, int N> struct detail::has_explicit_decay<h5::blitz::array<T,N>> : std::true_type {};
	template <class T, int N> struct decay<h5::blitz::array<T,N>>{ using type = T; };

	// get read access to datastore
	template <class Object, class T = typename impl::decay<Object>::type, int N = Object::rank_> inline
	std::enable_if_t< h5::blitz::is_supported<Object>::value,
	const T*> data(const Object& ref ){
			return ref.data();
	}
	// read write access
	template <class Object, class T = typename impl::decay<Object>::type, int N = Object::rank_> inline
	std::enable_if_t< h5::blitz::is_supported<Object>::value,
	T*> data( Object& ref ){
			return ref.data();
	}
	// rank
	template<class T, int N> struct rank<h5::blitz::array<T,N>> : public std::integral_constant<size_t,N>{};
	// determine rank and dimensions
	template <class T, int N>
	inline std::array<size_t,N> size( const h5::blitz::array<T,N>& ref ){
		std::array<size_t,N> dims;
		for(int i=0; i<N; ++i) dims[i] = ref.extent(i);
		return dims;
	}

	template <class T, int N> struct get<h5::blitz::array<T,N>> {
		static inline h5::blitz::array<T,N> ctor( std::array<size_t,N> dims ){
			::blitz::TinyVector<int,N> shape;
			for(int i=0; i<N; ++i) shape[i] = static_cast<int>(dims[i]);
			return h5::blitz::array<T,N>(shape);
	}};
}

namespace h5::meta {
		template <class T, int N> struct access_traits_t<h5::blitz::array<T,N>> {
			using element_t = T;
			static constexpr access_t kind = access_t::contiguous;
			static constexpr bool is_trivially_packable = true;
			static auto data(const h5::blitz::array<T,N>& c) noexcept { return h5::impl::data(c); }
			static auto data(h5::blitz::array<T,N>& c)       noexcept { return h5::impl::data(c); }
			static auto size(const h5::blitz::array<T,N>& c) noexcept { return h5::impl::size(c); }
			static std::size_t bytes(const h5::blitz::array<T,N>& c) noexcept {
				auto s = size(c); std::size_t n = 1;
				for (std::size_t i = 0; i < s.size(); ++i) n *= s[i];
				return n * sizeof(element_t);
			}
		};
}
#endif
