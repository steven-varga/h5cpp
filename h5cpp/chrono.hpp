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

// to see how std::time_t is defined:  echo | gcc -E -xc -include 'time.h' - | grep time_t

#ifndef  H5CPP_CHRONO_H 
#define H5CPP_CHRONO_H

namespace h5{
	template <typename T, typename F> T as( F from );
	#ifdef POSIX_TIME_HPP___
	/**
	 * must link against -lboost_date_time
	 */ 
	namespace chrono {
		typedef boost::posix_time::time_duration duration;
		typedef boost::posix_time::ptime ptime;
		typedef boost::posix_time::time_period period;
		typedef boost::posix_time::time_iterator iterator;

		typedef boost::posix_time::seconds seconds;
		typedef boost::posix_time::minutes minutes;
		typedef boost::posix_time::hours hours;
	}
	#endif
	#ifdef GREGORIAN_HPP__
		namespace chrono {
			typedef boost::gregorian::date date;
		}
	#endif

	#include "chrono_posix_time.hpp"
	#include "chrono_gregorian.hpp"

	#if ( defined POSIX_TIME_HPP___   &&  defined GREGORIAN_HPP__ )
		template <> inline chrono::date as<chrono::date>( const chrono::ptime& pt ){
			return pt.date();
		}
		template <> inline chrono::ptime as<chrono::ptime>( const chrono::date dt ){
			return chrono::ptime(dt);
		}
		template <> inline double as<double>( const chrono::date dt ){
			//return h5::as<double>(chrono::ptime(dt));
		}

	#endif

	namespace chrono{ namespace posix_time {
		template <typename T, typename F> T as( F from );
		/**  char* -> posix time -> double
		 */
		template <> inline double as<double>( const char* str ){
			chrono::ptime pt = h5::as<chrono::ptime>(str);
			return h5::as<double>( pt );
		}
		/** std::string -> posix time -> double
		 */
		template <> inline double as<double>( const std::string pt ){
			return as<double>( pt.data() );
		}
		/** double -> posix time -> std::string
		 */
		template <> inline std::string as<std::string>( double value ){
			const chrono::ptime pt = h5::as<chrono::ptime>(value);
			return h5::as<std::string>( pt );
		}
		/** tick -> posix time -> std::string
		 */ 
		template <> inline std::string as<std::string>( const unsigned long long int value ){
			const chrono::ptime pt = h5::as<chrono::ptime>(value);
			return h5::as<std::string>( pt );
		}
	}}

	namespace chrono{ namespace time_duration {
		template <typename T, typename F> T as( F from );
		/**  char* -> posix time -> double
		 */
		template <> inline double as<double>( const char* str ){
			chrono::duration pt = h5::as<chrono::duration>(str);
			return h5::as<double>( pt );
		}
		/** std::string -> posix time -> double
		 */
		template <> inline double as<double>( const std::string pt ){
			return as<double>( pt.data() );
		}
		/** double -> posix time -> std::string
		 */
		template <> inline std::string as<std::string>( double value ){
			const chrono::duration pt = h5::as<chrono::duration>(value);
			return h5::as<std::string>( pt );
		}
		/** tick -> posix time -> std::string
		 */ 
		template <> inline std::string as<std::string>( const unsigned long long int value ){
			const chrono::duration pt = h5::as<chrono::duration>(value);
			return h5::as<std::string>( pt );
		}
	}}
}

namespace h5{ namespace chrono{
		template <typename T> inline  std::vector<T> sequence(
				chrono::duration begin, chrono::seconds interval, chrono::duration end  ){

			std::vector<T> idx;
			chrono::duration it = begin+interval;
			while( it <= end )
				idx.push_back( h5::as<std::string>(it) ), it += interval;

			return idx;
		}
		template <typename T> inline  std::vector<T> sequence(
				const std::string&  begin, unsigned int interval, const std::string& end  ){

			return sequence<T>(
					h5::as<chrono::duration>(begin),  chrono::seconds( interval ), h5::as<chrono::duration>(end) );
		}
}}
#endif
