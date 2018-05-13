<!---
 Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>
--->


C++ API   {#link_api_templ}
================================

```cpp
using offset_t = std::initializer_list<hsize_t>;
using stride_t = std::initializer_list<hsize_t>;
using chunk_t  = std::initializer_list<hsize_t>;
fd_t ::= [ unique_ptr | shared_ptr | hid_t ]

herr_t = h5::write<T, fd_t>( fd_t fd, 
		const std::string& path,     // not thread safe, but within guidlines
		const T& ref ,offset_t os ,stride_t st ,chunk_t ch , dcpl_t dcpl , dapl_t dapl, dtpl_t dtpl ) noexcept([true|false]);
```
To help you to focus on the task at hand and worry about the details when really needed no need to memorize the entire function call, most of the arguments are either can be omitted, or replaced with sensible `h5::absent` or the equivalent CAPI default values: `H5P_DEFAULT` if auto conversion is enabled.
```cpp
h5::write(fd, "/my_object", arma::mat(10,20) );                               // entire dataset, no filters, nor chunks
h5::write(fd, "/my_object", {}, {}, {10,4});                                  // yes to chunks
h5::write(fd, "/my_object", h5::absent, h5::absent, {10,4});                  // same as above, with default values
h5::write(fd, "/my_object", h5::absent, h5::absent, {10,4}, h5::gzip{9} );    // setting gzip to max level, chunks must be used
h5::write(fd, "/my_object", {}, {}, {10,4}, h5::gzip{9} | h5::nan );          // setting gzip to max level, chunks must be used

// there is room for details through HDF5 CAPI property lists, by either creating one from CAPI calls or 
// use intializer list style approach. Notice that `chunks` are not set here, but passed as a function argument, 
// if this is not preffered, set chunk size using CAPI, then pass `h5::default` to function call.
dcpl_t dcpl{
	H5D_CHUNKED,            // layout: H5D_COMPACT | H5D_CONTIGUOUS | H5D_CHUNKED | H5D_VIRTUAL
	0,                      // H5D_CHUNK_DONT_FILTER_PARTIAL_CHUNKS | 0    
	9,                      // gzip compression level:
	NaN,                    // fill value
	H5D_FILL_TIME_ALLOC,    // fill time: H5D_FILL_TIME_IFSET | H5D_FILL_TIME_ALLOC | H5D_FILL_TIME_NEVER 
	H5D_ALLOC_TIME_LATE };  // alloc time: H5D_ALLOC_TIME_DEFAULT | H5D_ALLOC_TIME_EARLY | H5D_ALLOC_TIME_INCR | H5D_ALLOC_TIME_LATE
h5::write(fd, "/my_object", h5::default, h5::default, {10,4}, dcpl );          // call function with set data control property list, chunks passed

// as alternative, when you need more, you can always apply CAPI calls to dcpl_t:
hsize_t chunks[] = {10,4};
H5Pset_chunk( static_cast<hid_t>( dcpl ), 2, chunks );                        // CAPI calls to set up data control property list 
h5::write(fd, "/my_object", dcpl, h5::default, h5::default );                 // property list style call, h5::default values are optional
```

with `-DEXCEPTIONS_ENFORCED`
```
auto fd = h5::create( ... );
arma::mat m(10,20);
try{
	h5::write(fd, "/my_object"); // entire dataset no chunks, no filters

	h5::write(fd, "/my_object", {0,0},{1,1},{10,4}, h5::P_DEFAULT, h5::P_DEFAULT, h5::P_DEFAULT);
} catch( ... ){
}

```


HDF5 handlers and C++ RAII   {#link_raii_types}
================================================
All types are wrapped into `h5::impl::hid_t<T>` template then 
```yacc
T := [ file_handles | property_list ]
file_handles   := [ fd_t | ds_t | att_t | err_t | grp_t | id_t | obj_t ]
property_lists := [ file | group | link | dataset | type | object | attrib | access ]

#           create   access  transfer  copy 
file    := [fcpl_t | fapl_t                    ] 
group   := [gcpl_t | gapl_t                    ]
link    := [lcpl_t | lapl_t                    ]
dataset := [dcpl_t | dapl_t | dtpl_t           ]
type    := [         tapl_t                    ]
object  := [ocpl_t                   | ocpyl_t ]
attrib  := [acpl_t 
```




```cpp

struct some_class {
	// RAII: circumventing RAII is compile error
	some_class( std::string fn, std::string dn )
	// initialization is guaranteed in order, resource release in reverse order
	// also: no need for copy ctor allows simpler implementation
   	 :fd( h5::open( fn ) ), ds(h5::open(fd,dn))	{ // order is important: 1. open file, 2. open dataset
	}
	// some function that cannot easily circumvent RAII
	void open_file( std::string fn, std::string dn ){
	  //this->fd =  h5::open( fn );             // compile error, it is not RAII
	  //hid_t ds = h5::open(fd, "some_path");   // ditto, this conversion + assignment would break RAII
	                                            // use CAPI if you needs are not covered
	  auto ds_ok = h5::open(fd,"some_dataset"); // is OK, works as advertised: will close resources when leaving codeblock
	} // closes local ds_ok here

	const h5::fd_t& fd;
	const h5::ds_t& ds;
};


int main(){
	std::cout<<"----------------- BEGIN TEST  handle := [-1,1] any other undefined ----------- \n";
	#ifdef H5CPP_CAPI_CONVERSION_ENABLED
		std::cout << "YES CAPI AUTOMATIC CONVERSIONS ENABLED \n\n";
	#else
		std::cout << "NO CAPI AUTOMATIC CONVERSIONS ENABLED  \n\n";
	#endif

	{ // RAII usage inside class
		std::cout<<"----------------- begin class -----------------\n";
	                                   // fd_t takes responsibility and manages valid resource
		hid_t hdf_hand_over=1;         // calling H5?close on managed resource, or handing invalid descriptor
		                               // results undefined behavior when leaving code block
		h5::fd_t fd{hdf_hand_over};    // this is allowed, must know what you're doing, also easy to 'grep' 
		//h5::fd_t fd_wrong(fd);       // <-- compiler error: error: use of deleted function
		                               // no copy allowed, it must be a unique object
									   // use std::move instead
		h5::fd_t fd2 = std::move(fd);  // this is OK, fd is invalid, will not call H5?close multiple times
		some_class res("path","dataset"); // class instantiation acquires resources: RAII
		std::cout<<"----------------- end class -----------------\n";
	} // local resources are released

	{ //RAII inside code block 

		std::cout<<"----------------- begin block -----------------\n";
		h5::fd_t fd =  h5::open("example.h5"    // 
		                        /*,args...*/);  // dtor releases resource when leaving codeblock: RAII
		// auto fd2 = fd;         				// compile error since copy ctor deleted, to prevent passing it as copy
		const h5::fd_t& fd3 = fd; 				// this is ok, remains a single object, with a references
		auto ds = h5::open(fd, 
				         "name"/*,args*/);     // this is desirable, type safe call, no-copy guarantees resource remains valid
		                                        // returned 'ds' will follow RAII pattern
												// h5::create can return safe descriptor and signal error by throwing
		auto ds2 = h5::create<SomeObject>(fd,
		             "name"/*,args...*/ );
	                                            // or just ignore the return value, and have it released when leaving 'h5::create'
		h5::create<SomeObject>(fd,
				        "name"/*,args*/);
		// compile error: becuase returned value is cast to hid_t, then temporary fd_t will close underlying handle 
		// hid_t ds3 = h5::create<SomeObject>(fd, "name"/*,args...*/ );
	#ifdef H5CPP_CAPI_CONVERSION_DISABLED
		hid_t fd4 = static_cast<hid_t>( fd      // allowed C API calls through explicit conversion, 
				/*, args...*/ );                // others know what is happening
		// H5Dclose( fd4 ); leads to undefined behaviour, must not call on managed resource, 
		// correct usage is to open files|datasets with H5?open( ) then do business then call H5?close();
		H5Dwrite( static_cast<hid_t>(fd),0,0,0,0, nullptr );
	    //H5Dwrite( fd,0,0,0,0, nullptr );     // compile error: conversion not enabled
	#else
	                                            // allows C API calls through implicit conversion, 
	    H5Dwrite( fd,0,0,0,0, nullptr );        // conversion enabled, makes it easier to work with
		H5Dclose( fd );                         // is not a compiler error, but still is undefined; must not call 
	#endif
		std::cout<<"----------------- end block -----------------\n";
	}
	std::cout<<"----------------- END TEST -----------------";
	std::cout<<"\n\n";

}

```



```cpp
namespace h5{ namespace impl {
	struct unmanaged final {};
	template <class T> struct hid_t final {
		hid_t()=delete;
	};
	template <> struct hid_t<unmanaged> final {
		operator ::hid_t() const {
			return  handle;
		}
		hid_t( ::hid_t fd ) : handle(fd){}
		private:
		::hid_t handle;
	};
}}
```


C/C++ type map to HDF5 file space  						{#link_base_template_types}
==================================

TODO: write detailed description of supported types, and how memory space is mapped to file space



Support for Popular Scientific Libraries                {#link_linalg_template_types}
=========================================

TODO: write detailed description of supported types, and how memory space is mapped to file space


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

