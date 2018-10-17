<!---
 Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>
--->


Error handling  {#link_error_handler}
============================================================

Error handling follows the C++ [Guidline][1] and the philosophy H5CPP library is built around, that is to  help you to start without reading much of the documentation, and providing ample of room for more should you require it. The root of exception tree is: `h5::error::any` derived from std::`runtime_exception` in accordance with C++ guidelines [custom exceptions][2]. 
All HDF5 CAPI calls are considered as resource, and in case of error H5CPP aims to roll back to last known stable state, cleaning up all resource allocations between the call entry and thrown error. This mechanism is guaranteed by RAII. 

For granularity `io::[file|dataset|attribute]` exceptions provided, with the pattern to capture the entire subset by `::any`.
Exceptions thrown with error massages  \__FILE\__ and \__LINE\__ relevant to H5CPP template library with a brief description to help the developer to investigate. This error reporting mechanism uses a macro found inside **h5cpp/config.h** and maybe redefined:
```cpp
	...
// redefine macro before including <h5cpp/ ... >
#define H5CPP_ERROR_MSG( msg ) "MY_ERROR: " 
	+ std::string( __FILE__ ) + " this line: " + std::to_string( __LINE__ ) + " message-not-used"
#include <h5cpp/all> 
	...
```
An example to capture and handle errors centrally:
```cpp
	// some H5CPP IO routines used in your software
	void my_deeply_embedded_io_calls() {
		arma::mat M = arma::zeros(20,4);
		// compound IO operations in single call: 
		//     file create, dataset create, dataset write, dataset close, file close
		h5::write("report.h5","matrix.ds", M ); 
	}

	int main() {
		// capture errors centrally with the granularity you desire
		try {
			my_deeply_embedded_io_calls();		
		} catch ( const h5::error::io::dataset::create& e ){
			// handle file creation error
		} catch ( const h5::error::io::dataset::write& e ){
		} catch ( const h5::error::io::file::create& e ){
		} catch ( const h5::error::io::file::close& e ){
		} catch ( const h5::any& e ) {
			// print out internally generated error message, controlled by H5CPP_ERROR_MSG macro
			std::cerr << e.what() << std::endl;
		}
	}
```
Detailed CAPI error stack may be unrolled and dumped, muted, unmuted with provided methods:

* `h5::mute`   - saves current HDF5 CAPI error handler to thread local storage and replaces it with NULL handler, getting rid of all error messages produced by CAPI. CAVEAT: lean and mean, therefore no nested calls are supported. Should you require more sophisticated handler keep reading on.
* `h5::unmute` - restores previously saved error handler, error messages are handled according to previous handler.

usage:
```cpp
	h5::mute();
	 // ... prototyped part with annoying messages
	 // or the entire application ...
	h5::unmute(); 
```

* `h5::use_errorhandler()` - captures ALL CAPI error messages into thread local storage, replacing current CAPI error handler
comes handy when you want to provide details of error happened. 

`std::stack<std::string> h5::error_stack()` - walks through underlying CAPI error handler

usage:
```cpp
	int main( ... ) {
		h5::use_error_handler();
		try {
			... rest of the [ single | multi ] threaded application
		} catch( const h5::read_error& e  ){
			std::stack<std::string> msgs = h5::error_stack();
			for( auto msg: msgs )
				std::cerr << msg << std::endl;
		} catch( const h5::write_error& e ){
		} catch( const h5::file_error& e){
		} catch( ... ){
			// some other errors
		}
	}
```

**Design criteria**
- All HDF5 CAPI calls are checked with the only exception of `H5Lexsists` where the failure carries information, that the path does not exist yet. 
- Callbacks of CAPI routines doesn't throw any exceptions, honoring the HDF5 CAPI contract, hence allowing the CAPI call to clean up
- Error messages currently are collected in `H5Eall.hpp` may be customized
- Thrown exceptions are hierarchical
- Only RAII capable/wrapped objects used, guaranteed cleanup through stack unrolling

Exception hierarchy is embedded in namespaces, the chart should be interpreted as tree, for instance a file create exception is
`h5::error::io::file::create`. Keep in mind [namespace aliasing][3] allow you customization should you find the long names inconvenient:
```cpp
using file_error = h5::error::io::file
try{
} catch ( const file_error::create& e ){
	// do-your-thing(tm)
}

```
<pre>
h5::error : public std::runtime_error
  ::any               - captures ALL H5CPP runtime errors with the exception of `rollback`
  ::io::              - namespace: IO related error, see aliasing
  ::io::any           - collective capture of IO errors within this namespace/block recursively
      ::file::        - namespace: file related errors
	        ::any     - captures all exceptions within this namespace/block
            ::create  - create failed
			::open    - check if the object, in this case file exists at all, retry if networked resource
			::close   - resource may have been removed since opened
			::read    - may not be fixed, should software crash crash?
			::write   - is it read only? is recource still available since opening? 
			::misc    - errors which are not covered otherwise: start investigating from reported file/line
       ::dataset::    -
			::any
			::create
			::open
			::close
			::read
			::write
			::append
			::misc
      ::attribute::
			::any
			::create
			::open
			::close
			::read
			::write
			::misc
    ::property_list::
	  ::rollback
      ::any
	  ::misc
	  ::argument
</pre>

This is a work in progress, if for any reasons you think it could be improved, or some real life scenario is not represented please shoot me an email with the use case, a brief working example.


Diagnostics   {#link_diagnostics}
==================================
On occasions it comes handy to dump internal state of objects, while currently only `h5::sp_t` data-space descriptor and dimensions supported
in time most of HDF5 CAPI diagnostics/information calls will be added.

```cpp
	h5::ds_t ds =  ... ; 				// obtained by h5::open | h5::create call
	h5::sp_t sp = h5::get_space( ds );  // get the file space descriptor for hyperslab selection
	h5::select_hyperslab(sp,  ... );    // some complicated selection that may fail, and you want to debug
	std::cerr << sp << std::endl;       // prints out the available space
	try { 
		H5Dwrite(ds, ... );            // direct CAPI call fails for with invalid selection
	catch( ... ){
	}
```


[1]: https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#S-errors
[2]: https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Re-exception-types
[3]: https://en.cppreference.com/w/cpp/language/namespace_alias
