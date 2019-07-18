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
namespace h5::impl::detail {
	template<class T> // parent type, data_type is inherited from, see H5Iall.hpp top section for details 
	using dt_p = hid_t<T,H5Tclose,true,true,hdf5::any>;
	/*type id*/
	template<class T>
		struct hid_t<T,H5Tclose,true,true,hdf5::type> : public dt_p<T> {
		using parent = dt_p<T>;
		using parent::hid_t; // is a must because of ds_t{hid_t} ctor 
		using hidtype = T;
		hid_t() : parent( H5I_UNINIT){};
	};
	template <class T> using dt_t = hid_t<T,H5Tclose,true,true,hdf5::type>;
}

/* template specialization is for the preceding class, and should be used only for HDF5 ELEMENT types
 * which are in C/C++ the integral types of: char,short,int,long, ... and C POD types. 
 * anything else, the ones which are considered objects/classes are broken down into integral types + container 
 * then pointer read|write is obtained to the continuous slab and delegated to h5::read | h5::write.
 * IF the data is not in a continuous memory region then it must be copied! 
 */

#define H5CPP_REGISTER_TYPE_( C_TYPE, H5_TYPE )                                           \
namespace h5::impl::detail {                                                              \
	template <> struct hid_t<C_TYPE,H5Tclose,true,true,hdf5::type> : public dt_p<C_TYPE> {\
		using parent = dt_p<C_TYPE>;                                                      \
		using parent::hid_t;                                                              \
		using hidtype = C_TYPE;                                                           \
		hid_t() : parent( H5Tcopy( H5_TYPE ) ) { 										  \
			hid_t id = static_cast<hid_t>( *this );                                       \
			if constexpr ( std::is_pointer_v<C_TYPE> )                                    \
					H5Tset_size (id,H5T_VARIABLE), H5Tset_cset(id, H5T_CSET_UTF8);        \
		}                                                                                 \
	};                                                                                    \
}                                                                                         \
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
	os << ( std::is_pointer_v<T> ? "pointer" : "value" );
	os << ( H5Iis_valid( id ) > 0 ? " valid" : " invalid");
	return os;
}

#endif
