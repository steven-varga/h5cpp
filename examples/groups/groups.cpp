/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <h5cpp/all>
#include <cstddef>


int main() {

	auto fd = h5::create("001.h5", H5F_ACC_TRUNC);
	{ 	// create intermediate groups
		// NOTICE: const static h5::lcpl_t lcpl = h5::char_encoding{H5T_CSET_UTF8} | h5::create_intermediate_group{1};
		// the default h5::default_lcpl will create immediate path and the encoding is set to utf8
		h5::gr_t gr = h5::gcreate(fd, "my-group/sub/path");

		h5::lcpl_t lcpl = h5::create_intermediate_group{1};
		h5::gcreate(fd, "/mygroup", lcpl); // passing lcpl type explicitly
	}
	{ // exceptions
		h5::mute(); // mute CAPI error handler
		try { // this group already exists, will throw `h5::error::io::group::create` exception
			h5::gcreate(fd, "/mygroup", h5::dont_create_path);
		}catch (  const h5::error::io::group::create& e){
			std::cout << e.what() <<"\n";
		}

		// catching all exceptions 
		try { // this group already exists, will throw `h5::error::io::group::create` exception
			h5::gcreate(fd, "/mygroup", h5::dont_create_path);
		}catch (  const h5::error::any& e){
			std::cout << e.what() <<"\n";
		}
		h5::unmute(); // re-enable CAPI error handling
	}
	{ // opening a group and adding attributes
		auto gr = h5::gopen(fd, "/mygroup");
		std::initializer_list list = {1,2,3,4,5};
		h5::awrite(gr, std::make_tuple(
			"temperature", 42.0,
			"unit", "C",
			"vector of ints", std::vector<int>({1,2,3,4,5}),
			"initializer list", list,
			"strings", std::initializer_list({"first", "second", "third","..."})
		));
	}
}

