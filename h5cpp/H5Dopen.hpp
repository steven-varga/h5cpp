/*
 * Copyright (c) 2018 - 2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef  H5CPP_DOPEN_HPP 
#define  H5CPP_DOPEN_HPP

#include <hdf5.h>
#include "H5config.hpp"
#include "H5Eall.hpp"
#include "H5Iall.hpp"
#include "H5Pall.hpp"
#include "H5Zpipeline.hpp"
#include "H5Zpipeline_basic.hpp"
#include "H5Pdapl.hpp"
#include <type_traits>
#include <string>


namespace h5{
    /** \ingroup file-io 

     * open an existing dataset/document within an valid HDF5 file, the returned handle automatically closed when leaving code block
     * \par_fd \par_dataset_path \par_dapl  \returns_fd 
     * \sa_h5cpp \sa_hdf5  \sa_stl 
     * @code
     * { 
     *    h5::fd_t fd = h5::open("example.h5", H5F_ACC_RDWR); // open an hdf5 file in read-write mode
     *    h5::ds_t ds = h5::open(fd,"dataset.txt")            // obtain ds descriptor
     *    ...                                                 // do some work
     *  }                                                     // ds and fd closes automatically 
     * @endcode 
     */
    template<class L>
    inline typename std::enable_if< impl::is_location<L>::value,
    h5::ds_t>::type open(const L& loc, const std::string& path, const h5::dapl_t& dapl = h5::default_dapl ){

        H5CPP_CHECK_PROP( dapl, h5::error::io::dataset::open, "invalid data access property" );

        hid_t ds;
        H5CPP_CHECK_NZ((
            ds = H5Dopen2( static_cast<hid_t>(loc), path.data(), static_cast<hid_t>(dapl) )),
                                            h5::error::io::dataset::open, h5::error::msg::open_dataset );
        // custom h5cpp specific C++ high throughpout filter
        // is abstracted out and hidden under property list
        // see: H5Pdapl.hpp and H5Pchunk.hpp for details
        hid_t dcpl = H5Dget_create_plist( ds );

        switch( H5Pget_layout(dcpl) ){
            case H5D_LAYOUT_ERROR: break;
            case H5D_NLAYOUTS:  break;
            case H5D_COMPACT: break;
            case H5D_CONTIGUOUS: break;
            case H5D_CHUNKED:
                if( H5Pexist(dapl, H5CPP_DAPL_HIGH_THROUGPUT) ){
                    // grab pointer to uninitialized pipeline
                    h5::impl::pipeline_t<impl::basic_pipeline_t>* ptr;
                    H5Pget(dapl, H5CPP_DAPL_HIGH_THROUGPUT, &ptr);
                    hid_t type_id = H5Dget_type( static_cast<::hid_t>(ds) );
                    size_t element_size = H5Tget_size( type_id );
                    ptr->set_cache(dcpl, element_size);
                }
                break;
            case H5D_VIRTUAL: break;
        }

        h5::ds_t ds_{ds};
        //FIXME: temporary carry dapl around
        ds_.dapl = static_cast<hid_t>( dapl );
        return ds_;
    }
}

#endif

