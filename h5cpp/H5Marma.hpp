
/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_ARMA_HPP 
#define  H5CPP_ARMA_HPP

#if defined(ARMA_INCLUDES) || defined(H5CPP_USE_ARMADILLO)
namespace h5 { 	namespace arma {
		template<class T> using rowvec = ::arma::Row<T>;
		template<class T> using colvec = ::arma::Col<T>;
		template<class T> using colmat = ::arma::Mat<T>;
		template<class T> using cube   = ::arma::Cube<T>;
		template<class T> using sparse = ::arma::SpMat<T>;

		// is_linalg_type := filter
		template <class Object, class T = typename impl::decay<Object>::type> using is_supported =
		std::integral_constant<bool, std::is_same<Object,h5::arma::cube<T>>::value || std::is_same<Object,h5::arma::colmat<T>>::value
			|| std::is_same<Object,h5::arma::rowvec<T>>::value ||  std::is_same<Object,h5::arma::colvec<T>>::value>;
}}

namespace h5 { namespace impl {
	// 1.) object -> H5T_xxx

	template <class T> struct decay<h5::arma::rowvec<T>>{ typedef T type; };
	template <class T> struct decay<h5::arma::colvec<T>>{ typedef T type; };
	template <class T> struct decay<h5::arma::colmat<T>>{ typedef T type; };
	template <class T> struct decay<h5::arma::cube<T>>{ typedef T type; };
	template <class T> struct decay<h5::arma::sparse<T>>{ typedef T type; };

	// get read access to data store
	template <class Object, class T = typename impl::decay<Object>::type> inline
	typename std::enable_if< h5::arma::is_supported<Object>::value,
	const T*>::type data( const Object& ref ){
			return ref.memptr();
	}

	// read write access
	template <class Object, class T = typename impl::decay<Object>::type> inline
	typename std::enable_if< h5::arma::is_supported<Object>::value,
	T*>::type data( Object& ref ){
			return ref.memptr();
	}

	// rank
	template<class T> struct rank<h5::arma::rowvec<T>> : public std::integral_constant<size_t,1>{};
	template<class T> struct rank<h5::arma::colvec<T>> : public std::integral_constant<size_t,1>{};
	template<class T> struct rank<h5::arma::colmat<T>> : public std::integral_constant<size_t,2>{};
	template<class T> struct rank<h5::arma::cube<T>> : public std::integral_constant<size_t,3>{};
	template<class T> struct rank<h5::arma::sparse<T>> : public std::integral_constant<size_t,2>{};

	// determine rank and dimensions
	template <class T> inline std::array<size_t,1> size( const h5::arma::rowvec<T>& ref ){ return {ref.n_elem};}
	template <class T> inline std::array<size_t,1> size( const h5::arma::colvec<T>& ref ){ return {ref.n_elem};}
	template <class T> inline std::array<size_t,2> size( const h5::arma::colmat<T>& ref ){ return {ref.n_cols,ref.n_rows};}
	template <class T> inline std::array<size_t,3> size( const h5::arma::cube<T>& ref ){ return {ref.n_slices,ref.n_cols,ref.n_rows};}
	template <class T> inline std::array<size_t,2> size( const h5::arma::sparse<T>& ref ){ return {ref.n_cols,ref.n_rows};}


	// CTOR-s 
	template <class T> struct get<h5::arma::rowvec<T>> {
		static inline  h5::arma::rowvec<T> ctor( std::array<size_t,1> dims ){
			return h5::arma::rowvec<T>( dims[0] );
	}};
	template <class T> struct get<h5::arma::colvec<T>> {
		static inline h5::arma::colvec<T> ctor( std::array<size_t,1> dims ){
			return h5::arma::colvec<T>( dims[0] );
	}};
	template <class T> struct get<h5::arma::colmat<T>> {
		static inline h5::arma::colmat<T> ctor( std::array<size_t,2> dims ){
			return h5::arma::colmat<T>( dims[0], dims[1] );
	}};
	template <class T> struct get<h5::arma::cube<T>> {
		static inline h5::arma::colmat<T> ctor( std::array<size_t,3> dims ){
			return h5::arma::colmat<T>( dims[2], dims[0], dims[1] );
	}};
	template <class T> struct get<h5::arma::sparse<T>> {
		static inline h5::arma::sparse<T> ctor( const size_t* rowind, const size_t* colptr, const T* values, size_t n_rows, size_t n_cols ){
			//sp_mat(rowind, colptr, values, n_rows, n_cols)
			return h5::arma::sparse<T>(n_rows, n_cols);
	}};


	template <class T> struct is_multi <h5::arma::sparse<T>>: std::integral_constant<bool,true> {};
	template <class T> struct member <h5::arma::sparse<T>>{
		using type = std::tuple<h5::arma::rowvec<::arma::uword>, h5::arma::rowvec<::arma::uword>, h5::arma::rowvec<T>>;
		static constexpr size_t size = 3;
	};

	/* breaks up sparse mat into fields, and returns a tuple of pointers wrapped into arma::rowvec-s */
	template<class T>
	std::tuple<
		h5::arma::rowvec<::arma::uword>, // row_index
		h5::arma::rowvec<::arma::uword>, // col_ptrs
	   	h5::arma::rowvec<T> // values
	> get_fields( const arma::sparse<T>& sp ){
		sp.sync(); // the row_indes, col_ptrs and values are only valid after sync() is called, see SpMat_bones.hpp
		return std::make_tuple(
			h5::arma::rowvec<::arma::uword>((::arma::uword*)sp.row_indices, sp.n_nonzero + 1, true, false),
			h5::arma::rowvec<::arma::uword>((::arma::uword*)sp.col_ptrs, sp.n_cols + 2, true, false),
			h5::arma::rowvec<T>((T*) sp.values, sp.n_nonzero + 1, true, false)
			);
	}
	/* breaks up sparse mat into fields, and returns a tuple of pointers wrapped into arma::rowvec-s */
	template<class T>
	std::tuple<
		h5::arma::rowvec<::arma::uword>, // row_index
		h5::arma::rowvec<::arma::uword>, // col_ptrs
	   	h5::arma::rowvec<T> // values
	> get_fields( arma::sparse<T>& sp ){
		sp.sync(); // the row_indes, col_ptrs and values are only valid after sync() is called, see SpMat_bones.hpp
		return std::make_tuple(
			h5::arma::rowvec<::arma::uword>((::arma::uword*)sp.row_indices, sp.n_nonzero + 1, true, false),
			h5::arma::rowvec<::arma::uword>((::arma::uword*)sp.col_ptrs, sp.n_cols + 2, true, false),
			h5::arma::rowvec<T>((T*) sp.values, sp.n_nonzero + 1, true, false)
			);
	}

	// by default these names are used as data set names
	template<class T>
	constexpr std::tuple<const char*,const char*,const char*>
	get_field_names( const arma::sparse<T>& sp ){
		return {"row-index", "col-ptrs","values"};
	}
	template<class T> constexpr std::tuple<
		const char*, const char *, const char*, const char *,
		const char*, unsigned size_t, const char*, unsigned size_t >
	get_field_attributes( const arma::sparse<T>& sp ){
		return {
			"format", "CSC",
			"type","sparse matrix csc",
		  	"rows",sp.n_rows, "cols", sp.n_cols};
	}


}}
#endif
#endif
