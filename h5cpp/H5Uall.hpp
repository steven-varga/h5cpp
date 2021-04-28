/*
 * Copyright (c) 2018-2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_UTIL_HPP
#define  H5CPP_UTIL_HPP

#include "H5Tmeta.hpp"
#include <math.h>
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
#include <random>
#include <limits>

namespace h5::utils::string {
    /* literals used to generate random strings*/
    template <class T> struct literal{};
    template <> struct literal <char> {
        constexpr static char value [] = 
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    };
    template <> struct literal <wchar_t> {
        constexpr static wchar_t value [] = 
            L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    };
    template <> struct literal <char16_t> {
        constexpr static char16_t value [] = 
            u"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    };
    template <> struct literal <char32_t> {
        constexpr static char32_t value [] = 
            U"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    };
}

namespace h5::utils { // SCALARS
    template <bool B> using bool_t = std::integral_constant<bool, B>;
    template <class T> using is_float = bool_t<std::is_floating_point<T>::value>;
    template <class T> using is_integral = bool_t<std::is_integral<T>::value>;
    template <class T> using is_numeric = bool_t<std::is_arithmetic<T>::value && !std::is_same<T,bool>::value>;

    /** @brief base case for POD structs*/  
    template <class T, typename C=bool_t<true>> struct data {
          /** @ingroup utils
        * @brief returns default T POD or class type, paramters **min** and **max** are ignored
        * The default case provides a functional implementation as long as `T` has default constructor
        * specialize this class should you need control over the returned value 
        * @param min ignored
        * @param max ignored
        * @tparam T datatype of returned value
        * @code
        * pod_t value = h5::utils::data<pod_t>::get(10, 20);
        * @endcode  
        */
        static T get(size_t min, size_t max) {
            return T();
        }      
    };
    template <> struct data <bool>{
        /** @ingroup utils
        * @brief returns random true | false drawn from bernaulli distribution 
        * considering the domain of `T` dataype the routine draws a value from paramterized bernaulli distribution(nom/denom),
        * with the probability of `true` given by nom/denom ratio .
        * @param nom nominator
        * @param denom denominator
        * @code
        * bool value = h5::utils::data<bool>::get(0, 1);
        * @endcode  
        */
        static bool get(size_t nom, size_t denom) {
            std::random_device rd;
            std::default_random_engine rng(rd());
            std::bernoulli_distribution dist(nom / denom);
            return dist(rng);
        }
    };
    template <class T> struct data <T, is_numeric<T>>{
        /** @ingroup utils
        * @brief returns random value drawn from uniform distribution with length between **min** and **max**
        * considering the domain of `T` dataype the routine draws a value from a uniformed distribution within specified range
        * @param min lower bound of integral value
        * @param max upper bound of integral value
        * @tparam T datatype of returned value
        * @code
        * unsigned short value = h5::utils::data<unsigned short>::get(10, 20);
        * @endcode  
        */
        static T get(T min, T max) {
            using dist_t = typename std::conditional<std::is_integral<T>::value,
                std::uniform_int_distribution<T>, std::uniform_real_distribution<T>>::type;
            std::random_device rd;
            std::default_random_engine rng(rd());
            dist_t dist(min, max);
            return dist(rng);
        }
    };
    template <class T> struct data <std::basic_string<T>>{
        /** @ingroup utils
        * @brief returns random std::string<T> drawn from uniform distribution [A-Za-z] with length between **min** and **max**
        * @param min lower bound on the length
        * @param max upper bound on the length
        * @tparam T `char | wchar_t | char16_t | char32_t`
        * @code
        * std::string str = h5::utils::data<std::string>::get(10, 20);
        * @endcode  
        */
        static std::basic_string<T> get(size_t min, size_t max) {
            constexpr auto alphabet = h5::utils::string::literal<T>::value;
            std::random_device rd;
            std::default_random_engine rng(rd());
            std::uniform_int_distribution<> dist(0,sizeof(alphabet)/sizeof(*alphabet)-2);
            std::uniform_int_distribution<> string_length(min,max);

            std::basic_string<T> str;
            size_t N = string_length(rng);
            str.reserve(N);
            std::generate_n(std::back_inserter(str), N, [&]() {
                return alphabet[dist(rng)];
            });
            return str;
        }
    };
}

namespace h5::utils { // VECTORS
    template <class T> struct data <std::vector<T>, is_numeric<T>> {
        /** @ingroup utils
        * @brief returns random `l <= length <= u` vector  with integral values between **min** and **max**
        * considering the domain of an integral dataype the routine draws a set of values from a uniformed
        * distribution within specified range or within the std::numeric_limits<T>
        * @param l lower bound on vector size 
        * @param u upper bound on vector size
        * @param min std::numeric_limits<T>::min()
        * @param max std::numeric_limits<T>::max()
        * @tparam T datatype of returned value
        * @code
        * unsigned short value = h5::utils::data<unsigned short>::get(10, 20, 4, 17);
        * @endcode  
        */
        static std::vector<T> get(size_t l, size_t u, 
            T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()) {
            using dist_t = typename std::conditional<std::is_integral<T>::value,
                std::uniform_int_distribution<T>, std::uniform_real_distribution<T>>::type;

            std::random_device rd;
            std::default_random_engine rng(rd());
            std::uniform_int_distribution<size_t> size(l, u);
            std::vector<T> data(size(rng));
            dist_t dist(min, max);
            std::generate_n(data.begin(), data.size(), [&]() {
                return dist(rng);
            });
            return data;
        }
    };

    template <class T> struct data <std::vector<std::basic_string<T>>> {
        /** @ingroup utils
        * @brief returns random `l <= length <= u` vector of std::basic_string<T> between **min** and **max**
        * @param l lower bound on vector size 
        * @param u upper bound on vector size
        * @param min lower bound on string length
        * @param max upper bound on string length
        * @tparam T datatype of returned value
        * @code
        * unsigned short value = h5::utils::data<unsigned short>::get(10, 20, 4, 17);
        * @endcode  
        */
        static std::vector<std::basic_string<T>> get(size_t l, size_t u, size_t min = 5, size_t max = 21){
            std::random_device rd;
            std::default_random_engine rng(rd());
            std::uniform_int_distribution<size_t> size(l, u);
            std::vector<std::basic_string<T>> data(size(rng));
            
            std::generate_n(data.begin(), data.size(), [&]() {
                return h5::utils::data<std::basic_string<T>>::get(min, max);
            });
            return data;     
        }
    };




    /**
    template <typename T> inline 
    typename std::enable_if<!std::is_pointer<T>::value,
    std::unique_ptr<T>>::type get_test_data( size_t n, size_t m) {
        // T is not a pointer
        std::cout << "not pointer";
        std::unique_ptr<T> data (new T[n]);
        return data;
    }
    template <typename T> inline 
    typename std::enable_if<std::is_pointer<T>::value,
    std::unique_ptr<T>>::type get_test_data( size_t n, size_t m) {
        // T is a pointer
        std::cout << " pointer ";
        std::unique_ptr<T> data (new T[n]);
        return data;
    }
    */

/*
    template <> inline std::vector<std::string> get_test_data( size_t n, size_t m ){

        std::vector<std::string> data;
        data.reserve(n);

        static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz"
                                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        std::random_device rd;
        std::default_random_engine rng(rd());
        std::uniform_int_distribution<> dist(0,sizeof(alphabet)/sizeof(*alphabet)-2);
        std::uniform_int_distribution<> string_length(5,30);

        std::generate_n(std::back_inserter(data), data.capacity(),   [&] {
                std::string str;
                size_t N = string_length(rng);
                str.reserve(N);
                std::generate_n(std::back_inserter(str), N, [&]() {
                                return alphabet[dist(rng)];
                            });
                  return str;
                  });
        return data;
    }*/
}


// IMPLEMENTATION DETAIL
namespace h5::mock::tuple {
    template <class T, class E=void> T data( const size_t size ){
        using element_t = typename h5::impl::decay<T>::type;
        using type = typename std::conditional<true, int, T>::type;
        if constexpr ( std::is_pod<T>::value )
            std::cout << " +" << size;
        else
            std::cout << " -" << size;
        //auto data = h5::utils::get_test_data<element_t>( size );
        return T();
    }


    // tail case
    template <std::size_t N, class Tuple, size_t K = std::tuple_size<Tuple>::value, class... Fields>
    typename std::enable_if< N == 0,
    Tuple>::type build(const std::array<size_t,K>& size, std::tuple<Fields...> fields ){

        using object_t = typename std::tuple_element<N,Tuple>::type;
        if constexpr( std::is_array<object_t>::value ){
            return std::tuple<object_t>();
        }else
            return std::tuple_cat(
                std::make_tuple( data<object_t>( N ) ), fields);
    }
    //pumping 
    template <std::size_t N, class Tuple, size_t K = std::tuple_size<Tuple>::value, class... Fields>
    typename std::enable_if< std::isgreater(N,0),
    Tuple>::type build(const std::array<size_t,K>& size, std::tuple<Fields...> fields ){
        using object_t = typename std::tuple_element<N,Tuple>::type;
        return build<N-1, Tuple>( size,
                std::tuple_cat(std::make_tuple( data<object_t>( N ) ), fields ));
    }

}

// EXPOSED API
namespace h5::mock {
    /** RANDU
     * generates a uniformly distributed random sequence 
     * with lower and upper bound
     */
    template <class T, size_t N>
    inline std::array<T,N> randu( size_t min, size_t max){
        std::random_device rd;
        std::default_random_engine rng(rd());
        std::uniform_int_distribution<> dist(min,max);

        std::array<T,N> data;
        std::generate_n(data.begin(), N, [&]() {
                                return dist(rng);
                            });
        return data;
    }
/*
    template <class T> T dispatch( T v ){

        if constexpr ( impl::not_supported<T>::value ) // enum + union
            std::cout << " en";
        else if constexpr ( impl::is_reference<T>::value ) // pointers + references
            std::cout << " re";
        else if constexpr ( std::is_same<T, std::string>::value )
            v = utils::get_test_data<T>();
        else if constexpr ( impl::is_container<T>::value ){
            std::cout << " st";
        } else if constexpr ( impl::is_iterable<T>::value ){
        //  using value_t = typename element_t::value_t;
            std::cout << " it";
        } else if constexpr ( impl::is_adaptor<T>::value ) {
            using container_t = typename T::container_type;
            using value_t = typename T::value_type;
            std::cout << " ad";

            if constexpr( impl::is_compare<T>::value ){
                ; //std::cout << " pqueue";
            } else
                return T( dispatch<container_t>(
                        container_t() ));
        } else if constexpr ( std::is_arithmetic<T>::value ) {
            v = utils::get_test_data<T>();
        } else if constexpr ( std::is_array<T>::value )
            std::cout << " ar";
        else if constexpr ( std::is_pod<T>::value )
            v = utils::get_test_data<T>();
        else
            std::cout << " na";
        return v;
    }

    template<class tuple_t, size_t N> tuple_t fill( tuple_t&& tuple, std::array<size_t, N>& size ){
        meta::static_for<tuple_t>( [&]( auto i ){
            using element_t = typename std::tuple_element<i,tuple_t>::type;
            if constexpr ( impl::is_container<tuple_t>::value )
                dispatch(
                    std::get<i>( tuple ) );
            else static_assert(!impl::is_container<tuple_t>::value, "not supprted object...");
        });

        return tuple;
    }
*/
    /** DATA
     * paramterised with a tuple type at compile time and
     * returns a tuple_t of objects with randomly generated test data
     * min - lower bound on count
     * max - upper bound
     * fill_rate - applicable to sparse matrices 
     */
    template<class tuple_t>
    tuple_t data(size_t min = 50, size_t max = 100, double fill_rate = 0.1 ){
        // compile time counter
        constexpr size_t N = std::tuple_size<tuple_t>::value;
        std::array<size_t,N> size = randu<size_t, N>(min,max);
        // RVO
        return fill(tuple_t(), size);
    }
}

#endif

