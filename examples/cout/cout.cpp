/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <iostream>
#include <string>
#include <vector>

#include <h5cpp/all>

// =========================================================================
// h5cpp inspection-print demo — `operator<<` overloads in H5cout.hpp.
//
// Three families of operator<< overloads ship with h5cpp:
//
//   H5cout.hpp                IO-debug printers:
//                               operator<<(h5::dxpl_t)              dxpl handle (+ MPI mode if parallel)
//                               operator<<(h5::impl::array<T>)      offset_t / count_t / stride_t / block_t / current_dims_t / max_dims_t
//                               operator<<(h5::sp_t)                rank, current/max dims, selection bounds, blocks
//
//   H5Uall.hpp                STL pretty-printer:
//                               operator<<(std::vector / list / set / map / array / deque / forward_list / pair / tuple / stack / queue)
//                               recursive, truncates at H5CPP_CONSOLE_WIDTH
//
// This example shows the H5cout side: how to drop a `std::cout << x` into
// any IO call site for instant visibility into property lists, dataspaces,
// and hyperslab arguments — the values you'd otherwise have to chase
// through h5dump or a debugger.
// =========================================================================

int main() {
    h5::fd_t fd = h5::create("cout.h5", H5F_ACC_TRUNC);

    // ---------------------------------------------------------------------
    // 1. The array-shaped argument types (offset_t / count_t / stride_t /
    //    block_t / current_dims_t / max_dims_t) all stream as "{a,b,c,...}".
    //    Rank-0 prints "{n/a}". Dimensions equal to hsize_t-max print "inf".
    // ---------------------------------------------------------------------
    {
        std::cout << "[1] argument-type printers\n";
        std::cout << "  h5::offset{2,3}        = " << h5::offset{2, 3}        << "\n";
        std::cout << "  h5::count{10,20}       = " << h5::count{10, 20}       << "\n";
        std::cout << "  h5::stride{1,2}        = " << h5::stride{1, 2}        << "\n";
        std::cout << "  h5::block{4,4}         = " << h5::block{4, 4}         << "\n";
        std::cout << "  h5::current_dims{50,60}= " << h5::current_dims{50, 60}<< "\n";
        std::cout << "  h5::max_dims{H5S_UNLIMITED,40} = "
                  << h5::max_dims{H5S_UNLIMITED, 40} << "  (note 'inf' for unlimited)\n";
    }

    // ---------------------------------------------------------------------
    // 2. h5::sp_t — the dataspace object. The streamer dumps rank, current
    //    and max dimensions, selection bounds, validity, and (if a
    //    hyperslab is selected) every selected block in [start..end] form.
    //    Useful for debugging partial-IO selections live.
    // ---------------------------------------------------------------------
    {
        std::cout << "\n[2] dataspace printer\n";

        // Create a 2-D 10x20 dataset; print its dataspace.
        std::vector<double> data(200, 1.0);
        h5::write(fd, "/grid", data.data(),
                  h5::current_dims{10, 20}, h5::count{10, 20});
        h5::ds_t ds = h5::open(fd, "/grid");
        h5::sp_t space = h5::get_space(ds);

        // Default selection on a fresh space is "all".
        std::cout << "  fresh dataspace (all selected):\n" << space;

        // Narrow the selection to a 4x4 hyperslab at offset (2,3).
        h5::select_hyperslab(space, h5::offset{2, 3}, h5::count{4, 4});
        std::cout << "\n  after select_hyperslab(offset{2,3}, count{4,4}):\n"
                  << space;
    }

    // ---------------------------------------------------------------------
    // 3. h5::dxpl_t — the data-transfer property list. Default-printed it
    //    shows its handle id. Under H5_HAVE_PARALLEL the streamer also
    //    decodes the actual MPI IO mode after a collective transfer (chunk
    //    collective vs independent vs contiguous-collective vs mixed).
    // ---------------------------------------------------------------------
    {
        std::cout << "\n[3] dxpl printer\n";
        std::cout << "  default dxpl    : " << h5::default_dxpl << "\n";
    }

    // ---------------------------------------------------------------------
    // 4. Handle printers — six high-information specializations from
    //    H5cout.hpp. The other 13 handle types fall back to the generic
    //    template (handle id + validity + refcount).
    // ---------------------------------------------------------------------
    {
        std::cout << "\n[4] handle printers\n";

        // Use the live fd from earlier in this main().
        std::cout << "  fd_t   : " << fd << "\n";

        // Dataset printer — path, dtype, dims, layout, filters, attribute count.
        h5::ds_t ds = h5::open(fd, "/grid");
        std::cout << "  ds_t   : " << ds << "\n";

        // Group printer — create a group, populate via paths under it.
        h5::gr_t gr = h5::gcreate(fd, "/sensors");
        h5::write(fd, "/sensors/x", std::vector<double>{1.0, 2.0, 3.0});
        h5::write(fd, "/sensors/y", std::vector<double>{4.0, 5.0, 6.0});
        h5::awrite(gr, "units", std::string("meters"));
        std::cout << "  gr_t   : " << gr << "\n";

        // Attribute printer — open one of the awrite'd attributes.
        h5::at_t at = h5::open(gr, "units", h5::default_acpl);
        std::cout << "  at_t   : " << at << "\n";

        // dcpl printer — build one with chunk + gzip + alloc-time INCR.
        h5::dcpl_t dcpl{H5Pcreate(H5P_DATASET_CREATE)};
        hsize_t cdims[2] = {4, 8};
        H5Pset_chunk(static_cast<hid_t>(dcpl), 2, cdims);
        H5Pset_deflate(static_cast<hid_t>(dcpl), 6);
        H5Pset_alloc_time(static_cast<hid_t>(dcpl), H5D_ALLOC_TIME_INCR);
        std::cout << "  dcpl_t : " << dcpl << "\n";

        // fapl printer — driver, libver bounds, cache, alignment.
        h5::fapl_t fapl{H5Pcreate(H5P_FILE_ACCESS)};
        H5Pset_libver_bounds(static_cast<hid_t>(fapl),
                             H5F_LIBVER_V18, H5F_LIBVER_V112);
        std::cout << "  fapl_t : " << fapl << "\n";
    }

    // ---------------------------------------------------------------------
    // 5. STL types via the H5Uall.hpp pretty-printer (recursive, truncates
    //    at H5CPP_CONSOLE_WIDTH). The same idiom works for any iterable
    //    container; this is the one you've already seen in pprint / string.
    // ---------------------------------------------------------------------
    {
        std::cout << "\n[5] STL pretty-printer (H5Uall.hpp)\n";
        std::vector<std::string> tags = {"alpha", "beta", "gamma"};
        std::vector<std::vector<int>> ragged = {{1, 2}, {3, 4, 5}, {6}};
        std::cout << "  vector<string>      : " << tags   << "\n";
        std::cout << "  vector<vector<int>> : " << ragged << "\n";
    }

    std::cout << "\ncout.h5 written; structure visible via `h5ls -r cout.h5` or `h5dump cout.h5`.\n";
    return 0;
}
