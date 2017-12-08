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


#ifndef  H5CPP_MISC_H
#define H5CPP_MISC_H
/**  
 * @namespace h5
 * @brief public namespace
 */
namespace h5{
	/**  \ingroup file-io 
	 * @brief create an HDF5 file
	 * simple mapping from the original H5Fcreate with default values, for refined access call the H5F_ relevant 
	 * routines  
	 * @param path the location where the file is created
	 * @return an open hid_t  HDF5 file descriptor 
	 * @see <a href="https://support.hdfgroup.org/HDF5/Tutor/compress.html">GZIP</a> @see close 
	 * @see <a href="https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Create">H5Fcreate</a>
	 * \code 
	 * 		hid_t fd = h5::create("example.h5");  	// create file and save descripor
	 * 		h5::close(fd); 							// close file descriptor
	 * \endcode
	 */
    hid_t create( const std::string& path ){

        hid_t plist = H5Pcreate(H5P_FILE_ACCESS);
        hid_t fd = H5Fcreate(path.data(), H5F_ACC_TRUNC, H5P_DEFAULT, plist);
        H5Pclose(plist);
        return fd;
    };
	/** \ingroup file-io  
	 * open an existing HDF5 file.
	 * @param path the location of the file
	 * @param flags (H5F_ACC_RDWR[|H5F_ACC_SWMR_WRITE])| H5F_ACC_RDONLY 
	 * @see <a href="https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Open">H5Fopen</a>
	 */ 
    hid_t open(const std::string& path,  unsigned flags ){

        hid_t plist = H5Pcreate(H5P_FILE_ACCESS);
        	hid_t fd =  H5Fopen(path.data(), flags,  plist);

        H5Pclose(plist);
        return fd;
    };
	/** \ingroup file-io 
	 * open an existing HDF5 file
	 * @param fd valid HDF5 file descripotor
	 * @param path the location of the file
	 * @see <a href="https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Open">H5Fopen</a>
	 */ 
    hid_t open(hid_t fd, const std::string& path ){
     	return  H5Dopen(fd, path.data(), H5P_DEFAULT);
    };

	/** \ingroup file-io  
	 * closes opened file descriptor
	 * @param fd valid and opened file descriptor to an HDF5 file
	 * @see <a href="https://support.hdfgroup.org/HDF5/doc/RM/RM_H5F.html#File-Close">H5Fclose</a>
	 */
	void close(hid_t fd) {
		H5Fclose(fd);
	}
}



namespace h5 { namespace utils {

// macros which specialize on the 'base' template detect underlying properties such as type at compile type, as well define 
// general accessors so differences in properties can be mitigated
//
// modify the macro if new container type, or property accessor as added,  

/* ----------------------------- BEGIN META TEMPLATE -----------------------------------------*/
/* meta templates to leverage differences among containers: armadillo, stl, etc...
 * the variadic macro argument '...' is to escape commas inside: '{rows,cols,slices}'   
 */
#define H5CPP_BASE_TEMPLATE_SPEC( T, container, address, n_elem, R, ... )						\
template<> struct base<container<T>> {																\
	typedef T type; 																				\
	static const size_t rank=R; 																	\
}; 																									\
hsize_t size( const container<T>&ref ){																\
	return ref.n_elem; 																				\
}; 																									\
std::array<hsize_t,R> dims( const container<T>& ref ){												\
   	return __VA_ARGS__; 																			\
}; 						 																			\
T* data( container<T>& ref ){																		\
   	return ref.address;  																			\
}; 																									\
const T* data( const container<T>& ref ){															\
   	return ref.address;  																			\
}; 																									\


#define H5CPP_CTOR_SPEC( T,container, container_rank, ... ) 										\
template<> container<T> ctor<container<T>>(hsize_t hdf5_rank, const hsize_t* hdf5_dims ){ 			\
	/* it possible to have an HDF5 file space higher rank then of the container  					\
	 * we're mapping it to. let's collapse dimensions which exceed rank of output container 
	 *  
	 * */																							\
	hsize_t dims[container_rank];																	\
	int i=0; 																						\
	/* branch off if indeed the ranks mismatch, otherwise just a straight copy */ 					\
	if( container_rank < hdf5_rank ){ 																\
		dims[container_rank-1]=1; 																	\
		switch( container_rank ){ 																	\
			case H5CPP_RANK_VEC: /*easy: multiply them all*/										\
					for(;i<hdf5_rank;i++) dims[0] *= hdf5_dims[i]; 									\
				break; 																				\
			case H5CPP_RANK_MAT: /*return the first dimensions which are not equal to 1 \
								   then collapse the tail                        */					\
					for(; hdf5_dims[i] <= 1 && i<hdf5_rank;i++); dims[0] = hdf5_dims[i++]; 			\
					for(;i<hdf5_rank;i++) dims[1] *= hdf5_dims[i]; 									\
				break; 																				\
			case H5CPP_RANK_CUBE: /* ditto */														\
					for(; hdf5_dims[i] <= 1 && i<hdf5_rank;i++); dims[0] = hdf5_dims[i++]; 			\
					for(; hdf5_dims[i] <= 1 && i<hdf5_rank;i++); dims[1] = hdf5_dims[i++]; 			\
					for(;i<hdf5_rank;i++) dims[2] *= hdf5_dims[i]; 									\
				break; 																				\
		} 																							\
	} else 																							\
		for(; i<container_rank; i++) dims[i] = hdf5_dims[i]; 										\
	/*expands to constructor of the form: arma::Mat<int>(dims[0],dims[1]);  */ 						\
	return container<T> __VA_ARGS__ ; 																\
}; 																									\


/* ----------------------------- END META TEMPLATE -----------------------------------------*/

#define H5CPP_STL_TEMPLATE_SPEC(T) 																	\
	H5CPP_BASE_TEMPLATE_SPEC(T, std::vector, data(), size(), H5CPP_RANK_VEC,  {ref.size()})  		\
	H5CPP_CTOR_SPEC(T, std::vector, H5CPP_RANK_VEC, (dims[0]))										\

#ifdef ARMA_INCLUDES
/* definitions for armadillo containers */
	#define  H5CPP_ARMA_TEMPLATE_SPEC(T) 																				\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Row, memptr(), n_elem, H5CPP_RANK_VEC,  {ref.n_elem} ) 							\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Col, memptr(), n_elem, H5CPP_RANK_VEC,  {ref.n_elem} ) 							\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Mat, memptr(), n_elem, H5CPP_RANK_MAT,  {ref.n_rows, ref.n_cols} ) 				\
	H5CPP_BASE_TEMPLATE_SPEC(T, arma::Cube,memptr(), n_elem, H5CPP_RANK_CUBE, {ref.n_rows, ref.n_cols, ref.n_slices} ) 	\
	H5CPP_CTOR_SPEC(T, arma::Row,  H5CPP_RANK_VEC,  (dims[0]) )															\
	H5CPP_CTOR_SPEC(T, arma::Col,  H5CPP_RANK_VEC,  (dims[0]) )															\
	H5CPP_CTOR_SPEC(T, arma::Mat,  H5CPP_RANK_MAT,  (dims[0],dims[1]) )													\
	H5CPP_CTOR_SPEC(T, arma::Cube, H5CPP_RANK_CUBE, (dims[0],dims[1],dims[2]) )											\

#else
	#define H5CPP_ARMA_TEMPLATE_SPEC(T) /* empty definition on purpose as <armadillo> is not included */
#endif

/* specialize all templates to set of POD types */
#define SPECIALIZE(T)  H5CPP_ARMA_TEMPLATE_SPEC(T)  H5CPP_STL_TEMPLATE_SPEC(T) 								\

	template<typename T> struct base;
	template<typename T> T ctor(hsize_t rank, const hsize_t* dims ){}

	// modify these lines if new POD type added/removed
	H5CPP_STL_TEMPLATE_SPEC(std::string)
	SPECIALIZE(char) SPECIALIZE(short) SPECIALIZE(int) SPECIALIZE(long) SPECIALIZE(long long) SPECIALIZE(unsigned char)
	SPECIALIZE(unsigned short) SPECIALIZE(unsigned int) SPECIALIZE(unsigned long) SPECIALIZE(unsigned long long)
	SPECIALIZE(float) SPECIALIZE(double) H5CPP_STL_TEMPLATE_SPEC(long double)
//FIXME: armadillo doesn't accept 'long double' type
	//SPECIALIZE(cx_float) SPECIALIZE(cx_double)
#undef SPECIALIZE

// macros define maps from POD type to H5 datatypes, the returned hid_t object must be destroyed upon 
// cleanup to prevent resource leakage  
#define POD2H5T(POD_TYPE,H_TYPE) template<> hid_t h5type<POD_TYPE>(){ return  H5Tcopy(H_TYPE); }
	template<typename T> hid_t h5type( ){}

	POD2H5T(unsigned char,H5T_NATIVE_UCHAR) POD2H5T(unsigned short,H5T_NATIVE_USHORT) POD2H5T(unsigned int,H5T_NATIVE_UINT)
	POD2H5T(unsigned long,H5T_NATIVE_ULONG) POD2H5T(unsigned long long,H5T_NATIVE_ULLONG)

	POD2H5T(char,H5T_NATIVE_CHAR) POD2H5T(short,H5T_NATIVE_SHORT) POD2H5T(int,H5T_NATIVE_INT) 
	POD2H5T(long,H5T_NATIVE_LONG) POD2H5T(long long,H5T_NATIVE_LLONG)

	POD2H5T(bool,H5T_NATIVE_HBOOL) POD2H5T(float,H5T_NATIVE_FLOAT) POD2H5T(double,H5T_NATIVE_DOUBLE)
	//POD2H5T(long double,H5T_NATIVE_LDOUBLE) POD2H5T(cx_float,H5T_NATIVE_FLOAT) POD2H5T(cx_double,H5T_NATIVE_FLOAT)
    // std::string is treated as variable length datatype
	// FIXME: fixed length is ore efficient when read/written, should one proved 'fixed length string' as well? 
	template<> hid_t h5type<std::string>(){ hid_t type = H5Tcopy (H5T_C_S1);
	   	H5Tset_size (type,H5T_VARIABLE); return type; }


#undef POD2H5T
}}
#endif

