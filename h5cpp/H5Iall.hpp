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

	template<class hid, class... args_tt> 
	struct capi_t {
		using fn_t = herr_t (*)(::hid_t, args_tt... );
		using args_t = std::tuple<args_tt...>;
		using type = std::tuple<::hid_t,args_tt...>;
	};

}}

namespace h5 { namespace impl { namespace detail {

	// base template with T type, the capi_close function, and 
	// whether you allow conversion from_capi and to_capi of the hid_t type
	template <class T, capi_close_t capi_close,
	// conversion policy is controlled by template specialization
	// from_capi, to_capi whether you want capi hid_t converted to h5cpp typed hid_t<T> classes
			 bool from_capi, bool to_capi, bool is_property>
	struct hid_t final {
		hid_t()=delete;
	};
	// actual implementation with full conversion allowed
	template<class T, capi_close_t capi_close>
	struct hid_t<T,capi_close, true,true,false> {
		using hidtype = T;
		// from CAPI
		H5CPP__EXPLICIT hid_t( ::hid_t handle_ ) : handle( handle_ ){
			if( H5Iis_valid( handle_ ) )
				int count = H5Iinc_ref( handle_ );
		}
		// TO CAPI
		H5CPP__EXPLICIT operator ::hid_t() const {
			return  handle;
		}
        hid_t( std::initializer_list<::hid_t> fd )
		   : handle( *fd.begin()){
		}
		//TODO: have default constructor such that can initialize properties
		// see 'create.hpp line 164:  h5::dcpl_t dcpl = h5::dcpl_t {H5Pcreate(H5P_DATASET_CREATE)};
		// which is awkward
		hid_t() : handle(H5I_UNINIT){};
		hid_t( const hid_t& ref) {
			this->handle = ref.handle;
			H5Iinc_ref( handle );
		}
		hid_t& operator =( const hid_t& ref) {
			handle = ref.handle;
			H5Iinc_ref( handle );
			return *this;
		}
		/* move ctor must invalidate old handle */
		hid_t( hid_t<T,capi_close,true,true,false>&& ref ){
			handle = ref.handle;
			ref.handle = H5I_UNINIT;
		}
		~hid_t(){
			::herr_t err = 0;
			if( H5Iis_valid( handle ) )
				err = capi_close( handle );
		}
		private:
		::hid_t handle;
	};

	// disable from CAPI and TOCAPI conversions 
	//conversion ctor to packet table enabled, used for h5::impl::ds_t
	template<class T, capi_close_t capi_close>
	struct hid_t<T,capi_close, false,false,false> : private hid_t<T,capi_close,true,true,false> {
		using parent = hid_t<T,capi_close,true,true,false>;
		using parent::hid_t; // is a must because of ds_t{hid_t} ctor 
		using hidtype = T;
	};
	/*property id*/
	template<class T, capi_close_t capi_close>
	struct hid_t<T,capi_close, true,true,true> : public hid_t<T,capi_close,true,true,false> {
		using parent = hid_t<T,capi_close,true,true,false>;
		using parent::hid_t; // is a must because of ds_t{hid_t} ctor 
		using hidtype = T;

		hid_t& operator |=( const hid_t& ref){
			return *this;
		}

		hid_t& operator |( const hid_t& ref){
			return *this;
		}
	};
}}}

namespace h5 { namespace impl {
	// redefine this to disable conversion
	template <class T, capi_close_t capi_call> using hid_t = detail::hid_t<T,capi_call, true,true,false>;
	template <class T, capi_close_t capi_call> using pid_t = detail::hid_t<T,capi_call, true,true,true>;
}}
/*hide details */
namespace h5 {
	/*base template with no default ctors to prevent instantiation*/
	#define H5CPP__defhid_t( T_, D_ ) namespace impl{struct T_ final {};} using T_ = impl::hid_t<impl::T_,D_>;
	#define H5CPP__defpid_t( T_, D_ ) namespace impl{struct T_ final {};} using T_ = impl::pid_t<impl::T_,D_>;
	/*file:  */ H5CPP__defhid_t(fd_t, H5Fclose) /*dataset:*/	H5CPP__defhid_t(ds_t, H5Dclose) /* <- packet table: is specialization enabled */
	/*attrib:*/ H5CPP__defhid_t(at_t, H5Aclose) /*group:  */	H5CPP__defhid_t(gr_t, H5Gclose) /*object:*/	H5CPP__defhid_t(ob_t, H5Oclose)
	/*space: */ H5CPP__defhid_t(sp_t, H5Sclose) /*datatype:*/   H5CPP__defhid_t(dt_t, H5Tclose)

	/*each of these properties has a distinct proxy object to handle the details*/
	H5CPP__defpid_t(acpl_t,H5Pclose)
	H5CPP__defpid_t(dapl_t,H5Pclose) H5CPP__defpid_t(dxpl_t,H5Pclose) H5CPP__defpid_t(dcpl_t,H5Pclose)
	H5CPP__defpid_t(tapl_t,H5Pclose) H5CPP__defpid_t(tcpl_t,H5Pclose)
	H5CPP__defpid_t(fapl_t,H5Pclose) H5CPP__defpid_t(fcpl_t,H5Pclose) H5CPP__defpid_t(fmpl_t,H5Pclose)
	H5CPP__defpid_t(gapl_t,H5Pclose) H5CPP__defpid_t(gcpl_t,H5Pclose)
	H5CPP__defpid_t(lapl_t,H5Pclose) H5CPP__defpid_t(lcpl_t,H5Pclose)
	H5CPP__defpid_t(ocrl_t,H5Pclose) H5CPP__defpid_t(ocpl_t,H5Pclose)
	H5CPP__defpid_t(scpl_t,H5Pclose)
	#undef H5CPP__defpid_t
	#undef H5CPP__defhid_t
}

namespace h5 { namespace impl {
	/* proxy object that gets converted to property_id with restriction that 
	 * only same class properties may be daisy chained */
	template <class Derived, class phid_t>
	struct prop_base {
		prop_base(){ }
		// removed ctor
		~prop_base(){
			if( H5Iis_valid( handle ) ){
				H5Pclose( handle );
			}
	  	}

		template <class R> 	const prop_base<Derived, phid_t>&
		operator|( const R& rhs ) const {
			rhs.copy( handle );
			return *this;
		 }

		void copy(::hid_t handle_) const { /*CRTP idiom*/
			static_cast<const Derived*>(this)->copy_impl( handle_ );
		}
		/*transfering ownership to managed handle*/
		operator phid_t( ) const {
			static_cast<const Derived*>(this)->copy_impl( handle );
			H5Iinc_ref( handle ); /*keep this alive */
			return phid_t{handle};
		}
		::hid_t handle;
		using type = phid_t;
	};
	// definition
	template <class phid_t, defprop_t init, class capi, typename capi::fn_t capi_call> struct prop_t;
	/* CAPI macros are sequence calls: (H5OPEN, register_property_class), these are all meant to be read only/copied from,
	 */
	#define H5CPP__capicall( name, default_id ) ::hid_t default_##name(){ return default_id; } 									\
			template <class capi, typename capi::fn_t capi_call>  \
				using name##_call = prop_t<h5::name##_t, default_##name,  capi, capi_call>; 	\
			template <class... args> using name##_args = impl::capi_t<args...>; 								\

	H5CPP__capicall( acpl, H5P_ATTRIBUTE_CREATE) H5CPP__capicall( dapl, H5P_DATASET_ACCESS )
	H5CPP__capicall( dxpl, H5P_DATASET_XFER )    H5CPP__capicall( dcpl, H5P_DATASET_CREATE )
	H5CPP__capicall( tapl, H5P_DATATYPE_ACCESS ) H5CPP__capicall( tcpl, H5P_DATATYPE_CREATE )
	H5CPP__capicall( fapl, H5P_FILE_ACCESS )     H5CPP__capicall( fcpl, H5P_FILE_CREATE )
	H5CPP__capicall( fmpl, H5P_FILE_MOUNT )

	H5CPP__capicall( gapl, H5P_GROUP_ACCESS)     H5CPP__capicall( gcpl, H5P_GROUP_CREATE )
	H5CPP__capicall( lapl, H5P_LINK_ACCESS)      H5CPP__capicall( lcpl, H5P_LINK_CREATE	 )
	H5CPP__capicall( ocpl, H5P_OBJECT_COPY)      H5CPP__capicall( ocrl, H5P_OBJECT_CREATE )
	H5CPP__capicall( scpl, H5P_STRING_CREATE)
	#undef H5CPP__defid

	/*property with a capi function call with some arguments, also is the base for other properties */
	template <class phid_t, defprop_t init, class capi, typename capi::fn_t capi_call>
	struct prop_t : prop_base<prop_t<phid_t,init,capi,capi_call>,phid_t> {
		prop_t( typename capi::args_t values ) : args( values ) {
			this->handle = H5Pcreate(init());
		}
		prop_t(){
			this->handle = H5Pcreate(init());
		}
		void copy_impl(::hid_t id) const {
			//int i = capi_call + 1;
			/*CAPI needs `this` hid_t id passed along */
			capi_t capi_args = std::tuple_cat( std::tie(id), args );
			compat::apply(capi_call, capi_args);
		}
		using base_t =  prop_base<prop_t<phid_t,init,capi,capi_call>,phid_t>;
		using args_t = typename capi::args_t;
		using capi_t = typename capi::type;
		using type = phid_t;
		args_t args;
	};

	/* T prop is much similar to other properties, except the value needs HDF5 type info and passed by
	 * const void* pointer type. This is reflected by templating `prop_t` 
	 */
	template <class phid_t, defprop_t init, class capi, typename capi::fn_t capi_call, class T>
	struct tprop_t final : prop_t<phid_t,init,capi,capi_call> { // ::hid_t,const void*
		tprop_t( T value ) : base_t( std::make_tuple( utils::h5type<T>(), &this->value) ), value(value) {
		}
		~tprop_t(){ // don't forget that the first argument is a HDF5 type info, that needs closing
			if( H5Iis_valid( std::get<0>(this->args) ) )
				H5Tclose( std::get<0>(this->args) );
		}
		using type = phid_t;
		using base_t = prop_t<phid_t,init,capi,capi_call>;
		T value;
	};
	/* takes an arbitrary length of hsize_t sequence and calls H5Pset_xxx
	 */
	template <class phid_t, defprop_t init, class capi, typename capi::fn_t capi_call>
	struct aprop_t final : prop_t<phid_t,init,capi,capi_call> {
		aprop_t( std::initializer_list<hsize_t> values )
			: base_t( std::make_tuple( values.size(), this->values) ) {
			std::copy( std::begin(values), std::end(values), this->values);
		}
		hsize_t values[H5CPP_MAX_RANK];
		using type = phid_t;
		using base_t =  prop_t<phid_t,init,capi,capi_call>;
	};

	::hid_t get_aaa(){
		return 0;
	}

	// only data control property list set_chunk has this pattern, lets allow to define CAPI argument lists 
	// the same way as with others
	template <class capi, typename capi::fn_t capi_call>
		using dcpl_acall = aprop_t<h5::dcpl_t,default_dcpl, capi,capi_call>;
	// only data control property list set_value has this pattern, lets allow to define CAPI argument lists 
	// the same way as with others
	template <class capi, typename capi::fn_t capi_call, class T> using dcpl_tcall = tprop_t<h5::dcpl_t,default_dcpl, capi, capi_call, T>;
}}
#endif

