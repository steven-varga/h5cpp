/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_MISC_HPP
#define  H5CPP_MISC_HPP
namespace h5{
	using cx_double =  std::complex<double>; /**< scientific type */
	using cx_float = std::complex<float>;    /**< scientific type */
}




#define H5CPP_supported_elementary_types "supported elementary types ::= pod_struct | float | double |  [signed](int8 | int16 | int32 | int64)"

namespace h5 { namespace utils {
	template <class T>
	static constexpr bool is_supported = std::is_class<T>::value | std::is_arithmetic<T>::value;
	//static constexpr bool is_supported = std::is_pod<T>::value && std::is_class<T>::value | std::is_arithmetic<T>::value;
}}




namespace h5 { namespace utils {

	template <typename T> inline  std::vector<T> get_test_data( size_t n ){
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
// TODO: bang this so total memory alloc is same as 'n'
	template <> inline std::vector<std::string> get_test_data( size_t n ){

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
	}
}}
#endif

