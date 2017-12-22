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


#ifndef  H5CPP_CHRONO_PTIME_H 
#define H5CPP_CHRONO_PTIME_H

	//DURATION
	template <> inline chrono::duration as<chrono::duration>( const std::string& du ){
		return boost::posix_time::duration_from_string( du );
	}
	template <> inline chrono::duration as<chrono::duration>( char const* du ){
		std::string str( du );
		return boost::posix_time::duration_from_string( str );
	}
	/** ptime -> duration
	 */
	template <> inline chrono::duration as<chrono::duration>( const chrono::ptime pt ){
		return pt - chrono::ptime(chrono::date(1970,1,1));
	}
	/** double -> duration 
	 */ 
	template <> inline chrono::duration as<chrono::duration>( double du ){
		return boost::posix_time::time_duration(0,0,0, boost::posix_time::time_duration::ticks_per_second() * du);
	}
	// tick -> duration
	template <> inline chrono::duration as<chrono::duration>( unsigned long du ){
		return boost::posix_time::time_duration(0,0,0, du);
	}

	// POSIX TIME POINT
	template <> inline chrono::ptime as<chrono::ptime>( const std::string& pt ){
		//return boost::date_time::parse_delimited_time<chrono::ptime>(pt, 'T');
		return boost::posix_time::time_from_string( pt );
	}
	template <> inline chrono::ptime as<chrono::ptime>( char const* pt ){
		std::string pt_( pt );
//		return boost::date_time::parse_delimited_time<chrono::ptime>(pt, 'T');
		return boost::posix_time::time_from_string( pt_ );
	}
	template <> inline chrono::ptime as<chrono::ptime>( const time_t pt ){
		return boost::posix_time::from_time_t( pt );
	}
	/** tick -> posix time
	 */
	template <> inline chrono::ptime as<chrono::ptime>( unsigned long pt ){
		return boost::posix_time::ptime(boost::gregorian::date(1970,1,1), h5::as<chrono::duration>( pt ));
	}
	template <> inline chrono::ptime as<chrono::ptime>( chrono::duration du ){
		return boost::posix_time::ptime(boost::gregorian::date(1970,1,1), du);
	}
	/** double -> posix time
	 */
	template <> inline chrono::ptime as<chrono::ptime>( const double pt ){
		if( std::isnan( pt ) )
			return boost::posix_time::not_a_date_time;
		else
			return boost::posix_time::ptime(boost::gregorian::date(1970,1,1), h5::as<chrono::duration>(pt));
	}
	// unsigned long or ticks
	template <> inline unsigned long as<unsigned long>( const chrono::duration td ){
		return td.ticks();
	}
	template <> inline unsigned long as<unsigned long>(const  chrono::ptime pt ){
		return as<chrono::duration>( pt ).ticks();
	}
	// DOUBLE
	template <> inline double as<double>( const chrono::duration du ){
		return static_cast<double>(du.ticks())/du.ticks_per_second();
	}
	template <> inline double as<double>(const  chrono::ptime pt ){
		return as<double>(  pt - chrono::ptime(chrono::date(1970,1,1)) );
	}
	// string
	template <> inline std::string as<std::string>( const chrono::duration td ){
		return boost::posix_time::to_simple_string( td );
	}
	template <> inline std::string as<std::string>( const chrono::ptime pt ){
		return boost::posix_time::to_iso_extended_string( pt );
		//return boost::posix_time::to_simple_string( pt );
	}
	template <> inline std::string as<std::string, chrono::iterator>(  chrono::iterator it ){
		return boost::posix_time::to_simple_string( *it );
	};


#endif


