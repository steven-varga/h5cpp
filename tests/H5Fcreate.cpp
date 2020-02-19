/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include "utils/types.hpp"
	using yaxis_t = element_t;
	template <class T>
	using xaxis_t = container_t<T>;
#include "utils/main.hpp"

#include "plot/all"
#include <cstdlib>
#include <random>
#include <fmt/core.h>
#include <fmt/format.h>



// Tests factorial of 0.
TEST(H5F, create_acc_trunc) {
	try{
		h5::fd_t fd = h5::create( "acc_trunc.h5", H5F_ACC_TRUNC);
		EXPECT_GT( static_cast<hid_t>( fd ), -1 );
	} catch ( const h5::error::any& err ){
		ADD_FAILURE();
	}
}

TEST(H5F, create_acc_exlc) {
	try{
		h5::fd_t fd = h5::create( "acc_excl.h5", H5F_ACC_EXCL);
		EXPECT_GT( static_cast<hid_t>( fd ), -1 );
	} catch ( const h5::error::any& err ){
		ADD_FAILURE();
	}
}
TEST(H5F, set_user_block) {
//H5Pset_userblock sets the user block size of a file creation property list. The default user block size is 0; it may be set to any power of 2 equal to 512 or greater (512, 1024, 2048, etc.).

	for( size_t block : {0, 2, 4, 8}){
		try{
			h5::fd_t fd = h5::create( "user_block.h5", H5F_ACC_TRUNC,  h5::userblock{block * 512});
			EXPECT_GT( static_cast<hid_t>( fd ), -1 );
		} catch ( const h5::error::any& err ){
			ADD_FAILURE();
		}
	}
}

TEST(H5F, set_sizes) {
//H5Pset_sizes sets the byte size of the offsets and lengths used to address objects in an HDF5 file. This function is only valid for file creation property lists. Passing in a value of 0 for one of the sizeof_... parameters retains the current value. The default value for both values is the same as sizeof(hsize_t) in the library (normally 8 bytes). Valid values currently are 2, 4, 8 and 16.
	size_t offset, length;
	for( size_t i : {2, 4, 8}) { //FIXME: fails with 16
		for( size_t j : {2, 4, 8, 16}) {
			h5::fd_t fd = h5::create( "set_sizes.h5", H5F_ACC_TRUNC, h5::sizes{i,j} );
			hid_t fcpl = H5Fget_create_plist( static_cast<hid_t>(fd) );
			H5Pget_sizes(fcpl, &offset, &length );
			EXPECT_GT( static_cast<hid_t>( fd ), -1 );
			EXPECT_EQ( offset, i); 	EXPECT_EQ( length, j);
			H5Pclose(fcpl);
		}
	}
}
TEST(H5F, set_sym_k) {
/*
H5Pset_sym_k sets the size of parameters used to control the symbol table nodes.
This function is valid only for file creation property lists. Passing in a value of zero (0) for one of the parameters retains the current value.
ik is one half the rank of a B-tree that stores a symbol table for a group. Internal nodes of the symbol table are on average 75% full. That is, the average rank of the tree is 1.5 times the value of ik. The HDF5 library uses (ik*2) as the maximum # of entries before splitting a B-tree node. Since only 2 bytes are used in storing # of entries for a B-tree node in an HDF5 file, (ik*2) cannot exceed 65536. The default value for ik is 16.
lk is one half of the number of symbols that can be stored in a symbol table node. A symbol table node is the leaf of a symbol table tree which is used to store a group. When symbols are inserted randomly into a group, the group's symbol table nodes are 75% full on average. That is, they contain 1.5 times the number of symbols specified by lk. The default value for lk is 4.
*/
	unsigned ik,lk;
	for( size_t i : {16, 32, 64}) {
		for( size_t j : {4, 8}) {
			h5::fd_t fd = h5::create( "set_sym_k.h5", H5F_ACC_TRUNC, h5::sym_k{i,j} );
			hid_t fcpl = H5Fget_create_plist( static_cast<hid_t>(fd) );
			H5Pget_sym_k(fcpl, &ik, &lk );
			EXPECT_GT( static_cast<hid_t>( fd ), -1 );
			EXPECT_EQ( ik, i); 	EXPECT_EQ( lk, j);
			H5Pclose(fcpl);
		}
	}
}

TEST(H5F, set_istore_k) {
/*
H5Pset_istore_k sets the size of the parameter used to control the B-trees for indexing chunked datasets. This function is valid only for file creation property lists.
ik is one half the rank of a tree that stores chunked raw data. On average, such a tree will be 75% full, or have an average rank of 1.5 times the value of ik.

The HDF5 library uses (ik*2) as the maximum # of entries before splitting a B-tree node. Since only 2 bytes are used in storing # of entries for a B-tree node in an HDF5 file, (ik*2) cannot exceed 65536. The default value for ik is 32.
*/
	unsigned ik;
	unsigned max = 16385;
	for(size_t i=32; i < max; i*=2) {
		h5::fd_t fd = h5::create( "set_istore_k.h5", H5F_ACC_TRUNC, h5::istore_k{i} );
		hid_t fcpl = H5Fget_create_plist( static_cast<hid_t>(fd) );
		H5Pget_istore_k(fcpl, &ik );
		EXPECT_GT( static_cast<hid_t>( fd ), -1 );
		EXPECT_EQ( ik, i);
		H5Pclose(fcpl);
	}
}

TEST(H5F, set_file_space_page_size) {
/*
H5Pset_file_space_page_size sets the file space page size fsp_size used in paged aggregation and paged buffering.
fsp_size has a minimum size of 512. Setting a value less than 512 will return an error. The library default size for the file space page size when not set is 4096.
The size set via this routine may not be changed for the life of the file.
*/
	hsize_t value;
	unsigned max = 16*512;
	for(size_t i=512; i < max; i*=2) {
		h5::fd_t fd = h5::create( "set_file_space_page_size.h5", H5F_ACC_TRUNC, h5::file_space_page_size{i} );
		hid_t fcpl = H5Fget_create_plist( static_cast<hid_t>(fd) );
		H5Pget_file_space_page_size(fcpl, &value );
		EXPECT_GT( static_cast<hid_t>( fd ), -1 );
		EXPECT_EQ( value, i);
		H5Pclose(fcpl);
	}
}

TEST(H5F, set_file_space_strategy) {
/*
H5Pset_file_space_strategy sets the file space handling strategy, specifies persisting free-space or not (persist), and sets the free-space section size threshold in the file creation property list fcpl.
This setting cannot be changed for the life of the file.
As the H5F_FSPACE_STRATEGY_AGGR and H5F_FSPACE_STRATEGY_NONE strategies do not use the free-space managers, the persist and threshold settings will be ignored for those strategies.
typedef enum H5F_fspace_strategy_t {
          H5F_FSPACE_STRATEGY_FSM_AGGR = 0,  FSM, Aggregators, VFD  
          H5F_FSPACE_STRATEGY_PAGE = 1	     Paged FSM, VFD 
          H5F_FSPACE_STRATEGY_AGGR = 2	     Aggregators, VFD 
          H5F_FSPACE_STRATEGY_NONE = 3,      VFD 
          H5F_FSPACE_STRATEGY_NTYPES     
    } H5F_fspace_strategy_t;
*/
	hsize_t max = 0x1 << 16;
	for( unsigned strategy=H5F_FSPACE_STRATEGY_FSM_AGGR; strategy < H5F_FSPACE_STRATEGY_NTYPES; strategy++) {
		for(hsize_t threshold=512; threshold < max; threshold*=2) {
			//auto prop = h5::file_space_strategy{strategy, 0, threshold};
			//h5::fd_t fd = h5::create( "set_file_space_strategy.h5", 
			//	H5F_ACC_TRUNC, h5::file_space_strategy{{strategy, 0, threshold}} );
			//hid_t fcpl = H5Fget_create_plist( static_cast<hid_t>(fd) );
			//H5Pget_file_space_page_size(fcpl, &value );
			//EXPECT_GT( static_cast<hid_t>( fd ), -1 );
			EXPECT_EQ( 0, 1); //FIXME: !!!!!!!!!!
			//H5Pclose(fcpl);
	}}
}

TEST(H5F, set_shared_mesg_nindexes) {
/*
H5Pset_shared_mesg_nindexes sets the number of shared object header message indexes in the specified file creation property list.
This setting determines the number of shared object header message indexes that will be available in files created with this property list. These indexes can then be configured with H5Pset_shared_mesg_index.
If nindexes is set to 0 (zero), shared object header messages are disabled in files created with this property list.
*/
	hsize_t value;
	unsigned N = H5O_SHMESG_MAX_NINDEXES;
   	unsigned nidx;
	h5::fd_t fd = h5::create( "set_shared_mesg_nindexes.h5", H5F_ACC_TRUNC, h5::shared_mesg_nindexes{N} );
	hid_t fcpl_id = H5Fget_create_plist( static_cast<hid_t>(fd) );
	H5Pget_shared_mesg_nindexes( fcpl_id, &nidx );
	EXPECT_GT( static_cast<hid_t>( fd ), -1 );
	EXPECT_EQ( nidx, N);
	H5Pclose(fcpl_id);
}

TEST(H5F, set_shared_mesg_indexes) {
/*
H5Pset_shared_mesg_index is used to configure the specified shared object header message index, setting the types of messages that may be stored in the index and the minimum size of each message.
fcpl_id specifies the file creation property list.
index_num specifies the index to be configured. index_num is zero-indexed, so in a file with three indexes, they will be numbered 0, 1, and 2.
mesg_type_flags and min_mesg_size specify, respectively, the types and minimum size of messages that can be stored in this index.
Valid message types are as follows:
    	H5O_SHMESG_NONE_FLAG	No shared messages
    	H5O_SHMESG_SDSPACE_FLAG    	Simple dataspace message
    	H5O_SHMESG_DTYPE_FLAG	Datatype message
    	H5O_SHMESG_FILL_FLAG	Fill value message
    	H5O_SHMESG_PLINE_FLAG	Filter pipeline message
    	H5O_SHMESG_ATTR_FLAG	Attribute message
    	H5O_SHMESG_ALL_FLAG	All message types; i.e., equivalent to the following:
(H5O_SHMESG_SDSPACE_FLAG | H5O_SHMESG_DTYPE_FLAG | H5O_SHMESG_FILL_FLAG | H5O_SHMESG_PLINE_FLAG | H5O_SHMESG_ATTR_FLAG)
*/
	unsigned N = H5O_SHMESG_MAX_NINDEXES;
	try {
		h5::fd_t fd = h5::create( "set_shared_mesg_indexes.h5", H5F_ACC_TRUNC,
			h5::shared_mesg_index{0, H5O_SHMESG_DTYPE_FLAG, N-1} |  h5::shared_mesg_nindexes{N});
		EXPECT_GT( static_cast<hid_t>( fd ), -1 );
	} catch( const h5::error::any& err ){
		ADD_FAILURE();
	}
}

TEST(H5F, set_shared_mesg_phase_change) {
/*
H5Pset_shared_mesg_phase_change sets threshold values for storage of shared object header message indexes in a file. These phase change thresholds determine the point at which the index storage mechanism changes from a more compact list format to a more performance-oriented B-tree format, and vice-versa.
By default, a shared object header message index is initially stored as a compact list. When the number of messages in an index exceeds the threshold value of max_list, storage switches to a B-tree for improved performance. If the number of messages subsequently falls below the min_btree threshold, the index will revert to the list format.
If max_list is set to 0 (zero), shared object header message indexes in the file will be created as B-trees and will never revert to lists.
fcpl_id specifies the file creation property list.
*/
	unsigned N = H5O_SHMESG_MAX_NINDEXES;
	try {
		h5::fd_t fd = h5::create( "set_shared_mesg_phase_change.h5", H5F_ACC_TRUNC,
			h5::shared_mesg_phase_change{256,128});
		EXPECT_GT( static_cast<hid_t>( fd ), -1 );
	} catch( const h5::error::any& err ){
		ADD_FAILURE();
	}
}



/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/
