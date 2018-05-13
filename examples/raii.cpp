/*
 *   Copyright © <2018> Varga Consulting, Toronto, On          info@vargaconsulting.ca
 *   _________________________________________________________________________________
 */


#include "../api/capi.hpp"
#include "../api/raii.hpp"


/*
lightweight std::unique_ptr style wrap with restriction that no copy allowed, the alternative to 
1.) std::shared_ptr approach, where copies allowed, at the cost of reference count, and thread safe mechanism
2.) direct wrap to std::unique_ptr<hid_t,close_resource_fn> which indicates what is underneath, doesn't puzzle users, 
but makes auto conversion more complicated
3.) direct wrap to std::shared_ptr<hid_t, ...> same as above, sturdier + heavier too
 
as of options:
   one can choose between the capi style, and can't get leaner than that, or pass a reference or pass a reference with some reference counting magic
 
this is to show a light weight and functional implementation that releases resource, has a layout of size_of( hid_t ) , and comes with optional 
implicit conversion.

struct fd_t{
	// provide optional implicit conversion, or possibly go the other way: enforce conversion 
	#ifndef H5CPP_CAPI_CONVERSION_ENABLED
		explicit
	#endif
		operator hid_t(){ return handle; }
		fd_t(const fd_t&) = delete;
		fd_t(fd_t&& ref) = default;
		fd_t& operator=(fd_t&&) = default;

		fd_t(hid_t handle_ ):handle(handle_){}

		~fd_t(){
			std::cout<<"closing file   : " << this <<" :" << handle <<"\n";
		}
		private:
			hid_t handle;
	};
*/



int main(){

	std::cout<<"----------------- BEGIN -----------------\n";
	#ifdef H5CPP_CAPI_CONVERSION_ENABLED
		std::cout << "YES CAPI AUTOMATIC CONVERSIONS ENABLED\n\n";
	#else
		std::cout << "NO CAPI AUTOMATIC CONVERSIONS ENABLED\n\n";
	#endif

	{
		using namespace pr001;

		h5::fd_t fd =  h5::open("example.h5"/*,args...*/);  // dtor releases resource when leaving codeblock: RAII
		// auto fd2 = fd;         				// compile error since copy ctor deleted, to prevents passing as copy
		const h5::fd_t& fd3 = fd; 				// this is ok
		// h5::open( const fd_t& reference, const std::string& path);
		auto ds = h5::open(fd, "somedataset"/*, args*/);  // this is desirable, type safe call, no-copy guarantees resource remains valid
		                                        // returned 'ds' will follow RAII pattern
												// h5::create can return safe descriptor and signal error by throwing
		auto ds2 = h5::create<SomeObject>(fd,"some_other_object"/*, other args...*/ );
	                                            // or just ignore the return value, and have it released when leaving 'h5::create'
		h5::create<SomeObject>(fd,"some_other_object");
	#ifdef H5CPP_CAPI_CONVERSION_DISABLED
		hid_t fd4 = static_cast<hid_t>( fd /*, args...*/ );   // allows C API calls through explicit conversion, others know what is happening
		H5Dwrite( static_cast<hid_t>(fd),0,0,0,0, nullptr );
	    // H5Dwrite( fd,0,0,0,0, nullptr );     // compile error: conversion not enabled
	#else
	                                            // allows C API calls through implicit conversion, 
			                                    // 
	    H5Dwrite( fd,0,0,0,0, nullptr );        // conversion enabled
	#endif
	}
	std::cout<<"----------------- END -----------------";
	std::cout<<"\n\n";
}

