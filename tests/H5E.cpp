/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#define GOOGLE_STRIP_LOG 1
#include "utils/main.hpp"
#include "plot/all"
#include <cstdlib>
#include <random>

TEST_F(CartesianTest, cartesian_example) {

	arma::Mat<unsigned> M(nx, ny);
	plot::color::fg color{0x08000, 0x008080, 0x000000};
	test_case(
	[&](size_t i, size_t j, const std::string& lbl_x, const std::string& lbl_y) {
		M(i,j)= 0.1;
	});
	{ //M := outcome,  x_axis := pretty names of containers, y_axis := elementary_types 
	using namespace plot;
	heatmap("pix/typemap.svg", M, axis::x{x_axis}, axis::y{y_axis}, rotate::ccw{23},
			title{"this graph", position{align::center, align::top}, font{"Ubuntu Mono", "bold", 20},  rotate::ccw{45}},
		   	height{200}, width{300}, margin{5,5,5,5},
			footnote{"footnote", position{200,500}, color::fg{0xffaabb}, color::bg{0x0}, rotate::cw{35}, align::center},
			legend{
				text{"success","failure","not executed"},
				color, //color::fg{0xff0011, 0xbbaacc, 0xaabbcc},
			   	position{100,10}
			});
	}
}

int main( int argc, char**  argv ) {
	H5CPP_INIT_RUNNER(argc, argv);
	return RUN_ALL_TESTS();
}

-DHDF5_BUILD_CPP_LIB=OFF -DHDF5_ENABLE_DIRECT_VFD=ON -DHDF5_ENABLE_ROS3_VFD=ON
./configure --prefix=/usr/local --enable-direct-vfd  --enable-ros3-vfd --enable-build-mode=production
