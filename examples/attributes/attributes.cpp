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

	arma::mat matrix = arma::zeros(3,4); for(int i=0; i<matrix.n_elem; i++ ) matrix[i] = i;
	std::vector<sn::example::Record> vector = h5::utils::get_test_data<sn::example::Record>(40);
	sn::example::Record& record = vector[0];
	// set to use the latest file format version to able to use large size attributes
	h5::fd_t fd = h5::create("001.h5", H5F_ACC_TRUNC, h5::default_fcpl,
						h5::libver_bounds({H5F_LIBVER_V18, H5F_LIBVER_V18}) );
	h5::ds_t ds = h5::write(fd,"some dataset with attributes", M);
	{
		/*
		(gr_t | ds_t | dt_t = fd["/root/some/path"]) = some object;
		ds_t ds = ... ;
		

		gr["data set"] = some object;
		ds["attribute"] = some attribute;




		h5::ds_t obj = fd["/some/path"]["attribute"];
		//h5::gr_t obj = fd["/some/path"];*/
	}

	{
		ds["att_01"] = 42 ;
		ds["att_02"] = {1.,3.,4.,5.};
		ds["att_03"] = {'1','3','4','5'};
		ds["att_04"] = {"alpha", "beta","gamma","..."};

		ds["att_05"] = "const char[N]";
		ds["att_06"] = u8"const char[N]áééé";
		ds["att_07"] = std::string( "std::string");

		ds["att_08"] = record; // pod/compound datatype
		ds["att_09"] = vector; // vector of pod/compound type
		ds["att_10"] = matrix; // linear algebra object
	}

	/* supported types:
	 * T := integral | std::string | const char[] | POD struct*
	 * accept := T | std::vector<T> | linalg 
	 * * pod struct requires h5cpp compiler or manual labour
	 */
	{ // create + write
		h5::awrite(ds,"att_21", 42 );
		h5::awrite(ds,"att_22", {1.,3.,4.,5.} );
		h5::awrite(ds,"att_23", {'1','3','4','5'} );
		h5::awrite(ds,"att_24", {"alpha", "beta","gamma","..."} );

		h5::awrite(ds,"att_25", "const char[N]");
		h5::awrite(ds,"att_26", u8"const char[N]áééé");
		h5::awrite(ds,"att_27", std::string( "std::string") );

		h5::awrite(ds,"att_28", record ); // pod/compound datatype
		h5::awrite(ds,"att_29", vector ); // vector of pod/compound type
		h5::awrite(ds,"att_30", matrix ); // linear algebra object
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

