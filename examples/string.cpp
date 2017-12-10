#include <vector>
#include <armadillo>
#include <h5cpp>
#include <mock.hpp>

using namespace h5::mock;

int main(int argc, char **argv) {
	arma::wall_clock timer;

	hid_t fd = h5::create("string.h5");

	hid_t ds = h5::create<std::string>(fd,"/browser info",{H5S_UNLIMITED},{1024}, 0);
	timer.tic();

	long long size = 100'000ll; 
	std::vector<std::string> data = get_test_data<std::string>(size); 	//obtain a set of random length strings 


	{ // WRITE string data one shot into hdf5 dataset 
		h5::write(fd,"/name.str", data); 
	}
	{ // PARTIAL WRITE string data into hdf5 dataset 
		hid_t ds = h5::create<std::string>(fd, "/string.mat",{6,6},{2,2}, 9); 	// gzipped chunked {6x6} matrix<std::string> data-set 
			h5::write(ds, data, {2,1},{3,2} ); 									// partial write a {3x2} block to coordinates: {2x1}
		H5Dclose(ds); 
	}
	/** @brief PARTIAL READ string data  

	 * h5dump -d string.mat string.h5
	 *
		HDF5 "string.h5" {
			DATASET "string.mat" {
				DATATYPE  H5T_STRING {
				  STRSIZE H5T_VARIABLE;
				  STRPAD H5T_STR_NULLTERM;
				  CSET H5T_CSET_ASCII;
				  CTYPE H5T_C_S1;
			   }
		   DATASPACE  SIMPLE { ( 6, 6 ) / ( 6, 6 ) }
			   DATA {
					(0,0): NULL, NULL, 				NULL, 						NULL, NULL, NULL,
					(1,0): NULL, NULL, 				NULL, 						NULL, NULL, NULL,
					(2,0): NULL, "QQzQRNlHIEZR", 	"hYbcNzzLshW", 				NULL, NULL, NULL,
					(3,0): NULL, "qWNudKSiWLaOLsG", "gZLLzXmtNNBJp", 			NULL, NULL, NULL,
					(4,0): NULL, "roDxKp", 			"XmBIjFcrdTLimmEYEtaTvO", 	NULL, NULL, NULL,
					(5,0): NULL, NULL, 				NULL, 						NULL, NULL, NULL
		   		}
			}
		}
	*/
	{
		std::vector<std::string> data = h5::read<std::vector<std::string>>(fd,"/string.mat", {2,1},{3,2} ); // partial write a {3x2} block to coordinates: {2x1}
		int j=0;
		std::cout<<"PARTIAL READ: \n\t";
		for(auto i:data ) std::cout <<"[" <<j++<<"] " << i << " ";
		std::cout<<std::endl;
	}

	H5Dclose(ds);
	h5::close(fd);
}


