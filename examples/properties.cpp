
#include <h5cpp/all>
#include <cstddef>

/*
 * some of these examples are to present the basic idea how property lists are mapped, and can be used with H5CPP 
 * all objects are managed: upon leaving code block will do the right thing, may be passed to CAPI calls depending 
 * on implicit/explicit conversion policy defined
 *
 * for brevity only some of the property list are listed here, while most of the properties are mapped to managed resource 
 * the complex ones are left as an exercise for you to implement -- if you ever need them.
 *
 * The list of descriptors: 
 * 			attribute    : acpl_t  
 * 			data transfer: dapl_t  dcpl_t  dxpl_t 
 * 			type         : tapl_t  tcpl_t
 * 			file         : fapl_t  fcpl_t         fmpl_t
 * 			group        : gapl_t  gcpl_t
 * 			link         : lapl_t  lcpl_t
 * 			object       :         ocrl_t                ocpl_t
 * 			string       :         scpl_t
 */

int main(){
	struct my_struct{
		int a;
		double b;
	};
	{ //DATA CREATION PROPERTY LISTS
		my_struct STR={4,5.7};
		h5::dcpl_t dcpl = h5::chunk{1,4,5} | h5::deflate{4} | h5::layout_compact | h5::dont_filter_partial_chunks
				| h5::fill_value<my_struct>{STR} | h5::fill_time_never | h5::alloc_time_early 
				| h5::fletcher32 | h5::shuffle | h5::nbit;
	//	H5Dcreate(fd, "", static_cast<>(dcpl) )
	//TODO: filters, external, virtual,  szip, file_space_strategy
	}

	{// FILE CREATION PROPERTY LISTS
		h5::fcpl_t fcpl =
				  h5::sizes{2,4} | h5::userblock{512} | h5::sym_k{16,4} |  h5::istore_k{32} 
				| h5::file_space_page_size{4096} | h5::shared_mesg_index{0,H5O_SHMESG_NONE_FLAG,512} | h5::shared_mesg_phase_change{0,0};
	}

	{//FILE ACCESS PROPERTY LISTS
	//FIXME:  h5::fapl_direct{16,0,1024};
	h5::fapl_t fapl = h5::fclose_degree_weak | h5::fapl_core{2048,1} | h5::core_write_tracking{false,1} 
				| h5::fapl_family{H5F_FAMILY_DEFAULT,0} ;
	//FLAGS: fclose_degree_weak, fclose_degree_semi, fclose_degree_strong, fclose_degree_default

	//MAPPED: fclose_degree, fapl_core, core_write_tracking, fapl_family, family_offset, fapl_log, fapl_mpio, 
	//        multi_type, fapl_split, fapl_sec2, fapl_stdio, file_image, meta_block_size, page_buffer_size, sieve_buf_size,
	//        alignment, cache, elink_file_cache_size, evict_on_close, metadata_read_attempts, mdc_config,mdc_image_config, mdc_log_options
	//        all_coll_metadata_ops, coll_metadata_write, gc_reference, small_data_block_size, libver_bounds, object_flush_cb
	//TODO: fapl_direct, mpiposix, fapl_multi

	}

	{//GROUP CREATION PROPERTY LISTS
		h5::gcpl_t gcpl = h5::local_heap_size_hint{16} | h5::link_creation_order{1} | h5::est_link_info{1,2} | h5::link_phase_change{1,2};
	}
	{//GROUP ACCESS PROPERTY LISTS
	#ifdef H5_HAVE_PARALLEL
		h5::gapl_t gapl = all_coll_metadata_ops{true};
	#endif
	}
	{//LINK CREATION PROPERTY LISTS
		// char_encoding{} | create_intermediate_group{-1}
	}
	{//LINK ACCESS PROPERTY LISTS:  nlinks{size_t}, all_coll_metadata_ops{bool} elink_cb{}
	}

}

