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
#ifndef  H5CPP_CONTEXT_V110_H 
#define  H5CPP_CONTEXT_V110_H

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
				rank = H5Sget_simple_extent_dims(file_space, current_dims, NULL);
			H5Sclose(file_space);

			hid_t plist =  H5Dget_create_plist(ds);
				H5Pget_chunk(plist,rank, chunk_dims );
			H5Pclose(plist);

			chunk_size=1; for(int i=0;i<rank;i++) chunk_size*=chunk_dims[i];
			ptr = static_cast<T*>( calloc(chunk_size, sizeof(T)) );

			// close them in DTOR!!!
			mem_type = H5Dget_type(ds);
		}

		/** as side effect it flushes internal cache and closes  
		 *  related states: hdf5 memory and file space
		 */
		~context(){
			flush();
			H5Tclose(mem_type);
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
				H5DOappend( ds, H5P_DEFAULT, 0, chunk_size, mem_type, ptr );
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
			size_t count = *current_dims % chunk_size;
			if( count ) // there is left over then flush it
				H5DOappend( ds, H5P_DEFAULT, 0, count, mem_type, ptr );
		}

		private:
		T * ptr; //< ring buffer
		hid_t ds,mem_type,file_space;
		hsize_t chunk_dims[H5CPP_MAX_RANK],current_dims[H5CPP_MAX_RANK];
		hsize_t chunk_size;
		size_t rank;
	};
}

#endif
