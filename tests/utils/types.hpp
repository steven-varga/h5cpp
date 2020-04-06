/*
 * Copyright (c) 2019 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef  H5CPP_TYPE_HPP
#define  H5CPP_TYPE_HPP

#include <complex>
#include <array>
#include <vector>
#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <any>
//#include <h5cpp/utility>

namespace h5::test {
	template <class T>
	struct separator_t : std::integral_constant<size_t,3> {
		T type;
	};

	template <class T> struct is_separator : std::false_type {};
	template <class T> struct is_separator<separator_t<T>> : std::true_type {};

    struct pod_t {
      int    a;
      float  b;
      double c;
    };


	/*y axis are element types*/
	using numerical_t = std::tuple<
		unsigned char, unsigned short, unsigned int, unsigned long long int,
		char, short, int, long long int, float, double>;
	using other_t = std::tuple<pod_t>;
	using element_t = h5::impl::cat<numerical_t, other_t>::type;

	template <class T, size_t N=1> using array_t = std::tuple< // c like array, same memory size 
		T, T[N*N*N*N*N],    T[N*N*N][N*N],   T[N*N][N][N*N], T[N][N][N][N*N], T[N][N][N][N][N]>;

	template <class T, size_t N=1> using sequential_t = std::tuple<
		std::array<T, N>, std::array<std::vector<T>,N>,
		std::vector<T>, std::vector<std::array<T,N>>,std::vector<std::vector<T>>>;

	template <class T, class K=T, class V=T> using iterable_t = std::tuple<
		std::deque<T>, std::list<T>, std::forward_list<T>,
		std::set<T>, std::multiset<T>,
		std::map<K,V>, std::multimap<K,V>,
		std::unordered_set<T>, std::unordered_multiset<T>,
		std::unordered_map<K,V>, std::unordered_multimap<K, V>>;

	template <class T> using adaptor_t = std::tuple<
		std::stack<T, std::deque<T>>, std::stack<T, std::vector<T>>, std::stack<T, std::list<T>>,
		std::queue<T, std::deque<T>>, std::queue<T, std::list<T>>,
		std::priority_queue<T, std::deque<T>>, std::priority_queue<T, std::vector<T>>>;


	template <class T=int, class K=T, class V=T, size_t N=1>
	using stl_t = typename h5::impl::cat<
		sequential_t<T,N>, iterable_t<T,K,V>, adaptor_t<T>>::type;

	template <class T = int, size_t N = 1>
	using contigous_t = typename  h5::impl::cat<
		array_t<T,N>, sequential_t<T,N>>::type;


	template <class T>
	using linalg_t = typename h5::impl::cat<
		armadillo_t<T>,	eigen_t<T>, blitz_t<T>, blaze_t<T>, dlib_t<T>, itpp_t<T>, ublas_t<T>
		>::type;

	template <class T=int, class K=T, class V=T, size_t N=1>
	using all_t = typename h5::impl::cat<
		array_t<T,N>, stl_t<T,K,V,N>, linalg_t<T>>::type;

	template <class T>
	std::vector<std::string> h5_names(){
		std::vector<std::string> data;
		h5::meta::static_for<T>( [&]( auto i ) {
			data.push_back( h5::name<std::tuple_element_t<i,T>>::value);
		});
		return data;
	}
}

/*the type descriptor for pods*/
#include "pod_t.hpp"
#endif
