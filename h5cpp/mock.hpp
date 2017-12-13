/*
 * Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this  software  and associated documentation files (the "Software"), to deal in
 * the Software  without   restriction, including without limitation the rights to
 * use, copy, modify, merge,  publish,  distribute, sublicense, and/or sell copies
 * of the Software, and to  permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE  SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT  SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY  CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER
 * IN  AN  ACTION  OF  CONTRACT, TORT OR  OTHERWISE, ARISING  FROM,  OUT  OF  OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef  H5CPP_MOCK_H 
#define H5CPP_MOCK_H

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

