/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#include <hdf5.h>
#include "macros.h"
#include "utils.hpp"
#include <limits>
#include <initializer_list>

#ifndef  H5CPP_CREATE_H 
#define  H5CPP_CREATE_H

namespace h5 { namespace impl {
//WARNING:  `::hid_t` refers to CAPI identifier, h5::impl::hid_t<T> is a templated internal wrap with RAII, prefix with double colon when using it 
// in h5::impl namespace

	inline ::hid_t create(::hid_t fd, const char* path,
			size_t rank,  const hsize_t* max_dims, ::hid_t lcpl, ::hid_t dcpl, ::hid_t dapl, ::hid_t type ){

		hsize_t current_dims[H5CPP_MAX_RANK];
		hsize_t chunk_dims[H5CPP_MAX_RANK];
		int chunk_rank = H5Pget_chunk(dcpl, H5CPP_MAX_RANK, chunk_dims );
		// add support for chunks only if specified
		if( chunk_rank > 0 ){
			// set current dimensions to given one or zero if H5S_UNLIMITED
			// this mimics matlab(tm) behavior while allowing extendable matrices
			for(hsize_t i=0;i<rank;i++)
				current_dims[i] = max_dims[i] != H5S_UNLIMITED ? max_dims[i] : static_cast<hsize_t>(0);
		} else
			for(hsize_t i=0;i<rank;i++) current_dims[i] = max_dims[i];

		::hid_t space = H5Screate_simple( rank, current_dims, max_dims );
		::hid_t ds = H5Dcreate2(fd, path, type, space, lcpl, dcpl, dapl);
		H5Sclose(space);
		return -1;
	}
}}



/* @namespace h5
 * @brief public namespace
 */
namespace h5 {

	/** \ingroup io-create 
	 * \brief **T** template parameter defines the underlying representation of document created within HDF5 container 
	 * referenced by **fd** descriptor.
	 * **path** behaves much similar to POSIX files system path: either relative or absolute. HDF5 supports 
	 * arbitrary number of dimensions which is specified by **max_dim**, and **chunk_size** controls how this 
	 * array is accessed. When chunked access is used keep in mind small values may result in excessive 
	 * data cache operations.<br>
	 * \par_fd \par_dataset_path \par_max_dims \par_chunk_dims \par_deflate \tpar_T \tpar_FD \returns_ds
	 * \sa_h5cpp \sa_hdf5 \sa_stl \sa_linalg 
	 *
	 * @code
	 * example:
	 * 		h5::create<double>(fd, "matrix/double type",{100,10},{1,10}, 9); 	 
	 * 		h5::create<short>(fd, "array/short",{100,10,10});          			 
	 * 		h5::create<float>(fd, "stream",{H5S_UNLIMITED},{10}, 9); 			
	 *  																				
	 * 		auto ds = h5::create<float>(fd,"stl/partial",{100,vf.size()},{1,10}, 9);    
	 * 			h5::write(ds, vf, {2,0},{1,10} ); 										 	
	 * 			auto rvo  = h5::read< std::vector<float>>(ds); 							
		* @endcode 
	 */
	template <typename T> inline h5::ds_t create(const fd_t& fd, const std::string& path,
			h5::dims_t max_dims, const h5::lcpl_t& lcpl, const h5::dcpl_t& dcpl=H5P_DEFAULT, const h5::dapl_t& dapl=H5P_DEFAULT ) {
		hid_t type = utils::h5type<T>();
		::hid_t ds = impl::create(static_cast<hid_t>(fd), path.data(), max_dims.size(),  max_dims.begin(),
				static_cast<hid_t>(lcpl), static_cast<hid_t>(dcpl), static_cast<hid_t>(dapl), type );
		H5Tclose(type);
		return ds_t{ds};
   	}
	template <typename T> inline h5::ds_t create(const fd_t& fd, const std::string& path,
			h5::dims_t max_dims, const h5::dcpl_t& dcpl=H5P_DEFAULT ) {

		return ds_t{-1};
   	}


	/** \ingroup io-create 
	 * @brief create dataset within HDF5 file space with dimensions extracted from references object 
	 * \par_fd \par_dataset_path \par_ref \tpar_T \tpar_FD \returns_ds
	 * \sa_h5cpp \sa_hdf5 \sa_stl \sa_linalg  
	 * \code
	 * example:
	 * 	#include <hdf5.h>
	 * 	#include <armadillo>
	 * 	#include <h5cpp/all>
	 * 
	 *  int main(){
	 * 		auto fd = h5::create("some_file.h5", H5F_ACC_TRUNC);   // create file, fd is RAII, closes when leaving scope
	 * 		arma::mat M(100,10); 						           // define object
	 * 		h5::create(fd,"matrix",M); 		                       // create dataset from object
	 * 	}                                                          // RAII: fd gets closed as leaving scope
	 * \endcode
	 */
	template<typename T, typename fd_t=h5::fd_t, typename BaseType = typename utils::base<T>::type, size_t Rank = utils::base<T>::rank >
		inline hid_t create(const fd_t& fd, const std::string& path, const T& ref ){

		std::array<hsize_t,Rank> max_dims = h5::utils::get_dims( ref );
		std::array<hsize_t,Rank> chunk_dims={0}; // initialize to zeros
		//TODO: acommodate multiple hid_t -s and recursive free  for  utils::h5type<BaseType>()
 		//return impl::create(fd,path,Rank,max_dims.data(),chunk_dims.data(),H5CPP_NO_COMPRESSION, utils::h5type<BaseType>());
		return ds_t{-1};
	}
}
#endif

