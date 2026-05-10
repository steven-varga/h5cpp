/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

#if defined(NEWMAT_H) || defined(H5CPP_USE_NEWMAT)
namespace h5::newmat {
    using matrix = ::NEWMAT::Matrix;
    using colvec = ::NEWMAT::ColumnVector;
    using rowvec = ::NEWMAT::RowVector;

    template <class Object>
    using is_supported = std::bool_constant<
        std::is_same_v<Object, ::NEWMAT::Matrix> ||
        std::is_same_v<Object, ::NEWMAT::ColumnVector> ||
        std::is_same_v<Object, ::NEWMAT::RowVector>
    >;
}

namespace h5::meta {
    template<> struct is_contiguous<::NEWMAT::Matrix> : std::true_type {};
    template<> struct is_contiguous<::NEWMAT::ColumnVector> : std::true_type {};
    template<> struct is_contiguous<::NEWMAT::RowVector> : std::true_type {};
}

namespace h5::impl {
    template<> struct decay<::NEWMAT::Matrix>{ using type = ::NEWMAT::Real; };
    template<> struct decay<::NEWMAT::ColumnVector>{ using type = ::NEWMAT::Real; };
    template<> struct decay<::NEWMAT::RowVector>{ using type = ::NEWMAT::Real; };

    inline const ::NEWMAT::Real* data(const ::NEWMAT::Matrix& ref) { return ref.data(); }
    inline ::NEWMAT::Real* data(::NEWMAT::Matrix& ref) { return ref.data(); }
    inline const ::NEWMAT::Real* data(const ::NEWMAT::ColumnVector& ref) { return ref.data(); }
    inline ::NEWMAT::Real* data(::NEWMAT::ColumnVector& ref) { return ref.data(); }
    inline const ::NEWMAT::Real* data(const ::NEWMAT::RowVector& ref) { return ref.data(); }
    inline ::NEWMAT::Real* data(::NEWMAT::RowVector& ref) { return ref.data(); }

    template<> struct rank<::NEWMAT::Matrix> : public std::integral_constant<size_t,2>{};
    template<> struct rank<::NEWMAT::ColumnVector> : public std::integral_constant<size_t,1>{};
    template<> struct rank<::NEWMAT::RowVector> : public std::integral_constant<size_t,1>{};

    inline std::array<size_t,2> size(const ::NEWMAT::Matrix& ref) {
        return { (hsize_t)ref.nrows(), (hsize_t)ref.ncols() };
    }
    inline std::array<size_t,1> size(const ::NEWMAT::ColumnVector& ref) {
        return { (hsize_t)ref.size() };
    }
    inline std::array<size_t,1> size(const ::NEWMAT::RowVector& ref) {
        return { (hsize_t)ref.size() };
    }

    template<> struct get<::NEWMAT::Matrix> {
        static inline ::NEWMAT::Matrix ctor(std::array<size_t,2> dims) {
            return ::NEWMAT::Matrix((int)dims[0], (int)dims[1]);
        }
    };
    template<> struct get<::NEWMAT::ColumnVector> {
        static inline ::NEWMAT::ColumnVector ctor(std::array<size_t,1> dims) {
            return ::NEWMAT::ColumnVector((int)dims[0]);
        }
    };
    template<> struct get<::NEWMAT::RowVector> {
        static inline ::NEWMAT::RowVector ctor(std::array<size_t,1> dims) {
            return ::NEWMAT::RowVector((int)dims[0]);
        }
    };
}
#endif
