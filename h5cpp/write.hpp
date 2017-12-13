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


#ifndef  H5CPP_WRITE_H 
#define H5CPP_WRITE_H

namespace h5{
	/** \ingroup io-write 
	 * @brief partial write HDF5 dataset from memory region defined by pointer into opened HDF5 dataset 
	 * @param ds valid HDF5 dataset descriptor
	 * @param ptr pointer to in memory dataset  
	 * @param offset pointer to an array with the coordinates withing HDF5 dataset with rank of the file space, for instance a cube {0,0,0}
	 * @param count pointer to an array with amount of data returned, each dimension must be in (1,max_dimension), for instance {1,3,10} returns a matrix
	 * @tparam T type of [unsigned](char|short|int|long long)|(float|double) type    
	 */
	template<typename T> inline void write(hid_t ds, const T* ptr, const hsize_t* offset, const hsize_t* count ){

		hid_t type = utils::h5type<T>();
        hid_t file_space = H5Dget_space(ds);

		int rank =  H5Sget_simple_extent_ndims(file_space);
		hsize_t size = 1;for(int i=0;i<rank;i++) size*=count[i];
		hid_t mem_space = H5Screate_simple(1, &size, NULL );
		H5Sselect_all(mem_space);

		H5Sselect_hyperslab(file_space, H5S_SELECT_SET, offset, NULL, count, NULL);
			H5Dwrite(ds, type, mem_space, file_space,  H5P_DEFAULT, ptr);
		H5Sclose(mem_space); H5Sclose(file_space); H5Tclose(type);
	}
	/** \ingroup io-write 
	 * @brief partial write HDF5 dataset into opened HDF5 dataset 
	 * @param ds valid HDF5 dataset descriptor
	 * @param ptr pointer to in memory dataset  
	 * @param offset the coordinates withing HDF5 dataset with rank of the file space, for instance a cube {0,0,0}
	 * @param count amount of data returned, each dimension must be in (1,max_dimension), for instance {1,3,10} returns a matrix
	 * @tparam T type of [unsigned](char|short|int|long long)|(float|double) type    
	 */
	template<typename T> inline void write(hid_t ds, const T* ptr, std::initializer_list<hsize_t> offset,	std::initializer_list<hsize_t> count){
		h5::write<T>(ds,ptr,offset.begin(),count.begin());
	}
	/** \ingroup io-write 
	 * @brief write the entire object into HDF5 dataset 
	 * @param ds valid HDF5 dataset descriptor
	 * @param ref reference to object 
	 * @tparam T type of [unsigned](char|short|int|long long)|(float|double) type    
	 */
	template<typename T, typename BaseType = typename utils::base<T>::type, size_t Rank = utils::base<T>::rank >
		inline void write( hid_t ds, const T& ref){

			std::array<hsize_t,Rank> offset = {}; // initialize to zeros
			std::array<hsize_t,Rank> count = utils::get_dims( ref );
			const BaseType* ptr = utils::get_ptr( ref );
			h5::write<BaseType>(ds,ptr,offset.data(), count.data() );
	}

	/** \ingroup io-write 
	 * @brief partial write HDF5 dataset into opened HDF5 dataset 
	 * @param ds valid HDF5 dataset descriptor
	 * @param offset the coordinates withing HDF5 dataset with rank of the file space, for instance a cube {0,0,0}
	 * @param ref reference to object 
	 * @param count amount of data returned, each dimension must be in (1,max_dimension), for instance {1,3,10} returns a matrix
	 * @tparam T type of [unsigned](char|short|int|long long)|(float|double) type    
	 */
	template<typename T, typename BaseType = typename utils::base<T>::type>
		inline void write( hid_t ds, const T& ref, std::initializer_list<hsize_t> offset,	std::initializer_list<hsize_t> count){

		const BaseType* ptr = utils::get_ptr( ref );
		h5::write<BaseType>(ds,ptr,offset.begin(),count.begin());
	}
	/** \ingroup io-write 
	 * @brief  writes HDF5 dataset into opened HDF5 dataset
	 * @param fd valid file descriptor 
	 * @param path valid absolute path to HDF5 dataset
	 * @param ref reference to object 
	 * @tparam T type of [unsigned](char|short|int|long long)|(float|double) type    
	 */
	template<typename T> inline void write( hid_t fd, const std::string& path, const T& ref){

		hid_t ds = h5::utils::dataset_exist(fd, path ) ?
			H5Dopen(fd, path.data(), H5P_DEFAULT) : h5::create(fd,path,ref);
			h5::write(ds,ref );
        H5Dclose(ds);
	}
	/** \ingroup io-write 
	 * @brief partial write HDF5 dataset into opened HDF5 dataset 
	 * @param fd valid HDF5 file descriptor
	 * @param path valid absolute path to HDF5 dataset
	 * @param ref reference to object 
	 * @param offset the coordinates withing HDF5 dataset with rank of the file space, for instance a cube {0,0,0}
	 * @param count amount of data returned, each dimension must be in (1,max_dimension), for instance {1,3,10} returns a matrix
	 * @tparam T type of [unsigned](char|short|int|long long)|(float|double) type    
	 */
	template<typename T> inline void write( hid_t fd, const std::string& path, const T& ref,
			std::initializer_list<hsize_t> offset, std::initializer_list<hsize_t> count){
     	hid_t ds = H5Dopen (fd, path.data(), H5P_DEFAULT);
			h5::write(ds,ref,offset,count);
        H5Dclose(ds);
	}
	/** \ingroup io-write 
	 * @brief partial write HDF5 dataset into opened HDF5 dataset 
	 * @param fd valid HDF5 file descriptor
	 * @param path valid absolute path to HDF5 dataset
	 * @param ptr pointer to in memory dataset  
	 * @param offset the coordinates withing HDF5 dataset with rank of the file space, for instance a cube {0,0,0}
	 * @param count amount of data returned, each dimension must be in (1,max_dimension), for instance {1,3,10} returns a matrix
	 * @tparam T type of [unsigned](char|short|int|long long)|(float|double) type    
	 */
	template<typename T> inline void write( hid_t fd, const std::string& path, const T* ptr,
			std::initializer_list<hsize_t> offset, std::initializer_list<hsize_t> count){
     	hid_t ds = H5Dopen (fd, path.data(), H5P_DEFAULT);
			h5::write(ds,ptr,offset.begin(),count.begin());
        H5Dclose(ds);
	}


	//STD::STRING full specialization
	template<> inline void write<std::vector<std::string>,std::string>(
		 hid_t ds, const std::vector<std::string>& data, std::initializer_list<hsize_t> offset,	std::initializer_list<hsize_t> count){

        std::vector<char*> ptr;
		for( const auto& ref:data)
            ptr.push_back( strdup( ref.data()) );

      	hid_t mem_type =  utils::h5type<std::string>();
        hid_t file_space = H5Dget_space(ds);

		int rank =  H5Sget_simple_extent_ndims(file_space);

		hsize_t size = 1;for(int i=0;i<rank;i++) size*=*(count.begin()+i);
		hid_t mem_space = H5Screate_simple(1, &size, NULL );
		H5Sselect_all(mem_space);

		H5Sselect_hyperslab(file_space, H5S_SELECT_SET, offset.begin(), NULL, count.begin(), NULL);
			H5Dwrite(ds, mem_type, mem_space, file_space,  H5P_DEFAULT, ptr.data());
		H5Sclose(mem_space); H5Sclose(file_space); H5Tclose(mem_type);
        // rollback strdup 
        for( auto p:ptr ) free(p);

	}

	template<> inline void write<std::vector<std::string>,std::string,H5CPP_RANK_VEC>(
			hid_t ds, const std::vector<std::string>& data ){
 		h5::write(ds,data,{0},{data.size()} );
    }

}
#endif
