/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include "H5Pall.hpp"
#include "H5Zpipeline_basic.hpp"

#define H5CPP_DAPL_HIGH_THROUGHPUT "h5cpp_dapl_highthroughput"

	namespace h5::impl {
		inline herr_t dapl_pipeline_close( const char *name, size_t size, void *ptr ){
			(void)name; (void)size;
			delete *static_cast< impl::pipeline_t<impl::basic_pipeline_t>**>( ptr );
			return 0;
		}
		// Copy callback for the high_throughput pipeline property.
		//
		// HDF5 ≥ 1.10.7 internally copies the DAPL when an object (dataset, file,
		// attribute) is opened or created against it, so the object can carry its
		// own DAPL.  Without a copy callback, the property's bytes (the 8-byte
		// pipeline pointer) are memcpy'd verbatim — leaving both the user's DAPL
		// and HDF5's internal copy holding the same pipeline pointer.  When each
		// DAPL is later destroyed, dapl_pipeline_close fires on the same pointer
		// twice → double-free.  This is the root cause of the historic "crashes
		// on 1.12.x" TODO.  See issue #242.
		//
		// Fresh-allocation semantics are correct here because the pipeline is
		// per-write scratch state, not shared accumulating state — set_cache()
		// runs on every H5Dopen and on every h5::write/h5::read call.
		inline herr_t dapl_pipeline_copy( const char *name, size_t size, void *value ){
			(void)name; (void)size;
			using pipeline = impl::pipeline_t<impl::basic_pipeline_t>;
			*static_cast<pipeline**>(value) = new pipeline();
			return 0;
		}
		inline ::herr_t dapl_pipeline_set(::hid_t dapl ) {
			// ignore if already set
			if( H5Pexist(dapl, H5CPP_DAPL_HIGH_THROUGHPUT) ) return 0;
			using pipeline = impl::pipeline_t<impl::basic_pipeline_t>;
			pipeline* ptr = new pipeline();
			return H5Pinsert2(dapl, H5CPP_DAPL_HIGH_THROUGHPUT, sizeof( pipeline* ), &ptr,
				nullptr,                  // set
				nullptr,                  // get
				nullptr,                  // prp_del
				dapl_pipeline_copy,       // copy — issue #242 fix
				nullptr,                  // compare
				dapl_pipeline_close);
		}
}

namespace h5 {
// DATA ACCESS PROPERTY LISTS
#if H5_VERSION_GE(1,10,0)
	using efile_prefix 		   = impl::dapl_call< impl::dapl_args<hid_t,const char*>,H5Pset_efile_prefix>;
	using virtual_view         = impl::dapl_call< impl::dapl_args<hid_t,H5D_vds_view_t>,H5Pset_virtual_view>;
	using virtual_printf_gap   = impl::dapl_call< impl::dapl_args<hid_t,hsize_t>,H5Pset_virtual_printf_gap>;
#endif
	using chunk_cache          = impl::dapl_call< impl::dapl_args<hid_t,size_t, size_t, double>,H5Pset_chunk_cache>;
	//using num_threads  	   = impl::dapl_call< impl::dapl_args<hid_t, unsigned char>,impl::dapl_threads>;
	namespace flag {
		using high_throughput  = impl::dapl_call< impl::dapl_args<hid_t>,impl::dapl_pipeline_set>;
	}
	// Deprecated: direct-chunk is now the DEFAULT for chunked writes (no opt-in
	// needed), so this tag is a no-op affirmation kept only so existing call sites
	// keep compiling.  Removed in v2.x.y.
	[[deprecated("h5::high_throughput is the default for chunked writes now; the tag is a no-op, removed in v2.x.y")]]
	const static flag::high_throughput high_throughput;

	namespace impl {
		inline const h5::dapl_t& _dapl_singleton() {
			static h5::dapl_t* p = new h5::dapl_t(static_cast<h5::dapl_t>(H5Pcreate(H5P_DATASET_ACCESS)));
			return *p;
		}
		inline const h5::dapl_t& _default_dapl_singleton() {
			static h5::dapl_t* p = new h5::dapl_t(static_cast<h5::dapl_t>(H5Pcreate(H5P_DATASET_ACCESS)));
			return *p;
		}
	}
	inline const h5::dapl_t& dapl = impl::_dapl_singleton();
	//const static h5::dapl_t default_dapl = high_throughput;
	inline const h5::dapl_t& default_dapl = impl::_default_dapl_singleton();
}
