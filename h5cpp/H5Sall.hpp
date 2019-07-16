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
		array& operator=( array&& arg ) = default;
		array& operator=( array& arg ) = default;
		template<class C>
		explicit operator array<C>(){
			array<C> arr;
			arr.rank = rank;
			for( int i=0; i<rank; i++) arr[i] = data[i];
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
	}
#endif
