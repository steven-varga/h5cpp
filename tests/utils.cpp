/*
 * Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this  software  and associated documentation files (the "Software"), to deal in
 * the Software  without   restriction, including without limitation the rights to
 * use, copy, modify, merge,  publish,  distribute, sublicense, and/or sell copies
 * of the Software, and to  permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE  SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT  SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY  CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER
 * IN  AN  ACTION  OF  CONTRACT, TORT OR  OTHERWISE, ARISING  FROM,  OUT  OF  OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <gtest/gtest.h>
#include <h5cpp/utils.hpp>
#include "event_listener.hpp"
#include "abstract.h"

TEST(split_path, utils) {
	using namespace h5::utils;

	{
		auto s = split_path("dataset");
		std::cout << "["<< s.first << "][" << s.second <<"]\n";
	}

	{
		auto s = split_path("/dataset");
		std::cout << "["<< s.first << "][" << s.second <<"]\n";
	}
	{
		auto s = split_path("/full/path/to/dataset");
		std::cout << "["<< s.first << "][" << s.second <<"]\n";
	}
	{
		auto s = split_path("full/path/to/dataset.ext");
		std::cout << "["<< s.first << "][" << s.second <<"]\n";
	}


}


int main( int argc, char**  argv ) {

	testing::InitGoogleTest(&argc, argv);
	testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
	delete listeners.Release(listeners.default_result_printer());
  	listeners.Append(new MinimalistPrinter( argv[0] ) );
	/* will create/read/write/append to datasets in fd */
	return RUN_ALL_TESTS();
}
