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

#ifndef H5CPP_MAX_RANK
	#define H5CPP_MAX_RANK 5 //< maximum dimensions of stored arrays
#endif
#ifndef H5CPP_NO_COMPRESSION 
	#define H5CPP_NO_COMPRESSION 0 //< maximum dimensions of stored arrays
#endif
#ifndef H5CPP_DEFAULT_COMPRESSION 
	#define H5CPP_DEFAULT_COMPRESSION 9 //< maximum dimensions of stored arrays
#endif
#ifndef H5CPP_RANK_VEC 
	#define H5CPP_RANK_VEC 1
#endif
#ifndef H5CPP_RANK_MAT 
	#define H5CPP_RANK_MAT 2
#endif
#ifndef H5CPP_RANK_CUBE 
	#define H5CPP_RANK_CUBE 3
#endif


namespace h5{
	typedef std::complex<double> cx_double;
	typedef std::complex<float>  cx_float;
}
/** @defgroup file-io HDF5 file IO
 *  file IO operations
 *  @{
 *  @} 
 */

/** @defgroup io-create HDF5 dataset Create
 *  dataset CREATE operations
 *  @{
 *  @} 
 */

/** @defgroup io-read HDF5 dataset Read
 *  dataset READ  operations
 *  @{
 *  @} 
 */


/** @defgroup io-write HDF5 dataset Write
 *  dataset WRITE operations
 *  @{
 *  @} 
 */

/** @defgroup io-append HDF5 dataset Append
 *  dataset APPEND operations
 *  @{
 *  @} 
 */

/** @defgroup chrono 
* **conversions between types:** boost::[duration|ptime|date], double, unsigned long, std::string 
* with nanosecond resolution. For higher 1E-9 resolution see [BOOST date-time][1]. The internals 
* are in [chrono.md][2] and [more examples are here][3] 
* As side effect it includes boost headers and you also must link against  `-lboost_date_time`
*
*```
*     #include <boost/date_time/posix_time/posix_time.hpp>
*     #include <boost/date_time/gregorian/gregorian.hpp>
*```
*
*  **usage:**
*``` 
*	#include <h5cpp/core>
*	#include <h5cpp/chrono>
*	#include <h5cpp/io>
*		
*	...
*	auto pt = boost::posix_time::microsec_clock::local_time();
*	std::string time = h5::as<std::string>(pt);
*	...
*```
* [1]: http://www.boost.org/doc/libs/master/doc/html/date_time/details.html#date_time.buildinfo)
* [2]: \ref link-chrono
* [3]: \ref link-examples-chrono
* [4]: https://stat.ethz.ch/R-manual/R-devel/library/base/html/DateTimeClasses.html
* @{
* @} 
*/





