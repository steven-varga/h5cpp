/* Copyright (c) 2018-2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <blaze/Math.h>  // must precede blitz, due to interaction

namespace h5::test {
	template <class T> using blaze_t = std::tuple<
		blaze::DynamicVector<T,::blaze::rowVector>,
		blaze::DynamicVector<T,::blaze::columnVector>,
		blaze::DynamicMatrix<T,::blaze::rowMajor>,
		blaze::DynamicMatrix<T,::blaze::columnMajor>>;

	template <class T> struct name <::blaze::DynamicVector<T,::blaze::rowVector>> {
		static constexpr char const * value = "blaze::DynamicVector<T,rowVector>";
	};
	template <class T> struct name <::blaze::DynamicVector<T,::blaze::columnVector>> {
		static constexpr char const * value = "blaze::DynamicVector<T,columnVector>";
	};
	template <class T> struct name <::blaze::DynamicMatrix<T,::blaze::rowMajor>> {
		static constexpr char const * value = "blaze::DynamicVector<T,rowMajor>";
	};
	template <class T> struct name <::blaze::DynamicMatrix<T,::blaze::columnMajor>> {
		static constexpr char const * value = "blaze::DynamicVector<T,columnMajor>";
	};
}
