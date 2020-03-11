/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef H5CPP_TALL_HPP
#define H5CPP_TALL_HPP


namespace h5 {
    template<class T> hid_t register_struct(){ return H5I_UNINIT; }
}

/* template specialization from hid_t< .. > type which provides syntactic sugar in the form
 * h5::dt_t<int> dt; 
 * */
namespace h5 { namespace impl { namespace detail {
	template<class T> // parent type, data_type is inherited from, see H5Iall.hpp top section for details 
	using dt_p = hid_t<T,H5Tclose,true,true,hdf5::any>;
	/*type id*/
	template<class T>
		struct hid_t<T,H5Tclose,true,true,hdf5::type> : public dt_p<T> {
		using parent = dt_p<T>;
		using hidtype = T;
		hid_t( std::initializer_list<::hid_t> fd ) : parent( fd ){}
		hid_t() : parent( H5I_UNINIT){}
	};
	template <class T> using dt_t = hid_t<T,H5Tclose,true,true,hdf5::type>;
}}}

/* template specialization is for the preceding class, and should be used only for HDF5 ELEMENT types
 * which are in C/C++ the integral types of: char,short,int,long, ... and C POD types. 
 * anything else, the ones which are considered objects/classes are broken down into integral types + container 
 * then pointer read|write is obtained to the continuous slab and delegated to h5::read | h5::write.
 * IF the data is not in a continuous memory region then it must be copied! 
 */

#define H5CPP_REGISTER_TYPE_( C_TYPE, H5_TYPE )                                           \
namespace h5 { namespace impl { namespace detail { 	                                      \
	template <> struct hid_t<C_TYPE,H5Tclose,true,true,hdf5::type> : public dt_p<C_TYPE> {\
		using parent = dt_p<C_TYPE>;                                                      \
		using dt_p<C_TYPE>::hid_t;                                                              \
		using hidtype = C_TYPE;                                                           \
		hid_t() : parent( H5Tcopy( H5_TYPE ) ) { 										  \
			hid_t id = static_cast<hid_t>( *this );                                       \
			if constexpr ( std::is_pointer<C_TYPE>::value )                               \
					H5Tset_size (id,H5T_VARIABLE), H5Tset_cset(id, H5T_CSET_UTF8);        \
		}                                                                                 \
	};                                                                                    \
}}}                                                                                       \
namespace h5 {                                                                            \
	template <> struct name<C_TYPE> {                                                     \
		static constexpr char const * value = #C_TYPE;                                    \
	};                                                                                    \
}                                                                                         \

/* registering integral data-types for NATIVE ones, which means all data is stored in the same way 
 * in file and memory: TODO: allow different types for file storage
 * */
	H5CPP_REGISTER_TYPE_(bool,H5T_NATIVE_HBOOL)

	H5CPP_REGISTER_TYPE_(unsigned char, H5T_NATIVE_UCHAR) 			H5CPP_REGISTER_TYPE_(char, H5T_NATIVE_CHAR)
	H5CPP_REGISTER_TYPE_(unsigned short, H5T_NATIVE_USHORT) 		H5CPP_REGISTER_TYPE_(short, H5T_NATIVE_SHORT)
	H5CPP_REGISTER_TYPE_(unsigned int, H5T_NATIVE_UINT) 			H5CPP_REGISTER_TYPE_(int, H5T_NATIVE_INT)
	H5CPP_REGISTER_TYPE_(unsigned long int, H5T_NATIVE_ULONG) 		H5CPP_REGISTER_TYPE_(long int, H5T_NATIVE_LONG)
	H5CPP_REGISTER_TYPE_(unsigned long long int, H5T_NATIVE_ULLONG) H5CPP_REGISTER_TYPE_(long long int, H5T_NATIVE_LLONG)
	H5CPP_REGISTER_TYPE_(float, H5T_NATIVE_FLOAT) 					H5CPP_REGISTER_TYPE_(double, H5T_NATIVE_DOUBLE)
	H5CPP_REGISTER_TYPE_(long double,H5T_NATIVE_LDOUBLE)

	H5CPP_REGISTER_TYPE_(char*, H5T_C_S1)

// half float support: 
// TODO: factor out in a separate file
#ifdef HALF_HALF_HPP
   namespace h5::impl::detail {
	template <> struct hid_t<half_float::half, H5Tclose,true,true, hdf5::type> : public dt_p<half_float::half> {
		using parent = dt_p<half_float::half>;
		using dt_p<half_float::half>::hid_t;
		using hidtype = half_float::half;
		hid_t() : parent( H5Tcopy( H5T_NATIVE_FLOAT ) ) {

			H5Tset_fields( handle, 15, 10, 5, 0, 10);
			H5Tset_precision(handle, 16);
			H5Tset_ebias( handle, 15);
			H5Tset_size(handle,2);
			hid_t id = static_cast<hid_t>( *this );
		}
	};
}
namespace h5 {
	template <> struct name<half_float::half> {
		static constexpr char const * value = "half-float";
	};
}
#endif
// Open XDR doesn-t define namespace or 
#ifdef WITH_OPENEXR_HALF 
   namespace h5::impl::detail {
	template <> struct hid_t<OPENEXR_NAMESPACE::half, H5Tclose,true,true, hdf5::type> : public dt_p<OPENEXR_NAMESPACE::half> {
		using parent = dt_p<OPENEXR_NAMESPACE::half>;
		using dt_p<OPENEXR_NAMESPACE::half>::hid_t;
		using hidtype = OPENEXR_NAMESPACE::half;
		hid_t() : parent( H5Tcopy( H5T_NATIVE_FLOAT ) ) {

			H5Tset_fields( handle, 15, 10, 5, 0, 10);
			H5Tset_precision(handle, 16);
			H5Tset_ebias( handle, 15);
			H5Tset_size(handle,2);
			hid_t id = static_cast<hid_t>( *this );
		}
	};
}
namespace h5 {
	template <> struct name<OPENEXR_NAMESPACE::half> {
		static constexpr char const * value = "openexr half-float";
	};
}
#endif
#define H5CPP_REGISTER_STRUCT( POD_STRUCT ) H5CPP_REGISTER_TYPE_( POD_STRUCT, h5::register_struct<POD_STRUCT>() )

/* type alias is responsible for ALL type maps through H5CPP if you want to screw things up
 * start here.
 * template parameters:
 *  hid_t< C_TYPE being mapped, conversion_from_capi, conversion_to_capi, marker_for_this_type>
 * */


namespace h5 {
	template <class T> using dt_t = h5::impl::detail::hid_t<T,H5Tclose,true,true,h5::impl::detail::hdf5::type>;

	template<class T>
	hid_t copy( const h5::dt_t<T>& dt ){
		hid_t id = static_cast<hid_t>(dt);
		H5Iinc_ref( id );
		return id;
	}
}


template<class T>
inline std::ostream& operator<<(std::ostream &os, const h5::dt_t<T>& dt) {
	hid_t id = static_cast<hid_t>( dt );
	os << "data type: " << h5::name<T>::value << " ";
	os << ( std::is_pointer<T>::value ? "pointer" : "value" );
	os << ( H5Iis_valid( id ) > 0 ? " valid" : " invalid");

	size_t spos, epos, esize, mpos, msize;
	switch( H5Tget_class( id ) ){
		case H5T_INTEGER: ; break;
		case H5T_FLOAT:
			H5Tget_fields( id, &spos, &epos, &esize, &mpos, &msize );

			os << "\n\tebias: " << H5Tget_ebias( id ) 
				<< " norm: " <<  H5Tget_norm( id ) 
				<< " offset: " <<  H5Tget_offset( id )
			   	<< " precision: " << H5Tget_precision( id )
			   	<< " size: " << H5Tget_size( id )
				<< "\n\tspos:" << spos << " epos:" << epos << " esize:" << esize << " mpos:" << mpos <<" msize:" << msize
			   	<<"\n";
			 break;
		case H5T_STRING: ; break;
		case H5T_BITFIELD: ; break;
		case H5T_OPAQUE: ; break;
		case H5T_COMPOUND: ; break;
		case H5T_REFERENCE: ; break;
		case H5T_ENUM: ; break;
		case H5T_VLEN: ; break;
		case H5T_ARRAY: ;break;
	}
	/*
*/

	return os;
}

#endif
