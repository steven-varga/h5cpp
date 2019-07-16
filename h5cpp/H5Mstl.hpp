
/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_STL_HPP 
#define  H5CPP_STL_HPP

namespace h5 { namespace impl {
/*STL: */
	// 2.) filter is_xxx_type
	// 4.) write access
	// 5.) obtain dimensions of extents
	// 6.) ctor with right dimensions

	// 1.) object -> H5T_xxx
	template <class T, class...> struct decay{ typedef T type; };

	template <class T> struct decay<const T>{ typedef T type; };
	template <class T> struct decay<const T*>{ typedef T* type; };
	template <class T> struct decay<std::basic_string<T>>{ typedef T* type; };
	template <class T, signed N> struct decay<const T[N]>{ typedef T* type; };
	template <class T, signed N> struct decay<T[N]>{ typedef T* type; };

	template <class T> struct decay<std::initializer_list<const T*>>{ typedef const T* type; };
	template <class T> struct decay<std::initializer_list<T*>>{ typedef T* type; };
	template <class T> struct decay<std::initializer_list<T>>{ typedef T type; };

	template <class T> struct decay<std::vector<const T*>>{ typedef const T* type; };
	template <class T> struct decay<std::vector<T*>>{ typedef T* type; };
	template <class T> struct decay<std::vector<T>>{ typedef T type; };

	// helpers
	template <class T>
		using is_scalar = std::integral_constant<bool,
			std::is_integral<T>::value || std::is_pod<T>::value || std::is_same<T,std::string>::value>;
	template <class T, class B = typename impl::decay<T>::type>
		using is_rank01 = std::integral_constant<bool,
			std::is_same<T,std::initializer_list<B>>::value || 
			std::is_same<T,std::vector<B>>::value >;

	template<class T> struct rank : public std::integral_constant<size_t,0>{};
	template<> struct rank<std::string>: public std::integral_constant<size_t,1>{};
	template<class T> struct rank<T*>: public std::integral_constant<size_t,1>{};
	template<class T> struct rank<std::vector<T>>: public std::integral_constant<size_t,1>{};

	// 3.) read access
	template <class T> inline typename std::enable_if<std::is_integral<T>::value || std::is_pod<T>::value,
		const T*>::type data( const T& ref ){ return &ref; }
	template<class T> inline typename std::enable_if< impl::is_scalar<T>::value,
		const T*>::type data( const std::initializer_list<T>& ref ){ return ref.begin(); }
	inline const char* const* data( const std::initializer_list<const char*>& ref ){ return ref.begin(); }
	inline const char* data( const std::string& ref ){ return ref.c_str(); }
	template <class T> inline const T* data( const std::vector<T>& ref ){
		return ref.data();
	}
	template <class T> inline T* data( std::vector<T>& ref ){
		return ref.data();
	}
	// 4.) write access
	template <class T> inline typename std::enable_if<std::is_integral<T>::value,
	T*>::type data( T& ref ){ return &ref; }
	// 5.) obtain dimensions of extents
	template <class T> inline constexpr typename std::enable_if< impl::is_scalar<T>::value,
		std::array<size_t,0>>::type size( T value ){ return{}; }
	template <class T> inline typename std::enable_if< impl::is_rank01<T>::value,
		std::array<size_t,1>>::type size( const T& ref ){ return {ref.size()}; }
	// 6.) ctor with right dimensions
	template <class T> struct get {
	   	static inline T ctor( std::array<size_t,impl::rank<T>::value> dims ){
			return T(); }};
	template<class T>
	struct get<std::vector<T>> {
		static inline std::vector<T> ctor( std::array<size_t,1> dims ){
			return std::vector<T>( dims[0] );
	}};
}}
#endif
