# CSV to HDF5 

Public domain CSV example file obtained from [this link](https://data.bloomington.in.gov/dataset/117733fb-31cb-480a-8b30-fbf425a690cd/resource/8673744e-53f2-42d1-9d05-4e412bd55c94/download/monroe-county-crash-data2003-to-2015.csv)
 The CSV library is [Fast C++ CSV Parser](https://github.com/ben-strasser/fast-cpp-csv-parser)

# C++/C representation

arbitrary pod struct can be represented in HDF5 format, one easy representation of strings is character array. An alternative --often better performing --representation would be to factor out strings from numerical data, then save them in separate datasets.
```
#ifndef  CSV2H5_H 
#define  CSV2H5_H

/*define C++ representation as POD struct*/
struct input_t {
	long MasterRecordNumber;
	unsigned int Hour;
	double Latitude;
	double Longitude;
	char ReportedLocation[20]; // character arrays are supported
};
#endif
```

Reading the CSV is rather easy thanks to [Fast C++ CSV Parser](https://github.com/ben-strasser/fast-cpp-csv-parser), a single header file `csv.h` is attached to the project. Not only fast and simple but also elegantly allows to specify specific columns marked as ncols: `N_COLS`

```
io::CSVReader<N_COLS> in("input.csv"); // number of cols may be less, than total columns in a row, we're to read only 5
in.read_header(io::ignore_extra_column, "Master Record Number", "Hour", "Reported_Location","Latitude","Longitude");
[...]
while(in.read_row(row.MasterRecordNumber, row.Hour, ptr, row.Latitude, row.Longitude)){
	[...]
```

The HDF5 part is matching in simplicity:
```
	h5::fd_t fd = h5::create("output.h5",H5F_ACC_TRUNC);
	h5::pt_t pt = h5::create<input_t>(fd,  "monroe-county-crash-data2003-to-2015.csv",
				 h5::max_dims{H5S_UNLIMITED}, h5::chunk{1024} | h5::gzip{9} ); // compression, chunked, unlimited size
	[...]
	while(...){
		h5::append(pt, row); // append operator uses internal buffers to cache and convert row insertions to block/chunk operations
	}
	[...]
```

The TU translation unit is scanned with LLVM based `h5cpp` compiler and the necessary hdf5 specific type descriptors are produced:
```
#ifndef H5CPP_GUARD_mzMuQ
#define H5CPP_GUARD_mzMuQ

namespace h5{
    //template specialization of input_t to create HDF5 COMPOUND type
    template<> hid_t inline register_struct<input_t>(){
        //hsize_t at_00_[] ={20};            hid_t at_00 = H5Tarray_create(H5T_STRING,20,at_00_);
		hid_t at_00 = H5Tcopy (H5T_C_S1); H5Tset_size(at_00, 20);
        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (input_t));
        H5Tinsert(ct_00, "MasterRecordNumber",	HOFFSET(input_t,MasterRecordNumber),H5T_NATIVE_LONG);
        H5Tinsert(ct_00, "Hour",	HOFFSET(input_t,Hour),H5T_NATIVE_UINT);
        H5Tinsert(ct_00, "Latitude",	HOFFSET(input_t,Latitude),H5T_NATIVE_DOUBLE);
        H5Tinsert(ct_00, "Longitude",	HOFFSET(input_t,Longitude),H5T_NATIVE_DOUBLE);
        H5Tinsert(ct_00, "ReportedLocation",	HOFFSET(input_t,ReportedLocation),at_00);

        //closing all hid_t allocations to prevent resource leakage
        H5Tclose(at_00); 

        //if not used with h5cpp framework, but as a standalone code generator then
        //the returned 'hid_t ct_00' must be closed: H5Tclose(ct_00);
        return ct_00;
    };
}
H5CPP_REGISTER_STRUCT(input_t);

#endif
```

The entire project can be [downloaded from this link](https://github.com/steven-varga/HDFGroup-mailinglist/tree/master/csv-2020-03-03) but for completeness here is the source file:
```
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
```

the output of `h5dump -pH output.h5`
```
HDF5 "output.h5" {
GROUP "/" {
   GROUP "simple approach" {
      DATASET "dataset.csv" {
         DATATYPE  H5T_COMPOUND {
            H5T_STD_I64LE "MasterRecordNumber";
            H5T_STD_U32LE "Hour";
            H5T_IEEE_F64LE "Latitude";
            H5T_IEEE_F64LE "Longitude";
            H5T_ARRAY { [20] H5T_STD_I8LE } "ReportedLocation";
         }
         DATASPACE  SIMPLE { ( 199 ) / ( H5S_UNLIMITED ) }
         STORAGE_LAYOUT {
            CHUNKED ( 10 )
            SIZE 7347 (1.517:1 COMPRESSION)
         }
         FILTERS {
            COMPRESSION DEFLATE { LEVEL 9 }
         }
         FILLVALUE {
            FILL_TIME H5D_FILL_TIME_IFSET
            VALUE  H5D_FILL_VALUE_DEFAULT
         }
         ALLOCATION_TIME {
            H5D_ALLOC_TIME_INCR
         }
         ATTRIBUTE "cvs parser" {
            DATATYPE  H5T_STRING {
               STRSIZE H5T_VARIABLE;
               STRPAD H5T_STR_NULLTERM;
               CSET H5T_CSET_UTF8;
               CTYPE H5T_C_S1;
            }
            DATASPACE  SCALAR
         }
         ATTRIBUTE "data set" {
            DATATYPE  H5T_STRING {
               STRSIZE H5T_VARIABLE;
               STRPAD H5T_STR_NULLTERM;
               CSET H5T_CSET_UTF8;
               CTYPE H5T_C_S1;
            }
            DATASPACE  SCALAR
         }
      }
   }
}
}

```


