/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

#if defined(OPENCV_CORE_HPP) || defined(H5CPP_USE_OPENCV)
#include <opencv2/core.hpp>

namespace h5::opencv {
    template <class Object>
    using is_supported = std::bool_constant<
        std::is_same_v<Object, cv::Mat> ||
        std::is_same_v<Object, cv::Mat_<float>> ||
        std::is_same_v<Object, cv::Mat_<double>> ||
        std::is_same_v<Object, cv::Mat_<int>> ||
        std::is_same_v<Object, cv::Mat_<short>> ||
        std::is_same_v<Object, cv::Mat_<unsigned char>>
    >;
}

namespace h5::meta {
    template<> struct is_contiguous<cv::Mat> : std::true_type {};
    template<typename T> struct is_contiguous<cv::Mat_<T>> : std::true_type {};
}

namespace h5::impl {
    template<> struct decay<cv::Mat>{ using type = void; };
    template<> struct decay<cv::Mat_<float>>{ using type = float; };
    template<> struct decay<cv::Mat_<double>>{ using type = double; };
    template<> struct decay<cv::Mat_<int>>{ using type = int; };
    template<> struct decay<cv::Mat_<short>>{ using type = short; };
    template<> struct decay<cv::Mat_<unsigned char>>{ using type = unsigned char; };

    inline unsigned char* data(cv::Mat& ref) { return ref.data; }
    inline const unsigned char* data(const cv::Mat& ref) { return ref.data; }
    template<typename T> inline T* data(cv::Mat_<T>& ref) { return reinterpret_cast<T*>(ref.data); }
    template<typename T> inline const T* data(const cv::Mat_<T>& ref) { return reinterpret_cast<const T*>(ref.data); }

    template<> struct rank<cv::Mat> : public std::integral_constant<size_t,2>{};
    template<typename T> struct rank<cv::Mat_<T>> : public std::integral_constant<size_t,2>{};

    inline std::array<size_t,2> size(const cv::Mat& ref) {
        return { (hsize_t)ref.rows, (hsize_t)ref.cols };
    }
    template<typename T>
    inline std::array<size_t,2> size(const cv::Mat_<T>& ref) {
        return { (hsize_t)ref.rows, (hsize_t)ref.cols };
    }

    template<> struct get<cv::Mat> {
        static inline cv::Mat ctor(std::array<size_t,2> dims) {
            return cv::Mat((int)dims[0], (int)dims[1], CV_8U);
        }
    };
    template<> struct get<cv::Mat_<float>> {
        static inline cv::Mat_<float> ctor(std::array<size_t,2> dims) {
            return cv::Mat_<float>((int)dims[0], (int)dims[1]);
        }
    };
    template<> struct get<cv::Mat_<double>> {
        static inline cv::Mat_<double> ctor(std::array<size_t,2> dims) {
            return cv::Mat_<double>((int)dims[0], (int)dims[1]);
        }
    };
    template<> struct get<cv::Mat_<int>> {
        static inline cv::Mat_<int> ctor(std::array<size_t,2> dims) {
            return cv::Mat_<int>((int)dims[0], (int)dims[1]);
        }
    };
    template<> struct get<cv::Mat_<short>> {
        static inline cv::Mat_<short> ctor(std::array<size_t,2> dims) {
            return cv::Mat_<short>((int)dims[0], (int)dims[1]);
        }
    };
    template<> struct get<cv::Mat_<unsigned char>> {
        static inline cv::Mat_<unsigned char> ctor(std::array<size_t,2> dims) {
            return cv::Mat_<unsigned char>((int)dims[0], (int)dims[1]);
        }
    };
}
#endif
