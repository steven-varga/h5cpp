#include <armadillo>
#include <cstdint>

#include "struct.h"
#include <h5cpp/core>
	// generated file must be sandwiched between core and io 
	// to satisfy template dependencies in <h5cpp/io>  
	#include "generated.h"
#include <h5cpp/io>
#include <cstddef>
#include "utils.hpp"

int main(){
	arma::mat M = arma::zeros(5,6);


	h5::fd_t fd = h5::create("001.h5", H5F_ACC_TRUNC);
	h5::ds_t ds = h5::write(fd,"some dataset with attributes", M);


	/* supported types:
	 * T := integral | std::string | const char[] | POD struct*
	 * accept := T | std::vector<T> | linalg 
	 * * pod struct requires h5cpp compiler or manual labour
	 */
	{ // create + write
		arma::mat matrix = arma::zeros(3,4); for(int i=0; i<matrix.n_elem; i++ ) matrix[i] = i;
		std::vector<sn::example::Record> vector = h5::utils::get_test_data<sn::example::Record>(4);
		sn::example::Record& record = vector[0];

		h5::awrite(ds,"att_01", 42 );
		h5::awrite(ds,"att_02", {1.,3.,4.,5.} );
		h5::awrite(ds,"att_03", {'1','3','4','5'} );
		h5::awrite(ds,"att_04", {"alpha", "beta","gamma","..."} );

		h5::awrite(ds,"att_05", "const char[N]");
		h5::awrite(ds,"att_06", u8"const char[N]áééé");
		h5::awrite(ds,"att_07", std::string( "std::string") );

		h5::awrite(ds,"att_08", record ); // pod/compound datatype
		h5::awrite(ds,"att_09", vector ); // vector of pod/compound type
		h5::awrite(ds,"att_10", matrix ); // linear algebra object
	}

	{ // open + write -> attribute size must not change
		arma::mat att_01 = arma::ones(3,4);
		h5::awrite(ds,"att_01", att_01 );
		//TODO: attribute with different dimension
		// 1.) remove previous attribute
		// 2.) create new attribute
		// 3.) write attribute
		h5::awrite(ds,"att_02", {1.,3.,4.,5.,6.,.7} );
		// trigger runtime error and then handle it
		h5::mute();
		try{
			h5::awrite(ds,"att_02", {"one","two","..."} );
		} catch ( const h5::error::io::attribute::any& err ){
			std::cerr <<"INTENTIONAL ERROR: " <<err.what() << std::endl;
		}
		h5::unmute();
	}
	{ // reading back attribute is always single shot, no partial IO 
		int a = h5::aread<int>(ds,"att_01");
		arma::mat att_10 = h5::aread<arma::mat>(ds,"att_10");
		std::cerr << att_10 <<"\n";
	}
}

