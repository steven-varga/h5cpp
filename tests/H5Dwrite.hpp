/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_DWRITE_HPP
#define H5CPP_DWRITE_HPP

// defines an runtime_exception thrown to track template matching
// for various types;  see tests/H5Dwrite 
#ifndef H5CPP_WRITE_DISPATCH_TEST_
    #define H5CPP_WRITE_DISPATCH_TEST_(type, msg)
#endif
#include <string.h>

namespace h5::impl {
    template <class T,  class... args_t>
    h5::ds_t write_all( const h5::ds_t& ds, const T& ref,   args_t&&... args  )  {
        H5CPP_WRITE_DISPATCH_TEST_(T, "write_all(const ds_t, const T& ref, args...)")

        h5::sp_t file_space{0}, mem_space{0};
        const h5::dxpl_t& dxpl = h5::arg::get(h5::default_dxpl, args...);
        using element_t = typename impl::decay<T>::type;
        const element_t* ptr = nullptr;
        if constexpr(meta::has_data<T>::value)
            ptr = ref.data();
        else
            ptr = h5::impl::data( ref );
        h5::dt_t<void> file_type{H5Dget_type(ds)};
        h5::dt_t<T> mem_type = h5::create<T>();

        if (H5Tis_variable_str(mem_type)) {
            H5CPP_CHECK_NZ( H5Dwrite( ds, mem_type, mem_space, file_space, dxpl, &ptr),
                h5::error::io::dataset::write, h5::error::msg::write_dataset);
        } else {
            if(H5Tis_variable_str(file_type)) mem_type = file_type;
            H5CPP_CHECK_NZ( H5Dwrite( ds, mem_type, mem_space, file_space, dxpl, ptr),
                    h5::error::io::dataset::write, h5::error::msg::write_dataset);
        }
        return ds;
    }

    template <class T,  class... args_t>
    h5::ds_t write_selection( const h5::ds_t& ds, const T& ref,   args_t&&... args  )  {
    H5CPP_WRITE_DISPATCH_TEST_(T, "write_selection(const ds_t, const T& ref, args...)")
        using toffset = arg::tpos<h5::offset_t,args_t...>;
        using tstride = arg::tpos<h5::stride_t,args_t...>;
        using tblock = arg::tpos<h5::block_t,args_t...>;
        using tcount = arg::tpos<h5::count_t,args_t...>;

        const h5::dxpl_t& dxpl = h5::arg::get(h5::default_dxpl, args...);
        using element_t = typename impl::decay<T>::type;
        const element_t* ptr = nullptr;
        if constexpr(meta::has_data<T>::value)
            ptr = ref.data();
        else
            ptr = h5::impl::data( ref );
        h5::dt_t<void> file_type{H5Dget_type(ds)};
        h5::dt_t<T> mem_type = h5::create<T>();

        h5::sp_t file_space = H5Dget_space(ds);
        h5::dcpl_t dcpl{H5Dget_create_plist(ds)};
        // hyperslab selection is enabled only for chucked layout
        auto tuple = std::forward_as_tuple(args...);
        const h5::count_t& count = std::get<tcount::value>( tuple );
        int rank = h5::get_simple_extent_ndims( file_space );
        // capture arguments if specified or assign a default value
        const h5::offset_t& offset = arg::get( h5::offset(rank,0), args...);
        const h5::stride_t& stride = arg::get( h5::stride(rank,1), args...);
        const h5::block_t& block = arg::get( h5::block(rank,1), args...);

        hsize_t size = 1;for(int i=0;i<rank;i++) size *= count[i] * block[i];
        h5::sp_t mem_space = h5::create_simple( size );
        h5::select_all( mem_space );
        H5CPP_CHECK_NZ( H5Sselect_hyperslab( file_space, H5S_SELECT_SET, *offset, *stride, *count, *block),
            std::runtime_error,	h5::error::msg::select_hyperslab);
        // the type in file space , <void> marks type erasure
        if (H5Tis_variable_str(mem_type)) {
            H5CPP_CHECK_NZ( H5Dwrite( ds, mem_type, mem_space, file_space, dxpl, &ptr),
                h5::error::io::dataset::write, h5::error::msg::write_dataset);
        } else {
            if(H5Tis_variable_str(file_type)) mem_type = file_type;
            H5CPP_CHECK_NZ( H5Dwrite( ds, mem_type, mem_space, file_space, dxpl, ptr),
                    h5::error::io::dataset::write, h5::error::msg::write_dataset);
        }
        return ds;
    }


    template <class T, class... args_t>
    h5::current_dims_t get_current_dims( const T& ref, args_t&&... args ) {
        using tcount  = typename arg::tpos<const h5::count_t&,const args_t&...>;
        using toffset = typename arg::tpos<const h5::offset_t&,const args_t&...>;
        using tstride = typename arg::tpos<const h5::stride_t&,const args_t&...>;
        using tblock = typename arg::tpos<const h5::block_t&,const args_t&...>;

        // retrieve the dimensions from the object reference
        h5::count_t count = impl::size( ref );
		size_t rank = impl::rank<T>::value;
		h5::current_dims_t current_dims(rank,0);
        for(int i =0; i < rank; i++ ) current_dims[i] = count[i];
        if constexpr( tstride::present ){
            const h5::stride_t& stride =  std::get<tstride::value>( std::forward_as_tuple( args...) );
            if constexpr( tblock::present ){ // tricky, we have block as well
                const h5::block_t& block =  std::get<tblock::value>( std::forward_as_tuple( args...) );
                for(int i=0; i < rank; i++)
                    current_dims[i] *= (stride[i] - block[i] + 1);
            } else
                for(int i=0; i < rank; i++)
                    current_dims[i] *= stride[i];
        }
        if constexpr( toffset::present ) {
            const h5::offset_t& offset =  std::get<toffset::value>( std::forward_as_tuple( args...) );
            for(int i=0; i < rank; i++)
                current_dims[i]+=offset[i];
        }
        return current_dims;
    }
}

namespace h5 {
	template <class T>
	void write( const h5::ds_t& ds, const h5::sp_t& mem_space, const h5::sp_t& file_space, const h5::dxpl_t& dxpl, const T* ptr  ){
		H5CPP_CHECK_PROP( dxpl, h5::error::io::dataset::write, "invalid data transfer property" );
            using element_t = typename h5::impl::decay<T>::type;
		h5::dt_t<element_t> type;

		H5CPP_CHECK_NZ(
				H5Dwrite( static_cast<hid_t>( ds ), type, mem_space, file_space, static_cast<hid_t>(dxpl), ptr),
							h5::error::io::dataset::write, h5::error::msg::write_dataset);
	}

    template <class... args_t>
    ds_t write( const ds_t& ds, const char* const* ptr,   args_t&&... args  ) {
        H5CPP_WRITE_DISPATCH_TEST_(T, "write(const ds_t, const char* const* ref, args...)")

        using toffset = arg::tpos<h5::offset_t,args_t...>;
        using tstride = arg::tpos<h5::stride_t,args_t...>;
        using tblock = arg::tpos<h5::block_t,args_t...>;
        using tcount = arg::tpos<h5::count_t,args_t...>;
        static_assert(toffset::present || (!tstride::present && !tblock::present)
                ,"stride and block requires offset present!");
        h5::sp_t file_space{H5S_ALL}, mem_space{H5S_ALL}; // default CTOR is H5S_ALL which is `0`
        const h5::dxpl_t& dxpl = h5::arg::get(h5::default_dxpl, args...);
        using element_t = const char**;
        h5::dt_t<void> file_type{H5Dget_type(ds)};
        h5::dt_t<const char**> mem_type = h5::create<const char**>();
       // either dataset preexisted before call, or user took manual drive
        // in any event we have to do some selection
        if constexpr (tcount::present) {
            file_space = H5Dget_space(ds);
            h5::dcpl_t dcpl{H5Dget_create_plist(ds)};
            // hyperslab selection is enabled only for chucked layout
            auto tuple = std::forward_as_tuple(args...);
            const h5::count_t& count = std::get<tcount::value>( tuple );
            file_space = H5Dget_space(ds);
            int rank = h5::get_simple_extent_ndims( file_space );
            // capture arguments if specified or assign a default value
            const h5::offset_t& offset = arg::get( h5::offset(rank,0), args...);
            const h5::stride_t& stride = arg::get( h5::stride(rank,1), args...);
            const h5::block_t& block = arg::get( h5::block(rank,1), args...);

            hid_t dapl = h5::get_access_plist( ds );
            hsize_t size = 1;for(int i=0;i<rank;i++) size *= count[i] * block[i];
            mem_space = h5::create_simple( size );
            h5::select_all( mem_space );
            H5CPP_CHECK_NZ( H5Sselect_hyperslab( file_space, H5S_SELECT_SET, *offset, *stride, *block, *count),
                    std::runtime_error,	h5::error::msg::select_hyperslab);
        }
        if(H5Tis_variable_str(mem_type)) mem_type = file_type;
        H5Dwrite( ds, mem_type, mem_space, file_space, dxpl, ptr);
        return ds;
    }


    template <class T, class... args_t> using has_count = typename std::integral_constant<bool,
        !impl::is_linalg<T>::value || arg::tpos<h5::count_t, args_t...>::present>;
    // 
    template <class T,  class... args_t>
    typename std::enable_if<impl::is_contiguous<T>::value && has_count<T, args_t...>::value,
    ds_t>::type write( const ds_t& ds, const T& ref,   args_t&&... args  ) try {
        H5CPP_WRITE_DISPATCH_TEST_(T, "write(const ds_t, const T& ref, args...): contiguous")
        using toffset = arg::tpos<h5::offset_t,args_t...>;
        using tstride = arg::tpos<h5::stride_t,args_t...>;
        using tblock = arg::tpos<h5::block_t,args_t...>;
        using tcount = arg::tpos<h5::count_t,args_t...>;
        static_assert(toffset::present || (!tstride::present && !tblock::present)
                ,"stride and block requires offset present!");
        static_assert(!tcount::present || !std::is_pointer<T>::value
                ,"typed pointers require count{..} specified");
        h5::sp_t file_space{H5S_ALL}, mem_space{H5S_ALL}; // default CTOR is H5S_ALL which is `0`
        const h5::dxpl_t& dxpl = h5::arg::get(h5::default_dxpl, args...);
        using element_t = typename impl::decay<T>::type;
        const element_t* ptr = nullptr;
        if constexpr(meta::has_data<T>::value)
            ptr = ref.data();
        else
            ptr = h5::impl::data( ref );
        h5::dt_t<void> file_type{H5Dget_type(ds)};
        h5::dt_t<T> mem_type = h5::create<T>();

        // either dataset preexisted before call, or user took manual drive
        // in any event we have to do some selection
        if constexpr (tcount::present) {
            file_space = H5Dget_space(ds);
            h5::dcpl_t dcpl{H5Dget_create_plist(ds)};
            // hyperslab selection is enabled only for chucked layout
            auto tuple = std::forward_as_tuple(args...);
            const h5::count_t& count = std::get<tcount::value>( tuple );
            file_space = H5Dget_space(ds);
            int rank = h5::get_simple_extent_ndims( file_space );
            // capture arguments if specified or assign a default value
            const h5::offset_t& offset = arg::get( h5::offset(rank,0), args...);
            const h5::stride_t& stride = arg::get( h5::stride(rank,1), args...);
            const h5::block_t& block = arg::get( h5::block(rank,1), args...);

            hid_t dapl = h5::get_access_plist( ds );
            // custom pipeline requested
            if( H5Pexist(dapl, H5CPP_DAPL_HIGH_THROUGPUT) && H5Pget_layout(dcpl) == H5D_CHUNKED ){
                h5::impl::pipeline_t<impl::basic_pipeline_t>* filters;
                H5Pget(dapl, H5CPP_DAPL_HIGH_THROUGPUT, &filters);
                filters->write(ds, offset, stride, block, count, dxpl, ptr);
                return ds;
            } else {
                hsize_t size = 1;for(int i=0;i<rank;i++) size *= count[i] * block[i];
                mem_space = h5::create_simple( size );
                h5::select_all( mem_space );
                H5CPP_CHECK_NZ( H5Sselect_hyperslab( file_space, H5S_SELECT_SET, *offset, *stride, *count, *block),
                    std::runtime_error,	h5::error::msg::select_hyperslab);
            }
        }
        // the type in file space , <void> marks type erasure
        if (H5Tis_variable_str(mem_type)) {
            H5CPP_CHECK_NZ( H5Dwrite( ds, mem_type, mem_space, file_space, dxpl, &ptr),
                h5::error::io::dataset::write, h5::error::msg::write_dataset);
        } else {
            if(H5Tis_variable_str(file_type)) mem_type = file_type;
            H5CPP_CHECK_NZ( H5Dwrite( ds, mem_type, mem_space, file_space, dxpl, ptr),
                    h5::error::io::dataset::write, h5::error::msg::write_dataset);
        }
        return ds;
	} catch ( const std::runtime_error& err ){
		throw error::io::dataset::write( err.what() );
	}
    template <class T,  class... args_t>
    typename std::enable_if<impl::is_linalg<T>::value && !has_count<T, args_t...>::value,
    ds_t>::type write( const ds_t& ds, const T& ref,   args_t&&... args  ) try {
        using tcount = arg::tpos<h5::count_t,args_t...>;
        if constexpr( !tcount::present ) {
            h5::count_t count = h5::impl::size(ref);
            return write(ds, ref, args..., count);
        } else {
            return write(ds, ref, args...);
        }
        return ds;
	} catch ( const std::runtime_error& err ){
		throw error::io::dataset::write( err.what() );
	}
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
	template <class T, class... args_t>
    typename std::enable_if<!impl::is_contiguous<T>::value,
    ds_t>::type write(const ds_t& ds, const T& ref, args_t&&... args) try {
        H5CPP_WRITE_DISPATCH_TEST_(T, "write(const ds_t, const T& ref, args...): !contiguous")
        using element_t = typename impl::decay<typename impl::decay<T>::type>::type;
        using tcount = arg::tpos<h5::count_t,args_t...>;
        using toffset = arg::tpos<h5::offset_t,args_t...>;
        const h5::count_t count = impl::size(ref);
        h5::chunk_t chunk_dims;
        hsize_t chunk_size=0; chunk_size +=1;
        h5::dcpl_t dcpl{H5Dget_create_plist(ds)};
        switch(  H5Pget_layout(dcpl) ){
            case H5D_CHUNKED:
                chunk_dims.rank = H5Pget_chunk(dcpl, H5CPP_MAX_RANK, *chunk_dims);
                chunk_size = chunk_dims;
                break;
            case H5D_CONTIGUOUS: break;
            case H5D_COMPACT:
                   break;
            default :
                   break;
        }

        const h5::dxpl_t& dxpl = h5::arg::get(h5::default_dxpl, args...);
        h5::dt_t<element_t> mem_type = h5::create<std::string>();
        // use stack to store pointers
        if constexpr ( meta::has_iterator<T>::value ){
            auto end = ref.cend(), it = ref.cbegin();
            size_t i=0, length = ref.size();
            element_t const* buffer[length+1]; buffer[length] = nullptr;
            for(; it != end; it++, i++) {
                //if constexpr( meta::has_direct_access<element_t>::value )
                    buffer[i] = it->data();
                //else
                //    buffer[i] = impl::data(*it);
            }
            // FIXME: pgc++ fails: 
            //write(ds, buffer, args...);
        } else { 
           // we are array of pointers:  
            write(ds, &ref[0], args...);
        }
        return ds;
    } catch ( const std::runtime_error& err ){
		throw error::io::dataset::write( err.what() );
	}

    // create with type + write
    template <class F, class T, class... args_t>
    typename std::enable_if<!std::is_same<T,F>::value,
	h5::ds_t>::type write( const h5::fd_t& fd, const std::string& dataset_path, const T& ref,  args_t&&... args  ){
        H5CPP_WRITE_DISPATCH_TEST_(T, "write<T>(fd, path, T& ref, args...)")

		using element_t = typename impl::decay<T>::type;
        using tcurrent_dims = arg::tpos<h5::current_dims_t,args_t...>;
        using tcount = arg::tpos<h5::count_t,args_t...>;
        mute();
        ds_t ds;
        if( H5Lexists(fd, dataset_path.c_str(), H5P_DEFAULT ) > 0 ) {
            const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
            ds = h5::open(fd, dataset_path, dapl);
        } else {
            if constexpr(!tcurrent_dims::present && !impl::is_scalar<T>::value && !impl::is_array<T>::value){
                h5::current_dims_t current_dims = impl::get_current_dims(ref, args...);
                ds = h5::create<F>(fd, dataset_path, current_dims, args...);
            } else ds = h5::create<F>(fd, dataset_path, args...);
        }
		h5::unmute();
        if constexpr (tcurrent_dims::present && !tcount::present) {
            const h5::count_t count = impl::size(ref);
            h5::write(ds, ref, count, args...);
        } else
            h5::write(ds, ref, args...);
        return ds;
    }
	template <class T, class... args_t>
	h5::ds_t write( const h5::fd_t& fd, const std::string& dataset_path, const T& ref,  args_t&&... args  ){
        H5CPP_WRITE_DISPATCH_TEST_(T, "write(fd, path, T& ref, args...)")

		using element_t = typename impl::decay<T>::type;
        using tcurrent_dims = arg::tpos<h5::current_dims_t,args_t...>;
        using tcount = arg::tpos<h5::count_t,args_t...>;
        mute();
        ds_t ds;
        // dataset already exists let's open it with ``and get a handle
        if( H5Lexists(fd, dataset_path.c_str(), H5P_DEFAULT ) > 0 ) {
            const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
            ds = h5::open(fd, dataset_path, dapl);
        } else {
            if constexpr(!tcurrent_dims::present && !impl::is_scalar<T>::value && !impl::is_array<T>::value) {
                h5::current_dims_t current_dims = impl::get_current_dims(ref, args...);
                ds = h5::create<T>(fd, dataset_path, current_dims, args...);
                // we just created the matching file space, we can fast track it with S_ALL, S_ALL
                return impl::write_all(ds, ref, args...);
            } else // when `current_dims{..}` specified the target space is different from memory space
                ds = h5::create<T>(fd, dataset_path, args...);
        }
		h5::unmute();
        if constexpr (!tcount::present) {
            // `count{..}` is not specified, has to be retrieved from `ref` object
            const h5::count_t count = impl::size(ref);
            h5::write(ds, ref, count, args...);
        } else h5::write(ds, ref, args...);
        return ds;
	}

    // T[N][..] and std::initializer_list<T>
    template <class T, std::size_t N, class... args_t>
	ds_t write( const ds_t& ds, const T(& list)[N], args_t&&... args  ) try {
        H5CPP_WRITE_DISPATCH_TEST_(T, "write(ds, const T(&ref)[N], args...)")
	    return h5::write<T[N]>(ds, list, args...);
	} catch ( const std::runtime_error& err ){
		throw error::io::dataset::write( err.what() );
	}

    // T[N][..] and std::initializer_list<T>
    template <class T, std::size_t N, class... args_t>
    h5::ds_t write( const fd_t& fd, const std::string& dataset_path, const T(& list)[N], args_t&&... args  ) try {
        H5CPP_WRITE_DISPATCH_TEST_(T, "write<T>(fd, path, const T(&ref)[N], args...)")
	    return h5::write<T[N]>(fd, dataset_path, list, args...);
	} catch ( const std::runtime_error& err ){
		throw error::io::dataset::write( err.what() );
	}

    // CONVERSION: T[N][..] and std::initializer_list<T>
    template <class F, class T, std::size_t N, class... args_t>
    typename std::enable_if<!std::is_same<T,F>::value,
    h5::ds_t>::type write( const fd_t& fd, const std::string& dataset_path, const T(& ref)[N], args_t&&... args  ) try {
        H5CPP_WRITE_DISPATCH_TEST_(T, "write<F,T>(fd, path, const T(&ref)[N], args...)")
		using element_t = typename impl::decay<T>::type;
        using tcurrent_dims = arg::tpos<h5::current_dims_t,args_t...>;
        using tcount = arg::tpos<h5::count_t,args_t...>;
        mute();
        ds_t ds;
        if( H5Lexists(fd, dataset_path.c_str(), H5P_DEFAULT ) > 0 ) {
            const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
            ds = h5::open(fd, dataset_path, dapl);
        } else {
            if constexpr(!tcurrent_dims::present && !impl::is_scalar<T>::value && !impl::is_array<T>::value){
                h5::current_dims_t current_dims = impl::get_current_dims(ref, args...);
                ds = h5::create<F>(fd, dataset_path, current_dims, args...);
                // we just created the matching file space, we can fast track it with S_ALL, S_ALL
                return impl::write_all(ds, ref, args...);
            } else ds = h5::create<F>(fd, dataset_path, args...);
        }
		h5::unmute();
        if constexpr (!tcount::present) {
            // `count{..}` is not specified, has to be retrieved from `ref` object
            const h5::count_t count = impl::size(ref);
            h5::write(ds, ref, count, args...);
        } else h5::write(ds, ref, args...);
        return ds; //h5::write<T[N]>(fd, dataset_path, ref, args...);
	} catch ( const std::runtime_error& err ){
		throw error::io::dataset::write( err.what() );
	}

    template <class... args_t>
	h5::ds_t write( const std::string& file_path, const std::string& dataset_path, args_t&&... args  ){
        H5CPP_WRITE_DISPATCH_TEST_(T, "file...")
		h5::fd_t fd = h5::open( file_path, H5F_ACC_RDWR, h5::default_fapl );
		return h5::write( fd, dataset_path, args...);
	}
}
#endif
