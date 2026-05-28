/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#pragma once
#include <complex>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <iterator>
#include <type_traits>
#include <memory>
#include <cstdlib>

namespace h5 {
	using cx_double = std::complex<double>; /**< scientific type */
	using cx_float = std::complex<float>;    /**< scientific type */
}

#define H5CPP_supported_elementary_types "supported elementary types ::= pod_struct | enum | float | double |  [signed](int8 | int16 | int32 | int64)"
namespace h5::utils {
	// Scalar gate for the H5Dread/H5Awrite raw-pointer paths. Class types
	// (POD structs, wrappers like Celsius, opaque/precision-modified wrappers)
	// reach a dt_t<T> spec; arithmetic types and enums reach the native dt_t
	// path. The storage_representation layer routes enums as 'scalar', so we
	// admit them here too — without this, h5::read<std::vector<enum_t>> fails
	// the assert despite enum_t having a valid dt_t spec.
	template <class T> static constexpr bool is_supported =
		std::is_class_v<T> || std::is_arithmetic_v<T> || std::is_enum_v<T>;
	template <typename T> inline  std::vector<T> get_test_data( size_t n, size_t min, size_t max){
		std::random_device rd;
		std::default_random_engine rng(rd());
		std::uniform_int_distribution<> dist(0,n);

		std::vector<T> data;
		data.reserve(n);
		std::generate_n(std::back_inserter(data), n, [&]() {
			return dist(rng);
		});
		return data;
	}
	template <> inline std::vector<std::string> get_test_data( size_t n, size_t min, size_t max){
		std::vector<std::string> data;
		data.reserve(n);

		static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz" "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		std::random_device rd;
		std::default_random_engine rng(rd());
		std::uniform_int_distribution<> dist(0,sizeof(alphabet)/sizeof(*alphabet)-2);
		std::uniform_int_distribution<size_t> string_length(min, max);

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
	}
	template <typename T> inline  std::vector<T> get_test_data(size_t n){
		return get_test_data<T>(n, 0, n);
	}
	template <> inline std::vector<std::string> get_test_data( size_t n){

		std::vector<std::string> data;
		data.reserve(n);

		static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz"	"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		std::random_device rd;
		std::default_random_engine rng(rd());
		std::uniform_int_distribution<> dist(0,sizeof(alphabet)/sizeof(*alphabet)-2);
		std::uniform_int_distribution<> string_length(7, 17);
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
	}

}

namespace h5::impl {
    struct free {
        template <typename T>
        void operator()(T *p) const {
            using T_ = std::remove_const_t<T>;
            std::free( const_cast<T_*>(p) );
        }
    };
    template <typename T>
        using unique_ptr = std::unique_ptr<T, h5::impl::free>;
}
