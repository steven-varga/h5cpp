<!---
 Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>
--->

Easy to use  [HDF5][hdf5] C++ templates for Serial and Paralell HDF5  
----------------------------------------------------------------------

[Hierarchical Data Format][hdf5] prevalent in high performance scientific computing, sits directly on top of sequential or parallel file systems, providing block and stream operations on standardized or custom binary/text objects. Scientific computing platforms such as Python, R, Matlab, Fortran,  Julia [and many more...] come with the necessary libraries to read write HDF5 dataset. This edition simplifies interactions with [popular linear algebra libraries][304], provides [compiler assisted seamless object persistence][303], Standard Template Library support and equipped with novel [error handling architecture][400].

H5CPP is a novel approach to  persistence in the field of machine learning, it provides high performance sequential and block access to HDF5 containers through modern C++ [Download packages from here.](http://h5cpp.org/download) If you are interested in h5cpp LLVM/clang based source code transformation tool [you find it in this separate project.](https://github.com/steven-varga/h5cpp-compiler)

You can read this [blog post][500] published on HDFGroup Blog site to find out where the project is originated. [Click here Doxygen based Documentation][501] pages. Browse [highlighted examples][502], follow this link to [our spring presentation](http://webinar.h5cpp.org) or take a peek at the upcoming [ISC'19 BOF](http://isc19.hdf5.io), where I am [presenting H5CPP](https://forum.hdfgroup.org/t/hdf5-bof-at-isc-19/5692).

H5CPP for MPI 
---------------
Proud to announce to the HPC community that H5CPP is now MPI capable. The prerequisites are: c++17 capable MPI compiler, and linking against the Parallel HDF5 library. The template system provides the same easy to use functionality as in the serial version, and may be enabled by including parallel `hdf5.h` then passing `h5::mpiio({mpi_com, mpi_info})` to `h5::create | h5::open | h5::write | h5::read `, as well as `h5::independent` and `h5::collective` data transfer properties. There are examples for [independent][503], [collective][504] IO, as well as a [short program][505] to demonstrate throughput. The MPI extension supports all parallel HDF5 features, while the documentation is in progress please look at the tail end of `H5Pall.hpp` for details.

**Note:** `h5::append` operator and attributes are not yet tested, and probably are non-functional.

Tested against:
-----------------
- gcc 7.4.0, gcc 8.3.0, gcc 9.0.1
- clang 6.0

**Note:** the preferred compiler is gcc, however there is work put in to broaden the support for major modern C++ compilers. Please contact me if there is any shortcomings.

Templates:
----------

**create dataset within an opened hdf5 file**

```cpp
file ::= const h5::fd_t& fd | const std::string& file_path;
dataspace ::= const h5::sp_t& dataspace | const h5::current_dims& current_dim [, const h5::max_dims& max_dims ] |  
				[,const h5::current_dims& current_dim] , const h5::max_dims& max_dims;

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

All **file and dataset io** descriptors implement [raii idiom][301] and close underlying resource when going out of scope, 
and may be seamlessly passed to HDF5 CAPI calls when implicit conversion enabled. Similarly templates can take CAPI `hid_t` identifiers as arguments where applicable provided conversion policy allows. See [conversion policy][301] for details.

install:
-----------
On the [projects download page](http://h5cpp.org/download) you find debian, rpm and general tar.gz packages with detailed instructions. Or get the
download link to the header only [h5cpp-dev_1.10.4.deb](http://h5cpp.org/download/h5cpp-dev_1.10.4.1_amd64.deb) and the binary compiler 
[h5cpp_1.10.4.deb](http://h5cpp.org/download/h5cpp_1.10.4.1_amd64.deb) directly from this page.


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
h5::pt_t pt = h5::create<ns::nyse_stock_quote>( fd, 
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
[200]: http://h5cpp.org/md__home_steven_Documents_projects_h5cpp_profiling_README.html
[201]: http://h5cpp.org/examples.html
[202]: http://h5cpp.org/modules.html
[305]: md__home_steven_Documents_projects_h5cpp_docs_pages_compiler_trial.html#link_try_compiler
[400]: https://www.meetup.com/Chicago-C-CPP-Users-Group/events/250655716/
[401]: https://www.hdfgroup.org/2018/07/cpp-has-come-a-long-way-and-theres-plenty-in-it-for-users-of-hdf5/
[999]: http://h5cpp.org/cgi/redirect.py
[301]: http://h5cpp.org/md__home_steven_Documents_projects_h5cpp_docs_pages_conversion.html
[302]: http://h5cpp.org/md__home_steven_Documents_projects_h5cpp_docs_pages_exceptions.html
[303]: http://h5cpp.org/md__home_steven_Documents_projects_h5cpp_docs_pages_compiler.html
[304]: http://h5cpp.org/md__home_steven_Documents_projects_h5cpp_docs_pages_linalg.html
[305]: http://h5cpp.org/md__home_steven_Documents_projects_h5cpp_docs_pages_install.html
[400]: http://h5cpp.org/md__home_steven_Documents_projects_h5cpp_docs_pages_error_handling.html
[500]: http://h5cpp.org/md__home_steven_Documents_projects_h5cpp_docs_pages_blog.html
[501]: http://h5cpp.org/modules.html
[502]: http://h5cpp.org/examples.html
[503]: http://h5cpp.org/independent_8cpp-example.html
[504]: http://h5cpp.org/collective_8cpp-example.html
[505]: http://h5cpp.org/throughput_8cpp-example.html
