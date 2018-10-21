/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_MBASE_HPP
#define H5CPP_MBASE_HPP



// macros which specialize on the 'base' template detect underlying properties such as type at compile type, as well define 
// general accessors so differences in properties can be mitigated
//
// modify the macro if new container type, or property accessor as added,  

/* -------------------------------------------------------------------------------------------------*/

/* ----------------------------- BEGIN META TEMPLATE -----------------------------------------*/
/* meta templates to leverage differences among containers: armadillo, stl, etc...
 * the variadic macro argument '...' is to escape commas inside: '{rows,cols,slices}'   
 */

#define H5CPP_CTOR_SPEC( T,container, container_rank, ... ) 										\
		template<> inline container<T> ctor<container<T>>(hsize_t hdf5_rank, const hsize_t* hdf5_dims ){ 	\
			/* it possible to have an HDF5 file space higher rank then of the container  					\
			 * we're mapping it to. let's collapse dimensions which exceed rank of output container         \ 
			 * */																							\
			hsize_t dims[container_rank];																	\
			hsize_t i=0; 																					\
			/* branch off if indeed the ranks mismatch, otherwise just a straight copy */ 					\
			if( container_rank < hdf5_rank ){ 																\
				dims[container_rank-1]=1; 																	\
				switch( container_rank ){ 																	\
					case H5CPP_RANK_VEC: /*easy: multiply them all*/										\
							for(;i<hdf5_rank;i++) dims[0] *= hdf5_dims[i]; 									\
						break; 																				\
					case H5CPP_RANK_MAT: /*return the first dimensions which are not equal to 1             \
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
		} 																									\


/* TODO: rework so all const | ref | ptr | will execute
 * applied only on containers: stl::vector, arma::xxx, eigen::xxx, ... 
 * ----------------------------- END META TEMPLATE -----------------------------------------*/ 				\
#define H5CPP_BASE_TEMPLATE_SPEC( T, container, address, n_elem, R, ... )									\
		template<> struct base<container<T>&> {																\
			typedef T type; 																				\
			constexpr static const size_t rank = R;															\
		}; 																									\
		template<> struct base<container<T>> {																\
			typedef T type; 																				\
			constexpr static const size_t rank = R;															\
		};																									\
		inline hsize_t get_size( const container<T>&ref ){													\
			return n_elem; 						 															\
		} 																									\
		inline h5::count_t get_count( const container<T>& ref ){ 											\
			h5::count_t count( __VA_ARGS__ ); 																\
			count.rank = R; 																				\
			return count;   																				\
		} 						 																			\
		inline T* get_ptr( container<T>& ref ){																\
			return address;  																				\
		} 																									\
		inline const T* get_ptr( const container<T>& ref ){													\
			return address;  																				\
		} 																									\

#endif
