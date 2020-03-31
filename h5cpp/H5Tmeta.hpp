
/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_META123_HPP 
#define  H5CPP_META123_HPP

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

    template<class T, class... Ts> using is_stl = /* will use concepts once become available */
        std::disjunction<is_container<T,Ts...>,is_adaptor<T,Ts...>>;

    template <class... Args> using enable_or = std::enable_if<std::disjunction_v<Args...>>;
    template <class... Args> using enable_and = std::enable_if<std::conjunction_v<Args...>>;
    template <class T, class... Ts> struct decay { using type = T; };
    /* std::array<...,R> size<T>(){} template has to compute rank R at compile 
     * time, these templates, and their respective specializations aid to accomplish that*/
    template<class T, class... Ts> struct rank : public std::integral_constant<int,0> {}; // definition
    template <class T, size_t N> struct rank<T[N]> : public std::rank<T[N]> {}; // arrays
    template<class T, int N, class... Ts> using is_rank = std::integral_constant<bool, rank<T, Ts...>::value == N >;
    // helpers for is_rank<T>, don't need specialization, instead define 'rank'
    template<class T, class... Ts> using is_scalar = is_rank<T,0,Ts...>; // numerical | pod 
    template<class T, class... Ts> using is_vector = is_rank<T,1,Ts...>;
    template<class T, class... Ts> using is_matrix = is_rank<T,2,Ts...>;
    template<class T, class... Ts> using is_cube   = is_rank<T,3,Ts...>;

    template <class T, class D=typename impl::decay<T>::type>
    using is_string = typename std::integral_constant<bool,
        std::is_same<T,std::string>::value || std::is_same<D, std::string>::value>;
    /* Objects may reside in continuous memory region such as vectors, matrices, POD structures can be saved/loaded in a single transfer,
     * the rest needs to be handled on a member variable bases*/
    template <class T> struct is_contigious : std::false_type {};
    template <class T, class... Ts> struct is_linalg : std::false_type {};
    template <class C, class T, class... Cs> struct is_valid : std::false_type {};

#define h5cpp_decay( OBJECT )\
    template <class T, class... Ts> struct decay<OBJECT<const T*, Ts...>>{ using type = const T*; }; \
    template <class T, class... Ts> struct decay<OBJECT<T*, Ts...>>{       using type = T*; };       \
    template <class T, class... Ts> struct decay<OBJECT<T, Ts...>>{        using type = T; };        \

    template <class T> struct decay<const T>{ using type = T; };
    template <class T> struct decay<const T*>{ using type = T*; };
    template <class T, signed N> struct decay<const T[N]>{ using type = T*; };
    template <class T, signed N> struct decay<T[N]>{ using tyoe = T*; };


// DEFAULT CASE
    template <class T> struct rank<T*>: public std::integral_constant<size_t,1>{};
    template <class T, class... Ts> T* data(const T& ref ){ return &ref; };
    template <class T, class... Ts> std::array<size_t, 0> size(const T& ref ){ return {}; };
    template <class T, class... Ts> struct get {
        static inline T ctor( std::array<size_t,0> dims ){
            return T(); }};

//STD::STRING
    template<> struct rank<std::string>: public std::integral_constant<size_t,0>{};
    h5cpp_decay(std::basic_string)
    template <class T, class... Ts> T* data(const std::basic_string<T, Ts...>& ref ){
        return ref.data(); }
    template <class T, class... Ts> std::array<size_t,1> size( const std::basic_string<T, Ts...>& ref ){ return{ref.size()}; }
    template <class T, class... Ts> struct get<std::basic_string<T,Ts...>> {
        static inline std::basic_string<T,Ts...> ctor( std::array<size_t,1> dims ){
            return std::basic_string<T,Ts...>(); }};

// STD::INITIALIZER_LIST<T>
    //h5cpp_decay(std::initializer_list)
    template<class T> inline typename std::enable_if< impl::is_scalar<T>::value,
        const T*>::type data( const std::initializer_list<T>& ref ){ return ref.begin(); }
    inline const char* const* data( const std::initializer_list<const char*>& ref ){ return ref.begin(); }

// STD::VECTOR<T>
    template<class T> struct rank<std::vector<T>>: public std::integral_constant<size_t,1>{};
    h5cpp_decay(std::vector)

// STD::ARRAY<T>
    // h5cpp_decay macro won't work on this one, write templates manually  
    template <class T, size_t N> struct decay<std::array<const T*,N>>{ typedef const T* type; };
    template <class T, size_t N> struct decay<std::array<T*,N>>{ typedef T* type; };
    template <class T, size_t N> struct decay<std::array<T,N>>{ typedef T type; };

    // 3.) read access
    template <class T, size_t N> inline const T* data( const std::array<T,N>& ref ){ return ref.data(); }
    template <class T, size_t N> inline T* data( std::array<T,N>& ref ){ return ref.data(); }

    template <class T, size_t N> inline typename std::array<size_t,1> size( const std::array<T,N>& ref ){ return {N}; }
    template <class T, size_t N> struct rank<std::array<T,N>> : public std::integral_constant<int,1> {};
// END STD::ARRAY
    h5cpp_decay(std::forward_list)
    h5cpp_decay(std::list)

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
// HID_T
    template <class T> struct decay<h5::dt_t<T>>{ using type = T; };
}
namespace h5::impl::linalg {
    /*types accepted by BLAS/LAPACK*/
    using blas = std::tuple<float,double,std::complex<float>,std::complex<double>>;
}

namespace h5::impl {
    /* what handles may have attributes:  h5::acreate, h5::aread, h5::awrite*/
    template <class T, class... Ts> struct has_attribute : std::false_type {};
    template <> struct has_attribute<h5::gr_t> : std::true_type {};
    template <> struct has_attribute<h5::ds_t> : std::true_type {};
    template <> struct has_attribute<h5::ob_t> : std::true_type {};
    template <class T> struct has_attribute<h5::dt_t<T>> : std::true_type {};

    template <class T, class... Ts> struct is_location : std::false_type {};
    template <> struct is_location<h5::gr_t> : std::true_type {};
    template <> struct is_location<h5::fd_t> : std::true_type {};

    //template <class T> struct 
}
#endif
