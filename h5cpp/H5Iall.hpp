/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#pragma once
#include <hdf5.h>
#include "H5config.hpp"
#include "H5Eall.hpp"
#include <string>
#include <vector>
#include <tuple>
#include <memory>      /* std::shared_ptr — async descriptor exec field */
#include <initializer_list>

// Slice C (#286): forward-declare the registry helpers used by the RAII
// close path to detach the file-pool entry on H5Fclose.  A direct
// #include of H5io_registry.hpp here would introduce a cycle:
//   H5Iall.hpp → H5io_registry.hpp → H5Pthreads.hpp → H5Pall.hpp
//              → H5Tall.hpp → H5Iall.hpp
// The aggregator (h5cpp/core, h5cpp/all) includes H5io_registry.hpp
// after H5Iall.hpp, where the inline definitions are satisfied.
// registry_detach_file() is a thin free-function shim defined in
// H5io_registry.hpp; forward-declaring it here avoids requiring the
// complete type of io_registry_t.
namespace h5::impl {
    void registry_detach_file(::hid_t file_id);
}

#ifdef H5CPP_CONVERSION_IMPLICIT
	#define H5CPP__EXPLICIT
#else
	#define H5CPP__EXPLICIT explicit
#endif

namespace h5::impl {
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

	// Phase II — async descriptors carry a shared_ptr<executor_t> field
	// directly on the wrapper.  Why direct storage and not the FAPL slot
	// pattern from Phase I:  HDF5 1.10.9's H5Fget_access_plist returns a
	// synthetic FAPL reconstructed from standard properties only; user
	// properties installed via H5Pinsert2 are dropped.  Storing the
	// executor inside the wrapper class lets operation overloads in
	// Phase II PR-B reach it as `fd.exec` without round-tripping through
	// HDF5's property machinery.  std::shared_ptr's type-erased deleter
	// makes the forward declaration sufficient — the complete type is
	// only needed at h5::async::create / open (defined in H5async.hpp).
	struct executor_t;
}

namespace h5::impl::detail {
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
		hid_t() : handle(H5I_UNINIT){};
		hid_t( const hid_t& ref) {
			this->handle = ref.handle;
			if( H5Iis_valid( handle ) )
				H5Iinc_ref( handle );
		}
		hid_t& operator =( const hid_t& ref) {
            if (this == &ref) return *this;
            if( H5Iis_valid( handle ) ) {
                h5::impl::registry_detach_file( handle );
                capi_close( handle );
            }
			handle = ref.handle;
			if( H5Iis_valid( handle ) )
				H5Iinc_ref( handle );
			return *this;
		}
        hid_t& operator =( hid_t&& ref) {
            if (this == &ref) return *this;
            if( H5Iis_valid( handle ) ) {
                h5::impl::registry_detach_file( handle );
                capi_close( handle );
            }
			handle = ref.handle;
            ref.handle = H5I_UNINIT;
			return *this;
		}
		/* move ctor must invalidate old handle */
		hid_t( hid_t<T,capi_close,true,true,hdf5::any>&& ref ){
			handle = ref.handle;
			ref.handle = H5I_UNINIT;
		}
		~hid_t(){
			if( H5Iis_valid( handle ) ) {
                h5::impl::registry_detach_file( handle );
				capi_close( handle );
            }
		}
		protected:
		::hid_t handle;
	};

	// Phase II async-mode specialization — operator ::hid_t() is = delete'd so
	// user code that accidentally hands an async descriptor to a raw HDF5 C
	// API fails to compile with a clear "use of deleted function" diagnostic.
	// h5cpp internal code reaches the raw handle via the public `handle`
	// field (see workplan §4.4); user code routes through h5::write / h5::read
	// / etc. which detect the type via is_async_v<> and dispatch through the
	// FAPL-resolved executor.
	template<class T, capi_close_t capi_close>
	struct hid_t<T,capi_close, false,false,hdf5::any> {
		using hidtype = T;

		// from CAPI — mirrors the true,true ctor; explicit so an accidental
		// implicit promotion from ::hid_t doesn't slip an async wrapper in.
		H5CPP__EXPLICIT hid_t( ::hid_t handle_ ) : handle( handle_ ){
			if( H5Iis_valid( handle_ ) )
				H5Iinc_ref( handle_ );
		}

		// Factory ctor — h5::async::create / open construct the executor
		// during file creation and inject it here so operation overloads
		// (Phase II PR-B) can reach it via `fd.exec`.  Used by mode-
		// transitive factories too (ds_t inherits parent fd's executor).
		hid_t( ::hid_t handle_, std::shared_ptr<h5::impl::executor_t> e ) noexcept
			: handle( handle_ ), exec( std::move(e) ) {}

		// TO CAPI — DELETED.  Async descriptors must not be implicitly
		// converted back to ::hid_t; doing so would let user code call
		// HDF5 directly and bypass the executor thread.  Internal code
		// reads the raw value from `handle` directly.
		operator ::hid_t() const = delete;

		// direct-initialization ctor; matches the classic shape — does not
		// increment the refcount (caller owns the handle).
		hid_t( std::initializer_list<::hid_t> fd ) : handle( *fd.begin() ){}

		hid_t() : handle(H5I_UNINIT) {}

		hid_t( const hid_t& ref ){
			handle = ref.handle;
			if( H5Iis_valid( handle ) )
				H5Iinc_ref( handle );
			exec = ref.exec;             // shared_ptr copy bumps refcount
		}
		hid_t& operator=( const hid_t& ref ){
			if( this == &ref ) return *this;
			if( H5Iis_valid( handle ) )
				capi_close( handle );
			handle = ref.handle;
			if( H5Iis_valid( handle ) )
				H5Iinc_ref( handle );
			exec = ref.exec;
			return *this;
		}
		hid_t( hid_t&& ref ) noexcept {
			handle = ref.handle;
			ref.handle = H5I_UNINIT;
			exec = std::move(ref.exec);
		}
		hid_t& operator=( hid_t&& ref ) noexcept {
			if( this == &ref ) return *this;
			if( H5Iis_valid( handle ) )
				capi_close( handle );
			handle = ref.handle;
			ref.handle = H5I_UNINIT;
			exec = std::move(ref.exec);
			return *this;
		}
		~hid_t(){
			if( H5Iis_valid( handle ) )
				capi_close( handle );
		}

		// Public so internal h5cpp code (the executor, dispatch lambdas)
		// can read the raw id without invoking the deleted conversion.
		// User code is expected to use h5::write / h5::read / h5::async::*
		// factories rather than touch this field directly.
		::hid_t handle;

		// Phase II — shared_ptr to the executor that owns this descriptor's
		// HDF5 lifetime.  Populated by h5::async::create / open at the
		// file-level, then propagated to derived descriptors (async ds,
		// async at, etc.) by mode-transitive factories.  May be null on
		// default-constructed async wrappers (un-initialized state).
		std::shared_ptr<h5::impl::executor_t> exec;
	};

	// Phase II — async dataset id.  Mirrors hdf5::dataset (line above) but
	// with conversion to ::hid_t deleted.  Adds the `dapl` field and the
	// attribute subscript operator the classic ds_t exposes.
	template<class T, capi_close_t capi_close>
	struct hid_t<T,capi_close, false,false,hdf5::dataset>
		: public hid_t<T,capi_close,false,false,hdf5::any> {
		using parent = hid_t<T,capi_close,false,false,hdf5::any>;
		using parent::parent;
		using parent::handle;
		using hidtype = T;
		using at_t = hid_t<h5::impl::at_t,H5Aclose,false,false,hdf5::attribute>;

		hid_t(){
			this->handle = H5I_UNINIT;
			this->dapl   = H5I_UNINIT;
		}
		at_t operator[]( const char arg[] );

		::hid_t dapl;
	};

	// Phase II — async attribute id.
	template<class T, capi_close_t capi_close>
	struct hid_t<T,capi_close, false,false,hdf5::attribute>
		: public hid_t<T,capi_close,false,false,hdf5::any> {
		using parent = hid_t<T,capi_close,false,false,hdf5::any>;
		using parent::parent;
		using parent::handle;
		using hidtype = T;
		using at_t = hid_t<h5::impl::at_t,H5Aclose,false,false,hdf5::attribute>;

		hid_t(){
			this->handle = H5I_UNINIT;
			this->ds     = H5I_UNINIT;
		}

		template <class V> at_t operator=( V arg );
		template <class V> at_t operator=( const std::initializer_list<V> args ){ return at_t{H5I_UNINIT}; }

		::hid_t ds;
		std::string name;
	};
	/*property id*/
	template<class T, capi_close_t capi_close>
	struct hid_t<T,capi_close, true,true,hdf5::property> : public hid_t<T,capi_close,true,true,hdf5::any> {
		using parent = hid_t<T,capi_close,true,true,hdf5::any>;
		using hidtype = T;
        using hid_t<T,capi_close,true,true,hdf5::any>::hid_t;
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
		template <class V> at_t operator=( const std::initializer_list<V> args  ){return at_t{H5I_UNINIT}; };

		::hid_t ds;
		std::string name;
	};

}

namespace h5::impl {
	// redefine ::hid_t<..,from_capi,to_capi,...> to disable conversion, default setting: hid_t::<.., true,true,..>
	template <class T, capi_close_t capi_call> using aid_t = detail::hid_t<T,capi_call, true,true,detail::hdf5::attribute>;
	template <class T, capi_close_t capi_call> using hid_t = detail::hid_t<T,capi_call, true,true,detail::hdf5::any>;
	template <class T, capi_close_t capi_call> using pid_t = detail::hid_t<T,capi_call, true,true,detail::hdf5::property>;
	template <class T, capi_close_t capi_call> using did_t = detail::hid_t<T,capi_call, true,true,detail::hdf5::dataset>;

	// Phase II — async-mode variants.  Same shape as the classic aliases
	// above but with operator ::hid_t() = delete'd at the type level.
	// Users opt in by calling h5::async::create / h5::async::open; everything
	// downstream deduces these types through TAD.
	template <class T, capi_close_t capi_call> using async_aid_t = detail::hid_t<T,capi_call, false,false,detail::hdf5::attribute>;
	template <class T, capi_close_t capi_call> using async_hid_t = detail::hid_t<T,capi_call, false,false,detail::hdf5::any>;
	template <class T, capi_close_t capi_call> using async_did_t = detail::hid_t<T,capi_call, false,false,detail::hdf5::dataset>;
}

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
	 * see: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html */
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

	// Phase II — async-mode descriptor type aliases.  Parallel to the
	// classic h5::fd_t / h5::ds_t / h5::gr_t / h5::at_t above; the
	// underlying class template is the false,false specialization of
	// impl::hid_t so any attempt to pass one of these to a raw HDF5
	// C-API call fails with "use of deleted function".
	namespace async {
		using fd_t   = impl::async_hid_t<impl::fd_t,  H5Fclose>;
		using ds_t   = impl::async_did_t<impl::ds_t,  H5Dclose>;
		using at_t   = impl::async_aid_t<impl::at_t,  H5Aclose>;
		using gr_t   = impl::async_aid_t<impl::gr_t,  H5Gclose>;
		using ob_t   = impl::async_hid_t<impl::ob_t,  H5Oclose>;
	}

	// Phase II type-trait: is_async_v<T> answers "is T one of the
	// h5::async::* descriptors?".  Used by concept-constrained operation
	// overloads (Phase II PR-B) to pick the executor dispatch branch.
	template <class T>
	struct is_async : std::false_type {};

	template <class T, impl::capi_close_t C, int K>
	struct is_async< impl::detail::hid_t<T,C,false,false,K> > : std::true_type {};

	template <class T>
	inline constexpr bool is_async_v = is_async<std::decay_t<T>>::value;
}
