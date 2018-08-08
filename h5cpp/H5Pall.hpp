/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_PALL_HPP
#define  H5CPP_PALL_HPP
/** impl::property_group<H5Pset_property, args...> args... ::= all argument types except the first `this` hid_t prop_ID
 *  since it is implicitly passed by template class. 
 */ 
namespace h5 {

// FILE CREATION PROPERTY LISTS
using userblock                = impl::fcpl_call<H5Pset_userblock,  hsize_t>;
using istore_k                 = impl::fcpl_call<H5Pset_istore_k, unsigned>;
using file_space_page_size     = impl::fcpl_call<H5Pset_file_space_page_size, hsize_t>;
using shared_mesg_nindexes     = impl::fcpl_call<H5Pset_shared_mesg_nindexes, unsigned>;
using sizes                    = impl::fcpl_call<H5Pset_sizes, size_t,size_t>;
using sym_k                    = impl::fcpl_call<H5Pset_sym_k, unsigned,unsigned>;
using shared_mesg_index        = impl::fcpl_call<H5Pset_shared_mesg_index, unsigned,unsigned,unsigned>;
using shared_mesg_phase_change = impl::fcpl_call<H5Pset_shared_mesg_phase_change, unsigned,unsigned>;
// FILE ACCESS PROPERTY LISTS
using fclose_degree            = impl::fapl_call<H5Pset_fclose_degree, H5F_close_degree_t>;
using fapl_core                = impl::fapl_call<H5Pset_fapl_core, size_t, hbool_t>;
using core_write_tracking      = impl::fapl_call<H5Pset_core_write_tracking, hbool_t, size_t>;
using fapl_family              = impl::fapl_call<H5Pset_fapl_family, hsize_t, hid_t>;
using family_offset            = impl::fapl_call<H5Pset_family_offset, hsize_t>;
using fapl_log                 = impl::fapl_call<H5Pset_fapl_log, const char*,unsigned long long,size_t>;
using multi_type               = impl::fapl_call<H5Pset_multi_type, H5FD_mem_t>;
using fapl_split               = impl::fapl_call<H5Pset_fapl_split, const char*, hid_t,const char*,hid_t>;
using file_image               = impl::fapl_call<H5Pset_file_image, void*,size_t>;
using meta_block_size          = impl::fapl_call<H5Pset_meta_block_size, hsize_t>;
using page_buffer_size         = impl::fapl_call<H5Pset_page_buffer_size,  size_t,unsigned,unsigned>;
using sieve_buf_size           = impl::fapl_call<H5Pset_sieve_buf_size, size_t>;
using alignment                = impl::fapl_call<H5Pset_alignment, hsize_t, hsize_t>;
using cache                    = impl::fapl_call<H5Pset_cache, int,size_t,size_t,double>;
using elink_file_cache_size    = impl::fapl_call<H5Pset_elink_file_cache_size, unsigned>;
using evict_on_close           = impl::fapl_call<H5Pset_evict_on_close, hbool_t>;
using metadata_read_attempts   = impl::fapl_call<H5Pset_metadata_read_attempts, unsigned>;
using mdc_config               = impl::fapl_call<H5Pset_mdc_config, H5AC_cache_config_t *>;
using mdc_image_config         = impl::fapl_call<H5Pset_mdc_image_config, H5AC_cache_image_config_t*>;
using mdc_log_options          = impl::fapl_call<H5Pset_mdc_log_options, hbool_t,const char*,hbool_t>;
using libver_bounds            = impl::fapl_call<H5Pset_libver_bounds, H5F_libver_t,H5F_libver_t>;
namespace flag {
	using fapl_sec2                = impl::fapl_call<H5Pset_fapl_sec2>;
	using fapl_stdio               = impl::fapl_call<H5Pset_fapl_stdio>;
}
const static flag::fapl_sec2  sec2{};
const static flag::fapl_stdio stdio{};
const static h5::fclose_degree fclose_degree_weak{H5F_CLOSE_WEAK};
const static h5::fclose_degree fclose_degree_semi{H5F_CLOSE_SEMI};
const static h5::fclose_degree fclose_degree_strong{H5F_CLOSE_STRONG};
const static h5::fclose_degree fclose_degree_default{H5F_CLOSE_DEFAULT	};
// GROUP CREATION PROPERTY LISTS
using local_heap_size_hint     = impl::gcpl_call<H5Pset_local_heap_size_hint, size_t>;
using link_creation_order      = impl::gcpl_call<H5Pset_link_creation_order, unsigned>;
using est_link_info            = impl::gcpl_call<H5Pset_est_link_info, unsigned, unsigned>;
using link_phase_change        = impl::gcpl_call<H5Pset_link_phase_change,unsigned, unsigned>;
// GROUP ACCESS PROPERTY LISTS
// LINK CREATION PROPERTY LISTS
using char_encoding            = impl::lcpl_call<H5Pset_char_encoding,H5T_cset_t>;
using create_intermediate_group= impl::lcpl_call<H5Pset_create_intermediate_group, unsigned>;
const static h5::char_encoding ascii{H5T_CSET_ASCII};
const static h5::char_encoding utf8{H5T_CSET_UTF8};
const static h5::create_intermediate_group create_path{1};
// LINK ACCESS PROPERTY LISTS
using nlinks                   = impl::lapl_call<H5Pset_nlinks, size_t>;
using elink_cb                 = impl::lapl_call<H5Pset_elink_cb, H5L_elink_traverse_t, void*>;
using elink_prefix             = impl::lapl_call<H5Pset_elink_prefix, const char*>;
using elink_fapl               = impl::lapl_call<H5Pset_elink_fapl, hid_t>;
using elink_acc_flags          = impl::lapl_call<H5Pset_elink_acc_flags, unsigned>;
// DATA CREATION PROPERTY LISTS
using layout                   = impl::dcpl_call<H5Pset_layout,  H5D_layout_t>;
using deflate                  = impl::dcpl_call<H5Pset_deflate, unsigned>;
using gzip                     = deflate;
using fill_time                = impl::dcpl_call<H5Pset_fill_time, H5D_fill_time_t>;
using alloc_time               = impl::dcpl_call<H5Pset_alloc_time, H5D_alloc_time_t>;
using chunk_opts               = impl::dcpl_call<H5Pset_chunk_opts, unsigned>;
template<class T>
using fill_value               = impl::dcpl_tcall<H5Pset_fill_value, T>;                /*tcall ::= templated call with T*/
using chunk                    = impl::dcpl_acall<H5Pset_chunk,  int,  const hsize_t*>; /*acall ::= array call of hsize_t */
namespace flag{
	using fletcher32           = impl::dcpl_call<H5Pset_fletcher32>;
	using shuffle              = impl::dcpl_call<H5Pset_shuffle>;
	using nbit                 = impl::dcpl_call<H5Pset_nbit>;
}
const static flag::fletcher32 fletcher32{};
const static flag::shuffle shuffle{};
const static flag::nbit nbit{};
const static h5::layout layout_compact{H5D_COMPACT};
const static h5::layout layout_contigous{H5D_CONTIGUOUS};
const static h5::layout layout_chunked{H5D_CHUNKED};
const static h5::layout layout_virtual{H5D_VIRTUAL};
const static h5::fill_time fill_time_ifset{H5D_FILL_TIME_IFSET};
const static h5::fill_time fill_time_alloc{H5D_FILL_TIME_ALLOC};
const static h5::fill_time fill_time_never{H5D_FILL_TIME_NEVER};
const static h5::alloc_time alloc_time_default{H5D_ALLOC_TIME_DEFAULT};
const static h5::alloc_time alloc_time_early{H5D_ALLOC_TIME_EARLY};
const static h5::alloc_time alloc_time_incr{H5D_ALLOC_TIME_INCR};
const static h5::alloc_time alloc_time_late{H5D_ALLOC_TIME_LATE};
const static h5::chunk_opts dont_filter_partial_chunks{H5D_CHUNK_DONT_FILTER_PARTIAL_CHUNKS};
// DATA ACCESS PROPERTY LISTS
using chunk_cache              = impl::fapl_call<H5Pset_chunk_cache, size_t, size_t, double>;
using efile_prefix             = impl::fapl_call<H5Pset_efile_prefix, const char*>;
using virtual_view             = impl::fapl_call<H5Pset_virtual_view, H5D_vds_view_t>;
using virtual_printf_gap       = impl::fapl_call<H5Pset_virtual_printf_gap, hsize_t>;
// DATA TRANSFER PROPERTY LISTS
using buffer                   = impl::dxpl_call<H5Pset_buffer, size_t,void*,void*>;
using edc_check                = impl::dxpl_call<H5Pset_edc_check,H5Z_EDC_t>;
using filter_callback          = impl::dxpl_call<H5Pset_filter_callback, H5Z_filter_func_t,void*>;
using data_transform           = impl::dxpl_call<H5Pset_data_transform, const char *>;
using type_conv_cb             = impl::dxpl_call<H5Pset_type_conv_cb, H5T_conv_except_func_t,void*>;
using hyper_vector_size        = impl::dxpl_call<H5Pset_hyper_vector_size, size_t>;
using btree_ratios             = impl::dxpl_call<H5Pset_btree_ratios, double,double,double>;
// OBJECT CREATION PROPERTY LISTS
using ocreate_intermediate_group = impl::ocrl_call<H5Pset_create_intermediate_group,unsigned>;
using obj_track_times            = impl::ocrl_call<H5Pset_obj_track_times, hbool_t>;
using attr_phase_change          = impl::ocrl_call<H5Pset_attr_phase_change, unsigned,unsigned>;
using attr_creation_order        = impl::ocrl_call<H5Pset_attr_creation_order, unsigned>;
//static h5::ocreate_intermediate_group            ocreate_intermediate_group{1};
// OBJECT COPY PROPERTY LISTS
using copy_object                = impl::ocpl_call<H5Pset_copy_object, unsigned>;
using mcdt_search_cb             = impl::ocpl_call<H5Pset_mcdt_search_cb, H5O_mcdt_search_cb_t , void*>;
const static h5::copy_object shallow_hierarchy{H5O_COPY_SHALLOW_HIERARCHY_FLAG};
const static h5::copy_object expand_soft_link{H5O_COPY_EXPAND_SOFT_LINK_FLAG};
const static h5::copy_object expand_ext_link{H5O_COPY_EXPAND_EXT_LINK_FLAG};
const static h5::copy_object expand_reference{H5O_COPY_EXPAND_REFERENCE_FLAG};
const static h5::copy_object copy_without_attr{H5O_COPY_WITHOUT_ATTR_FLAG};
const static h5::copy_object merge_commited_dtype{H5O_COPY_MERGE_COMMITTED_DTYPE_FLAG};


#ifdef H5_HAVE_PARALLEL
using fapl_mpio                  = impl::fapl_call<H5Pset_fapl_mpio, MPI_Comm, MPI_Info>;
using all_coll_metadata_ops      = impl::fapl_call<H5Pset_all_coll_metadata_ops, hbool_t>;
using coll_metadata_write        = impl::fapl_call<H5Pset_coll_metadata_write, hbool_t>;
using gc_reference               = impl::fapl_call<H5Pset_gc_reference, unsigned>;
using small_data_block_size      = impl::fapl_call<H5Pset_small_data_block_size, hsize_t>;
using object_flush_cb            = impl::fapl_call<H5Pset_object_flush_cb, H5F_flush_cb_t,void*>;
using all_coll_metadata_ops      = impl::fapl_call<H5Pset_all_coll_metadata_ops, hbool_t.;
using all_coll_metadata_ops,     = impl::lapl_call<H5Pset_all_coll_metadata_ops, hbool_t>;
using all_coll_metadata_ops      = impl::gapl_call<H5Pset_all_coll_metadata_ops, hbool_t>;
using dxpl_mpio                  = impl::dxpl_call<H5Pset_dxpl_mpio, H5FD_mpio_xfer_t>;
using dxpl_mpio_chunk_opt        = impl::dxpl_call<H5Pset_dxpl_mpio_chunk_opt, H5FD_mpio_chunk_opt_t>;
using dxpl_mpio_chunk_opt_num    = impl::dxpl_call<H5Pset_dxpl_mpio_chunk_opt_num, unsigned>;
using mpio_chunk_opt_ratio       = impl::dxpl_call<H5Pset_mpio_chunk_opt_ratio, unsigned>;
using mpio_collective_opt        = impl::dxpl_call<H5Pset_mpio_collective_opt, H5FD_mpio_collective_opt_t>;
using dxpl_mpio                  = impl::dxpl_call<H5Pset_dxpl_mpio, H5FD_mpio_xfer_t *>;
using mpio_actual_chunk_opt_mode = impl::dxpl_call<H5Pset_mpio_actual_chunk_opt_mode, H5D_mpio_actual_chunk_opt_mode_t *>;
using mpio_actual_io_mode        = impl::dxpl_call<H5Pset_mpio_actual_io_mode, H5D_mpio_actual_io_mode_t*>
using mpio_no_collective_cause   = impl::dxpl_call<H5Pset_mpio_no_collective_cause, uint32_t *,uint32_t *>;
#endif
}

namespace h5 { namespace notimplemented_yet { // OBJECT COPY PROPERTY LISTS
	//using char_encoding_ =              = impl::fapl_call<H5Pset_char_encoding, H5T_cset_t)
	//static h5::char_encoding_ ascii{H5T_CSET_ASCII};
	//static h5::char_encoding_ utf8{H5T_CSET_UTF8};
    //FIXME: using fapl_direct =  impl::fapl_call< .. , size_t, size_t, size_t>;
	//TODO:	using vlen_mem_manager,) too complex to implement
    //TODO:	using append_flush -- too complex to implement
	//NOT MAPPED: fapl_direct, mpiposix, fapl_multi
	//MISSING:	H5CPP__PROPERTYLIST_FLAG(fapl, fapl_windows)
	//MISSING:	using file_image_callbacks, H5_file_image_callbacks_t* >;
}}

namespace h5 {
	const static h5::dapl_t default_dapl = static_cast<h5::dapl_t>( H5P_DEFAULT );
	const static h5::dcpl_t default_dcpl = static_cast<h5::dcpl_t>( H5P_DEFAULT );
	const static h5::dxpl_t default_dxpl = static_cast<h5::dxpl_t>( H5P_DEFAULT );
	const static h5::lcpl_t default_lcpl = h5::char_encoding{H5T_CSET_UTF8} | h5::create_intermediate_group{1};


	const static h5::fapl_t default_fapl = static_cast<h5::fapl_t>( H5P_DEFAULT );
	const static h5::fcpl_t default_fcpl = static_cast<h5::fcpl_t>( H5P_DEFAULT );

	const static h5::dapl_t dapl = static_cast<h5::dapl_t>( H5P_DEFAULT );
	const static h5::dcpl_t dcpl = static_cast<h5::dcpl_t>( H5Pcreate( H5P_DATATYPE_CREATE ));
	const static h5::dxpl_t dxpl = static_cast<h5::dxpl_t>( H5P_DEFAULT );
	const static h5::lcpl_t lcpl = h5::char_encoding{H5T_CSET_UTF8} | h5::create_intermediate_group{1};
	const static h5::fapl_t fapl = static_cast<h5::fapl_t>( H5P_DEFAULT );
	const static h5::fcpl_t fcpl = static_cast<h5::fcpl_t>( H5P_DEFAULT );
}

#endif

