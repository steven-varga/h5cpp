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
	sn::example::Record& record = vector[3];
	// set to use the latest file format version to able to use large size attributes
	h5::fd_t fd = h5::create("001.h5", H5F_ACC_TRUNC, h5::default_fcpl,
						h5::libver_bounds({H5F_LIBVER_V18, H5F_LIBVER_V18}) );
	h5::ds_t ds = h5::write(fd,"directory/dataset", M);

	{ // std::tuple
		/* bundled up operations with std::tuple<...>
		 */
		auto tuple = std::make_tuple(
				"temperature",           18,
				"pressure",              0.5,
				"const char*",           "const char[N]",
				"std::il<const char*>",  std::initializer_list<const char*>{"A","B","C","..."},
				"std::il<std::string>",  std::initializer_list<std::string>{"A","B","C","..."},
				"std::il<record>",       std::initializer_list<sn::example::Record>{vector[0],vector[1],vector[2]},
				"std::vec<const char*>",  std::vector<const char*>{"A","B","C","..."},
			//	"std::vec<std::string>",  std::vector<std::string>{"A","B","C","..."},
				"std::vec<record>",       std::vector<sn::example::Record>{vector[0],vector[1],vector[2]},
	
				"matrix",                matrix,
				"vector",                vector,
				"pod struct",            record
				);
		h5::gr_t gr = h5::gcreate(fd, "my-group" );
		h5::awrite(gr, tuple);
	}
return 0;
	{
		ds["att_01"] = 42 ;
		ds["att_02"] = {1.,2.,3.,4.};
		ds["att_03"] = {'1','2','3','4'};
		ds["att_04"] = {"alpha", "beta","gamma","..."};

		ds["att_05"] = "#05 const char[N]";
		ds["att_06"] = u8"#06 const char[N]áééé";
		ds["att_07"] = std::string( "#07 std::string");

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

		h5::awrite(ds,"att_25", "#25 const char[N]");
		h5::awrite(ds,"att_26", u8"#26 const char[N]áééé");
		h5::awrite(ds,"att_27", std::string( "#27 std::string") );

		h5::awrite(ds,"att_28", record ); // pod/compound datatype
		h5::awrite(ds,"att_29", vector ); // vector of pod/compound type
		h5::awrite(ds,"att_30", matrix ); // linear algebra object
	}
	{//directory
		h5::gr_t gr{H5Gopen(fd,"/directory", H5P_DEFAULT)};
		h5::awrite(gr,"att_32", 42 );
		h5::awrite(gr,"att_33", {1.,3.,4.,5.} );
		h5::awrite(gr,"att_34", {'1','3','4','5'} );
		h5::awrite(gr,"att_35", {"alpha", "beta","gamma","..."} );

		h5::awrite(gr,"att_36", "#36 const char[N]");
		h5::awrite(gr,"att_37", u8"#37 const char[N]áééé");
		h5::awrite(gr,"att_38", std::string( "#38 std::string") );

		h5::awrite(gr,"att_39", record ); // pod/compound datatype
		h5::awrite(gr,"att_40", vector ); // vector of pod/compound type
		h5::awrite(gr,"att_41", matrix ); // linear algebra object
	}



	{ // open + write -> attribute size must not change
		arma::mat att_01 = arma::ones(3,4);
		h5::awrite(ds,"att_01", att_01 );
		//TODO: attribute with different dimension
		// 1.) remove previous attribute
		// 2.) create new attribute
		// 3.) write attribute
		h5::awrite(ds,"att_02", {1.,2.,3.,4.,5.,6.} );
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
	{ // reading back attribute is always single shot, no partial IO 
		// vector of ints
		std::cout << "att_01 : "; int att_01 = h5::aread<int>(ds,"att_01"); std::cout << att_01 << "\n";
		// vector of doubles
		std::cout << "att_02 : "; auto att_02 = h5::aread<std::vector<double>>(ds,"att_02");
		std::cout << att_02 << "\n";
		// vector of ints
		std::cout << "att_03 : "; auto att_03 = h5::aread<std::vector<int>>(ds,"att_03");
		std::cout << att_03 << "\n";
		// vector of strings, NOTE: charaters or initializer lists are not yet supported
		std::cout << "att_04 : "; auto att_04 = h5::aread<std::vector<std::string>>(ds,"att_04");
		std::cout<< att_04 <<"\n";
		// single string
		std::cout << "att_05 : "; auto att_05 = h5::aread<std::string>(ds,"att_05");
		std::cout<< att_05 <<"\n";
			// POD type rank 0
		std::cout << "att_08 : "; sn::example::Record att_08 = h5::aread<sn::example::Record>(ds,"att_08");
		std::cout<< att_08.idx <<"\n";
		// POD type rank 1
		std::cout << "att_09 : "; auto att_09 = h5::aread<std::vector<sn::example::Record>>(ds,"att_09");
		for(auto v: att_09) std::cout << v.idx <<","; std::cout <<std::endl;
		// linear algebra object
		std::cout << "att_10 : "; auto att_10 = h5::aread<arma::mat>(ds,"att_10"); std::cout << att_10 << "\n";
	}
}

