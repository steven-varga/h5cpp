/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */


#ifndef  H5CPP_COMPAT_HPP
#define  H5CPP_COMPAT_HPP
//CREDITS: https://en.cppreference.com/w/cpp/utility/functional/invoke 

namespace h5::compat { // C++11 shim to lower from c++17
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

namespace h5::impl::compat {
// N4436 and https://en.cppreference.com/w/cpp/experimental/is_detected 

	struct nonesuch {
		nonesuch( ) = delete;
		~nonesuch( ) = delete;
		nonesuch( nonesuch const& ) = delete;
		void operator = ( nonesuch const& ) = delete;
	};

	template< class... > using void_t = void;
	namespace detail {
		template <class Default, class AlwaysVoid,
				  template<class...> class Op, class... Args>
		struct detector {
		  using value_t = std::false_type;
		  using type = Default;
		};

		template <class Default, template<class...> class Op, class... Args>
		struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
		  using value_t = std::true_type;
		  using type = Op<Args...>;
		};
	} // namespace detail

	template <template<class...> class Op, class... Args>
	using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

	template <template<class...> class Op, class... Args>
	using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

	template <class Default, template<class...> class Op, class... Args>
	using detected_or = detail::detector<Default, void, Op, Args...>;

	// helper templates
	template< template<class...> class Op, class... Args >
	constexpr bool is_detected_v = is_detected<Op, Args...>::value;
	template< class Default, template<class...> class Op, class... Args >
	using detected_or_t = typename detected_or<Default, Op, Args...>::type;
	template <class Expected, template<class...> class Op, class... Args>
	using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;
	template <class Expected, template<class...> class Op, class... Args>
	constexpr bool is_detected_exact_v = is_detected_exact<Expected, Op, Args...>::value;
	template <class To, template<class...> class Op, class... Args>
	using is_detected_convertible = std::is_convertible<detected_t<Op, Args...>, To>;
	template <class To, template<class...> class Op, class... Args>
	constexpr bool is_detected_convertible_v = is_detected_convertible<To, Op, Args...>::value;
}

#endif

