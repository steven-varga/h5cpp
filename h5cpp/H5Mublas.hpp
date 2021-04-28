
/*
 * Copyright (c) 2018 - 2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef  H5CPP_UBLAS_HPP 
#define  H5CPP_UBLAS_HPP

#include <hdf5.h>
#include "H5Tmeta.hpp"
#include <tuple>
#include <type_traits>
#include <array>


#if defined(_BOOST_UBLAS_MATRIX_) || defined(H5CPP_USE_UBLAS_MATRIX)
namespace h5 {  namespace ublas {
        template<class T> using rowmat  = ::boost::numeric::ublas::matrix<T>;
        template <class Object, class T = typename impl::decay<Object>::type> 
            using is_supportedm = std::integral_constant<bool, std::is_same<Object,h5::ublas::rowmat<T>>::value>;

    using element_t = std::tuple<short,int,long, unsigned short, unsigned int, unsigned long, float,double,std::complex<float>,std::complex<double>>;
    template<class T> struct is_valid : // doesn't have to be fast, only used for testing
        std::integral_constant<bool, h5::meta::tpos<T,element_t>::present> {};
}}
namespace h5 { namespace impl {
    // 1.) object -> H5T_xxx
    template <class T> struct decay<h5::ublas::rowmat<T>>{ typedef T type; };
    template <class T> struct is_linalg<h5::ublas::rowmat<T>> : std::true_type {};

    // get read access to datastaore
    template <class Object, class T = typename impl::decay<Object>::type> inline
    typename std::enable_if< h5::ublas::is_supportedm<Object>::value,
    const T*>::type data(const Object& ref ){
            return ref.data().begin();
    }
    // read write access
    template <class Object, class T = typename impl::decay<Object>::type> inline
    typename std::enable_if< h5::ublas::is_supportedm<Object>::value,
    T*>::type data( Object& ref ){
            return ref.data().begin();
    }
    template<class T> struct rank<h5::ublas::rowmat<T>> : public std::integral_constant<size_t,2>{};
    template <class T> inline std::array<size_t,2> size( const h5::ublas::rowmat<T>& ref ){ return {(hsize_t)ref.size2(),(hsize_t)ref.size1()};}
    template <class T> struct get<h5::ublas::rowmat<T>> {
        static inline h5::ublas::rowmat<T> ctor( std::array<size_t,2> dims ){
            return h5::ublas::rowmat<T>( dims[1], dims[0] );
    }};
}}
#endif

#if defined(_BOOST_UBLAS_VECTOR_) || defined(H5CPP_USE_UBLAS_VECTOR)
namespace h5 {  namespace ublas {
        template<class T> using rowvec = ::boost::numeric::ublas::vector<T>;
        template <class Object, class T = typename impl::decay<Object>::type>
            using is_supportedv = std::integral_constant<bool, std::is_same<Object,h5::ublas::rowvec<T>>::value>;
}}
namespace h5 { namespace impl {
    template <class T> struct decay<h5::ublas::rowvec<T>>{ typedef T type; };
    template <class T> struct is_linalg<h5::ublas::rowvec<T>> : std::true_type {};

    template <class Object, class T = typename impl::decay<Object>::type> inline
    typename std::enable_if< h5::ublas::is_supportedv<Object>::value,
    const T*>::type data(const Object& ref ){
            return ref.data().begin();
    }
    template <class Object, class T = typename impl::decay<Object>::type> inline
    typename std::enable_if< h5::ublas::is_supportedv<Object>::value,
    T*>::type data( Object& ref ){
            return ref.data().begin();
    }
    template<class T> struct rank<h5::ublas::rowvec<T>> : public std::integral_constant<size_t,1>{};
    template <class T> inline std::array<size_t,1> size( const h5::ublas::rowvec<T>& ref ){ return { (hsize_t)ref.size() };}
    template <class T> struct get<h5::ublas::rowvec<T>> {
        static inline h5::ublas::rowvec<T> ctor( std::array<size_t,1> dims ){
            return h5::ublas::rowvec<T>( dims[0] );
    }};
}}
#endif
#endif
