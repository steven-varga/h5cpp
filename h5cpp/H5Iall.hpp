/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */


#ifndef  H5CPP_IALL_HPP
#define  H5CPP_IALL_HPP


#ifdef H5CPP_CONVERSION_IMPLICIT
	#define H5CPP__EXPLICIT
#else
	#define H5CPP__EXPLICIT explicit
#endif

namespace h5 { namespace impl {
	using capi_close_t = ::herr_t(*)(::hid_t);
	using defprop_t = ::hid_t(*)();

	using prop_list_t = std::vector<int>;

	template<class hid, class... args_tt>
	struct capi_t {
		using fn_t = herr_t (*)(::hid_t, args_tt... );
		using args_t = std::tuple<args_tt...>;
		using type = std::tuple<::hid_t,args_tt...>;
	};
	//forward declarations
	struct at_t;
}}

namespace h5 { namespace impl { namespace detail {
	/* this mechanism is to alter the behaviour of h5::hid_t through 
	 * template specialization. The base class is ::any which provides
	 * conversion policy and resource cleanup
	 */ 
	namespace hdf5 { // fair use of copyrighted HDF5 symbol to ease on reading
		constexpr int any 		= 0x00;
		constexpr int property 	= 0x01;
		constexpr int type 		= 0x02;
		constexpr int dataset	= 0x04;
		constexpr int attribute	= 0x05;
	}

	// base template with T type, the capi_close function, and 
	// whether you allow conversion from_capi and to_capi of the hid_t type
	template <class T, capi_close_t capi_close,
	// conversion policy is controlled by template specialization
	// from_capi, to_capi whether you want capi hid_t converted to h5cpp typed hid_t<T> classes
			 bool from_capi, bool to_capi, int hdf5_class>
	struct hid_t final {
		hid_t()=delete;
	};

	// actual implementation with full conversion allowed
	template<class T, capi_close_t capi_close>
	struct hid_t<T,capi_close, true,true,hdf5::any> {
		using parent = hid_t<T,capi_close,true,true,hdf5::any>;
		using hidtype = T;
		// from CAPI
		H5CPP__EXPLICIT hid_t( ::hid_t handle_ ) : handle( handle_ ){
			if( H5Iis_valid( handle_ ) )
				H5Iinc_ref( handle_ );
		}
		// TO CAPI
		H5CPP__EXPLICIT operator ::hid_t() const {
			return  handle;
		}
        hid_t( std::initializer_list<::hid_t> fd ) // direct  initialization doesn't increment handle
		   : handle( *fd.begin()){
		}
		//TODO: have default constructor such that can initialize properties
		// see 'create.hpp line 164:  h5::dcpl_t dcpl = h5::dcpl_t {H5Pcreate(H5P_DATASET_CREATE)};
		// which is awkward
		hid_t() : handle(H5I_UNINIT){};
		hid_t( const hid_t& ref) {
			this->handle = ref.handle;
			if( H5Iis_valid( handle ) )
				H5Iinc_ref( handle );
		}
		hid_t& operator =( const hid_t& ref) {
			handle = ref.handle;
			if( H5Iis_valid( handle ) )
				H5Iinc_ref( handle );
			return *this;
		}
		/* move ctor must invalidate old handle */
		hid_t( hid_t<T,capi_close,true,true,hdf5::any>&& ref ){
			handle = ref.handle;
			ref.handle = H5I_UNINIT;
		}
		~hid_t(){
			::herr_t err = 0;
			if( H5Iis_valid( handle ) )
				err = capi_close( handle );
		}
		protected:
		::hid_t handle;
	};

	// disable from CAPI and TOCAPI conversions 
	//conversion ctor to packet table enabled, used for h5::impl::ds_t
	template<class T, capi_close_t capi_close>
	struct hid_t<T,capi_close, false,false,hdf5::any> : private hid_t<T,capi_close,true,true,hdf5::any> {
		using parent = hid_t<T,capi_close,true,true,hdf5::any>;
		using hidtype = T;
        hid_t( std::initializer_list<::hid_t> fd ) : parent( fd ){}
	};
	/*property id*/
	template<class T, capi_close_t capi_close>
	struct hid_t<T,capi_close, true,true,hdf5::property> : public hid_t<T,capi_close,true,true,hdf5::any> {
		using parent = hid_t<T,capi_close,true,true,hdf5::any>;
		using hidtype = T;
		//FIXME: delegating ctor, don't use `parent::hid_t` as crashes clang
        using hid_t<T,capi_close,true,true,hdf5::any>::hid_t;
        //using parent::hid_t; // is a must because of ds_t{hid_t} ctor 
		//hid_t( ::hid_t handle_ )  {}
		//hid_t& operator =( ::hid_t ref ) {}


		hid_t& operator |=( const hid_t& ref){
			return *this;
		}

		hid_t& operator |( const hid_t& ref){
			return *this;
		}
	};
	/*dataset id*/ //FIXME: eliminate extra field: ds  @see HDF5 CAPI BUG: https://jira.hdfgroup.org/browse/HDFFV-10934
	template<class T, capi_close_t capi_close>
	struct hid_t<T,capi_close, true,true,hdf5::dataset> : public hid_t<T,capi_close,true,true,hdf5::any> {
		using parent = hid_t<T,capi_close,true,true,hdf5::any>;
		//using parent::hid_t;  // is a must because of ds_t{hid_t} ctor 
		using hid_t<T,capi_close,true,true,hdf5::any>::hid_t;
        using parent::handle; // is a must because of ds_t{hid_t} ctor 
		using hidtype = T;
		using at_t = hid_t<h5::impl::at_t,H5Aclose,true,true,hdf5::attribute>;

		hid_t(){
			this->handle = H5I_UNINIT;
			this->dapl = H5I_UNINIT;
		};
		at_t operator[]( const char arg[] );

		::hid_t dapl;
	};
	/*attribute id*/ // FIXME: eliminate extra fields: ds, name
	template<class T, capi_close_t capi_close>
	struct hid_t<T,capi_close, true,true,hdf5::attribute> : public hid_t<T,capi_close,true,true,hdf5::any> {
		using parent = hid_t<T,capi_close,true,true,hdf5::any>;
		using hid_t<T,capi_close,true,true,hdf5::any>::hid_t;  // inhereting ctor 
		using parent::handle;
		using hidtype = T;
		using at_t = hid_t<h5::impl::at_t,H5Aclose,true,true,hdf5::attribute>;

		hid_t(){
			this->handle = H5I_UNINIT;
			this->ds = H5I_UNINIT;
		};

		template <class V> at_t operator=( V arg  );
		template <class V> at_t operator=( const std::initializer_list<V> args  ){};

		::hid_t ds;
		std::string name;
	};

}}}

namespace h5 { namespace impl {
	// redefine ::hid_t<..,from_capi,to_capi,...> to disable conversion, default setting: hid_t::<.., true,true,..>
	template <class T, capi_close_t capi_call> using aid_t = detail::hid_t<T,capi_call, true,true,detail::hdf5::attribute>;
	template <class T, capi_close_t capi_call> using hid_t = detail::hid_t<T,capi_call, true,true,detail::hdf5::any>;
	template <class T, capi_close_t capi_call> using pid_t = detail::hid_t<T,capi_call, true,true,detail::hdf5::property>;
	template <class T, capi_close_t capi_call> using did_t = detail::hid_t<T,capi_call, true,true,detail::hdf5::dataset>;
}}

/*hide gory details, and stamp out descriptors */
namespace h5 {
	/*base template with no default ctors to prevent instantiation*/
	#define H5CPP__defhid_t( T_, D_ ) namespace impl{struct T_ final {};} using T_ = impl::hid_t<impl::T_,D_>;
	#define H5CPP__defpid_t( T_, D_ ) namespace impl{struct T_ final {};} using T_ = impl::pid_t<impl::T_,D_>;
	#define H5CPP__defdid_t( T_, D_ ) namespace impl{struct T_ final {};} using T_ = impl::did_t<impl::T_,D_>;
	#define H5CPP__defaid_t( T_, D_ ) namespace impl{struct T_ final {};} using T_ = impl::aid_t<impl::T_,D_>;
	/*file:  */ H5CPP__defhid_t(fd_t, H5Fclose) /*dataset:*/	H5CPP__defdid_t(ds_t, H5Dclose) /* <- packet table: is specialization enabled */
	/*attrib:*/ H5CPP__defaid_t(at_t, H5Aclose) /*group:  */	H5CPP__defaid_t(gr_t, H5Gclose) /*object:*/	H5CPP__defhid_t(ob_t, H5Oclose)
	/*space: */ H5CPP__defhid_t(sp_t, H5Sclose) 
	/*datatype:*/   //H5CPP__defhid_t(dt_t, H5Tclose)

	/*each of these properties has a distinct proxy object to handle the details
	 * see: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#AttributeCreatePropFuncs */
	H5CPP__defpid_t(acpl_t,H5Pclose)
	H5CPP__defpid_t(dapl_t,H5Pclose) H5CPP__defpid_t(dxpl_t,H5Pclose) H5CPP__defpid_t(dcpl_t,H5Pclose)
	H5CPP__defpid_t(tapl_t,H5Pclose) H5CPP__defpid_t(tcpl_t,H5Pclose)
	H5CPP__defpid_t(fapl_t,H5Pclose) H5CPP__defpid_t(fcpl_t,H5Pclose) H5CPP__defpid_t(fmpl_t,H5Pclose)
	H5CPP__defpid_t(gapl_t,H5Pclose) H5CPP__defpid_t(gcpl_t,H5Pclose)
	H5CPP__defpid_t(lapl_t,H5Pclose) H5CPP__defpid_t(lcpl_t,H5Pclose)
	H5CPP__defpid_t(ocrl_t,H5Pclose) H5CPP__defpid_t(ocpl_t,H5Pclose)
	H5CPP__defpid_t(scpl_t,H5Pclose)
	#undef H5CPP__defaid_t
	#undef H5CPP__defpid_t
	#undef H5CPP__defhid_t
}
#endif
// T ::= impl::T_ 
// prop_t<h5::fapl_t, default_fapl,  capi, capi_call>
// prop_base< prop_t<phid_t,init,capi,capi_call>, phid_t >
