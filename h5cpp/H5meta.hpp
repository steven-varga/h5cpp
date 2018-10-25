/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_META_HPP 
#define  H5CPP_META_HPP

namespace h5{ namespace arg {
	// declaration
	template<class S, class... T > struct tpos;
	namespace detail {
		// declaration
		template<class search_pattern, int position, int count, bool branch, class prev_head, class arguments> struct tuple_pos;
		// initialization case 
		template<class S, int P, int C, bool B, class not_used, class... Tail >
		struct tuple_pos<S, P,C, B, not_used, std::tuple<Tail...>>
			: tuple_pos<S, P,C, false, not_used, std::tuple<Tail...>> { };
		// recursive case 
		template<class S, int P, int C, class not_used, class Head, class... Tail >
		struct tuple_pos<S, P,C,false, not_used, std::tuple<Head, Tail...>>
			: tuple_pos<S, P+1,C, std::is_convertible<Head,S>::value, Head, std::tuple<Tail...>> { };
		// match case 
		template<class S, int P, int C, class Type, class... Tail >
		struct tuple_pos<S, P,C,true, Type, std::tuple<Tail...>>
			: std::integral_constant<int,P>{
			using type  = Type;
			static constexpr bool present = true;
		 };
		// default case
		template<class S, class H, int P, int C>
		struct tuple_pos<S, P,C, false, H, std::tuple<>>
			: std::integral_constant<int,-1>{
			static constexpr bool present = false;
		};
	}
	template<class S, class... args_t > struct tpos
		: detail::tuple_pos<const S&,-1,0,false, void, std::tuple<args_t...>>{ };

	template <class S, class... args_t,
			 class idx = tpos</*search: */const S&,/*args: */ const args_t&...,const S&>>
	typename idx::type& 
	get( const S& def, args_t&&... args ){
		auto tuple = std::forward_as_tuple(args..., def );
		return std::get<idx::value>( tuple );
	}
	template <int idx, class... args_t>
	typename std::tuple_element<idx, std::tuple<args_t...> >::type&
	getn(args_t&&... args ){
		auto tuple = std::forward_as_tuple(args...);
		return std::get<idx>( tuple );
	}
}}

namespace h5 { namespace impl {
	//<public domain code> borrowed from c++ library reference: to lower c+14 to c++11
	template<bool B, class T, class F> struct conditional { typedef T type; };
	template<class T, class F> struct conditional<false, T, F> { typedef F type; };
}}

namespace h5 {
	// type-name helper class for compile time id and printout 
	template <class T> struct name {
		static constexpr char const * value = "n/a";
	};

}
#endif
