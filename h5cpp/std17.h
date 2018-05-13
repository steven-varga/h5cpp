


#if __cplusplus < 201703L
#include <type_traits>
#include <tuple>
#include <utility>
#include <functional>

namespace std {
//	template< class T >  constexpr bool is_member_function_pointer_v = std::is_member_function_pointer<T>::value;
}
// source: http://en.cppreference.com/w/cpp/utility/functional/invoke
namespace detail {
	template <class T> 	struct is_reference_wrapper : std::false_type {};
	template <class U> 	struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type {};
	template <class T>  constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;

	template <class T, class Type, class T1, class... Args>
	decltype(auto) INVOKE(Type T::* f, T1&& t1, Args&&... args){
//std::is_member_function_pointer<decltype(f)>::value 
		if( true ) {
			if(std::is_base_of<T, std::decay_t<T1>>::value )
				return (std::forward<T1>(t1).*f)(std::forward<Args>(args)...);
			else if  (is_reference_wrapper_v<std::decay_t<T1>>)
				return (t1.get().*f)(std::forward<Args>(args)...);
			else
				return ((*std::forward<T1>(t1)).*f)(std::forward<Args>(args)...);
		} else {
			//static_assert(std::is_member_object_pointer<decltype(f)>::value);
			//static_assert(sizeof...(args) == 0);
			if(std::is_base_of<T, std::decay_t<T1>>::value )
				return std::forward<T1>(t1).*f;
			else if(is_reference_wrapper_v<std::decay_t<T1>>)
				return t1.get().*f;
			else
				return (*std::forward<T1>(t1)).*f;
		}
	}

	template <class F, class... Args>
	decltype(auto) INVOKE(F&& f, Args&&... args) {
		  return std::forward<F>(f)(std::forward<Args>(args)...);
	}
} // namespace detail

namespace std {
	template< class F, class... Args>
	invoke_result_t<F, Args...> invoke(F&& f, Args&&... args) 
	  noexcept(std::is_nothrow_invocable_v<F, Args...>) {
		return detail::INVOKE(std::forward<F>(f), std::forward<Args>(args)...);
	}
}

/*source: http://en.cppreference.com/w/cpp/utility/apply */
namespace detail {
	template <class F, class Tuple, std::size_t... I>
	constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
	{
		return std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
	}
}  // namespace detail

namespace std {
template <class F, class Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t) {
		return detail::apply_impl(
			std::forward<F>(f), std::forward<Tuple>(t),
			std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
	}
}
#endif
