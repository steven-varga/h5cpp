/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#define GOOGLE_STRIP_LOG 1

#include <gtest/gtest.h>
#include <armadillo>
#include <h5cpp/core>
#include <h5cpp/mock.hpp>
#include <h5cpp/create.hpp>
#include <h5cpp/read.hpp>
#include <h5cpp/write.hpp>

#include "event_listener.hpp"
#include "abstract.h"

template <typename T> class IntegralTest : public AbstractTest<T>{};
typedef ::testing::Types<float> PrimitiveTypes;
//typedef ::testing::Types<H5CPP_TEST_PRIMITIVE_TYPES> PrimitiveTypes;
TYPED_TEST_CASE(IntegralTest, PrimitiveTypes);

TYPED_TEST(IntegralTest,create_current_dims) { // checks out!!!
	h5::dcpl_t dcpl = h5::chunk{2} | h5::gzip{4};
	// compile error
	// h5::create<TypeParam>(this->fd, this->name + "ddapl, chunk, dlcpl");

	if( h5::create<TypeParam>(this->fd, this->name + "ddapl, chunk, dlcpl", h5::current_dims{10},
				h5::default_dapl, 	h5::chunk{3} , h5::default_lcpl  ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + "ddapl, chunk, lcpl", h5::current_dims{10},
				h5::default_dapl, h5::chunk{3} , h5::create_path  ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + "chunk, lcpl", h5::current_dims{10},
				h5::chunk{3}, h5::create_path  ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + "pdcpl, lcpl", h5::current_dims{10},
				dcpl, h5::create_path  ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " gzip | chunk, lcp, dapl", h5::current_dims{10}, h5::default_dapl,
				h5::chunk{3} | h5::gzip{3}, h5::default_lcpl, h5::default_dapl  ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " 2d", h5::current_dims{5,10} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " 3d", h5::current_dims{5,10,20} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " chunk", h5::current_dims{5,10,20}, h5::chunk{5,1,1} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " lcpl, chunk", h5::current_dims{5,10,20}, h5:: default_lcpl, h5::chunk{5,1,1}  ) < 0 ) ADD_FAILURE();
}

TYPED_TEST(IntegralTest,create_max_dims) {
	// compile time static_assert: current_dims or max_dims must be set
	//if( h5::create<TypeParam>(this->fd, this->name + " 1d") < 0 ) ADD_FAILURE();

	if( h5::create<TypeParam>(this->fd, this->name + " 1d", h5::max_dims{5}, h5::default_dcpl ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " 2d", h5::max_dims{5,H5S_UNLIMITED} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " 3d", h5::max_dims{5,10,20}, h5::chunk{5,1,1} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + "ddapl, chunk, dlcpl", h5::max_dims{10},
				h5::create_path | h5::utf8, h5::dapl, h5::chunk{2} | h5::gzip{3} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " from temp: chunk", h5::max_dims{50}, h5::chunk{5} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " from temp: chunk{5}|gzip{1}", h5::max_dims{53}, h5::chunk{5}|h5::gzip{1} ) < 0 ) ADD_FAILURE();
}

TYPED_TEST(IntegralTest,create_current_max_dims) { // checks out!!!

	h5::dcpl_t dcpl = h5::chunk{2} | h5::gzip{4} | h5::fill_value<TypeParam>( 42 ) | h5::nbit | h5::shuffle;
	if( h5::create<TypeParam>(this->fd, this->name + " no dcpl", h5::current_dims{10}, h5::max_dims{50}  ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " with dcpl explicit", h5::current_dims{10}, h5::max_dims{50}, dcpl  ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " with dcpl: chunk | nbit", h5::current_dims{10}, h5::max_dims{50}, 
			h5::chunk{2} | h5::nbit   ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " with dcpl: chunk", h5::current_dims{10}, h5::max_dims{H5S_UNLIMITED},
			h5::chunk{2}  ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " with dcpl: chunk, lcpl", h5::current_dims{10}, h5::max_dims{50},
			h5::chunk{2}, h5::default_lcpl	) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " with dcpl: chunk, lcpl, dapl", h5::current_dims{10}, h5::max_dims{50},
			h5::chunk{2}, h5::default_lcpl, h5::default_dapl	) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd, this->name + " with dcpl 2", 
			h5::current_dims{10}, h5::max_dims{50}, h5::default_lcpl, h5::chunk{5} | h5::gzip{9}  ) < 0 ) ADD_FAILURE();
}

TYPED_TEST(IntegralTest,create_write_dcpl) {
	std::array<TypeParam,120> arr{};
	for( auto& i:arr) i = 22;
	// compile error: count_t{ ... } must be present
	// if( h5::write<TypeParam>(this->fd, this->name + " 1d", arr.data() ) < 0 ) ADD_FAILURE();
	if( h5::write<TypeParam>(this->fd, this->name + " 1d", arr.data(), h5::count{50} ) < 0 ) ADD_FAILURE();
	if( h5::write<TypeParam>(this->fd, this->name + " 1d, chunk, max_dims", arr.data(), h5::count{50}, h5::chunk{5}, h5::max_dims{400} ) < 0 ) ADD_FAILURE();
	if( h5::write<TypeParam>(this->fd, this->name + " 1d, chunk|gzip|shuffle", arr.data(),
				h5::count{50}, h5::chunk{5} | h5::gzip{5} | h5::shuffle  ) < 0 ) ADD_FAILURE();
	if( h5::write<TypeParam>(this->fd, this->name + " count", arr.data(), h5::count{50} ) < 0 ) ADD_FAILURE();
	if( h5::write<TypeParam>(this->fd, this->name + " count max_dims{200}", arr.data(), h5::count{50}, h5::max_dims{200} ) < 0 ) ADD_FAILURE();
	if( h5::write<TypeParam>(this->fd, this->name + " count, max_dims{inf}", arr.data(), h5::count{50}, h5::max_dims{H5S_UNLIMITED} ) < 0 ) ADD_FAILURE();
//	can't merge properties inside funcion call, chunks must be specified if other dcpl properties are!!!
	if( h5::write<TypeParam>(this->fd, this->name + " count, max_dims{inf} gzip | nbit", arr.data(), h5::count{50}, h5::max_dims{H5S_UNLIMITED},
			   h5::chunk{5} | h5::gzip{4} | h5::nbit	) < 0 ) ADD_FAILURE();

//	if( h5::write<TypeParam>(this->fd, this->name + " count, UL gzip | nbit, dapl", arr.data(), h5::count{50} 
//			, h5::max_dims{H5S_UNLIMITED}
			  // h5::chunk{5} | h5::gzip{7} | h5::nbit, h5::default_dapl
//	) < 0 ) ADD_FAILURE();
}

TYPED_TEST(IntegralTest,create_write_dxpl) {
	std::array<TypeParam,120> arr{};
	for( auto& i:arr) i = 22;
	if( h5::write<TypeParam>(this->fd, this->name + " 1d hyper_vec", arr.data(), h5::count{50}, h5::hyper_vector_size{512} ) < 0 ) ADD_FAILURE();
	if( h5::write<TypeParam>(this->fd, this->name + " 1d hyper_vec | btreeratio", arr.data(), h5::count{50}, 
				h5::hyper_vector_size{512} | h5::btree_ratios{1.,1.,1.}) < 0 ) ADD_FAILURE();
}


TYPED_TEST(IntegralTest,create_write_multidim) {
	std::array<TypeParam,120> arr{};
	for( auto& i:arr) i = 22;
	// compile error: count_t{ ... } must be present
	// if( h5::write<TypeParam>(this->fd, this->name + " 1d", arr.data() ) < 0 ) ADD_FAILURE();
	if( h5::write<TypeParam>( 
				this->fd, this->name +  " 3d"            // path 
				,arr.data(), h5::count{5,10,1}            // in memory properties
				,h5::offset{0,0,0}, h5::stride{1,2,1}      // file props
				//, h5::chunk{5,10,1}
				, h5::max_dims{100,100,H5S_UNLIMITED}
			   	) < 0 ) ADD_FAILURE();
}

TYPED_TEST(IntegralTest,write_block) {
	std::array<TypeParam,1200> arr{};
	int j=1; for( auto& i:arr) i = j++;
	h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name + " 2d", h5::current_dims{100,100} );
	if( h5::write<TypeParam>(ds, arr.data()
				,h5::count{2,4},  h5::block{3,2}       // in memory properties
				,h5::offset{0,0}, h5::stride{4,3}      // stride >= block size or overlap happens
			   	) < 0 ) ADD_FAILURE();
}
TYPED_TEST(IntegralTest,create_write_block) {
	std::array<TypeParam,1200> arr{};
	int j=1; for( auto& i:arr) i = j++;
	//h5::ds_t ds = h5::create<TypeParam>(this->fd, this->name + " 2d", h5::current_dims{100,100} );
	if( h5::write<TypeParam>(this->fd, this->name + " 2d", arr.data()
				,h5::count{2,4},  h5::block{3,2}       // in memory properties
				,h5::offset{0,0}, h5::stride{4,3}      // stride >= block size or overlap happens
			   	) < 0 ) ADD_FAILURE();
}


TYPED_TEST(IntegralTest,read_enabled_if_collision) {
	std::vector<TypeParam> buf(10); const std::string ext = "enabled_if_collision";
	h5::write<TypeParam>( this->fd, this->name + ext,  buf.data(), h5::count{ buf.size() } );

	auto vec = h5::read<std::vector<TypeParam>>(this->fd, this->name + ext ); // copy elision
	h5::read(this->fd, this->name + ext, buf.data(), h5::count{5} );          // into pointer
	h5::read(this->fd, this->name + ext, buf );                               // into reference
}

TYPED_TEST(IntegralTest, read_copy_elision) {
	std::vector<TypeParam> buf(200); const std::string ext = "copy_elision";
	h5::write<TypeParam>( this->fd, this->name + ext,  buf.data(), h5::count{ buf.size() } );

	auto vec_01 = h5::read<std::vector<TypeParam>>(this->fd, this->name + ext );
	for(int i=0; i<buf.size(); i++) ASSERT_EQ(buf[i], vec_01[i] );

	auto vec_02 = h5::read<std::vector<TypeParam>>(this->fd, this->name + ext, h5::count{5}, h5::offset{2}, h5::stride{2} );
	ASSERT_EQ(vec_02.size(), 5 );
}

TYPED_TEST(IntegralTest, read_reference ) {
	std::vector<TypeParam> buf(20); const std::string ext = "reference";
	h5::write<TypeParam>( this->fd, this->name + ext,  buf.data(), h5::count{ buf.size() } );

	h5::read<std::vector<TypeParam>>(this->fd, this->name + ext, buf );
	// compile error: h5::count_t is present!!!
	//h5::read<std::vector<TypeParam>>(this->fd, this->name + ext, buf, h5::count{5} );
//FIXME: what does this mean???
	//	h5::read<std::vector<TypeParam>>(this->fd, this->name + ext, buf, h5::offset{2}, h5::stride{2}, h5::count{3} );
}

TYPED_TEST(IntegralTest, read_pointer ) {
	std::vector<TypeParam> buf(20); const std::string ext = "reference";
	h5::write<TypeParam>( this->fd, this->name + ext,  buf.data(), h5::count{ buf.size() } );

	h5::read<TypeParam>(this->fd, this->name + ext, buf.data(), h5::count{5} );
}



TYPED_TEST(IntegralTest, read_simple) {
	std::array<TypeParam,120> arr{}; for( auto& i:arr) i = 22;

	h5::write<TypeParam>( this->fd, this->name +  "cube",
		arr.data(), h5::count{5,10,1},
		h5::offset{0,0,0}, h5::stride{1,2,1}, h5::max_dims{100,100,H5S_UNLIMITED} );

	auto vec = h5::read<std::vector<TypeParam>>(this->fd, this->name + "cube");

}
/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

