/*
 * Copyright (c) 2018-2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include "utils/abstract.hpp"
#include "plot/all"
#include <cstdlib>
#include <random>
#include <fmt/core.h>
#include <fmt/format.h>
#include "../h5cpp/H5meta.hpp"

namespace ns = h5::test;


TEST(string, element_type) {
    plot::color::fg palette{0x6b8e23, 0xff4500, 0xd8d8d8};
    std::vector<plot::data::point_t<unsigned>> data;
    std::vector<std::string> x_axis {"string literal","std::string","c","d"};
    std::vector<std::string> y_axis = {
        "fixed length",
        "variable length","compact layout","contiguous layout"};

    using namespace plot;
    heatmap("pix/string-representations.svg", data, palette,
		axis::x{x_axis, rotate::ccw{45}, position{10, 80} }, axis::y{y_axis, palette[1]},
		title{"String Types in HDF5", position{align::left, 15}},
		legend{
			text{"yes", "???", "n/a"},
			position{align::left, align::bottom}, palette, marker::rect
		});
}

/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

