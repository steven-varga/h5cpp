
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

	template <class C, class T = typename impl::decay<C>::type> using is_dense =
	std::integral_constant<bool, std::is_same<C,::arma::Row<T>>::value || std::is_same<C,::arma::Col<T>>::value
		|| std::is_same<C,::arma::Mat<T>>::value ||  std::is_same<C,::arma::Cube<T>>::value>;

	template <class C, class T = typename impl::decay<C>::type> using is_sparse =
		std::integral_constant<bool, std::is_same<C,::arma::SpMat<T>>::value>;

	template <class C> using is_supported = typename std::integral_constant<bool,
		is_dense<C>::value || is_sparse<C>::value>;


	template <class T> using csc_t = // csr_names = {"row-index", "col-ptr","values"};
		std::tuple<rowvec<::arma::uword>, rowvec<::arma::uword>, rowvec<T>>;
	template<class T> using annotation_t =
		std::tuple<const char*,const char *,  const char*,std::array<unsigned size_t,2>>;

	template <class T> csc_t<T> csc2tuple( const sparse<T>& sp ){
		return std::make_tuple(
			h5::arma::rowvec<::arma::uword>((::arma::uword*)sp.row_indices, sp.n_nonzero, true, false),
			h5::arma::rowvec<::arma::uword>((::arma::uword*)sp.col_ptrs, sp.n_cols, true, false),
			h5::arma::rowvec<T>((T*) sp.values, sp.n_nonzero, true, false) );
	}
}}

namespace h5::impl {
	template <class T> struct decay<h5::arma::rowvec<T>>{ typedef T type; };
	template <class T> struct decay<h5::arma::colvec<T>>{ typedef T type; };
	template <class T> struct decay<h5::arma::colmat<T>>{ typedef T type; };
	template <class T> struct decay<h5::arma::cube<T>>{ typedef T type; };
	template <class T> struct decay<h5::arma::sparse<T>>{ typedef T type; };
}

namespace h5::exp::linalg {

	template <class C> class is_dense<C, typename std::enable_if< 
		h5::arma::is_dense<C>::value >::type> : public std::integral_constant<bool,true>{};
	template <class C> class is_sparse<C, typename std::enable_if<
		h5::arma::is_sparse<C>::value >::type> : public std::integral_constant<bool,true>{};

}

namespace h5 { namespace impl {
	// 1.) object -> H5T_xxx

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
		static inline h5::arma::cube<T> ctor( std::array<size_t,3> dims ){
			return h5::arma::cube<T>( dims[2], dims[0], dims[1] );
	}};


	template <class T> struct is_multi <h5::arma::sparse<T>>: std::integral_constant<bool,true> {};
	template <class T> struct member <h5::arma::sparse<T>>{
		using type = std::tuple<h5::arma::rowvec<::arma::uword>, h5::arma::rowvec<::arma::uword>, h5::arma::rowvec<T>>;
		static constexpr size_t size = 3;
	};

	template <class T> struct get<h5::arma::sparse<T>> {
		using tuple_t = typename h5::impl::member<T>::type;
		static inline h5::arma::sparse<T> ctor( arma::csc_t<T>& ref ){
			auto rowindx = std::get<0>(ref);
			auto colptr = std::get<1>(ref);
			auto values = std::get<2>(ref);
			size_t n_rows = rowindx.n_elem;
			size_t n_cols = colptr.n_elem;
			std::cout <<"idx: " << rowindx.n_elem << " colptr: " << colptr.n_elem  << " value: " << values.n_elem <<"\n";
			//sp_mat(rowind, colptr, values, n_rows, n_cols);
			return h5::arma::sparse<T>(3, 4);
		}

	};

	/* breaks up sparse mat into fields, and returns a tuple of pointers wrapped into arma::rowvec-s */
	template<class T>
	arma::csc_t<T> get_fields( const arma::sparse<T>& sp ){
		sp.sync(); // the row_indes, col_ptrs and values are only valid after sync() is called, see SpMat_bones.hpp
		return arma::csc2tuple(sp);
	}
	/* breaks up sparse mat into fields, and returns a tuple of pointers wrapped into arma::rowvec-s */
	template<class T>
	arma::csc_t<T> get_fields( arma::sparse<T>& sp ){
		sp.sync(); // the row_indes, col_ptrs and values are only valid after sync() is called, see SpMat_bones.hpp
		return arma::csc2tuple(sp);
	}


	template <class T> h5::arma::annotation_t<T>
	get_field_attributes( const arma::sparse<T>& sp ){
		return {
			"format", "csc",
		  	"shape",  std::array<unsigned size_t,2>({sp.n_rows, sp.n_cols }) };
	}
}}

namespace h5::exp {
	template <class T> struct rank<::arma::Row<T>> : public std::integral_constant<int,1> {};
	template <class T> struct rank<::arma::Col<T>> : public std::integral_constant<int,1> {};
	template <class T> struct rank<::arma::Mat<T>> : public std::integral_constant<int,2> {};
	template <class T> struct rank<::arma::Cube<T>> : public std::integral_constant<int,3> {};
	template <class T> struct rank<::arma::SpCol<T>> : public std::integral_constant<int,1> {};
	template <class T> struct rank<::arma::SpRow<T>> : public std::integral_constant<int,1> {};
	template <class T> struct rank<::arma::SpMat<T>> : public std::integral_constant<int,2> {};
}
#endif
#endif
