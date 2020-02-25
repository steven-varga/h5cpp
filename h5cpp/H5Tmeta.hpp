
/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_STL_HPP 
#define  H5CPP_STL_HPP

#include <type_traits>
#include <array>
#include <vector>
#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <any>

//FIXME: move it elsewhere
#define H5CPP_supported_elementary_types "supported elementary types ::= pod_struct | float | double |  [signed](int8 | int16 | int32 | int64)"

namespace h5 { namespace utils {
	template <class T, class... Ts>
	static constexpr bool is_supported = std::is_class<T>::value | std::is_arithmetic<T>::value;
}}
// stl detection with templates, this probably should stay until concepts become mainstream
namespace h5::impl {
	template <class T, class... Ts> struct is_container : std::false_type {};
	template <class T, class... Ts> struct is_container <std::vector<T,Ts...>> : std::true_type{};
	template <class T, class... Ts> struct is_container <std::deque<T,Ts...>> : std::true_type{};
	template <class T, class... Ts> struct is_container <std::list<T,Ts...>> : std::true_type{};
	template <class T, class... Ts> struct is_container <std::forward_list<T,Ts...>> : std::true_type{};
	template <class T, class... Ts> struct is_container <std::set<T,Ts...>> : std::true_type{};
	template <class T, class... Ts> struct is_container <std::multiset<T,Ts...>> : std::true_type{};
	template <class T, class... Ts> struct is_container <std::map<T,Ts...>> : std::true_type{};
	template <class T, class... Ts> struct is_container <std::multimap<T,Ts...>> : std::true_type{};
	template <class T, class... Ts> struct is_container <std::unordered_set<T,Ts...>> : std::true_type{};
	template <class T, class... Ts> struct is_container <std::unordered_map<T,Ts...>> : std::true_type{};
	template <class T, class... Ts> struct is_container <std::unordered_multiset<T,Ts...>> : std::true_type{};
	template <class T, class... Ts> struct is_container <std::unordered_multimap<T,Ts...>> : std::true_type{};
	template <class T, size_t N> struct is_container <std::array<T,N>> : std::true_type{};

	template <class T, class... Ts> struct is_adaptor : std::false_type {};
	template <class T, class... Ts> struct is_adaptor <std::stack<T,Ts...>> : std::true_type{};
	template <class T, class... Ts> struct is_adaptor <std::queue<T,Ts...>> : std::true_type{};
	template <class T, class... Ts> struct is_adaptor <std::priority_queue<T,Ts...>> : std::true_type{};

	template<class T, class... Ts> using is_stl =
		std::disjunction<is_container<T,Ts...>,is_adaptor<T,Ts...>>;

	// feature detection: leave this code here until concepts
	using h5::impl::compat::is_detected;
	template<class T> using copy_assign_t = decltype(std::declval<T&>() = std::declval<const T&>());
	template<class T> using has_data_t = decltype(std::declval<T&>().data());
	template<class T> using has_size_t = decltype(std::declval<T&>().size());
	template<class T> using has_begin_t = decltype(std::declval<T&>().begin());
	template<class T> using has_const_begin_t = decltype(std::declval<T&>().begin());
	template<class T> using has_end_t = decltype(std::declval<T&>().end());
	template<class T> using has_const_end_t = decltype(std::declval<T&>().end());

	template<class T> using has_pop_t = decltype(std::declval<T&>().pop());
	template<class T> using has_swap_t = decltype(std::declval<T&>().swap());
	template<class T> using has_emplace_t = decltype(std::declval<T&>().emplace());

	template<class T> using has_container_t = typename T::container_type;
	template<class T> using has_value_compare_t = typename T::value_compare;
	template<class T> using has_const_iterator_t = typename T::const_iterator;
	template<class T> using has_difference_t = typename T::difference_type;

	template<class T, class... Args> using is_iterable = std::conjunction<
		is_detected<has_begin_t,T>>;
	template<class T, class... Args> using is_compare = std::conjunction<
		is_detected<has_value_compare_t,T>>;
	template<class T, class... Args> using not_supported = std::conjunction<
		std::is_enum<T>, std::is_union<T>>;
	template<class T, class... Args> using is_reference = std::conjunction<
		std::is_pointer<T>, std::is_reference<T>>;
}

namespace h5::exp {
	// helpers 
	template <class... Args> using enable_or = std::enable_if<std::disjunction_v<Args...>>;
	template <class... Args> using enable_and = std::enable_if<std::conjunction_v<Args...>>;

	template <class T, class...> struct decay { using type = T; };
#define h5cpp_decay_t( OBJECT )\
	template <class T> struct decay<OBJECT<const T*>>{ using type = const T*; };\
	template <class T> struct decay<OBJECT<T*>>{       using type = T*; }; 		\
	template <class T> struct decay<OBJECT<T>>{        using type = T; }; 		\

	template <class T> struct decay<const T>{ using type = T; };
	template <class T> struct decay<const T*>{ using type = T*; };
	template <class T> struct decay<std::basic_string<T>>{ using type = T*; };
	template <class T, signed N> struct decay<const T[N]>{ using type = T*; };
	template <class T, signed N> struct decay<T[N]>{ using type = T*; };
#undef h5cpp_decay_t
	namespace linalg {
		template<class C, class E = void> struct is_dense;
		template<class C, class E = void> struct is_sparse;
	}
}
namespace h5::impl {
	using h5::exp::enable_or;
	using h5::exp::enable_and;
	/* all numerical types listed here are supported by HDF5 as builtin NATIVE types, FIXME: add complex types 
	 * Passing by `values` strictly where copies are cheap, `references` are for larger objects, `pointers` are
	 * special cases to provide controlled direct access to underlying datastore: arma::mat<T>().mem_ptr();
	 * numerical ::= (signed | unsigned)[char | short | int | long | long int, long long int] | [float | double | long double]
	 * which are just std::arithmetic types
	 */

	/* std::array<...,R> size<T>(){} template has to compute rank R at compile 
	 * time, these templates aid to accomplish that. In addition to the provided */
	template<class T, class... Ts> struct rank : public std::integral_constant<int,0> {}; // definition
	template <class T, size_t N> struct rank<T[N]> : public std::rank<T[N]> {}; // arrays
	template<class T, int N, class... Ts> using is_rank = std::integral_constant<bool, rank<T, Ts...>::value == N >;

	template<class T, class... Ts> using is_scalar = is_rank<T,0,Ts...>; // numerical | pod 
	template<class T, class... Ts> using is_vector = is_rank<T,1,Ts...>;
	template<class T, class... Ts> using is_matrix = is_rank<T,2,Ts...>;
	template<class T, class... Ts> using is_cube   = is_rank<T,3,Ts...>;

	template<class T, class E = void> struct is_standard_layout : public std::false_type{};
	template <class T> class is_standard_layout<T, class enable_or<
		std::is_array<T>, h5::impl::is_detected<h5::impl::has_data_t,T>>::type>
	: public std::true_type{ };

	template <class T, class...> struct decay { using type = T; };
#define h5cpp_decay( OBJECT )\
	template <class T> struct decay<OBJECT<const T*>>{ using type = const T*; };\
	template <class T> struct decay<OBJECT<T*>>{       using type = T*; }; 		\
	template <class T> struct decay<OBJECT<T>>{        using type = T; }; 		\

	template <class T> void get_fields( T& sp ){}
	template <class T> void get_field_names( T& sp ){}
	template <class T> void get_field_attributes( T& sp ){}

/*STL: */
	// 2.) filter is_xxx_type
	// 4.) write access
	// 5.) obtain dimensions of extents
	// 6.) ctor with right dimensions

	// 1.) object -> H5T_xxx
	template <class T> struct decay<const T>{ using type = T; };
	template <class T> struct decay<const T*>{ using type = T*; };
	template <class T, signed N> struct decay<const T[N]>{ using type = T*; };
	template <class T, signed N> struct decay<T[N]>{ using tyoe = T*; };

	h5cpp_decay(std::basic_string)
	h5cpp_decay(std::initializer_list)
	h5cpp_decay(std::vector)

//TODO: REMOVE FROM	
	// type trait helpers
	//template <class T>
	//using is_scalar = std::integral_constant<bool,
	//	std::is_integral<T>::value || std::is_pod<T>::value || std::is_same<T,std::string>::value>;
	template <class T, class D=typename impl::decay<T>::type>
	using is_arithmetic = typename std::integral_constant<bool,
		std::is_integral<D>::value || std::is_floating_point<D>::value>;
	//all string versions must be sieved out, since HDF5 treats them differently from other types
	template <class T, class D=typename impl::decay<T>::type>
	using is_string = typename std::integral_constant<bool,
		std::is_same<T,std::string>::value || std::is_same<D, std::string>::value>;
	// tricky: literals may be converted to strings, and we don't want them here
	template <class T, class D=typename impl::decay<T>::type>
	using is_pod = typename std::integral_constant<bool,
		!is_arithmetic<T>::value && std::is_pod<D>::value && !is_string<T>::value>;


	/* Objects may reside in continuous memory region such as vectors, matrices, POD structures can be saved/loaded in a single transfer,
	 * the rest needs to be handled on a member variable bases, `h5::impl::is_coalesced` divides objects at compile time*/
	template <class T> struct is_multi : std::integral_constant<bool,false>{}; // TODO: remove
//TODO: REMOVE UNTIL ----------------------------------

	template <class T> struct member {
		using type = std::tuple<void>;
		static constexpr size_t size = 0;
	};

	template <class T> using csc_t = std::tuple< //compresses sparse row: index, colptr, values
			std::vector<unsigned long>, std::vector<unsigned long>, std::vector<T>>;

	const constexpr std::tuple<const char*, const char*, const char*> 
		csc_names = {"indices", "indptr","data"};
/*TODO: figure out */
	template<> struct rank<std::string>: public std::integral_constant<size_t,1>{};
	template<class T> struct rank<T*>: public std::integral_constant<size_t,1>{};
	template<class T> struct rank<std::vector<T>>: public std::integral_constant<size_t,1>{};

	// 3.) read access
	template <class T> inline typename std::enable_if<std::is_integral<T>::value || std::is_pod<T>::value,
		const T*>::type data( const T& ref ){ return &ref; }
	template<class T> inline typename std::enable_if< impl::is_scalar<T>::value,
		const T*>::type data( const std::initializer_list<T>& ref ){ return ref.begin(); }
	inline const char* const* data( const std::initializer_list<const char*>& ref ){ return ref.begin(); }
	inline const char* data( const std::string& ref ){ return ref.c_str(); }
	template <class T> inline const T* data( const std::vector<T>& ref ){
		return ref.data();
	}
	template <class T> inline T* data( std::vector<T>& ref ){
		return ref.data();
	}
	// 4.) write access
	template <class T> inline typename std::enable_if<std::is_integral<T>::value,
	T*>::type data( T& ref ){ return &ref; }
	// 5.) obtain dimensions of extents
	template <class T> inline constexpr typename std::enable_if< impl::is_scalar<T>::value,
		std::array<size_t,0>>::type size( T value ){ return{}; }
	template <class T> inline typename std::enable_if< impl::is_vector<T>::value,
		std::array<size_t,1>>::type size( const T& ref ){ return {ref.size()}; }
	// 6.) ctor with right dimensions
	template <class T> struct get {
	   	static inline T ctor( std::array<size_t,impl::rank<T>::value> dims ){
			return T(); }};
	template<class T>
	struct get<std::vector<T>> {
		static inline std::vector<T> ctor( std::array<size_t,1> dims ){
			return std::vector<T>( dims[0] );
	}};

// STD::ARRAY<T>
	template <class T, size_t N> struct decay<std::array<const T*,N>>{ typedef const T* type; };
	template <class T, size_t N> struct decay<std::array<T*,N>>{ typedef T* type; };
	template <class T, size_t N> struct decay<std::array<T,N>>{ typedef T type; };

	// 3.) read access
	template <class T, size_t N> inline const T* data( const std::array<T,N>& ref ){ return ref.data(); }
	template <class T, size_t N> inline T* data( std::array<T,N>& ref ){ return ref.data(); }

	template <class T, size_t N> inline typename std::array<size_t,1> size( const std::array<T,N>& ref ){ return {N}; }
	template <class T, size_t N> struct rank<std::array<T,N>> : public std::integral_constant<int,1> {};

	template <class T> struct decay<std::forward_list<const T*>>{ typedef const T* type; };
	template <class T> struct decay<std::forward_list<T*>>{ typedef T* type; };
	template <class T> struct decay<std::forward_list<T>>{ typedef T type; };

	template <class T> struct decay<std::list<const T*>>{ typedef const T* type; };
	template <class T> struct decay<std::list<T*>>{ typedef T* type; };
	template <class T> struct decay<std::list<T>>{ typedef T type; };
}
#endif
