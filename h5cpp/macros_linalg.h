/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_LINALG_MACROS_H 
#define H5CPP_LINALG_MACROS_H


/**************************************************************************************************************************************/
/* BLAZE                                                                                                                              */
/**************************************************************************************************************************************/
#if defined(_BLAZE_MATH_MODULE_H_) || defined(H5CPP_USE_BLAZE)
	namespace blaze {
		template<class T> using rowmat = blaze::DynamicMatrix<T,blaze::rowMajor>;
		template<class T> using colmat = blaze::DynamicMatrix<T,blaze::columnMajor>;
		template<class T> using rowvec = blaze::DynamicVector<T,blaze::rowVector>;
		template<class T> using colvec = blaze::DynamicVector<T,blaze::columnVector>;
	}
/* definitions for armadillo containers */
	#define  H5CPP_BLAZE_TEMPLATE_SPEC(T) 																				             \
	H5CPP_BASE_TEMPLATE_SPEC(T,::blaze::rowvec, ref.data(), ref.size(), H5CPP_RANK_VEC,  {ref.size()} ) 					         \
	H5CPP_BASE_TEMPLATE_SPEC(T,::blaze::colvec, ref.data(), ref.size(), H5CPP_RANK_VEC,  {ref.size()} ) 					         \
	H5CPP_BASE_TEMPLATE_SPEC(T,::blaze::rowmat, ref.data(), ref.columns()*ref.rows(), H5CPP_RANK_MAT,  {ref.columns(), ref.rows()} ) \
	H5CPP_BASE_TEMPLATE_SPEC(T,::blaze::colmat, ref.data(), ref.columns()*ref.rows(), H5CPP_RANK_MAT,  {ref.rows(), ref.columns()} ) \
	H5CPP_CTOR_SPEC(T,::blaze::rowvec,  H5CPP_RANK_VEC,  (dims[0]) )													\
	H5CPP_CTOR_SPEC(T,::blaze::colvec,   H5CPP_RANK_VEC,  (dims[0]) )													\
	H5CPP_CTOR_SPEC(T,::blaze::rowmat,   H5CPP_RANK_MAT,  (dims[1],dims[0]) ) 											\
	H5CPP_CTOR_SPEC(T,::blaze::colmat,   H5CPP_RANK_MAT,  (dims[0],dims[1]) )											\

#else
	#define H5CPP_BLAZE_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif
/**************************************************************************************************************************************/
/* ARMADILLO                                                                                                                          */
/**************************************************************************************************************************************/
#if defined(ARMA_INCLUDES) || defined(H5CPP_USE_ARMADILLO)
/* definitions for armadillo containers */
	#define  H5CPP_ARMA_TEMPLATE_SPEC(T) 																				        \
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Row, ref.memptr(), ref.n_elem, H5CPP_RANK_VEC,  {ref.n_elem} ) 							\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Col, ref.memptr(), ref.n_elem, H5CPP_RANK_VEC,  {ref.n_elem} ) 							\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Mat, ref.memptr(), ref.n_elem, H5CPP_RANK_MAT,  {ref.n_cols, ref.n_rows} ) 				\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Cube,ref.memptr(), ref.n_elem, H5CPP_RANK_CUBE, {ref.n_slices, ref.n_cols, ref.n_rows} ) 	\
	H5CPP_CTOR_SPEC(T, arma::Row,  H5CPP_RANK_VEC,  (dims[0]) )															\
	H5CPP_CTOR_SPEC(T, arma::Col,  H5CPP_RANK_VEC,  (dims[0]) )															\
	H5CPP_CTOR_SPEC(T, arma::Mat,  H5CPP_RANK_MAT,  (dims[1],dims[0]) )													\
	H5CPP_CTOR_SPEC(T, arma::Cube, H5CPP_RANK_CUBE, (dims[2],dims[1],dims[0]) )											\

#else
	#define H5CPP_ARMA_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif

/**************************************************************************************************************************************/
/* EIGEN3: doesn use versioning                                                                                                       */
/**************************************************************************************************************************************/
#if defined(EIGEN_CORE_H) || defined(H5CPP_USE_EIGEN3)
	namespace eigen {
		template<class T> using amat 	= Eigen::Array<T,Eigen::Dynamic,Eigen::Dynamic>;
		template<class T> using arowvec = Eigen::Array<T,1,Eigen::Dynamic>;
		template<class T> using acolvec = Eigen::Array<T,Eigen::Dynamic,1>;

		template<class T> using mat    = Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic>;
		template<class T> using rowvec = Eigen::Matrix<T,1,Eigen::Dynamic>;
		template<class T> using colvec = Eigen::Matrix<T,Eigen::Dynamic,1>;
	}
	#define H5CPP_EIGEN_TEMPLATE_SPEC(T) \
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::rowvec, ref.data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size()} )						\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::colvec, ref.data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size()} )						\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::mat,    ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )	\
	H5CPP_CTOR_SPEC(T, eigen::rowvec,  H5CPP_RANK_VEC,  (dims[0]) )																        \
	H5CPP_CTOR_SPEC(T, eigen::colvec,  H5CPP_RANK_VEC,  (dims[0]) )																        \
	H5CPP_CTOR_SPEC(T, eigen::mat,     H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
	\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::arowvec, ref.data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size()} )						\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::acolvec, ref.data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size()} )						\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::amat,    ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	H5CPP_CTOR_SPEC(T, eigen::arowvec,  H5CPP_RANK_VEC,  (dims[0]) )																    \
	H5CPP_CTOR_SPEC(T, eigen::acolvec,  H5CPP_RANK_VEC,  (dims[0]) )																    \
	H5CPP_CTOR_SPEC(T, eigen::amat,     H5CPP_RANK_MAT,  (dims[1], dims[0]) )														    \

#else
	#define H5CPP_EIGEN_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif

/**************************************************************************************************************************************/
/* BOOST MATRIX                                                                                                                       */
/**************************************************************************************************************************************/
#if defined(_BOOST_UBLAS_MATRIX_) || defined(H5CPP_USE_UBLAS_MATRIX)
	namespace ublas {
		template<class T> using matrix 	= boost::numeric::ublas::matrix<T>;
	}
	#define H5CPP_UBLASM_TEMPLATE_SPEC(T) \
	H5CPP_BASE_TEMPLATE_SPEC(T, ::ublas::matrix, ref.data().begin(), ref.size1()*ref.size2(), H5CPP_RANK_MAT,  { (hsize_t) ref.size2(), (hsize_t) ref.size1()} )	\
	H5CPP_CTOR_SPEC(T, ::ublas::matrix,     H5CPP_RANK_MAT,  (dims[1], dims[0]) )														\

#else
	#define H5CPP_UBLASM_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif
#if defined(_BOOST_UBLAS_VECTOR_) || defined(H5CPP_USE_UBLAS_VECTOR)
	namespace ublas {
		template<class T> using vector 	= boost::numeric::ublas::vector<T>;
	}
	#define H5CPP_UBLASV_TEMPLATE_SPEC(T) \
	H5CPP_BASE_TEMPLATE_SPEC(T, ::ublas::vector, ref.data().begin(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size() } )	\
	H5CPP_CTOR_SPEC(T, ::ublas::vector,     H5CPP_RANK_VEC,  (dims[0]) ) 													\

#else
	#define H5CPP_UBLASV_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif
#define H5CPP_UBLAS_TEMPLATE_SPEC(T) H5CPP_UBLASM_TEMPLATE_SPEC(T) H5CPP_UBLASV_TEMPLATE_SPEC(T)


/**************************************************************************************************************************************/
/* IT++                                                                                                                               */
/**************************************************************************************************************************************/
#if defined(MAT_H) || defined(H5CPP_USE_ITPP_MATRIX)
	#define H5CPP_ITPPM_TEMPLATE_SPEC(T) \
	H5CPP_BASE_TEMPLATE_SPEC(T,itpp::Mat, ref._data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )	\
	H5CPP_CTOR_SPEC(T, itpp::Mat,     H5CPP_RANK_MAT,  (dims[1], dims[0]) )												\

#else
	#define H5CPP_ITPPM_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif
#if defined(VEC_H) || defined(H5CPP_USE_ITPP_VECTOR)
	#define H5CPP_ITPPV_TEMPLATE_SPEC(T)                                                                            \
	H5CPP_BASE_TEMPLATE_SPEC(T, itpp::Vec, ref._data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size() } )	    \
	H5CPP_CTOR_SPEC(T, itpp::Vec,     H5CPP_RANK_VEC,  (dims[0]) ) 													\

#else
	#define H5CPP_ITPPV_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif
#define H5CPP_ITPP_TEMPLATE_SPEC(T) H5CPP_ITPPM_TEMPLATE_SPEC(T) H5CPP_ITPPV_TEMPLATE_SPEC(T)




/**************************************************************************************************************************************/
/* BLITZ                                                                                                                              */
/**************************************************************************************************************************************/
#if defined(BZ_ARRAY_ONLY_H) || defined(H5CPP_USE_BLITZ)
	namespace blitz {
		template<class T> using vector 	= blitz::Array<T,1>;
		template<class T> using matrix 	= blitz::Array<T,2>;
		template<class T> using qube 	= blitz::Array<T,3>;
	}

	#define H5CPP_BLITZ_TEMPLATE_SPEC(T) \
	H5CPP_BASE_TEMPLATE_SPEC(T,::blitz::vector,ref.data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t)ref.size() } )	                    \
	H5CPP_BASE_TEMPLATE_SPEC(T,::blitz::matrix,ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t)ref.cols(), (hsize_t)ref.rows()} )	\
	H5CPP_BASE_TEMPLATE_SPEC(T,::blitz::qube, ref.data(),   ref.size(), H5CPP_RANK_CUBE, { (hsize_t)ref.depth(),(hsize_t)ref.cols(),(hsize_t)ref.rows()} )	\
	H5CPP_CTOR_SPEC(T, ::blitz::vector,   H5CPP_RANK_VEC,  (dims[0]) )												            \
	H5CPP_CTOR_SPEC(T, ::blitz::matrix,   H5CPP_RANK_MAT,  (dims[1], dims[0]) )												    \
	H5CPP_CTOR_SPEC(T, ::blitz::qube,     H5CPP_RANK_CUBE, (dims[2], dims[1], dims[0]) )										\

#else
	#define H5CPP_BLITZ_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif

/**************************************************************************************************************************************/
/* DLIB                                                                                                                              */
/**************************************************************************************************************************************/
#if defined(DLIB_MATRIx_HEADER) || defined(H5CPP_USE_DLIB)
	#define H5CPP_DLIB_TEMPLATE_SPEC(T) \
	H5CPP_BASE_TEMPLATE_SPEC(T,dlib::matrix, &ref(0,0), ref.size(), H5CPP_RANK_MAT,  { (hsize_t)ref.nc(), (hsize_t)ref.nr()} )	\
	H5CPP_CTOR_SPEC(T, dlib::matrix,   H5CPP_RANK_MAT,  (dims[1], dims[0]) )												\

#else
	#define H5CPP_DLIB_TEMPLATE_SPEC(T) 
#endif

#endif // include guard close
