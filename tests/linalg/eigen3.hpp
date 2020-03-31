/* Copyright (c) 2018-2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <Eigen/Dense>

namespace h5::test {
	template <class T, int N=4> using eigen_t = std::tuple<
		Eigen::Matrix<T, Eigen::Dynamic,1, Eigen::ColMajor>,
		Eigen::Matrix<T, 1, Eigen::Dynamic, Eigen::RowMajor>,
		Eigen::Matrix<T, Eigen::Dynamic, N, Eigen::ColMajor>,
		Eigen::Matrix<T, N,Eigen::Dynamic, Eigen::RowMajor>,

		Eigen::Array<T, Eigen::Dynamic,1, Eigen::ColMajor>,
		Eigen::Array<T, 1, Eigen::Dynamic, Eigen::RowMajor>,
		Eigen::Array<T, Eigen::Dynamic, N, Eigen::ColMajor>,
		Eigen::Array<T, N, Eigen::Dynamic, Eigen::RowMajor>,

		Eigen::Matrix<T, N, 1, Eigen::ColMajor>,
		Eigen::Matrix<T, 1, N, Eigen::RowMajor>,
		Eigen::Matrix<T, N, N, Eigen::ColMajor>,
		Eigen::Matrix<T, N, N, Eigen::RowMajor>,

		Eigen::Array<T, N, 1, Eigen::ColMajor>,
		Eigen::Array<T, 1, N, Eigen::RowMajor>,
		Eigen::Array<T, N, N, Eigen::ColMajor>,
		Eigen::Array<T, N, N, Eigen::RowMajor>,

		Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>,
		Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>,
		Eigen::Matrix<T, N, Eigen::Dynamic, Eigen::ColMajor>,
		Eigen::Matrix<T, Eigen::Dynamic, N, Eigen::RowMajor>,

		Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>,
		Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>,
		Eigen::Array<T, N, Eigen::Dynamic, Eigen::ColMajor>,
		Eigen::Array<T, Eigen::Dynamic, N, Eigen::RowMajor>
		>;

	template <class T> struct name <::Eigen::Matrix<T,Eigen::Dynamic,1,Eigen::ColMajor>> {
		static constexpr char const * value = "Eigen::Matrix<T,Dynamic,1,ColMajor>";
	};
	template <class T> struct name <::Eigen::Matrix<T,1,Eigen::Dynamic,Eigen::RowMajor>> {
		static constexpr char const * value = "Eigen::Matrix<T,1,Dynamic,RowMajor>";
	};
	template <class T, int N> struct name <::Eigen::Matrix<T,Eigen::Dynamic,N,Eigen::ColMajor>> {
		static constexpr char const * value = "Eigen::Matrix<T,Dynamic,N,ColMajor>";
	};
	template <class T, int N> struct name <::Eigen::Matrix<T,N,Eigen::Dynamic,Eigen::RowMajor>> {
		static constexpr char const * value = "Eigen::Matrix<T,N,Dynamic,RowMajor>";
	};

	template <class T> struct name <::Eigen::Array<T,Eigen::Dynamic,1,Eigen::ColMajor>> {
		static constexpr char const * value = "Eigen::Array<T,Dynamic,1,ColMajor>";
	};
	template <class T> struct name <::Eigen::Array<T,1,Eigen::Dynamic,Eigen::RowMajor>> {
		static constexpr char const * value = "Eigen::Array<T,1,Dynamic,RowMajor>";
	};
	template <class T, int N> struct name <::Eigen::Array<T,Eigen::Dynamic,N,Eigen::ColMajor>> {
		static constexpr char const * value = "Eigen::Array<T,Dynamic,N,ColMajor>";
	};
	template <class T, int N> struct name <::Eigen::Array<T,N,Eigen::Dynamic,Eigen::RowMajor>> {
		static constexpr char const * value = "Eigen::Array<T,N,Dynamic,RowMajor>";
	};

	template <class T, int N> struct name <::Eigen::Matrix<T,N,1,Eigen::ColMajor>> {
		static constexpr char const * value = "Eigen::Matrix<T,N,1,ColMajor>";
	};
	template <class T, int N> struct name <::Eigen::Matrix<T,1,N,Eigen::RowMajor>> {
		static constexpr char const * value = "Eigen::Matrix<T,1,N,RowMajor>";
	};
	template <class T, int N, int M> struct name <::Eigen::Matrix<T,N,M,Eigen::ColMajor>> {
		static constexpr char const * value = "Eigen::Matrix<T,N,M,ColMajor>";
	};
	template <class T, int N, int M> struct name <::Eigen::Matrix<T,N,M,Eigen::RowMajor>> {
		static constexpr char const * value = "Eigen::Matrix<T,N,M,RowMajor>";
	};

	template <class T, int N> struct name <::Eigen::Array<T,N,1,Eigen::ColMajor>> {
		static constexpr char const * value = "Eigen::Array<T,N,1,ColMajor>";
	};
	template <class T, int N> struct name <::Eigen::Array<T,1,N,Eigen::RowMajor>> {
		static constexpr char const * value = "Eigen::Array<T,1,N,RowMajor>";
	};
	template <class T, int N, int M> struct name <::Eigen::Array<T,N,M,Eigen::ColMajor>> {
		static constexpr char const * value = "Eigen::Array<T,N,M,ColMajor>";
	};
	template <class T, int N, int M> struct name <::Eigen::Array<T,N,M,Eigen::RowMajor>> {
		static constexpr char const * value = "Eigen::Array<T,N,M,RowMajor>";
	};


	template <class T> struct name <::Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic,Eigen::ColMajor>> {
		static constexpr char const * value = "Eigen::Matrix<T,Dynamic,Dynamic,ColMajor>";
	};
	template <class T> struct name <::Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor>> {
		static constexpr char const * value = "Eigen::Matrix<T,Dynamic,Dynamic,RowMajor>";
	};
	template <class T, int N> struct name <::Eigen::Matrix<T,N,Eigen::Dynamic,Eigen::ColMajor>> {
		static constexpr char const * value = "Eigen::Matrix<T,N,Dynamic,ColMajor>";
	};
	template <class T, int N> struct name <::Eigen::Matrix<T,Eigen::Dynamic,N,Eigen::RowMajor>> {
		static constexpr char const * value = "Eigen::Matrix<T,Dynamic,N,RowMajor>";
	};

	template <class T> struct name <::Eigen::Array<T,Eigen::Dynamic,Eigen::Dynamic,Eigen::ColMajor>> {
		static constexpr char const * value = "Eigen::Array<T,Dynamic,Dynamic,ColMajor>";
	};
	template <class T> struct name <::Eigen::Array<T,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor>> {
		static constexpr char const * value = "Eigen::Array<T,Dynamic,Dynamic,RowMajor>";
	};
	template <class T, int N> struct name <::Eigen::Array<T,N,Eigen::Dynamic,Eigen::ColMajor>> {
		static constexpr char const * value = "Eigen::Array<T,N,Dynamic,ColMajor>";
	};
	template <class T, int N> struct name <::Eigen::Array<T,Eigen::Dynamic,N,Eigen::RowMajor>> {
		static constexpr char const * value = "Eigen::Array<T,Dynamic,N,RowMajor>";
	};
}
