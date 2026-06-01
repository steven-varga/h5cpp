/*
 * Copyright (c) 2018-2026 Steven Varga / Varga Labs — MIT
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Per-file I/O worker-pool registry — slice A (singleton + registry).
 *
 * Problem (issue #286):
 *   HDF5 (threadsafety OFF) silently strips every user property installed
 *   via H5Pinsert2 when H5Fget_access_plist reconstructs the FAPL from an
 *   open file id.  As a result, the worker_pool_t installed by h5::threads{N}
 *   on the original FAPL is unreachable at the write/read dispatch sites.
 *
 * Fix:
 *   Populate this process-global registry at file open/create time, while
 *   the original FAPL is still live.  Key = H5Fget_fileno (inode-like
 *   kernel file number), value = per-file state (pool + backpressure cap +
 *   open-descriptor reference count).  Dispatch sites call
 *   h5::impl::registry().resolve_pool(fileno) instead of going through HDF5.
 *
 * Thread-safety: all public methods of io_registry_t are guarded by mu_.
 *
 * Lifetime:
 *   attach() is called once per successful H5Fcreate / H5Fopen; detach()
 *   once per H5Fclose.  The last detach() for a given fileno releases the
 *   shared_ptr<worker_pool_t>, which joins the worker threads.
 */
#pragma once

#include "H5Pthreads.hpp"   // worker_pool_t, resolve_worker_pool, resolve_backpressure

#include <hdf5.h>

#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

namespace h5::impl {

// ─── singleton_t — CRTP shared_ptr singleton ─────────────────────────────────
//
// MIT-relicensed adaptation of sigma::singleton_t.  The derived class T must
// befriend singleton_t<T> and declare its constructor protected/private.
//
// Usage:
//   class Foo : public singleton_t<Foo> {
//       friend class singleton_t<Foo>;
//       Foo() = default;
//   public:
//       void hello();
//   };
//   auto sp = singleton_t<Foo>::initialize();   // first call — constructs
//   auto sp2 = singleton_t<Foo>::get_instance(); // subsequent calls
//   singleton_t<Foo>::is_initialized();          // query without throwing

template<class T>
class singleton_t {
public:
    singleton_t(const singleton_t&)            = delete;
    singleton_t(singleton_t&&)                 = delete;
    singleton_t& operator=(const singleton_t&) = delete;
    singleton_t& operator=(singleton_t&&)      = delete;

    // Construct T (forwarding args to its constructor) and store the
    // shared_ptr.  Throws std::logic_error if called more than once.
    template<class... A>
    static std::shared_ptr<T> initialize(A&&... a) {
        if (instance) {
            throw std::logic_error(
                "singleton_t<T>::initialize() called after the singleton "
                "has already been constructed");
        }
        instance = std::shared_ptr<T>(new T(std::forward<A>(a)...));
        return instance;
    }

    // Return the live shared_ptr.  Throws std::logic_error if initialize()
    // has not been called yet.
    static std::shared_ptr<T> get_instance() {
        if (!instance) {
            throw std::logic_error(
                "singleton_t<T>::get_instance() called before initialize()");
        }
        return instance;
    }

    // Non-throwing predicate — safe to call from any context.
    static bool is_initialized() noexcept {
        return static_cast<bool>(instance);
    }

protected:
    singleton_t()  = default;
    ~singleton_t() = default;

private:
    static std::shared_ptr<T> instance;
};

template<class T>
std::shared_ptr<T> singleton_t<T>::instance = nullptr;


// ─── file_io_t — per-file runtime state ──────────────────────────────────────

struct file_io_t {
    std::shared_ptr<worker_pool_t> pool;  // from h5::threads{N}, resolved on the live FAPL
    unsigned cap  = 0;                    // backpressure cap (in-flight chunk limit)
    unsigned refs = 0;                    // number of live opens referencing this fileno
};


// ─── io_registry_t — process-global per-file pool registry ───────────────────

class io_registry_t : public singleton_t<io_registry_t> {
    friend class singleton_t<io_registry_t>;

    std::mutex                                       mu_;
    std::unordered_map<unsigned long, file_io_t>     map_;   // key = H5Fget_fileno

    io_registry_t() = default;

public:
    // Register a file.
    //
    // First attach for a given fileno: store pool + cap, set refs = 1.
    // Subsequent attaches (same file opened again under the same fileno):
    // increment refs only — existing pool + cap are preserved.  This
    // matches POSIX inode semantics: two H5Fopen calls on the same
    // underlying file share one pool.
    inline void attach(unsigned long fileno,
                       std::shared_ptr<worker_pool_t> pool,
                       unsigned cap) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = map_.find(fileno);
        if (it == map_.end()) {
            map_.emplace(fileno, file_io_t{std::move(pool), cap, 1u});
        } else {
            it->second.refs++;
        }
    }

    // Retrieve the worker pool for fileno.  Returns nullptr when no pool
    // is registered (the file was opened without h5::threads{N}, or the
    // registry entry has been detached).
    inline std::shared_ptr<worker_pool_t> resolve_pool(unsigned long fileno) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = map_.find(fileno);
        if (it == map_.end()) return nullptr;
        return it->second.pool;
    }

    // Retrieve the backpressure cap for fileno.  Returns 0 when no entry
    // exists (caller should already have detected the absent pool and
    // fallen back to synchronous dispatch).
    inline unsigned resolve_cap(unsigned long fileno) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = map_.find(fileno);
        if (it == map_.end()) return 0u;
        return it->second.cap;
    }

    // Decrement the reference count for fileno.  Erases the entry (and
    // releases the pool shared_ptr) when the count reaches zero, which
    // triggers worker_pool_t::~worker_pool_t() → joins all worker threads
    // when no other shared_ptr holder remains.
    inline void detach(unsigned long fileno) {
        // Extract the dying pool under the lock, but let it DESTRUCT after
        // unlocking: the pool joins its workers, and doing that under mu_ could
        // deadlock against a concurrent registry call from those threads — and a
        // join is too slow to hold a global lock across.
        std::shared_ptr<worker_pool_t> dying_pool;
        {
            std::lock_guard<std::mutex> lk(mu_);
            auto it = map_.find(fileno);
            if (it == map_.end()) return;
            if (it->second.refs <= 1u) {
                dying_pool = std::move(it->second.pool);
                map_.erase(it);
            } else {
                it->second.refs--;
            }
        }
        // dying_pool destructs here, outside the registry lock.
    }
};


// ─── registry() — lazy singleton accessor ────────────────────────────────────

// Returns a reference to the live io_registry_t.  On the first call a
// std::once_flag ensures singleton_t<io_registry_t>::initialize() is
// invoked exactly once, even under concurrent first-callers.  All later
// calls skip the once_flag fast-path and return the cached reference.
inline io_registry_t& registry() {
    static std::once_flag flag;
    std::call_once(flag, [] { singleton_t<io_registry_t>::initialize(); });
    return *singleton_t<io_registry_t>::get_instance();
}


// ─── file key helpers ─────────────────────────────────────────────────────────

// Obtain the kernel file number from a FILE id (h5::fd_t / raw ::hid_t of
// type H5I_FILE).  Caller already holds a file id; no temporary is needed.
inline unsigned long file_key_of_file(::hid_t file_id) {
    unsigned long fileno = 0;
    H5Fget_fileno(file_id, &fileno);
    return fileno;
}

// Obtain the kernel file number from any valid HDF5 id (dataset, group,
// attribute, …).  Retrieves the owning file id via H5Iget_file_id (which
// increments the file's reference count), reads the fileno, then closes
// the temporary file id to restore the reference count.
inline unsigned long file_key(::hid_t id_in_file) {
    ::hid_t file_id = H5Iget_file_id(id_in_file);  // refcount++
    unsigned long fileno = 0;
    if (file_id >= 0) {
        H5Fget_fileno(file_id, &fileno);
        H5Fclose(file_id);                          // refcount--
    }
    return fileno;
}

// ─── registry_detach_file — shim callable from H5Iall.hpp ───────────────────
//
// Thin free-function that H5Iall.hpp forward-declares to avoid pulling in the
// full H5io_registry.hpp (which would introduce an include cycle through
// H5Pthreads.hpp → H5Pall.hpp → H5Tall.hpp → H5Iall.hpp).
//
// Called from the hid_t<..,true,true,hdf5::any> destructor and move/copy
// assign paths immediately before capi_close(handle) when the handle is a
// FILE id.  The guard checks H5Iis_valid and H5I_FILE so dataset / group /
// property-list closes are untouched.
inline void registry_detach_file(::hid_t handle) {
    if (handle > 0 && H5Iis_valid(handle) && H5Iget_type(handle) == H5I_FILE)
        registry().detach(file_key_of_file(handle));
}

#ifdef H5CPP_MULTITHREAD
// ─── close_global — close a conversion-off descriptor under the global HDF5 lock ─
//
// Every HDF5 close on a conversion-off handle runs while holding the one global
// HDF5 lock (capi_lock), so it never races another thread's C-API call.  The
// fileno is derived from the id itself (H5Fget_fileno) under the lock.  On the
// last file-id ref it also detaches the #286 registry pool entry — done OUTSIDE
// the HDF5 lock, because detach may join the worker pool (slow, and the pool
// threads must never need the HDF5 lock while we hold it).  Defined here (not
// H5Iall) because it needs the complete registry() — H5Iall only forward-declares it.
inline void close_global(::hid_t handle, capi_close_t capi_close) {
    if (handle <= 0) return;
    unsigned long fileno = 0;
    bool last_file = false;
    {
        capi_lock _lk;                                     // serialize with all other HDF5
        if (!H5Iis_valid(handle)) return;
        last_file = (H5Iget_type(handle) == H5I_FILE) && (H5Iget_ref(handle) <= 1);
        if (last_file) fileno = file_key_of_file(handle);  // H5Fget_fileno — under the lock
        capi_close(handle);                                // the actual close — under the lock
    }
    if (last_file) registry().detach(fileno);              // map op + pool join — OUTSIDE the lock
}
#endif

} // namespace h5::impl
