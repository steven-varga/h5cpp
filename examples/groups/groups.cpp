// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// =============================================================================
// h5cpp groups tutorial
// =============================================================================
//
// A group is HDF5's directory: a container for datasets, sub-groups, links, and
// attributes. The h5cpp surface is small:
//
//   h5::gcreate(parent, path, ...)        create a group
//   h5::gopen(parent, path)                open an existing group
//   h5::ls / h5::dfs / h5::bfs             list (level / DFS / BFS)
//   h5::exists(loc, path)                  test existence
//   h5::link_soft / link_hard / link_external   create links
//   h5::move(src_loc, src, dst_loc, dst)   rename or move
//   h5::copy(src_loc, src, dst_loc, dst)   deep copy
//   h5::unlink(loc, path)                  delete a link
//   h5::awrite(parent, name, value)        attach an attribute
//
// Nine sections, one concept per section:
//
//   1. Creating groups (file as parent, with intermediate paths)
//   2. Nested groups (group as parent)
//   3. Open + existence check (h5::exists)
//   4. Group attributes
//   5. Listing group contents (ls / dfs / bfs)
//   6. Links — soft, hard, external (h5::link_*)
//   7. Move / copy / unlink (h5::move, h5::copy, h5::unlink)
//   8. Error handling — specific vs umbrella exceptions

#include <h5cpp/all>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

void section(const char* title) {
    std::cout << "\n" << title << "\n"
              << std::string(std::strlen(title), '-') << "\n";
}

} // namespace

int main() {
    auto fd = h5::create("groups.h5", H5F_ACC_TRUNC);

    // ── 1. creating groups — file as parent, with intermediate paths ────────
    // h5::gcreate(fd, "/a/b/c") with a stock lcpl is an error if /a or /a/b
    // doesn't exist. Either compose an lcpl with h5::create_intermediate_group
    // explicitly, or use h5::default_lcpl which has it set.
    section("1. creating groups");
    {
        // Implicit: h5::default_lcpl already has create_intermediate_group set.
        h5::gr_t a = h5::gcreate(fd, "/sensors/imu/left");

        // Explicit: build the lcpl by hand.
        h5::lcpl_t lcpl = h5::char_encoding{H5T_CSET_UTF8}
                        | h5::create_intermediate_group{1};
        h5::gr_t b = h5::gcreate(fd, "/sensors/lidar", lcpl);

        std::cout << "  created /sensors/imu/left and /sensors/lidar\n";
    }

    // ── 2. nested groups — group as parent ──────────────────────────────────
    // gr_t is a valid parent for gcreate too. Use a relative path to add
    // children under it. This is how you build hierarchies incrementally
    // without rewriting the absolute prefix.
    section("2. nested groups (group as parent)");
    {
        h5::gr_t imu_left = h5::gopen(fd, "/sensors/imu/left");
        h5::gcreate(imu_left, "calibration");   // → /sensors/imu/left/calibration
        h5::gcreate(imu_left, "raw");           // → /sensors/imu/left/raw

        std::cout << "  added /sensors/imu/left/{calibration, raw}\n";
    }

    // ── 3. open + existence check ───────────────────────────────────────────
    // h5::exists(loc, path) returns bool (no exception on missing). Pattern:
    // check before opening to avoid exceptions in normal control flow.
    section("3. open + existence check");
    {
        for (const auto& path : {"/sensors/imu/left",
                                 "/sensors/imu/right",
                                 "/sensors/lidar"}) {
            bool present = h5::exists(fd, path);
            std::cout << "  " << path
                      << "  -> " << (present ? "exists" : "missing") << "\n";
            if (present) {
                h5::gr_t gr = h5::gopen(fd, path);
                (void)gr;   // RAII closes on scope exit
            }
        }
    }

    // ── 4. group attributes ─────────────────────────────────────────────────
    // h5::awrite(parent, name, value) accepts scalars, strings, vectors, and
    // initializer lists. Works on any handle that can carry attributes
    // (groups, datasets, named datatypes). The bracket-assignment shorthand
    // gr["name"] = value is a ds_t-only convenience — use awrite directly
    // for gr_t.
    section("4. group attributes");
    {
        h5::gr_t gr = h5::gopen(fd, "/sensors/imu/left");

        h5::awrite(gr, "temperature_C", 42.0);
        h5::awrite(gr, "unit",          std::string("celsius"));
        h5::awrite(gr, "channels",      std::vector<int>{0, 1, 2, 3, 4, 5});
        h5::awrite(gr, "axes",
            std::initializer_list<const char*>{"x", "y", "z"});

        std::cout << "  attached 4 attributes to /sensors/imu/left\n";
    }

    // ── 5. listing group contents ───────────────────────────────────────────
    // Three traversal flavours:
    //   h5::ls(fd, path)   — immediate children only
    //   h5::dfs(fd, path)  — recursive, depth-first, name-sorted
    //   h5::bfs(fd, path)  — recursive, breadth-first
    // All three return std::vector<std::string>, streamable via the pretty-printer.
    section("5. listing group contents");
    {
        std::cout << "  ls(/)              -> " << h5::ls(fd, "/")            << "\n";
        std::cout << "  ls(/sensors)       -> " << h5::ls(fd, "/sensors")     << "\n";
        std::cout << "  ls(/sensors/imu)   -> " << h5::ls(fd, "/sensors/imu") << "\n";

        std::cout << "\n  dfs(/sensors) -> " << h5::dfs(fd, "/sensors") << "\n";
        std::cout <<   "  bfs(/sensors) -> " << h5::bfs(fd, "/sensors") << "\n";
    }

    // ── 6. links — soft, hard, external ─────────────────────────────────────
    // HDF5 has three link flavours. h5cpp doesn't wrap H5Lcreate_* yet — use
    // the raw C API. Every h5cpp handle casts to hid_t for interop.
    //
    //   soft     — POSIX-symlink semantics: stores a path string, resolved
    //              lazily on open. Dangling soft links are allowed.
    //   hard     — Additional name pointing at the same object. Both names
    //              must point to objects in the same file.
    //   external — Like soft but the target is in another HDF5 file.
    section("6. links — soft, hard, external");
    {
        // soft link: /shortcuts/imu_l -> /sensors/imu/left
        // (default_lcpl auto-creates the /shortcuts parent group)
        h5::link_soft("/sensors/imu/left", fd, "/shortcuts/imu_l");

        // hard link: /shortcuts/imu_l_alias is a second name for the same group
        h5::link_hard(fd, "/sensors/imu/left",
                      fd, "/shortcuts/imu_l_alias");

        // external link: /external/peer -> /sensors/imu/left in external.h5
        // (target file doesn't have to exist at link-creation time)
        h5::link_external("external.h5", "/sensors/imu/left",
                          fd, "/external/peer");

        std::cout << "  /shortcuts -> " << h5::ls(fd, "/shortcuts") << "\n";
        std::cout << "  /external  -> " << h5::ls(fd, "/external")  << "\n";

        // Soft link is followed transparently — opening the link opens the target.
        if (h5::exists(fd, "/shortcuts/imu_l")) {
            h5::gr_t followed = h5::gopen(fd, "/shortcuts/imu_l");
            std::cout << "  opened via soft link: /shortcuts/imu_l "
                      << "(target = /sensors/imu/left)\n";
            (void)followed;
        }
    }

    // ── 7. move / copy / unlink ─────────────────────────────────────────────
    // h5::move renames or relocates an object. h5::copy duplicates an object
    // (with its attributes and any nested groups) to a new path — same file
    // or another file. h5::unlink removes a link; the target object is
    // reclaimed when its last name is unlinked.
    section("7. move / copy / unlink");
    {
        // Pre-create something to play with.
        h5::gcreate(fd, "/tmp/sketch");
        h5::awrite(h5::gopen(fd, "/tmp/sketch"), "purpose",
                   std::string("staging area"));

        // Move (rename within the file): /tmp/sketch -> /archive/v1
        h5::move(fd, "/tmp/sketch", fd, "/archive/v1");
        std::cout << "  after move:   /tmp     -> " << h5::ls(fd, "/tmp")     << "\n";
        std::cout << "                /archive -> " << h5::ls(fd, "/archive") << "\n";

        // Copy: clone /sensors/imu/left into /archive/imu_snapshot. Attributes,
        // sub-groups, everything underneath comes along.
        h5::copy(fd, "/sensors/imu/left", fd, "/archive/imu_snapshot");
        std::cout << "  after copy:   /archive -> " << h5::ls(fd, "/archive") << "\n";
        std::cout << "                /archive/imu_snapshot -> "
                  << h5::ls(fd, "/archive/imu_snapshot") << "\n";

        // Unlink: drop /shortcuts/imu_l_alias. The target group /sensors/imu/left
        // survives because it still has its own name (and the soft link).
        h5::unlink(fd, "/shortcuts/imu_l_alias");
        std::cout << "  after unlink: /shortcuts -> " << h5::ls(fd, "/shortcuts") << "\n";
        std::cout << "                /sensors/imu/left still exists: "
                  << (h5::exists(fd, "/sensors/imu/left") ? "yes" : "no") << "\n";
    }

    // ── 8. error handling — specific vs umbrella exceptions ────────────────
    // h5cpp's exception hierarchy lets you catch precisely (per-API-call class)
    // or broadly (h5::error::any base). Mute HDF5's CAPI diagnostic stack when
    // intentionally triggering errors so the noise doesn't muddy your logs.
    section("8. error handling");
    {
        h5::mute();   // suppress CAPI error stack for the deliberate failures

        try {                                  // specific catch
            h5::gcreate(fd, "/sensors/imu/left");   // already exists
        } catch (const h5::error::io::group::create& e) {
            std::cout << "  specific:  " << e.what() << "\n";
        }

        try {                                  // umbrella catch
            h5::gopen(fd, "/does/not/exist");
        } catch (const h5::error::any& e) {
            std::cout << "  umbrella:  " << e.what() << "\n";
        }

        h5::unmute();
    }

    std::cout << "\nWrote everything to ./groups.h5\n";
    std::cout << "Inspect with:  h5dump -n groups.h5  (full tree)\n";
    return 0;
}
