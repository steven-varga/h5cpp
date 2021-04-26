/*
 * Copyright (c) 2018-2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_PDCPL_HPP
#define  H5CPP_PDCPL_HPP

#define H5CPP_DCPL_MULTI_DATASET "h5cpp_dcpl_multi_dataset"

namespace h5::impl {
    inline herr_t dcpl_multi_dataset_close( const char *name, size_t size, void *ptr ){
        return 0;
    }
    inline ::herr_t dcpl_multi_dataset(::hid_t dcpl, const char** values){
        // ignore if already set 
        if( H5Pexist(dcpl, H5CPP_DCPL_MULTI_DATASET) ) return 0;
        return H5Pinsert2(dapl, H5CPP_DCPL_MULTI_DATASET, 8, 0,
            nullptr, nullptr, nullptr, nullptr, nullptr,    nullptr);
    }

    template <class capi, typename capi::fn_t capi_call>
        using dcpl_const_char_call = aprop_t<dcpl_t, default_dcpl,  capi, capi_call, const char*>;
}

inline ::herr_t h5cpp_dcpl_multi_dataset(::hid_t dcpl, const char** ){
    std::cout << "[!!!!!!!!!!!!!!!!!!!!!]";
    return 0;
}

namespace h5 {
    using multi                    = impl::dcpl_const_char_call< impl::dcpl_args<hid_t, const char**>, h5cpp_dcpl_multi_dataset>;
    #if H5_VERSION_GE(1,8,16)
        using chunk                = impl::dcpl_acall< impl::dcpl_args<hid_t,int,const hsize_t*>, H5Pset_chunk>; /*acall ::= array call of hsize_t */
    #endif
    #if H5_VERSION_GE(1,10,0)
        using layout               = impl::dcpl_call< impl::dcpl_args<hid_t,H5D_layout_t>,H5Pset_layout>;
        using chunk_opts           = impl::dcpl_call< impl::dcpl_args<hid_t,unsigned>,H5Pset_chunk_opts>;
    #endif
    using deflate                  = impl::dcpl_call< impl::dcpl_args<hid_t,unsigned>,H5Pset_deflate>;
    using gzip                     = deflate;
    using fill_time                = impl::dcpl_call< impl::dcpl_args<hid_t,H5D_fill_time_t>,H5Pset_fill_time>;
    using alloc_time               = impl::dcpl_call< impl::dcpl_args<hid_t,H5D_alloc_time_t>,H5Pset_alloc_time>;
    using chunk_opts               = impl::dcpl_call< impl::dcpl_args<hid_t,unsigned>,H5Pset_chunk_opts>;
    template<class T> /*tcall ::= templated call with T*/
    using fill_value               = impl::dcpl_tcall< impl::dcpl_args<hid_t,hid_t,const void*>, H5Pset_fill_value, T>;
    using chunk                    = impl::dcpl_acall< impl::dcpl_args<hid_t,int,const hsize_t*>, H5Pset_chunk>; /*acall ::= array call of hsize_t */
    namespace flag{
        using fletcher32           = impl::dcpl_call< impl::dcpl_args<hid_t>,H5Pset_fletcher32>;
        using shuffle              = impl::dcpl_call< impl::dcpl_args<hid_t>,H5Pset_shuffle>;
        using nbit                 = impl::dcpl_call< impl::dcpl_args<hid_t>,H5Pset_nbit>;
    }
    const static flag::fletcher32 fletcher32;
    const static flag::shuffle shuffle;
    const static flag::nbit nbit;


    const static h5::layout compact{H5D_COMPACT};
    const static h5::layout contigous{H5D_CONTIGUOUS};
    const static h5::layout chunked{H5D_CHUNKED};
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
}

namespace h5 {
    const static h5::dcpl_t dcpl = static_cast<h5::dcpl_t>( H5P_DEFAULT );
    const static h5::dcpl_t default_dcpl = static_cast<h5::dcpl_t>( H5P_DEFAULT );
}
#endif
