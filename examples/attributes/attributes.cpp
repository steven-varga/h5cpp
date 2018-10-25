#include<armadillo>
#include <h5cpp/all>
#include <cstddef>

int main(){
	arma::mat M = arma::zeros(5,6);


	h5::fd_t fd = h5::create("001.h5", H5F_ACC_TRUNC);
	auto ds = h5::write(fd,"some dataset with attributes", M);
	/* supported types:
	 * T := integral | std::string 
	 * accept := T | std::vector<T> 
	 */
	{ // create + write
		arma::mat att_01 = arma::zeros(3,4); for(int i=0; i<att_01.n_elem; i++ ) att_01[i] = i;
		std::cerr << att_01 <<"\n";
		h5::awrite(ds,"att_01", att_01 );
		h5::awrite(ds,"att_03", {1.,3.,4.,5.} );
		h5::awrite(ds,"att_04", {'1','3','4','5'} );
		h5::awrite(ds,"att_05", {"alpha", "beta","gamma","..."} );

		h5::awrite(ds,"att_06", "const char[N]");
		h5::awrite(ds,"att_07", std::string( "std::string") );

		h5::awrite(ds,"att_02", 42 );
	}

	{ // open + write -> attribute size must not change
		arma::mat att_01 = arma::ones(3,4);
		h5::awrite(ds,"att_01", att_01 );
		//TODO: attribute with different dimension
		// 1.) remove previous attribute
		// 2.) create new attribute
		// 3.) write attribute
		h5::awrite(ds,"att_03", {1.,3.,4.,5.   ,6.,.7} );
		// trigger runtime error and then handle it
		h5::mute();
		try{
			h5::awrite(ds,"att_03", {"one","two","..."} );
		} catch ( const h5::error::io::attribute::any& err ){
			std::cerr <<"INTENTIONAL ERROR: " <<err.what() << std::endl;
		}
		h5::unmute();
	}
	{ // reading back attribute is always single shot, no partial IO 
		int a = h5::aread<int>(ds,"att_02");
		arma::mat att_01 = h5::aread<arma::mat>(ds,"att_01");
		std::cerr << "epxecting 'ones' becuase of the rewrite: \n";
		std::cerr << att_01 <<"\n";
	}
}

