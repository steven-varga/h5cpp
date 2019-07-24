/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

/**
 * @file This file contains size types for different uses in hdf5
 * (e.g. data set dimensions, chunk dimensions or strides)
 */

#ifndef  H5CPP_SALL_HPP
#define  H5CPP_SALL_HPP

namespace h5::impl {
	/**
	 * @brief Tag usen in combination with \ref h5::impl::array.
	 * @see h5::impl::array
	 * @see h5::max_dims_t
	 */
	struct max_dims_t{};
	/**
	 * @brief Tag usen in combination with \ref h5::impl::array.
	 * @see h5::impl::array
	 * @see h5::current_dims_t
	 */
	struct current_dims_t{};
	/**
	 * @brief Tag usen in combination with \ref h5::impl::array.
	 * @see h5::impl::array
	 * @see h5::dims_t
	 */
	struct dims_t{};
	/**
	 * @brief Tag usen in combination with \ref h5::impl::array.
	 * @see h5::impl::array
	 * @see h5::chunk_t
	 */
	struct chunk_t{};
	/**
	 * @brief Tag usen in combination with \ref h5::impl::array.
	 * @see h5::impl::array
	 * @see h5::offset_t
	 */
	struct offset_t{};
	/**
	 * @brief Tag usen in combination with \ref h5::impl::array.
	 * @see h5::impl::array
	 * @see h5::stride_t
	 */
	struct stride_t{};
	/**
	 * @brief Tag usen in combination with \ref h5::impl::array.
	 * @see h5::impl::array
	 * @see h5::count_t
	 */
	struct count_t{};
	/**
	 * @brief Tag usen in combination with \ref h5::impl::array.
	 * @see h5::impl::array
	 * @see h5::block_t
	 */
	struct block_t{};

	/**
	 * @brief A tagged array of hsize_t to store dimensions of chunks, offsets, etc.
	 * @see h5::max_dims_t
	 * @see h5::current_dims_t
	 * @see h5::dims_t
	 * @see h5::chunk_t
	 * @see h5::offset_t
	 * @see h5::stride_t
	 * @see h5::count_t
	 * @see h5::block_t
	 */
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
}

/*PUBLIC CALLS*/
namespace h5 {
	/**
	 * @brief Tagged array used to specify the maximum dimensions of a dataset.
	 * @see h5::impl::array
	 * @see h5::impl::max_dims_t
	 */
	using max_dims_t       = impl::array<impl::max_dims_t>;
	/**
	 * @brief Tagged array used to specify the current dimensions of a dataset.
	 * @see h5::impl::array
	 * @see h5::impl::current_dims_t
	 */
	using current_dims_t   = impl::array<impl::current_dims_t>;
	/**
	 * @brief Tagged array used to describe the dimensions of a datatype.
	 * @see h5::impl::array
	 * @see h5::impl::dims_t
	 */
	using dims_t   = impl::array<impl::dims_t>;
	/**
	 * @brief Tagged array used to specify the chunk size used by hdf5 for a dataset.
	 * @see h5::impl::array
	 * @see h5::impl::chunk_t
	 */
	using chunk_t  = impl::array<impl::chunk_t>;
	/**
	 * @brief Tagged array used to specify the offset when selecting a hyperslab
	 * in a hdf5 dataset.
	 *
	 * This is the first blocks offset in the dataset.
	 * Further reading in the <a href="https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-SelectHyperslab">reference</a>,
	 * the <a href="https://support.hdfgroup.org/HDF5/doc/UG/HDF5_Users_Guide-Responsive%20HTML5/index.html#t=HDF5_Users_Guide%2FDataspaces%2FHDF5_Dataspaces_and_Partial_I_O.htm%23TOC_7_4_2_Programming_Modelbc-8&rhtocid=7.2.0_2">user manual</a>
	 * or on <a href="https://stackoverflow.com/a/46557034">stack overflow</a>.
	 * @see h5::impl::array
	 * @see h5::impl::offset_t
	 */
	using offset_t = impl::array<impl::offset_t>;
	/**
	 * @brief Tagged array used to specify the stride when selecting a hyperslab
	 * in a hdf5 dataset.
	 *
	 * This is the stride between blocks selected from the dataset.
	 * Further reading in the <a href="https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-SelectHyperslab">reference</a>,
	 * the <a href="https://support.hdfgroup.org/HDF5/doc/UG/HDF5_Users_Guide-Responsive%20HTML5/index.html#t=HDF5_Users_Guide%2FDataspaces%2FHDF5_Dataspaces_and_Partial_I_O.htm%23TOC_7_4_2_Programming_Modelbc-8&rhtocid=7.2.0_2">user manual</a>
	 * or on <a href="https://stackoverflow.com/a/46557034">stack overflow</a>.
	 * @see h5::impl::array
	 * @see h5::impl::stride_t
	 */
	using stride_t = impl::array<impl::stride_t>;
	/**
	 * @brief Tagged array used to specify the block count when selecting a hyperslab
	 * in a hdf5 dataset.
	 *
	 * This is the number of blocks selected from the dataset
	 * along its different dimensions.
	 * Further reading in the <a href="https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-SelectHyperslab">reference</a>,
	 * the <a href="https://support.hdfgroup.org/HDF5/doc/UG/HDF5_Users_Guide-Responsive%20HTML5/index.html#t=HDF5_Users_Guide%2FDataspaces%2FHDF5_Dataspaces_and_Partial_I_O.htm%23TOC_7_4_2_Programming_Modelbc-8&rhtocid=7.2.0_2">user manual</a>
	 * or on <a href="https://stackoverflow.com/a/46557034">stack overflow</a>.
	 * @see h5::impl::array
	 * @see h5::impl::count_t
	 */
	using count_t  = impl::array<impl::count_t>;
	/**
	 * @brief Tagged array used to specify the block size when selecting a hyperslab
	 * in a hdf5 dataset.
	 *
	 * This is the size of one block selected from the dataset.
	 * Further reading in the <a href="https://support.hdfgroup.org/HDF5/doc/RM/RM_H5S.html#Dataspace-SelectHyperslab">reference</a>,
	 * the <a href="https://support.hdfgroup.org/HDF5/doc/UG/HDF5_Users_Guide-Responsive%20HTML5/index.html#t=HDF5_Users_Guide%2FDataspaces%2FHDF5_Dataspaces_and_Partial_I_O.htm%23TOC_7_4_2_Programming_Modelbc-8&rhtocid=7.2.0_2">user manual</a>
	 * or on <a href="https://stackoverflow.com/a/46557034">stack overflow</a>.
	 * @see h5::impl::array
	 * @see h5::impl::block_t
	 */
	using block_t  = impl::array<impl::block_t>;

	///@see h5::offset_t
	using offset = offset_t;
	///@see h5::stride_t
	using stride = stride_t;
	///@see h5::count_t
	using count  = count_t;
	///@see h5::block_t
	using block  = block_t;
	///@see h5::dims_t
	using dims   = dims_t;
	///@see h5::current_dims_t
	using current_dims   = current_dims_t;
	///@see h5::max_dims_t
	using max_dims       = max_dims_t;
	}
#endif
