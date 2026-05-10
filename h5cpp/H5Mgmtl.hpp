/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

#if defined(GMTL_MATRIX_H) || defined(H5CPP_USE_GMTL)
namespace h5::gmtl {
    template<class T, unsigned M, unsigned N> using matrix = ::gmtl::Matrix<T, M, N>;
    template<class T, unsigned N> using vec = ::gmtl::Vec<T, N>;
    template<class T, unsigned N> using point = ::gmtl::Point<T, N>;

    template <class Object, class T = typename impl::decay<Object>::type>
    using is_supported = std::bool_constant<
        std::is_same_v<Object, h5::gmtl::matrix<T, 3, 3>> ||
        std::is_same_v<Object, h5::gmtl::matrix<T, 4, 4>> ||
        std::is_same_v<Object, h5::gmtl::vec<T, 3>> ||
        std::is_same_v<Object, h5::gmtl::vec<T, 4>> ||
        std::is_same_v<Object, h5::gmtl::point<T, 3>> ||
        std::is_same_v<Object, h5::gmtl::point<T, 4>>
    >;
}

namespace h5::meta {
    template<class T, unsigned M, unsigned N> struct is_contiguous<h5::gmtl::matrix<T, M, N>> : std::true_type {};
    template<class T, unsigned N> struct is_contiguous<h5::gmtl::vec<T, N>> : std::true_type {};
    template<class T, unsigned N> struct is_contiguous<h5::gmtl::point<T, N>> : std::true_type {};
}

namespace h5::impl {
    template<class T, unsigned M, unsigned N> struct decay<h5::gmtl::matrix<T, M, N>>{ using type = T; };
    template<class T, unsigned N> struct decay<h5::gmtl::vec<T, N>>{ using type = T; };
    template<class T, unsigned N> struct decay<h5::gmtl::point<T, N>>{ using type = T; };

    template<class T, unsigned M, unsigned N>
    const T* data(const h5::gmtl::matrix<T, M, N>& ref) { return ref.getData(); }
    template<class T, unsigned M, unsigned N>
    T* data(h5::gmtl::matrix<T, M, N>& ref) { return const_cast<T*>(ref.getData()); }

    template<class T, unsigned N>
    const T* data(const h5::gmtl::vec<T, N>& ref) { return ref.getData(); }
    template<class T, unsigned N>
    T* data(h5::gmtl::vec<T, N>& ref) { return const_cast<T*>(ref.getData()); }

    template<class T, unsigned N>
    const T* data(const h5::gmtl::point<T, N>& ref) { return ref.getData(); }
    template<class T, unsigned N>
    T* data(h5::gmtl::point<T, N>& ref) { return const_cast<T*>(ref.getData()); }

    template<class T, unsigned M, unsigned N> struct rank<h5::gmtl::matrix<T, M, N>> : public std::integral_constant<size_t,2>{};
    template<class T, unsigned N> struct rank<h5::gmtl::vec<T, N>> : public std::integral_constant<size_t,1>{};
    template<class T, unsigned N> struct rank<h5::gmtl::point<T, N>> : public std::integral_constant<size_t,1>{};

    template<class T, unsigned M, unsigned N>
    inline std::array<size_t,2> size(const h5::gmtl::matrix<T, M, N>& ref) {
        return { M, N };
    }
    template<class T, unsigned N>
    inline std::array<size_t,1> size(const h5::gmtl::vec<T, N>& ref) {
        return { N };
    }
    template<class T, unsigned N>
    inline std::array<size_t,1> size(const h5::gmtl::point<T, N>& ref) {
        return { N };
    }

    template<class T, unsigned M, unsigned N> struct get<h5::gmtl::matrix<T, M, N>> {
        static inline h5::gmtl::matrix<T, M, N> ctor(std::array<size_t,2> dims) {
            return h5::gmtl::matrix<T, M, N>();
        }
    };
    template<class T, unsigned N> struct get<h5::gmtl::vec<T, N>> {
        static inline h5::gmtl::vec<T, N> ctor(std::array<size_t,1> dims) {
            return h5::gmtl::vec<T, N>();
        }
    };
    template<class T, unsigned N> struct get<h5::gmtl::point<T, N>> {
        static inline h5::gmtl::point<T, N> ctor(std::array<size_t,1> dims) {
            return h5::gmtl::point<T, N>();
        }
    };
}
#endif
