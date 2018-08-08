/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */
#ifndef  H5CPP_DCREATE_HPP 
#define  H5CPP_DCREATE_HPP


namespace h5{ namespace arg {
	// declaration
	template<class S, class... T > struct tpos;
	namespace detail {
		// declaration
		template<class search_pattern, int position, int count, bool branch, class prev_head, class arguments> struct tuple_pos;
		// initialization case 
		template<class S, int P, int C, bool B, class not_used, class... Tail >
		struct tuple_pos<S, P,C, B, not_used, std::tuple<Tail...>>
			: tuple_pos<S, P,C, false, not_used, std::tuple<Tail...>> { };
		// recursive case 
		template<class S, int P, int C, class not_used, class Head, class... Tail >
		struct tuple_pos<S, P,C,false, not_used, std::tuple<Head, Tail...>>
			: tuple_pos<S, P+1,C, std::is_convertible<Head,S>::value, Head, std::tuple<Tail...>> { };
		// match case 
		template<class S, int P, int C, class Type, class... Tail >
		struct tuple_pos<S, P,C,true, Type, std::tuple<Tail...>>
			: std::integral_constant<int,P>{
			using type  = Type;
			static constexpr bool present = true;
		 };
		// default case
		template<class S, class H, int P, int C>
		struct tuple_pos<S, P,C, false, H, std::tuple<>>
			: std::integral_constant<int,-1>{
			static constexpr bool present = false;
		};
	}
	template<class S, class... args_t > struct tpos
		: detail::tuple_pos<const S&,-1,0,false, void, std::tuple<args_t...>>{ };

	template <class S, class... args_t, 
			 class idx = tpos</*search: */const S&,/*args: */ const args_t&...,const S&>>
	auto& get( const S& def, args_t&&... args ){
		auto tuple = std::forward_as_tuple(args..., def );
		return std::get<idx::value>( tuple );
	}
	template <int idx, class... args_t>
	auto& getn(args_t&&... args ){
		auto tuple = std::forward_as_tuple(args...);
		return std::get<idx>( tuple );
	}
}}

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
	template<class T, class... args_t>
	h5::ds_t create( const h5::fd_t& fd, const std::string& dataset_path, args_t&&... args ){
		// compile time check of property lists: 
		using tcurrent_dims = typename arg::tpos<const h5::current_dims_t&,const args_t&...>;
		using tmax_dims 	= typename arg::tpos<const h5::max_dims_t&,const args_t&...>;
		using tlcpl 		= typename arg::tpos<const h5::lcpl_t&,const args_t&...>;
		using tdcpl 		= typename arg::tpos<const h5::dcpl_t&,const args_t&...>;
		using tdapl 		= typename arg::tpos<const h5::dapl_t&,const args_t&...>;
		//TODO: make copy of default dcpl
		h5::dcpl_t default_dcpl{ H5Pcreate(H5P_DATASET_CREATE) };
		// get references to property lists or system default values 
		const h5::lcpl_t& lcpl = arg::get(h5::default_lcpl, args...);
		const h5::dcpl_t& dcpl = arg::get(default_dcpl, args...);
		const h5::dapl_t& dapl = arg::get(h5::default_dapl, args...);
		// and dimensions

		h5::current_dims_t current_dims_default{0}; // if no current dims_present 
		// this mutable value will be referenced
		const h5::current_dims_t& current_dims = arg::get(current_dims_default, args...);
		const h5::max_dims_t&     max_dims =     arg::get(h5::max_dims_t{0}, args... );
		bool has_unlimited_dimension = false;
		size_t rank = 0;
		h5::sp_t space_id{-1}; // 

		if constexpr( tmax_dims::present ){
			rank = max_dims.size();
			if constexpr( !tcurrent_dims::present ){
				// set current dimensions to given one or zero if H5S_UNLIMITED mimics matlab(tm) behavior while allowing extendable matrices
				for(hsize_t i=0; i<rank; i++)
					current_dims_default[i] = max_dims[i] != H5S_UNLIMITED
						? max_dims[i] : (has_unlimited_dimension=true, static_cast<hsize_t>(0));
				current_dims_default.rank = rank;
			}
		} else static_assert( tcurrent_dims::present,"current or max dimensions must be present in order to create a dataset!" );

		if( !tdcpl::present && ( has_unlimited_dimension || (tcurrent_dims::present && tmax_dims::present)) ){
			current_dims_t chunk{0}; chunk.rank = current_dims.rank;
			for(hsize_t i=0; i<rank; i++)
				chunk[i] = current_dims[i] ? current_dims[i] : 1;
			H5Pset_chunk(static_cast<::hid_t>(default_dcpl), chunk.rank, chunk.begin() );
		}

		// use move semantics to set space
		space_id =  tmax_dims::present ?
			std::move( h5::create_simple( current_dims, max_dims ) ) :  std::move( h5::create_simple( current_dims ) );

		hid_t type = utils::h5type<T>();
		::hid_t ds = H5Dcreate2( static_cast<hid_t>(fd), dataset_path.data(), type, static_cast<hid_t>(space_id),
								static_cast<hid_t>(lcpl), static_cast<hid_t>(dcpl), static_cast<hid_t>(dapl) );
		H5Tclose(type);
		return h5::ds_t{ds};
	}
	// delegate to h5::fd_t 
	template<class T, class... args_t>
	inline h5::ds_t create( const std::string& file_path, const std::string& dataset_path, args_t&&... args ){
		h5::fd_t fd = h5::open(file_path, H5F_ACC_RDWR, h5::default_fapl);
		return h5::create<T>(fd, dataset_path, args...);
	}
}
#endif

