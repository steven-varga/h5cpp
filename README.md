<!---
 Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>
--->

Easy to use  [HDF5][hdf5] **C++11** compiler assisted templates for HDF5 with [LIVE DEMO][999] 
----------------------------------------------------------------------------------------------------

[Hierarchical Data Format][hdf5] prevalent in high performance scientific computing, sits directly on top of sequential or parallel file systems, providing block and stream operations on standardized or custom binary/text objects. Scientific computing platforms such as Python, R, Matlab, Fortran,  Julia [and many more...] come with the necessary libraries to read write HDF5 dataset. This edition simplifies interactions with [popular linear algebra libraries][304], provides [compiler assisted seamless object persistence][303], Standard Template Library support and equipped with novel [error handling architecture][400].

NEWS:
------------
- **2018-oct-??** Continuous Integration for Linux and Windows 
- **2018-oct-??** comprehensive [error handling][400] system added
- **2018-aug-13** The backporting of the template library to c++11 has been completed the templates are pure c++11 with no higher requirements. 
- **2018-aug-01** [H5CPP presentation at Chicago C++ UsersGroup][200] with David Pearah and Gerd Heber, HDFGroup
- **2018-jul-30** [HDFGroup Blog post: ][401] the short story of h5cpp
- **2018-jun-01** pythonic syntax and h5cpp compiler prototyped with c++17 requirements
- **2018-apr-12** cooperation with Gerd Heber, HDFGroup
- **2010-jan-06** approach HDFGroup with the idea of h5cpp template library for modern C++


How simple can it get?
```cpp
   h5::write( "arma.h5", "arma vec inside matrix",  V // object contains 'count' and rank being written
            ,h5::current_dims{40,50}  // control file_space directly where you want to place vector
            ,h5::offset{5,0}            // when no explicit current dimension given current dimension := offset .+ object_dim .* stride (hadamard product)  
            ,h5::stride{4,4}, h5::block{3,3}
            ,h5::max_dims{40,H5S_UNLIMITED} )
// wouldn't it be nice to have unlimited dimension? 
// if no explicit chunk is set, then the object dimension is
// used as unit chunk
        
```
The above example is to demonstrate partial create + write with extendable datasets which can be turned into high performance packet table: `h5::pt_t ` by a simple conversion `h5::pt_t pt = h5::open( ... )` ,  `h5::pt_t pt = h5::create(...)` or `h5::pt_t pt = ds` where `h5::ds_t ds = ... `.   


`h5::create | h5::read | h5::write | h5::append` take RAII enabled descriptors or CAPI hid_t ones -- depending on conversion policy: seamless or controlled fashion. The optional arguments may be placed in any order, compile time computed and come with intelligent compile time error messages.

```cpp
h5::write( "arma.h5", "arma vec inside matrix",  V 
      ,h5::stride{4,4}, h5::block{3,3}, h5::current_dims{40,50} 
      ,h5::offset{5,0}, h5::max_dims{40,H5S_UNLIMITED}  );
```
Wouldn't it be nice not to have to  remember everything? Focus on the idea, write it out intuitively and refine it later. The function construct below compiles into the same instructions as above. 

```cpp
h5::write( "arma.h5", "arma vec inside matrix",  V 
      ,h5::max_dims{40,H5S_UNLIMITED}, h5::stride{4,4},  h5::current_dims{40,50}
      ,h5::block{3,3}, h5::offset{5,0},   );
```

Why should you know about HDF5 at all, isn't it work about ideas? the details can be filled in later when needed:
```cpp 
// supported objects:  raw pointers | armadillo | eigen3 | blaze | blitz++ | it++ | dlib | uBlas | std::vector
arma::vec V(4); // some vector: armadillo, eigen3, 
h5::write( "arma.h5", "path/to/dataset name",  V);
```
How about some really complicated POD struct type that your client or colleague  wants to see in action right now?
Invoke clang++ based `h5cpp` compiler on the translation unit -- group of files which are turned into a single object file -- and call it done!
```cpp
namespace sn {
	namespace typecheck {
		struct Record { /*the types with direct mapping to HDF5*/
			char  _char; unsigned char _uchar; short _short; unsigned short _ushort; int _int; unsigned int _uint;
			long _long; unsigned long _ulong; long long int _llong; unsigned long long _ullong;
			float _float; double _double; long double _ldouble;
			bool _bool;
			// wide characters are not supported in HDF5
			// wchar_t _wchar; char16_t _wchar16; char32_t _wchar32;
		};
	}
	namespace other {
		struct Record {                    // POD struct with nested namespace
			MyUInt                    idx; // typedef type 
			MyUInt                     aa; // typedef type 
			double            field_02[3]; // const array mapped 
			typecheck::Record field_03[4]; //
		};
	}
	namespace example {
		struct Record {                    // POD struct with nested namespace
			MyUInt                    idx; // typedef type 
			float             field_02[7]; // const array mapped 
			sn::other::Record field_03[5]; // embedded Record
			sn::other::Record field_04[5]; // must be optimized out, same as previous
			other::Record  field_05[3][8]; // array of arrays 
		};
	}
	namespace not_supported_yet {
		// NON POD: not supported in phase 1
		// C++ Class -> PODstruct -> persistence[ HDF5 | ??? ] -> PODstruct -> C++ Class 
		struct Container {
			double                            idx; // 
			std::string                  field_05; // c++ object makes it non-POD
			std::vector<example::Record> field_02; // ditto
		};
	}
	/* BEGIN IGNORED STRUCT */
	// these structs are not referenced with h5::read|h5::write|h5::create operators
	// hence compiler should ignore them.
	struct IgnoredRecord {
		signed long int   idx;
		float        field_0n;
	};
	/* END IGNORED STRUCTS */
```
I did try to make the above include file  as  ugly and complicated as I could. But do you really need to read it? What if you had a technology at your disposal that can do it for you?  
In your code all you have to do is to trigger the compiler by making any `h5::` operations on the desired data structure. It works without the hmmm boring details?
```cpp
...
std::vector<sn::example::Record> vec 
    = h5::utils::get_test_data<sn::example::Record>(20);
// mark vec  with an h5:: operator and delegate 
// the details to h5cpp compiler
h5::write(fd, "orm/partial/vector one_shot", vec );
...
``` 
H5CPP is a novel approach to  persistence in the field of machine learning, it provides high performance sequential and block access to HDF5 containers through modern C++.

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

how to use:
-----------
`sudo make install` will copy the header files and `h5cpp.pc` package config file into `/usr/local/` or copy them and ship it with your project.
There is no other dependency than hdf5 libraries and include files. However to activate the template specialization for any given library you must include that library first then h5cpp. In case the auto detection fails turn template specialization on by defining:
```cpp
#define [ H5CPP_USE_BLAZE | H5CPP_USE_ARMADILLO | H5CPP_USE_EIGEN3 | H5CPP_USE_UBLAS_MATRIX 
	| H5CPP_USE_UBLAS_VECTOR | H5CPP_USE_ITPP_MATRIX | H5CPP_USE_ITPP_VECTOR | H5CPP_USE_BLITZ | H5CPP_USE_DLIB | H5CPP_USE_ETL ]
```
if you're interested in `h5cpp` compiler please [visit this page][305] to recreate the required clang7.0 toolchain. 

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
[200]: http://h5cpp.ca/md__home_steven_Documents_projects_h5cpp_profiling_README.html
[201]: http://h5cpp.ca/examples.html
[202]: http://h5cpp.ca/modules.html
[305]: md__home_steven_Documents_projects_h5cpp_docs_pages_compiler_trial.html#link_try_compiler
[400]: https://www.meetup.com/Chicago-C-CPP-Users-Group/events/250655716/
[401]: https://www.hdfgroup.org/2018/07/cpp-has-come-a-long-way-and-theres-plenty-in-it-for-users-of-hdf5/
[999]: http://h5cpp.ca/cgi/redirect.py
[301]: http://h5cpp.ca/md__home_steven_Documents_projects_h5cpp_docs_pages_conversion.html
[302]: http://h5cpp.ca/md__home_steven_Documents_projects_h5cpp_docs_pages_exceptions.html
[303]: http://h5cpp.ca/md__home_steven_Documents_projects_h5cpp_docs_pages_compiler.html
[304]: http://h5cpp.ca/md__home_steven_Documents_projects_h5cpp_docs_pages_linalg.html
[305]: http://h5cpp.ca/md__home_steven_Documents_projects_h5cpp_docs_pages_install.html
[400]: http://h5cpp.ca/md__home_steven_Documents_projects_h5cpp_docs_pages_error_handling.html
