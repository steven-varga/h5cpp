<!---

 Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>

 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this  software  and associated documentation files (the "Software"), to deal in
 the Software  without   restriction, including without limitation the rights to
 use, copy, modify, merge,  publish,  distribute, sublicense, and/or sell copies
 of the Software, and to  permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE  SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT  SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY  CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER
 IN  AN  ACTION  OF  CONTRACT, TORT OR  OTHERWISE, ARISING  FROM,  OUT  OF  OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--->

EXAMPLES:
=========

string.cpp:
-----------
to demonstrate variable length null terminated/ C style string storage to/from std::vector and HDF5 dataset

```c++
	// partial write strings into a matrix of variable length strings

	hid_t ds = h5::create<std::string>(fd, "/string.mat",{6,6},{2,2}, 9); 	// gzipped chunked {6x6} matrix<std::string> data-set 
		h5::write(ds, data, {2,1},{3,2} ); 									// partial write a {3x2} block to coordinates: {2x1}
	H5Dclose(ds); 
```

h5dump -d string.mat string.h5


```c++
HDF5 "string.h5" {
	DATASET "string.mat" {
		DATATYPE  H5T_STRING {
		  STRSIZE H5T_VARIABLE;
		  STRPAD H5T_STR_NULLTERM;
		  CSET H5T_CSET_ASCII;
		  CTYPE H5T_C_S1;
	   }
   DATASPACE  SIMPLE { ( 6, 6 ) / ( 6, 6 ) }
	   DATA {
			(0,0): NULL, NULL, 				NULL, 						NULL, NULL, NULL,
			(1,0): NULL, NULL, 				NULL, 						NULL, NULL, NULL,
			(2,0): NULL, "QQzQRNlHIEZR", 	"hYbcNzzLshW", 				NULL, NULL, NULL,
			(3,0): NULL, "qWNudKSiWLaOLsG", "gZLLzXmtNNBJp", 			NULL, NULL, NULL,
			(4,0): NULL, "roDxKp", 			"XmBIjFcrdTLimmEYEtaTvO", 	NULL, NULL, NULL,
			(5,0): NULL, NULL, 				NULL, 						NULL, NULL, NULL
		}
	}
}
	
```

