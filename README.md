<!---
 Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>
--->

Easy to use  [HDF5][hdf5] C++17 compiler assisted templates for HDF5  [with live trial][999]
----------------------------------------------------------------------------------------------------

[Hierarchical Data Format][hdf5] prevalent in high performance scientific computing, sits directly on top of sequential or parallel file systems, providing block and stream operations on standardized or custom binary/text objects. Scientific computing platforms such as Python, R, Matlab, Fortran,  Julia [and many more...] come with the necessary libraries to read write HDF5 dataset. This edition simplifies interactions with popular linear algebra libraries, provides [compiler assisted seamless object persistence](@ref link_h5cpp_compiler), Standard Template Library support.

Templates:
----------

**create dataset within an opened hdf5 file**

```cpp
file ::= const h5::fd_t& fd | const std::string& file_path;
dataspace ::= const h5::sp_t& dataspace | const h5::current_dims& current_dim [, const h5::max_dims& max_dims ] |  
				[,const h5::current_dims& current_dim] , const h5::max_dims& max_dims;
lcpl ::=

template <typename T> h5::ds_t create( file, const std::string& dataset_path, dataspace, 
						[, const h5::lcpl_t& lcpl] [, const h5::dcpl_t& dcpl] [, const h5::dapl_t& dapl]  );

```

**read a dataset and return a reference of the created object**
```cpp
dataset ::= (const h5::fd_t& fd | const std::string& file_path, const std::string& dataset_path ) | const h5::ds_t& ds;

template <typename T> T read( dataset
							[, const h5::offset_t& offset]  [, const h5::stride_t& stride] [, const h5::count_t& count]
							[, const h5::dxpl_t& dxpl ] ) const;
template <typename T> h5::err_t read( dataset, T& ref 
							[, const h5::offset_t& offset]  [, const h5::stride_t& stride] [, const h5::count_t& count]
							[, const h5::dxpl_t& dxpl ] ) [noexcept] const;						 
```

**write dataset into a specified location**
```cpp
dataset ::= (const h5::fd_t& fd | const std::string& file_path, const std::string& dataset_path ) | const h5::ds_t& ds;

template <typename T> h5::err_t write( dataset, const T* ptr
			[,const hsize_t* offset] [,const hsize_t* stride] ,const hsize_t* count [, const h5::dxpl_t dxpl ]  ) noexcept;
template <typename T> h5::err_t write( dataset,  const T& ref
			[,const h5::offset_t& offset] [,const h5::stride_t& stride]  [,const& h5::dxcpl_t& dxpl] ) [noexept];
```

**append to extendable C++/C struct dataset**
```cpp
#include <h5cpp/core>
	#include "your_data_definition.h"
#include <h5cpp/io>
template <typename T> void h5::append(h5::pt_t& ds, const T& ref) [noexcept];
```

All **file and dataset io** descriptors implement [raii idiom](@ref link_raii_idiom) and close underlying resource when going out of scope, 
and may be seamlessly passed to HDF5 CAPI calls when implicit conversion enabled. Similarly templates can take CAPI `hid_t` identifiers as arguments where applicable provided conversion policy allows. See [conversion policy](@ref link_conversion_policy) for details.

how to use:
-----------
`sudo make install` will copy the header files and `h5cpp.pc` package config file into `/usr/local/` or copy them and ship it with your project.
There is no other dependency than hdf5 libraries and include files. However to activate the template specialization for any given library you must include that library first then h5cpp. In case the auto detection fails turn template specialization on by defining:
```cpp
#define [ H5CPP_USE_BLAZE | H5CPP_USE_ARMADILLO | H5CPP_USE_EIGEN3 | H5CPP_USE_UBLAS_MATRIX 
	| H5CPP_USE_UBLAS_VECTOR | H5CPP_USE_ITPP_MATRIX | H5CPP_USE_ITPP_VECTOR | H5CPP_USE_BLITZ | H5CPP_USE_DLIB | H5CPP_USE_ETL ]
```


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

short  examples:
----------------
*to read/map a 10x5 matrix from a 3D array from location {3,4,1}*
```cpp
#include <armadillo>
#include <h5cpp/all>
...
auto fd = h5::open("some_file.h5", H5F_ACC_RDWR);
/* the RVO arma::Mat<double> object will have the size 10x5 filled*/
try {
	/* will drop extents of unit dimension returns a 2D object */
	auto M = h5::read<arma::mat>(fd,"path/to/object", 
			h5::offset{3,4,1}, h5::count{10,1,5}, h5::stride{3,1,1} ,h5::block{2,1,1} );
} catch (const std::runtime_error& ex ){
	...
}
// fd closes underlying resource (raii idiom)
```
*to write the entire matrix back to a different file*
```cpp
#include <Eigen/Dense>
#include <h5cpp/all>

h5::fd_t fd = h5::create("some_file.h5",H5F_ACC_TRUNC);
h5::write(fd,"/result",M);
```

*to create an dataset recording a stream of struct into an extendable chunked dataset with GZIP level 9 compression:*
```cpp
#include <h5cpp/core>
	#include "your_data_definition.h"
#include <h5cpp/io>
...
auto ds = h5::create<some_type>(fd,"bids", h5::max_dims{H5S_UNLIMITED}, h5::chunk{1000} | h5::gzip{9});
```
*to append records to an HDF5 datastream* 
```cpp
#include <h5cpp/core>
	#include "your_data_definition.h"
#include <h5cpp/io>
auto fd = h5::create("NYSE high freq dataset.h5");
h5::pt_t pt = h5::packet_table::create<ns::nyse_stock_quote>( fd, 
		"price_quotes/2018-01-05.qte",h5::max_dims{H5S_UNLIMITED}, h5::chunk{1024} | h5::gzip{9} );
quote_update_t qu;

bool having_a_good_day{true};
while( having_a_good_day ){
	try{
		recieve_data_from_udp_stream( qu )
		h5::append(pt, qu);
	} catch ( ... ){
	  if( cant_fix_connection() )
	  		having_a_good_day = false; 
	}
}
```

Requirements:
------------
c++17 capable compiler or above until backporting to c++14 is completed. The following instructions will help you to
install and set to default gcc 8.0 on ubuntu 16.04 LTS, whereas [this document will help you with details](@ref link_h5cpp_install)
```bash
sudo add-apt-repository ppa:jonathonf/gcc-8.1 
sudo apt-get update 
sudo apt-get upgrade
sudo apt-get install gcc-8 g++-8
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 100 --slave /usr/bin/g++ g++ /usr/bin/g++-8
```

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

[300]: @ref link_h5cpp_compiler
[301]: @ref link_conversion_policy
[302]: @ref link_exception_policy
[305]: md__home_steven_Documents_projects_h5cpp_docs_pages_compiler_trial.html#link_try_compiler

[999]: http://h5cpp.ca/cgi/redirect.py
<!--
Community Edition vs. [Professional and Enterprise Edition][305]
--------------------------------------------------------------------------------------

|      | linalg | STL         | python objects| Rcpp | [ compiler][300]  | [type][301] and [exception][302] policy | MPI | throughput optimizer | support       |
|------|--------|-------------|-----|---------|------|-----------------------------|-----------|---------------------|---------------|
| CE   | yes    | std::vector |N/A  | N/A     |N/A   | N/A               | N/A     | N/A       | N/A                 |
| PE   | yes    | full        | yes | yes     |yes   | optional          | optional| optional  | optional            |
| EE   | yes    | full        | yes | yes     |yes   | yes               | yes     | yes       | email,chat, phone   |
-->


