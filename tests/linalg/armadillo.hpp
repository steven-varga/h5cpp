/*
 * Copyright (c) 2018-2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <armadillo>
namespace h5::test::armadillo {
    template<class T> using dense_t  = std::tuple< ::arma::Col<T>, ::arma::Row<T>, ::arma::Mat<T>, ::arma::Cube<T>>;
    template<class T> using sparse_t = std::tuple<::arma::SpMat<T>>;
    template<class T> using all_t = typename h5::impl::cat< dense_t<T>, sparse_t<T>>::type;
}
namespace h5::test {
	template <class T> using armadillo_t = std::tuple<
			::arma::Col<T>,::arma::Row<T>, ::arma::Mat<T>, ::arma::SpMat<T>, ::arma::Cube<T>>;

	template <class T> struct name <::arma::Col<T>> {
		static constexpr char const * value = "arma::Col<T>";
	};
	template <class T> struct name <::arma::Row<T>> {
		static constexpr char const * value = "arma::Row<T>";
	};
	template <class T> struct name <::arma::Mat<T>> {
		static constexpr char const * value = "arma::Mat<T>";
	};
	template <class T> struct name <::arma::SpMat<T>> {
		static constexpr char const * value = "arma::SpMat<T>";
	};
	template <class T> struct name <::arma::Cube<T>> {
		static constexpr char const * value = "arma::Cube<T>";
	};
}
