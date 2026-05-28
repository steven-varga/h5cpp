/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#pragma once
#include <string>
#include <vector>
#include <array>
#include <forward_list>
#include <initializer_list>
#include <type_traits>
#include "H5meta.hpp"    // for h5::meta::has_size, has_data, is_sequential_like

namespace h5::impl {
/*STL: */
	// 2.) filter is_xxx_type
	// 4.) write access
	// 5.) obtain dimensions of extents
	// 6.) ctor with right dimensions

	// Helpers to exclude types that have explicit decay specialisations below.
	// Used by the structural fallback to avoid partial-specialisation ambiguity.
	namespace detail {
		template <class T> struct has_explicit_decay : std::false_type {};
		template <class C, class Tr, class A>
		struct has_explicit_decay<std::basic_string<C,Tr,A>> : std::true_type {};
		template <class T, class A>
		struct has_explicit_decay<std::vector<T,A>> : std::true_type {};
		template <class T, std::size_t N>
		struct has_explicit_decay<std::vector<std::array<T,N>>> : std::true_type {};
		template <class T>
		struct has_explicit_decay<std::initializer_list<T>> : std::true_type {};
	}

	// 1.) object -> H5T_xxx
	// Primary uses class=void so partial specialisations can use enable_if.
	template <class T, class = void> struct decay { using type = T; };

	template <class T> struct decay<const T>{ using type = T; };
	template <class T> struct decay<const T*>{ using type = T*; };
	template <class T> struct decay<std::basic_string<T>>{ using type = const T*; };
	template <class T, signed N> struct decay<const T[N]>{ using type = T*; };
	template <class T, signed N> struct decay<T[N]>{ using type = T*; };

	template <class T> struct decay<std::initializer_list<const T*>>{ using type = const T*; };
	template <class T> struct decay<std::initializer_list<T*>>{ using type = T*; };
	template <class T> struct decay<std::initializer_list<T>>{ using type = T; };

	template <class T> struct decay<std::vector<const T*>>{ using type = const T*; };
	template <class T> struct decay<std::vector<T*>>{ using type = T*; };
	template <class T> struct decay<std::vector<T>>{ using type = T; };
	// vector<array<T,N>>: flatten through the inner fixed extent so the I/O
	// pipeline treats the buffer as N*M contiguous T elements (matches the
	// dataset's effective extent). Without this specialisation `decay` stops
	// at array<T,N> and the dataset gets sized by vec.size() instead of
	// vec.size()*N.
	template <class T, std::size_t N>
	struct decay<std::vector<std::array<T,N>>>{ using type = T; };

	// Gap 2: structural fallback for containers not explicitly registered above.
	// The enable_if exclusion prevents ambiguity with the explicit specs for
	// vector, basic_string, and initializer_list (all have value_type too).
	// Linalg mapper specialisations (H5Marma, H5Meigen, …) are explicit and
	// must register in detail::has_explicit_decay if they carry value_type.
	// const/reference/pointer/array variants are handled by the explicit specs
	// above (decay<const T>, decay<const T*>, decay<T[N]>, etc.).
	template <class T>
	struct decay<T, std::enable_if_t<
		h5::meta::has_value_type<T>::value &&
		!detail::has_explicit_decay<T>::value &&
		!std::is_const_v<T> &&
		!std::is_reference_v<T> &&
		!std::is_array_v<T> &&
		!std::is_pointer_v<T>>>
	{ using type = typename T::value_type; };

	// helpers
	template <class T>
		using is_scalar = std::integral_constant<bool,
			std::is_integral<T>::value || (std::is_standard_layout_v<T> && std::is_trivial_v<T>) || std::is_same<T,std::string>::value>;
	template <class T, class B = typename impl::decay<T>::type>
		using is_rank01 = std::integral_constant<bool,
			std::is_same<T,std::initializer_list<B>>::value || 
			std::is_same<T,std::vector<B>>::value >;

	template<class T> struct rank : public std::integral_constant<size_t,0>{};
	template<> struct rank<std::string>: public std::integral_constant<size_t,1>{};
	template<class T> struct rank<T*>: public std::integral_constant<size_t,1>{};
	template<class T> struct rank<std::vector<T>>: public std::integral_constant<size_t,1>{};

	// 3.) read access
	template <class T> inline typename std::enable_if<std::is_integral<T>::value || (std::is_standard_layout_v<T> && std::is_trivial_v<T>),
		const T*>::type data( const T& ref ){ return &ref; }
	template<class T> inline typename std::enable_if< impl::is_scalar<T>::value,
		const T*>::type data( const std::initializer_list<T>& ref ){ return ref.begin(); }
	inline const char* const* data( const std::initializer_list<const char*>& ref ){ return ref.begin(); }
	inline const char* data( const std::string& ref ){ return ref.c_str(); }
	template <class T, class A> inline const T* data( const std::vector<T, A>& ref ){
		return ref.data();
	}
	template <class T, class A> inline T* data( std::vector<T, A>& ref ){
		return ref.data();
	}
	template <class T, std::size_t N> inline const T* data( const std::array<T,N>& ref ){
		return ref.data();
	}
	template <class T, std::size_t N> inline T* data( std::array<T,N>& ref ){
		return ref.data();
	}
	// vector<array<T,N>>: hand the buffer back as flat T* so the inner write/read
	// path operates on N*M contiguous elements (POD layout makes this aliasing safe).
	template <class T, std::size_t N, class A>
	inline const T* data( const std::vector<std::array<T,N>, A>& ref ){
		return ref.empty() ? nullptr : ref.front().data();
	}
	template <class T, std::size_t N, class A>
	inline T* data( std::vector<std::array<T,N>, A>& ref ){
		return ref.empty() ? nullptr : ref.front().data();
	}
	// std::array<T,N>: report size as {N} so the attribute/dataset is created
	// with rank-1 extent N rather than rank-0 scalar (which is_scalar<> implies
	// because std::array is standard_layout+trivial).
	template <class T, std::size_t N>
	inline std::array<size_t,1> size( const std::array<T,N>& ){
		return {N};
	}
	// 4.) write access
	template <class T> inline std::enable_if_t<std::is_integral_v<T>,
	T*> data( T& ref ){ return &ref; }
	// 5.) obtain dimensions of extents
	template <class T> inline constexpr std::enable_if_t< impl::is_scalar<T>::value,
		std::array<size_t,0>> size( T ){ return{}; }
	template <class T, class A> inline std::array<size_t,1> size( const std::vector<T, A>& ref ){ return {ref.size()}; }
	// vector<array<T,N>>: total flat extent so the dataset is sized N*M, not N.
	template <class T, std::size_t N, class A>
	inline std::array<size_t,1> size( const std::vector<std::array<T,N>, A>& ref ){
		return {ref.size() * N};
	}
	// Structural fallback for the read path. Deliberately NOT named `data` so it
	// doesn't join the impl::data overload set — linalg mappers (blitz, ublas,
	// eigen, …) each define their own `impl::data(Object& ref)` template gated
	// by a per-library is_supported<T>, and adding another generic template here
	// would cause overload ambiguity for those types. Called from H5Dread.hpp's
	// read fallback only when `impl::rank<T>::value == 0` (i.e., no by-name
	// spec). Returns `ref.data()` for any T that exposes a `.data()` member.
	template <class T>
	inline auto structural_data(T& ref) -> decltype(ref.data()) {
		return ref.data();
	}
	template <class T> inline std::array<size_t,1> size( const std::initializer_list<T>& ref ){ return {ref.size()}; }
	template <class T, class A>
	inline std::array<size_t,1> size( const std::forward_list<T,A>& ref ){
		return {static_cast<size_t>(std::distance(ref.begin(), ref.end()))};
	}
} // h5::impl

// forward_list lacks .size(), so rank<T> (which checks has_size<T>) would return 0,
// misclassifying it as scalar. Specialize explicitly to rank-1.
namespace h5::meta {
	template <class T, class A>
	struct rank<std::forward_list<T,A>> : public std::integral_constant<std::size_t, 1> {};
}

namespace h5::impl {
	// Gap 2: structural size() — containers with .size() not explicitly covered.
	// Returns rank-1 extent for any non-scalar with a .size() member.
	// The explicit vector<T,A> overload above is more specific and wins for vectors.
	template <class T>
	inline auto size(const T& ref)
		-> std::enable_if_t<
			!impl::is_scalar<T>::value &&
			h5::meta::has_size<T>::value,
			std::array<size_t,1>>
	{ return {ref.size()}; }
	// Rank-0 fallback for non-scalar types with no .size()
	template <class T>
	inline constexpr auto size(const T&)
		-> std::enable_if_t<
			!impl::is_scalar<T>::value &&
			!h5::meta::has_size<T>::value,
			std::array<size_t,0>>
	{ return {}; }
	// 6.) ctor with right dimensions
	template <class T> struct get {
	   	static inline T ctor( std::array<size_t,impl::rank<T>::value> ){
			return T(); }};
	template<class T>
	struct get<std::vector<T>> {
		static inline std::vector<T> ctor( std::array<size_t,1> dims ){
			return std::vector<T>( dims[0] );
	}};
	// vector<array<T,N>>: dims[0] is the flat extent (= vec.size()*N), so divide
	// to recover the outer count when constructing the readback buffer.
	template<class T, std::size_t N>
	struct get<std::vector<std::array<T,N>>> {
		static inline std::vector<std::array<T,N>> ctor( std::array<size_t,1> dims ){
			return std::vector<std::array<T,N>>( dims[0] / N );
	}};
}
