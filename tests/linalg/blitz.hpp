/* Copyright (c) 2018-2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <blitz/array.h>

namespace h5::test {
	template <class T> using blitz_t = std::tuple<
		blitz::Array<T,1>,blitz::Array<T,2>,blitz::Array<T,3>>;

	template <class T> struct name <::blitz::Array<T,1>> {
		static constexpr char const * value = "blitz::Array<T,1>";
	};
	template <class T> struct name <::blitz::Array<T,2>> {
		static constexpr char const * value = "blitz::Array<T,2>";
	};
	template <class T> struct name <::blitz::Array<T,3>> {
		static constexpr char const * value = "blitz::Array<T,3>";
	};
}
