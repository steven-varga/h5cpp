/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#define GOOGLE_STRIP_LOG 1


#include <gtest/gtest.h>
#include "event_listener.hpp"

//testing private members
#define private public

#include "abstract.h"
#include <h5cpp/core>
	#include "struct.h"
#include <h5cpp/io>

/* packet table identifiers increment references twice
 *
 */  

template <typename T> class PacketTableTest : public AbstractTest<T>{};
typedef ::testing::Types<H5CPP_TEST_PACKET_TABLE_TYPES> PrimitiveTypes;
TYPED_TEST_CASE(PacketTableTest, PrimitiveTypes);

TYPED_TEST(PacketTableTest, copy_assign_operator) {
	h5::pt_t pt;
	ASSERT_FALSE( H5Iis_valid(pt.ds.handle));
	{
		auto ds = h5::create<TypeParam>(this->fd, this->name + "test copy assign ctor", h5::max_dims{H5S_UNLIMITED}, h5::chunk{20} | h5::gzip{9} );
		ASSERT_TRUE( H5Iis_valid(ds.handle));   // must be valid
		ASSERT_EQ( H5Iget_ref( ds.handle ), 1); // reference is not yet incremented
		pt = ds; // +1 for conversion ctor: pt(ds) and +1 for assignemnt 
		ASSERT_EQ( H5Iget_ref( pt.ds.handle ), 3);
	}
	ASSERT_EQ( H5Iget_ref( pt.ds.handle ), 2);  //  ds goes out of scope, decrements reference
	ASSERT_TRUE(H5Iis_valid(pt.ds.handle));     // pt_t handle must be still valid
}


TYPED_TEST(PacketTableTest, raii_ref_counting) {
	// obtain descriptor pt
	h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name + "test raii", h5::max_dims{H5S_UNLIMITED}, h5::chunk{20} | h5::gzip{9} );
	ASSERT_TRUE( H5Iis_valid( ds.handle ) );  // precondition
	int count_ds = H5Iget_ref( ds.handle );
	ASSERT_EQ(1, count_ds);
	{ // enclosed on purpose to TEST RAII
		h5::pt_t pt{ds};
		ASSERT_TRUE( H5Iis_valid( pt.ds.handle ) );   // this must be VALID
		int count_pt = H5Iget_ref( pt.ds.handle );
		ASSERT_EQ( count_ds + 2, count_pt );  // this must be VALID
	}
	// must be closed
	ASSERT_TRUE( H5Iis_valid( ds.handle ) );  // this must be invalid 
}



TYPED_TEST(PacketTableTest, copy_ctor ) {
	// obtain descriptor pt
	h5::pt_t pt = h5::create<TypeParam>(this->fd, this->name + "copy_ctor", h5::max_dims{H5S_UNLIMITED} );
	int count_0 = H5Iget_ref( pt.ds );
	h5::pt_t cp(pt);
	int count_1 = H5Iget_ref( cp.ds );
	ASSERT_EQ( count_0 + 2, count_1 );
	// deep copy, it has own cache
	ASSERT_NE(pt.ptr, cp.ptr );
}

TYPED_TEST(PacketTableTest, copy_init) {
	// obtain descriptor pt
	h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name + "copy_init", h5::max_dims{H5S_UNLIMITED} );
	h5::pt_t pt = ds;
	ASSERT_TRUE( H5Iis_valid( pt.ds ) );  // this must be VALID
	ASSERT_TRUE( pt.ptr != NULL);
	ASSERT_GT( pt.rank, 0);
   	ASSERT_GT( pt.element_size, 0 );

	for( int i=0; i < pt.rank;  i++ )
		ASSERT_GT( pt.chunk_dims[i], 0 );
	EXPECT_EQ( H5Iget_ref( pt.ds ), 3 );
}

TYPED_TEST(PacketTableTest, default_init) {
	// obtain descriptor pt
	h5::pt_t pt;
	ASSERT_FALSE( H5Iis_valid( pt.ds ) );  // this must be VALID
	ASSERT_TRUE( pt.ptr == NULL);
	ASSERT_EQ( pt.rank, 0);
   	ASSERT_EQ( pt.element_size, 0 );
}

TYPED_TEST(PacketTableTest, direct_init) {
	// obtain descriptor pt
	h5::pt_t pt( h5::create<TypeParam>(this->fd, this->name + "direct_init", h5::max_dims{H5S_UNLIMITED} ) );
	ASSERT_TRUE( H5Iis_valid( pt.ds ) );  // this must be VALID
	ASSERT_TRUE( pt.ptr != NULL);
	ASSERT_GT( pt.rank, 0);
   	ASSERT_GT( pt.element_size, 0 );
	for( int i=0; i < pt.rank;  i++ )
		ASSERT_GT( pt.chunk_dims[i], 0 );
	EXPECT_EQ( H5Iget_ref( pt.ds ), 2 );
}


TYPED_TEST(PacketTableTest, write_test) {
	auto stream = h5::utils::get_test_data<TypeParam>(200);
	// obtain descriptor pt
	h5::pt_t pt = h5::create<sn::StructType>(this->fd, this->name + "stream of struct",
			h5::max_dims{H5S_UNLIMITED}, h5::chunk{20} | h5::gzip{9} );
	for( auto record : stream )
			h5::append(pt, record);
}


/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

