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

// must be included before <h5cpp/core>
#include <armadillo>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <h5cpp/core>
#include <h5cpp/mock.hpp>

#include "event_listener.hpp"
#include "abstract.h"
#include <h5cpp/chrono.hpp>
#include <h5cpp/create.hpp>
#include <h5cpp/read.hpp>
#include <h5cpp/write.hpp>

namespace ch = h5::chrono;




//-------------------------------------- PTIME -----------------------------------------------
// ptime: double,string,duration,unsigned long
TEST(double, ptime) {
	ch::ptime pt = boost::posix_time::microsec_clock::local_time();
	double value =  h5::as<double>( pt );  ch::ptime pt2 = h5::as<ch::ptime>(value);

	EXPECT_STREQ( boost::posix_time::to_simple_string(pt2).data(), boost::posix_time::to_simple_string(pt).data() );
}
TEST(string, ptime) {
	ch::ptime pt = boost::posix_time::microsec_clock::local_time();
	auto value =  h5::as<std::string>( pt );

	EXPECT_STREQ( value.data(), boost::posix_time::to_iso_extended_string(pt).data() );
}
TEST(duration, ptime) {
	ch::ptime pt = boost::posix_time::microsec_clock::local_time();
	ch::duration du = h5::as<ch::duration>( pt );
	ch::ptime value =  h5::as<ch::ptime>( du );

	EXPECT_STREQ( boost::posix_time::to_simple_string(value).data(), boost::posix_time::to_simple_string(pt).data() );
}
TEST(unsigned_long, ptime) {
	ch::ptime pt = boost::posix_time::microsec_clock::local_time();
	ch::ptime pt2 =  h5::as<ch::ptime>( h5::as<unsigned long>( pt ));

	EXPECT_STREQ( boost::posix_time::to_simple_string(pt2).data(), boost::posix_time::to_simple_string(pt).data() );
}


//-------------------------------------- DURATION -----------------------------------------------
// duration: double,string,ptime,unsigned long
TEST(double, duration) {
	ch::duration du =  h5::as<ch::duration>( boost::posix_time::microsec_clock::local_time() );
	double value =  h5::as<double>( du );  
	std::string a = h5::as<std::string>( du ); 
	std::string b = h5::as<std::string>( h5::as<ch::duration>( value) );
	EXPECT_STREQ( a.data(), b.data() );
}
TEST(string, duration) {
	ch::duration du =  h5::as<ch::duration>( boost::posix_time::microsec_clock::local_time() );
	std::string a =  h5::as<std::string>( du );
	std::string b =  boost::posix_time::to_simple_string( du );
	EXPECT_STREQ( a.data(), b.data() );
}
TEST(ptime, duration) {
	ch::duration du =  h5::as<ch::duration>( boost::posix_time::microsec_clock::local_time() );
	ch::ptime value =  h5::as<ch::ptime>( du );
	std::string a =  boost::posix_time::to_simple_string( h5::as<ch::duration>( value ));
	std::string b =  boost::posix_time::to_simple_string( du );
	EXPECT_STREQ( a.data(), b.data() );
}
TEST(unsigned_long, duration) {
	ch::duration du =  h5::as<ch::duration>( boost::posix_time::microsec_clock::local_time() );
	unsigned long value =  h5::as<unsigned long>( du );
	std::string a =  boost::posix_time::to_simple_string( h5::as<ch::duration>( value ));
	std::string b =  boost::posix_time::to_simple_string( du );
	EXPECT_STREQ( a.data(), b.data() );
}

TEST(double, string) {
	ch::ptime pt =  boost::posix_time::microsec_clock::local_time();
	std::string a = "2010-01-31 00:00:00.123456";
	double d = ch::posix_time::as<double>( a );
	std::string b = ch::posix_time::as<std::string>(d);
	std::string c = h5::as<std::string>( h5::as<ch::ptime>(d) );
	//std::cout <<"*** "<< a <<" " << b << " " << c  <<" ***";
	EXPECT_STREQ( c.data(), b.data() );
}

//*** 2010-01-31 00:00:00.123456 2010-01-31T00:00:00.123456 2010-01-31T00:00:00.123456 ***string
TEST(ptime,parse) {
	auto a = h5::as<ch::ptime>("2010-01-31 00:00:00.123456" );
	auto b = h5::as<ch::ptime>("2010-01-31 00:00:00" );
}
TEST(duration,parse) {
	auto a = h5::as<ch::duration>("01:02:03" );
	auto b = h5::as<ch::duration>("00:100:70" );
}
TEST(date, parse) {
	auto a = h5::as<ch::date>("2010-01-31" );
	auto b = h5::as<ch::date>("2010-Jan-31" );
	EXPECT_DOUBLE_EQ( h5::as<double>(a), h5::as<double>(b) );
	EXPECT_TRUE( h5::as<ch::date>("2010-01-29") < h5::as<ch::date>("2010-01-30")   );
	EXPECT_TRUE( h5::as<ch::date>("2010-01-30") > h5::as<ch::date>("2010-01-29")   );
}
TEST(duration, double2string) {
	double seconds = 50000.0;
	std::string str = h5::chrono::time_duration::as<std::string>(seconds);
	double value = h5::chrono::time_duration::as<double>(str);
	EXPECT_DOUBLE_EQ( seconds, value );
}
TEST(duration,string2double) {
	std::string du = "01:00:00";
	double value = h5::chrono::time_duration::as<double>(du);
	std::string str = h5::chrono::time_duration::as<std::string>(value);
	EXPECT_STREQ( du.data(), str.data() );
}




/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_TEST_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

