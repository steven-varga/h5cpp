/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#pragma once
#include "H5capi.hpp"
#include "H5Pall.hpp"
#include <algorithm>
#include <deque>
#include <string>
#include <vector>
#include <stdexcept>

	namespace h5::impl {
		// Trait: any h5cpp file/group handle or raw hid_t.
		template<class H> using is_valid_loc =
			std::bool_constant<std::is_same_v<H, h5::fd_t> ||
			                   std::is_same_v<H, h5::gr_t> ||
			                   std::is_same_v<H, ::hid_t>>;

		// Callback for H5Literate (single level, used by h5::ls).
		inline static herr_t iterate_callback( ::hid_t gid, const char *name, const H5L_info_t *info, void *op_data){
			(void)gid; (void)info;
			// this must not throw error, CAPI has to clean up
		try {
			std::vector<std::string> *data =  static_cast<std::vector<std::string>* >(op_data);
			data->push_back( std::string(name) );
		} catch ( ... ){
			return -1;
		}
		return 0;
	}

	// Callback for H5Lvisit (recursive, used by h5::dfs / h5::bfs). HDF5 hands
	// the relative path from the visit root in `name`; we keep the path itself,
	// not just the leaf — that's what makes the result globally addressable.
	inline static herr_t visit_callback( ::hid_t /*gid*/, const char* name,
	                                      const H5L_info_t* /*info*/, void* op_data ) {
		try {
			auto* data = static_cast<std::vector<std::string>*>(op_data);
			data->emplace_back(name);
		} catch ( ... ) {
			return -1;
		}
		return 0;
	}
}

namespace h5 {
    // ── listing ─────────────────────────────────────────────────────────────

    // Single-level listing — immediate children of `directory` (datasets,
    // sub-groups, and links). For recursive traversal use h5::dfs / h5::bfs.
    inline std::vector<std::string> ls(const h5::fd_t& fd,  const std::string& directory ){
        hid_t group_id;
		H5CPP_CHECK_NZ(
				(group_id = H5Gopen( static_cast<hid_t>(fd), directory.c_str(), H5P_DEFAULT )), std::runtime_error,	 h5::error::msg::open_group );
        std::vector<std::string> files;
        H5CPP_CHECK_NZ( H5Literate( group_id, H5_INDEX_NAME, H5_ITER_INC, 0, &impl::iterate_callback, &files ), std::runtime_error,	h5::error::msg::list_directory );
		H5CPP_CHECK_NZ( H5Gclose(group_id), std::runtime_error,	 h5::error::msg::close_group);
        return files;
    }

    // Recursive listing — every object path under `directory` in
    // name-sorted depth-first order. Paths are relative to `directory`
    // (e.g. starting from "/sensors" you get "imu", "imu/left",
    // "imu/left/raw", "lidar", …). HDF5's H5Lvisit does the work.
    inline std::vector<std::string> dfs(const h5::fd_t& fd, const std::string& directory) {
        hid_t group_id;
        H5CPP_CHECK_NZ(
            (group_id = H5Gopen(static_cast<hid_t>(fd), directory.c_str(), H5P_DEFAULT)),
            std::runtime_error, h5::error::msg::open_group);
        std::vector<std::string> paths;
        H5CPP_CHECK_NZ(
            H5Lvisit(group_id, H5_INDEX_NAME, H5_ITER_INC, &impl::visit_callback, &paths),
            std::runtime_error, h5::error::msg::list_directory);
        H5CPP_CHECK_NZ(H5Gclose(group_id),
            std::runtime_error, h5::error::msg::close_group);
        return paths;
    }

    // Recursive listing in breadth-first order — all paths at depth 1 before
    // any path at depth 2, etc. Implementation: collect via H5Lvisit (DFS),
    // then stable-sort by slash count. Same set as dfs(), just shuffled into
    // BFS order — useful when you want to truncate output by depth or process
    // top-level subgroups first.
    inline std::vector<std::string> bfs(const h5::fd_t& fd, const std::string& directory) {
        auto paths = dfs(fd, directory);
        std::stable_sort(paths.begin(), paths.end(),
            [](const std::string& a, const std::string& b) {
                return std::count(a.begin(), a.end(), '/')
                     < std::count(b.begin(), b.end(), '/');
            });
        return paths;
    }

    // ── existence + identity ────────────────────────────────────────────────

    // True if `path` resolves to an object or link under `loc`. Cheap wrapper
    // around H5Lexists; returns false on missing path (not throws).
    template <class Loc>
    inline std::enable_if_t<impl::is_valid_loc<Loc>::value, bool>
    exists(const Loc& loc, const std::string& path) {
        return H5Lexists(static_cast<hid_t>(loc), path.c_str(), H5P_DEFAULT) > 0;
    }

    // ── link creation ───────────────────────────────────────────────────────

    // Soft link (POSIX symlink semantics): stores `target_path` as a string;
    // resolved lazily on open. Dangling links are allowed at creation.
    template <class Loc>
    inline std::enable_if_t<impl::is_valid_loc<Loc>::value, void>
    link_soft(const std::string& target_path,
              const Loc& link_loc, const std::string& link_name,
              const h5::lcpl_t& lcpl = h5::default_lcpl) {
        H5CPP_CHECK_NZ(
            H5Lcreate_soft(target_path.c_str(),
                           static_cast<hid_t>(link_loc), link_name.c_str(),
                           static_cast<hid_t>(lcpl), H5P_DEFAULT),
            std::runtime_error, "couldn't create soft link");
    }

    // Hard link: a second name for the same object in the same file. Both
    // names must reach the same HDF5 file via their location handles.
    template <class L1, class L2>
    inline std::enable_if_t<impl::is_valid_loc<L1>::value && impl::is_valid_loc<L2>::value, void>
    link_hard(const L1& target_loc, const std::string& target_path,
              const L2& link_loc,   const std::string& link_name,
              const h5::lcpl_t& lcpl = h5::default_lcpl) {
        H5CPP_CHECK_NZ(
            H5Lcreate_hard(static_cast<hid_t>(target_loc), target_path.c_str(),
                           static_cast<hid_t>(link_loc),   link_name.c_str(),
                           static_cast<hid_t>(lcpl), H5P_DEFAULT),
            std::runtime_error, "couldn't create hard link");
    }

    // External link: like soft, but the target lives in another HDF5 file.
    // Neither the file nor the target path needs to exist at creation time.
    template <class Loc>
    inline std::enable_if_t<impl::is_valid_loc<Loc>::value, void>
    link_external(const std::string& target_file, const std::string& target_path,
                  const Loc& link_loc, const std::string& link_name,
                  const h5::lcpl_t& lcpl = h5::default_lcpl) {
        H5CPP_CHECK_NZ(
            H5Lcreate_external(target_file.c_str(), target_path.c_str(),
                               static_cast<hid_t>(link_loc), link_name.c_str(),
                               static_cast<hid_t>(lcpl), H5P_DEFAULT),
            std::runtime_error, "couldn't create external link");
    }

    // ── move / copy / unlink ────────────────────────────────────────────────

    // Move (rename) an object. If src_loc == dst_loc, this is a rename.
    // For cross-file moves use export/import (HDF5 doesn't support move
    // across files directly).
    template <class L1, class L2>
    inline std::enable_if_t<impl::is_valid_loc<L1>::value && impl::is_valid_loc<L2>::value, void>
    move(const L1& src_loc, const std::string& src_name,
         const L2& dst_loc, const std::string& dst_name,
         const h5::lcpl_t& lcpl = h5::default_lcpl) {
        H5CPP_CHECK_NZ(
            H5Lmove(static_cast<hid_t>(src_loc), src_name.c_str(),
                    static_cast<hid_t>(dst_loc), dst_name.c_str(),
                    static_cast<hid_t>(lcpl), H5P_DEFAULT),
            std::runtime_error, "couldn't move object");
    }

    // Copy an object (group, dataset, named datatype) including attributes
    // and group hierarchy. Source and destination locations may be in the
    // same or different files. Uses H5Ocopy with default options.
    template <class L1, class L2>
    inline std::enable_if_t<impl::is_valid_loc<L1>::value && impl::is_valid_loc<L2>::value, void>
    copy(const L1& src_loc, const std::string& src_name,
         const L2& dst_loc, const std::string& dst_name) {
        H5CPP_CHECK_NZ(
            H5Ocopy(static_cast<hid_t>(src_loc), src_name.c_str(),
                    static_cast<hid_t>(dst_loc), dst_name.c_str(),
                    H5P_DEFAULT, H5P_DEFAULT),
            std::runtime_error, "couldn't copy object");
    }

    // Delete a link. The referenced object is reclaimed when its reference
    // count drops to zero — hard-linked objects survive until every name is
    // unlinked.
    template <class Loc>
    inline std::enable_if_t<impl::is_valid_loc<Loc>::value, void>
    unlink(const Loc& loc, const std::string& path) {
        H5CPP_CHECK_NZ(
            H5Ldelete(static_cast<hid_t>(loc), path.c_str(), H5P_DEFAULT),
            std::runtime_error, "couldn't delete link");
    }
}
