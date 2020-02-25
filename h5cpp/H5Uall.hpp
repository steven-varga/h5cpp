/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_UTIL_HPP
#define  H5CPP_UTIL_HPP
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



namespace h5 { namespace utils {

	template <typename T> inline T get_test_data(){
		return T();
	}

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
	template <> inline std::string get_test_data(){

		static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz"
										"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		std::random_device rd;
		std::default_random_engine rng(rd());
		std::uniform_int_distribution<> dist(0,sizeof(alphabet)/sizeof(*alphabet)-2);
		std::uniform_int_distribution<> string_length(5,30);

		std::string str;
		size_t N = string_length(rng);
		str.reserve(N);
		std::generate_n(std::back_inserter(str), N, [&]() {
			return alphabet[dist(rng)];
		});
		 return str;
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


// IMPLEMENTATION DETAIL
namespace h5::mock::tuple {
	template <class T, class E=void> T data( const size_t size ){
		using element_t = typename h5::impl::decay<T>::type;
		using type = typename std::conditional<true, int, T>::type;
		if constexpr ( std::is_pod<T>::value )
			std::cout << " +" << size;
		else
			std::cout << " -" << size;
		//auto data = h5::utils::get_test_data<element_t>( size );
		return T();
	}


	// tail case
	template <std::size_t N, class Tuple, size_t K = std::tuple_size<Tuple>::value, class... Fields>
	typename std::enable_if< N == 0,
	Tuple>::type build(const std::array<size_t,K>& size, std::tuple<Fields...> fields ){

		using object_t = typename std::tuple_element<N,Tuple>::type;
		if constexpr( std::is_array<object_t>::value ){
			return std::tuple<object_t>();
		}else
			return std::tuple_cat(
				std::make_tuple( data<object_t>( N ) ), fields);
	}
	//pumping 
	template <std::size_t N, class Tuple, size_t K = std::tuple_size<Tuple>::value, class... Fields>
	typename std::enable_if< std::isgreater(N,0),
	Tuple>::type build(const std::array<size_t,K>& size, std::tuple<Fields...> fields ){
		using object_t = typename std::tuple_element<N,Tuple>::type;
		return build<N-1, Tuple>( size,
				std::tuple_cat(std::make_tuple( data<object_t>( N ) ), fields ));
	}

}

namespace h5::meta::impl {
	template <size_t S,  class callable_t, size_t ...Is>
	constexpr void static_for( callable_t&& callback, std::integer_sequence<size_t, Is...> ){
		( callback( std::integral_constant<size_t, S + Is>{ } ),... );
	}
}
namespace h5::meta {
	// tuple_t -- tuple we are to iterate through
	// S -- start value
	// callable_t -- Callable object
	template <class tuple_t, size_t S = 0, class callable_t >
	constexpr void static_for( callable_t&& callback ){
		constexpr size_t N = std::tuple_size<tuple_t>::value;
		impl::static_for<S, callable_t>(
				std::forward<callable_t>(callback),
				std::make_integer_sequence<size_t, N - S>{ } );
	}
}


// EXPOSED API
namespace h5::mock {
	/** RANDU
	 * generates a uniformly distributed random sequence 
	 * with lower and upper bound
	 */
	template <class T, size_t N>
	inline std::array<T,N> randu( size_t min, size_t max){
		std::random_device rd;
		std::default_random_engine rng(rd());
		std::uniform_int_distribution<> dist(min,max);

		std::array<T,N> data;
		std::generate_n(data.begin(), N, [&]() {
								return dist(rng);
							});
		return data;
	}

	template <class T> T dispatch( T v ){

		if constexpr ( impl::not_supported<T>::value ) // enum + union
			std::cout << " en";
		else if constexpr ( impl::is_reference<T>::value ) // pointers + references
			std::cout << " re";
		else if constexpr ( std::is_same<T, std::string>::value )
			v = utils::get_test_data<T>();
		else if constexpr ( impl::is_container<T>::value ){
			std::cout << " st";
		} else if constexpr ( impl::is_iterable<T>::value ){
		//	using value_t = typename element_t::value_t;
			std::cout << " it";
		} else if constexpr ( impl::is_adaptor<T>::value ) {
			using container_t = typename T::container_type;
			using value_t = typename T::value_type;
			std::cout << " ad";

			if constexpr( impl::is_compare<T>::value ){
				; //std::cout << " pqueue";
			} else
				return T( dispatch<container_t>(
						container_t() ));
		} else if constexpr ( std::is_arithmetic<T>::value ) {
			v = utils::get_test_data<T>();
		} else if constexpr ( std::is_array<T>::value )
			std::cout << " ar";
		else if constexpr ( std::is_pod<T>::value )
			v = utils::get_test_data<T>();
		else
			std::cout << " na";
		return v;
	}

	template<class tuple_t, size_t N> tuple_t fill( tuple_t&& tuple, std::array<size_t, N>& size ){
		meta::static_for<tuple_t>( [&]( auto i ){
			using element_t = typename std::tuple_element<i,tuple_t>::type;
			if constexpr ( impl::is_container<tuple_t>::value )
				dispatch(
					std::get<i>( tuple ) );
			else static_assert(!impl::is_container<tuple_t>::value, "not supprted object...");
    	});

		return tuple;
	}

	/** DATA
	 * paramterised with a tuple type at compile time and
	 * returns a tuple_t of objects with randomly generated test data
	 * min - lower bound on count
	 * max - upper bound
	 * fill_rate - applicable to sparse matrices 
	 */
	template<class tuple_t>
	tuple_t data(size_t min = 50, size_t max = 100, double fill_rate = 0.1 ){
		// compile time counter
		constexpr size_t N = std::tuple_size<tuple_t>::value;
		std::array<size_t,N> size = randu<size_t, N>(min,max);
		// RVO
		return fill(tuple_t(), size);
	}
}

#endif

