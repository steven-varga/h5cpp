/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */


#include <armadillo>

#include "utils/abstract.hpp"
#include "plot/all"
#include <cstdlib>
#include <random>
#include <fmt/core.h>
#include <fmt/format.h>
#include "../h5cpp/H5meta.hpp"

namespace ns = h5::test;

template<class T>
using type_class = std::tuple< // when changing content, update the `y_axis`
	std::is_compound<T>, std::is_trivial<T>, std::is_standard_layout<T>, std::is_pod<T>, std::is_aggregate<T>, ns::separator_t<T>,
	h5::impl::is_scalar<T>, h5::impl::is_vector<T>, h5::impl::is_matrix<T>, ns::separator_t<T>,
    h5::meta::has_data<T>, h5::meta::has_iterator<T>, h5::meta::has_size<T>, h5::meta::has_value_type<T>,ns::separator_t<T>,
    h5::impl::is_stl<T>, h5::impl::is_array<T>, h5::impl::is_contiguous<T>
    >;
std::vector<std::string> y_axis = { // must match `type_class<T>`
	"std::is_compound","std::is_trivial","std::is_standard_layout", "std::is_pod", "std::is_aggregate", "",
	"is_scalar","is_vector","is_matrix", "",
    ".data()", ".begin()/.end()",".size()", "T::value_type", "", "is_stl", "is_array", "is_contiguous"
};

TEST(IntegralTest, plot) {
    using all_containers = std::tuple<int, int[10], int[3][4], std::byte, ns::pod_t, std::complex<float>, std::string, 
        std::vector<int>, std::array<std::array<int,3>,4>, std::vector<std::string>, arma::mat >;
    std::vector<std::string> x_axis = {"int", "int[N]", "int[N][M]","std::byte","ns::pod_t", "std::complex<float>", "std::string",  
        "std::vector<int>","std::array<std::array<int,N>,M>", "std::vector<std::string>", "arma::mat"};
    plot::color::fg palette{0x6b8e23, 0xff4500, 0xd8d8d8};
    std::vector<plot::data::point_t<unsigned>> data;

    h5::meta::static_for<all_containers>( [&]( auto y ){
        using Ty = typename std::tuple_element<y, all_containers>::type;
        h5::meta::static_for<type_class<Ty>>( [=, &data]( auto x ){
            using Tx = typename std::tuple_element<x, type_class<Ty>>::type;
            if( ns::is_separator<Tx>::value || ns::is_separator<Ty>::value )
                data.push_back({x,y, 2});
            else
                data.push_back({x,y, !Tx::value});
        });
    });

    //M := outcome,  x_axis := pretty names of containers, y_axis := elementary_types 
    using namespace plot;    heatmap("pix/type_map.svg", data, palette,
		axis::x{y_axis, rotate::ccw{45}, position{10, 80} }, axis::y{x_axis, palette[1]},
		title{"Compile Type Map", position{align::left, 15}},
		legend{
			text{"yes", "no", "n/a"},
			position{align::left, align::bottom}, palette, marker::rect
		});
}


/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

