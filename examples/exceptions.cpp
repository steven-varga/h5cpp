/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */


#include <functional>
#include <iostream>
#include <stack>
#include <sstream>
#include <streambuf>
#include <iomanip>

#include <hdf5.h>

struct ErrorController {
	ErrorController(){
		//install user specific handler
		H5Eset_auto(H5E_DEFAULT, ErrorController::error_handler, this );
	}
	/* forward all calls then once cleaned up H5_CAPI stack throw error */
	template<class capi_t, class... args_t>
	hid_t call(capi_t fn, args_t&&... args ){
		hid_t err =  (*fn)(std::forward<args_t>(args)...);
		if( err < 0 )
			throw std::runtime_error( walk_error() );
		return err;
	}
	/* static helper to replace error stack */
	static herr_t error_handler (long int a, void *ptr) {
		hid_t es = H5Eget_current_stack();
		// walk error stack and update state
		herr_t err = H5Ewalk(es, H5E_WALK_DOWNWARD, ErrorController::error_walker, ptr );
		H5Eclear( es );
		// allow C API to do cleanup, by CAPI contract
		return es;
	}

	/* a possible implementation of error walker */
	static herr_t error_walker(unsigned int n, const H5E_error_t* ed, void *ptr){

		ErrorController * _this = static_cast<ErrorController*>( ptr );
		std::stringstream ss;

		ss << n <<" " <<ed->file_name <<" "<< ed->line <<" "<<ed->func_name<<" " << ed->desc << std::endl;

		_this->stack.push( ss.str() );

		return 0; 
	}

	std::string walk_error(){
		std::stringstream ss;
		while( !stack.empty() )
			ss << stack.top(), stack.pop();
		return ss.str();
	}

	private:
		std::stack<std::string> stack;
};


int main(){

	ErrorController wrap;

	std::cout << "------------------- NO EXCEPTION -------------------  \n";
	// will call installed error handler, which can't throw from the inside
	if( hid_t fd = H5Fopen("nonexisting_file.h5",0,0) )
		std::cerr << wrap.walk_error();  // but can safely do this
	// that may not that good looking for everyone 

	std::cout << "------------------- TRY - CATCH -------------------  \n";
	try{
	// wrap it:
		wrap.call(H5Fopen,"non_existing_file.h5",0,0);
	} catch ( std::exception& e ){
		std::cout << e.what();
	}
	std::cout << "------------------- END -------------------  \n";
}



