/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_DCREATE_HPP 
#define  H5CPP_DCREATE_HPP
#include <limits>

//TODO: if constexpr(..){} >= c++17
namespace h5 {
    /** \func_create_hdr
    * \code
    * examples:
    * //creates a dataset with 2*myvec.size() + offset
    * auto ds = h5::create( "path/to/file.h5", "path/to/dataset", myvec, h5::offset{5}, h5::stride{2} );
    * // explicit dataset spec  
    * \endcode  
    * \par_file_path \par_dataset_path \par_current_dims \par_max_dims 
    * \par_lcpl \par_dcpl \par_dapl  \tpar_T \returns_ds
    */ 
    template<class T, class L, class... args_t>
    inline typename std::enable_if< impl::is_location<L>::value,
    h5::ds_t>::type create( const L& loc, const std::string& dataset_path, args_t&&... args ) try {
        // compile time check of property lists: 
        using tcurrent_dims = typename arg::tpos<const h5::current_dims_t&,const args_t&...>;
        using tmax_dims     = typename arg::tpos<const h5::max_dims_t&,const args_t&...>;
        using tlcpl         = typename arg::tpos<const h5::lcpl_t&,const args_t&...>;
        using tdcpl         = typename arg::tpos<const h5::dcpl_t&,const args_t&...>;
        using tdapl         = typename arg::tpos<const h5::dapl_t&,const args_t&...>;
        using ttype         = typename arg::tpos<const h5::dt_t<T>&,const args_t&...>;
        
        //TODO: make copy of default dcpl
        h5::dcpl_t default_dcpl{ H5Pcreate(H5P_DATASET_CREATE) };
        // get references to property lists or system default values 
        const h5::lcpl_t& lcpl = arg::get(h5::default_lcpl, args...);
        const h5::dcpl_t& dcpl = arg::get(default_dcpl, args...);
        const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);

        H5CPP_CHECK_PROP( lcpl, h5::error::property_list::misc, "invalid list control property" );
        H5CPP_CHECK_PROP( dcpl, h5::error::property_list::misc, "invalid data control property" );
        H5CPP_CHECK_PROP( dapl, h5::error::property_list::misc, "invalid data access property" );
        // and dimensions
        h5::current_dims_t current_dims_default{0}; // if no current dims_present 
        // this mutable value will be referenced
        const h5::current_dims_t& current_dims = arg::get(current_dims_default, args...);
        const h5::max_dims_t& max_dims = arg::get(h5::max_dims_t{0}, args... );
        h5::sp_t space_id{H5I_UNINIT}; // set to invalid state 
        h5::ds_t ds{H5I_UNINIT};

        // accounting
        bool is_equal_dims = true, is_unlimited = false,
            is_extendable = true, is_filtering_on = false;
        hsize_t total_space = sizeof(T);

        if constexpr( tmax_dims::present ){
            // check if `current_dims` present as well, if not, copy `max_dims` into `current_dims`
            // such ranks marked as `H5S_UNLIMITED` will have `current_dims` marked as `0` 
            size_t rank = max_dims.size();
            if constexpr( !tcurrent_dims::present ) {
                // set current dimensions to given one or zero if H5S_UNLIMITED mimics matlab(tm) behavior while allowing extendable matrices
                for(hsize_t i=0; i<rank; i++){
                    current_dims_default[i] = max_dims[i] != H5S_UNLIMITED
                        ? max_dims[i] :  static_cast<hsize_t>(0);
                    // taint compact | contiguous layout if unlimited
                    if( !is_unlimited && max_dims[i] == H5S_UNLIMITED ) is_unlimited = true;
                }
                current_dims_default.rank = rank;
            } else { // both dimensions are provided, check if match, unlimited 
                for(hsize_t i=0; i<rank; i++) {
                    if( is_equal_dims && max_dims[i] != current_dims[i] ) is_equal_dims = false;
                    if( !is_unlimited && max_dims[i] == H5S_UNLIMITED ) is_unlimited = true;
                    total_space *= max_dims[i];
                }
            }
            // at this point we have valid `current_dims` and `max_dims` to describe file space
            space_id = std::move( h5::create_simple( current_dims, max_dims ) );
        } else if (tcurrent_dims::present) {
            // only `current_dims` are present, dataset will have same `max_dims` and is not extendable
            for(hsize_t i=0; i<current_dims.rank; i++)
                total_space *= current_dims[i];
            space_id = std::move( h5::create_simple( current_dims ) );
        } else  // no dimensions are provided, we are dealing with a scalar
            space_id = std::move(h5::sp_t{H5Screate(H5S_SCALAR)});

        //LAYOUT: by default it is contiguous
        is_extendable = is_unlimited || !is_equal_dims;
        if constexpr ( tdcpl::present ) is_filtering_on = H5Pget_nfilters(dcpl) > 0;

        // at this point we have all the information to decide if compact dataset
        if( !is_filtering_on && !is_extendable && total_space < H5CPP_COMPACT_PAYLOAD_MAX_SIZE )
            set_compact_layout(default_dcpl);
        if( !tdcpl::present && ( is_unlimited || (tcurrent_dims::present && tmax_dims::present)) ) {
            chunk_t chunk{0}; chunk.rank = current_dims.rank;
            for(hsize_t i=0; i<current_dims.rank; i++)
                chunk[i] = current_dims[i] ? current_dims[i] : 1;
            h5::set_chunk(default_dcpl, chunk );
        }
        // if type is specified use it, otherwise execute callback: h5::create
        using element_t = typename impl::decay<T>::type;
        h5::dt_t<T> type; ;
        if constexpr( ttype::present )
            type = arg::get(dt_t<T>(), args...);
        else
            type = create<T>();
        return h5::createds(loc, dataset_path, type, space_id, lcpl, dcpl, dapl);

    } catch( const std::runtime_error& err ) {
            throw h5::error::io::dataset::create( err.what() );
    }

    // delegate to h5::fd_t 
    template<class T, class... args_t>
    inline h5::ds_t create( const std::string& file_path, const std::string& dataset_path, args_t&&... args ){
        h5::fd_t fd = h5::open(file_path, H5F_ACC_RDWR, h5::default_fapl);
        return h5::create<T>(fd, dataset_path, args...);
    }
}
#endif

