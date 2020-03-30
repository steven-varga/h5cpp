/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#define GOOGLE_STRIP_LOG 1
#include <gtest/gtest.h>
#define private public
#define protected public
#include <h5cpp/core>
	#include "struct.h"
#include "event_listener.hpp"
#include "abstract.h"
#include <h5cpp/io>
#include <sstream>


template <typename T> class H5ATest : public AbstractTest<T>{};
typedef ::testing::Types<H5CPP_TEST_PRIMITIVE_TYPES> PrimitiveTypes;
TYPED_TEST_CASE(H5ATest, PrimitiveTypes);

/************************************
 * TEST CREATE
 ************************************/
TYPED_TEST(H5ATest,acreate_ds_t) {
	auto ds = h5::create<TypeParam>(this->fd, "/dataset/acreate/" + this->name, h5::current_dims{10});
	EXPECT_TRUE(H5Iis_valid(ds.handle));

	auto att = h5::acreate<TypeParam>(ds, this->name, h5::current_dims{10});
	EXPECT_TRUE(H5Iis_valid(att.handle));
	htri_t res = H5Aexists(static_cast<hid_t>(ds), this->name.data()); //Returns a positive value if attr_name exists.
	ASSERT_GT(res, 0);
}

TYPED_TEST(H5ATest,acreate_gr_t) {
	auto gr = h5::gcreate(this->fd, "/group/acreate/" + this->name);
	EXPECT_TRUE(H5Iis_valid(gr.handle));

	auto att = h5::acreate<TypeParam>(gr, this->name, h5::current_dims{10});
	EXPECT_TRUE(H5Iis_valid(att.handle));
	htri_t res = H5Aexists(static_cast<hid_t>(gr), this->name.data()); //Returns a positive value if attr_name exists.
	ASSERT_GT(res, 0);
}

TYPED_TEST(H5ATest,acreate_dt_t) {
	std::string name = "/type/acreate/" + this->name;
	{
		h5::dt_t<TypeParam> dt;
		EXPECT_TRUE(H5Iis_valid(dt.handle));
		h5::lcpl_t lcpl = h5::create_path;
		// commit datatype, so we can add attribute
		herr_t err = H5Tcommit( static_cast<hid_t>(this->fd), name.data(),
				static_cast<hid_t>(dt), static_cast<hid_t>(lcpl), H5P_DEFAULT, H5P_DEFAULT );
		// Returns a non-negative value if successful; otherwise returns a negative value.
		ASSERT_GT(err, -1);
	}
	h5::dt_t<TypeParam> dt{H5Topen( static_cast<hid_t>(this->fd), name.data(), H5P_DEFAULT )};
	EXPECT_TRUE(H5Iis_valid(dt.handle));

	auto att = h5::acreate<TypeParam>(dt, this->name, h5::current_dims{10});
	EXPECT_TRUE(H5Iis_valid(att.handle));
	htri_t res = H5Aexists(static_cast<hid_t>(dt), this->name.data()); //Returns a positive value if attr_name exists.
	ASSERT_GT(res, 0);
}

/************************************
 * TEST WRITE
 ************************************/
TYPED_TEST(H5ATest,awrite_ds_t) {
	TypeParam data;
	auto ds = h5::create<TypeParam>(this->fd, "/dataset/awrite/" + this->name, h5::current_dims{10});
	EXPECT_TRUE(H5Iis_valid(ds.handle));

	auto att = h5::acreate<TypeParam>(ds, this->name, h5::current_dims{10});
	EXPECT_TRUE(H5Iis_valid(att.handle));
	htri_t res = H5Aexists(static_cast<hid_t>(ds), this->name.data()); //Returns a positive value if attr_name exists.
	ASSERT_GT(res, 0);
}

TYPED_TEST(H5ATest,awrite_gr_t) {
	auto gr = h5::gcreate(this->fd, "/group/awrite/" + this->name);
	EXPECT_TRUE(H5Iis_valid(gr.handle));

	auto att = h5::acreate<TypeParam>(gr, this->name, h5::current_dims{10});
	EXPECT_TRUE(H5Iis_valid(att.handle));
	htri_t res = H5Aexists(static_cast<hid_t>(gr), this->name.data()); //Returns a positive value if attr_name exists.
	ASSERT_GT(res, 0);
}

TYPED_TEST(H5ATest,awrite_dt_t) {
	std::string name = "/type/awrite/" + this->name;
	{
		h5::dt_t<TypeParam> dt;
		EXPECT_TRUE(H5Iis_valid(dt.handle));
		h5::lcpl_t lcpl = h5::create_path;
		// commit datatype, so we can add attribute
		herr_t err = H5Tcommit( static_cast<hid_t>(this->fd), name.data(),
				static_cast<hid_t>(dt), static_cast<hid_t>(lcpl), H5P_DEFAULT, H5P_DEFAULT );
		// Returns a non-negative value if successful; otherwise returns a negative value.
		ASSERT_GT(err, -1);
	}
	h5::dt_t<TypeParam> dt{H5Topen( static_cast<hid_t>(this->fd), name.data(), H5P_DEFAULT )};
	EXPECT_TRUE(H5Iis_valid(dt.handle));

	auto att = h5::acreate<TypeParam>(dt, this->name, h5::current_dims{10});
	EXPECT_TRUE(H5Iis_valid(att.handle));
	htri_t res = H5Aexists(static_cast<hid_t>(dt), this->name.data()); //Returns a positive value if attr_name exists.
	ASSERT_GT(res, 0);
}



/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

