/*
 * Copyright (c) 2018-2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_META_HPP 
#define  H5CPP_META_HPP

#include "compat.hpp"
#include <type_traits>
#include <utility>
#include <tuple>
#include <array>

namespace h5 {
	// type-name helper class for compile time id and printout 
	template <class T> struct name {
		static constexpr char const * value = "n/a";
	};
}

namespace h5::meta::detail { // feature detection
    using compat::is_detected;
    template<class T> using value_t = typename T::value_type;
    template<class T> using get_value_type = compat::detected_or<compat::nonesuch, value_t, T>;
    template<class T> using has_value_type = is_detected<value_t, T>;
    // default case
    template <class T, class TagOrClass, class E = void> struct is_value_type {
        constexpr static bool value = false;
    };
    // templated classes are tagged for identification 
    template <class T, class TagOrClass> struct is_value_type <T, TagOrClass,
        typename std::enable_if<has_value_type<T>::value>::type > {
        constexpr static bool value = std::is_convertible<typename detail::get_value_type<T>::type, TagOrClass>::value;
    };
    // enum classes can't be tagged, nor pods may need tagging
    template <class T, class TagOrClass> struct is_value_type <T, TagOrClass,
        typename std::enable_if<!has_value_type<T>::value>::type > {
        constexpr static bool value = std::is_convertible<T, TagOrClass>::value;
    };
}

namespace h5::meta {
    template <typename T> using value_type_f = typename T::value_type;
    template <typename T> using data_f = decltype(std::declval<T>().data());
    template <typename T> using size_f = decltype(std::declval<T>().size());
    template <typename T> using begin_f = decltype(std::declval<T>().begin());
    template <typename T> using end_f = decltype(std::declval<T>().end());
    template <typename T> using cbegin_f = decltype(std::declval<T>().cbegin());
    template <typename T> using cend_f = decltype(std::declval<T>().cend());

    template <typename T> using value = compat::detected_or<T, value_type_f, T>;
    template <typename T> using has_value_type = compat::is_detected<value_type_f, T>;
    template <typename T> using has_data = compat::is_detected<data_f, T>;
    template <typename T> using has_direct_access = compat::is_detected<data_f, T>;
    template <typename T> using has_size = compat::is_detected<size_f, T>;
    template <typename T> using has_begin = compat::is_detected<begin_f, T>;
    template <typename T> using has_end = compat::is_detected<end_f, T>;
    template <typename T> using has_cbegin = compat::is_detected<cbegin_f, T>;
    template <typename T> using has_cend = compat::is_detected<cend_f, T>;

    template <typename T> using has_iterator = std::integral_constant<bool, has_begin<T>::value && has_end<T>::value >;
    template <typename T> using has_const_iterator = std::integral_constant<bool, has_cbegin<T>::value && has_cend<T>::value >;
}

// STATIC FOR_LOOP
namespace h5::meta::detail {
    template <size_t S,  class callable_t, size_t ...Is>
    constexpr void static_for( callable_t&& callback, std::integer_sequence<size_t, Is...> ){
        ( callback( std::integral_constant<size_t, S + Is>{ } ),... );
    }
}
namespace h5::meta {
    // tuple_t -- tuple we are to iterate through
    // S -- start value
    // callable_t -- Callable object
    template <class tuple_t, size_t S = 0, class callable_t >
    constexpr void static_for( callable_t&& callback ){
        constexpr size_t N = std::tuple_size<tuple_t>::value;
        detail::static_for<S, callable_t>(
                std::forward<callable_t>(callback),
                std::make_integer_sequence<size_t, N - S>{ } );
    }
}

namespace h5::meta {
    namespace detail {
        // declaration
        template<class search_pattern, int position, int count, bool branch, class prev_head, class... arguments> struct tuple_pos;
        // initialization case 
        template<class S, int P, int C, bool B, class not_used, class... Tail >
        struct tuple_pos<S, P,C, B, not_used, std::tuple<Tail...>>
            : tuple_pos<S, P,C, false, not_used, std::tuple<Tail...>> {
         };
        // recursive case 
        template<class S, int P, int C, class not_used, class Head, class... Tail >
        struct tuple_pos<S, P,C,false, not_used, std::tuple<Head, Tail...>>
            : tuple_pos<S, P+1,C, std::is_same<Head,S>::value, Head, std::tuple<Tail...>> { };
        // match case 
        template<class S, int P, int C, class Type, class... Tail >
        struct tuple_pos<S, P,C,true, Type, std::tuple<Tail...>>
            : std::integral_constant<int,P>{
            using type  = Type;
            static constexpr bool present = true;
         };
        // default case
        template<class S, class H, int P, int C>
        struct tuple_pos<S, P,C, false, H, std::tuple<>>
            : std::integral_constant<int,-P>{
            static constexpr bool present = false;
        };
    }
    /* searches through list of `Ts...` types, and sets 
     * tpos<>::present, tpos<>::value, tpos<>::type as a function of `T` is found
     */
    template<class S, class... T> struct tpos;
    template<class S, class... Ts> struct tpos <S,std::tuple<Ts...>>
        : detail::tuple_pos<S, -1,0,false, void, std::tuple<Ts...>>{ };
}

namespace h5::arg {
    // declaration
    template<class S, class... T > struct tpos;
    namespace detail {
        // declaration
        template<class search_pattern, int position, int count, bool branch, class prev_head, class arguments> struct tuple_pos;
        // initialization case 
        template<class S, int P, int C, bool B, class not_used, class... Tail >
        struct tuple_pos<S, P,C, B, not_used, std::tuple<Tail...>>
            : tuple_pos<S, P,C, false, not_used, std::tuple<Tail...>> { };
        // recursive case 
        template<class S, int P, int C, class not_used, class Head, class... Tail >
        struct tuple_pos<S, P,C,false, not_used, std::tuple<Head, Tail...>>
            : tuple_pos<S, P+1,C, std::is_convertible<Head,S>::value, Head, std::tuple<Tail...>> { };
        // match case 
        template<class S, int P, int C, class Type, class... Tail >
        struct tuple_pos<S, P,C,true, Type, std::tuple<Tail...>>
            : std::integral_constant<int,P>{
            using type  = Type;
            static constexpr bool present = true;
         };
        // default case
        template<class S, class H, int P, int C>
        struct tuple_pos<S, P,C, false, H, std::tuple<>>
            : std::integral_constant<int,-1>{
            static constexpr bool present = false;
        };
        template <class S, class... args_t,
                 class idx = tpos</*search: */const S&,/*args: */ const args_t&...,const S&>>
        typename idx::type&  get( const S& def, args_t&&... args ){
            auto tuple = std::forward_as_tuple(args..., def );
            return std::get<idx::value>( tuple );
        }
    }
    template<class S, class... args_t > struct tpos
        : detail::tuple_pos<const S&,-1,0,false, void, std::tuple<args_t...>>{ };

    template <class S, class... args_t,
             class idx = tpos</*search: */const S&,/*args: */ const args_t&...,const S&>>
    typename idx::type&
    get( const S& def, args_t&&... args ){
        auto tuple = std::forward_as_tuple(args..., def );
        return std::get<idx::value>( tuple );
    }
    template <int idx, class... args_t>
    typename std::tuple_element<idx, std::tuple<args_t...> >::type&
    getn(args_t&&... args ){
        auto tuple = std::forward_as_tuple(args...);
        return std::get<idx>( tuple );
    }
}

namespace h5::impl {
    //<public domain code> borrowed from c++ library reference: to lower c+14 to c++11
    template<bool B, class T, class F> struct conditional { typedef T type; };
    template<class T, class F> struct conditional<false, T, F> { typedef F type; };
}


namespace h5::impl {
    template <typename T, typename ...>
    struct cat {
        using type = T; };

    template <template <class ...> class C,
              class ... Ts1, class ... Ts2, class ... Ts3>
    struct cat<C<Ts1...>, C<Ts2...>, Ts3...>
       : public cat<C<Ts1..., Ts2...>, Ts3...>{ };
}

// extent: see H5Tall
namespace h5::meta::impl {
    template <std::size_t...> struct extents{ };

    template <class T, std::size_t N, std::size_t... next>
    struct extents_ : public extents_<T, N-1, std::extent<T,N-1>::value, next...>{ };

    template <class T, std::size_t... next> struct extents_<T, 0, next... >{
        using type = extents<next...>;
    };

    template <class T, std::size_t N=std::rank<T>::value>
    using get_extents = typename extents_<T, N>::type;

    template<typename T, std::size_t N=std::rank<T>::size, std::size_t... I> constexpr auto get_extent( extents<I...> ) {
        return std::array<hsize_t, N>{ { I...} };
    }
}
namespace h5::meta {
    template<typename T, std::size_t N=std::rank<T>::value>
    constexpr auto get_extent() {
        return impl::get_extent<hsize_t, N>( impl::get_extents<T>{} );
    }
}

// get type names at compile time
namespace h5::meta::impl {
    #define h5cpp_array__ 'a','r','r','a','y'
    #define h5cpp_char__ 'c','h','a','r'
    #define h5cpp_int__  'i','n','t'
    #define h5cpp_short__ 's','h','o','r','t'
    #define h5cpp_long__ 'l','o','n','g'
    #define h5cpp_longlong__ h5cpp_long__,' ',h5cpp_long__
    #define h5cpp_float__ 'f','l','o','a','t'
    #define h5cpp_double__ 'd','o','u','b','l','e'
    #define h5cpp_longdouble__ h5cpp_long__,' ',h5cpp_double__
    template<class T, size_t... N> struct name {
        constexpr static const char value[] =  {h5cpp_array__,'[', N..., ']', '\0'};
    };

    #define h5cpp_unsigned_name__( type__, name__ ) \
        template<size_t... N> struct name <unsigned type__,N...> { \
            constexpr static const char value[] =  {'u','n','s','i','g','n','e','d',' ',name__,'[',  N..., ']',0}; \
        };\

    #define h5cpp_signed_name__( type__, name__ ) \
        template<size_t... N> struct name <type__,N...> { \
            constexpr static const char value[] =  {name__, '[', N..., ']',0}; \
        };\

    #define h5cpp_name(type__) h5cpp_signed_name__(type__, h5cpp_##type__##__)  h5cpp_unsigned_name__(type__, h5cpp_##type__##__)
    #define h5cpp_fname(type__) h5cpp_signed_name__(type__, h5cpp_##type__##__)
    #define h5cpp_fname2(type1__, type2__) h5cpp_signed_name__(type1__ type2__, h5cpp_##type1__##type2__##__)
    #define h5cpp_name2(type1__, type2__) \
        h5cpp_signed_name__(type1__ type2__, h5cpp_##type1__##type2__##__)   \
        h5cpp_unsigned_name__(type1__ type2__, h5cpp_##type1__##type2__##__) \

    h5cpp_name(char) h5cpp_name(int) h5cpp_name(short) h5cpp_name(long) 
    h5cpp_name2(long, long)
    h5cpp_fname(float) h5cpp_fname(double)
    h5cpp_fname2(long, double)


    template <class T, char... digits> using text = impl::name<typename std::remove_all_extents<T>::type, digits...>;

    template <int rank, class T, int... digits> struct parse;
    template <int rank, class T, int rem, int... digits> struct parse_extent;

    // recursive case, popping each extent one by one
    template <int rank, class T, int... digits> struct parse
        : parse_extent<rank-1, T, std::extent<T,rank-1>::value, digits...>{};
    // terminal case with rank zero
    template <class T, int... digits> struct parse <0, T, digits...> 
        : text<T, digits...> {};
    // converting extent number to sequence of digits
    template <int rank, class T, int rem, int... digits> struct parse_extent
        : parse_extent<rank, T, rem/10, (rem % 10) + '0', digits...> {};
    template <int rank, class T,  int... digits> struct parse_extent <rank, T, 0, digits...>
        : parse<rank, T, ',',digits...> {}; // separate dimensions with comma: ','
    template <class T,  int... digits> struct parse_extent <0, T, 0, digits...>
        : parse<0, T, digits...> {};        // terminal case has no commas:
#undef h5cpp_short__
}
namespace h5::meta {
template<class T> struct extent_to_string :
    h5::meta::impl::parse<std::rank<T>::value, T>{};
}
#endif
