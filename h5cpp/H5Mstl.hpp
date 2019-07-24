
/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

/**
 * @file This file contains meta functions and normal functions
 * to deal with c++ types. (e.g. to get the element type.)
 */

#ifndef  H5CPP_STL_HPP 
#define  H5CPP_STL_HPP

namespace h5::impl {
/*STL: */
	// 2.) filter is_xxx_type
	// 4.) write access
	// 5.) obtain dimensions of extents
	// 6.) ctor with right dimensions

	// 1.) object -> H5T_xxx
	/**
	 * @brief This meta function returns the element type of a class
	 *
	 * \note This is a customization point for new types.
	 *
	 * @ingroup customization-point
	 */
	template <class T, class...> struct decay{ typedef T type; };
	template <class T, class...Ts> using decay_t = typename decay<T, Ts...>::type;

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
	template< class T >
	inline constexpr bool is_scalar_v = std::is_integral_v<T> || std::is_pod_v<T> || std::is_same_v<T,std::string>;

	template <class T, class B = impl::decay_t<T>>
		using is_rank01 = std::bool_constant<
			std::is_same_v<T,std::initializer_list<B>> ||
			std::is_same_v<T,std::vector<B>> >;

	/**
	 * @brief This meta function returns the number of dimensions encapsulated
	 * in the given c++ type.
	 *
	 * \note This is a customization point for new types.
	 *
	 * @ingroup customization-point
	 */
	template<class T> struct rank : public std::integral_constant<size_t,0>{};
	template<> struct rank<std::string>: public std::integral_constant<size_t,1>{};
	template<class T> struct rank<T*>: public std::integral_constant<size_t,1>{};
	template<class T> struct rank<std::vector<T>>: public std::integral_constant<size_t,1>{};

	// 3.) read access
	/**
	 * @brief This function provides const read access to the data buffer
	 * of a type.
	 *
	 * The access is provided as a const pointer to the element type.
	 *
	 * \note This is a customization point for new types.
	 *
	 * @ingroup customization-point
	 */
	template <class T> inline std::enable_if_t<std::is_integral_v<T> || std::is_pod_v<T>, const T*>
		data( const T& ref ){ return &ref; }
	template<class T> inline std::enable_if_t< impl::is_scalar_v<T>, const T*>
		data( const std::initializer_list<T>& ref ){ return ref.begin(); }
	inline const char* const* data( const std::initializer_list<const char*>& ref ){ return ref.begin(); }
	inline const char* data( const std::string& ref ){ return ref.c_str(); }
	template <class T> inline const T* data( const std::vector<T>& ref ){
		return ref.data();
	}
	// 4.) write access
	/**
	 * @brief This function provides read / write access to the data buffer
	 * of a type.
	 *
	 * The access is provided as a pointer to the element type.
	 *
	 * \note This is a customization point for new types.
	 *
	 * @ingroup customization-point
	 */
	template <class T> inline T* data( std::vector<T>& ref ){
		return ref.data();
	}
	template <class T> inline std::enable_if_t<std::is_integral_v<T>, T*>
	data( T& ref ){ return &ref; }
	// 5.) obtain dimensions of extents
	/**
	 * @brief This function provides the sizes of the dimensions encapsulated in
	 * the given type.
	 *
	 * \note This is a customization point for new types.
	 *
	 * @ingroup customization-point
	 */
	template <class T> inline constexpr std::enable_if_t< impl::is_scalar_v<T>, std::array<size_t,0>>
		size( T value ){ return{}; }
	template <class T> inline std::enable_if_t< impl::is_rank01<T>::value, std::array<size_t,1>>
		size( const T& ref ){ return {ref.size()}; }
	// 6.) ctor with right dimensions
	/**
	 * @brief This function calls the constructor of a type initializing any
	 * encapsulated dimensions to the given sizes.
	 * @param dims Size of encapsulated dimensions
	 *
	 * \note This is a customization point for new types.
	 *
	 * @ingroup customization-point
	 */
	template <class T> struct get {
	   	static inline T ctor( std::array<size_t,impl::rank<T>::value> dims ){
			return T(); }};
	template<class T>
	struct get<std::vector<T>> {
		static inline std::vector<T> ctor( std::array<size_t,1> dims ){
			return std::vector<T>( dims[0] );
	}};
}
#endif
