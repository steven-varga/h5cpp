/* Copyright (c) 2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include "csv.h"
// data structure include file: `struct.h` must precede 'generated.h' as the latter contains dependencies
// from previous
#include "struct.h"

#include <h5cpp/core>      // has handle + type descriptors
// sandwiched: as `h5cpp/io` depends on `henerated.h` which needs `h5cpp/core`
	#include "generated.h" // uses type descriptors
#include <h5cpp/io>        // uses generated.h + core 

int main(){

	// create HDF5 container
	h5::fd_t fd = h5::create("output.h5",H5F_ACC_TRUNC);
	// create dataset   
	// chunk size is unrealistically small, usually you would set this such that ~= 1MB or an ethernet jumbo frame size
	h5::ds_t ds = h5::create<input_t>(fd,  "simple approach/dataset.csv",
				 h5::max_dims{H5S_UNLIMITED}, h5::chunk{10} | h5::gzip{9} );
	// `h5::ds_t` handle is seamlessly cast to `h5::pt_t` packet table handle, this could have been done in single step
	// but we need `h5::ds_t` handle to add attributes
	h5::pt_t pt = ds;
	// attributes may be added to `h5::ds_t` handle
	ds["data set"] = "monroe-county-crash-data2003-to-2015.csv";
	ds["cvs parser"] = "https://github.com/ben-strasser/fast-cpp-csv-parser"; // thank you!

	constexpr unsigned N_COLS = 5;
	io::CSVReader<N_COLS> in("input.csv"); // number of cols may be less, than total columns in a row, we're to read only 5
	in.read_header(io::ignore_extra_column, "Master Record Number", "Hour", "Reported_Location","Latitude","Longitude");
	input_t row;                           // buffer to read line by line
	char* ptr;      // indirection, as `read_row` doesn't take array directly
	while(in.read_row(row.MasterRecordNumber, row.Hour, ptr, row.Latitude, row.Longitude)){
		strncpy(row.ReportedLocation, ptr, STR_ARRAY_SIZE); // defined in struct.h
		h5::append(pt, row);
		std::cout << std::string(ptr) << "\n";
	}
	// RAII closes all allocated resources
}
