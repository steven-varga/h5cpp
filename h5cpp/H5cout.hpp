/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include "H5capi.hpp"
#include "H5misc.hpp"
#include "H5Eall.hpp"
#include <ostream>
#include <string>
#include <vector>
#include <limits>
#include <cstdlib>

inline std::ostream& operator<< (std::ostream& os, const h5::dxpl_t& dxpl) {
	os <<"handle: " << static_cast<hid_t>( dxpl );
#ifdef H5_HAVE_PARALLEL
	H5D_mpio_actual_io_mode_t io_mode;
	H5Pget_mpio_actual_io_mode( static_cast<hid_t>(dxpl), &io_mode);

		switch( io_mode ){
			case H5D_MPIO_NO_COLLECTIVE:
				os << "No collective I/O was performed. Collective I/O was not requested or collective I/O isn't possible on this dataset.";
				break;
			case H5D_MPIO_CHUNK_INDEPENDENT:
				os << "HDF5 performed one the chunk collective optimization schemes and each chunk was accessed independently.";
				break;
			case H5D_MPIO_CHUNK_COLLECTIVE:
				os << "HDF5 performed one the chunk collective optimization schemes and each chunk was accessed collectively";
				break;
			case H5D_MPIO_CHUNK_MIXED:
				os <<"HDF5 performed one the chunk collective optimization schemes and some chunks were accessed independently, some collectively";
				break;
			case H5D_MPIO_CONTIGUOUS_COLLECTIVE:
				os <<"Collective I/O was performed on a contiguous dataset.";
			   	break;
		}
#endif
    return os;
}

template <class T> inline
std::ostream& operator<<(std::ostream& os, const h5::impl::array<T>& arr){
	os << "{";
	if( arr.rank )
		// rank > 0 such as: vector,matrix,cube,...
		for(int i=0;i<arr.rank; i++){
			char sep = i != arr.rank - 1  ? ',' : '}';
			if( arr[i] < std::numeric_limits<hsize_t>::max() )
				os << arr[i] << sep;
			else
				os << "inf" << sep;
		}
	else // rank 0 : single values
		os << "n/a}";
	return os;
}

inline
std::ostream& operator<<(std::ostream &os, const h5::sp_t& sp) {
	//htri_t H5Sis_regular_hyperslab( hid_t space_id ) 1.10.0
	//herr_t H5Sget_select_bounds(hid_t space_id, hsize_t *start, hsize_t *end )
	// hssize_t H5Sget_select_npoints( hid_t space_id )
	hid_t id = static_cast<hid_t>( sp );
	#if H5_VERSION_GE(1,10,0)

	#endif
	h5::mute();

	h5::offset_t start,end;
	h5::current_dims_t current_dims;
	h5::max_dims_t max_dims;
	unsigned rank = h5::get_simple_extent_dims( sp, current_dims, max_dims);
	hsize_t total_elements = H5Sget_simple_extent_npoints( id );
 	H5Sget_select_bounds(id, *start, *end);
	start.rank = end.rank = rank;
	hssize_t nblocks = H5Sget_select_hyper_nblocks( id );
	hssize_t nelements = H5Sget_select_elem_npoints( id );

	// if slection valid
	std::string is_valid;
	htri_t is_valid_ = H5Sselect_valid(id);
	if(is_valid_ > 0 ) is_valid = "within extent";
	if(is_valid_ == 0 ) is_valid = "not in extent";
	if(is_valid_ < 0 ) is_valid = "error occured";

    os << "[rank]\t" << rank << "\t[total elements]\t" << total_elements << std::endl;
   	os << "[dimensions]\tcurrent: " << current_dims << "\tmaximum: " << max_dims << std::endl;
	os << "[selection]\tstart: " << start << "\tend:" << end << std::endl;
	os << "[selection]\t" << is_valid <<std::endl;
	// nblocks is -1 when no hyperslab is selected; guard to avoid calloc overflow
	if( nblocks > 0 ){
		hsize_t ncoordinates = static_cast<hsize_t>(2 * rank) * static_cast<hsize_t>(nblocks);
		h5::impl::unique_ptr<hsize_t> buffer{
				static_cast<hsize_t*>( std::calloc( ncoordinates, sizeof(hsize_t))) };
		if( H5Sget_select_hyper_blocklist(id, 0, static_cast<hsize_t>(nblocks), buffer.get() ) >= 0 ){
			os << "[selected element count]\t" << nelements << std::endl;
			os << "[selected block count]\t" << nblocks <<std::endl;
			os << "[selected blocks]\t";
			for( hsize_t i=0; i<static_cast<hsize_t>(nblocks); i++){
				os << "[{";
				for(hsize_t j=0; j<rank; j++) os << *( buffer.get() + i*2*rank+j ) << (j < rank-1 ? "," : "}{");
				for(hsize_t j=rank; j<2*rank; j++) os << *( buffer.get() + i*2*rank+j ) << ( j < 2*rank-1 ? "," : "}");
				os << "] ";
			}
		}
	}
	h5::unmute();
	return os;
}

// std::vector<T> stream insertion is now handled by the generic STL
// pretty-printer in H5Uall.hpp (operator<< for any iterable container). That
// version recurses into the element type so nested forms like
// vector<vector<T>>, vector<array<T,N>>, vector<pair<K,V>> all stream
// correctly. The previous vector-only overload was kept here for legacy
// reasons but didn't recurse and lost element types like std::array; removed.


// =============================================================================
// Handle pretty-printers — generic + selective specializations
// =============================================================================
//
// Every h5cpp handle (fd_t / ds_t / gr_t / at_t / sp_t / 14 property-list
// variants / 5 async variants) is an alias of h5::impl::detail::hid_t<...>.
// The generic operator<< below covers them all uniformly: handle id, RAII
// validity, refcount. Higher-information specializations follow for the
// six handles where deep introspection is most valuable (fd_t, ds_t, gr_t,
// at_t, dcpl_t, fapl_t).
//
// All printers bracket their HDF5 introspection in h5::mute()/unmute() so
// invalid or H5I_UNINIT handles don't dump error stacks into the output.
//
// Note: these printers call H5I* / H5P* / H5D* introspection APIs and are
// not free — fine for debug logs, avoid inside hot loops.
// =============================================================================

namespace h5::impl::detail {
    // Human-readable tag per handle. Default falls back to "hid_t".
    template <class T> constexpr const char* class_tag() noexcept { return "hid_t"; }
}

// One specialization per impl tag struct (matches the H5CPP__defXid_t macros
// at H5Iall.hpp:329-343). Async variants share the same tag struct as their
// classic counterparts, so they reuse these specializations.
#define H5CPP__class_tag(T_, NAME_) \
    namespace h5::impl::detail { \
        template <> inline constexpr const char* class_tag<h5::impl::T_>() noexcept { return NAME_; } \
    }
H5CPP__class_tag(fd_t,   "fd_t")
H5CPP__class_tag(ds_t,   "ds_t")
H5CPP__class_tag(at_t,   "at_t")
H5CPP__class_tag(gr_t,   "gr_t")
H5CPP__class_tag(ob_t,   "ob_t")
H5CPP__class_tag(sp_t,   "sp_t")
H5CPP__class_tag(acpl_t, "acpl_t")
H5CPP__class_tag(dapl_t, "dapl_t")
H5CPP__class_tag(dxpl_t, "dxpl_t")
H5CPP__class_tag(dcpl_t, "dcpl_t")
H5CPP__class_tag(tapl_t, "tapl_t")
H5CPP__class_tag(tcpl_t, "tcpl_t")
H5CPP__class_tag(fapl_t, "fapl_t")
H5CPP__class_tag(fcpl_t, "fcpl_t")
H5CPP__class_tag(fmpl_t, "fmpl_t")
H5CPP__class_tag(gapl_t, "gapl_t")
H5CPP__class_tag(gcpl_t, "gcpl_t")
H5CPP__class_tag(lapl_t, "lapl_t")
H5CPP__class_tag(lcpl_t, "lcpl_t")
H5CPP__class_tag(ocrl_t, "ocrl_t")
H5CPP__class_tag(ocpl_t, "ocpl_t")
H5CPP__class_tag(scpl_t, "scpl_t")
#undef H5CPP__class_tag

namespace h5::impl::detail {
    // Generic printer — works for both classic (true,true) and async
    // (false,false) variants because both expose the raw `handle` field.
    template <class T, capi_close_t F, bool From, bool To, int K>
    inline std::ostream& operator<<(std::ostream& os, const hid_t<T,F,From,To,K>& h) {
        h5::mute();
        // Classic (true,true) variants have `handle` protected — use the
        // explicit converting operator. Async (false,false) variants delete
        // that operator but expose `handle` publicly.
        ::hid_t id;
        if constexpr (From && To) id = static_cast<::hid_t>(h);
        else                      id = h.handle;
        os << "h5::" << class_tag<T>() << "{ handle=" << id;
        if (id != H5I_UNINIT && H5Iis_valid(id) > 0) {
            os << " valid=yes refs=" << H5Iget_ref(id) << " }";
        } else {
            os << " valid=no }";
        }
        h5::unmute();
        return os;
    }
}

// The non-template specializations below MUST live in h5::impl::detail
// (the namespace the hid_t<...> primary template is defined in) so ADL
// finds them. The h5::fd_t / h5::ds_t / etc. type aliases do not introduce
// `namespace h5` into their associated-namespace set.
namespace h5::impl::detail {

    // h5::fd_t — file path + open mode + libver bounds + file size.
    inline std::ostream& operator<<(std::ostream& os, const h5::fd_t& fd) {
        h5::mute();
        const ::hid_t id = static_cast<::hid_t>(fd);
        os << "h5::fd_t{ handle=" << id;
        if (id == H5I_UNINIT || H5Iis_valid(id) <= 0) { os << " valid=no }"; h5::unmute(); return os; }

        // Path on disk: H5Fget_name needs sizing first.
        ssize_t name_len = H5Fget_name(id, nullptr, 0);
        std::string path(name_len > 0 ? static_cast<size_t>(name_len) : 0, '\0');
        if (name_len > 0) H5Fget_name(id, path.data(), name_len + 1);
        os << " path='" << path << "'";

        // Open intent (read-only vs read-write).
        unsigned intent = 0;
        if (H5Fget_intent(id, &intent) >= 0)
            os << " mode=" << ((intent & H5F_ACC_RDWR) ? "RDWR" : "RDONLY");

        // libver bounds from the file's FAPL.
        ::hid_t fapl = H5Fget_access_plist(id);
        if (fapl >= 0) {
            H5F_libver_t low = H5F_LIBVER_EARLIEST, high = H5F_LIBVER_EARLIEST;
            if (H5Pget_libver_bounds(fapl, &low, &high) >= 0)
                os << " libver=[" << static_cast<int>(low) << "," << static_cast<int>(high) << "]";
            H5Pclose(fapl);
        }

        // File size in bytes.
        hsize_t fsize = 0;
        if (H5Fget_filesize(id, &fsize) >= 0)
            os << " size=" << fsize;

        os << " }";
        h5::unmute();
        return os;
    }

    // h5::ds_t — dataset path + class + rank + dims + chunk dims if chunked + filter count.
    inline std::ostream& operator<<(std::ostream& os, const h5::ds_t& ds) {
        h5::mute();
        const ::hid_t id = static_cast<::hid_t>(ds);
        os << "h5::ds_t{ handle=" << id;
        if (id == H5I_UNINIT || H5Iis_valid(id) <= 0) { os << " valid=no }"; h5::unmute(); return os; }

        // Object path (may be empty if anonymous / not yet linked).
        ssize_t name_len = H5Iget_name(id, nullptr, 0);
        std::string path(name_len > 0 ? static_cast<size_t>(name_len) : 0, '\0');
        if (name_len > 0) H5Iget_name(id, path.data(), name_len + 1);
        os << " path='" << path << "'";

        // Datatype class + size.
        ::hid_t tid = H5Dget_type(id);
        if (tid >= 0) {
            static const char* tcls[] = {"INTEGER","FLOAT","TIME","STRING","BITFIELD","OPAQUE",
                                         "COMPOUND","REFERENCE","ENUM","VLEN","ARRAY"};
            H5T_class_t c = H5Tget_class(tid);
            if (c >= 0 && c < 11) os << " dtype=" << tcls[c];
            os << " elsize=" << H5Tget_size(tid);
            H5Tclose(tid);
        }

        // Dataspace dims.
        ::hid_t sid = H5Dget_space(id);
        if (sid >= 0) {
            int rank = H5Sget_simple_extent_ndims(sid);
            os << " rank=" << rank;
            if (rank > 0 && rank <= H5S_MAX_RANK) {
                std::vector<hsize_t> dims(rank);
                H5Sget_simple_extent_dims(sid, dims.data(), nullptr);
                os << " dims={";
                for (int i = 0; i < rank; ++i) os << dims[i] << (i+1<rank ? "," : "");
                os << "}";
            }
            H5Sclose(sid);
        }

        // Chunk dims + filter count when chunked.
        ::hid_t dcpl = H5Dget_create_plist(id);
        if (dcpl >= 0) {
            H5D_layout_t layout = H5Pget_layout(dcpl);
            static const char* lyt[] = {"COMPACT","CONTIGUOUS","CHUNKED","VIRTUAL"};
            if (layout >= 0 && layout < 4) os << " layout=" << lyt[layout];
            if (layout == H5D_CHUNKED) {
                int crank = H5Pget_chunk(dcpl, 0, nullptr);
                if (crank > 0) {
                    std::vector<hsize_t> cdims(crank);
                    H5Pget_chunk(dcpl, crank, cdims.data());
                    os << " chunk={";
                    for (int i = 0; i < crank; ++i) os << cdims[i] << (i+1<crank ? "," : "");
                    os << "}";
                }
            }
            int nfilters = H5Pget_nfilters(dcpl);
            if (nfilters >= 0) os << " filters=" << nfilters;
            H5Pclose(dcpl);
        }

        // Attribute count is intentionally omitted — H5Oget_info* has
        // a moving API surface across HDF5 1.12.x patch releases (info2/info3
        // gated by version-and-compat-macros mix), too brittle for a
        // pretty-printer. Use h5ls if you need it.

        os << " }";
        h5::unmute();
        return os;
    }

    // h5::gr_t — group path + child count.
    inline std::ostream& operator<<(std::ostream& os, const h5::gr_t& gr) {
        h5::mute();
        const ::hid_t id = static_cast<::hid_t>(gr);
        os << "h5::gr_t{ handle=" << id;
        if (id == H5I_UNINIT || H5Iis_valid(id) <= 0) { os << " valid=no }"; h5::unmute(); return os; }

        ssize_t name_len = H5Iget_name(id, nullptr, 0);
        std::string path(name_len > 0 ? static_cast<size_t>(name_len) : 0, '\0');
        if (name_len > 0) H5Iget_name(id, path.data(), name_len + 1);
        os << " path='" << path << "'";

        H5G_info_t gi{};
        if (H5Gget_info(id, &gi) >= 0) os << " children=" << gi.nlinks;
        // Attribute count omitted — see ds_t printer comment.

        os << " }";
        h5::unmute();
        return os;
    }

    // h5::at_t — attribute name + parent path + datatype class + dataspace rank.
    inline std::ostream& operator<<(std::ostream& os, const h5::at_t& at) {
        h5::mute();
        const ::hid_t id = static_cast<::hid_t>(at);
        os << "h5::at_t{ handle=" << id;
        if (id == H5I_UNINIT || H5Iis_valid(id) <= 0) { os << " valid=no }"; h5::unmute(); return os; }

        ssize_t nlen = H5Aget_name(id, 0, nullptr);
        std::string aname(nlen > 0 ? static_cast<size_t>(nlen) : 0, '\0');
        if (nlen > 0) H5Aget_name(id, nlen + 1, aname.data());
        os << " name='" << aname << "'";

        ::hid_t tid = H5Aget_type(id);
        if (tid >= 0) {
            static const char* tcls[] = {"INTEGER","FLOAT","TIME","STRING","BITFIELD","OPAQUE",
                                         "COMPOUND","REFERENCE","ENUM","VLEN","ARRAY"};
            H5T_class_t c = H5Tget_class(tid);
            if (c >= 0 && c < 11) os << " dtype=" << tcls[c];
            H5Tclose(tid);
        }
        ::hid_t sid = H5Aget_space(id);
        if (sid >= 0) {
            int rank = H5Sget_simple_extent_ndims(sid);
            os << " rank=" << rank;
            H5Sclose(sid);
        }

        os << " }";
        h5::unmute();
        return os;
    }

    // h5::dcpl_t — layout + chunk dims + alloc time + fill value defined + filter pipeline.
    inline std::ostream& operator<<(std::ostream& os, const h5::dcpl_t& dcpl) {
        h5::mute();
        const ::hid_t id = static_cast<::hid_t>(dcpl);
        os << "h5::dcpl_t{ handle=" << id;
        if (id == H5I_UNINIT || H5Iis_valid(id) <= 0) { os << " valid=no }"; h5::unmute(); return os; }

        H5D_layout_t layout = H5Pget_layout(id);
        static const char* lyt[] = {"COMPACT","CONTIGUOUS","CHUNKED","VIRTUAL"};
        if (layout >= 0 && layout < 4) os << " layout=" << lyt[layout];
        if (layout == H5D_CHUNKED) {
            int rank = H5Pget_chunk(id, 0, nullptr);
            if (rank > 0) {
                std::vector<hsize_t> cdims(rank);
                H5Pget_chunk(id, rank, cdims.data());
                os << " chunk={";
                for (int i = 0; i < rank; ++i) os << cdims[i] << (i+1<rank ? "," : "");
                os << "}";
            }
        }

        H5D_alloc_time_t at = H5D_ALLOC_TIME_DEFAULT;
        if (H5Pget_alloc_time(id, &at) >= 0) {
            static const char* a[] = {"DEFAULT","EARLY","LATE","INCR"};
            int idx = (at == H5D_ALLOC_TIME_DEFAULT) ? 0
                    : (at == H5D_ALLOC_TIME_EARLY)   ? 1
                    : (at == H5D_ALLOC_TIME_LATE)    ? 2
                    : (at == H5D_ALLOC_TIME_INCR)    ? 3 : 0;
            os << " alloc=" << a[idx];
        }

        H5D_fill_value_t fv = H5D_FILL_VALUE_UNDEFINED;
        if (H5Pfill_value_defined(id, &fv) >= 0)
            os << " fill=" << (fv == H5D_FILL_VALUE_DEFAULT ? "default"
                            :  fv == H5D_FILL_VALUE_USER_DEFINED ? "user" : "none");

        int nfilters = H5Pget_nfilters(id);
        if (nfilters >= 0) {
            os << " filters=[";
            for (int i = 0; i < nfilters; ++i) {
                unsigned flags = 0; size_t cd_nelem = 0; unsigned cd_vals[8]{}; char fname[64]{}; unsigned fcfg = 0;
                cd_nelem = 8;
                H5Z_filter_t f = H5Pget_filter2(id, static_cast<unsigned>(i),
                                                &flags, &cd_nelem, cd_vals,
                                                sizeof(fname)-1, fname, &fcfg);
                os << (i ? "," : "") << (fname[0] ? fname : "filter") << "(" << f << ")";
            }
            os << "]";
        }

        os << " }";
        h5::unmute();
        return os;
    }

    // h5::fapl_t — driver name + libver bounds + cache config + alignment.
    inline std::ostream& operator<<(std::ostream& os, const h5::fapl_t& fapl) {
        h5::mute();
        const ::hid_t id = static_cast<::hid_t>(fapl);
        os << "h5::fapl_t{ handle=" << id;
        if (id == H5I_UNINIT || H5Iis_valid(id) <= 0) { os << " valid=no }"; h5::unmute(); return os; }

        ::hid_t did = H5Pget_driver(id);
        if (did >= 0) os << " driver_id=" << did;

        H5F_libver_t low = H5F_LIBVER_EARLIEST, high = H5F_LIBVER_EARLIEST;
        if (H5Pget_libver_bounds(id, &low, &high) >= 0)
            os << " libver=[" << static_cast<int>(low) << "," << static_cast<int>(high) << "]";

        int    mdc_slots = 0; size_t rdcc_slots = 0; size_t rdcc_bytes = 0; double rdcc_w0 = 0.0;
        if (H5Pget_cache(id, &mdc_slots, &rdcc_slots, &rdcc_bytes, &rdcc_w0) >= 0)
            os << " cache={mdc=" << mdc_slots << " rdcc_slots=" << rdcc_slots
               << " rdcc_bytes=" << rdcc_bytes << " w0=" << rdcc_w0 << "}";

        hsize_t threshold = 0, alignment = 0;
        if (H5Pget_alignment(id, &threshold, &alignment) >= 0)
            os << " align={threshold=" << threshold << " align=" << alignment << "}";

        os << " }";
        h5::unmute();
        return os;
    }
}



