/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */


#ifndef  H5CPP_COMPAT_HPP
#define  H5CPP_COMPAT_HPP
//CREDITS: https://en.cppreference.com/w/cpp/utility/functional/invoke 

namespace compat { // C++11 shim to lower from c++17
	template <std::size_t ...> struct index_sequence{ };

	template <std::size_t N, std::size_t ... next>
	struct index_sequence_ : public index_sequence_<N-1U, N-1U, next...>{ };

	template <std::size_t ... next> struct index_sequence_<0U, next ... >{ 
		using type = index_sequence<next ... >;
	};

	template <std::size_t N>
	using make_index_sequence = typename index_sequence_<N>::type;

	template <class F, class Tuple, std::size_t... I>
	constexpr herr_t apply_impl( F&& f, Tuple&& t, compat::index_sequence<I...> ){
		return std::forward<F>(f)( std::get<I>(std::forward<Tuple>(t))...);
	}

	template <class F, class Tuple>
	constexpr herr_t apply(F&& f, Tuple&& t){
		using TP = typename std::decay<Tuple>::type;
		return apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
			compat::make_index_sequence<std::tuple_size<TP>::value>{});
	}
}


#endif

