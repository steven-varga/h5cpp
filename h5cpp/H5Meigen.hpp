/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_MEIGEN_HPP 
#define H5CPP_MEIGEN_HPP

/**************************************************************************************************************************************/
/* EIGEN3: doesn't use versioning                                                                                                       */
/**************************************************************************************************************************************/
#if defined(EIGEN_CORE_H) || defined(H5CPP_USE_EIGEN3)
	namespace eigen {
		template<class T> using arr_cm  = Eigen::Array<T,Eigen::Dynamic,Eigen::Dynamic,Eigen::ColMajor>;
		template<class T> using arr_rm  = Eigen::Array<T,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor>;
		template<class T> using a1xd_rm = Eigen::Array<T,1,Eigen::Dynamic, Eigen::RowMajor>;
		template<class T> using a2xd_rm = Eigen::Array<T,2,Eigen::Dynamic, Eigen::RowMajor>;
		template<class T> using a3xd_rm = Eigen::Array<T,3,Eigen::Dynamic, Eigen::RowMajor>;
		template<class T> using a4xd_rm = Eigen::Array<T,4,Eigen::Dynamic, Eigen::RowMajor>;
		template<class T> using adx1_rm = Eigen::Array<T,Eigen::Dynamic,1, Eigen::RowMajor>;
		template<class T> using adx2_rm = Eigen::Array<T,Eigen::Dynamic,2, Eigen::RowMajor>;
		template<class T> using adx3_rm = Eigen::Array<T,Eigen::Dynamic,3, Eigen::RowMajor>;
		template<class T> using adx4_rm = Eigen::Array<T,Eigen::Dynamic,4, Eigen::RowMajor>;

		template<class T> using adx1_cm = Eigen::Array<T,Eigen::Dynamic,1, Eigen::ColMajor>;
		template<class T> using adx2_cm = Eigen::Array<T,Eigen::Dynamic,2, Eigen::ColMajor>;
		template<class T> using adx3_cm = Eigen::Array<T,Eigen::Dynamic,3, Eigen::ColMajor>;
		template<class T> using adx4_cm = Eigen::Array<T,Eigen::Dynamic,4, Eigen::ColMajor>;
		template<class T> using a1xd_cm = Eigen::Array<T,1,Eigen::Dynamic, Eigen::ColMajor>;
		template<class T> using a2xd_cm = Eigen::Array<T,2,Eigen::Dynamic, Eigen::ColMajor>;
		template<class T> using a3xd_cm = Eigen::Array<T,3,Eigen::Dynamic, Eigen::ColMajor>;
		template<class T> using a4xd_cm = Eigen::Array<T,4,Eigen::Dynamic, Eigen::ColMajor>;

		template<class T> using mat_cm  = Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic,Eigen::ColMajor>;
		template<class T> using mat_rm  = Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor>;

		template<class T> using m1xd_rm = Eigen::Matrix<T,1,Eigen::Dynamic, Eigen::RowMajor>;
		template<class T> using m2xd_rm = Eigen::Matrix<T,2,Eigen::Dynamic, Eigen::RowMajor>;
		template<class T> using m3xd_rm = Eigen::Matrix<T,3,Eigen::Dynamic, Eigen::RowMajor>;
		template<class T> using m4xd_rm = Eigen::Matrix<T,4,Eigen::Dynamic, Eigen::RowMajor>;
		template<class T> using mdx1_rm = Eigen::Matrix<T,Eigen::Dynamic,1, Eigen::RowMajor>;
		template<class T> using mdx2_rm = Eigen::Matrix<T,Eigen::Dynamic,2, Eigen::RowMajor>;
		template<class T> using mdx3_rm = Eigen::Matrix<T,Eigen::Dynamic,3, Eigen::RowMajor>;
		template<class T> using mdx4_rm = Eigen::Matrix<T,Eigen::Dynamic,4, Eigen::RowMajor>;

		template<class T> using mdx1_cm = Eigen::Matrix<T,Eigen::Dynamic,1, Eigen::ColMajor>;
		template<class T> using mdx2_cm = Eigen::Matrix<T,Eigen::Dynamic,2, Eigen::ColMajor>;
		template<class T> using mdx3_cm = Eigen::Matrix<T,Eigen::Dynamic,3, Eigen::ColMajor>;
		template<class T> using mdx4_cm = Eigen::Matrix<T,Eigen::Dynamic,4, Eigen::ColMajor>;
		template<class T> using m1xd_cm = Eigen::Matrix<T,1,Eigen::Dynamic, Eigen::ColMajor>;
		template<class T> using m2xd_cm = Eigen::Matrix<T,2,Eigen::Dynamic, Eigen::ColMajor>;
		template<class T> using m3xd_cm = Eigen::Matrix<T,3,Eigen::Dynamic, Eigen::ColMajor>;
		template<class T> using m4xd_cm = Eigen::Matrix<T,4,Eigen::Dynamic, Eigen::ColMajor>;
	}

	#define H5CPP_EIGEN_TEMPLATE_SPEC(T) \
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::a1xd_rm, ref.data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size()} )						\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::a2xd_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::a3xd_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::a4xd_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::adx1_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::adx2_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::adx3_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::adx4_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::arr_rm,  ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::adx1_cm, ref.data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size()} )						\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::adx2_cm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::adx3_cm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::adx4_cm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::a2xd_cm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::a3xd_cm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::a4xd_cm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::arr_cm,  ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::m1xd_rm, ref.data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size()} )						\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::m2xd_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::m3xd_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::m4xd_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::mdx1_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::mdx2_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::mdx3_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::mdx4_rm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::mat_rm,  ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.rows(), (hsize_t) ref.cols()} )\
	\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::mdx1_cm, ref.data(), ref.size(), H5CPP_RANK_VEC,  { (hsize_t) ref.size()} )						\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::mdx2_cm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::mdx3_cm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::mdx4_cm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::m2xd_cm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::m3xd_cm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::m4xd_cm, ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	H5CPP_BASE_TEMPLATE_SPEC(T, eigen::mat_cm,  ref.data(), ref.size(), H5CPP_RANK_MAT,  { (hsize_t) ref.cols(), (hsize_t) ref.rows()} )\
	\
	H5CPP_CTOR_SPEC(T, eigen::adx2_rm, H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::adx3_rm, H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::adx4_rm, H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::a1xd_rm, H5CPP_RANK_VEC,  (dims[0]) )																        \
	H5CPP_CTOR_SPEC(T, eigen::a2xd_rm, H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::a3xd_rm, H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::a4xd_rm, H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::arr_rm,  H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::adx1_cm, H5CPP_RANK_VEC,  (dims[0]) )																        \
	H5CPP_CTOR_SPEC(T, eigen::adx2_cm, H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::adx3_cm, H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::adx4_cm, H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::a2xd_cm, H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::a3xd_cm, H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::a4xd_cm, H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::arr_cm,  H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
\
	H5CPP_CTOR_SPEC(T, eigen::mdx2_rm, H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::mdx3_rm, H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::mdx4_rm, H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::m1xd_rm, H5CPP_RANK_VEC,  (dims[0]) )																        \
	H5CPP_CTOR_SPEC(T, eigen::m2xd_rm, H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::m3xd_rm, H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::m4xd_rm, H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::mat_rm,  H5CPP_RANK_MAT,  (dims[0], dims[1]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::mdx1_cm, H5CPP_RANK_VEC,  (dims[0]) )																        \
	H5CPP_CTOR_SPEC(T, eigen::mdx2_cm, H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::mdx3_cm, H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::mdx4_cm, H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::m2xd_cm, H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::m3xd_cm, H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::m4xd_cm, H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
	H5CPP_CTOR_SPEC(T, eigen::mat_cm,  H5CPP_RANK_MAT,  (dims[1], dims[0]) )													        \
\

#else
	#define H5CPP_EIGEN_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif

#endif // include guard close
