/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#define H5CPP_WRITE_DISPATCH_TEST_(T, msg){ \
    std::cout << "macro: " << msg << "\n"; \
}; \




#include <armadillo>

#define H5CPP_USE_ARMADILLO

#include "utils/abstract.hpp"
#include "../h5cpp/H5Dcreate.hpp"
#include "../h5cpp/H5Dopen.hpp"
#include "../h5cpp/H5Dwrite.hpp"
#include "../h5cpp/H5Marma.hpp"


namespace ns = h5::test;
template <typename T> struct H5Marma : public TestWithOpenHDF5<T> {};
template <typename T> struct H5Marma_write : public TestWithOpenHDF5<T> {};
template <typename T> struct H5Marma_read : public TestWithOpenHDF5<T> {};
typedef ::testing::Types<short> element_t;
//,int,long, unsigned short, unsigned int, unsigned long, float,double> element_t;
TYPED_TEST_SUITE(H5Marma, element_t, ns::element_names_t);
TYPED_TEST_SUITE(H5Marma_write, element_t, ns::element_names_t);
TYPED_TEST_SUITE(H5Marma_read, element_t, ns::element_names_t);

H5D_layout_t get_layout(const h5::ds_t& ds ){
    h5::dcpl_t dcpl{H5Dget_create_plist(ds)};
    return H5Pget_layout(dcpl);
}
/*
// cartesian product of [element_t X all_t]
TYPED_TEST(H5Marma, is_linalg) {
    using cartesian_product =  h5::test::armadillo::all_t<TypeParam>;
    h5::meta::static_for<cartesian_product>( [&]( auto i ){
        using T = typename std::tuple_element<i, cartesian_product>::type;
        ASSERT_TRUE(h5::impl::is_linalg<T>::value);
    });
}

// cartesian product of [element_t X all_t]
TYPED_TEST(H5Marma, has_data) {
    using cartesian_product =  h5::test::armadillo::all_t<TypeParam>;
    h5::meta::static_for<cartesian_product>( [&]( auto i ){
        using T = typename std::tuple_element<i, cartesian_product>::type;
        ASSERT_FALSE(h5::meta::has_data<T>::value);
    });
}

// cartesian product of [element_t X all_t]
TYPED_TEST(H5Marma, has_iterator) {
    using cartesian_product =  h5::test::armadillo::all_t<TypeParam>;
    h5::meta::static_for<cartesian_product>( [&]( auto i ){
        using T = typename std::tuple_element<i, cartesian_product>::type;
        ASSERT_TRUE(h5::meta::has_iterator<T>::value);
    });
}

// cartesian product of [element_t X all_t]
TYPED_TEST(H5Marma, is_contiguous) {
    using dense_product  = h5::test::armadillo::dense_t<TypeParam>;
    using sparse_product = h5::test::armadillo::sparse_t<TypeParam>;
    h5::meta::static_for<dense_product>( [&]( auto i ){
        using T = typename std::tuple_element<i, dense_product>::type;
        ASSERT_TRUE(h5::impl::is_contiguous<T>::value);
    });
    h5::meta::static_for<sparse_product>( [&]( auto i ){
        using T = typename std::tuple_element<i, sparse_product>::type;
        ASSERT_FALSE(h5::impl::is_contiguous<T>::value);
    });
}
// cartesian product of [element_t X all_t]
TYPED_TEST(H5Marma, decay) {
    using dense_product  = h5::test::armadillo::all_t<TypeParam>;
    h5::meta::static_for<dense_product>( [&]( auto i ){
        using T = typename std::tuple_element<i, dense_product>::type;
        using element_t = typename h5::impl::decay<T>::type;
        ::testing::StaticAssertTypeEq<element_t, TypeParam>();
    });
}

TYPED_TEST(H5Marma, rowvec_data) {
    arma::Row<TypeParam> object(10);
    ASSERT_EQ(h5::impl::data(object), object.memptr());
}

TYPED_TEST(H5Marma, colvec_data) {
    arma::Col<TypeParam> object(10);
    ASSERT_EQ(h5::impl::data(object), object.memptr());
}
TYPED_TEST(H5Marma, matrix_data) {
    arma::Mat<TypeParam> object(10,10);
    ASSERT_EQ(h5::impl::data(object), object.memptr());
}
TYPED_TEST(H5Marma, cube_data) {
    arma::Cube<TypeParam> object(10,10,10);
    ASSERT_EQ(h5::impl::data(object), object.memptr());
}
*/
TYPED_TEST(H5Marma_write, matrix){
	arma::mat M(2,4); std::iota(M.begin(), M.end(), 1);
	h5::ds_t ds = h5::create<short>(this->fd,this->name,
	    h5::current_dims{8,10}, h5::max_dims{8,H5S_UNLIMITED}, h5::chunk{2,4} | h5::fill_value<short>{80} |  h5::gzip{9});
	h5::write( ds, M, h5::count{1,8}); //, h5::offset{1,1}, h5::stride{1,2});
    ASSERT_TRUE(false);
}
/*
TYPED_TEST(H5Marma_write, matrix_fat){
	arma::mat M(2,4); std::iota(M.begin(), M.end(), 1);
	h5::write( this->fd, this->name, M,
        h5::offset{1,1}, h5::count{2,4},
        h5::current_dims{4,6}, h5::max_dims{60,H5S_UNLIMITED}, h5::chunk{4,2} | h5::fill_value<short>{80} |  h5::gzip{9} );
}
TYPED_TEST(H5Marma_write, matrix_tall){
	arma::mat M(2,4); std::iota(M.begin(), M.end(), 1);
	h5::write( this->fd, this->name, M, 
        h5::offset{1,1}, h5::count{4,2}, 
        h5::current_dims{6,4}, h5::max_dims{60,H5S_UNLIMITED}, h5::chunk{2,4} | h5::fill_value<short>{80} |  h5::gzip{9} );
}






TYPED_TEST(H5Marma_write, colvector){
    arma::Col<TypeParam> data(10); std::iota(data.begin(), data.end(), 1);
    h5::write(this->fd, this->name, data, h5::count{2}, h5::offset{4});
}
*/
/*
TYPED_TEST(H5Marma_write, colmat){
    arma::Mat<TypeParam> data(5,5); std::iota(data.begin(), data.end(), 1);
    h5::ds_t ds = h5::write(this->fd, this->name, data);
}
TYPED_TEST(H5Marma_write, cube){
    arma::Cube<TypeParam> data(2,3,4); std::iota(data.begin(), data.end(), 1);
    h5::ds_t ds = h5::write(this->fd, this->name, data);
}

TYPED_TEST(H5Marma_write, colvector_unlimited){
    arma::Col<TypeParam> data(10); std::iota(data.begin(), data.end(), 1);
    h5::ds_t ds = h5::write(this->fd, this->name, data,  h5::max_dims{H5S_UNLIMITED});
}
TYPED_TEST(H5Marma_write, colmat_unlimited){
    arma::Mat<TypeParam> data(5,5); std::iota(data.begin(), data.end(), 1);
    h5::ds_t ds = h5::write(this->fd, this->name, data, h5::max_dims{H5S_UNLIMITED,H5S_UNLIMITED});
}
TYPED_TEST(H5Marma_write, cube_unlimited){
    arma::Cube<TypeParam> data(2,3,4); std::iota(data.begin(), data.end(), 1);
    h5::ds_t ds = h5::write(this->fd, this->name, data, h5::max_dims{H5S_UNLIMITED,H5S_UNLIMITED,H5S_UNLIMITED});
}

*/

/*
TYPED_TEST(ArmadilloTest, ArmadilloWrite) {

	arma::Col<TypeParam>  C(10); 	 C.ones();
	arma::Row<TypeParam>  R(10);
	arma::Mat<TypeParam>  M(3,5);    M.ones();
	arma::Cube<TypeParam> Q(3,4,5);  Q.ones();
	// PLAIN
	h5::write(this->fd,this->name+".col", C);
	h5::write(this->fd,this->name+".row", R);

	h5::write(this->fd,this->name+".col unlim", C, h5::max_dims{H5S_UNLIMITED});
	h5::write(this->fd,this->name+".col unlim, gzip | chunk", C, h5::max_dims{H5S_UNLIMITED}, h5::gzip{9} | h5::chunk{10});
	h5::write(this->fd,this->name+".custom ", C, h5::count{1,10}, h5::current_dims{4,12}, h5::offset{2,2}, h5::max_dims{12,H5S_UNLIMITED} );
}

TYPED_TEST(ArmadilloTest, ReadArmadillo) {

	arma::Col<TypeParam>  C(3);        C.ones();
	arma::Row<TypeParam>  R(3);        R.ones();
	arma::Mat<TypeParam>  M(20,20);    for(int i=0; i < M.size(); i++ ) M[i] = i;
	arma::Cube<TypeParam> Q(3,5,7);    Q.ones();

	// PLAIN
	h5::write(this->fd,this->name+".col", C);
	h5::write(this->fd,this->name+".row", R);
	h5::write(this->fd,this->name+".mat", M);
	h5::write(this->fd,this->name+".cube", Q);
	{ // READ ENTIRE DATASET
	auto c = h5::read<arma::Col<TypeParam>>(this->fd,this->name+".col");
	auto r = h5::read<arma::Row<TypeParam>>(this->fd,this->name+".row");
	auto m = h5::read<arma::Mat<TypeParam>>(this->fd,this->name+".mat");
	auto q = h5::read<arma::Cube<TypeParam>>(this->fd,this->name+".cube");
	}
	{ //READ PARTIAL DATASET
	auto c = h5::read<arma::Col<TypeParam>>(this->fd,this->name+".col",h5::count{2}, h5::offset{1} );
	auto r = h5::read<arma::Row<TypeParam>>(this->fd,this->name+".row", h5::offset{0}, h5::count{3});
	arma::Mat<TypeParam> m(8,6);
	h5::read(this->fd,this->name+".mat", m.memptr(), h5::count{8,6}, h5::offset{2,3} );
	}
}

TYPED_TEST(ArmadilloTest, BlockReadArmadillo) {

	arma::Mat<TypeParam>  M(20,20);    for(int i=0; i < M.size(); i++ ) M[i] = i;
	arma::Mat<TypeParam> m(8,6);

	h5::write(this->fd,this->name+".mat", M);
	auto m_ = h5::read<arma::imat>(this->fd,this->name+".mat", h5::stride{3,3}, h5::count{2,2}, h5::block{2,2} );
	h5::read(this->fd,this->name+".mat", m.memptr(), h5::stride{3,3}, h5::count{2,2}, h5::block{2,2} );
}
*/

/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

