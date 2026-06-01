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
#include <mutex>       /* H5CPP_MULTITHREAD global HDF5 lock */

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

	// ── process-global HDF5 lock (H5CPP_MULTITHREAD) ─────────────────────────────
	// HDF5 built Threadsafety-OFF keeps its allocator state global and lock-free —
	// the H5FL free-lists and the H5CX API-context — so EVERY C-API call must be
	// mutually exclusive (entering from a second thread, even sequentially, corrupts
	// those lists: ASan shows a free-list block allocated on one thread, double-freed
	// by H5_term_library on another).  This is exactly what HDF5's own
	// --enable-threadsafe build does: ONE global lock around the C-API.
	//
	// Efficiency: take a single process-global mutex ONCE at the outermost h5cpp→HDF5
	// boundary on each thread; nested h5cpp calls just bump a thread-local depth (no
	// re-lock), so a top-level op costs one lock/unlock regardless of how many C-API
	// calls it makes.  In a classic build capi_lock is an empty no-op (zero cost).
	// All C++17 (thread_local + std::mutex) — no version branching needed.
#ifdef H5CPP_MULTITHREAD
	inline std::mutex& hdf5_mutex() noexcept { static std::mutex m; return m; }
	inline thread_local unsigned hdf5_lock_depth = 0u;
	struct capi_lock {
		bool top_;
		capi_lock() noexcept : top_(hdf5_lock_depth++ == 0u) { if (top_) hdf5_mutex().lock(); }
		~capi_lock() { if (top_) hdf5_mutex().unlock(); --hdf5_lock_depth; }
		capi_lock(const capi_lock&) = delete;
		capi_lock& operator=(const capi_lock&) = delete;
	};
	// Close a conversion-off descriptor under the global lock (+ #286 registry
	// detach on the last file ref).  Defined in H5io_registry.hpp (needs the
	// complete registry); forward-declared here to avoid the include cycle
	// H5io_registry → H5Pthreads → H5Pall → H5Tall → H5Iall.
	void close_global(::hid_t handle, capi_close_t capi_close);
#else
	struct capi_lock { capi_lock() = default; };   // classic build: no-op, zero cost
#endif
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
			h5::impl::capi_lock _lk;   // MT: H5I refcount / property-list close touch HDF5 global state
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
			h5::impl::capi_lock _lk;
			if( H5Iis_valid( handle ) )
				H5Iinc_ref( handle );
		}
		hid_t& operator =( const hid_t& ref) {
            if (this == &ref) return *this;
            h5::impl::capi_lock _lk;
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
            h5::impl::capi_lock _lk;
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
			h5::impl::capi_lock _lk;
			if( H5Iis_valid( handle ) ) {
                h5::impl::registry_detach_file( handle );
				capi_close( handle );
            }
		}

		using at_t = hid_t<h5::impl::at_t,H5Aclose,true,true,hdf5::attribute>;
		/**
		 * @brief Attribute indexer — `parent["name"] = value` writes, `T v = parent["name"]` reads.
		 *
		 * Returns a transient `h5::at_t` carrying this parent's handle and the
		 * attribute name. Combine with `at_t::operator=(V)` to write, or with
		 * the templated `at_t::operator V() const` to read. Mirrors the
		 * `h5::create` / `h5::aread` / `h5::awrite` free-function surface.
		 */
		at_t operator[]( const char arg[] );

		// Relinquish the raw id WITHOUT closing it — the destructor becomes a
		// no-op.  Used to build a non-owning borrowed view of a file id (e.g. to
		// drive the sync write gateway on the async collector thread).
		::hid_t release() noexcept { ::hid_t h = handle; handle = H5I_UNINIT; return h; }

		protected:
		::hid_t handle;
	};

	// Conversion-off backing — the hardened / H5CPP_MULTITHREAD boundary.  Same
	// ownership semantics as the true,true backing; the ONLY differences are
	// (1) operator ::hid_t() is *explicit* (no silent decay off the collector) and
	// (2) under H5CPP_MULTITHREAD every close is routed onto the one global
	// collector thread.  Layout is identical to the classic handle (a single
	// ::hid_t) — NO fat member: the collector is a process-global singleton, so
	// nothing needs to be carried per handle (this is the global-realignment
	// payoff — the #286 per-file/fileno lookup and the carried shared_ptr are gone).
	template<class T, capi_close_t capi_close>
	struct hid_t<T,capi_close, false,false,hdf5::any> {
		using hidtype = T;

		// from CAPI — wrapping a raw id into an OWNING handle is NOT a collector
		// bypass (the wrapped handle still routes its close through the collector),
		// so this stays implicit like the classic backing; only the TO-CAPI decay is
		// hardened.  Internal code relies on it, e.g. read(hid_t fd,…) → h5::open(fd).
		H5CPP__EXPLICIT hid_t( ::hid_t handle_ ) : handle( handle_ ){
			h5::impl::capi_lock _lk;   // H5Iis_valid/H5Iinc_ref touch the global H5I table
			if( H5Iis_valid( handle_ ) )
				H5Iinc_ref( handle_ );
		}

		// TO CAPI — EXPLICIT only.  Implicit decay (the silent path a thread could
		// use to call raw HDF5 off the collector) is killed; a deliberate
		// static_cast<hid_t>(x) still works — the visible, on-collector escape
		// h5cpp internals (H5capi.hpp et al.) use.
		explicit operator ::hid_t() const { return handle; }

		// direct-initialization (borrowed view): does not inc_ref.
		hid_t( std::initializer_list<::hid_t> fd ) : handle( *fd.begin() ){}

		hid_t() : handle(H5I_UNINIT) {}

		hid_t( const hid_t& ref ){
			handle = ref.handle;
			h5::impl::capi_lock _lk;
			if( H5Iis_valid( handle ) )
				H5Iinc_ref( handle );
		}
		hid_t& operator=( const hid_t& ref ){
			if( this == &ref ) return *this;
			close_();
			handle = ref.handle;
			h5::impl::capi_lock _lk;
			if( H5Iis_valid( handle ) )
				H5Iinc_ref( handle );
			return *this;
		}
		hid_t( hid_t&& ref ) noexcept {
			handle = ref.handle;
			ref.handle = H5I_UNINIT;
		}
		hid_t& operator=( hid_t&& ref ) noexcept {
			if( this == &ref ) return *this;
			close_();
			handle = ref.handle;
			ref.handle = H5I_UNINIT;
			return *this;
		}
		~hid_t(){ close_(); }

		// Relinquish the raw id WITHOUT closing it — the destructor becomes a
		// no-op.  Mirrors the true,true backing; used to build a non-owning
		// borrowed view of a handle to drive a gateway on the collector thread.
		::hid_t release() noexcept { ::hid_t h = handle; handle = H5I_UNINIT; return h; }

		// Attribute subscript — mirrors the true,true `any` backing so ob_t["name"]
		// resolves (the out-of-line defs live in H5Awrite.hpp).
		using at_t = hid_t<h5::impl::at_t,H5Aclose,false,false,hdf5::attribute>;
		at_t operator[]( const char arg[] );

		// Public so internal h5cpp code reads the raw id directly.  User code
		// routes through h5::write / h5::read / etc.
		::hid_t handle;

	private:
		// Route the close: under H5CPP_MULTITHREAD through the global HDF5 lock
		// (close_global also does the #286 registry detach on the last file ref);
		// otherwise the classic direct close.  The MT branch does H5Iis_valid INSIDE
		// the lock — never an unlocked C-API call.
		void close_() noexcept {
#ifdef H5CPP_MULTITHREAD
			if( handle > 0 ) h5::impl::close_global( handle, capi_close );
#else
			if( H5Iis_valid( handle ) ) {
				h5::impl::registry_detach_file( handle );
				capi_close( handle );
			}
#endif
		}
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

	// Free helper backing at_t's implicit-read conversion (defined out-of-line
	// in H5Aread.hpp once h5::aread is visible). The conversion operator stays
	// inline in the class body and forwards here: MSVC (VS2022 14.4x) emits a
	// C1001 internal compiler error on an *out-of-line* member-template
	// conversion operator of an explicit specialization, which the examples'
	// bracket-syntax reads instantiate. A plain free function template compiles
	// cleanly out-of-line on every toolchain. (#282)
	template <class V> V at_read(::hid_t ds, const std::string& name);

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
		// gr_t["name"] subscript — mirrors the true,true attribute backing.
		at_t operator[]( const char arg[] );

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

		/**
		 * @brief Attribute indexer — `gr["name"] = value` writes, `T v = gr["name"]` reads.
		 *
		 * Returns a transient `h5::at_t` carrying this parent's handle and
		 * the attribute name. Re-declared on this spec (not inherited from
		 * the any spec) so name lookup finds it on the derived type. See
		 * the matching `at_t::operator=(V)` and `at_t::operator V() const`.
		 */
		at_t operator[]( const char arg[] );

		/**
		 * @brief Implicit read: `T v = parent["attr"]`.
		 *
		 * Lives on this spec so it fires on the `at_t` that `operator[]`
		 * returns (which carries the parent `ds` and attribute `name`).
		 * Forwards to `h5::aread<V>(ds, name)`. Excludes `::hid_t` to keep
		 * the base-spec `operator ::hid_t() const` available for plain
		 * `static_cast` on the handle itself.
		 *
		 * @tparam V  any type accepted by `h5::aread<V>` — see @ref link_base_template_types.
		 * @throws h5::error::io::attribute::read  if `ds` is invalid (the
		 *         at_t was default-constructed or its parent was UNINIT).
		 */
		template <class V,
			class = std::enable_if_t<!std::is_same_v<V, ::hid_t>>>
		operator V() const { return h5::impl::detail::at_read<V>(this->ds, this->name); }

		::hid_t ds;
		std::string name;
	};

}

namespace h5::impl {
	// Conversion-policy → backing selection.  detail::hid_t provides two diagonal
	// backings: <true,true> (full conversion, the classic handle) and <false,false>
	// (operator ::hid_t() *explicit* — the hardened / H5CPP_MULTITHREAD boundary).
	// H5CPP_CONVERSION_TO_CAPI_DISABLED (which H5CPP_MULTITHREAD force-defines, see
	// H5config.hpp) flips the public OBJECT-id facade to the conversion-off backing.
	// One facade, two backings, picked at compile time — there is no separate
	// `async::` type set anymore; multithread IS the build mode.
	// Property-list ids (pid_t) stay classic true,true: there is no false,false
	// `property` backing, and plists are constructed/consumed locally, never
	// carried across the collector.
#if defined(H5CPP_CONVERSION_TO_CAPI_DISABLED) || defined(H5CPP_CONVERSION_FROM_CAPI_DISABLED)
	inline constexpr bool from_capi_v = false;
	inline constexpr bool to_capi_v   = false;
#else
	inline constexpr bool from_capi_v = true;
	inline constexpr bool to_capi_v   = true;
#endif
	template <class T, capi_close_t capi_call> using aid_t = detail::hid_t<T,capi_call, from_capi_v,to_capi_v,detail::hdf5::attribute>;
	template <class T, capi_close_t capi_call> using hid_t = detail::hid_t<T,capi_call, from_capi_v,to_capi_v,detail::hdf5::any>;
	template <class T, capi_close_t capi_call> using pid_t = detail::hid_t<T,capi_call, true,true,detail::hdf5::property>;
	template <class T, capi_close_t capi_call> using did_t = detail::hid_t<T,capi_call, from_capi_v,to_capi_v,detail::hdf5::dataset>;
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

	// (The former h5::async:: descriptor namespace and is_async_v<> trait are
	// retired: multithread is now a compile-time build mode — h5::fd_t IS the
	// conversion-off handle under H5CPP_MULTITHREAD — so there is no second type
	// set to distinguish and dispatch is by the macro, not a runtime trait.)
}
