
/* Copyright (c) 2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  CSV2H5_H 
#define  CSV2H5_H

constexpr int STR_ARRAY_SIZE = 20;
/*define C++ representation as POD struct*/
struct input_t {
	long MasterRecordNumber;
	unsigned int Hour;
	double Latitude;
	double Longitude;
	char ReportedLocation[STR_ARRAY_SIZE];
};
#endif
