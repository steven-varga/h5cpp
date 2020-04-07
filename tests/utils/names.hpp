/* Copyright (c) 2018-2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef  H5CPP_TEST_NAMES_HPP
#define  H5CPP_TEST_NAMES_HPP
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
namespace h5::test {
	template <> struct name<::h5::test::pod_t>{
		static constexpr char const * value = "pod_t";
	};
	template <class T> struct name<separator_t<T>> {
		static constexpr char const * value = "";
	};

	template <class T, size_t N> struct name<T[N]> {
		static constexpr char const * value = "T[i]";
	};
	template <class T, size_t I, size_t J> struct name<T[I][J]> {
		static constexpr char const * value = "T[i,j]";
	};
	template <class T, size_t I, size_t J, size_t K> struct name<T[I][J][K]> {
		static constexpr char const * value = "T[i,j,k]";
	};
	template <class T, size_t I, size_t J, size_t K, size_t M> struct name<T[I][J][K][M]> {
		static constexpr char const * value = "T[i,j,k,m]";
	};
	template <class T, size_t I, size_t J, size_t K, size_t M, size_t N> struct name<T[I][J][K][M][N]> {
		static constexpr char const * value = "T[i,j,k,m,n]";
	};

	template <class T, size_t N> struct name <std::array<T,N>> {
		static constexpr char const * value = "std::array<T,N>";
	};
	template <class T, size_t N> struct name <std::array<std::vector<T>,N>> {
		static constexpr char const * value = "std::array<std::vector<T>,N>";
	};
	template <class T> struct name <std::vector<T>> {
		static constexpr char const * value = "std::vector<T>";
	};
	template <class T> struct name <std::vector<std::vector<T>>> {
		static constexpr char const * value = "std::vector<std::vector<T>>";
	};
	template <class T, size_t N> struct name <std::vector<std::array<T,N>>> {
		static constexpr char const * value = "std::vector<std::array<T,N>>";
	};


	template <class T> struct name <std::deque<T>> {
		static constexpr char const * value = "std::deque<T>";
	};
	template <class T> struct name <std::forward_list<T>> {
		static constexpr char const * value = "std::forward_list<T>";
	};
	template <class T> struct name <std::list<T>> {
		static constexpr char const * value = "std::list<T>";
	};
	template <class T> struct name <std::set<T>> {
		static constexpr char const * value = "std::set<T>";
	};
	template <class T> struct name <std::multiset<T>> {
		static constexpr char const * value = "std::multiset<T>";
	};
	template <class K, class V> struct name <std::map<K,V>> {
		static constexpr char const * value = "std::map<K,V>";
	};
	template <class K, class V> struct name <std::multimap<K,V>> {
		static constexpr char const * value = "std::multimap<K,V>";
	};
	template <class T> struct name <std::unordered_set<T>> {
		static constexpr char const * value = "std::unordered_set<T>";
	};
	template <class T> struct name <std::unordered_multiset<T>> {
		static constexpr char const * value = "std::unordered_multiset<T>";
	};
	template <class K, class V> struct name <std::unordered_map<K,V>> {
		static constexpr char const * value = "std::unordered_map<K,V>";
	};
	template <class K, class V> struct name <std::unordered_multimap<K,V>> {
		static constexpr char const * value = "std::unordered_multimap<K,V>";
	};

	template <class T> struct name <std::stack<T, std::deque<T>>> {
		static constexpr char const * value = "std::stack<T,deque>";
	};
	template <class T> struct name <std::stack<T, std::vector<T>>> {
		static constexpr char const * value = "std::stack<T,vector>";
	};
	template <class T> struct name <std::stack<T, std::list<T>>> {
		static constexpr char const * value = "std::stack<T,list>";
	};


	template <class T> struct name <std::queue<T, std::deque<T>>> {
		static constexpr char const * value = "std::queue<T, deque>";
	};
	template <class T> struct name <std::queue<T, std::list<T>>> {
		static constexpr char const * value = "std::queue<T,list>";
	};

	template <class T> struct name <std::priority_queue<T, std::deque<T>>> {
		static constexpr char const * value = "std::priority_queue<T,deque>";
	};
	template <class T> struct name <std::priority_queue<T, std::list<T>>> {
		static constexpr char const * value = "std::priority_queue<T,list>";
	};
	template <class T> struct name <std::priority_queue<T, std::vector<T>>> {
		static constexpr char const * value = "std::priority_queue<T,vector>";
	};

	template <class T>
	std::vector<std::string> names(){
		std::vector<std::string> data;
		h5::meta::static_for<T>( [&]( auto i ) {
			data.push_back( name<std::tuple_element_t<i,T>>::value);
		});
		return data;
	}
	struct element_names_t {
		template <typename T> static std::string GetName(int i) {
		   return  h5::name<T>::value;
		 }
	};
}
namespace h5 {
	template <class T>
	std::vector<std::string> names(){
		std::vector<std::string> data;
		h5::meta::static_for<T>( [&]( auto i ) {
			data.push_back( h5::name<std::tuple_element_t<i,T>>::value);
		});
		return data;
	}
}
#endif
