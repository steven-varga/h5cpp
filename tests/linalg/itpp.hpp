/* Copyright (c) 2018-2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <itpp/itbase.h>

namespace h5::test {
	template <class T> using itpp_t = std::tuple<itpp::Vec<T>,itpp::Mat<T>>;

	template <class T> struct name <::itpp::Vec<T>> {
		static constexpr char const * value = "itpp::Vec<T>";
	};
	template <class T> struct name <::itpp::Mat<T>> {
		static constexpr char const * value = "itpp::Mat<T>";
	};
}
