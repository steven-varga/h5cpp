/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */


#ifndef  H5CPP_RAII_H 
#define H5CPP_RAII_H

#define H5CPP_CAPI_CONVERSION_DISABLED ! H5CPP_CAPI_CONVERSION_ENABLED

#include <hdf5.h>
#include <hdf5_hl.h> //TODO: is it needed?
#include <iostream>
#include <memory>
#include <initializer_list>

#define H5CPP_CONVERSION_FROM_CAPI
#define H5CPP_CONVERSION_TO_CAPI


#ifdef H5CPP_CONVERSION_IMPLICIT
	#define H5CPP__EXPLICIT
#else
	#define H5CPP__EXPLICIT explicit
#endif

#ifdef H5CPP_CONVERSION_FROM_CAPI 
	#define H5CPP__CONVERSION_CTOR_FROM_CAPI(T_)            \
		H5CPP__EXPLICIT hid_t( ::hid_t fd ) : handle( fd ){ \
		if( H5Iis_valid( fd ) ) {                           \
		    int count = H5Iinc_ref( fd );                   \
		    } /* TODO: throw runtime_error */               \
		}                                                   \

#else
	#define H5CPP__CONVERSION_CTOR_FROM_CAPI(T_) ;
#endif

#ifdef H5CPP_CONVERSION_TO_CAPI 
	#define H5CPP__CONVERSION_CTOR_TO_CAPI(T_)             \
		H5CPP__EXPLICIT operator ::hid_t() const {         \
			return  handle;                                \
		}                                                  \

#else
	#define H5CPP__CONVERSION_TO_CAPI(T_)
#endif

namespace h5 { namespace impl {
	template <class T> struct hid_t final {
		hid_t()=delete;
	};
}}

#define H5CPP_MACRO_WRAP_HID_T__( T_, D_ )                 \
	struct T_ final { };                                   \
	template <> struct hid_t<T_> {                         \
		hid_t() = delete;                                  \
        hid_t( std::initializer_list<::hid_t> fd )         \
		   : handle( *fd.begin()){                         \
		}                                                  \
		H5CPP__CONVERSION_CTOR_TO_CAPI(T_)                 \
		H5CPP__CONVERSION_CTOR_FROM_CAPI(T_)               \
		/* move ctor must invalidate old handle */         \
		hid_t( hid_t<T_>&& ref ){                          \
			this->handle = ref.handle;                     \
			ref.handle = H5I_UNINIT;                       \
		}                                                  \
		~hid_t(){                                          \
			if( H5Iis_valid( handle ) )                    \
				D_( handle );                              \
		}                                                  \
		private:                                           \
		::hid_t handle;                                    \
	};                                                     \

/*hide details */
#define H5CPP_MACRO_WRAP_HID_T( T_, D_ )                   \
	namespace impl { H5CPP_MACRO_WRAP_HID_T__(T_,D_) }     \
	using T_ = impl::hid_t<impl::T_>;                      \

namespace h5 {
	/*base template with no default ctors to prevent instantiation*/

	H5CPP_MACRO_WRAP_HID_T(fd_t, H5Fclose) 	H5CPP_MACRO_WRAP_HID_T(ds_t, H5Dclose) 	H5CPP_MACRO_WRAP_HID_T(pt_t, H5PTclose)

	H5CPP_MACRO_WRAP_HID_T(acpl_t,H5Pclose) 
	H5CPP_MACRO_WRAP_HID_T(dapl_t,H5Pclose) H5CPP_MACRO_WRAP_HID_T(dxpl_t,H5Pclose) H5CPP_MACRO_WRAP_HID_T(dcpl_t,H5Pclose)
	H5CPP_MACRO_WRAP_HID_T(tapl_t,H5Pclose)	H5CPP_MACRO_WRAP_HID_T(tcpl_t,H5Pclose)
	H5CPP_MACRO_WRAP_HID_T(fapl_t,H5Pclose)	H5CPP_MACRO_WRAP_HID_T(fcpl_t,H5Pclose) H5CPP_MACRO_WRAP_HID_T(fmpl_t,H5Pclose)
	H5CPP_MACRO_WRAP_HID_T(gapl_t,H5Pclose) H5CPP_MACRO_WRAP_HID_T(gcpl_t,H5Pclose)
	H5CPP_MACRO_WRAP_HID_T(lapl_t,H5Pclose)	H5CPP_MACRO_WRAP_HID_T(lcpl_t,H5Pclose)
	H5CPP_MACRO_WRAP_HID_T(ocrl_t,H5Pclose) H5CPP_MACRO_WRAP_HID_T(ocpl_t,H5Pclose)
	H5CPP_MACRO_WRAP_HID_T(scpl_t,H5Pclose)
}

#include "macros_proplist.h"

#undef H5CPP_CONVERSION_FROM_CAPI
#undef H5CPP_CONVERSION_TO_CAPI
#endif
