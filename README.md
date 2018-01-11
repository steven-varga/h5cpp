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


an easy to use c++11 templates between ([std::vector][1] | [armadillo][2] ) and [HDF5][3] datasets 
--------------------------------------------------------------------------------------------------

Hierarchical Data Format or HDF5 prevalent in high performance scientific computing, sits directly on top of sequential or parallel file systems, providing block and sequential operations on standardized or custom binary/text objects.Scientific computing platforms such as Julia, Matlab, R, Python, C/C++, Fortran come with the necessary libraries to read write HDF5 dataset. However the [C/C++ API][4] provided by HDF Group requires detailed understanding the file format and doesn't support popular c++ objects such as **armadillo**,**stl**

HDF5 CPP is a set of routines to simplify the process and by implementing **CREATE,READ,WRITE,APPEND** operations on **fixed** or **variable length** N dimensional arrays.
This header only implementation supports **raw pointers**, **stl::vector**, **armadillo**  matrix library by directly operating on the underlying data-store of the object hence avoiding unnecessary memory allocations.

performance: 
------------
|    experiment                               | time  | trans/sec | Mbyte/sec |
|:--------------------------------------------|------:|----------:|----------:|
|append:  1E6 x 64byte struct                 |  0.06 |   16.46E6 |   1053.87 |
|append: 10E6 x 64byte struct                 |  0.63 |   15.86E6 |   1015.49 |
|append: 50E6 x 64byte struct                 |  8.46 |    5.90E6 |    377.91 |
|append:100E6 x 64byte struct                 | 24.58 |    4.06E6 |    260.91 |
|write:  Matrix<float> [10e6 x  16] no-chunk  |  0.4  |    0.89E6 |   1597.74 |
|write:  Matrix<float> [10e6 x 100] no-chunk  |  7.1  |    1.40E6 |    563.36 |

Lenovo 230 i7 8G ram laptop on Linux Mint 18.1 system

requirements:
-------------
1. installed serial HDF5 libraries:
	- pre-compiled on ubuntu: `sudo apt install libhdf5-serial-dev hdf5-tools hdf5-helpers hdfview`
	- from source: [HDF5 download][5]
	`./configure --prefix=/usr/local --enable-build-mode=production --enable-shared --enable-static --enable-optimization=high --with-default-api-version=v110 --enable-hl`
	`make -j4` then `sudo make install`

2. C++11 or above capable compiler installed HDF5 libraries: `sudo apt install build-essential g++`
3. set the location of the include library, and c++11 or higher flag: `h5c++  -Iyour/project/../h5cpp -std=c++14` or `CFLAGS += pkg-config --cflags h5cpp`
4. optionally include `<armadillo>`  header file before including `<h5cpp/all>`

usage:
-------
`sudo make install` will copy the header files and `h5cpp.pc` package config file into `/usr/local/` or copy them and ship it with your project. There is no other dependency than hdf5 libraries and include files. However in order to use armadillo  correctly you have to be sure to include them first.

*to read/map a 10x5 matrix from a 3D array from location {3,4,1}*
```cpp
#include <h5cpp/all>
...
hid_t fd h5::open("some_file.h5");
	/* the RVO arma::Mat<double> object will have the size 10x5 filled*/
	auto M = h5::read<arma::mat>(fd,"path/to/matrix",{3,4,1},{10,1,5});
h5::close(fd);
```

*to write the entire matrix back to a different file*
```cpp
#include <h5cpp/all>
...
hid_t fd = h5::create("output.h5")
	h5::write(fd,"/result",M);
h5::close(fd);
```
*to create an dataset recording a stream of struct into an extendable chunked dataset with GZIP level 9 compression:*
```cpp
#include <h5cpp/core>
	#include "your_data_definition.h"
#include <h5cpp/io>
...
hid_t ds = h5::create<some_type>(fd,"bids",{H5S_UNLIMITED},{1000}, 9);
```
*to append records to an HDF5 datastream* 
```cpp
#include <h5cpp/core>
	#include "your_data_definition.h"
#include <h5cpp/io>

auto ctx = h5::context<some_struct>( dataset );
for( record:entire_dataset)
			h5::append(ctx, record );
```

Templates:
-----------

**create dataset within an opened hdf5 file**
```cpp
using par_t = std::initializer_list<hsize_t>

template <typename T> hid_t create(  hid_t fd, const std::string& path, const T ref );
template <typename T> hid_t create(hid_t fd, const std::string& path, par_t max_dims, par_t chunk_dims={},
															const int32_t deflate = H5CPP_NO_COMPRESSION );
```

**read a dataset and return a reference of the created object**
```cpp
using par_t = std::initializer_list<hsize_t>

template <typename T> T read(hid_t fd, const std::string& path ); 
template <typename T> T read(hid_t ds ); 
template <typename T> T read(hid_t ds, par_t offset, par_t count  ); 
template <typename T> T read(hid_t fd, const std::string& path, par_t offset, par_t count  );
```

**write dataset into a specified location**
```cpp
using par_t = std::initializer_list<hsize_t>

template <typename T> void write(hid_t ds, const T* ptr, const hsize_t* offset, const hsize_t* count );
template <typename T> void write(hid_t ds, const T* ptr, par_t offset, par_t count);
template <typename T> void write(hid_t ds, const T& ref, par_t offset, par_t count);
template <typename T> void write(hid_t fd, const std::string& path, const T& ref);
template <typename T> void write(hid_t fd, const std::string& path, const T& ref, par_t offset, par_t count);
template <typename T> void write(hid_t fd, const std::string& path, const T* ptr, par_t offset, par_t count);
```

**append to extentable C++/C struct dataset]**
```cpp
#include <h5cpp/core>
	#include "your_data_definition.h"
#include <h5cpp/io>

template <typename T> context<T>( hid_t ds);
template <typename T> context<T>( hid_t fd, const std::string& path);
template <typename T> void append( h5::context<T>& ctx, const T& ref);
```

supported types:
---------------- 

```yacc
	T := ([unsigned] ( int8_t | int16_t | int32_t | int64_t )) | ( float | double  )
	S := T | c/c++ struct | std::string
	ref 	:= std::vector<S> | arma::Row<T> | arma::Col<T> | arma::Mat<T> | arma::Cube<T>
	ptr 	:= T* 
	accept 	:= ref | ptr 
```

in addition to the standard data types offered by BLAS/LAPACK systems `std::vector` also supports `std::string` data-types mapping N dimensional variable-length C like string HDF5 data-sets to `std::vector<std::string>` objects.


[documentation](http://h5cpp.ca/modules.html), [examples](http://h5cpp.ca/examples.html), google-test:
----------------------------------------------------------------------------------------------------
`make all` generates doxygen documention into docs/html and compiles `examples/*.cpp`
In `tests` directory there are instruction on google test suit, similarly you find instructions in 
`h5cpp/profiling`

**to build documentation, examples and profile code install:**

```shell
apt install build-essential libhdf5-serial-dev
apt install google-perftools kcachegrind
apt install doxygen doxygen-gui markdown
```

TODO:
-----
1. statistical profiling of read|write|create operations, and visualization
2. replace macro generics with templates, resulting clean c++11 experience
3. support for [eigen3][6], [boost matrix library][7]
4. sparse matrix support: [compressed sparse row][9], [compressed sparse column][10]
5. implement  complex numbers, `std::vector<bool>`

98. optional pre-compiled libraries (libh5cpp.so|libh5cpp.a)
99. add more test cases [in progress]

<div style="text-align: right">
**Copyright (c) 2018 vargaconsulting, Toronto,ON Canada** <steven@vargaconsulting.ca>
</div>

[1]: http://en.cppreference.com/w/cpp/container/vector
[2]: http://arma.sourceforge.net
[3]: https://support.hdfgroup.org/HDF5/doc/H5.intro.html
[4]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5Front.html
[5]: https://support.hdfgroup.org/HDF5/release/obtain5.html
[6]: http://eigen.tuxfamily.org/index.php?title=Main_Page
[7]: http://www.boost.org/doc/libs/1_65_1/libs/numeric/ublas/doc/matrix.htm
[8]: https://julialang.org/
[9]: https://en.wikipedia.org/wiki/Sparse_matrix#Compressed_sparse_row_.28CSR.2C_CRS_or_Yale_format.29
[10]: https://en.wikipedia.org/wiki/Sparse_matrix#Compressed_sparse_column_.28CSC_or_CCS.29

[40]: https://support.hdfgroup.org/HDF5/Tutor/HDF5Intro.pdf
