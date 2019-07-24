/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

/**
 * @file This contains template meta programming functions used in h5cpp.
 */

#ifndef  H5CPP_META_HPP 
#define  H5CPP_META_HPP

namespace h5::arg {
	// declaration
	/**
	 * @brief Meta function to determine whether T... contains a type
	 * convertible into 'const S&'.
	 *
	 * If S is not contained, the class will contain the members:
	 * \li present = false
	 * \li value = -1
	 *
	 * If S is contained, the class will contain the members:
	 * \li present = true
	 * \li type = the first type convertible 'into const S&'
	 * \li value = index of type in T...
	 */
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
			: tuple_pos<S, P+1,C, std::is_convertible_v<Head,S>, Head, std::tuple<Tail...>> { };
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

	/**
	 * @brief Return the first element of args convertible into const S& or the default value.
	 * @param def Default value
	 * @param args The arguments to search through.
	 * @return The first element of args convertible into const S& or the default value.
	 */
	template <class S, class... args_t,
			 class idx = tpos</*search: */const S&,/*args: */ const args_t&...,const S&>>
	typename idx::type& 
	get( const S& def, args_t&&... args ){
		auto tuple = std::forward_as_tuple(args..., def );
		return std::get<idx::value>( tuple );
	}

	/**
	 * @brief Returns the n-th argument passed to this function.
	 * @param args The arguments to index.
	 * @return The n-th argument passed to this function.
	 */
	template <int idx, class... args_t>
	std::tuple_element_t<idx, std::tuple<args_t...> >&
	getn(args_t&&... args ){
		auto tuple = std::forward_as_tuple(args...);
		return std::get<idx>( tuple );
	}
}

namespace h5 {
	/**
	 * @brief Type-name helper class for compile time id and printout.
	 *
	 * \note This is a customization point for new types.
	 *
	 * @ingroup customization-point
	 */
	template <class T> struct name {
		static constexpr char const * value = "n/a";
	};
}
#endif
