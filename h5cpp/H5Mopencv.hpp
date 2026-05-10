/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

#if defined(OPENCV_CORE_HPP) || defined(H5CPP_USE_OPENCV)

namespace h5::meta {
    template <class T> struct is_contiguous<cv::Mat_<T>> : std::true_type {};
    template <class T, int N> struct is_contiguous<cv::Vec<T,N>> : std::true_type {};
    template <class T, int M, int N> struct is_contiguous<cv::Matx<T,M,N>> : std::true_type {};
}

namespace h5::impl {
    // decay
    template <class T> struct decay<cv::Mat_<T>>{ using type = T; };
    template <class T, int N> struct decay<cv::Vec<T,N>>{ using type = T; };
    template <class T, int M, int N> struct decay<cv::Matx<T,M,N>>{ using type = T; };

    // data
    template <class T> inline
    const T* data( const cv::Mat_<T>& ref ){
        return reinterpret_cast<const T*>(ref.data);
    }
    template <class T> inline
    T* data( cv::Mat_<T>& ref ){
        return reinterpret_cast<T*>(ref.data);
    }
    template <class T, int N> inline
    const T* data( const cv::Vec<T,N>& ref ){
        return ref.val;
    }
    template <class T, int N> inline
    T* data( cv::Vec<T,N>& ref ){
        return ref.val;
    }
    template <class T, int M, int N> inline
    const T* data( const cv::Matx<T,M,N>& ref ){
        return ref.val;
    }
    template <class T, int M, int N> inline
    T* data( cv::Matx<T,M,N>& ref ){
        return ref.val;
    }

    // rank
    template <class T> struct rank<cv::Mat_<T>> : public std::integral_constant<size_t,2>{};
    template <class T, int N> struct rank<cv::Vec<T,N>> : public std::integral_constant<size_t,1>{};
    template <class T, int M, int N> struct rank<cv::Matx<T,M,N>> : public std::integral_constant<size_t,2>{};

    // size
    template <class T> inline std::array<size_t,2> size( const cv::Mat_<T>& ref ){
        return {(size_t)ref.rows, (size_t)ref.cols};
    }
    template <class T, int N> inline std::array<size_t,1> size( const cv::Vec<T,N>& ref ){
        return {(size_t)N};
    }
    template <class T, int M, int N> inline std::array<size_t,2> size( const cv::Matx<T,M,N>& ref ){
        return {(size_t)M, (size_t)N};
    }

    // CTOR-s
    template <class T> struct get<cv::Mat_<T>> {
        static inline cv::Mat_<T> ctor( std::array<size_t,2> dims ){
            return cv::Mat_<T>( (int)dims[0], (int)dims[1] );
    }};
    template <class T, int N> struct get<cv::Vec<T,N>> {
        static inline cv::Vec<T,N> ctor( std::array<size_t,1> dims ){
            return cv::Vec<T,N>();
    }};
    template <class T, int M, int N> struct get<cv::Matx<T,M,N>> {
        static inline cv::Matx<T,M,N> ctor( std::array<size_t,2> dims ){
            return cv::Matx<T,M,N>();
    }};
}

// cv::Mat runtime dispatch overloads
namespace h5 {
    template <class... args_t>
    inline h5::ds_t write(const h5::ds_t& ds, const cv::Mat& mat, args_t&&... args) {
        if (CV_MAT_CN(mat.type()) != 1)
            throw std::runtime_error("h5::write(cv::Mat) requires single-channel cv::Mat");
        cv::Mat continuous = mat.isContinuous() ? mat : mat.clone();
        auto dispatch = [&](auto dummy) -> h5::ds_t {
            using T = decltype(dummy);
            cv::Mat_<T> mat_(continuous.rows, continuous.cols, reinterpret_cast<T*>(continuous.data));
            return h5::write(ds, mat_, std::forward<args_t>(args)...);
        };
        switch (CV_MAT_DEPTH(continuous.type())) {
            case CV_8U:  return dispatch(static_cast<uint8_t>(0));
            case CV_8S:  return dispatch(static_cast<int8_t>(0));
            case CV_16U: return dispatch(static_cast<uint16_t>(0));
            case CV_16S: return dispatch(static_cast<int16_t>(0));
            case CV_32S: return dispatch(static_cast<int32_t>(0));
            case CV_32F: return dispatch(static_cast<float>(0));
            case CV_64F: return dispatch(static_cast<double>(0));
            default: throw std::runtime_error("unsupported cv::Mat element type");
        }
    }

    template <class... args_t>
    inline h5::ds_t write(const h5::fd_t& fd, const std::string& dataset_path, const cv::Mat& mat, args_t&&... args) {
        if (CV_MAT_CN(mat.type()) != 1)
            throw std::runtime_error("h5::write(cv::Mat) requires single-channel cv::Mat");
        cv::Mat continuous = mat.isContinuous() ? mat : mat.clone();
        auto dispatch = [&](auto dummy) -> h5::ds_t {
            using T = decltype(dummy);
            cv::Mat_<T> mat_(continuous.rows, continuous.cols, reinterpret_cast<T*>(continuous.data));
            return h5::write(fd, dataset_path, mat_, std::forward<args_t>(args)...);
        };
        switch (CV_MAT_DEPTH(continuous.type())) {
            case CV_8U:  return dispatch(static_cast<uint8_t>(0));
            case CV_8S:  return dispatch(static_cast<int8_t>(0));
            case CV_16U: return dispatch(static_cast<uint16_t>(0));
            case CV_16S: return dispatch(static_cast<int16_t>(0));
            case CV_32S: return dispatch(static_cast<int32_t>(0));
            case CV_32F: return dispatch(static_cast<float>(0));
            case CV_64F: return dispatch(static_cast<double>(0));
            default: throw std::runtime_error("unsupported cv::Mat element type");
        }
    }
}

#endif
