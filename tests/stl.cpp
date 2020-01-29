/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#define GOOGLE_STRIP_LOG 1

#include <gtest/gtest.h>
#include <armadillo>
#include <h5cpp/core>

#include "event_listener.hpp"
#include "abstract.h"
#include <h5cpp/io>

template <typename T> class STLTest : public AbstractTest<T>{};
typedef ::testing::Types<H5CPP_TEST_STL_VECTOR_TYPES> StlTypes;

// instantiate for listed types
TYPED_TEST_CASE(STLTest, StlTypes);

TYPED_TEST(STLTest, stl_write_compact) {

	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(10);
	std::iota( std::begin(vec), std::end(vec), 1);
	// PLAIN: create + write
	auto ds = h5::write(this->fd,this->name, vec, h5::compact );
	h5::dcpl_t dcpl{ // get dcpl
		H5Dget_create_plist(static_cast<hid_t>(ds))};
	H5D_layout_t layout = H5Pget_layout(static_cast<hid_t>(dcpl));
	ASSERT_EQ(layout, H5D_COMPACT); // must be chunked layout
}
// 
TYPED_TEST(STLTest, stl_write_contigous) {

	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(10);
	std::iota( std::begin(vec), std::end(vec), 1);
	// PLAIN: create + write
	auto ds = h5::write(this->fd,this->name, vec);
	h5::dcpl_t dcpl{ // get dcpl
		H5Dget_create_plist(static_cast<hid_t>(ds))};
	H5D_layout_t layout = H5Pget_layout(static_cast<hid_t>(dcpl));
	ASSERT_EQ(layout, H5D_CONTIGUOUS); // must match layout
}

TYPED_TEST(STLTest, stl_write_chunked) {

	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(10);
	std::iota( std::begin(vec), std::end(vec), 1);
	// PLAIN: create + write
	auto ds = h5::write(this->fd,this->name, vec,  h5::stride{3}, h5::max_dims{H5S_UNLIMITED}  );
	h5::dcpl_t dcpl{ // get dcpl
		H5Dget_create_plist(static_cast<hid_t>(ds))};
	H5D_layout_t layout = H5Pget_layout(static_cast<hid_t>(dcpl));
	ASSERT_EQ(layout, H5D_CHUNKED); // must be chunked layout
}


TYPED_TEST(STLTest, stl_read_full) {
	using T = std::vector<TypeParam>;

	h5::fd_t fd = this->fd;
	std::string& dataset_name = this->name;

	T dset_01 = h5::utils::get_test_data<TypeParam>(100);
	h5::write(fd, dataset_name, dset_01);
	T dset_02 = h5::read<T>(fd, dataset_name );

	ASSERT_EQ(dset_01.size(), dset_02.size()); // must be same size
	for( size_t i=0; i < dset_01.size(); i++)  // same values
		ASSERT_EQ( dset_01[i], dset_02[i]);
}

TYPED_TEST(STLTest, stl_read_partial) {

	using T = std::vector<TypeParam>;
	const size_t N = 10, O = 7;
	h5::fd_t fd = this->fd;
	std::string& dataset_name = this->name;

	T dset_01 = h5::utils::get_test_data<TypeParam>(100);
	h5::write(fd, dataset_name, dset_01, h5::chunk{7});
	T dset_02 = h5::read<T>(fd, dataset_name, h5::count{N}, h5::offset{O});

	ASSERT_EQ(dset_02.size(), N); // must be same size
	for( size_t i=O; i < N; i++)  // same values
		ASSERT_EQ( dset_01[i + O], dset_02[i]);
}


TYPED_TEST(STLTest, stl_create_write_continous) {
	using T = std::vector<TypeParam>;
	h5::fd_t fd = this->fd;
	std::string& dataset_name = this->name;
	auto ds = h5::create<TypeParam>(fd, dataset_name, h5::current_dims{100});
	T dset_01 = h5::utils::get_test_data<TypeParam>(100);
	h5::write(ds, dset_01 );

	h5::dcpl_t dcpl{ // get dcpl
		H5Dget_create_plist(static_cast<hid_t>(ds))};
	H5D_layout_t layout = H5Pget_layout(static_cast<hid_t>(dcpl));
	ASSERT_EQ(layout, H5D_CONTIGUOUS); // must match layout
}


TYPED_TEST(STLTest, stl_create_write_dcpl) {
	using T = std::vector<TypeParam>;
	const size_t N = 100;
	std::vector<TypeParam> dset_01 = h5::utils::get_test_data<TypeParam>(N);
	std::vector<TypeParam> dset_02(N); // buffer for read 

	auto ds = h5::write(this->fd, this->name, dset_01, h5::chunk{2} | h5::gzip{4}, h5::current_dims{N});
	h5::read(ds, dset_02.data(), h5::count{N} ); // direct read into buffer size must be defined with h5::count{...}

	ASSERT_EQ(dset_01.size(), dset_02.size()); // must be same size
	for( size_t i=0; i < dset_01.size(); i++)  // same values
		ASSERT_EQ( dset_01[i], dset_02[i]);

}

TYPED_TEST(STLTest, stl_partial) {
	try {
		auto ds = h5::create<TypeParam>(this->fd, this->name, h5::current_dims{20,200,40}, h5::chunk{20,1,1} );
		std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(20);
		h5::write(ds, vec, h5::offset{0,0,0}, h5::count{1,20,1}, h5::stride{1,1,1} );
	} catch( const h5::error::any& err ){
		ASSERT_TRUE(false);
	}
}

TYPED_TEST(STLTest, stl_multidim_gzip) {
	// block write with offset 
	auto ds = h5::create<TypeParam>(this->fd, this->name, h5::current_dims{20,200,40}, h5::chunk{20,10,10} | h5::gzip{9} );
	std::vector<TypeParam> vec = h5::utils::get_test_data<TypeParam>(20);

	h5::write(ds, vec, h5::offset{2,3,0}, h5::count{4,5,1});
}


/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

