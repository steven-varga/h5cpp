/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_SALL_HPP
#define  H5CPP_SALL_HPP

namespace h5{ namespace impl {
	struct max_dims_t{}; struct current_dims_t{};
	struct dims_t{}; struct chunk_t{};  struct offset_t{}; struct stride_t{};  struct count_t{}; struct block_t{};

	template <typename T>
	struct array  {

		array( const std::initializer_list<hsize_t> list  ) : rank( list.size() ) {
			for(int i=0; i<rank; i++) data[i] = *(list.begin() + i);
		}
		array() = default;
		array( array&& arg ) = default;
		array( array& arg ) = default;
		array& operator=( array&& arg ) = default;
		array& operator=( array& arg ) = default;
		hsize_t& operator[](size_t i){ return *(data + i); }
		const hsize_t& operator[](size_t i) const { return *(data + i); }
		hsize_t* operator*() { return data; }
		const hsize_t* operator*() const { return data; }

		friend std::ostream & operator<<(std::ostream& cout, const array& arr){
			int i=0;
			cout << "{"; for(;i<arr.rank-1; i++) cout << arr[i] << ","; cout << arr[i] << "}";
			return cout;
		}

		using type = T;
		size_t size() const { return rank; }
		const hsize_t* begin()const { return data; }
		hsize_t* begin() { return data; }
		int rank;
		hsize_t data[H5CPP_MAX_RANK];
	};
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
}
#endif

