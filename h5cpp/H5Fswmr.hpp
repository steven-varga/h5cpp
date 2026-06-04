/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * SWMR (Single-Writer/Multiple-Reader) — the whole feature in one header:
 * feature gate, dispatch tags, filesystem-class check, and the
 * create / flush / refresh / start_swmr_write entry points.
 */
#pragma once

#ifndef __linux__
#  error "h5cpp SWMR is Linux-only in this release"
#endif

#include <hdf5.h>

// Available only when the HDF5 build exposes the SWMR access flags.
#if defined(H5F_ACC_SWMR_WRITE) && defined(H5F_ACC_SWMR_READ)
#  define H5CPP_HAS_SWMR 1
#endif

#ifdef H5CPP_HAS_SWMR

#include "H5Eall.hpp"
#include "H5Iall.hpp"
#include "H5Fcreate.hpp"   // h5::create(raw flags) — the swmr_write factory delegates to it

#include <sys/vfs.h>       // statfs
#include <cstdlib>         // getenv
#include <string>
#include <iostream>

namespace h5 {
	// dispatch tags for the SWMR writer/reader create path
	struct swmr_write_t final {};
	struct swmr_read_t  final {};
	const static h5::swmr_write_t swmr_write{};
	const static h5::swmr_read_t  swmr_read{};
}

// ── filesystem-class detection ───────────────────────────────────────────────
// SWMR relies on POSIX page-cache coherence between writer and readers on one
// host; network / layered / parallel filesystems break it. Policy: WARN at open,
// never refuse. Opt out with H5CPP_SWMR_NO_FS_CHECK=1.
namespace h5::impl::swmr {

	// statfs f_type magics; the ones absent from <linux/magic.h> are vendored.
	enum : unsigned long {
		h5_EXT4_MAGIC      = 0xEF53,
		h5_XFS_MAGIC       = 0x58465342,
		h5_BTRFS_MAGIC     = 0x9123683E,
		h5_TMPFS_MAGIC     = 0x01021994,
		h5_F2FS_MAGIC      = 0xF2F52010,
		h5_NFS_MAGIC       = 0x6969,
		h5_SMB_MAGIC       = 0x517B,
		h5_CIFS_MAGIC      = 0xFF534D42,
		h5_OVERLAYFS_MAGIC = 0x794C7630,
		h5_LUSTRE_MAGIC    = 0x0BD00BD0,
		h5_GPFS_MAGIC      = 0x47504653,
		h5_BEEGFS_MAGIC    = 0x19830326,
		h5_FUSE_MAGIC      = 0x65735546
	};

	enum class fs_verdict { local_ok, network_bad, layered_caution, parallel_caution, unknown };
	struct fs_info { fs_verdict verdict = fs_verdict::unknown; const char* name = "unknown"; };

	inline fs_info classify( unsigned long f_type ) {
		switch( f_type ) {
			case h5_EXT4_MAGIC:      return { fs_verdict::local_ok,         "ext4" };
			case h5_XFS_MAGIC:       return { fs_verdict::local_ok,         "xfs" };
			case h5_BTRFS_MAGIC:     return { fs_verdict::local_ok,         "btrfs" };
			case h5_TMPFS_MAGIC:     return { fs_verdict::local_ok,         "tmpfs" };
			case h5_F2FS_MAGIC:      return { fs_verdict::local_ok,         "f2fs" };
			case h5_NFS_MAGIC:       return { fs_verdict::network_bad,      "nfs" };
			case h5_SMB_MAGIC:       return { fs_verdict::network_bad,      "smb" };
			case h5_CIFS_MAGIC:      return { fs_verdict::network_bad,      "cifs" };
			case h5_OVERLAYFS_MAGIC: return { fs_verdict::layered_caution,  "overlayfs" };
			case h5_LUSTRE_MAGIC:    return { fs_verdict::parallel_caution, "lustre" };
			case h5_GPFS_MAGIC:      return { fs_verdict::parallel_caution, "gpfs" };
			case h5_BEEGFS_MAGIC:    return { fs_verdict::parallel_caution, "beegfs" };
			case h5_FUSE_MAGIC:      return { fs_verdict::unknown,          "fuse" };
			default:                 return { fs_verdict::unknown,          "unknown" };
		}
	}

	// Classify the filesystem backing `path`; warn (once, to cerr) when SWMR
	// coherence is at risk. Never throws, never refuses. Returns the verdict.
	inline fs_verdict check_filesystem( const std::string& path ) noexcept {
		if( const char* off = std::getenv("H5CPP_SWMR_NO_FS_CHECK");
				off != nullptr && off[0] != '\0' && off[0] != '0' )
			return fs_verdict::unknown;

		struct statfs sfs;
		if( ::statfs( path.c_str(), &sfs ) != 0 )
			return fs_verdict::unknown;

		const fs_info info = classify( static_cast<unsigned long>( sfs.f_type ) );
		switch( info.verdict ) {
			case fs_verdict::network_bad:
				std::cerr << "h5cpp SWMR WARNING: '" << path << "' is on " << info.name
				          << " — network filesystems break reader cache coherence; "
				             "SWMR visibility is NOT guaranteed (issue #267).\n";
				break;
			case fs_verdict::layered_caution:
				std::cerr << "h5cpp SWMR WARNING: '" << path << "' is on " << info.name
				          << " — layered/overlay filesystems may hide writer flushes "
				             "from readers in other namespaces (issue #267).\n";
				break;
			case fs_verdict::parallel_caution:
				std::cerr << "h5cpp SWMR WARNING: '" << path << "' is on " << info.name
				          << " — parallel filesystems require correct mount options for "
				             "SWMR coherence; not certified (issue #267).\n";
				break;
			case fs_verdict::unknown:
			case fs_verdict::local_ok:
				break;
		}
		return info.verdict;
	}
}

namespace h5 {

	// h5::create(path, h5::swmr_write, h5::latest_version) — creates a NORMAL
	// latest-format RDWR file. SWMR can't be enabled at create (no datasets yet);
	// call h5::start_swmr_write(fd) once the datasets exist. Requires latest_version.
	template<class... args_t>
	inline h5::fd_t create( const std::string& path, h5::swmr_write_t, args_t&&... args ) {
		using tlibver = typename arg::tpos<const h5::libver_bounds&, const args_t&...>;
		static_assert( tlibver::present,
			"h5::create(..., h5::swmr_write, ...) requires h5::latest_version — "
			"SWMR needs (LATEST,LATEST) library-version bounds." );
		const auto slash = path.find_last_of('/');
		const std::string dir = (slash == std::string::npos) ? std::string(".")
		                                                      : path.substr(0, slash + 1);
		h5::impl::swmr::check_filesystem( dir );
		const h5::libver_bounds& libver = arg::get( h5::latest_version, args... );
		h5::fapl_t fapl = static_cast<h5::fapl_t>( libver );
		return h5::create( path, H5F_ACC_TRUNC, h5::default_fcpl, fapl );
	}

	// h5::flush(ds) — writer-side SWMR metadata flush (H5Dflush). After it returns,
	// the appended data is visible to readers that subsequently h5::refresh(ds).
	inline void flush( const h5::ds_t& ds ) {
		H5CPP_CHECK_NZ( H5Dflush( static_cast<::hid_t>( ds ) ),
				h5::error::io::dataset::write,
				"h5::flush: H5Dflush failed — is the file opened SWMR-write?" );
	}

	// h5::refresh(ds) — reader-side metadata reload (H5Drefresh) so the reader sees
	// the writer's latest flushed state. Poll cadence is application policy.
	inline void refresh( const h5::ds_t& ds ) {
		H5CPP_CHECK_NZ( H5Drefresh( static_cast<::hid_t>( ds ) ),
				h5::error::io::dataset::read,
				"h5::refresh: H5Drefresh failed — is the file opened SWMR-read?" );
	}

	// h5::start_swmr_write(fd) — transition an open RDWR (latest-bounds) file into
	// SWMR-write once its datasets exist. Idempotent: a no-op when the file is
	// already SWMR-write (e.g. opened that way on a warm restart).
	inline void start_swmr_write( const h5::fd_t& fd ) {
		unsigned intent = 0;
		if( H5Fget_intent( static_cast<::hid_t>( fd ), &intent ) >= 0
				&& (intent & H5F_ACC_SWMR_WRITE) )
			return;
		H5CPP_CHECK_NZ( H5Fstart_swmr_write( static_cast<::hid_t>( fd ) ),
				h5::error::io::file::write,
				"h5::start_swmr_write: H5Fstart_swmr_write failed — file must be "
				"opened RDWR with latest_version bounds and all observed datasets "
				"must already exist." );
	}
}

#endif // H5CPP_HAS_SWMR
