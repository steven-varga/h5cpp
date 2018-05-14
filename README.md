<!---
 Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>
--->

an easy to use c++11 templates between popular matrix algebra systems and [HDF5][hdf5] datasets 
--------------------------------------------------------------------------------------------------------------------------------------------------

[Hierarchical Data Format][hdf5] prevalent in high performance scientific computing, sits directly on top of sequential or parallel file systems, providing block and sequential operations on standardized or custom binary/text objects. Scientific computing platforms such as Julia, Matlab, R, Python, C/C++, Fortran come with the necessary libraries to read write HDF5 dataset. However the [C/C++ API][4] provided by HDF Group requires detailed understanding the file format and doesn't support popular [c++ scientific libraries][11].

HDF5 CPP is to simplify object persistence by implementing **CREATE,READ,WRITE,APPEND** operations on **fixed** or **variable length** N dimensional arrays.
This header only implementation supports [raw pointers][99] | [armadillo][100] | [eigen3][102] | [blaze][106] | [blitz++][103] |  [it++][104] | [dlib][105] |  [uBlas][101] | [std::vector][1]
by directly operating on the underlying data-store, avoiding intermediate/temporary memory allocations.
The api [is doxygen documented][202], furnished with  [examples][201], as well as [profiled][200].

supported classes:
----------------------
```yacc
T := ([unsigned] ( int8_t | int16_t | int32_t | int64_t )) | ( float | double  )
S := T | c/c++ struct | std::string
ref := std::vector<S> 
	| arma::Row<T> | arma::Col<T> | arma::Mat<T> | arma::Cube<T> 
	| Eigen::Matrix<T,Dynamic,Dynamic> | Eigen::Matrix<T,Dynamic,1> | Eigen::Matrix<T,1,Dynamic>
	| Eigen::Array<T,Dynamic,Dynamic>  | Eigen::Array<T,Dynamic,1>  | Eigen::Array<T,1,Dynamic>
	| blaze::DynamicVector<T,rowVector> |  blaze::DynamicVector<T,colVector>
	| blaze::DynamicVector<T,blaze::rowVector> |  blaze::DynamicVector<T,blaze::colVector>
	| blaze::DynamicMatrix<T,blaze::rowMajor>  |  blaze::DynamicMatrix<T,blaze::colMajor>
	| itpp::Mat<T> | itpp::Vec<T>
	| blitz::Array<T,1> | blitz::Array<T,2> | blitz::Array<T,3>
	| dlib::Matrix<T>   | dlib::Vector<T,1> 
	| ublas::matrix<T>  | ublas::vector<T>
ptr 	:= T* 
accept 	:= ref | ptr 
```

In addition to the standard data types offered by BLAS/LAPACK systems and [POD struct][12] -s,  `std::vector` also supports `std::string` data-types mapping N dimensional variable-length C like string HDF5 data-sets to `std::vector<std::string>` objects.

how to use:
-----------
`sudo make install` will copy the header files and `h5cpp.pc` package config file into `/usr/local/` or copy them and ship it with your project.
There is no other dependency than hdf5 libraries and include files. However to activate the template specialization for any given library you must include that library first then h5cpp. In case the auto detection fails turn template specialization on by defining:
```cpp
#define [ H5CPP_USE_BLAZE | H5CPP_USE_ARMADILLO | H5CPP_USE_EIGEN3 | H5CPP_USE_UBLAS_MATRIX 
	| H5CPP_USE_UBLAS_VECTOR | H5CPP_USE_ITPP_MATRIX | H5CPP_USE_ITPP_VECTOR | H5CPP_USE_BLITZ | H5CPP_USE_DLIB | H5CPP_USE_ETL ]
```

short  examples:
----------------
*to read/map a 10x5 matrix from a 3D array from location {3,4,1}*
```cpp
#include <armadillo>
#include <h5cpp/all>
...
auto fd h5::open("some_file.h5", H5F_ACC_RDWR);
	/* the RVO arma::Mat<double> object will have the size 10x5 filled*/
	try {
		auto M = h5::read<arma::mat>(fd,"path/to/matrix",{3,4,1},{10,1,5});
	} catch (const std::runtime_error& ex ){
		...
	}
// fd closes underlying resource imlementing raii idiom
```
*to write the entire matrix back to a different file*
```cpp
#include <Eigen/Dense>
#include <h5cpp/all>
...
auto fd = h5::create("output.h5")
	h5::write(fd,"/result",M);
```

*to create an dataset recording a stream of struct into an extendable chunked dataset with GZIP level 9 compression:*
```cpp
#include <h5cpp/core>
	#include "your_data_definition.h"
#include <h5cpp/io>
...
auto ds = h5::create<some_type>(fd,"bids",{H5S_UNLIMITED},{1000}, 9);
```
*to append records to an HDF5 datastream* 
```cpp
#include <h5cpp/core>
	#include "your_data_definition.h"
#include <h5cpp/io>
auto fd = h5::create("NYSE high freq dataset.h5");
auto ds = h5::packet_table::create<ns::nyse_stock_quote>( fd, "price_quotes/2018-01-05.qte",{H5S_UNLIMITED},{1024}, 9 );
quote_update_t qu;

bool having_a_good_day{true};
while( having_a_good_day ){
	try{
		recieve_data_from_udp_stream( qu )
		h5::append(ds, qu );
	} catch ( ... ){
	  if( cant_fix_connection() )
	  		having_a_good_day = false; 
	}
}
```
Templates:
-----------

**create dataset within an opened hdf5 file**
```cpp
template <typename T> h5::ds_t create( const h5::fd_t& fd, const std::string& path, const T& ref ) noexcept;
template <typename T> h5::ds_t create( const h5::fd_t& fd, const std::string& path, h5::dims_t max_dims, h5::chunk_t chunk_dims={},
															const int32_t deflate = H5CPP_NO_COMPRESSION ) noexcept;
```

**read a dataset and return a reference of the created object**
```cpp
template <typename T> T read(const std::string& file, const std::string& path ); 
template <typename T> T read(const h5::fd_t& fd, const std::string& path ); 
template <typename T> T read(const h5::ds_t& ds ) noexcept; 
template <typename T> T read(const h5::ds_t& ds, h5::offset_t offset, h5::count_t count  ) noexcept; 
template <typename T> T read(const h5::ds_t& ds, const std::string& path, h5::offset_t offset, h5::count_t count  );
```

**write dataset into a specified location**
```cpp
template <typename T> h5::err_t write(const h5::ds_t& ds, const T* ptr, const hsize_t* offset, const hsize_t* count ) noexcept;
template <typename T> h5::err_t write(const h5::ds_t& ds, const T* ptr, h5::offset_t offset, h5::count_t count) noexcept;
template <typename T> h5::err_t write(const h5::ds_t& ds, const T& ref, h5::offset_t offset, h5::count_t count) noexcept;
template <typename T> h5::err_t write(const h5::ds_t& fd, const std::string& path, const T& ref) noexcept;
template <typename T> h5::err_t write(const h5::ds_t& fd, const std::string& path, const T& ref, h5::offset_t offset, h5::count_t count) noexcept;
template <typename T> h5::err_t write(const h5::ds_t& fd, const std::string& path, const T* ptr, h5::offset_t offset, h5::count_t count) noexcept;
```

**append to extentable C++/C struct dataset**
```cpp
#include <h5cpp/core>
	#include "your_data_definition.h"
#include <h5cpp/io>

template <typename T> void h5::append(const h5::pt_t& ds, const T& ref) noexcept;
```

**file and dataset io** all descriptors implement raii idiom and close underlying resource when going out of scope, in addition may be
 seamlessly passed/receive HDF5 CAPI types with implicit conversion enabled `-DHCPP_IMPLICIT_CONVERSION_ON`  or though explicit:
  `hid_t capi_fd = static_cast<hid_t>( h5cpp_fd );` The library defaults to `-DHCPP_IMPLICIT_CONVERSION_OFF` facilitating type safety.

```cpp
h5::fd_t h5::create( const std::string& path, unsigned flags, const h5::cpl_t& cpl = h5::CP_DEFAULT, const h5::apl_t& apl = h5::AP_DEFAULT );
h5::fd_t h5::open( const std::string& path, unsigned flags, const h5::apl_t& apl = h5::AP_DEFAULT );
h5::ds_t h5::open( const h5::fd_t& fd, const std::string& path, unsigned flags, const h5::apl_t& apl = h5::AP_DEFAULT );
h5::pt_t h5::pocket_table::open( const h5::fd_t& fd, const std::string& path, unsigned flags, const h5::apl_t& apl = h5::AP_DEFAULT );
```



<!--
DONE:
-----
* [eigen3][102], [ublas][101], [itpp][104] [blitz][103] [blaze][106]  added
-->

<div style="text-align: right">
**Copyright (c) 2018 vargaconsulting, Toronto,ON Canada** <steven@vargaconsulting.ca>
</div>

[hdf5]: https://support.hdfgroup.org/HDF5/doc/H5.intro.html


[1]: http://en.cppreference.com/w/cpp/container/vector
[2]: http://arma.sourceforge.net
[4]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5Front.html
[5]: https://support.hdfgroup.org/HDF5/release/obtain5.html
[6]: http://eigen.tuxfamily.org/index.php?title=Main_Page
[7]: http://www.boost.org/doc/libs/1_65_1/libs/numeric/ublas/doc/matrix.htm
[8]: https://julialang.org/
[9]: https://en.wikipedia.org/wiki/Sparse_matrix#Compressed_sparse_row_.28CSR.2C_CRS_or_Yale_format.29
[10]: https://en.wikipedia.org/wiki/Sparse_matrix#Compressed_sparse_column_.28CSC_or_CCS.29
[11]: https://en.wikipedia.org/wiki/List_of_numerical_libraries#C++
[12]: http://en.cppreference.com/w/cpp/concept/StandardLayoutType

[40]: https://support.hdfgroup.org/HDF5/Tutor/HDF5Intro.pdf



[99]: https://en.wikipedia.org/wiki/C_(programming_language)#Pointers
[100]: http://arma.sourceforge.net/
[101]: http://www.boost.org/doc/libs/1_66_0/libs/numeric/ublas/doc/index.html
[102]: http://eigen.tuxfamily.org/index.php?title=Main_Page#Documentation
[103]: https://sourceforge.net/projects/blitz/
[104]: https://sourceforge.net/projects/itpp/
[105]: http://dlib.net/linear_algebra.html
[106]: https://bitbucket.org/blaze-lib/blaze
[107]: https://github.com/wichtounet/etl


[200]: http://h5cpp.ca/md__home_steven_Documents_projects_h5cpp_profiling_README.html
[201]: http://h5cpp.ca/examples.html
[202]: http://h5cpp.ca/modules.html

