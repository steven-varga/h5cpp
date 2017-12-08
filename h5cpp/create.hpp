/*
 * Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this  software  and associated documentation files (the "Software"), to deal in
 * the Software  without   restriction, including without limitation the rights to
 * use, copy, modify, merge,  publish,  distribute, sublicense, and/or sell copies
 * of the Software, and to  permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE  SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT  SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY  CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER
 * IN  AN  ACTION  OF  CONTRACT, TORT OR  OTHERWISE, ARISING  FROM,  OUT  OF  OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef  H5CPP_CREATE_H 
#define H5CPP_CREATE_H


namespace h5 { namespace impl {
	hid_t create(hid_t fd, const std::string& path_,
			size_t rank,  const hsize_t* max_dims,  const hsize_t* chunk_dims, int32_t deflate, hid_t type){

		hsize_t current_dims[rank];
		hid_t group, dcpl,space, ds;

		std::pair<std::string, std::string> path = h5::utils::split_path( path_ );
		// creates path if doesn't exists otherwise opens it
		if(!path.first.empty()){
			group = h5::utils::group_exist(fd,path.first,true);
			if( group < 0)
				return group;
			}else
				group = fd;

		dcpl = H5Pcreate(H5P_DATASET_CREATE);
		// NaN is platform and type dependent, branch out on native types
		if( H5Tequal(H5T_NATIVE_DOUBLE, type) ){
			double value = std::numeric_limits<double>::quiet_NaN();
			H5Pset_fill_value( dcpl, type, &value  );
		}else if( H5Tequal(H5T_NATIVE_FLOAT, type) ){
			float value = std::numeric_limits<float>::quiet_NaN();
			H5Pset_fill_value( dcpl, type, &value );
		}else if( H5Tequal(H5T_NATIVE_LDOUBLE, type) ){
			long double value = std::numeric_limits<long double>::quiet_NaN();
			H5Pset_fill_value( dcpl, type, &value );
		}

		// this prevents unreadable datasets in hdf-view or julia
		H5Pset_fill_time(dcpl,H5D_FILL_TIME_ALLOC);
		// add support for chunks only if specified
		if( *chunk_dims ){
			// set current dimensions to given one or zero if H5S_UNLIMITED
			// this mimics matlab(tm) behavior while allowing extendable matrices
			for(int i=0;i<rank;i++)
				current_dims[i] = max_dims[i] != H5S_UNLIMITED ? max_dims[i] : static_cast<hsize_t>(0);

			H5Pset_chunk(dcpl, rank, chunk_dims);
			if( deflate ) H5Pset_deflate (dcpl, deflate);
		} else
			for(int i=0;i<rank;i++) current_dims[i] = max_dims[i];

		space = H5Screate_simple( rank, current_dims, max_dims );
		ds = H5Dcreate2(group, path.second.data(), type, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
		if( !path.first.empty() )
			H5Gclose( group );
		H5Pclose(dcpl); H5Sclose( space); H5Tclose(type);
		return ds;
	}

}}



/* @namespace h5
 * @brief public namespace
 */
namespace h5 {

	/** \ingroup io-create 
	 * @brief create dataset within HDF5 file space with given properties  
	 * @param fd opened HDF5 file descripor
	 * @param path full path where the newly created object will be placed
	 * @param max_dims size of the object, H5S_UNLIMITED to mark extendable dimension
	 * @param chunk_dims for better performance data sets maybe stored in chunks, which is a unit size 
	 * 		  for IO operations. Streaming, and filters may be applied only on chunked datsets.
	 * @param deflate 0-9 controls <a href="https://support.hdfgroup.org/HDF5/Tutor/compress.html"> GZIP compression</a>
	 * @tparam T [unsigned](char|short|int|long long) | (float|double)    
	 * @return opened dataset descriptor of hid_t, that must be closed with H5Dclose  
	 * \code
	 * examples:
	 * 		hid_t ds = create<double>(fd, "matrix/double type",{100,10},{1,10}, 9); 	// create a matrix with level 9 gzip compression 
	 * 		hid_t ds = create<short>(fd, "array/short",{100,10,10});          			// a cube of shorts, no compression or chunks 
	 * 		hid_t ds = create<float>(fd, "stream",{H5S_UNLIMITED},{10}, 9); 			// extensible dataset to record a stream
	 *  																				//
	 * 		hid_t ds = h5::create<float>(fd,"stl/partial",{100,vf.size()},{1,10}, 9);   // creating a dataset, then saving an stl::vector 
	 * 			h5::write(ds, vf, {2,0},{1,10} ); 										// write data into 4th row 	
	 * 			auto rvo  = h5::read< std::vector<float>>(ds); 							// read back dataset as stl::vector
	 * 		H5Dclose(ds); 																// closing descriptor
	 * \endcode  
	 */
	template <typename T> hid_t create(hid_t fd, const std::string& path,
			std::initializer_list<hsize_t> max_dims, std::initializer_list<hsize_t> chunk_dims={},
			const int32_t deflate = H5CPP_NO_COMPRESSION ){

		return impl::create(fd,path,max_dims.size(), max_dims.begin(), chunk_dims.begin(), deflate, utils::h5type<T>() );
   	}

	/** \ingroup io-create 
	 * @brief create dataset within HDF5 file space with dimensions extracted from references object 
	 * @param fd opened HDF5 file descripor
	 * @param path full path where the newly created object will be placed
	 * @param ref stl|arma|eigen valid templated object with dimensions       
	 * @tparam T [unsigned](char|short|int|long long) | (float|double)    
	 * @return opened dataset descriptor of hid_t, that must be closed with H5Dclose 
	 * \code
	 * example:
	 * 	#include <hdf5.h>
	 * 	#include <armadillo>
	 * 	#include <h5cpp>
	 *  
	 *  int main(){
	 * 		hid_t fd = h5::create("some_file.h5"); 		// create file
	 * 		arma::mat M(100,10); 						// define object
	 * 		hid_t ds = h5::create(fd,"matrix",M); 		// create dataset from object
	 * 		H5Dclose(ds); 								// close dataset
	 * 		h5::close(fd); 								// and file descriptor
	 * 	}
	 * \endcode 
	 */
	template<typename T, typename BaseType = typename utils::base<T>::type, size_t Rank = utils::base<T>::rank >
		hid_t create(  hid_t fd, const std::string& path, const T ref ){

		std::array<hsize_t,Rank> max_dims = utils::dims( ref );
		std::array<hsize_t,Rank> chunk_dims={}; // initialize to zeros
 		return impl::create(fd,path,Rank,max_dims.data(),chunk_dims.data(),H5CPP_NO_COMPRESSION, utils::h5type<BaseType>());
	}
}
#endif

