# CSV to HDF5

This example shows the small pattern for streaming rows from a CSV file into an HDF5 packet table. The point is simple: a row-at-a-time text source becomes a compressed, chunked, attribute-annotated HDF5 dataset without anyone touching `H5Tinsert` by hand.

The CSV reader is the header-only [Fast C++ CSV Parser](https://github.com/ben-strasser/fast-cpp-csv-parser). The sample data is a [public-domain Monroe County crash dataset](https://data.bloomington.in.gov/dataset/117733fb-31cb-480a-8b30-fbf425a690cd/resource/8673744e-53f2-42d1-9d05-4e412bd55c94/download/monroe-county-crash-data2003-to-2015.csv).

## Files

| File            | Purpose                                                              |
| --------------- | -------------------------------------------------------------------- |
| `csv2hdf5.cpp`  | Reads `input.csv` row by row, appends each row to a packet table     |
| `struct.h`      | POD `input_t` — the on-disk row layout                               |
| `generated.h`   | H5CPP-compiler output: `register_struct<input_t>` HDF5 compound type |
| `input.csv`     | Sample CSV (copied next to the binary by the build)                  |
| `Makefile`      | Standalone Makefile (CMake target is `examples-csv`)                 |

## Row Layout

The C++ side defines the row as a plain POD. Strings are stored inline as fixed-length character arrays — the simplest representation for HDF5, and adequate when the strings are short and bounded. For long or variable-length text, splitting the strings into a separate dataset is often the better call.

```cpp
constexpr int STR_ARRAY_SIZE = 20;

struct input_t {
    long          MasterRecordNumber;
    unsigned int  Hour;
    double        Latitude;
    double        Longitude;
    char          ReportedLocation[STR_ARRAY_SIZE];
};
```

## Includes

```cpp
#include "csv.h"
#include "struct.h"
#include <h5cpp/all>
#include "generated.h"
```

`<h5cpp/all>` pulls in everything h5cpp needs. The compiler-generated `generated.h` carries the HDF5 compound descriptor for `input_t` and follows the h5cpp includes.

## Reading the CSV

`CSVReader<N>` is templated on the number of columns. The header line lets you pick columns by name and ignore the rest:

```cpp
constexpr unsigned N_COLS = 5;
io::CSVReader<N_COLS> in("input.csv");

in.read_header(io::ignore_extra_column,
    "Master Record Number", "Hour", "Reported_Location",
    "Latitude", "Longitude");
```

Then the row pump:

```cpp
input_t row;
char*   ptr;   // CSVReader hands strings out as char* — we copy into row's fixed array

while (in.read_row(row.MasterRecordNumber, row.Hour, ptr,
                   row.Latitude, row.Longitude)) {
    memset(row.ReportedLocation, 0, STR_ARRAY_SIZE);
    strncpy(row.ReportedLocation, ptr, STR_ARRAY_SIZE - 1);
    h5::append(pt, row);
}
```

`h5::append` buffers row insertions internally and flushes them as chunks — single-row writes do not turn into single-row HDF5 transactions.

## Writing the Packet Table

Create the file, create the dataset, attach attributes, hand off to the packet-table handle:

```cpp
h5::fd_t fd = h5::create("output.h5", H5F_ACC_TRUNC);

h5::ds_t ds = h5::create<input_t>(fd, "simple approach/dataset.csv",
    h5::max_dims{H5S_UNLIMITED},  h5::chunk{10} | h5::gzip{9});

ds["data set"]   = "monroe-county-crash-data2003-to-2015.csv";
ds["cvs parser"] = "https://github.com/ben-strasser/fast-cpp-csv-parser";

h5::pt_t pt = ds;     // ds_t casts to pt_t — same handle, packet-table view
```

A few things going on here:

- `h5::ds_t` is the dataset handle; attributes are written on it.
- `h5::pt_t` is the packet-table view of the same dataset; it knows how to buffer + flush appends.
- `h5::max_dims{H5S_UNLIMITED}` makes the dataset extendable along its single axis.
- `h5::chunk{10} | h5::gzip{9}` is a deliberately tiny chunk for a small demo. In production, size the chunk so that one chunk is ≈ 1 MiB or one network MTU.

## H5CPP-Compiler Output

`generated.h` is what the LLVM-based `h5cpp` compiler produces by scanning the TU. It is the HDF5 type descriptor for `input_t` — what would otherwise be a hand-rolled `H5Tcreate(H5T_COMPOUND, ...)` block:

```cpp
#pragma once

#include <h5cpp/all>
#include "struct.h"

namespace h5 {
    template<> hid_t inline register_struct<input_t>(){
        hsize_t at_00_[] ={20};            hid_t at_00 = H5Tarray_create(H5T_NATIVE_CHAR,1,at_00_);

        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (input_t));
        H5Tinsert(ct_00, "MasterRecordNumber",	HOFFSET(input_t,MasterRecordNumber),H5T_NATIVE_LONG);
        H5Tinsert(ct_00, "Hour",	HOFFSET(input_t,Hour),H5T_NATIVE_UINT);
        H5Tinsert(ct_00, "Latitude",	HOFFSET(input_t,Latitude),H5T_NATIVE_DOUBLE);
        H5Tinsert(ct_00, "Longitude",	HOFFSET(input_t,Longitude),H5T_NATIVE_DOUBLE);
        H5Tinsert(ct_00, "ReportedLocation",	HOFFSET(input_t,ReportedLocation),at_00);

        //closing all hid_t allocations to prevent resource leakage
        H5Tclose(at_00); 

        return ct_00;
    };
}
H5CPP_REGISTER_STRUCT(input_t);
```

You do not edit this file. The compiler regenerates it whenever `struct.h` or the source TU changes.

## On-Disk Result

`h5dump -pH output.h5`:

```text
HDF5 "output.h5" {
GROUP "/" {
   GROUP "simple approach" {
      DATASET "dataset.csv" {
         DATATYPE  H5T_COMPOUND {
            H5T_STD_I64LE  "MasterRecordNumber";
            H5T_STD_U32LE  "Hour";
            H5T_IEEE_F64LE "Latitude";
            H5T_IEEE_F64LE "Longitude";
            H5T_ARRAY { [20] H5T_STD_I8LE } "ReportedLocation";
         }
         DATASPACE  SIMPLE { ( 199 ) / ( H5S_UNLIMITED ) }
         STORAGE_LAYOUT {
            CHUNKED ( 10 )
            SIZE 7347 (1.517:1 COMPRESSION)
         }
         FILTERS { COMPRESSION DEFLATE { LEVEL 9 } }
         ATTRIBUTE "data set"   { ... }
         ATTRIBUTE "cvs parser" { ... }
      }
   }
}
}
```

Variable-length attribute strings, a fixed-size character-array column inside the compound, an unlimited-extent dimension chunked at 10, gzip-9 — all from the C++ above.

## Build Notes

The example is wired into the CMake build as `examples-csv`. The build copies `input.csv` next to the binary in the build directory so `./examples-csv` runs without a path argument. To run from anywhere:

```sh
cd <build-dir>
./examples-csv         # writes output.h5 in the current directory
h5dump -pH output.h5   # inspect the result
```

## Mental Model

```text
CSV row  →  POD struct  →  packet-table append  →  chunked, compressed dataset
```

The CSV reader hands you typed columns. The struct is the on-disk row layout. The packet table buffers the appends. The compound type comes from the H5CPP compiler. No `H5Tinsert`, `H5Sclose`, or `H5Dclose` in user code.
