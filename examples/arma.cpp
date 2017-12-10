#include <armadillo>
#include <h5cpp>

int main(){
	{ // CREATE - WRITE
		arma::mat M(4,5); 					// create a matrix
		hid_t fd = h5::create("arma.h5"); 	// and a file
			h5::write(fd,"arma/M",M); 		// save dataset
		h5::close(fd); 						// close fd
	}
	{ // CREATE - READ 
		hid_t fd = h5::open("arma.h5", H5F_ACC_RDWR); 			// you can have multiple fd open with H5F_ACC_RDONLY, but single write
			hid_t ds = h5::create<float>(fd,"dataset",{3,2});	// create dataset, default to NaN-s
			auto M = h5::read<arma::mat>( ds ); 				// read data back as matrix
			M.print();
		h5::close(fd); 											// close fd
	}
	{ // READ
		hid_t fd = h5::open("arma.h5", H5F_ACC_RDONLY); 	// you can have multiple fd open with H5F_ACC_RDONLY, but single write
		arma::mat M = h5::read<arma::mat>(fd,"arma/M"); 	// read entire edataset back, must have enough free memory
		h5::close(fd); 										// close fd
		M.print();
	}
}


