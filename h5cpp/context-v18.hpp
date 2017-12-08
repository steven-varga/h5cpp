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

#include <memory>
#ifndef  H5CPP_CONTEXT_V18_H 
#define H5CPP_CONTEXT_V18_H

namespace h5 {
	/**
	 * @brief to maintain dataset context for state-full operations, this way reducing
	 * H5 library calls 
	 */ 
	template<typename T> struct context {
		public:
		/**
		 * @param ds valud dataset descriptor which must be held open until
		 * context destroyed
		 * @tparam T ::= pod struct | integral type
		 * \code 
		 * 		hid_t ds = ... get datatset descriptor ...
		 * 		{  // new code block  
		 * 		auto ctx = h5::context<some_struct>(); 
		 * 			... repetative append operations ...
		 * 		}  // flush is called, when ctx destroyed 
		 * 		H5Dclose(ds); // descriptor remains valid inside code block
		 * \endcode
		 */ 
		context(hid_t ds) :ds(ds){
			hid_t file_space = H5Dget_space(ds);
				rank = H5Sget_simple_extent_dims(file_space, current_dims, max_dims);
			H5Sclose(file_space);


			hid_t plist =  H5Dget_create_plist(ds);
				H5Pget_chunk(plist,rank, chunk_dims );
			H5Pclose(plist);

			for(int i=0;i<rank;i++) count[i]=1,offset[i]=0;
			chunk_size=1; for(int i=0;i<rank;i++) chunk_size*=chunk_dims[i];

			ptr = static_cast<T*>( calloc(chunk_size, sizeof(T)) );

			// close them in DTOR!!!
			file_type = H5Dget_type(ds);
			mem_space = H5Screate_simple(1, &chunk_size, NULL );
			H5Sselect_all(mem_space);
		}

		/** as side effect it flushes internal cache and closes  
		 *  related states: hdf5 memory and file space
		 */
		~context(){
			flush();
			H5Sclose(mem_space);
			H5Tclose(file_type);
			free(ptr);
		}

		/** appends object to HDF5 file stream
		 * @param ref const reference to object being saved
		 * @tparam T::= pod type | integral type
		 *
		 * to avoid exessive HDF5 library calls internal state is maintained
		 * in addition to chunk size data cache. Upon DTOR the cache is flushed 
		 * therefore it is imperative to have a valid dataset descriptor at the time
		 * \code
		 * 		hid_t ds = ... get datatset descriptor ...
		 * 		{
		 * 		auto ctx = h5::context<some_struct>();
		 * 			for( auto ref:dataset )
		 * 				h5::append(ctx, ref);
		 * 		} // flush is called, when ctx destroyed 
		 * 		H5Dclose(ds); // descriptor remains valid inside code block
		 * \endcode
		 */
		void append( const T& ref ){

			size_t k = (++*current_dims -1)% chunk_size;
			ptr[k] = ref;
			if( k == chunk_size - 1  )
				*count=chunk_size, save2file();
		}

		/** saves internal cache to HDF5 file
		 * 
		 * to avoid exessive HDF5 library calls internal state is maintained
		 * in addition to chunk size data cache. Upon DTOR the cache is flushed 
		 * therefore it is imperative to have a valid dataset descriptor at the time
		 * \code
		 * 		hid_t ds = ... get datatset descriptor ...
		 * 		{
		 * 		auto ctx = h5::context<some_struct>();
		 * 			for( auto ref:dataset )
		 * 				h5::append(ctx, ref);
		 * 		} // flush is called, when ctx destroyed 
		 * 		H5Dclose(ds); // descriptor remains valid inside code block
		 * \endcode
		 */
		void flush(){
			*count = *current_dims % chunk_size;
			if( *count ){ // there is left over then flush it
				// select the remainder of the memory
				H5Sselect_hyperslab(mem_space, H5S_SELECT_SET,offset, NULL, count, NULL);
				save2file();
			}
		}

		private:
		void save2file( ){

			H5Dset_extent(ds, current_dims ); 		// make space
			*current_dims -= *count; 				// 
			// select target
			file_space = H5Dget_space(ds);
				H5Sselect_hyperslab(file_space, H5S_SELECT_SET, current_dims, NULL, count, NULL);
				H5Dwrite(ds, file_type, mem_space, file_space,  H5P_DEFAULT, ptr);
			H5Sclose(file_space);
			*current_dims += *count;
		}

		private:
		T * ptr; //< ring buffer
		hid_t ds,mem_type,file_type,mem_space,file_space;
		hsize_t current_dims[H5CPP_MAX_RANK];
		hsize_t max_dims[H5CPP_MAX_RANK];
		hsize_t chunk_dims[H5CPP_MAX_RANK];
		hsize_t offset[H5CPP_MAX_RANK];
		hsize_t count[H5CPP_MAX_RANK],chunk_size;
		size_t rank;
	};
}

#endif
