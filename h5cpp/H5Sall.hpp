/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_SALL_HPP
#define  H5CPP_SALL_HPP

namespace h5{ namespace impl {
	struct max_dims_t{}; struct current_dims_t{};
	struct dims_t{}; struct chunk_t{};  struct offset_t{}; struct stride_t{};  struct count_t{}; struct block_t{};

	template <typename T, int N=H5CPP_MAX_RANK>
	struct array  {

		array( const std::initializer_list<size_t> list  ) : rank( list.size() ) {
			for(int i=0; i<rank; i++) data[i] = *(list.begin() + i);
		}
		// support linalg objects upto 3 dimensions or cubes
		template<class A> array( const std::array<A,0> l ) : rank(0), data{} {}
		template<class A> array( const std::array<A,1> l ) : rank(1), data{l[0]} {}
		template<class A> array( const std::array<A,2> l ) : rank(2), data{l[0],l[1]} {}
		template<class A> array( const std::array<A,3> l ) : rank(3), data{l[0],l[1],l[2]} {}

		/** @brief computes the total space 
		 */	
		explicit operator const hsize_t () const {
			hsize_t size = 1; 
			for(hsize_t i=0; i<rank; i++) size *= data[i];
			return size;
		}
		// automatic conversion to std::array means to collapse tail dimensions
		template<class A>
		operator const std::array<A,0> () const {
			return {};
		}
		template<class A>
		operator const std::array<A,1> () const {
			size_t a = data[0];
			for(int i=1;i<rank;i++) a*=data[i];
			return {a};
		}
		template<class A>
		operator const std::array<A,2> () const {
			size_t a,b;	a = data[0]; b = data[1];
			for(int i=2;i<rank;i++) b*=data[i];
			return {a,b};
		}
		template<class A>
		operator const std::array<A,3> () const {
			size_t a,b,c;	a = data[0]; b = data[1]; c = data[2];
			for(int i=3;i<rank;i++) c*=data[i];
			return {a,b,c};
		}

		array() : rank(0){};
		array( array&& arg ) = default;
		array( array& arg ) = default;
		array(const array& arg) = default;
		array& operator=( array&& arg ) = default;
		array& operator=( array& arg ) = default;
		template<class C>
		explicit operator array<C>(){
			array<C> arr;
			arr.rank = rank;
			hsize_t i=0;
			for(; i<rank; i++) arr[i] = data[i];
			// the shape/dimension may be different, be certain the 
			// remaining is initilized to ones
			for(; i<N; i++) arr[i] = 1;

			return arr;
		}

		hsize_t& operator[](size_t i){ return *(data + i); }
		const hsize_t& operator[](size_t i) const { return *(data + i); }
		hsize_t* operator*() { return data; }
		const hsize_t* operator*() const { return data; }

		using type = T;
		size_t size() const { return rank; }
		const hsize_t* begin()const { return data; }
		hsize_t* begin() { return data; }
		int rank;
		hsize_t data[N];
	};

	template <class T> inline
	size_t nelements( const h5::impl::array<T>& arr ){
		size_t size = 1;
		for( int i=0; i<arr.rank; i++) size *= arr[i];
		return size;
	}
}}


/*PUBLIC CALLS*/
namespace h5 {
	using max_dims_t       = impl::array<impl::max_dims_t>;
	using current_dims_t   = impl::array<impl::current_dims_t>;
	using dims_t   = impl::array<impl::dims_t>;
	using chunk_t  = impl::array<impl::chunk_t>;
	using offset_t = impl::array<impl::offset_t>;
	using stride_t = impl::array<impl::stride_t>;
	using count_t  = impl::array<impl::count_t>;
	using block_t  = impl::array<impl::block_t>;

	using offset = offset_t;
	using stride = stride_t;
	using count  = count_t;
	using block  = block_t;
	using dims   = dims_t;
	using current_dims   = current_dims_t;
	using max_dims       = max_dims_t;

	//*< usage: `const h5::stride_t& stride = h5::impl::arg::get( h5::default_stride, args...);` */
	const static h5::count_t   default_count{1,1,1,1,1,1,1};  /**< used as block in hyperblock selection as default value */
	const static h5::offset_t  default_offset{0,0,0,0,0,0,0}; /**< used as offset in hyperblock selection as default value */
	const static h5::stride_t  default_stride{1,1,1,1,1,1,1}; /**< used as stride in hyperblock selection as default value */
	const static h5::block_t   default_block{1,1,1,1,1,1,1};  /**< used as block in hyperblock selection as default value */
}
namespace h5::impl {
	/** \ingroup internal
	 * @brief computes `h5::current_dims_t` considering the context 
	 * Current and maximum dimension of an HDF5 object is how much space is available for immediate or further use within a dataset.
	 * This routing computes the size along each dimesnion of `current_dims` and it considers the object size given by `count`, its 
	 * coordinates within the filespace `h5::offset`, how it is spaced `h5::stride` and the `h5::block` size.
	 * @param count rank and size of the object along each dimension
	 * @tparam T C++ type of dataset being written into HDF5 container
	 *
	 * <br/>The following arguments are context sensitive, may be passed in arbitrary order and with the exception
	 * of `T object` being saved, the arguments are optional. The arguments are set to sensible values, and in most cases
	 * will provide good performance by default, with that in mind, it is an easy high level fine tuning mechanism to 
	 * get the best experience witout trading readability. 
	 *
	 * @param h5::stride_t skip this many blocks along given dimension
	 * @param h5::block_t only used when `stride` is specified 
	 * @param h5::offset_t writes `T object` starting from this coordinates, considers this shift into `h5::current_dims` when applicable
	 */ 
	template <class... args_t> inline 
	h5::current_dims_t get_current_dims(const h5::count_t& count,  args_t&&... args ){
		// premise: h5::current_dims{} is not present
		using toffset = typename arg::tpos<const h5::offset_t&,const args_t&...>;
		using tstride = typename arg::tpos<const h5::stride_t&,const args_t&...>;
		using tblock = typename arg::tpos<const h5::block_t&,const args_t&...>;

		h5::current_dims_t current_dims;
		hsize_t rank = current_dims.rank = count.rank;

		for(hsize_t i =0; i < rank; i++ )
			current_dims[i] = count[i];
		// TODO: following syntax looks better
		//auto current_dims = static_cast<h5::current_dims_t>(count);

		if constexpr( tstride::present ){ // when stride is not specified block == count
			const h5::stride_t& stride =  std::get<tstride::value>( std::forward_as_tuple( args...) );
			if constexpr( tblock::present ){ // tricky, we have block as well
				const h5::block_t& block =  std::get<tblock::value>( std::forward_as_tuple( args...) );
				for(hsize_t i=0; i < rank; i++)
					current_dims[i] *= (stride[i] - block[i] + 1);
			} else // block is not present, we are stretching `current_dims` with `stride`
				for(hsize_t i=0; i < rank; i++)
					current_dims[i] *= stride[i];
		}
		// we increment dimension with the specified offset, if any
		if constexpr( toffset::present ){
			const h5::offset_t& offset =  std::get<toffset::value>( std::forward_as_tuple( args...) );
			for(hsize_t i=0; i < rank; i++)
				current_dims[i]+=offset[i];
		}
		return current_dims;
	}
}

#endif
