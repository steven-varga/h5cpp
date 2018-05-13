/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include <hdf5.h>
#include "macros.h"
#include "misc.hpp"
#include <initializer_list>

#ifndef  H5CPP_READ_H 
#define  H5CPP_READ_H
namespace h5{ namespace impl{ // implementation details namespace

	template<typename T> inline T* read( hid_t ds, T* ptr, const hsize_t* offset, const hsize_t* count ) noexcept {

		hid_t file_space = H5Dget_space(ds);
        hsize_t rank = H5Sget_simple_extent_ndims(file_space);
		hid_t mem_type = utils::h5type<T>();
		hid_t mem_space  = H5Screate_simple(rank,count,NULL);
		H5Sselect_all(mem_space);


		H5Sselect_hyperslab(file_space, H5S_SELECT_SET, offset, NULL, count, NULL);
		H5Dread(ds, mem_type, mem_space, file_space, H5P_DEFAULT, ptr );
        H5Tclose(mem_type); H5Sclose(file_space); H5Sclose(mem_space);
		return ptr;
	}

	inline std::vector<std::string> read(hid_t ds, const hsize_t* offset, const hsize_t* count ) noexcept {

		hid_t file_space = H5Dget_space(ds);
		hsize_t rank = H5Sget_simple_extent_ndims(file_space);
		hid_t mem_space  = H5Screate_simple(rank,count,NULL);
		hid_t mem_type = utils::h5type<std::string>();
		H5Sselect_all(mem_space);

		std::vector<std::string> data_set = utils::ctor<std::vector<std::string>>(rank, count );

		// read
		char ** ptr = static_cast<char **>(
						malloc( data_set.size() * sizeof(char *)));

		H5Sselect_hyperslab(file_space, H5S_SELECT_SET, offset, NULL, count, NULL);
		H5Dread(ds, mem_type, mem_space, file_space, H5P_DEFAULT, ptr );

		for(hsize_t i=0; i<data_set.size(); i++)
			if( ptr[i] != NULL )
				data_set[i] = std::string( ptr[i] );

		H5Dvlen_reclaim (mem_type, mem_space, H5P_DEFAULT, ptr);
        free(ptr);
		H5Sclose(file_space); H5Sclose(mem_space); H5Tclose(mem_type);

		// end read	
		return data_set;
	}

}}

namespace h5 {
	/** \ingroup io-read 
	 * @brief reads entire HDF5 dataset and returns object defined by T template
	 * \par_ds \tpar_obj \tpar_DS \returns_object
	 * \sa_hdf5 \sa_h5cpp \sa_linalg \sa_stl 
	 * @code
	 * example:
	 *     auto ds = h5::open("some_file,h5","some_dataset");
	 *     // returns with std::move semantics
	 *     stl::vector<float> entire_dataset = h5::read<stl::vector<float>>( ds );		
	 * @endcode 
	 */
	template<typename T,  typename DS=h5::ds_t, typename BaseType = typename utils::base<T>::type> 
		inline T read( const DS& ds ) noexcept {
		hid_t ds_ = static_cast<hid_t>(ds);
		hid_t file_space = H5Dget_space( ds_ );
		hsize_t offset[H5CPP_MAX_RANK]={0}; // all zeros
		hsize_t count[H5CPP_MAX_RANK];
		hsize_t rank = H5Sget_simple_extent_dims(file_space, count, NULL);
		T data_set = utils::ctor<T>(rank, count );
		BaseType * ptr = utils::get_ptr( data_set );
		impl::read(ds_, ptr, offset, count);
		return data_set;
	}

	/** \ingroup io-read 
	 * @brief partial reads HDF5 dataset into a memory region defined by pointer
	 * \par_ds  \par_ptr \par_offset \par_count  \tpar_obj \tpar_DS \returns_object
	 * \sa_hdf5 \sa_h5cpp \sa_linalg \sa_stl 
	 */
	template<typename T,  typename DS=h5::ds_t> inline T* read( const DS& ds, T* ptr,
			std::initializer_list<hsize_t> offset, std::initializer_list<hsize_t> count ) noexcept {
		return impl::read( static_cast<hid_t>( ds ),ptr,offset.begin(),count.begin() );
	}
	/** \ingroup io-read 
	 * @brief partial reads HDF5 dataset into T object then returns it 
	 * \par_ds  \par_offset \par_count  \tpar_obj \returns_object
	 * \sa_hdf5 \sa_h5cpp \sa_linalg \sa_stl 
	 */
	template<typename T, typename ds_t=h5::ds_t, typename BaseType = typename utils::base<T>::type > inline T read( const ds_t& ds, 
			std::initializer_list<hsize_t> offset, std::initializer_list<hsize_t> count ) noexcept {
		hsize_t rank = count.size();
		T data_set = utils::ctor<T>(rank, count.begin() );
		BaseType * ptr = utils::get_ptr( data_set );
		impl::read(static_cast<hid_t>( ds ), ptr, offset.begin(), count.begin());
		return data_set;
	}
	/** \ingroup io-read 
	 * @brief partial reads HDF5 variable string dataset   
	 * \par_ds \returns_string_vec
	 * \sa_hdf5 \sa_h5cpp \sa_linalg \sa_stl  
	 */
/*	template<> inline std::vector<std::string> read<std::vector<std::string>,hid_t,std::string>( hid_t ds ) noexcept {
		hsize_t offset[H5CPP_MAX_RANK]={}; // all zeros
		hsize_t count[H5CPP_MAX_RANK];
		return impl::read(ds, offset, count);
	}*/
	/** \ingroup io-read 
	 * @brief partial reads HDF5 variable string dataset 
	 * \par_ds \par_offset \par_count \returns_string_vec
	 * \sa_hdf5 \sa_h5cpp \sa_linalg \sa_stl 
	 */
	/*template<> inline std::vector<std::string> read<std::vector<std::string>,hid_t,std::string>(hid_t ds,
			std::initializer_list<hsize_t> offset, std::initializer_list<hsize_t> count  ) noexcept {
		return impl::read(ds, offset.begin(), count.begin() );
	}*/
	/** \ingroup io-read 
	 * @brief reads entire HDF5 dataset specified by path and returns object defined by T template
	 * throws std::runtime_error if dataset not found, and return value is undefined
	 * \par_fd \par_dataset_path \tpar_obj \returns_object
	 * \sa_hdf5 \sa_h5cpp \sa_linalg \sa_stl  
	 * \code
	 * example:
	 * try{
	 * 	stl::vector<float> entire_dataset = 
	 * 				h5::read<stl::vector<float>>( fd,"absolute/path" );	
	 * } catch( const std::runtime_error& ex ) {
	 * 	std::cerr << ex.what();
	 * }	
	 * \endcode
	 */
	template<typename T> inline T read( hid_t fd, const std::string& path ){
     	hid_t ds = H5Dopen(fd, path.data(), H5P_DEFAULT);
		if( ds < 0) throw std::runtime_error("dataset not found: " + path );
			const T& data = h5::read<T>(ds);
        H5Dclose(ds);
		return data;
	}
	/** \ingroup io-read 
	 * @brief reads entire HDF5 dataset specified by path and returns object defined by T template
	 * throws std::runtime_error if dataset not found, and return value is undefined 
	 * \par_file_path \par_dataset_path \tpar_obj \returns_object
	 * \sa_h5cpp \sa_hdf5 \sa_stl \sa_linalg  
	 * 	 * @see [std::runtime_error][10]
	 * \code
	 * example:
	 * try{
	 * 	stl::vector<float> entire_dataset = 
	 * 				h5::read<stl::vector<float>>( "myfile.h5","absolute/path" );	
	 * } catch( const std::runtime_error& ex ) {
	 * 	std::cerr << ex.what();
	 * }	
	 * \endcode
	 * [10]: http://en.cppreference.com/w/cpp/error/exception 
	 */
	template<typename T> inline T read( const std::string& file, const std::string& path ){
     	hid_t fd = h5::open( file, H5F_ACC_RDONLY);
			const T& data = h5::read<T>(fd, path);
        H5Fclose(fd);
		return data;
	}

	/** \ingroup io-read 
	 * @brief partial reads HDF5 dataset into a memory region defined by pointer 
	 * throws std::runtime_error if dataset not found
	 * \par_fd \par_ptr \par_offset \par_count \tpar_T \returns_object
	 * \sa_hdf5 \sa_h5cpp \sa_linalg \sa_stl 
 	 * @exception std::runtime_error - if dataset not found
	 * @see [std::runtime_error][10]
	 *
	 * \code
	 * example:
	 * float* ptr = malloc(100);
	 * try{
	 * 	h5::read<float*>( fd,"absolute/path",ptr, {10},{100} );	
	 * } catch( const std::runtime_error& ex ) {
	 * 	std::cerr << ex.what();
	 * }
	 * ...
	 *
	 * \endcode
	 * [10]: http://en.cppreference.com/w/cpp/error/exception 
	 */
	template<typename T> inline T* read( hid_t fd, const std::string& path, T* ptr, 
			std::initializer_list<hsize_t> offset, std::initializer_list<hsize_t> count ){

     	hid_t ds = H5Dopen(fd, path.data(), H5P_DEFAULT);
			if( ds < 0) throw std::runtime_error("dataset not found: " + path );
			T* data = h5::read<T>(ds,ptr,offset,count);
        H5Dclose(ds);
		return data;
	}

	/** \ingroup io-read 
	 * @brief partial reads HDF5 dataset into T object then returns it 
	 * \par_fd \par_dataset_path \par_offset \par_count \tpar_T \returns_object
	 * \sa_hdf5 \sa_h5cpp \sa_linalg \sa_stl 
	 */
	template<typename T> inline T read( hid_t fd, const std::string& path, std::initializer_list<hsize_t> offset, std::initializer_list<hsize_t> count ){
		hid_t ds = H5Dopen(fd, path.data(), H5P_DEFAULT);
			T data = h5::read<T>(ds,offset,count);
        H5Dclose(ds);
		return data;
	}
}
#endif
