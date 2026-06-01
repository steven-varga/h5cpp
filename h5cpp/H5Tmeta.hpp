/*
 * Copyright (c) 2018 - 2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

#include "H5Iall.hpp"
#include "H5meta.hpp"
#include <type_traits>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

#include <tuple>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

// H5CPP_supported_elementary_types is defined once in H5misc.hpp (the
// canonical copy that includes `enum`); H5misc is always pulled in ahead of
// this header via h5cpp/core, and the macro is only consumed by the
// static_assert diagnostics in H5Dread/H5Dwrite. The duplicate definition
// that used to live here conflicted with H5misc's and produced a redefinition
// warning in every TU including core (#289).

// stl detection with templates, this probably should stay until concepts become mainstream
namespace h5::meta {
 
    template <class T, class... Ts> struct is_array : public std::is_array<T>{};
    template <class T, size_t N> struct is_array <std::array<T,N>> : std::true_type{};

    template<class T> using is_stl = /* will use concepts once become available */
        std::disjunction<meta::has_iterator<T>, meta::has_data<T>,meta::has_size<T>,meta::has_value_type<T>>;

    template <class... Args> using enable_or = std::enable_if<std::disjunction_v<Args...>>;
    template <class... Args> using enable_and = std::enable_if<std::conjunction_v<Args...>>;
    template <class T, class... Ts> struct decay {
        using type = typename meta::value<T>::type; };
    template <class T, size_t N> struct decay<T[N]>{ // support for array types
        using type = typename std::remove_all_extents<T>::type; };
    template <size_t N> struct decay<char[N]>{ using type = char*; };
    template <size_t N> struct decay<const char[N]>{ using type = char*; };
    template <class T, class... Ts> struct decay<std::basic_string<T, Ts...>>{ using type = const T*; };
    template <class T, class... Ts> struct decay<std::basic_string_view<T, Ts...>>{ using type = const T*; };

    /* std::array<...,R> size<T>(){} template has to compute rank R at compile 
     * time, these templates, and their respective specializations aid to accomplish that*/
    template<class T, class... Ts> struct rank : public std::integral_constant<int,
        meta::has_size<T>::value> {}; // definition
    template<class U> struct rank<U[]>  : public std::integral_constant<std::size_t, rank<U>::value + 1>{};
    template<class U, std::size_t N> struct rank<U[N]> : public std::integral_constant<std::size_t, rank<U>::value + 1>{};
    template<class U, std::size_t N> struct rank<U*[N]> : public std::integral_constant<std::size_t, rank<U*>::value>{};
    template <size_t N> struct rank<char[N]> : public std::integral_constant<int,0>{}; // character literals

    template<class T, int N, class... Ts> using is_rank = std::bool_constant<rank<T, Ts...>::value == N >;
    // helpers for is_rank<T>, don't need specialization, instead define 'rank'
    template<class T, class... Ts> using is_scalar = is_rank<T,0,Ts...>; // numerical | pod 
    template<class T, class... Ts> using is_vector = is_rank<T,1,Ts...>;
    template<class T, class... Ts> using is_matrix = is_rank<T,2,Ts...>;
    template<class T, class... Ts> using is_cube   = is_rank<T,3,Ts...>;

    template <class T, class D=typename meta::decay<T>::type>
    using is_string = typename std::bool_constant<std::is_same_v<T,std::basic_string<char>> || std::is_same_v<D, std::basic_string<char>> || 
        std::is_same_v<T,std::basic_string<wchar_t>> || std::is_same_v<D, std::basic_string<wchar_t>> || 
        std::is_same_v<T,std::basic_string<char16_t>> || std::is_same_v<D, std::basic_string<char16_t>> || 
        std::is_same_v<T,std::basic_string<char32_t>> || std::is_same_v<D, std::basic_string<char32_t>> ||
        std::is_same_v<T,std::basic_string_view<char>> || std::is_same_v<D, std::basic_string<char>> || 
        std::is_same_v<T,std::basic_string_view<wchar_t>> || std::is_same_v<D, std::basic_string_view<wchar_t>> || 
        std::is_same_v<T,std::basic_string_view<char16_t>> || std::is_same_v<D, std::basic_string_view<char16_t>> || 
        std::is_same_v<T,std::basic_string_view<char32_t>> || std::is_same_v<D, std::basic_string_view<char32_t>>>;


    /* Objects may reside in continuous memory region such as vectors, matrices, POD structures can be saved/loaded in a single transfer,
     * the rest needs to be handled on a member variable bases*/
    template <class T, class... Ts> struct is_contiguous : std::integral_constant<bool, (std::is_standard_layout_v<T> && std::is_trivial_v<T>)> {};
    template <class T, class... Ts> struct is_contiguous <std::basic_string<T,Ts...>> : std::true_type {};
    template <class T, class... Ts> struct is_contiguous <std::basic_string_view<T,Ts...>> : std::true_type {};
    template <size_t N> struct is_contiguous <const char*[N]> : std::false_type {};

    template <class T> struct is_contiguous <std::complex<T>> : std::true_type{};
    template <class... Ts> struct is_contiguous <std::vector<bool,Ts...>> : std::false_type {};
    template <class T, class... Ts> struct is_contiguous <std::vector<T,Ts...>> :
        std::integral_constant<bool, (std::is_standard_layout_v<T> && std::is_trivial_v<T>)>{};
    template <class T, size_t N> struct is_contiguous <std::array<T,N>> :
        std::integral_constant<bool, (std::is_standard_layout_v<T> && std::is_trivial_v<T>)>{};

    template <class T, class... Ts> struct is_linalg : std::false_type {};
    template <class C, class T, class... Cs> struct is_valid : std::false_type {};

    template <class T> using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

    template <class T> using key_type_f    = typename T::key_type;
    template <class T> using mapped_type_f = typename T::mapped_type;
    template <class T> using key_compare_f = typename T::key_compare;
    template <class T> using hasher_f      = typename T::hasher;
    template <class T> using resize_f      = decltype(std::declval<T&>().resize(std::declval<std::size_t>()));

    template <class T> struct is_fixed_text_like : std::false_type {};
    template <std::size_t N> struct is_fixed_text_like<char[N]>       : std::true_type {};
    template <std::size_t N> struct is_fixed_text_like<const char[N]> : std::true_type {};

    template <class T> struct is_vl_text_like : std::false_type {};
    template <> struct is_vl_text_like<char*>       : std::true_type {};
    template <> struct is_vl_text_like<const char*> : std::true_type {};
    template <class Tr> struct is_vl_text_like<std::basic_string_view<char, Tr>> : std::true_type {};
    template <class Tr, class A> struct is_vl_text_like<std::basic_string<char, Tr, A>> : std::true_type {};

    template <class T> struct is_text_like
        : std::bool_constant<is_fixed_text_like<remove_cvref_t<T>>::value
                          || is_vl_text_like<remove_cvref_t<T>>::value> {};

    namespace detail_capabilities {
        template <class T> struct is_array_like_impl : std::false_type {};
        template <class T, std::size_t N> struct is_array_like_impl<T[N]>            : std::true_type {};
        template <class T, std::size_t N> struct is_array_like_impl<std::array<T,N>> : std::true_type {};
    }
    template <class T> struct is_array_like : detail_capabilities::is_array_like_impl<remove_cvref_t<T>> {};
    template <class T> struct is_iterable : has_iterator<remove_cvref_t<T>> {};
    template <class T> struct is_resizable : compat::is_detected<resize_f, remove_cvref_t<T>> {};
    template <class T> struct is_sequential_like : std::bool_constant<is_iterable<T>::value
        && compat::is_detected<value_type_f,  remove_cvref_t<T>>::value && !compat::is_detected<key_type_f, remove_cvref_t<T>>::value
        && !compat::is_detected<mapped_type_f, remove_cvref_t<T>>::value && !is_text_like<T>::value> {};

    template <class T> struct is_associative_like : std::bool_constant<is_iterable<T>::value
        && compat::is_detected<key_type_f,    remove_cvref_t<T>>::value && compat::is_detected<key_compare_f, remove_cvref_t<T>>::value> {};
    template <class T> struct is_unordered_like : std::bool_constant<is_iterable<T>::value
        && compat::is_detected<key_type_f, remove_cvref_t<T>>::value && compat::is_detected<hasher_f,   remove_cvref_t<T>>::value> {};

    template <class T> struct is_set_like  : std::bool_constant<compat::is_detected<key_type_f,    remove_cvref_t<T>>::value
        && compat::is_detected<value_type_f,  remove_cvref_t<T>>::value  && !compat::is_detected<mapped_type_f, remove_cvref_t<T>>::value> {};

    template <class T> struct is_map_like : std::bool_constant<compat::is_detected<key_type_f,    remove_cvref_t<T>>::value
        && compat::is_detected<mapped_type_f, remove_cvref_t<T>>::value && compat::is_detected<value_type_f,  remove_cvref_t<T>>::value> {};

    template <class T> struct is_stl_like : std::bool_constant<is_sequential_like<T>::value
        || is_associative_like<T>::value || is_unordered_like<T>::value> {};

    template <class T> struct is_enumerated_like : std::is_enum<remove_cvref_t<T>> {};
    template <class T> struct is_bitfield_like : std::false_type {};
    template <class A> struct is_bitfield_like<std::vector<bool, A>> : std::true_type {};

    template <class T> struct is_opaque_like : std::false_type {};
    template <> struct is_opaque_like<void*> : std::true_type {};
    template <> struct is_opaque_like<const void*> : std::true_type {};
    template <> struct is_opaque_like<void**> : std::true_type {};
    template <> struct is_opaque_like<const void**> : std::true_type {};

    template <class T> struct has_data_pointer : std::bool_constant<std::is_pointer_v<
        compat::detected_or_t<void, data_f, remove_cvref_t<T>>>> {};

    // Marker trait set to std::true_type by H5CPP_REGISTER_TYPE_ and
    // H5CPP_REGISTER_STRUCT macros (see H5Tall.hpp). Gates the aggregate
    // storage_representation_impl fallback so unregistered POD aggregates fall
    // through to 'unsupported' and are rejected at compile time by the static_assert
    // stoppers in H5Dwrite/H5Dread/H5Awrite/H5Aread. Preserves review item A7.
    template <class T> struct has_registered_compound : std::false_type {};

    enum class storage_representation_t {
        unsupported, scalar, c_array, linear_value_dataset, key_value_dataset, ragged_vlen_dataset, fixed_inner_extent_dataset, vlen_text_dataset, fixed_length_string,
        // Canonical fixed-extent mapping (Winston model):
        //   array_element   — top-level T[N] / std::array<T,N> (non-char) → scalar dataspace + H5T_ARRAY[N] dt_t<T>
        //   array_dataset   — vector/list/set/etc.<std::array<T,N>> (non-char) → rank-1 dataspace of H5T_ARRAY[N] elements
        //   fls_dataset     — vector/list/set/etc.<std::array<char,N>> → rank-1 dataspace of H5T_C_S1+H5Tset_size(N) elements
        array_element, array_dataset, fls_dataset };
        
    namespace detail_capabilities {

    // Marker for types that have an explicit storage_representation_impl specialisation.
    // The structural fallbacks (Gap 4) check this to avoid partial-specialisation
    // ambiguity with the well-known STL container specs below.
    // Third-party / user-defined containers should NOT specialise this — they rely
    // on the structural fallback firing automatically.
    template <class T> struct has_explicit_storage_repr : std::false_type {};
    template <class T, class A>
    struct has_explicit_storage_repr<std::vector<T,A>>           : std::true_type {};
    template <class A>
    struct has_explicit_storage_repr<std::vector<bool,A>>        : std::true_type {};
    template <class T, std::size_t N>
    struct has_explicit_storage_repr<std::array<T,N>>            : std::true_type {};
    template <class T, class A>
    struct has_explicit_storage_repr<std::deque<T,A>>            : std::true_type {};
    template <class T, class A>
    struct has_explicit_storage_repr<std::list<T,A>>             : std::true_type {};
    template <class T, class A>
    struct has_explicit_storage_repr<std::forward_list<T,A>>     : std::true_type {};
    template <class T, class C, class A>
    struct has_explicit_storage_repr<std::set<T,C,A>>            : std::true_type {};
    template <class T, class C, class A>
    struct has_explicit_storage_repr<std::multiset<T,C,A>>       : std::true_type {};
    template <class T, class H, class E, class A>
    struct has_explicit_storage_repr<std::unordered_set<T,H,E,A>>        : std::true_type {};
    template <class T, class H, class E, class A>
    struct has_explicit_storage_repr<std::unordered_multiset<T,H,E,A>>   : std::true_type {};
    template <class K, class V, class C, class A>
    struct has_explicit_storage_repr<std::map<K,V,C,A>>          : std::true_type {};
    template <class K, class V, class C, class A>
    struct has_explicit_storage_repr<std::multimap<K,V,C,A>>     : std::true_type {};
    template <class K, class V, class H, class E, class A>
    struct has_explicit_storage_repr<std::unordered_map<K,V,H,E,A>>      : std::true_type {};
    template <class K, class V, class H, class E, class A>
    struct has_explicit_storage_repr<std::unordered_multimap<K,V,H,E,A>> : std::true_type {};

    template <class T, class = void> struct storage_representation_impl
        : std::integral_constant<storage_representation_t, storage_representation_t::unsupported> {};

    template <class... Ts> struct has_explicit_storage_repr<std::tuple<Ts...>> : std::true_type {};
    template <class K, class V> struct has_explicit_storage_repr<std::pair<K,V>> : std::true_type {};
    // W4: explicit storage reps for scalar text / complex — registered so the
    // structural fallbacks (sequential_like / map_like / aggregate) don't shadow them.
    template <class Tr, class A>
    struct has_explicit_storage_repr<std::basic_string<char, Tr, A>>      : std::true_type {};
    template <class Tr>
    struct has_explicit_storage_repr<std::basic_string_view<char, Tr>>    : std::true_type {};
    template <> struct has_explicit_storage_repr<char*>                   : std::true_type {};
    template <> struct has_explicit_storage_repr<const char*>             : std::true_type {};
    template <class T> struct has_explicit_storage_repr<std::complex<T>>  : std::true_type {};
    template <class... Ts> struct storage_representation_impl<std::tuple<Ts...>>
        : std::integral_constant<storage_representation_t, storage_representation_t::scalar> {};
    template <class K, class V> struct storage_representation_impl<std::pair<K,V>>
        : std::integral_constant<storage_representation_t, storage_representation_t::scalar> {};

    // arithmetic and enum scalars
    template <class T> struct storage_representation_impl<T,
        typename std::enable_if<std::is_arithmetic<T>::value || std::is_enum<T>::value>::type>
        : std::integral_constant<storage_representation_t, storage_representation_t::scalar> {};

    // W4: scalar text — std::basic_string<char,...> standalone (the vector<string>
    // case is already covered below). HDF5 variable-length string maps cleanly.
    template <class Tr, class A>
    struct storage_representation_impl<std::basic_string<char, Tr, A>>
        : std::integral_constant<storage_representation_t, storage_representation_t::vlen_text_dataset> {};
    template <class Tr>
    struct storage_representation_impl<std::basic_string_view<char, Tr>>
        : std::integral_constant<storage_representation_t, storage_representation_t::vlen_text_dataset> {};

    // W4: raw C-string pointers — same vlen_text path as std::string.
    template <> struct storage_representation_impl<char*>
        : std::integral_constant<storage_representation_t, storage_representation_t::vlen_text_dataset> {};
    template <> struct storage_representation_impl<const char*>
        : std::integral_constant<storage_representation_t, storage_representation_t::vlen_text_dataset> {};

    // W4: std::complex<T> (floating-point T) — scalar value with two HDF5 fields.
    // Routed via dt_t<complex<T>> in H5Tall.hpp (native H5T_COMPLEX or compound fallback).
    template <class T>
    struct storage_representation_impl<std::complex<T>,
        std::enable_if_t<std::is_floating_point<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::scalar> {};

    // char[N] is treated as a fixed-length HDF5 string at the top-level write/
    // read boundary — H5Tcopy(H5T_C_S1) + H5Tset_size(N), scalar dataspace.
    // Other T[N] (e.g., int[10], double[4]) keep the c_array representation.
    // Compound-internal char[N] fields (the compiler-emitted path) are
    // unaffected — those go through H5Tinsert with an explicit array type.
    template <std::size_t N> struct storage_representation_impl<char[N]>
        : std::integral_constant<storage_representation_t, storage_representation_t::fixed_length_string> {};

    // Canonical mapping (Winston model): top-level T[N] / std::array<T,N> for
    // non-char T lands as a scalar dataspace with an H5T_ARRAY[N] element
    // type. This is the `array_element` storage.  char[N] (above) and
    // std::array<char,N> (below) take the fixed_length_string path instead.
    template <class T, std::size_t N>
    struct storage_representation_impl<T[N], std::enable_if_t<
        !std::is_same_v<std::remove_cv_t<T>, char>
        && !std::is_array_v<T>>>
        : std::integral_constant<storage_representation_t, storage_representation_t::array_element> {};
    template <class T, std::size_t N, std::size_t M> struct storage_representation_impl<T[N][M]>
        : std::integral_constant<storage_representation_t, storage_representation_t::array_element> {};
    template <class T, std::size_t N, std::size_t M, std::size_t P> struct storage_representation_impl<T[N][M][P]>
        : std::integral_constant<storage_representation_t, storage_representation_t::array_element> {};
    template <class T, std::size_t N, std::size_t M, std::size_t P, std::size_t Q>
    struct storage_representation_impl<T[N][M][P][Q]>
        : std::integral_constant<storage_representation_t, storage_representation_t::array_element> {};
    template <class T, std::size_t N, std::size_t M, std::size_t P, std::size_t Q, std::size_t R>
    struct storage_representation_impl<T[N][M][P][Q][R]>
        : std::integral_constant<storage_representation_t, storage_representation_t::array_element> {};
    template <class T, std::size_t N, std::size_t M, std::size_t P, std::size_t Q, std::size_t R, std::size_t S>
    struct storage_representation_impl<T[N][M][P][Q][R][S]>
        : std::integral_constant<storage_representation_t, storage_representation_t::array_element> {};
    template <class T, std::size_t N, std::size_t M, std::size_t P, std::size_t Q, std::size_t R, std::size_t S, std::size_t U>
    struct storage_representation_impl<T[N][M][P][Q][R][S][U]>
        : std::integral_constant<storage_representation_t, storage_representation_t::array_element> {};

    // contiguous sequence containers — generic vector<T> and array<T,N>
    // more-specific specializations (vector<vector<T>>, vector<string>, vector<array<T,N>>) take priority
    // std::vector<bool> is a bit-packing specialization with no contiguous bool* — must be unsupported
    template <class A> struct storage_representation_impl<std::vector<bool,A>>
        : std::integral_constant<storage_representation_t, storage_representation_t::unsupported> {};
    // Primary vector spec — flat element type with linear-value storage.
    // The enable_if excludes inner types handled by the more specific specs
    // below (vector<vector>, vector<string>, vector<array>, vector<list>,
    // vector<set>, etc.) — these would otherwise create ambiguous template
    // instantiations because C++ partial ordering doesn't compare enable_if.
    template <class T, class A>
    struct storage_representation_impl<std::vector<T,A>, std::enable_if_t<
        !is_sequential_like<T>::value &&
        !is_set_like<T>::value &&
        !is_associative_like<T>::value &&
        !is_text_like<T>::value &&
        !is_array_like<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    // std::array<char,N> → fixed_length_string (same path as char[N]).
    template <std::size_t N>
    struct storage_representation_impl<std::array<char,N>>
        : std::integral_constant<storage_representation_t, storage_representation_t::fixed_length_string> {};

    // std::array<T,N> for non-char T → array_element (scalar + H5T_ARRAY).
    // Excludes inner stl-like/text-like/bitfield types so they go through
    // the structural fallback / unsupported as appropriate.
    template <class T, std::size_t N>
    struct storage_representation_impl<std::array<T,N>,
        std::enable_if_t<
            !std::is_same_v<std::remove_cv_t<T>, char> &&
            !is_stl_like<T>::value &&
            !is_text_like<T>::value &&
            !is_bitfield_like<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::array_element> {};

    // std::array<std::array<T,N>, M> — nested fixed extent. Routes through
    // array_element with a two-level H5T_ARRAY type (composition is handled
    // at the dt_t<> layer in H5Tall.hpp).
    template <class T, std::size_t N, std::size_t M>
    struct storage_representation_impl<std::array<std::array<T,N>, M>>
        : std::integral_constant<storage_representation_t, storage_representation_t::array_element> {};

    template <class T, class A>
    struct storage_representation_impl<std::deque<T,A>, std::enable_if_t<!is_array_like<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    template <class T, class A>
    struct storage_representation_impl<std::list<T,A>, std::enable_if_t<!is_array_like<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    template <class T, class A>
    struct storage_representation_impl<std::forward_list<T,A>, std::enable_if_t<!is_array_like<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    template <class T, class C, class A>
    struct storage_representation_impl<std::set<T,C,A>, std::enable_if_t<!is_array_like<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    template <class T, class C, class A>
    struct storage_representation_impl<std::multiset<T,C,A>, std::enable_if_t<!is_array_like<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    template <class T, class H, class E, class A>
    struct storage_representation_impl<std::unordered_set<T,H,E,A>, std::enable_if_t<!is_array_like<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    template <class T, class H, class E, class A>
    struct storage_representation_impl<std::unordered_multiset<T,H,E,A>, std::enable_if_t<!is_array_like<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};

    template <class K, class V, class C, class A> struct storage_representation_impl<std::map<K,V,C,A>>
        : std::integral_constant<storage_representation_t, storage_representation_t::key_value_dataset> {};
    template <class K, class V, class C, class A> struct storage_representation_impl<std::multimap<K,V,C,A>>
        : std::integral_constant<storage_representation_t, storage_representation_t::key_value_dataset> {};
    template <class K, class V, class H, class E, class A> struct storage_representation_impl<std::unordered_map<K,V,H,E,A>>
        : std::integral_constant<storage_representation_t, storage_representation_t::key_value_dataset> {};
    template <class K, class V, class H, class E, class A> struct storage_representation_impl<std::unordered_multimap<K,V,H,E,A>>
        : std::integral_constant<storage_representation_t, storage_representation_t::key_value_dataset> {};

    template <class T, class A0, class A1> struct storage_representation_impl<std::vector<std::vector<T,A0>,A1>>
        : std::integral_constant<storage_representation_t, (!is_text_like<T>::value && !is_stl_like<T>::value)
                  ? storage_representation_t::ragged_vlen_dataset : storage_representation_t::unsupported> {};
    template <class Tr, class A0, class A1> struct storage_representation_impl<std::vector<std::basic_string<char, Tr, A0>,A1>>
        : std::integral_constant<storage_representation_t, storage_representation_t::vlen_text_dataset> {};
    // vector<std::array<char,N>> → fls_dataset (rank-1 of fixed-length-string elements).
    template <std::size_t N, class A>
    struct storage_representation_impl<std::vector<std::array<char,N>,A>>
        : std::integral_constant<storage_representation_t, storage_representation_t::fls_dataset> {};

    // vector<std::array<T,N>> for non-char T → array_dataset (rank-1 of H5T_ARRAY elements).
    template <class T, std::size_t N, class A>
    struct storage_representation_impl<std::vector<std::array<T,N>,A>, std::enable_if_t<
        !std::is_same_v<std::remove_cv_t<T>, char>>>
        : std::integral_constant<storage_representation_t, storage_representation_t::array_dataset> {};

    // Generic: non-vector, non-array iterable Outer<std::array<T,N>>
    // (list, deque, set, forward_list, etc.) — routes to array_dataset
    // (non-char T) or fls_dataset (char T). Excludes std::vector (has
    // dedicated specs above) and std::array (handled by the array_element
    // nested spec for compile-time fixed outer extents).
    template <class Outer>
    struct storage_representation_impl<Outer, std::enable_if_t<
        (is_sequential_like<Outer>::value || is_set_like<Outer>::value) &&
        !std::is_same_v<Outer, std::vector<typename Outer::value_type,
            std::allocator<typename Outer::value_type>>> &&
        !is_array_like<Outer>::value &&
        is_array_like<typename Outer::value_type>::value &&
        !std::is_same_v<typename Outer::value_type::value_type, char>>>
        : std::integral_constant<storage_representation_t, storage_representation_t::array_dataset> {};

    template <class Outer>
    struct storage_representation_impl<Outer, std::enable_if_t<
        (is_sequential_like<Outer>::value || is_set_like<Outer>::value) &&
        !std::is_same_v<Outer, std::vector<typename Outer::value_type,
            std::allocator<typename Outer::value_type>>> &&
        !is_array_like<Outer>::value &&
        is_array_like<typename Outer::value_type>::value &&
        std::is_same_v<typename Outer::value_type::value_type, char>>>
        : std::integral_constant<storage_representation_t, storage_representation_t::fls_dataset> {};

    // vector<L> for iterable L (list / deque / forward_list / set / multiset /
    // unordered_set / unordered_multiset). Each outer element is a variable-
    // length inner sequence; serialise as an hvl_t<value_type> at write time
    // and walk back via iterator-collect at read time. The inner value_type
    // must be flat (not itself stl-like / text-like) — same gate as the
    // vector<vector<T>> spec above.
    // vector<L> for ITERABLE L (list / deque / forward_list / set / multiset /
    // unordered_set / unordered_multiset). Excludes the cases already handled
    // explicitly above: vector<vector<T>>, vector<string>, vector<array<T,N>>.
    // Inner value_type must be flat (not stl-like / text-like).
    template <class L, class A>
    struct storage_representation_impl<std::vector<L, A>, std::enable_if_t<
        (is_sequential_like<L>::value || is_set_like<L>::value) &&
        !std::is_same_v<L, std::vector<typename L::value_type,
            std::allocator<typename L::value_type>>> &&   // vector<vector<T>>
        !is_array_like<L>::value &&                       // vector<array<T,N>>
        !is_text_like<L>::value &&                        // vector<string>
        compat::is_detected<value_type_f, L>::value &&
        !is_text_like<typename L::value_type>::value &&
        !is_stl_like<typename L::value_type>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::ragged_vlen_dataset> {};

    // Gap 4: structural fallbacks for third-party / unregistered containers.
    // has_explicit_storage_repr<T> guards against ambiguity with the STL explicit
    // specs above; any type NOT in that set reaches these fallbacks automatically.
    // Sequential-like (has begin/end/value_type, no key_type/mapped_type, not text/bitfield):
    template <class T>
    struct storage_representation_impl<T, std::enable_if_t<
        is_sequential_like<T>::value &&
        !is_text_like<T>::value &&
        !is_bitfield_like<T>::value &&
        !has_explicit_storage_repr<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};
    // Map-like (has key_type + mapped_type):
    template <class T>
    struct storage_representation_impl<T, std::enable_if_t<
        is_map_like<T>::value &&
        !has_explicit_storage_repr<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::key_value_dataset> {};
    // Set-like (has key_type + value_type, no mapped_type): flat value dataset.
    // Mirrors the sequential-like fallback — std::set sits in this slot via its
    // named partial spec; this fallback opens the door for custom set-shaped
    // containers (tiny::set, abseil::flat_hash_set, etc.).
    template <class T>
    struct storage_representation_impl<T, std::enable_if_t<
        is_set_like<T>::value &&
        !is_map_like<T>::value &&
        !is_text_like<T>::value &&
        !has_explicit_storage_repr<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::linear_value_dataset> {};

    // W4: aggregate / registered-compound fallback. Gated on has_registered_compound<T>
    // so unregistered POD aggregates fall through to 'unsupported' and are caught by
    // the static_assert stoppers (review item A7). Excludes arithmetic / enum / array /
    // text / iterable types so explicit specs above always win.
    template <class T>
    struct storage_representation_impl<T, std::enable_if_t<
        has_registered_compound<T>::value &&
        !std::is_arithmetic_v<T> &&
        !std::is_enum_v<T> &&
        !is_array_like<T>::value &&
        !is_text_like<T>::value &&
        !is_iterable<T>::value &&
        !has_explicit_storage_repr<T>::value>>
        : std::integral_constant<storage_representation_t, storage_representation_t::scalar> {};
    }

    template <class T> struct storage_representation : detail_capabilities::storage_representation_impl<remove_cvref_t<T>> {};
    template <class T> constexpr storage_representation_t storage_representation_v = storage_representation<T>::value;

    template <class T> struct is_tuple : std::false_type {};
    template <class... Ts> struct is_tuple<std::tuple<Ts...>> : std::true_type {};
    template <class T> inline constexpr bool is_tuple_v = is_tuple<remove_cvref_t<T>>::value;

    template <class T, class = void>
    struct storage_traits_impl_t;
    template <class T, class = void>
    struct is_transport_contiguous_impl_t;

    template <class T>
    using storage_traits_t = storage_traits_impl_t<remove_cvref_t<T>>;
    template <class T>
    struct is_transport_contiguous_t : is_transport_contiguous_impl_t<remove_cvref_t<T>> {};
    template <class T>
    inline constexpr bool is_transport_contiguous_v = is_transport_contiguous_t<T>::value;

    template <class T, class>
    struct storage_traits_impl_t {
        static constexpr bool supported   = false;
        static constexpr bool owns_handle = false;
    };

    template <class T>
    struct storage_traits_impl_t<T, std::enable_if_t<std::is_arithmetic_v<T>>> {
        static constexpr bool supported   = true;
        static constexpr bool owns_handle = false;
        static hid_t create_type() noexcept {
            if constexpr      (std::is_same_v<T, bool>)               return H5T_NATIVE_HBOOL;
            else if constexpr (std::is_same_v<T, char>)               return H5T_NATIVE_CHAR;
            else if constexpr (std::is_same_v<T, unsigned char>)      return H5T_NATIVE_UCHAR;
            else if constexpr (std::is_same_v<T, short>)              return H5T_NATIVE_SHORT;
            else if constexpr (std::is_same_v<T, unsigned short>)     return H5T_NATIVE_USHORT;
            else if constexpr (std::is_same_v<T, int>)                return H5T_NATIVE_INT;
            else if constexpr (std::is_same_v<T, unsigned int>)       return H5T_NATIVE_UINT;
            else if constexpr (std::is_same_v<T, long>)               return H5T_NATIVE_LONG;
            else if constexpr (std::is_same_v<T, unsigned long>)      return H5T_NATIVE_ULONG;
            else if constexpr (std::is_same_v<T, long long>)          return H5T_NATIVE_LLONG;
            else if constexpr (std::is_same_v<T, unsigned long long>) return H5T_NATIVE_ULLONG;
            else if constexpr (std::is_same_v<T, float>)              return H5T_NATIVE_FLOAT;
            else if constexpr (std::is_same_v<T, double>)             return H5T_NATIVE_DOUBLE;
            else if constexpr (std::is_same_v<T, long double>)        return H5T_NATIVE_LDOUBLE;
            else return H5I_INVALID_HID;
        }
    };

    template <class T>
    struct storage_traits_impl_t<T, std::enable_if_t<is_vl_text_like<T>::value>> {
        static constexpr bool supported   = true;
        static constexpr bool owns_handle = true;
        static hid_t create_type() noexcept {
            hid_t dt = H5Tcopy(H5T_C_S1);
            H5Tset_size(dt, H5T_VARIABLE);
            H5Tset_cset(dt, H5T_CSET_UTF8);
            return dt;
        }
    };

    template <class T>
    struct storage_traits_impl_t<T, std::enable_if_t<is_fixed_text_like<T>::value>> {
        static constexpr bool supported   = true;
        static constexpr bool owns_handle = true;
        static hid_t create_type() noexcept {
            hid_t dt = H5Tcopy(H5T_C_S1);
            H5Tset_size(dt, sizeof(T));
            return dt;
        }
    };

    template <class T>
    struct storage_traits_impl_t<T, std::enable_if_t<
        is_array_like<T>::value && !is_text_like<T>::value>> {
        using elem_t = typename meta::decay<T>::type;
        static constexpr bool supported   = storage_traits_t<elem_t>::supported;
        static constexpr bool owns_handle = true;
        static hid_t create_type() noexcept {
            if constexpr (std::is_array_v<T>)
                return make_c_array(std::make_index_sequence<std::rank_v<T>>{});
            else
                return make_std_array();
        }
    private:
        template <std::size_t... Is>
        static hid_t make_c_array(std::index_sequence<Is...>) noexcept {
            using scalar_t = std::remove_all_extents_t<T>;
            hid_t base = storage_traits_t<scalar_t>::create_type();
            hsize_t dims[] = { static_cast<hsize_t>(std::extent_v<T, Is>)... };
            hid_t dt = H5Tarray_create2(base, sizeof...(Is), dims);
            if constexpr (storage_traits_t<scalar_t>::owns_handle) H5Tclose(base);
            return dt;
        }
        static hid_t make_std_array() noexcept {
            hid_t base = storage_traits_t<elem_t>::create_type();
            hsize_t dims[] = { static_cast<hsize_t>(std::tuple_size_v<T>) };
            hid_t dt = H5Tarray_create2(base, 1, dims);
            if constexpr (storage_traits_t<elem_t>::owns_handle) H5Tclose(base);
            return dt;
        }
    };

    template <class T, class> struct is_transport_contiguous_impl_t : std::false_type {};
    template <class T> struct is_transport_contiguous_impl_t<T, std::enable_if_t<std::is_arithmetic_v<T>>> : std::true_type {};
    template <class T> struct is_transport_contiguous_impl_t<T, std::enable_if_t<is_fixed_text_like<T>::value>> : std::true_type {};
    template <class T> struct is_transport_contiguous_impl_t<T, std::enable_if_t<
        is_array_like<T>::value && !is_text_like<T>::value &&
        !is_stl_like<typename meta::decay<T>::type>::value>>
        : is_transport_contiguous_t<typename meta::decay<T>::type> {};

    // Trivially copyable aggregates are safe for bulk memcpy: no padding surprises,
    // no non-trivial copy semantics.  Excludes arrays and text already handled above.
    template <class T> struct is_transport_contiguous_impl_t<T, std::enable_if_t<
        std::is_aggregate_v<T> &&
        !is_array_like<T>::value &&
        !is_text_like<T>::value &&
        std::is_trivially_copyable_v<T>>> : std::true_type {};

    // Gap 1: contiguous STL sequence containers (vector<T>, span<T>, linalg types, etc.)
    // Triggers when T exposes a data() pointer and size(), but is not a C/std::array,
    // not text, and not arithmetic.
    // The element type inferred from data() must be standard-layout and trivial
    // (prevents nested containers like vector<vector<T>> or vector<string> from matching).
    template <class T>
    struct is_transport_contiguous_impl_t<T, std::enable_if_t<
        !is_array_like<T>::value &&
        !is_text_like<T>::value &&
        !std::is_arithmetic_v<T> &&
        has_data_pointer<T>::value &&
        meta::has_size<T>::value &&
        std::is_standard_layout_v<
            std::remove_pointer_t<
                compat::detected_or_t<void, data_f, remove_cvref_t<T>>>> &&
        std::is_trivial_v<
            std::remove_pointer_t<
                compat::detected_or_t<void, data_f, remove_cvref_t<T>>>>>>
        : is_transport_contiguous_t<
            std::remove_pointer_t<
                compat::detected_or_t<void, data_f, remove_cvref_t<T>>>> {};

    namespace detail {
        // Flat-buffer layout for std::tuple<Ts...>.
        // std::tuple is not standard-layout, so staging buffers use this custom
        // C-style layout (each field at its natural alignment, in declaration order).
        template <class Tuple, std::size_t I, class = void> struct tuple_field_end;
        template <class Tuple>
        struct tuple_field_end<Tuple, 0, void> {
            static constexpr std::size_t value = sizeof(std::tuple_element_t<0, Tuple>);
        };
        template <class Tuple, std::size_t I>
        struct tuple_field_end<Tuple, I, std::enable_if_t<(I > 0)>> {
            using elem_t = std::tuple_element_t<I, Tuple>;
            static constexpr std::size_t prev = tuple_field_end<Tuple, I-1>::value;
            static constexpr std::size_t value = ((prev + alignof(elem_t) - 1) & ~(alignof(elem_t) - 1)) + sizeof(elem_t);
        };

        template <class Tuple, std::size_t I, class = void> struct tuple_field_offset;
        template <class Tuple>
        struct tuple_field_offset<Tuple, 0, void> {
            static constexpr std::size_t value = 0;
        };
        template <class Tuple, std::size_t I>
        struct tuple_field_offset<Tuple, I, std::enable_if_t<(I > 0)>> {
            using elem_t = std::tuple_element_t<I, Tuple>;
            static constexpr std::size_t value =
                (tuple_field_end<Tuple, I-1>::value + alignof(elem_t) - 1) & ~(alignof(elem_t) - 1);
        };

        template <class... Ts>
        struct tuple_layout {
            using tuple_t = std::tuple<Ts...>;
            static constexpr std::size_t count = sizeof...(Ts);

            template <std::size_t I>
            static constexpr std::size_t offset() noexcept {
                return tuple_field_offset<tuple_t, I>::value;
            }
            template <std::size_t... Is>
            static constexpr std::size_t max_align_impl(std::index_sequence<Is...>) noexcept {
                std::size_t r = 1;
                ((r = r < alignof(std::tuple_element_t<Is, tuple_t>) ? alignof(std::tuple_element_t<Is, tuple_t>) : r), ...);
                return r;
            }
            static constexpr std::size_t max_alignment() noexcept {
                return max_align_impl(std::make_index_sequence<count>{});
            }
            static constexpr std::size_t total_size() noexcept {
                constexpr std::size_t end = tuple_field_end<tuple_t, count - 1>::value;
                constexpr std::size_t ma  = max_align_impl(std::make_index_sequence<count>{});
                return (end + ma - 1) & ~(ma - 1);
            }
        };

        // Convenience wrapper: unwrap std::tuple<Ts...> → tuple_layout<Ts...>
        template <class Tuple> struct tuple_layout_t;
        template <class... Ts>
        struct tuple_layout_t<std::tuple<Ts...>> {
            static constexpr std::size_t total_size()    noexcept { return tuple_layout<Ts...>::total_size(); }
            static constexpr std::size_t max_alignment() noexcept { return tuple_layout<Ts...>::max_alignment(); }
            template <std::size_t I>
            static constexpr std::size_t offset()        noexcept { return tuple_layout<Ts...>::template offset<I>(); }
            static void to_buffer(const std::tuple<Ts...>& t, char* buf) noexcept {
                tuple_to_buffer_impl(t, buf, std::make_index_sequence<sizeof...(Ts)>{});
            }
            static void from_buffer(std::tuple<Ts...>& t, const char* buf) noexcept {
                buffer_to_tuple_impl(t, buf, std::make_index_sequence<sizeof...(Ts)>{});
            }
        private:
            template <std::size_t... Is>
            static void tuple_to_buffer_impl(const std::tuple<Ts...>& t, char* buf, std::index_sequence<Is...>) noexcept {
                (std::memcpy(buf + tuple_layout<Ts...>::template offset<Is>(),
                             static_cast<const void*>(&std::get<Is>(t)),
                             sizeof(std::tuple_element_t<Is, std::tuple<Ts...>>)), ...);
            }
            template <std::size_t... Is>
            static void buffer_to_tuple_impl(std::tuple<Ts...>& t, const char* buf, std::index_sequence<Is...>) noexcept {
                (std::memcpy(static_cast<void*>(&std::get<Is>(t)),
                             buf + tuple_layout<Ts...>::template offset<Is>(),
                             sizeof(std::tuple_element_t<Is, std::tuple<Ts...>>)), ...);
            }
        };
    } // namespace detail

    // DEFAULT CASE
    template <class T> struct rank<T*>: public std::integral_constant<size_t,1>{};
    template <class T, class... Ts>
        std::enable_if_t<std::is_integral<T>::value || (std::is_standard_layout_v<T> && std::is_trivial_v<T>), const T*> data(const T& ref ){ return &ref; };
    template <class T>
        std::enable_if_t<std::is_integral_v<T> || (std::is_standard_layout_v<T> && std::is_trivial_v<T>), T*> data(T& ref ){ return &ref; };
    // STL / string / scalar specializations
    template <class T, class... Ts> inline const T* data( const std::vector<T, Ts...>& ref ){ return ref.data(); }
    template <class T, class... Ts> inline       T* data(       std::vector<T, Ts...>& ref ){ return ref.data(); }
    template <class T, class... Ts> inline const T* data( const std::basic_string<T, Ts...>& ref ){ return ref.data(); }
    template <class T>                inline const T* data( const std::initializer_list<T>& ref ){ return ref.begin(); }
    inline const char* data( const char* ref ){ return ref; }
    template <class T, class... Ts>
        std::enable_if_t<meta::has_size<T>::value, std::array<size_t,1>
        > size(const T& ref){
        return {ref.size()};
    };
    template <class T, size_t N>
        std::array<size_t,1> size(const T(&ref)[N]){ return {N};};
    // scalars
    template <class T>
        std::enable_if_t<meta::is_scalar<T>::value, std::array<size_t,0>> size( const T& ){ return {}; }
    // non-scalar types without .size()
    template <class T>
        std::enable_if_t<!meta::is_scalar<T>::value && !meta::has_size<T>::value, std::array<size_t,0>> size( const T& ){ return {}; }
    template <class T, class... Ts> struct get {
        static inline T ctor( std::array<size_t,0> dims ){
            return T(); }};
    // ARRAYS
    template <class T,int N0> const T* data( const T(&ref)[N0]){ return &ref[0];};
    template <class T,int N1,int N0> const T* data( const T(&ref)[N1][N0]){ return &ref[0][0];};
    template <class T,int N2,int N1,int N0> const T* data( const T(&ref)[N2][N1][N0]){ return &ref[0][0][0];};
    template <class T,int N3,int N2,int N1,int N0> const T* data( const T(&ref)[N3][N2][N1][N0]){ return &ref[0][0][0][0];};
    template <class T,int N4,int N3,int N2,int N1,int N0> const T* data( const T(&ref)[N4][N3][N2][N1][N0]){ return &ref[0][0][0][0][0];};
    template <class T,int N5,int N4,int N3,int N2,int N1,int N0> const T* data( const T(&ref)[N5][N4][N3][N2][N1][N0]){ return &ref[0][0][0][0][0][0];};
    template <class T,int N6,int N5,int N4,int N3,int N2,int N1,int N0> const T* data( const T(&ref)[N6][N5][N4][N3][N2][N1][N0]){ return &ref[0][0][0][0][0][0][0];};

    template <class T,int N0>  T* data(T(&ref)[N0]){ return &ref[0];};
    template <class T,int N1,int N0>T* data(T(&ref)[N1][N0]){ return &ref[0][0];};
    template <class T,int N2,int N1,int N0>T* data(T(&ref)[N2][N1][N0]){ return &ref[0][0][0];};
    template <class T,int N3,int N2,int N1,int N0>T* data(T(&ref)[N3][N2][N1][N0]){ return &ref[0][0][0][0];};
    template <class T,int N4,int N3,int N2,int N1,int N0>T* data(T(&ref)[N4][N3][N2][N1][N0]){ return &ref[0][0][0][0][0];};
    template <class T,int N5,int N4,int N3,int N2,int N1,int N0>T* data(T(&ref)[N5][N4][N3][N2][N1][N0]){ return &ref[0][0][0][0][0][0];};
    template <class T,int N6,int N5,int N4,int N3,int N2,int N1,int N0>T* data(T(&ref)[N6][N5][N4][N3][N2][N1][N0]){ return &ref[0][0][0][0][0][0][0];};

    template <class T, int N> std::array<size_t, std::rank<T[N]>::value>
        size(const T* ref ){ return  h5::meta::get_extent<T[N]>(); };

    //STD::STRING
    template<> struct rank<std::basic_string<char>>: public std::integral_constant<size_t,0>{};
    template<> struct rank<std::basic_string<wchar_t>>: public std::integral_constant<size_t,0>{};
    template<> struct rank<std::basic_string<char16_t>>: public std::integral_constant<size_t,0>{};
    template<> struct rank<std::basic_string<char32_t>>: public std::integral_constant<size_t,0>{};
    template<> struct rank<std::basic_string_view<char>>: public std::integral_constant<size_t,0>{};
    template<> struct rank<std::basic_string_view<wchar_t>>: public std::integral_constant<size_t,0>{};
    template<> struct rank<std::basic_string_view<char16_t>>: public std::integral_constant<size_t,0>{};
    template<> struct rank<std::basic_string_view<char32_t>>: public std::integral_constant<size_t,0>{};
  
    template <class T, class... Ts> std::array<size_t,1> size( const std::basic_string<T, Ts...>& ref ){ return{ref.size()}; }
    inline std::array<size_t,1> size( const char* ref ){ return {std::char_traits<char>::length(ref)}; }
    template <class T, class... Ts> struct get<std::basic_string<T,Ts...>> {
        static inline std::basic_string<T,Ts...> ctor( std::array<size_t,1> dims ){
            return std::basic_string<T,Ts...>(); }};

// STD::INITIALIZER_LIST<T>
    template<int N0> struct rank<std::initializer_list<char[N0]>>: public std::integral_constant<size_t,1>{};
    template<int N0, int N1> struct rank<std::initializer_list<char[N0][N1]>>: public std::integral_constant<size_t,2>{};
    template<class T> struct rank<std::initializer_list<T>>: public std::integral_constant<size_t,1>{};
    template <class T> inline std::array<size_t,1> size( const std::initializer_list<T>& ref ){ return {ref.size()}; }

// STD::VECTOR<T>
    template<class T> struct rank<std::vector<T>>: public std::integral_constant<size_t,1>{};
    template <class T, class... Ts> std::array<size_t,1> size( const std::vector<T, Ts...>& ref ){ return{ref.size()}; }
    template<class T> struct get<std::vector<T>> {
        static inline std::vector<T> ctor( std::array<size_t,1> dims ){
            return std::vector<T>( dims[0] );
    }};

// STD::ARRAY<T>
    // 3.) read access
    template <class T, size_t N> inline const T* data( const std::array<T,N>& ref ){ return ref.data(); }
    template <class T, size_t N> inline T* data( std::array<T,N>& ref ){ return ref.data(); }
    template <class T, size_t N> inline typename std::array<size_t,1> size( const std::array<T,N>& ref ){ return {N}; }
    template <class T, size_t N> struct rank<std::array<T,N>> : public std::integral_constant<int,1> {};
// END STD::ARRAY

    template <class T> void get_fields( T& sp ){}
    template <class T> void get_field_names( T& sp ){}
    template <class T> void get_field_attributes( T& sp ){}

// NON_CONTIGUOUS 
    template <class T> struct member {
        using type = std::tuple<void>;
        static constexpr size_t size = 0;
    };
    template <class T> using csc_t = std::tuple< //compresses sparse row: index, colptr, values
            std::vector<unsigned long>, std::vector<unsigned long>, std::vector<T>>;
    const constexpr std::tuple<const char*, const char*, const char*> 
        csc_names = {"indices", "indptr","data"};
}
namespace h5::meta::linalg {
    /*types accepted by BLAS/LAPACK*/
    using blas = std::tuple<float,double,std::complex<float>,std::complex<double>>;
}

namespace h5::meta {
    // what handles may have attributes:  h5::acreate, h5::aread, h5::awrite
    template <class T, class... Ts> struct has_attribute : std::false_type {};
    template <> struct has_attribute<h5::gr_t> : std::true_type {};
    template <> struct has_attribute<h5::ds_t> : std::true_type {};
    template <> struct has_attribute<h5::ob_t> : std::true_type {};

    template <class T, class... Ts> struct is_location : std::false_type {};
    template <> struct is_location<h5::gr_t> : std::true_type {};
    template <> struct is_location<h5::fd_t> : std::true_type {};
}

// Gap 3: access_t enum and access_traits_t<T> — the executable memory-access contract.
// Describes how to obtain a data pointer, size, and byte count for a given type.
// Used to decouple I/O dispatch from concrete container types.
namespace h5::meta {
    enum class access_t {
        object,      // scalar / arithmetic / std-layout aggregate — direct memcpy via &v safe
        composite,   // non-std-layout aggregate (std::tuple) — needs pack/unpack via traits::pack/unpack
        contiguous,  // has .data() pointer + is_transport_contiguous (bulk memcpy safe)
        pointers,    // has .data() but element is not flat (e.g., vector<string>, vector<tuple>)
        iterators,   // begin/end traversal only — no direct pointer
        text,        // variable-length or fixed-length text (std::string, char*, etc.)
        unsupported
    };

    // Registry for types with explicit access_traits_t specializations in mapper files.
    // Prevents ambiguous partial-specialization resolution between generic fallbacks
    // and mapper-provided access_traits_t.
    namespace detail {
        template <class T> struct has_explicit_access_traits : std::false_type {};
        // vector<array<T,N>>: registered as explicit so the generic contiguous
        // fallback (which would size by vec.size()) doesn't claim it.
        template <class T, std::size_t N, class A>
        struct has_explicit_access_traits<std::vector<std::array<T,N>, A>> : std::true_type {};
        // std::array<char,N>: explicit text/FLS spec — overrides the generic
        // contiguous-container fallback that would treat it as a byte sequence.
        template <std::size_t N>
        struct has_explicit_access_traits<std::array<char, N>> : std::true_type {};
        // std::pair<K,V>: explicit object access (not aggregate, not arithmetic)
        template <class K, class V>
        struct has_explicit_access_traits<std::pair<K,V>> : std::true_type {};
        // std::tuple<Ts...>: explicit composite access — needs pack/unpack
        // because tuple is not guaranteed standard-layout.
        template <class... Ts>
        struct has_explicit_access_traits<std::tuple<Ts...>> : std::true_type {};
    }

    // Primary (unsupported — no match)
    template <class T, class = void>
    struct access_traits_t {
        static constexpr access_t kind = access_t::unsupported;
    };

    // Arithmetic scalars and enums
    template <class T>
    struct access_traits_t<T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>> {
        using element_t  = T;
        using pointer_t  = const T*;
        static constexpr access_t kind = access_t::object;
        static constexpr bool is_trivially_packable = true;
        static const T*  data(const T& v)  noexcept { return &v; }
        static T*        data(T& v)        noexcept { return &v; }
        static constexpr std::array<std::size_t,0> size(const T&) noexcept { return {}; }
        static constexpr std::size_t bytes(const T&) noexcept { return sizeof(T); }
    };

    // Plain aggregates: any struct/class that is an aggregate but not arithmetic,
    // array-like, or text-like.
    // Provides the memory-access contract so h5::write(ds, pod_value) works once
    // storage_traits_impl_t<T> is populated (old dt_t path or future C++26 reflection).
    template <class T>
    struct access_traits_t<T, std::enable_if_t<
        std::is_aggregate_v<T> &&
        !std::is_arithmetic_v<T> &&
        !is_array_like<T>::value &&
        !is_text_like<T>::value>> {
        using element_t  = T;
        using pointer_t  = const T*;
        static constexpr access_t kind = access_t::object;
        static constexpr bool is_trivially_packable = std::is_trivially_copyable_v<T>;
        static const T*  data(const T& v)  noexcept { return &v; }
        static T*        data(T& v)        noexcept { return &v; }
        static constexpr std::array<std::size_t,0> size(const T&) noexcept { return {}; }
        static constexpr std::size_t bytes(const T&) noexcept { return sizeof(T); }
    };

    // Text-like types (std::string, std::string_view, etc.) — handled by HDF5 string types, not raw memcpy
    template <class T>
    struct access_traits_t<T, std::enable_if_t<
        !detail::has_explicit_access_traits<remove_cvref_t<T>>::value &&
        is_text_like<T>::value &&
        !std::is_array_v<T> &&
        !std::is_pointer_v<remove_cvref_t<T>>>> {
        using element_t  = typename remove_cvref_t<T>::value_type;
        using pointer_t  = const element_t*;
        static constexpr access_t kind = access_t::text;
        static constexpr bool is_trivially_packable = false;
        static auto data(const T& s) noexcept { return s.data(); }
        static auto data(T& s)       noexcept { return s.data(); }
        static std::array<std::size_t,1> size(const T& s) noexcept { return {static_cast<std::size_t>(s.size())}; }
    };

    // Raw C-string pointers (char*, const char*) — same vlen_text path as std::string,
    // but value_type doesn't exist on a pointer so the spec above excludes them via
    // !std::is_pointer_v. element_t is fixed at char (matching dt_t<char*> = H5T_C_S1 VLEN).
    template <class T>
    struct access_traits_t<T, std::enable_if_t<
        !detail::has_explicit_access_traits<remove_cvref_t<T>>::value &&
        is_text_like<T>::value &&
        std::is_pointer_v<remove_cvref_t<T>>>> {
        using element_t  = char;
        using pointer_t  = const char*;
        static constexpr access_t kind = access_t::text;
        static constexpr bool is_trivially_packable = false;
        static const char* data(const T& s) noexcept { return s; }
        static std::array<std::size_t,1> size(const T& s) noexcept {
            return { s ? std::char_traits<char>::length(s) : std::size_t{0} };
        }
    };

    // C-style arrays T[N] — contiguous by definition
    template <class T, std::size_t N>
    struct access_traits_t<T[N]> {
        using element_t  = typename std::remove_all_extents_t<T[N]>;
        using pointer_t  = const element_t*;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static const element_t* data(const T(&a)[N]) noexcept { return reinterpret_cast<const element_t*>(a); }
        static element_t*       data(T(&a)[N])       noexcept { return reinterpret_cast<element_t*>(a); }
        static constexpr std::array<std::size_t,1> size(const T(&)[N]) noexcept { return {N}; }
        static constexpr std::size_t bytes(const T(&)[N]) noexcept { return N * sizeof(T); }
    };

    // char[N] — fixed-length HDF5 string override of the generic T[N] spec.
    // Sets kind=text + reports N as the string capacity (used by the dispatch
    // when constructing H5Tcopy(H5T_C_S1)+H5Tset_size(N)).
    template <std::size_t N>
    struct access_traits_t<char[N]> {
        using element_t  = char;
        using pointer_t  = const char*;
        static constexpr access_t kind = access_t::text;
        static constexpr std::size_t fixed_length = N;
        static constexpr bool is_trivially_packable = false;
        static const char* data(const char (&a)[N]) noexcept { return a; }
        static char*       data(char (&a)[N])       noexcept { return a; }
        static constexpr std::array<std::size_t, 1> size(const char (&)[N]) noexcept { return {N}; }
        static constexpr std::size_t bytes(const char (&)[N]) noexcept { return N; }
    };

    // std::array<char, N> — same as char[N]: fixed-length string at the
    // top-level write/read boundary.  Mirror of the char[N] spec above.
    template <std::size_t N>
    struct access_traits_t<std::array<char, N>> {
        using element_t  = char;
        using pointer_t  = const char*;
        static constexpr access_t kind = access_t::text;
        static constexpr std::size_t fixed_length = N;
        static constexpr bool is_trivially_packable = false;
        static const char* data(const std::array<char, N>& a) noexcept { return a.data(); }
        static char*       data(std::array<char, N>& a)       noexcept { return a.data(); }
        static constexpr std::array<std::size_t, 1> size(const std::array<char, N>&) noexcept { return {N}; }
        static constexpr std::size_t bytes(const std::array<char, N>&) noexcept { return N; }
    };

    // Contiguous sequence containers: has .data() pointer + transport contiguous element
    template <class T>
    struct access_traits_t<T, std::enable_if_t<
        !detail::has_explicit_access_traits<remove_cvref_t<T>>::value &&
        !std::is_array_v<T> &&
        has_data_pointer<T>::value &&
        meta::has_size<T>::value &&
        is_transport_contiguous_v<T>>> {
        using element_t  = std::remove_pointer_t<
                               compat::detected_or_t<void*, data_f, remove_cvref_t<T>>>;
        using pointer_t  = const element_t*;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static auto data(const T& c) noexcept { return c.data(); }
        static auto data(T& c)       noexcept { return c.data(); }
        static std::array<std::size_t,1> size(const T& c) noexcept { return {static_cast<std::size_t>(c.size())}; }
        static std::size_t bytes(const T& c) noexcept { return c.size() * sizeof(element_t); }
    };

    // Non-contiguous sequence containers with .data() (e.g., vector<string>)
    // Excludes text-like (string, string_view) — those route via the dedicated
    // text spec above; without this exclusion access_traits_t<std::string> would
    // be ambiguous because is_transport_contiguous_v<string>=false makes it look
    // like a "non-contiguous container with data()" to this spec.
    template <class T>
    struct access_traits_t<T, std::enable_if_t<
        !detail::has_explicit_access_traits<remove_cvref_t<T>>::value &&
        !std::is_array_v<T> &&
        !is_text_like<T>::value &&
        has_data_pointer<T>::value &&
        meta::has_size<T>::value &&
        compat::is_detected<value_type_f, remove_cvref_t<T>>::value &&
        !is_transport_contiguous_v<T>>> {
        using element_t  = typename remove_cvref_t<T>::value_type;
        static constexpr access_t kind = access_t::pointers;
        static constexpr bool is_trivially_packable = false;
        static auto data(const T& c) noexcept { return c.data(); }
        static std::array<std::size_t,1> size(const T& c) noexcept { return {static_cast<std::size_t>(c.size())}; }
    };

    // Iterator-only containers: begin/end but no .data() (list, set, map, ...)
    template <class T>
    struct access_traits_t<T, std::enable_if_t<
        !detail::has_explicit_access_traits<remove_cvref_t<T>>::value &&
        !std::is_array_v<T> &&
        !has_data_pointer<T>::value &&
        meta::has_iterator<T>::value &&
        compat::is_detected<value_type_f, remove_cvref_t<T>>::value>> {
        using element_t  = typename remove_cvref_t<T>::value_type;
        static constexpr access_t kind = access_t::iterators;
        static constexpr bool is_trivially_packable = false;
        static std::array<std::size_t,1> size(const T& c) noexcept {
            if constexpr (meta::has_size<T>::value)
                return {static_cast<std::size_t>(c.size())};
            else
                return {static_cast<std::size_t>(std::distance(c.begin(), c.end()))};
        }
    };

    // std::pair<K,V>: object access — single addressable value, compound HDF5 type
    template <class K, class V>
    struct access_traits_t<std::pair<K,V>> {
        using element_t  = std::pair<K,V>;
        using pointer_t  = const std::pair<K,V>*;
        static constexpr access_t kind = access_t::object;
        static constexpr bool is_trivially_packable = std::is_trivially_copyable_v<std::pair<K,V>>;
        static const std::pair<K,V>* data(const std::pair<K,V>& v) noexcept { return &v; }
        static std::pair<K,V>*       data(std::pair<K,V>& v)       noexcept { return &v; }
        static constexpr std::array<std::size_t,0> size(const std::pair<K,V>&) noexcept { return {}; }
        static constexpr std::size_t bytes(const std::pair<K,V>&) noexcept { return sizeof(std::pair<K,V>); }
    };

    // std::tuple<Ts...>: composite access — std::tuple is not guaranteed standard-layout,
    // so dispatch must pack into a flat buffer matching the HDF5 compound type's field
    // offsets (see detail::tuple_layout). pack/unpack replace data()/sizeof on this kind.
    template <class... Ts>
    struct access_traits_t<std::tuple<Ts...>> {
        using element_t  = std::tuple<Ts...>;
        using pointer_t  = const std::tuple<Ts...>*;
        using layout_t   = detail::tuple_layout_t<std::tuple<Ts...>>;
        static constexpr access_t kind = access_t::composite;
        static constexpr bool is_trivially_packable = false;
        static constexpr std::array<std::size_t,0> size(const std::tuple<Ts...>&) noexcept { return {}; }
        static constexpr std::size_t bytes() noexcept { return layout_t::total_size(); }
        static constexpr std::size_t bytes(const std::tuple<Ts...>&) noexcept { return layout_t::total_size(); }
        static void pack(const std::tuple<Ts...>& t, char* buf) noexcept { layout_t::to_buffer(t, buf); }
        static void unpack(std::tuple<Ts...>& t, const char* buf) noexcept { layout_t::from_buffer(t, buf); }
    };

    // std::complex<T>: object access — trivially copyable, standard-layout; not aggregate
    // so it falls through to unsupported without this explicit specialization.
    template <class T>
    struct access_traits_t<std::complex<T>, std::enable_if_t<std::is_floating_point_v<T>>> {
        using element_t  = std::complex<T>;
        using pointer_t  = const std::complex<T>*;
        static constexpr access_t kind = access_t::object;
        static constexpr bool is_trivially_packable = true;
        static const std::complex<T>* data(const std::complex<T>& v) noexcept { return &v; }
        static std::complex<T>*       data(std::complex<T>& v)       noexcept { return &v; }
        static constexpr std::array<std::size_t,0> size(const std::complex<T>&) noexcept { return {}; }
        static constexpr std::size_t bytes(const std::complex<T>&) noexcept { return sizeof(std::complex<T>); }
    };

    // vector<array<T,N>>: contiguous, but the inner extent N means the flat
    // element count is vec.size()*N, not vec.size(). The generic contiguous
    // fallback sizes by vec.size() only and produces a dataset that's 1/N too
    // small — see has_explicit_access_traits registration above.
    // vector<std::array<T,N>> — Canonical mapping (Winston):
    //   element_t = std::array<T,N> (so the array_dataset dispatch can
    //   recover the inner extent via std::tuple_size).
    //   size      = {c.size()}     (outer count only).
    // The previous flatten-to-(outer*N) layout was the fixed_inner_extent
    // path; superseded by the H5T_ARRAY-element representation.
    template <class T, std::size_t N, class A>
    struct access_traits_t<std::vector<std::array<T,N>, A>> {
        using element_t  = std::array<T,N>;
        using pointer_t  = const std::array<T,N>*;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static const T* data(const std::vector<std::array<T,N>,A>& c) noexcept {
            return c.empty() ? nullptr : c.front().data();
        }
        static T* data(std::vector<std::array<T,N>,A>& c) noexcept {
            return c.empty() ? nullptr : c.front().data();
        }
        static std::array<std::size_t,1> size(const std::vector<std::array<T,N>,A>& c) noexcept {
            return {c.size()};
        }
        static std::size_t bytes(const std::vector<std::array<T,N>,A>& c) noexcept {
            return c.size() * N * sizeof(T);
        }
    };

    template <class T>
    inline constexpr access_t access_kind_v = access_traits_t<remove_cvref_t<T>>::kind;
}
