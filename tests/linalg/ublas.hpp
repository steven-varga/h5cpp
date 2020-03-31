/* Copyright (c) 2018-2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>


namespace h5::test {
	template <class T> using ublas_t = std::tuple<boost::numeric::ublas::vector<T>,boost::numeric::ublas::matrix<T>>;

	template <class T> struct name <::boost::numeric::ublas::vector<T>>{
		static constexpr char const * value = "ublas::vector<T>";
	};
	template <class T> struct name <::boost::numeric::ublas::matrix<T>>{
		static constexpr char const * value = "ublas::matrix<T>";
	};
}
